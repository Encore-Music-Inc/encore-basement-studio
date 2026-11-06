// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

#pragma once

// Help menu support: checking for a newer Basement Studio (the releases on
// GitHub), and Report a Problem: a zip of the log, settings and what Basement
// Studio saw, and an email to ENCORE. support to send it with.

namespace support
{
    inline const char* repo() { return "Encore-Music-Inc/encore-basement-studio"; }
    inline const char* email() { return "support@encoremusic.studio"; }

    /** Percent-encodes text for a mailto: link (UTF-8; everything but letters, digits and -._~) */
    inline juce::String mailtoEscape (const juce::String& text)
    {
        juce::String out;
        for (auto* p = text.toRawUTF8(); *p != 0; ++p)
        {
            const auto c = (unsigned char) *p;
            if (juce::CharacterFunctions::isLetterOrDigit ((char) c) && c < 128) out << (char) c;
            else if (c == '-' || c == '.' || c == '_' || c == '~') out << (char) c;
            else out << "%" << juce::String::toHexString ((int) c).paddedLeft ('0', 2).toUpperCase();
        }
        return out;
    }

    /** The mailto: link for an email to ENCORE. support */
    inline juce::String mailtoLink (const juce::String& subject, const juce::String& body)
    {
        return juce::String ("mailto:") + email() + "?subject=" + mailtoEscape (subject) + "&body=" + mailtoEscape (body.replace ("\r\n", "\n").replace ("\n", "\r\n"));
    }

    /** Opens a new email to ENCORE. support in the person's mail app */
    inline bool emailSupport (const juce::String& subject, const juce::String& body)
    {
        return juce::Process::openDocument (mailtoLink (subject, body), {});
    }

    /** What the problem email starts with: the version, the operating system and where the log is */
    inline juce::String reportEmailBody (const juce::String& whatHappened, const juce::File& zip)
    {
        juce::String b;
        b << "What happened:\n" << (whatHappened.isNotEmpty() ? whatHappened : juce::String ("\n")) << "\n\n"
          << "ENCORE. Basement Studio " << ProjectInfo::versionString << "\n"
          << "Operating system: " << juce::SystemStats::getOperatingSystemName() << (juce::SystemStats::isOperatingSystem64Bit() ? " (64-bit)" : "") << "\n"
          << "Log file: " << studioLogFile().getFullPathName() << "\n";
        if (zip != juce::File())
            b << "\nPlease attach " << zip.getFileName() << " from your Desktop (" << zip.getFullPathName() << "). "
              << "It has the log and your audio setup, no recordings.\n";
        return b;
    }

    //==============================================================================
    /** "v0.4.1" or "0.4.1" as numbers, for comparing */
    inline juce::Array<int> versionNumbers (juce::String v)
    {
        v = v.trim().trimCharactersAtStart ("vV");
        juce::Array<int> n;
        for (auto& part : juce::StringArray::fromTokens (v.upToFirstOccurrenceOf ("-", false, false), ".", {}))
            n.add (part.getIntValue());
        while (n.size() < 3) n.add (0);
        return n;
    }

    inline bool isNewer (const juce::String& candidate, const juce::String& current)
    {
        auto a = versionNumbers (candidate), b = versionNumbers (current);
        for (int i = 0; i < juce::jmax (a.size(), b.size()); ++i)
            if (a[i] != b[i]) return a[i] > b[i];
        return false;
    }

