// Encore: Basement Studio
// Copyright (C) 2026 Encore Studio. AGPLv3 (see LICENSE).
// Creates the built-in StrumProcessor (JucePlugin_Name comes from CMakeLists.txt).
#include <JuceHeader.h>
#include "../../encore/plugins/Strum/Source/PluginProcessor.h"

juce::AudioPluginInstance* makeEncoreStrum() { return new StrumProcessor(); }
