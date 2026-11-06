// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
// Creates the built-in EqProcessor (JucePlugin_Name comes from CMakeLists.txt).
#include <JuceHeader.h>
#include "../../external/encore-vst/plugins/EQ/Source/PluginProcessor.h"

juce::AudioPluginInstance* makeEncoreEQ() { return new EqProcessor(); }
