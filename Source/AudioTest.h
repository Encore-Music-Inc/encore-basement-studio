// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

#pragma once
// --audiotest <dir>: the live side of the app (monitoring, meters, recording, stopping) run against a pretend
// four-input interface, with a tone on one input. It opens no real audio or MIDI device, so it's safe while
// another app holds the interface. Results go to <dir>/audiotest.log.
namespace audiotest
{
    /** Stands in for an audio interface: an audio thread that feeds a tone into one input and watches the outputs */
    struct FakeInterface : public juce::Thread
    {
        FakeInterface (te::HostedAudioDeviceInterface& h, int inputs, int outputs)
            : juce::Thread ("fake interface"), hosted (h), numIn (inputs), numOut (outputs) {}
        ~FakeInterface() override { stopThread (2000); }

        void run() override
        {
            static constexpr int blockSize = 512;
            static constexpr double rate = 48000.0;
            juce::AudioBuffer<float> buffer (juce::jmax (numIn, numOut), blockSize);
            juce::MidiBuffer midi;
            double phase = 0, next = juce::Time::getMillisecondCounterHiRes();
            while (! threadShouldExit())
            {
                buffer.clear();
                if (const int ch = toneOn.load(); ch >= 0 && ch < numIn)
                    for (int i = 0; i < blockSize; ++i, phase += juce::MathConstants<double>::twoPi * 440.0 / rate)
                        buffer.setSample (ch, i, level * (float) std::sin (phase));
                hosted.processBlock (buffer, midi);
                float peak = 0;
                for (int c = 0; c < numOut; ++c) peak = juce::jmax (peak, buffer.getMagnitude (c, 0, blockSize));
                outPeak.store (juce::jmax (outPeak.load(), peak));
                next += blockSize / rate * 1000.0;
                if (const auto ms = next - juce::Time::getMillisecondCounterHiRes(); ms > 0) wait ((int) ms + 1);
            }
        }
        float takePeak() { return outPeak.exchange (0.0f); }

        static constexpr float level = 0.25f;
        te::HostedAudioDeviceInterface& hosted;
        const int numIn, numOut;
        std::atomic<int> toneOn { -1 };
        std::atomic<float> outPeak { 0.0f };
    };

