// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

#pragma once

// Finishing an export: the rendered mix (a float WAV from Tracktion) can go
// through ENCORE. Master's chain (one of its styles, a loudness target and how
// hard it works) or be normalized to streaming loudness, and is then written at
// the bit depth asked for. All of that is encore-audio-processing's: the one
// mastering driver (dsp/Mastering.h), the one loudness normalizer
// (dsp/Channels.h) and the one float to integer conversion, with dither
// (dsp/Dither.h). The song itself never changes. Plus a short before/after
// preview: two renders of the same 30 s, played in sync so you can flip
// between them at the same loudness.

#include "dsp/Mastering.h"
#include "dsp/AbCompare.h"
#include "dsp/Channels.h"
#include "dsp/Dither.h"

namespace mastering
{
    using MC = vs::MasterChain;

    struct Settings
    {
        int style = -1;            // ENCORE. Master's style (Balanced, Heavy...), or -1 for no mastering
        float intensity = 0.5f;    // 0..1, 0.5 = the style as fitted
        int target = 0;            // an index into targetNames
        bool on() const { return style >= 0; }
    };

    // The styles offered: the ones fitted to measured reference masters (the
    // first ones in encore-audio-processing's list), with its names and blurbs
    constexpr int offeredStyles = 4;
    inline juce::StringArray styleNames()
    {
        juce::StringArray s;
        for (int i = 0; i < offeredStyles; ++i) s.add (MC::style (i).name);
        return s;
    }
    inline juce::StringArray styleBlurbs()
    {
        juce::StringArray s;
        for (int i = 0; i < offeredStyles; ++i) s.add (MC::style (i).blurb);
        return s;
    }
    /** The loudness targets offered: encore-audio-processing's (MasterChain's targets), named here */
    inline juce::StringArray targetNames()
    {
        const char* const words[] { "Streaming", "Balanced", "Loud" };
        juce::StringArray s;
        for (int i = 0; i < MC::numTargets; ++i) s.add (juce::String (words[i]) + " (" + juce::String (MC::targetLufs (i), 0) + " LUFS)");
        return s;
    }
    inline float targetLufs (int target) { return MC::targetLufs (target); }

    inline bool isEncoreMaster (te::Plugin& p)
    {
        auto* ep = dynamic_cast<te::ExternalPlugin*> (&p);
        auto* entry = ep != nullptr ? EncorePluginFormat::find (ep->desc.fileOrIdentifier) : nullptr;
        return entry != nullptr && juce::String (entry->id) == "encore:master";
    }

    /** Masters a mix in place with encore-audio-processing's one mastering driver
        (vs::Mastering): it learns on the loudest 30 s, then renders the whole song
        until it is on the loudness target. A mono mix comes out stereo. */
    inline void masterBuffer (juce::AudioBuffer<float>& audio, double fs, const Settings& m)
    {
        const int len = audio.getNumSamples();
        if (len == 0 || audio.getNumChannels() < 1) return;
        MC::Settings s;
        s.style = juce::jlimit (0, MC::numStyles - 1, m.style);
        s.intensity = juce::jlimit (0.0f, 1.0f, m.intensity);
        s.targetLufs = targetLufs (m.target);

        const float* l = audio.getReadPointer (0);
        const float* r = audio.getNumChannels() > 1 ? audio.getReadPointer (1) : nullptr;
        constexpr int feed = vs::Mastering::block;
        vs::Mastering driver;
        driver.begin (fs, s, len);
        for (int i = 0; i < len; i += feed)
            driver.feedEnergy (l + i, r != nullptr ? r + i : nullptr, juce::jmin (feed, len - i));
        const auto w = driver.learningWindow();
        for (long long i = 0; i < w.length; i += feed)
        {
            const int at = (int) (w.from + i);
            driver.feedWindow (l + at, r != nullptr ? r + at : nullptr, (int) juce::jmin ((long long) feed, w.length - i));
        }
        driver.learn();

        juce::AudioBuffer<float> out (2, len);
        std::vector<float> ol ((size_t) (vs::Mastering::maxRenderOutput (feed) + 2 * feed)), orr (ol.size());
        do
        {
            driver.beginRender();
            ol.resize ((size_t) (vs::Mastering::maxRenderOutput (feed) + driver.maxFinishOutput()));
            orr.resize (ol.size());
            int o = 0;
            auto take = [&] (int k)
            {
                const int keep = juce::jmin (k, len - o);
                if (keep > 0) { out.copyFrom (0, o, ol.data(), keep); out.copyFrom (1, o, orr.data(), keep); }
                o += k;
            };
            for (int i = 0; i < len; i += feed)
                take (driver.render (l + i, r != nullptr ? r + i : nullptr, juce::jmin (feed, len - i), ol.data(), orr.data()));
            take (driver.finishRender (ol.data(), orr.data()));
        }
        while (driver.nextPass());
        audio = std::move (out);
    }

