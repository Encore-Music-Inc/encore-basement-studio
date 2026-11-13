// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
//
// The effects and instruments Basement Studio offers beside the ENCORE.
// plugins: each is encore-audio-processing's (dsp/Chorus.h, Phaser.h,
// EqBand.h, TimeStretch.h, Compressor.h, Loudness.h's limiter, Space.h's delay,
// Synth.h, Sampler.h), hosted here as a plugin of the "Encore" format (see
// EncorePlugins.h) with its controls as parameters, so songs save and
// automate them like any plugin. These hosts only pass parameters and audio
// through: every sample is made by the headers.
#pragma once

#include <tuple>

#include "dsp/Chorus.h"
#include "dsp/Compressor.h"
#include "dsp/EqBand.h"
#include "dsp/Loudness.h"
#include "dsp/Mix.h"
#include "dsp/Phaser.h"
#include "dsp/Sampler.h"
#include "dsp/Space.h"
#include "dsp/Synth.h"
#include "dsp/TimeStretch.h"

namespace studiofx
{
    using APVTS = juce::AudioProcessorValueTreeState;
    using Layout = APVTS::ParameterLayout;

    /** What every host shares: stereo (or instrument) buses, its parameters, a
        plain editor of them, and its state saved as the parameters' values */
    class Host : public juce::AudioPluginInstance
    {
    public:
        Host (juce::String identifier, juce::String displayName, bool isInstrument, Layout layout)
            : juce::AudioPluginInstance (isInstrument ? BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                                      : BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                                                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
              apvts (*this, nullptr, "Studio", std::move (layout)), id (std::move (identifier)), name (std::move (displayName)), instrument (isInstrument)
        {}

        void fillInPluginDescription (juce::PluginDescription& d) const override
        {
            d.name = d.descriptiveName = name;
            d.pluginFormatName = "Encore";
            d.category = instrument ? "Instrument" : "Effect";
            d.manufacturerName = "ENCORE.";
            d.version = ProjectInfo::versionString;
            d.fileOrIdentifier = id;
            d.uniqueId = d.deprecatedUid = id.hashCode();
            d.isInstrument = instrument;
            d.numInputChannels = instrument ? 0 : 2;
            d.numOutputChannels = 2;
        }

        const juce::String getName() const override { return name; }
        bool acceptsMidi() const override { return instrument; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 2.0; }
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout& l) const override
        {
            return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
                && (instrument || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
        }

        juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
        bool hasEditor() const override { return true; }

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock& dest) override
        {
            auto state = apvts.copyState();
            saveExtra (state);
            if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
        }
        void setStateInformation (const void* data, int size) override
        {
            if (auto xml = getXmlFromBinary (data, size))
            {
                auto state = juce::ValueTree::fromXml (*xml);
                if (state.hasType (apvts.state.getType())) { apvts.replaceState (state); loadExtra (state); }
            }
        }

        APVTS apvts;

    protected:
        float value (const char* paramId) const { return apvts.getRawParameterValue (paramId)->load(); }
        virtual void saveExtra (juce::ValueTree&) {}
        virtual void loadExtra (const juce::ValueTree&) {}

        // The host's tempo (120 when it doesn't say)
        double tempo()
        {
            if (auto* ph = getPlayHead())
                if (auto pos = ph->getPosition())
                    if (auto b = pos->getBpm()) return *b;
            return 120.0;
        }

