// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
//
// The drum grid: a step editor for a drum track's MIDI clip, in the same pane
// as the piano roll (double-click the clip; the header switches between them).
//
//   click a step          puts a hit there (or takes it away)
//   drag across steps     paints hits (or erases, if the first one was a hit)
//   Shift+click           an accent (louder hit), or back to normal
//   right-click           velocity, and clearing or filling a whole row
//   click a drum's name   plays it
//
// Drum tracks play one of ENCORE.'s kits (encore-audio-processing's drum
// synthesizer): drum sounds made the first time (no samples to download),
// written into the song folder so the song is complete on its own. The drum
// grid's Kit box swaps them. Rows follow General MIDI, so any MIDI drum part or drum
// plugin lines up too.
#pragma once

#include "dsp/DrumSynth.h"
#include "dsp/Music.h"

namespace drums
{
    struct Lane { int note; juce::String name; };

    /** The kit's rows, top to bottom (General MIDI notes), named by ENCORE.'s one drum name rule */
    inline const std::vector<Lane>& lanes()
    {
        static const std::vector<Lane> l = []
        {
            std::vector<Lane> out;
            for (int note : vs::music::drumLanes) out.push_back ({ note, juce::String (vs::music::drumName (note)) });
            return out;
        }();
        return l;
    }

    /** The name of any drum note (an imported part can use notes the kit has no row for) */
    inline juce::String gmName (int note) { return juce::String (vs::music::drumName (note)); }

    inline bool isDrumTrack (te::AudioTrack& t) { return t.state.getProperty ("basementRole").toString() == "drums"; }

    //==============================================================================
    // The kits are encore-audio-processing's (dsp/DrumSynth.h): its drum
    // synthesizer and its kit list, the same kits everywhere in ENCORE.
    namespace kit
    {
        namespace ds = vs::drumsynth;
        constexpr double fs = 48000.0;

        /** The kits the drum grid offers: the ones with a drum of their own on every
            row (a pad kit leaves some of the grid's rows to stand-ins) */
        inline const std::vector<ds::Kit>& styles()
        {
            static const std::vector<ds::Kit> k = []
            {
                std::vector<ds::Kit> out;
                for (const auto& kit : ds::kits())
                {
                    bool every = true;
                    for (int note : vs::music::drumLanes)
                    {
                        bool has = false;
                        for (const auto& p : kit.pads) has = has || (p.note == note && p.sound[0] != 0);
                        every = every && has;
                    }
                    if (every) out.push_back (kit);
                }
                return out;
            }();
            return k;
        }

        /** A kit by its name (songs save the name); Studio, the first drum tracks' kit, when unknown */
        inline int styleIndex (const juce::String& name)
        {
            int studio = 0;
            for (int i = 0; i < (int) styles().size(); ++i)
            {
                if (name == styles()[(size_t) i].name) return i;
                if (juce::String (styles()[(size_t) i].id) == "studio") studio = i;
            }
            return studio;
        }
        inline int defaultStyle() { return styleIndex ("Studio"); }

        /** Where a kit's WAVs go in a song ("Drum kit" for the Studio kit, as songs made before had it) */
        inline juce::File folderFor (const juce::File& songFolder, int style)
        {
            const auto& k = styles()[(size_t) juce::jlimit (0, (int) styles().size() - 1, style)];
            return songFolder.getChildFile (juce::String (k.id) == "studio" ? juce::String ("Drum kit") : "Drum kit " + juce::String (k.name));
        }

        /** The file a row's sound is kept in. The names are the ones songs made
            before already have, so a song's own kit files keep playing as they were */
        inline juce::String fileName (int note)
        {
            switch (note)
            {
                case 41: return "Low tom";
                case 45: return "Mid tom";
                case 48: return "High tom";
                default: return gmName (note);
            }
        }

        /** A row's sound in a kit, at the kit's level */
        inline ds::Buffer make (int note, int style)
        {
            const auto& k = styles()[(size_t) juce::jlimit (0, (int) styles().size() - 1, style)];
            const char* id = ds::kitSound (k, note);
            return id != nullptr ? ds::renderSound (id, fs) : ds::Buffer {};
        }

