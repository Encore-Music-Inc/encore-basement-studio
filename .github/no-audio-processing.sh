#!/usr/bin/env bash
# Fails when Basement Studio carries audio processing of its own. All of
# ENCORE.'s audio processing lives in encore-audio-processing (its dsp/
# headers, which come with external/encore-vst): the tuner, mastering, loudness,
# dither, the clicks, the drum kits, the built-in effects and instruments,
# levels, meters and the fader law, pan, fades, waveform pictures, resampling,
# the preview A/B, note names and note edits are called from there. JUCE's own
# processing classes and buffer mixing count too, and so do Tracktion's export
# loudness and dither, since using them would be a second version of
# something that repository has. Tracktion's own playback code is patched to
# call it too (patches/tracktion-audio-processing.patch); what stays
# Tracktion's is summing tracks, the timeline, plugin hosting and reading a
# clip backwards, which that repository has no counterpart for.
#
# The exceptions, each with its reason, are the files listed in `allowed`.
set -euo pipefail
cd "$(dirname "$0")/.."

pattern='juce::dsp::(IIR|FIR|FFT|Oversampling|Convolution|StateVariable[A-Za-z]*|LinkwitzRiley[A-Za-z]*|Compressor|Limiter|Reverb|Gain|Panner|WindowingFunction|DelayLine|Chorus|Phaser|LadderFilter|ProcessorDuplicator|BallisticsFilter|NoiseGate|WaveShaper|Bias|DryWetMixer)\b'
pattern+='|\b(SmoothedValue|LinearSmoothedValue|IIRFilter|IIRCoefficients|LagrangeInterpolator|CatmullRomInterpolator|WindowedSincInterpolator|Decibels)\b'
pattern+='|\.(getMagnitude|getRMSLevel|applyGain|applyGainRamp|readMaxLevels)\s*\('
pattern+='|shouldNormaliseByLUFS|ditheringEnabled\s*=\s*(true|p\.)'
# buffer mixing and gain ramps, a waveform picture, a resampler, a fader law by slider skew, a pan law written out
pattern+='|\.(addFrom|addFromWithRamp|copyFromWithRamp|applyGainRamp)\s*\(|juce::AudioThumbnail\b|ResamplingAudioSource'
pattern+='|setSkewFactorFromMidPoint\s*\(\s*-|pan\s*\*\s*gain'
# a gain applied sample by sample by hand
pattern+='|[a-z]\s*\*\s*[a-z]+\.getSample\s*\('

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
