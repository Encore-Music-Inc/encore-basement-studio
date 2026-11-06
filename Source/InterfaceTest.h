// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

#pragma once
// --interfacetest <word> <dir>: the live side of the app on a REAL interface (the word picks it, like "Volt"), through
// its ASIO driver and through Windows audio. Every block is measured at the very end of the engine and then blanked,
// so nothing reaches the speakers, even with a hot mic and monitoring on. Its settings are its own profile, and
// recordings stay in <dir>. Results go to <dir>/interfacetest.log.
namespace interfacetest
{
    static constexpr int maxChannels = 16;

    /** Peak and power per channel, written on the audio thread and read off on the message thread */
    struct Levels
    {
        void add (const float* const* data, int channels, int samples)
        {
            for (int c = 0; c < juce::jmin (channels, maxChannels); ++c)
            {
                if (data[c] == nullptr) continue;
                float p = 0; double e = 0;
                for (int i = 0; i < samples; ++i) { p = juce::jmax (p, std::abs (data[c][i])); e += data[c][i] * data[c][i]; }
                if (p > peak[c].load()) peak[c].store (p);
                energy[c].store (energy[c].load() + e);
            }
            count.fetch_add (samples);
            ++blocks;
        }
        void reset() { for (auto& p : peak) p = 0.0f; for (auto& e : energy) e = 0.0; count = 0; blocks = 0; }
        float peakOf (int c) const { return peak[c].load(); }
        float rmsOf (int c) const { return count.load() > 0 ? (float) std::sqrt (energy[c].load() / (double) count.load()) : 0.0f; }
        float loudest() const { float m = 0; for (auto& p : peak) m = juce::jmax (m, p.load()); return m; }

        std::atomic<float> peak[maxChannels] {};
        std::atomic<double> energy[maxChannels] {};
        std::atomic<juce::int64> count { 0 };
        std::atomic<int> blocks { 0 };
    };