    /** How well a release file suits this computer: 0 not at all, higher is better.
        Windows: the installer, else the zip. Mac: the disk image. Linux: the package for
        this machine's architecture (amd64 / x86_64 or arm64 / aarch64): the AppImage when
        Basement Studio is running as one, else the .deb. */
    inline int downloadRank (const juce::String& name)
    {
       #if JUCE_WINDOWS
        if (! name.containsIgnoreCase ("windows")) return 0;
        return name.endsWithIgnoreCase ("setup.exe") ? 2 : name.endsWithIgnoreCase (".zip") ? 1 : 0;
       #elif JUCE_MAC
        return name.containsIgnoreCase ("macos") && name.endsWithIgnoreCase (".dmg") ? 2 : 0;
       #elif JUCE_LINUX
        #if defined (__aarch64__)
         const char* debArch = "_arm64.deb"; const char* imageArch = "-aarch64.AppImage";
        #elif defined (__x86_64__)
         const char* debArch = "_amd64.deb"; const char* imageArch = "-x86_64.AppImage";
        #else
         const char* debArch = nullptr; const char* imageArch = nullptr;
        #endif
        if (debArch == nullptr) return 0;
        const bool runningAsAppImage = juce::SystemStats::getEnvironmentVariable ("APPIMAGE", {}).isNotEmpty();
        if (name.endsWithIgnoreCase (debArch))   return runningAsAppImage ? 1 : 2;
        if (name.endsWithIgnoreCase (imageArch)) return runningAsAppImage ? 2 : 1;
        return 0;
       #else
        juce::ignoreUnused (name);
        return 0;
       #endif
    }

    struct Release
    {
        juce::String version, page, download, notes;
        bool found = false;
        juce::String problem;   // couldn't check (offline...)
    };