        static void addFloat (Layout& l, const char* paramId, const char* label, juce::NormalisableRange<float> range, float def, const char* unit = "")
        {
            l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { paramId, 1 }, label, range, def,
                                                                juce::AudioParameterFloatAttributes().withLabel (unit)));
        }
        static void addChoice (Layout& l, const char* paramId, const char* label, juce::StringArray choices, int def)
        {
            l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { paramId, 1 }, label, choices, def));
        }
        static juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
        {
            juce::NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r;
        }

        static float* const* channels (juce::AudioBuffer<float>& b) { return b.getArrayOfWritePointers(); }

        juce::String id, name;
        bool instrument;
    };

    //==============================================================================
    class Chorus : public Host
    {
    public:
        Chorus() : Host ("encore:chorus", "ENCORE. Chorus", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "depth", "Depth", { 0.0f, vs::Chorus::maxDepthMs, 0.01f }, 3.0f, "ms");
            addFloat (l, "speed", "Speed", skewed (0.05f, 10.0f, 1.0f), 1.0f, "Hz");
            addFloat (l, "width", "Width", { 0.0f, 1.0f }, 0.5f);
            addFloat (l, "mix", "Mix", { 0.0f, 1.0f }, 0.5f);
            return l;
        }
        void prepareToPlay (double fs, int) override { fx.prepare (fs); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            fx.setSettings ({ value ("depth"), value ("speed"), value ("width"), value ("mix") });
            fx.process (channels (b), juce::jmin (2, b.getNumChannels()), b.getNumSamples());
        }
    private:
        vs::Chorus fx;
    };

    class Phaser : public Host
    {
    public:
        Phaser() : Host ("encore:phaser", "ENCORE. Phaser", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "depth", "Depth", { 0.0f, 10.0f, 0.01f }, 5.0f, "oct");
            addFloat (l, "rate", "Rate", skewed (0.01f, 5.0f, 0.4f), 0.4f);
            addFloat (l, "feedback", "Feedback", { 0.0f, 0.95f }, 0.7f);
            return l;
        }
        void prepareToPlay (double fs, int) override { fx.prepare (fs); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            fx.setSettings ({ value ("depth"), value ("rate"), value ("feedback") });
            fx.process (channels (b), juce::jmin (2, b.getNumChannels()), b.getNumSamples());
        }
    private:
        vs::Phaser fx;
    };

    // A low- or high-pass filter: one EQ band (it glides to a new setting)
    class Filter : public Host
    {
    public:
        Filter() : Host ("encore:filter", "ENCORE. Filter", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addChoice (l, "mode", "Mode", { "Low-pass", "High-pass" }, 0);
            addFloat (l, "frequency", "Frequency", skewed (20.0f, 20000.0f, 1000.0f), 4000.0f, "Hz");
            addFloat (l, "q", "Resonance", skewed (0.5f, 10.0f, 1.0f), 0.7071f);
            addChoice (l, "slope", "Slope", { "12 dB/oct", "24 dB/oct", "48 dB/oct" }, 0);
            return l;
        }
        void prepareToPlay (double fs, int) override { fx.prepare (fs); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            vs::EqBand::Settings s;
            s.type = value ("mode") < 0.5f ? vs::EqBand::highCut : vs::EqBand::lowCut;
            s.freq = value ("frequency"); s.q = value ("q"); s.slope = (int) value ("slope");
            const int n = b.getNumSamples(), nc = juce::jmin (2, b.getNumChannels());
            for (int start = 0; start < n; start += 32)   // the band glides once per 32 samples
            {
                fx.setTarget (s);
                fx.process (channels (b), nc, start, juce::jmin (32, n - start));
            }
        }
    private:
        vs::EqBand fx;
    };

    class PitchShifter : public Host
    {
    public:
        PitchShifter() : Host ("encore:pitch", "ENCORE. Pitch Shifter", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "semitones", "Pitch", { -24.0f, 24.0f, 0.01f }, 0.0f, "st");
            return l;
        }
        void prepareToPlay (double fs, int block) override
        {
            fx.prepare (fs, 2, juce::jmax (block, 512));
            setLatencySamples (fx.latency());
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            fx.setSemitones (value ("semitones"));
            fx.process (channels (b), juce::jmin (2, b.getNumChannels()), b.getNumSamples());
        }
    private:
        vs::PitchShift fx;
    };

    class Compressor : public Host
    {
    public:
        Compressor() : Host ("encore:compressor", "ENCORE. Compressor", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "threshold", "Threshold", { -60.0f, 0.0f, 0.1f }, -18.0f, "dB");
            addFloat (l, "ratio", "Ratio", skewed (1.0f, 20.0f, 4.0f), 3.0f, ":1");
            addFloat (l, "attack", "Attack", skewed (0.1f, 100.0f, 10.0f), 10.0f, "ms");
            addFloat (l, "release", "Release", skewed (10.0f, 1000.0f, 120.0f), 120.0f, "ms");
            addFloat (l, "makeup", "Makeup", { 0.0f, 24.0f, 0.1f }, 0.0f, "dB");
            addChoice (l, "detect", "Detection", { "Peak", "RMS" }, 0);
            return l;
        }
        void prepareToPlay (double fs, int) override { fx.prepare (fs); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            fx.setDetail ({ value ("threshold"), value ("ratio"), value ("attack"), value ("release"), value ("makeup"), value ("detect") > 0.5f });
            fx.process (channels (b), juce::jmin (2, b.getNumChannels()), b.getNumSamples());
        }
    private:
        vs::Compressor fx;
    };

    class Limiter : public Host
    {
    public:
        Limiter() : Host ("encore:limiter", "ENCORE. Limiter", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "ceiling", "Ceiling", { -12.0f, 0.0f, 0.1f }, -1.0f, "dBTP");
            addFloat (l, "release", "Release", skewed (10.0f, 1000.0f, 100.0f), 100.0f, "ms");
            return l;
        }
        void prepareToPlay (double fs, int block) override
        {
            rate = fs;
            fx.prepare (fs, juce::jmax (block, 512));
            setLatencySamples (fx.getLatency());
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            fx.setCeilingDb (value ("ceiling"));
            fx.setReleaseMs (value ("release"), rate);
            juce::dsp::AudioBlock<float> block (b.getArrayOfWritePointers(), (size_t) juce::jmin (2, b.getNumChannels()), (size_t) b.getNumSamples());
            fx.process (block);
        }
    private:
        vs::TruePeakLimiter fx;
        double rate = 48000.0;
    };

    // The delay: Space's (the reverb off), its time a note at the song's tempo
    class Delay : public Host
    {
    public:
        Delay() : Host ("encore:delay", "ENCORE. Delay", false, layout()) {}
        static Layout layout()
        {
            Layout l;
            addChoice (l, "time", "Time", { "Slap", "1/16", "1/8", "Dotted 1/8", "1/4", "1/2" }, (int) vs::Space::eighth);
            addFloat (l, "amount", "Amount", { 0.0f, 1.0f }, 0.35f);
            addFloat (l, "feedback", "Feedback", { 0.0f, 0.9f }, 0.35f);
            return l;
        }
        void prepareToPlay (double fs, int block) override { fx.prepare ({ fs, (juce::uint32) juce::jmax (block, 512), 2 }); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            if (b.getNumChannels() < 2) return;
            fx.setParams (0.0f, 0.5f, value ("amount"), vs::Space::delaySeconds ((int) value ("time"), tempo()), value ("feedback"));
            fx.process (channels (b), b.getNumSamples(), true);
        }
    private:
        vs::Space fx;
    };

    //==============================================================================
    class Synth : public Host
    {
    public:
        Synth() : Host ("encore:synth", "ENCORE. Synth", true, layout()) {}
        static Layout layout()
        {
            Layout l;
            const juce::StringArray waves { "Sine", "Triangle", "Saw up", "Saw down", "Pulse", "Noise" };
            for (int o = 0; o < vs::Synth::numOscillators; ++o)
            {
                const auto p = "osc" + juce::String (o + 1) + "_", n = "Osc " + juce::String (o + 1) + " ";
                l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { p + "on", 1 }, n + "On", o == 0));
                l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { p + "wave", 1 }, n + "Wave", waves, 2));
                l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { p + "semis", 1 }, n + "Tune", -36, 36, 0));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "cents", 1 }, n + "Fine", juce::NormalisableRange<float> (-100.0f, 100.0f), 0.0f));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "level", 1 }, n + "Level", juce::NormalisableRange<float> (-60.0f, 0.0f), -6.0f));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "pw", 1 }, n + "Pulse width", juce::NormalisableRange<float> (0.05f, 0.95f), 0.5f));
                l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { p + "voices", 1 }, n + "Voices", 1, vs::Synth::maxUnison, 1));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "detune", 1 }, n + "Detune", juce::NormalisableRange<float> (0.0f, 100.0f), 10.0f));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "spread", 1 }, n + "Spread", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { p + "pan", 1 }, n + "Pan", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
            }
            addChoice (l, "filter_type", "Filter", { "Off", "Low-pass", "Band-pass", "High-pass" }, 1);
            addChoice (l, "filter_slope", "Filter slope", { "12 dB/oct", "24 dB/oct" }, 0);
            addFloat (l, "filter_freq", "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 8000.0f, "Hz");
            addFloat (l, "filter_res", "Resonance", skewed (0.5f, 10.0f, 1.0f), 0.7f);
            addFloat (l, "filter_key", "Key tracking", { 0.0f, 1.0f }, 0.0f);
            addFloat (l, "filter_env", "Filter envelope", { -60.0f, 60.0f }, 0.0f, "st");
            addFloat (l, "filter_vel", "Filter velocity", { 0.0f, 1.0f }, 0.0f);
            for (auto [p, n, a, d, s, r] : { std::tuple { "amp_", "Amp ", 0.005f, 0.3f, 0.8f, 0.3f }, std::tuple { "fenv_", "Filter env ", 0.005f, 0.3f, 0.0f, 0.3f } })
            {
                const juce::String ps (p), ns (n);
                addFloatS (l, ps + "a", ns + "attack", skewed (0.001f, 10.0f, 0.3f), a, "s");
                addFloatS (l, ps + "d", ns + "decay", skewed (0.001f, 10.0f, 0.5f), d, "s");
                addFloatS (l, ps + "s", ns + "sustain", { 0.0f, 1.0f }, s, "");
                addFloatS (l, ps + "r", ns + "release", skewed (0.001f, 10.0f, 0.5f), r, "s");
            }
            for (int k = 0; k < vs::Synth::numLfos; ++k)
            {
                const auto p = "lfo" + juce::String (k + 1) + "_", n = "LFO " + juce::String (k + 1) + " ";
                l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { p + "wave", 1 }, n + "Wave", waves, 0));
                addFloatS (l, p + "rate", n + "Rate", skewed (0.05f, 20.0f, 3.0f), 5.0f, "Hz");
                addFloatS (l, p + "depth", n + "Depth", { 0.0f, 24.0f }, 0.0f, "");
                l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { p + "target", 1 }, n + "To",
                                                                     juce::StringArray { "Nothing", "Pitch", "Filter", "Level", "Pulse width" }, 0));
            }
            addChoice (l, "mode", "Play", { "Poly", "Mono", "Legato" }, 0);
            l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "voices", 1 }, "Voices", 1, vs::Synth::maxVoices, 8));
            addFloat (l, "glide", "Glide", skewed (0.0f, 2.0f, 0.2f), 0.0f, "s");
            addFloat (l, "velocity", "Velocity to level", { 0.0f, 1.0f }, 1.0f);
            addFloat (l, "level", "Level", { -60.0f, 6.0f, 0.1f }, -6.0f, "dB");
            return l;
        }

        void prepareToPlay (double fs, int) override { synth.prepare (fs); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi) override
        {
            synth.setSettings (settings());
            b.clear();
            if (b.getNumChannels() < 2) return;
            float* l = b.getWritePointer (0);
            float* r = b.getWritePointer (1);
            int at = 0;
            for (const auto m : midi)
            {
                const int when = juce::jlimit (0, b.getNumSamples(), m.samplePosition);
                if (when > at) { synth.render (l + at, r + at, when - at); at = when; }
                const auto msg = m.getMessage();
                if (msg.isNoteOn()) synth.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff()) synth.noteOff (msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff()) synth.allNotesOff (msg.isAllSoundOff());
                else if (msg.isPitchWheel()) synth.setPitchBend ((float) ((msg.getPitchWheelValue() - 8192) / 8192.0 * 2.0));
            }
            if (b.getNumSamples() > at) synth.render (l + at, r + at, b.getNumSamples() - at);
        }

    private:
        static void addFloatS (Layout& l, const juce::String& paramId, const juce::String& label, juce::NormalisableRange<float> range, float def, const char* unit)
        {
            l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { paramId, 1 }, label, range, def, juce::AudioParameterFloatAttributes().withLabel (unit)));
        }
        float v (const juce::String& p) const { return apvts.getRawParameterValue (p)->load(); }

        vs::Synth::Settings settings() const
        {
            vs::Synth::Settings s;
            for (int o = 0; o < vs::Synth::numOscillators; ++o)
            {
                const auto p = "osc" + juce::String (o + 1) + "_";
                auto& x = s.osc[(size_t) o];
                x.on = v (p + "on") > 0.5f; x.wave = (vs::Oscillator::Wave) (int) v (p + "wave"); x.semitones = (int) v (p + "semis");
                x.cents = v (p + "cents"); x.levelDb = v (p + "level"); x.pulseWidth = v (p + "pw"); x.unison = (int) v (p + "voices");
                x.detuneCents = v (p + "detune"); x.spread = v (p + "spread"); x.pan = v (p + "pan");
            }
            s.filter = { (vs::Synth::FilterType) (int) v ("filter_type"), v ("filter_slope") > 0.5f, v ("filter_freq"), v ("filter_res"),
                         v ("filter_key"), v ("filter_env"), v ("filter_vel") };
            s.ampEnv = { v ("amp_a"), v ("amp_d"), v ("amp_s"), v ("amp_r") };
            s.filterEnv = { v ("fenv_a"), v ("fenv_d"), v ("fenv_s"), v ("fenv_r") };
            for (int k = 0; k < vs::Synth::numLfos; ++k)
            {
                const auto p = "lfo" + juce::String (k + 1) + "_";
                s.lfo[(size_t) k] = { (vs::Oscillator::Wave) (int) v (p + "wave"), v (p + "rate"), v (p + "depth"), (vs::Synth::LfoTarget) (int) v (p + "target") };
            }
            s.mode = (vs::Synth::VoiceMode) (int) v ("mode"); s.voices = (int) v ("voices"); s.glideSeconds = v ("glide");
            s.velocityToLevel = v ("velocity"); s.levelDb = v ("level");
            return s;
        }

        vs::Synth synth;
    };

    //==============================================================================
    // The sampler: sounds from files, each over a range of notes; a drum kit
    // puts one sound on each drum's note. The sounds are saved with the song
    // as their files and settings, and read again when it opens.
    class Sampler : public Host
    {
    public:
        Sampler() : Host ("encore:sampler", "ENCORE. Sampler", true, layout()) {}
        static Layout layout()
        {
            Layout l;
            addFloat (l, "level", "Level", { -60.0f, 6.0f, 0.1f }, 0.0f, "dB");
            return l;
        }

        struct SoundFile { juce::File file; juce::String name; int root = 60, low = 60, high = 60; float gainDb = 0, pan = 0; bool oneShot = true; int choke = 0; };

        /** Sets the sounds (reads each file; call it off the audio thread) */
        void setSounds (const std::vector<SoundFile>& list)
        {
            const juce::ScopedLock sl (getCallbackLock());
            files = list;
            sampler.clearSounds();
            juce::AudioFormatManager fm; fm.registerBasicFormats();
            for (const auto& f : files)
            {
                std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f.file));
                if (r == nullptr) continue;
                juce::AudioBuffer<float> buf ((int) r->numChannels, (int) r->lengthInSamples);
                r->read (&buf, 0, buf.getNumSamples(), 0, true, true);
                vs::Sampler::Sound s;
                s.left.assign (buf.getReadPointer (0), buf.getReadPointer (0) + buf.getNumSamples());
                if (buf.getNumChannels() > 1) s.right.assign (buf.getReadPointer (1), buf.getReadPointer (1) + buf.getNumSamples());
                s.sampleRate = r->sampleRate; s.rootNote = f.root; s.lowNote = f.low; s.highNote = f.high;
                s.gainDb = f.gainDb; s.pan = f.pan; s.oneShot = f.oneShot; s.choke = f.choke;
                sampler.addSound (std::move (s));
            }
        }
        const std::vector<SoundFile>& getSounds() const { return files; }

        void prepareToPlay (double fs, int) override { sampler.prepare (fs); gain.reset (fs, 0.05); }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi) override
        {
            b.clear();
            if (b.getNumChannels() < 2) return;
            float* l = b.getWritePointer (0);
            float* r = b.getWritePointer (1);
            int at = 0;
            for (const auto m : midi)
            {
                const int when = juce::jlimit (0, b.getNumSamples(), m.samplePosition);
                if (when > at) { sampler.render (l + at, r + at, when - at); at = when; }
                const auto msg = m.getMessage();
                if (msg.isNoteOn()) sampler.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff()) sampler.noteOff (msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff()) sampler.allNotesOff (msg.isAllSoundOff());
            }
            if (b.getNumSamples() > at) sampler.render (l + at, r + at, b.getNumSamples() - at);
            gain.setTarget (vs::dbToGain (value ("level")));
            vs::mix::applyGain (gain, b.getArrayOfWritePointers(), 2, b.getNumSamples());
        }

    protected:
        void saveExtra (juce::ValueTree& state) override
        {
            juce::ValueTree sounds ("Sounds");
            for (const auto& f : files)
            {
                juce::ValueTree s ("Sound");
                s.setProperty ("file", f.file.getFullPathName(), nullptr); s.setProperty ("name", f.name, nullptr);
                s.setProperty ("root", f.root, nullptr); s.setProperty ("low", f.low, nullptr); s.setProperty ("high", f.high, nullptr);
                s.setProperty ("gain", f.gainDb, nullptr); s.setProperty ("pan", f.pan, nullptr);
                s.setProperty ("oneShot", f.oneShot, nullptr); s.setProperty ("choke", f.choke, nullptr);
                sounds.appendChild (s, nullptr);
            }
            state.removeChild (state.getChildWithName ("Sounds"), nullptr);
            state.appendChild (sounds, nullptr);
        }
        void loadExtra (const juce::ValueTree& state) override
        {
            std::vector<SoundFile> list;
            for (const auto s : state.getChildWithName ("Sounds"))
                list.push_back ({ juce::File (s["file"].toString()), s["name"].toString(), (int) s["root"], (int) s["low"], (int) s["high"],
                                  (float) s["gain"], (float) s["pan"], (bool) s["oneShot"], (int) s["choke"] });
            setSounds (list);
        }

    private:
        vs::Sampler sampler;
        std::vector<SoundFile> files;
        vs::Ramp<true> gain;
    };
}