    /** Sits at the very end of the engine: measures what would go to the speakers, then blanks it */
    struct OutputTap : public juce::AudioProcessor
    {
        explicit OutputTap (Levels& l) : levels (l) {}
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            levels.add (b.getArrayOfReadPointers(), b.getNumChannels(), b.getNumSamples());
            b.clear();
        }
        const juce::String getName() const override { return "output tap"; }
        void prepareToPlay (double, int) override {}
        void releaseResources() override {}
        double getTailLengthSeconds() const override { return 0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}
        Levels& levels;
    };

    /** A second listener on the driver: the inputs exactly as they arrive, before the engine has touched them */
    struct InputTap : public juce::AudioIODeviceCallback
    {
        explicit InputTap (Levels& l) : levels (l) {}
        void audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out, int numOut, int samples,
                                               const juce::AudioIODeviceCallbackContext&) override
        {
            levels.add (in, numIn, samples);
            for (int c = 0; c < numOut; ++c) if (out[c] != nullptr) juce::FloatVectorOperations::clear (out[c], samples);
        }
        void audioDeviceAboutToStart (juce::AudioIODevice*) override {}
        void audioDeviceStopped() override {}
        Levels& levels;
    };

    /** The engine's warnings land in the log, not in a dialog nobody's there to click */
    struct TestUI : public HeadlessUIBehaviour
    {
        void showWarningMessage (const juce::String& m) override { HeadlessUIBehaviour::showWarningMessage (m); if (onWarning) onWarning (m); }
        void showWarningAlert (const juce::String& t, const juce::String& m) override { if (onWarning) onWarning (t + ": " + m); }
        std::function<void (const juce::String&)> onWarning;
    };

    inline int run (const juce::String& word, const juce::File& dir)
    {
        using namespace juce;
        int failures = 0;
        dir.createDirectory();
        auto out = dir.getChildFile ("interfacetest.log");
        out.replaceWithText ({});
        auto note = [&] (const String& s) { out.appendText (s + "\n"); };
        auto expect = [&] (const String& name, bool ok, const String& detail = {})
        {
            note ((ok ? "PASS " : "FAIL ") + name + (detail.isEmpty() ? String() : "  " + detail));
            if (! ok) ++failures;
        };
        auto pump = [] (double seconds) { MessageManager::getInstance()->runDispatchLoopUntil ((int) (seconds * 1000)); };
        auto dB = [] (float v) { return String (Decibels::gainToDecibels (v, -160.0f), 1) + " dB"; };

        auto* behaviour = new StudioBehaviour (true);
        auto* ui = new TestUI();
        ui->onWarning = [&] (const String& m) { note ("  engine says: " + m); };
        te::Engine engine { "Encore Studio Interfacetest", std::unique_ptr<te::UIBehaviour> (ui), std::unique_ptr<te::EngineBehaviour> (behaviour) };
        auto& dm = engine.getDeviceManager();
        Levels heardIn, heardOut;
        dm.setGlobalOutputAudioProcessor (std::make_unique<OutputTap> (heardOut));   // before anything opens
        InputTap inputTap (heardIn);
        dm.initialise();
        pump (0.5);

        auto setupInputs = [&]
        {
            for (int pass = 0; pass < 8; ++pass)
            {
                Array<te::WaveInputDevice*> pairs;
                for (int i = 0; i < dm.getNumWaveInDevices(); ++i)
                    if (auto w = dm.getWaveInDevice (i)) if (! w->isTrackDevice() && w->isStereoPair()) pairs.add (w);
                if (pairs.isEmpty()) break;
                for (auto* w : pairs) dm.setDeviceNumChannels (*w, 1);
                pump (0.3);
            }
            StringArray names;
            for (int i = 0; i < dm.getNumWaveInDevices(); ++i)
                if (auto w = dm.getWaveInDevice (i))
                {
                    w->setEnabled (true);
                    if (w->getMonitorMode() == te::InputDevice::MonitorMode::automatic) w->setMonitorMode (te::InputDevice::MonitorMode::off);   // as MainComponent::setupInputs
                    names.add (w->getName());
                }
            return names;
        };

        auto driverSection = [&] (const String& label, const String& type, const String& inName, const String& outName)
        {
            note ("");
            note ("===== " + label + ": " + type + ", in \"" + inName + "\", out \"" + outName + "\"");
            auto& jm = dm.deviceManager;
            jm.removeAudioCallback (&inputTap);
            XmlElement want ("DEVICESETUP");
            want.setAttribute ("deviceType", type);
            want.setAttribute ("audioOutputDeviceName", outName);
            want.setAttribute ("audioInputDeviceName", inName);
            auto err = jm.initialise (512, 512, &want, false);
            pump (1.0);
            auto* d = jm.getCurrentAudioDevice();
            expect (label + ": the driver opens", err.isEmpty() && d != nullptr, err);
            if (d == nullptr) return;
            jm.addAudioCallback (&inputTap);
            const double startRate = d->getCurrentSampleRate();
            const int startBlock = d->getCurrentBufferSizeSamples();
            note ("device: " + d->getTypeName() + " / " + d->getName() + ", rate " + String (startRate) + ", block " + String (startBlock)
                    + ", " + String (d->getDefaultBufferSize()) + " default, in latency " + String (d->getInputLatencyInSamples())
                    + ", out latency " + String (d->getOutputLatencyInSamples()));
            note ("input channels: " + d->getInputChannelNames().joinIntoString (", ") + " (active " + String (d->getActiveInputChannels().countNumberOfSetBits()) + ")");
            note ("output channels: " + d->getOutputChannelNames().joinIntoString (", ") + " (active " + String (d->getActiveOutputChannels().countNumberOfSetBits()) + ")");
            note ("rates: " + [&] { StringArray s; for (auto r : d->getAvailableSampleRates()) s.add (String (r, 0)); return s.joinIntoString (", "); }());
            note ("block sizes: " + [&] { StringArray s; for (auto b : d->getAvailableBufferSizes()) s.add (String (b)); return s.joinIntoString (", "); }());

            auto inputs = setupInputs();
            note ("inputs: " + inputs.joinIntoString (", "));
            expect (label + ": the interface's inputs show up", inputs.size() >= 2, String (inputs.size()));

            // the driver delivers audio, in time, and what's on each input
            heardIn.reset();
            const auto t0 = Time::getMillisecondCounterHiRes();
            pump (3.0);
            const auto seconds = (Time::getMillisecondCounterHiRes() - t0) / 1000.0;
            const auto xrunsAtStart = d->getXRunCount();
            const auto got = heardIn.count.load() / startRate;
            expect (label + ": audio arrives on time", std::abs (got - seconds) < 0.25, String (got, 2) + " s in " + String (seconds, 2) + " s, xruns " + String (xrunsAtStart));
            const int numIn = jmin (inputs.size(), d->getActiveInputChannels().countNumberOfSetBits(), maxChannels);
            float inputPeak[maxChannels] {};
            for (int c = 0; c < numIn; ++c)
            {
                inputPeak[c] = heardIn.peakOf (c);
                note ("  raw input " + String (c + 1) + ": peak " + dB (inputPeak[c]) + ", rms " + dB (heardIn.rmsOf (c)));
            }

            auto songDir = dir.getChildFile (type.retainCharacters ("ASIOWindowsAudio"));
            songDir.deleteRecursively();
            songDir.createDirectory();
            auto songFile = songDir.getChildFile ("Interface Test.encore");
            behaviour->songFolder = [songFile] { return songFile.getParentDirectory(); };
            auto edit = songs::create (engine, songFile);
            edit->ensureNumberOfAudioTracks (numIn);
            auto& tr = edit->getTransport();
            tr.ensureContextAllocated();
            auto tracks = te::getAudioTracks (*edit);
            pump (0.5);
            for (int c = 0; c < numIn; ++c)
            {
                bool ok = false;
                for (auto instance : edit->getAllInputDevices())
                    if (instance->getInputDevice().getName() == inputs[c])
                        ok = instance->setTarget (tracks[c]->itemID, false, nullptr, 0).has_value();
                expect (label + ": track " + String (c + 1) + " takes " + inputs[c], ok);
            }
            tr.ensureContextAllocated();
            pump (0.5);
            auto listen = [&] (double s) { heardOut.reset(); heardIn.reset(); pump (s); return heardOut.loudest(); };

            // monitoring, one input at a time: silent, the input comes through at about its own level, silent again
            auto monitorCheck = [&] (const String& what, int c)
            {
                auto before = listen (0.6);
                expect (what + " input " + String (c + 1) + ": silent with monitoring off", before == 0.0f, dB (before));
                EngineHelpers::setInputMonitoring (*tracks[c], true);
                pump (0.5);
                auto heard = listen (1.0);
                const auto wasIn = heardIn.peakOf (c);
                if (wasIn == 0.0f) note ("  " + what + " input " + String (c + 1) + ": nothing on this input at all (digital zero), so monitoring can't be told apart from silence");
                else
                {
                    const auto gap = Decibels::gainToDecibels (heard, -160.0f) - Decibels::gainToDecibels (wasIn, -160.0f);
                    expect (what + " input " + String (c + 1) + ": comes through at its own level", std::abs (gap) < 9.0f,
                            "in " + dB (wasIn) + ", out " + dB (heard) + " (L " + dB (heardOut.peakOf (0)) + ", R " + dB (heardOut.peakOf (1)) + ")");
                }
                EngineHelpers::setInputMonitoring (*tracks[c], false);
                pump (0.4);
                auto after = listen (0.6);
                expect (what + " input " + String (c + 1) + ": silent again", after == 0.0f, dB (after));
            };
            for (int c = 0; c < numIn; ++c) monitorCheck (label + ":", c);

            auto settle = [&] (te::TimePosition from)
            {
                EngineHelpers::stopTransport (tr, from);
                for (int i = 0; i < 6 || ! EngineHelpers::settleStop (tr); ++i)
                {
                    EngineHelpers::settleStop (tr);
                    pump (0.05);
                    if (i > 200) break;
                }
                pump (0.1);
            };
            auto stoppedCleanly = [&] (const String& what, te::TimePosition from)
            {
                settle (from);
                auto a = tr.getPosition();
                pump (1.0);
                auto b = tr.getPosition();
                expect (what + ": everything stops, the playhead stays put", ! tr.isPlaying() && ! tr.isRecording() && a == b && b >= te::TimePosition(),
                        String (a.inSeconds(), 3) + " then " + String (b.inSeconds(), 3));
            };
            auto setPosition = [&] (double s) { tr.setPosition (te::TimePosition::fromSeconds (s)); pump (0.2); };

            // recording every input at once for 30 s, then the take is the right length, on each track, at the input's level
            for (int c = 0; c < numIn; ++c) EngineHelpers::armTrack (*tracks[c], true);
            tr.ensureContextAllocated();
            pump (0.6);
            auto armed = listen (0.6);
            expect (label + ": arming doesn't open the inputs to the speakers", armed == 0.0f, dB (armed));
            setPosition (0);
            heardIn.reset();
            const auto recT0 = Time::getMillisecondCounterHiRes();
            tr.record (false);
            pump (30.0);
            const auto recSeconds = (Time::getMillisecondCounterHiRes() - recT0) / 1000.0;
            float recPeak[maxChannels] {}, recRms[maxChannels] {};
            for (int c = 0; c < numIn; ++c) { recPeak[c] = heardIn.peakOf (c); recRms[c] = heardIn.rmsOf (c); }
            expect (label + ": still recording after 30 s", tr.isRecording());
            note ("  xruns after 30 s of recording: " + String (d->getXRunCount() - xrunsAtStart));
            stoppedCleanly (label + ": 30 s recording, Space", {});
            pump (0.8);
            for (int c = 0; c < numIn; ++c)
            {
                auto* clip = dynamic_cast<te::WaveAudioClip*> (tracks[c]->getClips().getLast());
                expect (label + ": input " + String (c + 1) + " take landed on its track", clip != nullptr);
                if (clip == nullptr) continue;
                AudioFormatManager fm;
                fm.registerBasicFormats();
                std::unique_ptr<AudioFormatReader> r { fm.createReaderFor (clip->getSourceFileReference().getFile()) };
                expect (label + ": input " + String (c + 1) + " take can be read", r != nullptr);
                if (r == nullptr) continue;
                const auto length = r->lengthInSamples / r->sampleRate;
                expect (label + ": input " + String (c + 1) + " take's length is right", std::abs (length - recSeconds) < 0.5,
                        String (length, 3) + " s of " + String (recSeconds, 3) + " s, " + String (r->numChannels) + " channel(s), " + String (r->bitsPerSample) + " bit, " + String (r->sampleRate, 0));
                AudioBuffer<float> take ((int) r->numChannels, (int) r->lengthInSamples);
                r->read (&take, 0, take.getNumSamples(), 0, true, true);
                const auto peak = take.getMagnitude (0, 0, take.getNumSamples());
                const auto rms = take.getRMSLevel (0, 0, take.getNumSamples());
                if (recPeak[c] == 0.0f) note ("  input " + String (c + 1) + ": digital zero while recording, take peak " + dB (peak));
                else expect (label + ": input " + String (c + 1) + " take matches what came in", std::abs (Decibels::gainToDecibels (rms, -160.0f) - Decibels::gainToDecibels (recRms[c], -160.0f)) < 3.0f,
                             "take rms " + dB (rms) + ", peak " + dB (peak) + "; raw rms " + dB (recRms[c]) + ", peak " + dB (recPeak[c]));
            }
            for (int c = 0; c < numIn; ++c) EngineHelpers::armTrack (*tracks[c], false);
            tr.ensureContextAllocated();
            pump (0.5);

            // the take plays back, and time runs at the right speed
            setPosition (1.0);
            heardOut.reset();
            const auto playT0 = Time::getMillisecondCounterHiRes();
            tr.play (false);
            pump (4.0);
            const auto played = tr.getPosition().inSeconds() - 1.0;
            const auto playedFor = (Time::getMillisecondCounterHiRes() - playT0) / 1000.0;
            expect (label + ": the playhead keeps time", std::abs (played - playedFor) < 0.3, String (played, 3) + " s in " + String (playedFor, 3) + " s");
            if (recPeak[0] > 0.0f) expect (label + ": the take plays back", heardOut.loudest() > 0.0f, dB (heardOut.loudest()));
            stoppedCleanly (label + ": playing, Space", te::TimePosition::fromSeconds (1.0));

            // Space hard on the heels of Record or Play, and shortly after: the blocks are a real driver's now
            EngineHelpers::armTrack (*tracks[0], true);
            tr.ensureContextAllocated();
            pump (0.5);
            for (const double gap : { 0.0, 0.03, 0.1, 0.3, 1.0 })
                for (int i = 0; i < 3; ++i)
                {
                    setPosition (0);
                    note ("  record, then Space " + String (gap, 2) + " s later");
                    tr.record (false);
                    if (gap > 0) pump (gap);
                    note ("  stopping");
                    stoppedCleanly (label + ": Space " + String (gap, 2) + " s after Record, try " + String (i + 1), {});
                    setPosition (0);
                    tr.play (false);
                    if (gap > 0) pump (gap);
                    stoppedCleanly (label + ": Space " + String (gap, 2) + " s after Play, try " + String (i + 1), {});
                }

            // monitoring while recording
            {
                EngineHelpers::setInputMonitoring (*tracks[0], true);
                pump (0.5);
                setPosition (0);
                tr.record (false);
                pump (0.5);
                auto heard = listen (1.0);
                if (heardIn.peakOf (0) > 0.0f) expect (label + ": monitoring is heard while recording", heard > 0.0f, dB (heard));
                stoppedCleanly (label + ": recording with monitoring on", {});
                EngineHelpers::setInputMonitoring (*tracks[0], false);
                pump (0.4);
            }
            EngineHelpers::armTrack (*tracks[0], false);
            tr.ensureContextAllocated();
            pump (0.4);

            // every block size and rate the driver offers (from the usual ones), then monitoring still works
            auto setupNow = jm.getAudioDeviceSetup();
            auto changeTo = [&] (double rate, int block)
            {
                auto s = setupNow;
                s.sampleRate = rate;
                s.bufferSize = block;
                auto e = jm.setAudioDeviceSetup (s, true);
                pump (1.2);
                tr.ensureContextAllocated (true);   // what the app does after a device change
                pump (0.6);
                return e;
            };
            auto rates = d->getAvailableSampleRates();
            auto sizes = d->getAvailableBufferSizes();
            for (const int block : { 64, 128, 256, 512, 1024, 2048 })
            {
                if (! sizes.contains (block)) continue;
                auto e = changeTo (startRate, block);
                auto* now = jm.getCurrentAudioDevice();
                const auto size = now != nullptr ? now->getCurrentBufferSizeSamples() : -1;
                expect (label + ": block size " + String (block) + " takes", e.isEmpty() && size == block, e + " now " + String (size));
                if (e.isEmpty()) monitorCheck (label + " at block " + String (block) + ":", 0);
            }
            for (const double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
            {
                if (! rates.contains (rate)) continue;
                auto e = changeTo (rate, startBlock);
                auto* now = jm.getCurrentAudioDevice();
                const auto actual = now != nullptr ? now->getCurrentSampleRate() : 0.0;
                expect (label + ": rate " + String (rate, 0) + " takes", e.isEmpty() && actual == rate, e + " now " + String (actual));
                if (e.isEmpty()) monitorCheck (label + " at " + String (rate, 0) + ":", 0);
            }
            changeTo (startRate, startBlock);

            // the interface goes away and comes back
            jm.closeAudioDevice();
            pump (0.6);
            jm.restartLastAudioDevice();
            pump (1.2);
            setupInputs();
            tr.ensureContextAllocated (true);
            pump (0.8);
            expect (label + ": the interface came back", jm.getCurrentAudioDevice() != nullptr);
            if (jm.getCurrentAudioDevice() != nullptr) monitorCheck (label + " after coming back:", 0);

            tr.stop (false, true);
            edit = nullptr;
            pump (0.3);
        };

        // the interface's own ASIO driver, by the same rule the app uses, then Windows audio
        for (auto* t : dm.deviceManager.getAvailableDeviceTypes())
        {
            t->scanForDevices();
            note ("driver type " + t->getTypeName() + ": in [" + t->getDeviceNames (true).joinIntoString (", ") + "] out [" + t->getDeviceNames (false).joinIntoString (", ") + "]");
        }
        auto named = [&] (const String& type, bool in)
        {
            StringArray hits;
            for (auto* t : dm.deviceManager.getAvailableDeviceTypes())
                if (t->getTypeName() == type)
                    for (auto& n : t->getDeviceNames (in))
                        if (n.containsIgnoreCase (word)) hits.add (n);
            return hits;
        };
        if (auto asio = named ("ASIO", false); asio.size() > 0) driverSection ("ASIO", "ASIO", asio[0], asio[0]);
        else note ("no ASIO driver has \"" + word + "\" in its name");
        {
            auto ins = named ("Windows Audio", true), outs = named ("Windows Audio", false);
            if (ins.size() > 0 && outs.size() > 0) driverSection ("Windows", "Windows Audio", ins[0], outs[0]);
            else note ("Windows Audio has no input and output with \"" + word + "\" in the name");
        }

        dm.deviceManager.removeAudioCallback (&inputTap);
        dm.closeDevices();
        note ("");
        note (failures == 0 ? "ALL PASSED" : String (failures) + " FAILED");
        return failures == 0 ? 0 : 1;
    }
}
