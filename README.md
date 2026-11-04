# ENCORE. Basement Studio

A free, open-source DAW for bands and home studios, from [ENCORE.](https://encoremusic.studio)
Record, edit and mix on your own computer, with the ENCORE. plugins built in,
and (soon) send finished songs straight to your ENCORE. account.

ENCORE. Basement Studio is built on [Tracktion Engine](https://github.com/Tracktion/tracktion_engine)
and [JUCE](https://juce.com).

## Download

From [Releases](https://github.com/jimmyrlondo/encore-studio/releases):

- **Windows**: `Encore-Studio-<version>-windows-setup.exe`, the installer
  (Start menu, desktop shortcut, opens .encore songs). Or
  `Encore-Studio-windows.zip` to run it from any folder without installing.
  It isn't code-signed yet, so SmartScreen warns the first time: click
  **More info**, then **Run anyway**.
- **Mac** (Apple Silicon and Intel): `Encore-Studio-<version>-macos.dmg`. Open
  it and drag `Encore Studio` to Applications. It isn't notarized yet, so the
  first time right-click it, choose **Open**, then **Open** again.
- **Linux and Raspberry Pi**: see below.

### Install on Linux / Raspberry Pi

Runs on 64-bit Raspberry Pi OS Bookworm or newer, Debian 12 or newer, and
Ubuntu 22.04 or newer (plus Mint, Pop!_OS and others built on them). Pick the
file for your machine: **amd64** / **x86_64** for regular PCs and laptops,
**arm64** / **aarch64** for a Raspberry Pi 4 or 5 running 64-bit Raspberry
Pi OS (the 32-bit version isn't supported).

- **The .deb** (Ubuntu, Debian, Mint, Pop!_OS, Raspberry Pi OS):
  download `encore-basement-studio_<version>_amd64.deb` (or `_arm64.deb`),
  then in a terminal, from the folder you downloaded it to:

  ```
  sudo apt install ./encore-basement-studio_<version>_arm64.deb
  ```

  ENCORE. Basement Studio is then in your applications menu (under Sound &
  Video), `encore-basement-studio` starts it from a terminal, and .encore
  songs open in it with a double-click. Remove it with
  `sudo apt remove encore-basement-studio`.
- **The AppImage** (almost any distribution, nothing to install):
  download `Encore-Basement-Studio-<version>-x86_64.AppImage` (or
  `-aarch64.AppImage`), make it runnable and start it:

  ```
  chmod +x Encore-Basement-Studio-*.AppImage
  ./Encore-Basement-Studio-*.AppImage
  ```

  If it says FUSE is missing, install `libfuse2` (`libfuse2t64` on Ubuntu
  24.04 and later), or run it with `--appimage-extract-and-run`.

Audio works through ALSA out of the box. JACK is supported too, and so is
PipeWire (the standard on current Ubuntu, Fedora and Raspberry Pi OS) through
its JACK support. For low latency, add yourself to the `audio` group
(`sudo usermod -aG audio $USER`, then log out and back in).

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
    and harmony), Verb, Equalize and Master. Nothing to install.
  - **Your plugins**: ENCORE. Basement Studio finds your VST3 and VST2 plugins by itself (in the
    background, each checked in a separate helper process so a broken plugin
    can't crash it, and one that stops to show a window, like an activation
    prompt, is set aside) and lists them by maker. When a plugin is installed as
    both, only the VST3 is listed. Add more plugin folders in Preferences.
  - A few simple built-in effects (EQ, compressor, reverb, delay and more).
- **Instruments and MIDI**: Project > Add Instrument Track with any of your
  instrument plugins (drums, keys, samplers) or the built-in synth, played
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
  master, stems if you want them, cover art and the song's details. Signing
  in and uploading straight from ENCORE. Basement Studio come once the ENCORE. API is ready
  (docs/ENCORE-API.md).
- **Safe**: songs save as one folder each, ENCORE. Basement Studio keeps an automatic copy
  every two minutes, and offers it back if it ever closes unexpectedly.
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

## Coming next

- Sign in with ENCORE. and upload a song to your catalog as a draft
- A drum editor, and more MIDI tools

## Build

Needs CMake and a C++20 compiler: Visual Studio 2022 (or its Build Tools) with
the C++ workload on Windows, Xcode on a Mac.

```
git clone https://github.com/jimmyrlondo/encore-studio.git
cd encore-studio
git submodule update --init --depth 1 external/JUCE external/tracktion_engine
cmake -B build                       # Windows: add -A x64; Mac: add -G Xcode
cmake --build build --config Release
```

(Don't use `--recurse-submodules`: Tracktion Engine has its own copy of JUCE as
a submodule with an SSH address; ENCORE. Basement Studio uses `external/JUCE`, pinned to the
same commit, instead.)

On Linux (x64 or arm64, including 64-bit Raspberry Pi OS), install the
libraries first, then build Release (the 32-bit plugin bridge is Windows only):

```
sudo apt install build-essential cmake ninja-build pkg-config \
  libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libxi-dev
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The app ends up in `build/EncoreStudio_artefacts/Release/`. Checks you can run:

- `--selftest <folder>`: makes songs, imports, mixes, exports, reopens, and
  runs each built-in ENCORE. plugin, without a window (exit code 0 = all good,
  details in `<folder>/selftest.log`). GitHub runs it on every build.
- `--plugintest <report file> [folder]`: loads every VST3 and VST2 in a folder
  (default: the standard plugin folders) the way
  the Effects menu does and reports what happened with each one.
- `--profile <name>`: keeps settings separately, for testing without touching
  your own.

The code for the ENCORE. plugins is in `encore/` (copied from the plugin repo with
`tools/sync-encore-plugins.ps1`).

## License

ENCORE. Basement Studio is free software under the GNU Affero General Public License v3 (see
`LICENSE`). It uses JUCE under the AGPLv3 and Tracktion Engine under the GPLv3. VST2 plugins are hosted through
[FST](https://git.iem.at/zmoelnig/FST) (GPLv3 or later, in `external/fst`),
an independent reimplementation of the VST2 interface. MP3 export uses
[LAME](https://lame.sourceforge.io) 3.100 (LGPL, in `external/lame`).
The arrange view in `Source/edit/` started from Tracktion Engine's example
code (GPLv3) and is marked where it was changed. The ENCORE. plugins in
`encore/` are part of ENCORE. Basement Studio under the same license.

Copyright (C) 2026 Encore Studio. The ENCORE. name and logo (including
`Assets/encore-icon.*`) belong to Encore Studio; forks are welcome under the
license but should use their own name.
