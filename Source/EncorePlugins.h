// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
//
// ENCORE.'s own plugins (Vox, Verb, Equalize, Master, Strum) built into Basement
// Studio, from encore-vst. They're offered as their own plugin format, "Encore"
// (an internal identifier, which encore-vst's EncoreProcessorBase matches), so the engine treats
// them exactly like any other plugin (parameters, automation, saved state)
// while they load instantly: there's nothing to scan and no file to find.
#pragma once

juce::AudioPluginInstance* makeEncoreVox();
juce::AudioPluginInstance* makeEncoreVerb();
juce::AudioPluginInstance* makeEncoreEQ();
juce::AudioPluginInstance* makeEncoreMaster();
juce::AudioPluginInstance* makeEncoreStrum();

class EncorePluginFormat : public juce::AudioPluginFormat
{
public:
    struct Entry { const char* id; const char* name; juce::AudioPluginInstance* (*make)(); const char* category; };

    /** Which plugin an identifier means: "encore:vox", "encore:equalize"... (what songs save).
        The plugins' own EncoreProcessorBase makes the same identifier from their name. */
    static const Entry* find (const juce::String& id)
    {
        for (auto& e : entries())
            if (id.equalsIgnoreCase (e.id)) return &e;
        return nullptr;
    }

    static const std::array<Entry, 5>& entries()
    {
        static const std::array<Entry, 5> list { {
            { "encore:vox",      "ENCORE. Vox",      &makeEncoreVox,    "Vocal chain" },
            { "encore:verb",     "ENCORE. Verb",     &makeEncoreVerb,   "Reverb" },
            { "encore:equalize", "ENCORE. Equalize", &makeEncoreEQ,     "EQ" },
            { "encore:master",   "ENCORE. Master",   &makeEncoreMaster, "Mastering" },
            { "encore:strum",    "ENCORE. Strum",    &makeEncoreStrum,  "Acoustic guitar" },
        } };
        return list;
    }

    static juce::PluginDescription describe (const Entry& e)
    {
        juce::PluginDescription d;
        d.name = d.descriptiveName = e.name;
        d.pluginFormatName = "Encore";
        d.category = e.category;
        d.manufacturerName = "ENCORE.";
        d.version = ProjectInfo::versionString;
        d.fileOrIdentifier = e.id;
        d.uniqueId = d.deprecatedUid = juce::String (e.id).hashCode();
        d.isInstrument = false;
        d.numInputChannels = d.numOutputChannels = 2;
        return d;
    }

    juce::String getName() const override { return "Encore"; }

    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& results, const juce::String& id) override
    {
        if (auto* e = find (id)) results.add (new juce::PluginDescription (describe (*e)));
    }

    bool fileMightContainThisPluginType (const juce::String& id) override         { return id.startsWith ("encore:"); }
    juce::String getNameOfPluginFromIdentifier (const juce::String& id) override
    {
        if (auto* e = find (id)) return e->name;
        return id;
    }
    bool pluginNeedsRescanning (const juce::PluginDescription&) override          { return false; }
    bool doesPluginStillExist (const juce::PluginDescription& d) override         { return fileMightContainThisPluginType (d.fileOrIdentifier); }
    bool canScanForPlugins() const override                                       { return false; }
    bool isTrivialToScan() const override                                         { return true; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool, bool) override
    {
        juce::StringArray ids;
        for (auto& e : entries()) ids.add (e.id);
        return ids;
    }
    juce::FileSearchPath getDefaultLocationsToSearch() override                   { return {}; }
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const override { return false; }

protected:
    void createPluginInstance (const juce::PluginDescription& d, double sampleRate, int blockSize, PluginCreationCallback callback) override
    {
        if (auto* e = find (d.fileOrIdentifier))
            {
                std::unique_ptr<juce::AudioPluginInstance> p (e->make());
                p->setRateAndBufferSizeDetails (sampleRate, blockSize);
                callback (std::move (p), {});
                return;
            }
        callback (nullptr, "Not one of the ENCORE. plugins: " + d.fileOrIdentifier);
    }
};

/** Makes the ENCORE. plugins available to an engine (and lists them, so they're in every Effects menu) */
inline void registerEncorePlugins (te::Engine& engine)
{
    auto& pm = engine.getPluginManager();
    for (auto* f : pm.pluginFormatManager.getFormats())
        if (f->getName() == "Encore") return;
    pm.pluginFormatManager.addFormat (std::make_unique<EncorePluginFormat>());
    for (auto& e : EncorePluginFormat::entries())
        pm.knownPluginList.addType (EncorePluginFormat::describe (e));
}