    /** The newest published release on GitHub (prereleases count: that's what Studio ships as for now).
        Runs the request on a background thread; answers on the message thread. */
    inline void latestRelease (std::function<void (Release)> answer)
    {
        juce::Thread::launch ([answer]
        {
            Release r;
            juce::URL url ("https://api.github.com/repos/" + juce::String (repo()) + "/releases?per_page=10");
            int status = 0;
            auto in = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                                .withExtraHeaders ("User-Agent: ENCORE-Basement-Studio\r\nAccept: application/vnd.github+json")
                                                .withConnectionTimeoutMs (8000)
                                                .withStatusCode (&status));
            if (in == nullptr || status != 200)
                r.problem = in == nullptr ? "couldn't reach GitHub (offline?)" : "GitHub answered " + juce::String (status);
            else
            {
                const auto json = juce::JSON::parse (in->readEntireStreamAsString());
                if (auto* list = json.getArray())
                    for (auto& rel : *list)
                    {
                        if ((bool) rel["draft"]) continue;
                        const auto tag = rel["tag_name"].toString();
                        if (r.found && ! isNewer (tag, r.version)) continue;
                        r.found = true;
                        r.version = tag.trimCharactersAtStart ("vV");
                        r.page = rel["html_url"].toString();
                        r.notes = rel["body"].toString();
                        r.download = {};
                        int best = 0;
                        if (auto* assets = rel["assets"].getArray())
                            for (auto& a : *assets)
                            {
                                // the best file for this computer (see downloadRank); .sha256 files never match
                                const int rank = downloadRank (a["name"].toString());
                                if (rank > best) { best = rank; r.download = a["browser_download_url"].toString(); }
                            }
                    }
                if (! r.found) r.problem = "no releases listed";
            }
            juce::MessageManager::callAsync ([answer, r] { answer (r); });
        });
    }

    //==============================================================================
    /** Builds "ENCORE Basement Studio problem report <date>.zip" on the Desktop: what happened (in their words),
        the computer and audio setup, the log, the settings, and optionally the open song's
        arrangement file (no audio). Returns the zip, or an empty File. */
    inline juce::File makeReport (const juce::String& whatHappened, const juce::String& systemInfo, const juce::File& songFile)
    {
        const auto now = juce::Time::getCurrentTime();
        auto zip = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                       .getNonexistentChildFile ("ENCORE Basement Studio problem report " + now.formatted ("%Y-%m-%d %H%M"), ".zip", false);
        juce::ZipFile::Builder b;
        juce::String readme;
        readme << "ENCORE. Basement Studio problem report" << juce::newLine
               << "Made " << now.toString (true, true) << juce::newLine << juce::newLine
               << "What happened:" << juce::newLine << (whatHappened.isNotEmpty() ? whatHappened : juce::String ("(not filled in)")) << juce::newLine << juce::newLine
               << systemInfo;
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("studio-report-" + juce::String (juce::Random::getSystemRandom().nextInt64()));
        tmp.createDirectory();
        auto report = tmp.getChildFile ("report.txt");
        report.replaceWithText (readme);
        b.addFile (report, 9, "report.txt");
        auto folder = settingsFolder();
        if (studioLogFile().existsAsFile()) b.addFile (studioLogFile(), 9, studioLogFile().getFileName());
        for (auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.settings;*.xml"))
            if (f.getSize() < 2000000) b.addFile (f, 9, "settings/" + f.getFileName());
        if (songFile.existsAsFile()) b.addFile (songFile, 9, "song/" + songFile.getFileName());
        bool ok = false;
        {
            juce::FileOutputStream out (zip);
            ok = out.openedOk() && b.writeToStream (out, nullptr);
        }
        tmp.deleteRecursively();
        if (! ok) { zip.deleteFile(); return {}; }
        return zip;
    }

    /** Help > Report a Problem: say what happened, then the zip is made and an email to ENCORE. support opens */
    class ReportComponent : public juce::Component
    {
    public:
        std::function<void (juce::String whatHappened, bool includeSong)> onMake;

        ReportComponent (bool hasSong, bool crashedLastTime)
        {
            hubui::styleLabel (intro, 14.0f, theme::paper);
            intro.setText (juce::String (crashedLastTime ? "ENCORE. Basement Studio didn't close properly last time. Sorry about that.\n" : "")
                             + "Tell us what happened. ENCORE. Basement Studio makes a zip on your Desktop with this, its log and your audio setup "
                               "(no recordings), then opens an email to " + juce::String (email()) + " to send it with.",
                           juce::dontSendNotification);
            intro.setJustificationType (juce::Justification::topLeft);
            addAndMakeVisible (intro);
            hubui::styleEditor (what);
            what.setMultiLine (true, true);
            what.setReturnKeyStartsNewLine (true);
            what.setTextToShowWhenEmpty ("What were you doing, and what went wrong?", theme::muted);
            addAndMakeVisible (what);
            song.setButtonText ("Include the open song's arrangement (tracks and effects, no audio)");
            song.setToggleState (hasSong, juce::dontSendNotification);
            song.setEnabled (hasSong);
            song.setColour (juce::ToggleButton::textColourId, theme::paper);
            song.setColour (juce::ToggleButton::tickColourId, theme::accent);
            addAndMakeVisible (song);
            make.setColour (juce::TextButton::buttonColourId, theme::brand);
            make.onClick = [this]
            {
                if (onMake) onMake (what.getText().trim(), song.getToggleState());
                close();
            };
            cancel.onClick = [this] { close(); };
            addAndMakeVisible (make);
            addAndMakeVisible (cancel);
            setSize (560, 360);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::surface); }
        void resized() override
        {
            auto r = getLocalBounds().reduced (24, 20);
            intro.setBounds (r.removeFromTop (64));
            r.removeFromTop (6);
            auto b = r.removeFromBottom (36);
            make.setBounds (b.removeFromRight (150));
            b.removeFromRight (8);
            cancel.setBounds (b.removeFromRight (90));
            r.removeFromBottom (10);
            song.setBounds (r.removeFromBottom (26));
            r.removeFromBottom (8);
            what.setBounds (r);
        }

    private:
        juce::Label intro;
        juce::TextEditor what;
        juce::ToggleButton song;
        juce::TextButton make { "Make the report" }, cancel { "Cancel" };
        void close() { if (auto* d = findParentComponentOfClass<juce::DialogWindow>()) d->exitModalState (0); }
    };
}