        /** A kit's WAVs in a folder (made if they aren't there), as float WAVs */
        inline juce::File ensure (const juce::File& folder, int style)
        {
            folder.createDirectory();
            for (auto& l : lanes())
            {
                auto f = folder.getChildFile (fileName (l.note) + ".wav");
                if (f.existsAsFile()) continue;
                const auto sound = make (l.note, style);
                if (sound.empty()) continue;
                juce::AudioBuffer<float> audio (1, (int) sound.size());
                audio.copyFrom (0, 0, sound.data(), (int) sound.size());
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
                if (os == nullptr) continue;
                auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (fs).withNumChannels (1).withBitsPerSample (32)
                                                      .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
                if (w != nullptr) w->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
            }
            return folder;
        }

        /** Loads a kit into a Sampler: each sound on its own key, played to the end of the
            sample, at the kit's own levels (the hats, toms and cymbals spread a little) */
        inline void load (te::SamplerPlugin& sampler, const juce::File& songFolder, int style = defaultStyle())
        {
            auto folder = ensure (folderFor (songFolder, style), style);
            while (sampler.getNumSounds() > 0) sampler.removeSound (0);
            for (auto& l : lanes())
            {
                auto f = folder.getChildFile (fileName (l.note) + ".wav");
                if (! f.existsAsFile() || sampler.addSound (f.getFullPathName(), l.name, 0.0, 0.0, 0.0f).isNotEmpty()) continue;
                const int i = sampler.getNumSounds() - 1;
                sampler.setSoundParams (i, l.note, l.note, l.note);
                sampler.setSoundOpenEnded (i, true);
                const float pan = l.note == 42 || l.note == 46 ? 0.25f : l.note == 41 ? -0.3f : l.note == 48 ? 0.3f : l.note == 51 ? 0.35f : l.note == 49 ? -0.35f : 0.0f;
                sampler.setSoundGains (i, 0.0f, pan);
            }
            sampler.flushPendingUpdates();
        }
    }
}