    /** Reads a rendered mix (a float WAV), masters it or normalizes it to -14 LUFS
        when asked, and writes it out: WAV at the bit depth asked for (16 and 24
        quantized with dither, 32 as float) or FLAC (16 or 24, with dither).
        Safe to run off the message thread. Returns an empty string, or what went wrong. */
    inline juce::String finishFile (const juce::File& source, const juce::File& dest, const Settings& m, bool loudness,
                                    int bitDepth, bool flac)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (source));
        if (reader == nullptr) return "couldn't read the rendered mix back";
        const double fs = reader->sampleRate;
        juce::AudioBuffer<float> audio ((int) juce::jmax (1u, reader->numChannels), (int) reader->lengthInSamples);
        reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
        reader = nullptr;
        if (m.on()) masterBuffer (audio, fs, m);
        else if (loudness) vs::channels::normalize (audio.getArrayOfWritePointers(), juce::jmin (2, audio.getNumChannels()), audio.getNumSamples(), fs,
                                                     vs::channels::Normalize::lufs);

        const int bits = flac ? juce::jmin (24, bitDepth) : bitDepth;
        const bool asFloat = ! flac && bits == 32;
        dest.deleteFile();
        std::unique_ptr<juce::OutputStream> stream (dest.createOutputStream().release());
        if (stream == nullptr) return "couldn't write " + dest.getFullPathName();
        juce::WavAudioFormat wav;
        juce::FlacAudioFormat flacFormat;
        juce::AudioFormat& format = flac ? (juce::AudioFormat&) flacFormat : (juce::AudioFormat&) wav;
        const int numCh = audio.getNumChannels();
        auto options = juce::AudioFormatWriterOptions().withSampleRate (fs).withNumChannels (numCh).withBitsPerSample (bits)
                           .withSampleFormat (asFloat ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                                      : juce::AudioFormatWriterOptions::SampleFormat::integral);
        auto writer = format.createWriterFor (stream, options);
        if (writer == nullptr) return "couldn't write that format";
        if (asFloat) return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()) ? juce::String() : "the write didn't finish";

        // 16 and 24 bits: encore-audio-processing's quantizer (dither when the
        // audio isn't already on the grid); JUCE's writers take whole 32-bit
        // integers, so each value moves up to the top bits
        vs::Quantizer quantizer (bits);
        constexpr int block = 8192;
        std::vector<std::vector<std::int32_t>> ints ((size_t) numCh, std::vector<std::int32_t> ((size_t) block));
        std::vector<std::int32_t*> outPtr ((size_t) numCh);
        std::vector<const int*> writePtr ((size_t) numCh + 1, nullptr);
        for (int c = 0; c < numCh; ++c) { outPtr[(size_t) c] = ints[(size_t) c].data(); writePtr[(size_t) c] = ints[(size_t) c].data(); }
        std::vector<const float*> in ((size_t) numCh);
        for (int at = 0; at < audio.getNumSamples(); at += block)
        {
            const int n = juce::jmin (block, audio.getNumSamples() - at);
            for (int c = 0; c < numCh; ++c) in[(size_t) c] = audio.getReadPointer (c, at);
            quantizer.process (in.data(), numCh, n, outPtr.data());
            for (auto& ch : ints)
                for (int i = 0; i < n; ++i) ch[(size_t) i] = (std::int32_t) ((std::uint32_t) ch[(size_t) i] << (32 - bits));
            if (! writer->write (writePtr.data(), n)) return "the write didn't finish";
        }
        return {};
    }

    //==============================================================================
    /** Plays two equal-length renders in sync through the audio interface (alongside
        Tracktion, which is stopped meanwhile), switching between them without a gap */
    class PreviewPlayer : public juce::AudioIODeviceCallback
    {
    public:
        void load (juce::AudioBuffer<float> mastered, juce::AudioBuffer<float> original, double rate)
        {
            const juce::ScopedLock sl (lock);
            a = std::move (mastered);
            b = std::move (original);
            fileRate = rate;
            pos = 0;
            // how much louder the mastered one is (integrated loudness, encore-audio-processing's
            // BS.1770 meter), for a fair "same loudness" comparison
            auto lufs = [rate] (const juce::AudioBuffer<float>& x)
            {
                return vs::channels::loudnessOf (x.getArrayOfReadPointers(), juce::jmin (2, x.getNumChannels()), x.getNumSamples(), rate);
            };
            const float la = lufs (a), lb = lufs (b);
            const bool measurable = la > -70.0f && lb > -70.0f;
            matchDb = measurable ? lb - la : 0.0f;
            loudnessDifferenceDb = measurable ? la - lb : 0.0f;
            prepareCompare();
        }

        bool hasAudio() const { const juce::ScopedLock sl (lock); return a.getNumSamples() > 0; }
        float loudnessDifferenceDb = 0.0f;
        std::atomic<bool> playing { false }, original { false }, sameLoudness { false };

        void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* out, int numOut, int n,
                                               const juce::AudioIODeviceCallbackContext&) override
        {
            // JUCE mixes this into Tracktion's output, so write every sample (silence when not previewing)
            for (int c = 0; c < numOut; ++c)
                if (out[c] != nullptr) juce::FloatVectorOperations::clear (out[c], n);
            const juce::ScopedTryLock sl (lock);
            if (! sl.isLocked() || ! playing || a.getNumSamples() == 0 || std::abs (fileRate - deviceRate) > 1.0) return;
            // the main pair (outputs 1 and 2): both renders read in step, encore-audio-processing's
            // A/B (dsp/AbCompare.h) plays one, crossfading when the choice changes, with the
            // mastered one at the original's loudness when "same loudness" is on
            if ((int) work[0].size() < n) return;   // a block longer than the device said it would send
            for (int i = 0; i < n; ++i)
            {
                if (pos >= a.getNumSamples()) pos = 0;   // loops
                for (int c = 0; c < 2; ++c)
                {
                    work[c][(size_t) i] = a.getSample (juce::jmin (c, a.getNumChannels() - 1), pos);
                    orig[c][(size_t) i] = pos < b.getNumSamples() ? b.getSample (juce::jmin (c, b.getNumChannels() - 1), pos) : 0.0f;
                }
                ++pos;
            }
            if (sameLoudness) compare.setFixedMatch (matchDb); else compare.setNoMatch();
            compare.setHearOriginal (original);
            compare.takeOriginal (orig[0].data(), orig[1].data(), n);
            compare.mix (work[0].data(), work[1].data(), n);
            for (int c = 0; c < juce::jmin (2, numOut); ++c)
                if (out[c] != nullptr) juce::FloatVectorOperations::copy (out[c], work[c].data(), n);
        }
        void audioDeviceAboutToStart (juce::AudioIODevice* d) override
        {
            const juce::ScopedLock sl (lock);
            deviceRate = d->getCurrentSampleRate();
            maxBlock = juce::jmax (maxBlock, d->getCurrentBufferSizeSamples());
            prepareCompare();
        }
        void audioDeviceStopped() override {}

    private:
        juce::CriticalSection lock;
        juce::AudioBuffer<float> a, b;
        double fileRate = 0, deviceRate = 0;
        int pos = 0, maxBlock = 4096;
        float matchDb = 0.0f;
        vs::AbCompare compare;
        std::vector<float> work[2], orig[2];

        void prepareCompare()   // with the lock held
        {
            for (auto* v : { &work[0], &work[1], &orig[0], &orig[1] }) v->assign ((size_t) maxBlock, 0.0f);
            compare.setFixedMatch (matchDb);
            compare.prepare (fileRate > 0 ? fileRate : 48000.0, maxBlock, 0);
        }
    };
}
