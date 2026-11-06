// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
// Creates the built-in MasterProcessor (JucePlugin_Name comes from CMakeLists.txt).
#include <JuceHeader.h>
#include "../../external/encore-vst/plugins/Master/Source/PluginProcessor.h"

juce::AudioPluginInstance* makeEncoreMaster() { return new MasterProcessor(); }
