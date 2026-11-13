# ENCORE. Basement Studio

A DAW for bands and home studios, from [ENCORE.](https://encoremusic.studio).
Record, edit and mix on your own computer, with the ENCORE. plugins built in,
then make a release package for ENCORE. (File > Send to ENCORE.: the mastered
mix, stems, cover art and the song's details, ready to upload in the browser).
Runs on Windows, macOS, Linux and Raspberry Pi (ENCORE. OS, or 64-bit
Raspberry Pi OS).

Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU Affero General
Public License v3 (see `LICENSE`). The ENCORE. name and logo are not licensed
(see `TRADEMARKS.md`). Third-party code: `THIRD-PARTY-NOTICES.md`. Outside
contributions are not accepted (see `CONTRIBUTING.md`).
Contact: support@encoremusic.studio

Built on [Tracktion Engine](https://github.com/Tracktion/tracktion_engine)
(GPLv3) and [JUCE](https://juce.com) (AGPLv3).

## Downloads

Each release (under Releases on this repo) has, with a `.sha256` file next to
each download:

- **Windows 10 and 11 (64-bit)**: `ENCORE-Basement-Studio-<version>-windows-setup.exe`,
  the installer (Start menu, desktop shortcut, opens .encore songs, and it can
  also install ENCORE. VST for your other DAWs). Or
  `ENCORE-Basement-Studio-<version>-windows-x64.zip` to run it from any folder
  without installing. It isn't code-signed, so SmartScreen warns the
  first time: click **More info**, then **Run anyway**.
- **Mac** (Apple Silicon and Intel, macOS 11 or newer):
  `ENCORE-Basement-Studio-<version>-macos.dmg`. Open it and drag
  `ENCORE Basement Studio` to Applications. Double-clicking a `.encore` song
  then opens it in ENCORE. Basement Studio. The disk image also holds
  ENCORE. VST's Mac installer (`ENCORE-VST-<version>-macos.pkg`), for the same
  plugins in Logic, GarageBand and your other music apps; it asks for Open
  Anyway the same way the first time. It isn't signed or notarized, so
  the first time macOS says it was "Not Opened": click **Done** (not Move to
  Trash), go to **System Settings > Privacy & Security**, scroll to Security
  and click **Open Anyway**, then open the app again and click **Open Anyway**.
  You only do this once. If it still won't open, run
  `xattr -dr com.apple.quarantine "/Applications/ENCORE Basement Studio.app"`
  in Terminal.
- **Linux and Raspberry Pi**: `encore-basement-studio_<version>_amd64.deb` and
  `_arm64.deb`, and `ENCORE-Basement-Studio-<version>-x86_64.AppImage` and
  `-aarch64.AppImage`. See below. For the ENCORE. plugins in your other music
  apps (Ardour, Reaper, Bitwig and others), get ENCORE. VST's Linux download
  from [its releases](https://github.com/Encore-Music-Inc/encore-vst/releases).

Versions 0.4.3 and earlier were published from a different repository.

### Install on Linux / Raspberry Pi

Runs on 64-bit Raspberry Pi OS Bookworm or newer (and ENCORE. OS), Debian 12
or newer, and Ubuntu 22.04 or newer (plus Mint, Pop!_OS and others built on
them). Pick the file for your machine: **amd64** / **x86_64** for regular PCs
and laptops, **arm64** / **aarch64** for a Raspberry Pi 4 or 5 running a
64-bit system (32-bit isn't supported).

- **The .deb** (Ubuntu, Debian, Mint, Pop!_OS, Raspberry Pi OS): from the
  folder you downloaded it to:

  ```bash
  sudo apt install ./encore-basement-studio_<version>_arm64.deb
  ```

  ENCORE. Basement Studio is then in your applications menu (under Sound &
  Video), `encore-basement-studio` starts it from a terminal, and .encore
  songs open in it with a double-click. Remove it with
  `sudo apt remove encore-basement-studio`.
- **The AppImage** (almost any distribution, nothing to install): make it
  runnable and start it:

  ```bash
  chmod +x ENCORE-Basement-Studio-*.AppImage
  ./ENCORE-Basement-Studio-*.AppImage
  ```

  If it says FUSE is missing, install `libfuse2` (`libfuse2t64` on Ubuntu
  24.04 and later), or run it with `--appimage-extract-and-run`.

Audio works through ALSA out of the box. JACK is supported too, and so is
PipeWire through
its JACK support. For low latency, add yourself to the `audio` group
(`sudo usermod -aG audio $USER`, then log out and back in).

## Help and problems

In the app, Help > Report a Problem (or "Report a problem" on the start screen)
makes a zip of the log and your audio setup (no recordings) on your Desktop and
opens an email to support@encoremusic.studio with the version, your system and
where the log is: attach the zip and send it. Or write to
support@encoremusic.studio directly. More at https://encoremusic.studio.

## What it does

- **Hub**: ENCORE. Basement Studio opens on a start page. Pick up a recent song or set up a
  new one: start from a template (Blank, Singer-songwriter, Full band, Live
  multitrack, Podcast, Mix stems, or your own), fill in title, artist, album,
  genre, tempo, time signature, key and notes, choose the interface you record
  from, its sample rate and bit depth, how many tracks to start with and which
  input feeds each one.
- **Record** any number of tracks at once. Every input on your interface is
  its own input. Level meters on every track show the input while it's armed
  (with a clip light), a headphones button lets you hear yourself, the take's
  waveform draws as you play, and a metronome with a count-in you can see.
  Loop a section and record over it, or punch in on just that part.
- **Your interface's own driver**: on Windows, ENCORE. Basement Studio uses the interface's
  ASIO driver when it has one (low latency, no Windows audio settings in the
  way) and switches to it by itself the first time.
- **Edit**: snap to the grid (bar, beat, 1/8, 1/16 or off; hold Shift to
  ignore it), trim clips from either edge, drag fades from their top corners
  (overlapping clips crossfade), drag clips between tracks, box-select,
  Shift-click, nudge, cut, copy, paste, duplicate, split, clip gain,
  normalize, reverse, mute, takes, and right-click any clip for all of it.
  Markers along the ruler (Intro, Verse, Chorus...) to jump around the song.
- **Mix**: a mixer (F3) with a channel strip per track (effects, sends,
  pan, mute, solo, record, fader and meter), group tracks (a whole drum kit
  or all the guitars on one fader: Ctrl-click track headers, then Ctrl+G),
  FX tracks that several tracks can send to (one shared reverb or delay), and
  a master strip with effects
  on the whole mix. Volume and pan automation drawn right on the tracks
  (press A). Effects on any track:
  - **The ENCORE. plugins, built in**: ENCORE. Vox (a whole vocal chain with tune
    and harmony), Verb, Equalize, Master and Strum (the acoustic guitar fixer).
    Nothing to install.
  - **Your plugins**: ENCORE. Basement Studio finds your plugins by itself: VST3
    and VST2 everywhere, Audio Units on a Mac, and LV2 and LADSPA on Linux (in
    the background, each checked in a separate helper process so a broken plugin
    can't crash it, and one that stops to show a window, like an activation
    prompt, is set aside) and lists them by maker. A plugin installed in several
    formats is listed once, in the first of VST3, Audio Unit, LV2, VST2 and
    LADSPA. Add more plugin folders in Preferences.
  - ENCORE.'s own effects, built in: Compressor, Limiter, Delay, Chorus,
    Phaser, Pitch Shifter and Filter. Tracktion Engine's own built-in effects
    aren't included: a song made with an earlier version that used them opens
    without them.
- **Instruments and MIDI**: Project > Add Instrument Track with any of your
  instrument plugins (drums, keys, samplers) or ENCORE. Synth, played
  and recorded from your MIDI keyboard or pads; import MIDI files too.
  A piano roll (double-click a MIDI clip): click to add notes, drag to move or
  resize, a velocity strip, grid snap, quantize, transpose and velocity
  (also in the MIDI menu for whole clips).
- **Templates for real setups**: band templates sized for 2, 4 and 8-input
  interfaces (drums on every input first, then the rest overdubbed), with the
  tracks already grouped; each one says whether it fits your interface.
- **Export** the mix or every track as its own file (stems), the whole song
  or just the loop, as WAV (16, 24 or 32-bit float), FLAC or MP3, at any sample
  rate, with or without the master effects, optionally normalised to -14 LUFS
  for streaming. Or zip the whole song folder.
- **Send to ENCORE.** (File > Export): a release package for ENCORE. Music: the
  master, stems if you want them, cover art and the song's details, which you
  then upload on encoremusic.studio.
- **Safe**: songs save as one folder each, ENCORE. Basement Studio keeps an automatic copy
  as you work, and offers it back if it ever closes unexpectedly.
- **Tuner** (T): a chromatic tuner on any input, 30 Hz to 1.5 kHz (bass
  low B to the top of a guitar), with an adjustable A4.
- Menus like a real DAW (File, Edit, Project, Audio, MIDI, Score, Media,
  Transport, Studio, Window, Help), resizable tracks, taller waveforms for
  quiet recordings (Ctrl+Shift+Up), and a status bar with your interface,
  latency and CPU.

Keys: Space play/stop, R record, L loop, C click, Home back to the start,
Ctrl+S save, Ctrl+N new song, Ctrl+O open, Ctrl+T add track, Ctrl+I import,
Ctrl+E export, Alt+X split, Ctrl+Left/Right nudge, Insert adds a marker,
Shift+Left/Right jumps between markers, Ctrl+Z / Ctrl+Y undo / redo.
Help > Keyboard Shortcuts has the full list.

## What is here

```
Source/                             the app (Main.cpp, MainComponent.h, Hub.h, ...)
Source/edit/                        the arrange view, mixer, piano roll, tuner
Source/encore/                      creates the built-in ENCORE. plugins (Make<Name>.cpp)
Source/bridge/                      the 32-bit plugin bridge (Windows)
Assets/                             the ENCORE. icon
installer/                          Windows installer (Inno Setup), Linux .deb and AppImage
patches/                            ENCORE.'s changes to Tracktion Engine, applied by CMake
tools/                              Windows plugin window test
external/encore-vst                 the ENCORE. plugins (submodule), with the audio processing and JUCE it pins
external/tracktion_engine           Tracktion Engine (submodule)
external/fst                        FST, the VST2 interface headers (GPLv3)
external/lame                       LAME's MP3 encoder library (LGPL)
```

## Pinned releases

Two pins. **encore-vst** decides the plugins, the audio processing and JUCE:
Basement Studio uses the encore-audio-processing that encore-vst pins, and the
JUCE that pins, so it builds exactly what that ENCORE. VST release was built
and tested with, and its Windows installer bundles that same release's
installer. **Tracktion Engine** is the other. CI checks that each row is a
published release and is exactly what the folder is pinned to
(`.github/pins/check.sh`); an exception says why.

<!-- pinned-releases:start -->
| Folder | Repository | Release |
|---|---|---|
| `external/encore-vst` | [encore-vst](https://github.com/Encore-Music-Inc/encore-vst) | `v0.7.1` |
| `external/encore-vst/external/encore-audio-processing` | [encore-audio-processing](https://github.com/Encore-Music-Inc/encore-audio-processing), pinned by encore-vst (shown, not pinned here) | `v2.3.0` |
| `external/encore-vst/external/encore-audio-processing/external/JUCE` | [JUCE](https://github.com/juce-framework/JUCE), pinned by encore-audio-processing (shown, not pinned here) | `9.0.5` |
| `external/tracktion_engine` | [Tracktion Engine](https://github.com/Tracktion/tracktion_engine) | commit `a5e4499` (exception: no Tracktion Engine release supports JUCE 9. Move to the first release that does) |
<!-- pinned-releases:end -->

Basement Studio builds the five plugins from encore-vst's own sources, with
encore-vst's `shared/` and the audio processing on the include path (so
`#include "ui/Theme.h"` and `#include "dsp/MasterChain.h"` resolve), so it runs
exactly the plugin and audio processing code the products ship.

Fetch the pinned folders by path, one level at a time, never with
`--recursive` (Tracktion Engine has its own submodules, not used here):

```bash
git clone https://github.com/Encore-Music-Inc/encore-basement-studio.git
cd encore-basement-studio
git submodule update --init --depth 1 external/encore-vst external/tracktion_engine
git -C external/encore-vst submodule update --init --depth 1 external/encore-audio-processing
git -C external/encore-vst/external/encore-audio-processing submodule update --init --depth 1 external/JUCE
```

To use a new ENCORE. VST release (and the audio processing and JUCE it pins),
check out its tag, update the table above, and commit both in a pull request
titled "Use encore-vst vX.Y.Z (was vA.B.C)":

```bash
git -C external/encore-vst fetch --depth 1 origin tag vX.Y.Z
git -C external/encore-vst checkout vX.Y.Z
git -C external/encore-vst submodule update --init --depth 1 external/encore-audio-processing
git -C external/encore-vst/external/encore-audio-processing submodule update --init --depth 1 external/JUCE
git add external/encore-vst README.md
```

## Build

Needs CMake (the minimum is in `CMakeLists.txt`), Ninja and a C++20 compiler.

### Windows

Visual Studio 2022 (or its Build Tools) with the C++ workload. From a
"x64 Native Tools Command Prompt":

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The 32-bit plugin bridge is its own 32-bit build (from an "x86 Native Tools
Command Prompt"); it goes beside `ENCORE Basement Studio.exe`:

```bat
cmake -B build32 -G Ninja -DCMAKE_BUILD_TYPE=Release -DSTUDIO_BRIDGE_ONLY=ON
cmake --build build32
```

The installer, `dist\ENCORE-Basement-Studio-<version>-windows-setup.exe`, needs
Inno Setup 6 and both builds: `ISCC.exe installer\EncoreBasementStudio.iss`. To offer
ENCORE. VST in it, put an ENCORE. VST Setup at `build\vst\ENCORE-VST-setup.exe`
first (CI downloads the newest one from the encore-vst releases).

### macOS

Xcode (its command line tools), plus `brew install ninja`. One app for Apple
Silicon and Intel:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
cmake --build build
```

### Linux and Raspberry Pi (x64 or arm64)

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libxi-dev
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The .deb and the AppImage (needs `dpkg-dev`, `librsvg2-bin`, `curl` and `file`):

```bash
bash installer/linux/package.sh "build/EncoreBasementStudio_artefacts/Release/ENCORE Basement Studio" dist all
```

The app ends up in `build/EncoreBasementStudio_artefacts/Release/`, as
`ENCORE Basement Studio` (`.exe` on Windows, `.app` on a Mac). File and folder
names leave out the period of ENCORE.; everything people read says
ENCORE. Basement Studio.

Settings, the log (`ENCORE Basement Studio.log`), templates and click sounds
are in `ENCORE/Basement Studio` in the system's app data folder
(`%APPDATA%\ENCORE\Basement Studio` on Windows, `~/Library/ENCORE/Basement Studio`
on a Mac, `~/.config/ENCORE/Basement Studio` on Linux). New songs go in
`Music/ENCORE Basement Studio` unless you pick another folder.

## Tests

- `--selftest <folder>`: makes songs, imports, mixes, exports, reopens, and
  runs each built-in ENCORE. plugin, without a window or an audio device
  (exit code 0 = all good, details in `<folder>/selftest.log`). CI runs it on
  every build. On Linux without a display, run it under `xvfb-run -a`.
- `--plugintest <report file> [folder]`: loads every plugin Basement Studio
  looks for (VST3 and VST2; Audio Units on a Mac; LV2 and LADSPA on Linux) in a
  folder (default: the standard plugin folders) the way the Effects menu does,
  and reports what happened with each one. CI runs it on Linux with the ENCORE.
  VST release the pin points to, as LV2 and VST3.
- `--audiotest <folder>`: monitoring, meters and recording against a pretend
  four-input interface.
- `--profile <name>`: keeps settings separately (in `ENCORE/<name>`), for
  testing without touching your own.

The plugins' and the audio processing's own test suites run in encore-vst and
encore-audio-processing. CI also runs `ReferenceTests` (a target here), which
checks encore-audio-processing's reference set against its headers as this
repository compiles them, and `.github/no-audio-processing.sh`, which fails if
Basement Studio grows audio processing of its own: all of it comes from
encore-audio-processing.

## CI

`.github/workflows/ci.yml`. Each push starts one run (push for `main` and `v*`
tags, plus pull requests, manual runs and a scheduled run).

| When | What runs |
|---|---|
| Pull request | in parallel: Linux x64, Windows x64 (with the 32-bit plugin bridge) and macOS on Apple Silicon: the app and its self-test, and on Linux ENCORE. VST's LV2 and VST3 plugins loaded in it. No installers or packages |
| Push to `main`, or a manual run | in parallel: Windows x64 (installer and zip), macOS (dmg), Linux x64 and arm64 (.deb and AppImage, plus the audio test), each with the self-test; then the Linux packages installed and self-tested on each system `ci.yml` lists. The files, each with a `.sha256`, are kept as workflow artifacts |
| On the schedule in `ci.yml`, or a manual run with `checks: weekly` | the pull request checks, plus the app built with AddressSanitizer and UndefinedBehaviorSanitizer on Linux x64 running its self-test (memory errors and undefined behaviour). Undefined behaviour is reported and the run carries on, so one run lists every finding; a memory error stops it. Exceptions, each with its reason, are in `.github/sanitizers/ubsan.supp`. A failure shows as a red run under Actions |
| `v*` tag | no build: the files from the successful `main` build of the tag's commit are attached to the Release |

- A pull request or push that only changes `*.md`, `LICENSE` or
  `.github/CODEOWNERS` skips the builds (skipped counts as passing). So does a
  pull request that only changes the version line in `CMakeLists.txt` and
  `CHANGELOG.md`; the push to `main` after merging builds everything.
- "CI passed" is the one check to require: it fails if any build or test job
  failed or was cancelled.
- Linux builds inside Debian 12 (so the packages run on Bookworm and newer),
  and arm64 builds on GitHub's native arm64 runner.
- Pull requests build the Mac app for Apple Silicon only; `main` builds it
  universal.
- Every action is pinned to a commit, with its version in a comment.
  Dependabot proposes updates (`.github/dependabot.yml`).

### Build speed

- Ninja on every platform, all cores.
- A compiler cache: ccache on Linux and macOS, sccache on Windows, kept between
  runs with `actions/cache`, keyed on OS, architecture, compiler and the JUCE
  and Tracktion pins.
- No full LTO (JUCE's `juce_recommended_lto_flags` is not used).
  `-DENCORE_LTO=THIN` turns on ThinLTO (Clang) or incremental LTCG (MSVC), the
  same switch as encore-vst's; it is off.
- Submodules are checked out at depth 1, never recursively.

## Releases

Versions are `vMAJOR.MINOR.PATCH`. The version lives in `CMakeLists.txt`
(`project(EncoreBasementStudio VERSION ...)`) and nowhere else: the app's About box
and start screen, the Windows installer (through `build/version.iss`, written
by CMake), the Linux packages and the Mac disk image all read it from there.
Change it on `main` (through a pull request) before releasing. CI refuses a tag
that doesn't match it.

Each release's notes are written in `CHANGELOG.md` in the same pull request
that changes the version; CI fails until the version has its section.

A person at Encore Music, Inc. creates each release in GitHub: Releases, Draft
a new release, choose a new tag `vX.Y.Z` on `main`, leave the title and
description empty, Publish. The tag starts CI, which doesn't rebuild: it takes
the files from the successful `main` build of that exact commit, attaches them
to the Release, and sets its title and notes from `CHANGELOG.md`. If that commit has no
successful `main` build (it's still running, or the push only changed docs and
skipped the builds), the release job fails and says so: run CI by hand on
`main` (Actions, CI, Run workflow), then re-run the release job. Workflow
artifacts expire, so release a commit while its build's files are still there.