//==============================================================================
class DrumGrid : public NoteEditor,
                 private juce::Timer
{
public:
    DrumGrid (te::MidiClip& c) : clip (&c)
    {
        setWantsKeyboardFocus (true);
        title.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, theme::paper);
        addAndMakeVisible (title);
        for (int i : { 0, 1, 2, 3, 5 }) gridBox.addItem (midiedit::grids()[(size_t) i].name, i + 1);
        gridBox.setSelectedId (3, juce::dontSendNotification);   // 1/16
        gridBox.setTooltip ("Step size");
        gridBox.onChange = [this] { repaint(); };
        addAndMakeVisible (gridBox);
        for (int i = 0; i < (int) drums::kit::styles().size(); ++i) kitBox.addItem (juce::String ("Kit: ") + juce::String (juce::CharPointer_UTF8 (drums::kit::styles()[(size_t) i].name)), i + 1);
        if (auto* t = dynamic_cast<te::AudioTrack*> (c.getTrack()))
            kitBox.setSelectedId (drums::kit::styleIndex (t->state.getProperty ("drumKit", "Studio").toString()) + 1, juce::dontSendNotification);
        kitBox.setTooltip ("Which drum kit this track plays");
        kitBox.onChange = [this] { switchKit (kitBox.getSelectedId() - 1); };
        addAndMakeVisible (kitBox);
        pianoButton.setTooltip ("Edit these notes in the piano roll instead");
        pianoButton.onClick = [this] { if (onSwitchView) onSwitchView(); };
        addAndMakeVisible (pianoButton);
        closeButton.setTooltip ("Close (Esc)");
        closeButton.onClick = [this] { if (onClose) onClose(); };
        addAndMakeVisible (closeButton);
        for (auto* b : std::initializer_list<juce::Component*> { &gridBox, &kitBox, &pianoButton, &closeButton }) b->setMouseClickGrabsKeyboardFocus (false);
        updateRows();
        startTimerHz (20);
    }

    ~DrumGrid() override { stopAudition(); }

    te::MidiClip* getClip() const override { return clip.get(); }
    juce::Array<te::MidiNote*> targetNotes() override { return clip != nullptr ? midiedit::allNotes (*clip) : juce::Array<te::MidiNote*>(); }
    double gridBeats() const override { return midiedit::grids()[(size_t) juce::jlimit (0, 5, gridBox.getSelectedId() - 1)].beats; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::base);
        g.setColour (theme::surface);
        g.fillRect (getLocalBounds().removeFromTop (headerH));
        g.setColour (theme::strong);
        g.fillRect (getLocalBounds().removeFromTop (1));
        if (clip == nullptr) return;

        const auto area = gridArea();
        const int steps = numSteps();
        const double step = gridBeats();
        const int perBeat = juce::jmax (1, juce::roundToInt (1.0 / step));
        const int perBar = perBeat * juce::jmax (1, beatsPerBar());
        for (int row = 0; row < rows.size(); ++row)
        {
            auto y = area.getY() + row * rowH;
            // the drum's name
            g.setColour (row % 2 ? theme::subtle : theme::surface);
            g.fillRect (0, y, nameW, rowH);
            g.setColour (rows[row].note == lastAuditioned ? theme::accent : theme::text2);
            g.setFont (juce::FontOptions (12.5f));
            g.drawText (rows[row].name, 10, y, nameW - 14, rowH, juce::Justification::centredLeft, true);
            // its steps
            for (int s = firstStep; s < steps; ++s)
            {
                auto cell = cellBounds (row, s);
                if (cell.getX() > area.getRight()) break;
                const bool beatGroup = (s / perBeat) % 2 == 0;
                g.setColour (beatGroup ? theme::raised : theme::strong.interpolatedWith (theme::raised, 0.5f));
                g.fillRoundedRectangle (cell.reduced (1.5f), 3.0f);
                if (auto* n = noteAt (rows[row].note, s))
                {
                    const float v = n->getVelocity() / 127.0f;
                    g.setColour (theme::accent.withAlpha (0.35f + 0.65f * v));
                    g.fillRoundedRectangle (cell.reduced (2.5f), 3.0f);
                    if (n->getVelocity() >= 120) { g.setColour (theme::paper); g.fillEllipse (cell.getCentreX() - 2.5f, cell.getCentreY() - 2.5f, 5.0f, 5.0f); }
                }
            }
        }
        // bar lines and numbers
        g.setFont (juce::FontOptions (11.0f));
        for (int s = firstStep; s <= steps; ++s)
        {
            const float x = cellX (s);
            if (x > area.getRight()) break;
            if (s % perBar == 0)
            {
                g.setColour (theme::muted.withAlpha (0.6f));
                g.fillRect (x - 1.0f, (float) area.getY(), 1.5f, (float) area.getHeight());
                g.setColour (theme::muted);
                if (s < steps) g.drawText ("Bar " + juce::String (s / perBar + 1), (int) x + 3, headerH + 2, 60, 14, juce::Justification::centredLeft);
            }
        }
        // the playhead
        auto& transport = clip->edit.getTransport();
        const double beat = clip->edit.tempoSequence.toBeats (transport.getPosition()).inBeats() - clip->getStartBeat().inBeats();   // from the clip's start
        if (beat >= 0 && beat < clip->getLengthInBeats().inBeats())
        {
            const float x = cellX (firstStep) + (float) ((beat / step - firstStep) * cellW);
            if (x >= area.getX())
            {
                g.setColour (theme::paper.withAlpha (0.8f));
                g.fillRect (x, (float) area.getY(), 1.5f, (float) area.getHeight());
            }
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().removeFromTop (headerH).reduced (10, 5);
        closeButton.setBounds (r.removeFromRight (28));
        r.removeFromRight (10);
        pianoButton.setBounds (r.removeFromRight (90));
        r.removeFromRight (6);
        gridBox.setBounds (r.removeFromRight (90));
        r.removeFromRight (6);
        kitBox.setBounds (r.removeFromRight (120));
        r.removeFromRight (10);
        title.setBounds (r);
        // cells as wide as fit (the whole clip when it can), never narrower than 14 px
        const int avail = gridArea().getWidth();
        cellW = juce::jlimit (14.0f, 44.0f, avail / (float) juce::jmax (1, numSteps()));
        rowH = juce::jlimit (16, 26, gridArea().getHeight() / juce::jmax (1, rows.size()));
    }

    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (clip == nullptr) return;
        grabKeyboardFocus();
        const auto p = e.getPosition();
        const int row = rowAt (p.y);
        if (row < 0) return;
        if (p.x < nameW) { audition (rows[row].note, 110); if (e.mods.isPopupMenu()) rowMenu (row); return; }
        const int s = stepAt (p.x);
        if (s < 0) return;
        if (e.mods.isPopupMenu()) { cellMenu (row, s); return; }
        clip->edit.getUndoManager().beginNewTransaction ("Drum hits");
        auto* existing = noteAt (rows[row].note, s);
        if (e.mods.isShiftDown())
        {
            painting = Paint::none;
            if (existing != nullptr) existing->setVelocity (existing->getVelocity() >= 120 ? 100 : 127, &clip->edit.getUndoManager());
            else addHit (row, s, 127);
            audition (rows[row].note, 127);
        }
        else if (existing != nullptr) { painting = Paint::erase; removeHit (row, s); }
        else { painting = Paint::add; addHit (row, s, 100); audition (rows[row].note, 100); }
        lastCell = { row, s };
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (painting == Paint::none || clip == nullptr) return;
        const int row = rowAt (e.y), s = stepAt (e.x);
        if (row < 0 || s < 0 || juce::Point<int> (row, s) == lastCell) return;
        lastCell = { row, s };
        if (painting == Paint::add && noteAt (rows[row].note, s) == nullptr) { addHit (row, s, 100); audition (rows[row].note, 100); }
        if (painting == Paint::erase) removeHit (row, s);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { painting = Paint::none; }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const int visible = (int) (gridArea().getWidth() / cellW);
        firstStep = juce::jlimit (0, juce::jmax (0, numSteps() - visible), firstStep - juce::roundToInt ((w.deltaX != 0 ? w.deltaX : w.deltaY) * 16.0f));
        repaint();
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k.getKeyCode() == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        return false;   // everything else (Space, R...) goes to the app as usual
    }

