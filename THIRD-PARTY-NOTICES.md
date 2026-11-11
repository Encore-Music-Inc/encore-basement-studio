# Third-party notices

ENCORE. Basement Studio's own code is Copyright (C) 2026 Encore Music, Inc. and
licensed under the GNU Affero General Public License v3 (see `LICENSE`). The
pieces below come from elsewhere and keep their own licences. All of them can
be used in an AGPLv3 program: the GPLv3 parts through section 13 of the GPLv3
and the AGPLv3, the LGPL parts because the whole program's source is available
to rebuild against a changed copy, and the rest are permissive.

## ENCORE. plugins (encore-vst)

- What: the five ENCORE. plugins (Vox, Verb, Equalize, Master, Strum) and what
  they share (`shared/`: `EncoreProcessorBase.h`, `ui/`). Basement Studio
  compiles their processors and editors in.
- Where: git submodule at `external/encore-vst`
  (https://github.com/Encore-Music-Inc/encore-vst), pinned to its release v0.7.0
- Licence: GNU AGPLv3, Copyright (C) Encore Music, Inc. Its
  `external/encore-audio-processing` (and that repo's `external/JUCE`) are the
  audio processing and JUCE below.

## ENCORE. Audio Processing (encore-audio-processing)

- What: ENCORE.'s shared audio processing headers (`dsp/...`), release v2.2.0
- Where: `external/encore-vst/external/encore-audio-processing`, pinned by encore-vst
  (https://github.com/Encore-Music-Inc/encore-audio-processing)
- Licence: GNU AGPLv3, Copyright (C) Encore Music, Inc. Its `external/JUCE`
  is the JUCE below.

## JUCE

- What: the JUCE framework, release 9.0.5 (commit `d60b7a78030661574a615b586d9325ba7a7fa7dd`)
- Where: `external/encore-vst/external/encore-audio-processing/external/JUCE`,
  pinned by encore-audio-processing (https://github.com/juce-framework/JUCE)
- Used by: the whole app, the ENCORE. plugins inside it and the 32-bit plugin bridge
- Licence: dual licensed under the GNU AGPLv3 and the commercial JUCE licence.
  ENCORE. Basement Studio uses JUCE under the AGPLv3, so no commercial JUCE
  licence is needed. The full terms are in `external/encore-vst/external/encore-audio-processing/external/JUCE/LICENSE.md` once the
  submodule is checked out.

### Libraries bundled with JUCE

JUCE vendors further libraries that its modules compile in, depending on the
module and platform. The modules used here (`juce_audio_devices`,
`juce_audio_formats`, `juce_audio_processors`, `juce_audio_utils`, `juce_dsp`,
`juce_gui_extra` and what they depend on) can bring in: zlib (Zlib), libpng
(libpng-2.0), the IJG JPEG library (IJG), libwebp (BSD-3-Clause), FLAC
(BSD-3-Clause), Ogg and Vorbis (BSD-3-Clause), Opus, opusfile and libopusenc
(BSD-3-Clause), HarfBuzz (MIT), SheenBidi (Apache-2.0), PlutoVG and LunaSVG
(MIT), CHOC (ISC), QuickJS (MIT) and Chromium's cubic bezier code
(BSD-3-Clause). The complete inventory, with versions, licences and locations,
is JUCE's software bill of materials: `external/encore-vst/external/encore-audio-processing/external/JUCE/JUCE.spdx.json`.

### Steinberg VST3 SDK

- What: the VST3 SDK 3.8.0, bundled with JUCE
  (`external/encore-vst/external/encore-audio-processing/external/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK`)
- Used by: hosting VST3 plugins
- Licence: MIT. VST is a registered trademark of Steinberg Media Technologies GmbH.

### Steinberg ASIO SDK

- What: the ASIO SDK 2.3 interface headers (`asio.h`, `asiosys.h`,
  `iasiodrv.h`), bundled with JUCE
  (`external/encore-vst/external/encore-audio-processing/external/JUCE/modules/juce_audio_devices/native/asio/`)
- Used by: the Windows build, for audio interfaces' own ASIO drivers
  (`JUCE_ASIO=1` in `CMakeLists.txt`)
- Licence: Steinberg's proprietary ASIO licence or the GNU GPLv3, at the
  user's choice (`external/encore-vst/external/encore-audio-processing/external/JUCE/modules/juce_audio_devices/native/asio/LICENSE.txt`).
  ENCORE. Basement Studio uses it under the GPLv3, which needs no agreement
  with Steinberg. ASIO is a trademark of Steinberg Media Technologies GmbH.

### Apple Audio Unit SDK

- What: AudioUnitSDK 1.1.0, bundled with JUCE
- Used by: hosting Audio Unit plugins on macOS
- Licence: Apache-2.0

JUCE also ships the AAX SDK (Avid's licence or GPLv3) and the LV2 libraries
(ISC). Neither is enabled in this build.

## Tracktion Engine

- What: Tracktion Engine, the audio engine (editing, playback, recording,
  rendering), at commit `a5e44996f5f08dfa6af5f5d02d900fb3cfa37422`
- Where: git submodule at `external/tracktion_engine`
  (https://github.com/Tracktion/tracktion_engine). ENCORE.'s changes to it are
  in `patches/` (per-track monitoring, and its playback calling ENCORE. Audio
  Processing for fades, gains and pan, levels, resampling, time stretching,
  clicks, automation curves and delay compensation), applied when CMake configures.
- Also: `Source/edit/Components.*`, `Source/edit/Utilities.h` and
  `Source/edit/PluginWindow.h` started from Tracktion Engine's example code
  (`examples/common/`), Copyright Tracktion Software Corporation.
- Licence: dual licensed under the GNU GPLv3 (or later) and Tracktion's
  commercial licences. ENCORE. Basement Studio uses it under the GPLv3, so no
  commercial Tracktion licence is needed. See `external/tracktion_engine/LICENSE.md`.

### Libraries bundled with Tracktion Engine

The ones compiled into this build:

| Library | Where (under `external/tracktion_engine/modules/`) | Licence |
|---|---|---|
| CHOC | `3rd_party/choc` | ISC, Copyright Tracktion Corporation |
| crill | `3rd_party/crill` | Boost Software License 1.0 |
| magic_enum | `3rd_party/magic_enum` | MIT, Copyright Daniil Goncharov |
| MPMCQueue (rigtorp) | `3rd_party/rigtorp` | MIT, Copyright Erik Rigtorp |
| rpmalloc | `3rd_party/rpmalloc` | public domain (or MIT), Mattias Jansson |
| tl::expected | `3rd_party/expected` | CC0 1.0 |

Tracktion Engine's other bundled code (Airwindows, SoundTouch, libsamplerate,
its own copy of Signalsmith Stretch, doctest) is switched off or only used by
its own tests. Clip time stretching uses ENCORE. Audio Processing's stretcher
(`patches/tracktion-audio-processing.patch`), whose notices cover Signalsmith Stretch.

## FST

- What: FST 0.177.0 by IOhannes m zmölnig (IEM), a clean-room
  reimplementation of the VST2 plugin interface headers, with three small
  fixes (see `external/fst/README.md`)
- Where: `external/fst`
- Used by: hosting VST2 plugins, and the 32-bit plugin bridge on Windows
- Licence: GNU GPLv3 or later (`external/fst/LICENSE`). ENCORE. Basement
  Studio is not a Steinberg product and is not "VST Compatible" in
  Steinberg's trademark sense.

## LAME

- What: libmp3lame from LAME 3.100 (https://lame.sourceforge.io), the MP3
  encoder, unchanged apart from our own `config.h` (see `external/lame/README-basement.md`)
- Where: `external/lame`, built as its own static library
- Used by: MP3 export
- Licence: GNU LGPL 2 or later (`external/lame/COPYING`, `external/lame/LICENSE`).
  Anyone can rebuild ENCORE. Basement Studio against a different or changed
  LAME from this repository's source, which is what the LGPL asks.

## LADSPA

- What: the LADSPA plugin header (`ladspa.h`), from the system's `ladspa-sdk`
  package at build time
- Used by: hosting LADSPA plugins on Linux
- Licence: GNU LGPL 2.1 or later

## Linux packages

The AppImage is made with linuxdeploy (MIT, https://github.com/linuxdeploy/linuxdeploy)
and carries copies of some of the system libraries the app links (for
example FreeType and fontconfig), each under its own licence as published by
its distribution. The .deb carries none: it depends on the system's packages.
Its `/usr/share/doc/encore-basement-studio/copyright` lists the licences above.
