#!/usr/bin/env bash
# Fails when Basement Studio carries audio processing of its own. All of
# ENCORE.'s audio processing lives in encore-audio-processing (its dsp/
# headers, which come with external/encore-vst): the tuner, mastering, loudness,
# dither, the clicks, the drum kits, levels and note names are called from
# there. JUCE's own processing classes count too, and so do Tracktion's
# export loudness and dither, since using them would be a second version of
# something that repository has. Tracktion's playback engine (mixing, plugin
# hosting, its meters) is the DAW itself and isn't matched.
#
# The exceptions, each with its reason, are the files listed in `allowed`.
set -euo pipefail
cd "$(dirname "$0")/.."

pattern='juce::dsp::(IIR|FIR|FFT|Oversampling|Convolution|StateVariable[A-Za-z]*|LinkwitzRiley[A-Za-z]*|Compressor|Limiter|Reverb|Gain|Panner|WindowingFunction|DelayLine|Chorus|Phaser|LadderFilter|ProcessorDuplicator|BallisticsFilter|NoiseGate|WaveShaper|Bias|DryWetMixer)\b'
pattern+='|\b(SmoothedValue|LinearSmoothedValue|IIRFilter|IIRCoefficients|LagrangeInterpolator|CatmullRomInterpolator|WindowedSincInterpolator|Decibels)\b'
pattern+='|\.(getMagnitude|getRMSLevel|applyGain|applyGainRamp|readMaxLevels)\s*\('
pattern+='|shouldNormaliseByLUFS|ditheringEnabled\s*=\s*(true|p\.)'

# Test pages, not the app's sound: they make test tones and measure the raw
# signals of an interface, a plugin or an export to check them
allowed=(
  Source/SelfTest.h
  Source/InterfaceTest.h
  Source/AudioTest.h
  Source/PluginTest.h
)

args=()
for f in "${allowed[@]}"; do args+=(--exclude="$(basename "$f")"); done
if hits=$(grep -rnE "$pattern" Source --include='*.cpp' --include='*.h' "${args[@]}"); then
    echo "$hits"
    echo "::error::Audio processing in Basement Studio. Call encore-audio-processing's headers instead (and when it lacks something, add it there first)."
    exit 1
fi
echo "No audio processing outside encore-audio-processing."
