#pragma once

/*
    The waveform pictures Tracktion asks for (a clip's, and a recording's while
    it is made): the lowest and highest sample of every block, kept by
    encore-audio-processing's waveform picture (dsp/Waveform.h,
    vs::waveform::MinMax), read here for each pixel. A file is read on the
    thumbnail cache's background thread; a recording arrives through addBlock.
    Studio's UI behaviours hand these out from createAudioThumbnail, so every
    waveform Tracktion draws comes from here.
*/

#include "dsp/Waveform.h"

class WaveformPicture : public juce::AudioThumbnailBase,
                        private juce::TimeSliceClient
{
public:
    WaveformPicture (int samplesPerBlock, juce::AudioFormatManager& formatsToUse, juce::AudioThumbnailCache& cacheToUse)
        : blockSize (juce::jmax (1, samplesPerBlock)), formats (formatsToUse), cache (cacheToUse)
    {
        picture.reset (1, blockSize);
    }

    ~WaveformPicture() override { stopReading(); }

    void clear() override
    {
        stopReading();
        const juce::ScopedLock sl (lock);
        picture.reset (1, blockSize);
        numChannels = 0;
        rate = 0;
        total = 0;
        loaded = 0;
        hash = 0;
        source = nullptr;
        sendChangeMessage();
    }

    bool setSource (juce::InputSource* newSource) override
    {
        clear();
        if (newSource == nullptr) return false;
        source.reset (newSource);
        hash = source->hashCode();
        auto* stream = source->createInputStream();
        if (stream == nullptr) return false;
        std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor (std::unique_ptr<juce::InputStream> (stream)));
        if (r == nullptr) return false;
        startReading (std::move (r));
        return true;
    }

    void setReader (juce::AudioFormatReader* newReader, juce::int64 hashCode) override
    {
        clear();
        hash = hashCode;
        if (newReader != nullptr) startReading (std::unique_ptr<juce::AudioFormatReader> (newReader));
    }

    // The pictures are quick to make again, so nothing is cached between runs
    bool loadFrom (juce::InputStream&) override { return false; }
    void saveTo (juce::OutputStream&) const override {}

    int getNumChannels() const noexcept override { return numChannels; }
    double getTotalLength() const noexcept override { return rate > 0 ? (double) total / rate : 0.0; }
    bool isFullyLoaded() const noexcept override { return loaded >= total; }
    juce::int64 getNumSamplesFinished() const noexcept override { return loaded; }
    juce::int64 getHashCode() const override { return hash; }

    float getApproximatePeak() const override
    {
        const juce::ScopedLock sl (lock);
        return picture.peak();
    }

    void getApproximateMinMax (double startTime, double endTime, int channelIndex, float& minValue, float& maxValue) const noexcept override
    {
        const juce::ScopedLock sl (lock);
        const auto r = picture.range (channelIndex, (juce::int64) (startTime * rate), (juce::int64) (endTime * rate));
        minValue = r.lo;
        maxValue = r.hi;
    }

    void drawChannel (juce::Graphics& g, const juce::Rectangle<int>& area, double startTime, double endTime,
                      int channelNum, float verticalZoomFactor) override
    {
        if (area.isEmpty() || rate <= 0 || endTime <= startTime) return;
        const juce::ScopedLock sl (lock);
        const float mid = (float) area.getCentreY(), half = area.getHeight() * 0.5f * verticalZoomFactor;
        const double samplesPerPixel = (endTime - startTime) * rate / area.getWidth();
        const auto clip = g.getClipBounds().getIntersection (area);
        for (int x = clip.getX(); x < clip.getRight(); ++x)
        {
            const double from = startTime * rate + (x - area.getX()) * samplesPerPixel;
            const auto r = picture.range (channelNum, (juce::int64) from, (juce::int64) std::ceil (from + samplesPerPixel));
            if (r.lo == 0.0f && r.hi == 0.0f) continue;
            const float top = juce::jlimit ((float) area.getY(), (float) area.getBottom(), mid - r.hi * half);
            const float bottom = juce::jlimit ((float) area.getY(), (float) area.getBottom(), mid - r.lo * half);
            g.fillRect ((float) x, top, 1.0f, juce::jmax (1.0f, bottom - top));
        }
    }

    void drawChannels (juce::Graphics& g, const juce::Rectangle<int>& area, double startTime, double endTime, float verticalZoomFactor) override
    {
        const int n = juce::jmax (1, numChannels.load());
        for (int c = 0; c < n; ++c)
        {
            const int y0 = area.getY() + area.getHeight() * c / n, y1 = area.getY() + area.getHeight() * (c + 1) / n;
            drawChannel (g, { area.getX(), y0, area.getWidth(), y1 - y0 }, startTime, endTime, c, verticalZoomFactor);
        }
    }

    // A recording as it is made
    void reset (int newNumChannels, double sampleRate, juce::int64 totalSamplesInSource) override
    {
        stopReading();
        const juce::ScopedLock sl (lock);
        numChannels = newNumChannels;
        rate = sampleRate;
        total = totalSamplesInSource;
        loaded = 0;
        picture.reset (juce::jmax (1, numChannels.load()), blockSize, totalSamplesInSource);
    }

    void addBlock (juce::int64, const juce::AudioBuffer<float>& data, int startOffsetInBuffer, int numSamples) override
    {
        if (numSamples <= 0 || data.getNumChannels() == 0) return;
        {
            const juce::ScopedLock sl (lock);
            const float* ch[8] {};
            const int nch = juce::jmin (8, data.getNumChannels());
            for (int c = 0; c < nch; ++c) ch[c] = data.getReadPointer (c, startOffsetInBuffer);
            picture.add (ch, nch, numSamples);
            loaded += numSamples;
            total = juce::jmax (total.load(), loaded.load());
        }
        sendChangeMessage();
    }

private:
    const int blockSize;
    juce::AudioFormatManager& formats;
    juce::AudioThumbnailCache& cache;
    juce::CriticalSection lock;
    vs::waveform::MinMax picture;
    std::unique_ptr<juce::InputSource> source;
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> chunk;
    std::atomic<int> numChannels { 0 };
    std::atomic<double> rate { 0.0 };
    std::atomic<juce::int64> total { 0 }, loaded { 0 };
    juce::int64 hash = 0;

    void startReading (std::unique_ptr<juce::AudioFormatReader> r)
    {
        {
            const juce::ScopedLock sl (lock);
            numChannels = (int) r->numChannels;
            rate = r->sampleRate;
            total = r->lengthInSamples;
            loaded = 0;
            picture.reset (juce::jmax (1, (int) r->numChannels), blockSize, r->lengthInSamples);
        }
        reader = std::move (r);
        cache.getTimeSliceThread().addTimeSliceClient (this);
    }

    void stopReading()
    {
        cache.getTimeSliceThread().removeTimeSliceClient (this);
        reader = nullptr;
    }

    int useTimeSlice() override
    {
        if (reader == nullptr) return -1;
        const auto at = loaded.load();
        const int n = (int) juce::jmin ((juce::int64) 65536, total.load() - at);
        if (n <= 0) { reader = nullptr; return -1; }
        chunk.setSize ((int) reader->numChannels, n, false, false, true);
        reader->read (&chunk, 0, n, at, true, true);
        {
            const juce::ScopedLock sl (lock);
            picture.add (chunk.getArrayOfReadPointers(), chunk.getNumChannels(), n);
            loaded += n;
        }
        sendChangeMessage();
        return 0;
    }
};