private:
    te::SafeSelectable<te::MidiClip> clip;
    juce::Label title;
    juce::ComboBox gridBox, kitBox;

    /** Swaps the track's Sampler to another of Studio's kits (written into the song folder the first time) */
    void switchKit (int style)
    {
        if (clip == nullptr) return;
        auto* t = dynamic_cast<te::AudioTrack*> (clip->getTrack());
        if (t == nullptr || ! clip->edit.editFileRetriever) return;
        for (auto p : t->pluginList.getPlugins())
            if (auto* s = dynamic_cast<te::SamplerPlugin*> (p))
            {
                drums::kit::load (*s, clip->edit.editFileRetriever().getParentDirectory(), style);
                t->state.setProperty ("drumKit", juce::String (drums::kit::styles()[(size_t) style].name), nullptr);
                return;
            }
    }
    juce::TextButton pianoButton { "Piano roll" }, closeButton { "X" };
    struct Row { int note; juce::String name; };
    juce::Array<Row> rows;
    enum class Paint { none, add, erase } painting = Paint::none;
    juce::Point<int> lastCell { -1, -1 };
    int firstStep = 0, rowH = 22;
    float cellW = 24.0f;
    static constexpr int headerH = 34, nameW = 110, rulerH = 18;

    te::MPESourceID auditionSource = te::createUniqueMPESourceID();
    int lastAuditioned = -1;
    juce::uint32 auditionUntil = 0;

    juce::Rectangle<int> gridArea() const { return getLocalBounds().withTrimmedTop (headerH + rulerH).withTrimmedLeft (nameW).withTrimmedBottom (4); }
    // the bar length where the clip starts, by the time signature there
    int beatsPerBar() const { return clip != nullptr ? clip->edit.tempoSequence.getTimeSigAt (clip->getPosition().getStart()).numerator.get() : 4; }
    int numSteps() const { return clip != nullptr ? juce::jmax (1, (int) std::ceil (clip->getLengthInBeats().inBeats() / gridBeats() - 1.0e-6)) : 16; }
    float cellX (int s) const { return gridArea().getX() + (s - firstStep) * cellW; }
    juce::Rectangle<float> cellBounds (int row, int s) const { return { cellX (s), (float) (gridArea().getY() + row * rowH), cellW, (float) rowH }; }
    int rowAt (int y) const { const int r = (y - gridArea().getY()) / rowH; return y >= gridArea().getY() && r < rows.size() ? r : -1; }
    int stepAt (int x) const
    {
        if (x < gridArea().getX()) return -1;
        const int s = firstStep + (int) ((x - gridArea().getX()) / cellW);
        return s < numSteps() ? s : -1;
    }

    /** Steps are counted from the start of the clip; notes from its content start */
    double stepBeat (int s) const { return s * gridBeats() + (clip->getStartBeat() - clip->getContentStartBeat()).inBeats(); }

    te::MidiNote* noteAt (int pitch, int s) const
    {
        const double from = stepBeat (s), to = from + gridBeats();
        for (auto* n : clip->getSequence().getNotes())
            if (n->getNoteNumber() == pitch && n->getStartBeat().inBeats() >= from - 1.0e-4 && n->getStartBeat().inBeats() < to - 1.0e-4) return n;
        return nullptr;
    }

    void addHit (int row, int s, int velocity)
    {
        clip->getSequence().addNote (rows[row].note, te::BeatPosition::fromBeats (stepBeat (s)), te::BeatDuration::fromBeats (gridBeats() * 0.9),
                                     velocity, 0, &clip->edit.getUndoManager());
    }

    void removeHit (int row, int s)
    {
        while (auto* n = noteAt (rows[row].note, s)) clip->getSequence().removeNote (*n, &clip->edit.getUndoManager());
    }

    /** The kit's rows, plus a row for any other drum note the clip has */
    void updateRows()
    {
        rows.clear();
        for (auto& l : drums::lanes()) rows.add ({ l.note, l.name });
        if (clip == nullptr) return;
        std::set<int> extra;
        for (auto* n : clip->getSequence().getNotes())
        {
            bool known = false;
            for (auto& l : rows) known = known || l.note == n->getNoteNumber();
            if (! known) extra.insert (n->getNoteNumber());
        }
        for (int note : extra) rows.add ({ note, drums::gmName (note) });
    }

    void rowMenu (int row)
    {
        juce::PopupMenu m;
        const int perBeat = juce::jmax (1, juce::roundToInt (1.0 / gridBeats()));
        auto fill = [this, row] (int every, const juce::String& what)
        {
            return [this, row, every, what]
            {
                clip->edit.getUndoManager().beginNewTransaction (what);
                for (int s = 0; s < numSteps(); ++s) { removeHit (row, s); if (every > 0 && s % every == 0) addHit (row, s, 100); }
                repaint();
            };
        };
        m.addSectionHeader (rows[row].name);
        m.addItem ("Clear this row", fill (0, "Clear drum row"));
        m.addItem ("A hit every beat", fill (perBeat, "Fill drum row"));
        if (perBeat >= 2) m.addItem ("A hit every 1/8", fill (perBeat / 2, "Fill drum row"));
        if (perBeat >= 4) m.addItem ("A hit every 1/16", fill (perBeat / 4, "Fill drum row"));
        m.showMenuAsync ({});
    }

    void cellMenu (int row, int s)
    {
        juce::PopupMenu m;
        auto* n = noteAt (rows[row].note, s);
        for (auto [v, name] : std::initializer_list<std::pair<int, const char*>> { { 127, "Accent (127)" }, { 100, "Normal (100)" }, { 70, "Soft (70)" }, { 40, "Ghost (40)" } })
            m.addItem (name, true, n != nullptr && n->getVelocity() == v, [this, row, s, v = v]
            {
                clip->edit.getUndoManager().beginNewTransaction ("Drum hit velocity");
                if (auto* x = noteAt (rows[row].note, s)) x->setVelocity (v, &clip->edit.getUndoManager());
                else addHit (row, s, v);
                audition (rows[row].note, v);
                repaint();
            });
        if (n != nullptr) m.addItem ("Remove", [this, row, s] { clip->edit.getUndoManager().beginNewTransaction ("Drum hits"); removeHit (row, s); repaint(); });
        m.showMenuAsync ({});
    }

    void audition (int note, int velocity)
    {
        auto* at = clip != nullptr ? dynamic_cast<te::AudioTrack*> (clip->getTrack()) : nullptr;
        if (at == nullptr || ! instruments::isInstrumentTrack (*at)) return;
        stopAudition();
        at->injectLiveMidiMessage (juce::MidiMessage::noteOn (10, note, (juce::uint8) juce::jlimit (1, 127, velocity)), auditionSource);
        lastAuditioned = note;
        auditionUntil = juce::Time::getMillisecondCounter() + 250;
        repaint();
    }

    void stopAudition()
    {
        if (lastAuditioned < 0) return;
        if (auto* at = clip != nullptr ? dynamic_cast<te::AudioTrack*> (clip->getTrack()) : nullptr)
            at->injectLiveMidiMessage (juce::MidiMessage::noteOff (10, lastAuditioned), auditionSource);
        lastAuditioned = -1;
        auditionUntil = 0;
    }

    void timerCallback() override
    {
        if (clip == nullptr || ! clip->state.getParent().isValid())
        {
            if (onClose) juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<DrumGrid> (this)] { if (safe && safe->onClose) safe->onClose(); });
            stopTimer();
            return;
        }
        if (auditionUntil != 0 && juce::Time::getMillisecondCounter() > auditionUntil) stopAudition();
        auto* track = clip->getTrack();
        int hits = clip->getSequence().getNumNotes();
        auto text = "Drums:  " + clip->getName() + (track != nullptr ? "  on  " + track->getName() : juce::String()) + "     " + juce::String (hits) + (hits == 1 ? " hit" : " hits");
        if (title.getText() != text) title.setText (text, juce::dontSendNotification);
        repaint();
    }
};