    inline int run (const juce::File& dir)
    {
        using namespace juce;
        int failures = 0;
        dir.createDirectory();
        auto out = dir.getChildFile ("audiotest.log");
        out.replaceWithText ({});
        auto note = [&] (const String& s) { out.appendText (s + "\n"); };
        auto expect = [&] (const String& name, bool ok, const String& detail = {})
        {
            note ((ok ? "PASS " : "FAIL ") + name + (detail.isEmpty() ? String() : "  " + detail));
            if (! ok) ++failures;
        };
        auto pump = [] (double seconds) { MessageManager::getInstance()->runDispatchLoopUntil ((int) (seconds * 1000)); };
        auto dB = [] (float v) { return String (Decibels::gainToDecibels (v, -120.0f), 1) + " dB"; };

        auto songDir = dir.getChildFile ("Audio Test");
        songDir.deleteRecursively();
        songDir.createDirectory();
        auto songFile = songDir.getChildFile ("Audio Test.encore");

        auto* behaviour = new StudioBehaviour (true);
        behaviour->songFolder = [songFile] { return songFile.getParentDirectory(); };
        te::Engine engine { "Encore Studio Audiotest", std::make_unique<HeadlessUIBehaviour>(), std::unique_ptr<te::EngineBehaviour> (behaviour) };
        auto& dm = engine.getDeviceManager();

        auto& hosted = dm.getHostedAudioDeviceInterface();
        te::HostedAudioDeviceInterface::Parameters params;
        params.sampleRate = 48000;
        params.blockSize = 512;
        params.inputChannels = 4;
        params.outputChannels = 2;
        params.useMidiDevices = false;
        params.inputNames = { "Pretend 1", "Pretend 2", "Pretend 3", "Pretend 4" };
        hosted.initialise (params);
        hosted.prepareToPlay (params.sampleRate, params.blockSize);
        FakeInterface device (hosted, params.inputChannels, params.outputChannels);
        device.startThread (Thread::Priority::highest);
        pump (0.5);

        // the app's own input setup: split stereo pairs into mono inputs, and turn them all on
        auto setupInputs = [&] (bool kept = false)
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
                    if (! kept) w->setMonitorMode (te::InputDevice::MonitorMode::off);   // the hosted device starts them on; the app's start off
                    names.add (w->getName());
                }
            return names;
        };
        auto inputs = setupInputs();
        note ("inputs: " + inputs.joinIntoString (", "));
        expect ("the pretend interface shows four mono inputs", inputs.size() >= 4, String (inputs.size()));

        auto edit = songs::create (engine, songFile);
        edit->ensureNumberOfAudioTracks (2);
        edit->getTransport().ensureContextAllocated();
        auto& tr = edit->getTransport();
        auto tracks = te::getAudioTracks (*edit);
        auto& one = *tracks[0];
        auto& two = *tracks[1];
        pump (0.5);

        auto assign = [&] (te::AudioTrack& t, const String& name)
        {
            for (auto instance : edit->getAllInputDevices())
                if (instance->getInputDevice().getName() == name)
                    return instance->setTarget (t.itemID, false, nullptr, 0).has_value();
            return false;
        };
        expect ("track 1 takes input 1", assign (one, inputs[0]));
        expect ("track 2 takes input 3", assign (two, inputs[2]));
        tr.ensureContextAllocated();
        pump (0.5);

        // what the track's meter reads while the track is armed or monitoring: the input it's fed by
        struct Meter
        {
            explicit Meter (te::LevelMeasurer& m) : measurer (m) { measurer.addClient (client); }
            ~Meter() { measurer.removeClient (client); }
            float read (double seconds)
            {
                client.getAndClearAudioLevel (0);
                juce::MessageManager::getInstance()->runDispatchLoopUntil ((int) (seconds * 1000));
                return client.getAndClearAudioLevel (0).dB;
            }
            te::LevelMeasurer& measurer;
            te::LevelMeasurer::Client client;
        };
        auto inputOf = [&] (te::AudioTrack& t) -> te::InputDevice*
        {
            for (auto instance : edit->getAllInputDevices())
                if (te::isOnTargetTrack (*instance, t, 0)) return &instance->getInputDevice();
            return nullptr;
        };
        auto listen = [&] (double seconds) { device.takePeak(); pump (seconds); return device.takePeak(); };

        // 1. monitoring, the transport stopped (where you are when you check a mic)
        device.toneOn = 0;
        {
            auto* in = inputOf (one);
            auto heard = listen (0.6);
            expect ("silence before monitoring", heard < 0.001f, dB (heard) + " mode=" + String (in != nullptr ? (int) in->getMonitorMode() : -1)
                     + " armed=" + String ((int) EngineHelpers::isTrackArmed (one)) + " monitoring=" + String ((int) EngineHelpers::isInputMonitoringEnabled (one)));
        }
        EngineHelpers::setInputMonitoring (one, true);
        pump (0.5);
        {
            auto heard = listen (0.8);
            expect ("monitor on, stopped: the input reaches the outputs", heard > 0.05f, dB (heard));
            if (auto* in = inputOf (one))
            {
                Meter meter (in->levelMeasurer);
                auto level = meter.read (0.6);
                expect ("monitor on, stopped: the input meter moves", level > -40.0f, String (level, 1) + " dB");
            }
            else expect ("monitor on, stopped: the input meter moves", false, "no input on the track");
            Meter track (one.getLevelMeterPlugin()->measurer);
            auto level = track.read (0.6);
            expect ("monitor on, stopped: the track level moves", level > -40.0f, String (level, 1) + " dB");
        }
        device.toneOn = 1;
        expect ("input 2 doesn't come through track 1's monitoring", listen (0.6) < 0.001f);
        device.toneOn = 0;
        EngineHelpers::setInputMonitoring (one, false);
        pump (0.5);
        expect ("monitor off: silent again", listen (0.6) < 0.001f);

        // 2. the same on an input past the first pair
        device.toneOn = 2;
        EngineHelpers::setInputMonitoring (two, true);
        pump (0.5);
        expect ("monitor on input 3", listen (0.8) > 0.05f);
        EngineHelpers::setInputMonitoring (two, false);
        pump (0.5);

        // 2b. two tracks on the same input (overdubs): each one's monitor button is its own
        {
            edit->ensureNumberOfAudioTracks (4);
            auto all = te::getAudioTracks (*edit);
            auto& shared = *all[2];
            device.toneOn = 0;
            expect ("track 3 also takes input 1", assign (shared, inputs[0]));
            tr.ensureContextAllocated();
            pump (0.5);
            Meter oneLevel (one.getLevelMeterPlugin()->measurer), sharedLevel (shared.getLevelMeterPlugin()->measurer);
            EngineHelpers::setInputMonitoring (one, true);
            pump (0.5);
            auto a1 = oneLevel.read (0.6), a3 = sharedLevel.read (0.6);
            expect ("shared input: monitoring track 1 leaves track 3 off", EngineHelpers::isInputMonitoringEnabled (one) && ! EngineHelpers::isInputMonitoringEnabled (shared));
            expect ("shared input: only track 1 plays the input", a1 > -40.0f && a3 < -90.0f, "track 1 " + String (a1, 1) + " dB, track 3 " + String (a3, 1) + " dB");
            // a track that joins the input while another is monitoring doesn't monitor
            auto& late = *all[3];
            expect ("track 4 also takes input 1", assign (late, inputs[0]));
            tr.ensureContextAllocated();
            pump (0.5);
            Meter lateLevel (late.getLevelMeterPlugin()->measurer);
            auto a4 = lateLevel.read (0.6);
            expect ("shared input: a track added later isn't monitoring", ! EngineHelpers::isInputMonitoringEnabled (late) && a4 < -90.0f, String (a4, 1) + " dB");
            EngineHelpers::setInputMonitoring (shared, true);
            EngineHelpers::setInputMonitoring (one, false);
            pump (0.5);
            a1 = oneLevel.read (0.6); a3 = sharedLevel.read (0.6);
            expect ("shared input: switched over to track 3", ! EngineHelpers::isInputMonitoringEnabled (one) && EngineHelpers::isInputMonitoringEnabled (shared)
                     && a1 < -90.0f && a3 > -40.0f, "track 1 " + String (a1, 1) + " dB, track 3 " + String (a3, 1) + " dB");
            expect ("shared input: still heard", listen (0.8) > 0.05f);
            EngineHelpers::setInputMonitoring (shared, false);
            pump (0.5);
            expect ("shared input: both off is silent", listen (0.6) < 0.001f);
            for (auto* t : { &shared, &late })
                for (auto instance : edit->getAllInputDevices())
                    if (instance->getTargets().contains (t->itemID)) [[maybe_unused]] auto r = instance->removeTarget (t->itemID, nullptr);
            tr.ensureContextAllocated();
            pump (0.5);
        }

        // 3. monitoring while the song plays
        device.toneOn = 0;
        tr.play (false);
        pump (0.5);
        EngineHelpers::setInputMonitoring (one, true);
        pump (0.5);
        expect ("monitor on while playing", listen (0.8) > 0.05f);
        expect ("turning monitoring on doesn't stop the song", tr.isPlaying());
        tr.stop (false, false);
        pump (0.3);
        expect ("monitoring carries on after a stop", listen (0.8) > 0.05f);

        // 4. the interface goes away and comes back (a power cycle, or another app takes it and gives it back)
        {
            auto setup = dm.deviceManager.getAudioDeviceSetup();
            dm.deviceManager.closeAudioDevice();
            pump (0.5);
            dm.deviceManager.restartLastAudioDevice();
            pump (0.8);
            setupInputs (true);
            tr.ensureContextAllocated (true);   // what the app does after a device change
            pump (0.8);
            expect ("the interface came back", dm.deviceManager.getCurrentAudioDevice() != nullptr);
            expect ("track 1 kept its input", inputOf (one) != nullptr);
            auto heard = listen (0.8);
            expect ("monitoring works after the interface comes back", heard > 0.05f, dB (heard));
            if (auto* in = inputOf (one))
            {
                Meter meter (in->levelMeasurer);
                auto level = meter.read (0.6);
                expect ("the input meter moves after the interface comes back", level > -40.0f, String (level, 1) + " dB");
            }
        }
        EngineHelpers::setInputMonitoring (one, false);
        pump (0.5);

        // 5. recording, and stopping it the way Space does
        te::TimePosition recordFrom;
        auto clipsOn = [] (te::AudioTrack& t) { return t.getClips().size(); };
        auto stoppedCleanly = [&] (const String& what)
        {
            EngineHelpers::stopTransport (tr, recordFrom);   // Space, see MainComponent::togglePlay
            expect (what + ": flags clear at once", ! tr.isPlaying() && ! tr.isRecording());
            for (int i = 0; i < 4 || ! EngineHelpers::settleStop (tr); ++i)
            {
                EngineHelpers::settleStop (tr);
                pump (0.05);
                if (i > 200) break;
            }
            pump (0.1);
            auto a = tr.getPosition();
            pump (1.0);
            auto b = tr.getPosition();
            expect (what + ": everything stops, the playhead stays put", ! tr.isPlaying() && ! tr.isRecording() && a == b,
                    String (a.inSeconds(), 3) + " then " + String (b.inSeconds(), 3));
            expect (what + ": the playhead isn't before the song", b >= te::TimePosition(), String (b.inSeconds(), 3));
        };
        auto start = [&]
        {
            const auto t0 = Time::getMillisecondCounterHiRes();
            tr.record (false);
            auto from = tr.getPosition();
            for (int i = 0; i < 300 && tr.getPosition() == from; ++i) pump (0.01);
            return Time::getMillisecondCounterHiRes() - t0;
        };
        auto setPosition = [&] (double s) { tr.setPosition (te::TimePosition::fromSeconds (s)); pump (0.2); };

        EngineHelpers::armTrack (one, true);
        tr.ensureContextAllocated();
        pump (0.5);
        expect ("track 1 armed", EngineHelpers::isTrackArmed (one));
        expect ("arming alone doesn't open the input to the speakers", listen (0.6) < 0.001f && ! EngineHelpers::isInputMonitoringEnabled (one));
        {
            setPosition (0);
            int before = clipsOn (one);
            auto ms = start();
            expect ("recording starts quickly", ms < 500.0, String (ms, 0) + " ms");
            expect ("recording is running", tr.isRecording() && tr.isPlaying());
            pump (2.0);
            stoppedCleanly ("record then Space");
            pump (0.5);
            expect ("the take landed on the track", clipsOn (one) == before + 1, String (clipsOn (one)));
            if (auto* clip = dynamic_cast<te::WaveAudioClip*> (one.getClips().getLast()))
            {
                auto f = clip->getSourceFileReference().getFile();
                AudioFormatManager fm;
                fm.registerBasicFormats();
                if (std::unique_ptr<AudioFormatReader> r { fm.createReaderFor (f) })
                {
                    Range<float> range[2];
                    r->readMaxLevels (0, r->lengthInSamples, range, (int) jmin (2u, r->numChannels));
                    auto peak = jmax (std::abs (range[0].getStart()), std::abs (range[0].getEnd()));
                    expect ("the take's length is right", r->lengthInSamples / r->sampleRate > 1.5, String (r->lengthInSamples / r->sampleRate, 2) + " s");
                    expect ("the take is at the input's level", std::abs (peak - FakeInterface::level) < 0.05f, String (peak, 3));
                }
                else expect ("the take can be read", false, f.getFullPathName());
            }
        }

        // Space again straight after (a bounced key) doesn't start it up again by itself
        {
            setPosition (1.0);
            start();
            pump (1.0);
            tr.stop (false, false);
            tr.stop (false, false);
            pump (0.3);
            auto a = tr.getPosition();
            pump (0.6);
            expect ("a double stop stays stopped", ! tr.isPlaying() && a == tr.getPosition());
        }

        // with the click, a loop, and through a count-in
        {
            edit->clickTrackEnabled = true;
            setPosition (0);
            start();
            pump (1.0);
            stoppedCleanly ("recording with the click");
            edit->clickTrackEnabled = false;
        }
        {
            tr.setLoopRange ({ te::TimePosition(), te::TimePosition::fromSeconds (2.0) });
            tr.looping = true;
            setPosition (0);
            start();
            pump (2.6);
            stoppedCleanly ("recording in a loop");
            tr.looping = false;
        }
        for (const int mode : { 1, 3 })
        {
            engine.getPropertyStorage().setProperty (te::SettingID::countInMode, mode);
            setPosition (0);
            const int beats = edit->getNumCountInBeats();
            note ("count-in beats: " + String (beats));
            tr.record (false);
            pump (0.25);   // Space during the count-in
            stoppedCleanly ("stopped during a count-in of " + String (beats));
            setPosition (0);
            tr.record (false);
            pump (beats * 0.5 + 1.0);
            stoppedCleanly ("stopped after a count-in of " + String (beats));
        }
        engine.getPropertyStorage().setProperty (te::SettingID::countInMode, 0);

        // two presses in a row: Record, then Space before the engine has got going
        for (int i = 0; i < 5; ++i)
        {
            setPosition (0);
            tr.record (false);
            stoppedCleanly ("Space straight after Record, try " + String (i + 1));
        }
        for (int i = 0; i < 5; ++i)
        {
            setPosition (0);
            tr.play (false);
            stoppedCleanly ("Space straight after Play, try " + String (i + 1));
        }

        // monitoring on while recording: you hear yourself, then it all stops
        {
            EngineHelpers::setInputMonitoring (one, true);
            pump (0.5);
            setPosition (0);
            start();
            pump (0.3);
            auto heard = listen (0.8);
            expect ("monitoring is heard while recording", heard > 0.05f, dB (heard));
            stoppedCleanly ("recording with monitoring on");
            EngineHelpers::setInputMonitoring (one, false);
            pump (0.3);
        }

        EngineHelpers::armTrack (one, false);
        device.toneOn = -1;
        tr.stop (false, true);
        device.stopThread (2000);
        edit = nullptr;
        dm.removeHostedAudioDeviceInterface();
        note (failures == 0 ? "ALL PASSED" : String (failures) + " FAILED");
        return failures == 0 ? 0 : 1;
    }
}
