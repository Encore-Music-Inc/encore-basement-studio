# Notes for Claude (and whoever is driving it)

This is ENCORE. Basement Studio: the desktop DAW (C++, JUCE and Tracktion
Engine) with the ENCORE. plugins built in, its installers and packages, and its
self-tests. Notes from ENCORE.'s other and older repositories do not apply here;
this file does.

## Decisions (already made, don't revisit)

- Licence: GNU AGPLv3 (`LICENSE`). Copyright holder: Encore Music, Inc.
  JUCE is used under the AGPLv3 and Tracktion Engine under the GPLv3, so no
  commercial JUCE or Tracktion licence is needed. GPLv3 code such as FST (VST2
  hosting) and the ASIO SDK (used under its GPLv3 option) stays.
- The repo is public. Outside contributions are not accepted
  (`CONTRIBUTING.md`). The ENCORE. name and logo are not licensed (`TRADEMARKS.md`).
- Contact: support@encoremusic.studio
- Always write ENCORE. (capitals and the period) in anything people read, and
  "ENCORE. OS" with a space.
- No long dashes in any text. Use commas, periods or parentheses.
- Don't write "DSP" in names, titles or prose for people (in music business it
  means Spotify or Apple Music). Say "audio processing". Code folders and
  identifiers keep the name.
- Anything third-party goes in `THIRD-PARTY-NOTICES.md` (and, for what the
  Linux packages carry, the copyright file in `installer/linux/package.sh`).
- The product: Windows x64 (installer and zip), macOS (universal dmg), Linux
  x64 and arm64 (.deb and AppImage; arm64 is Raspberry Pi and ENCORE. OS).
- Only the DAW lives here. ENCORE.'s browser tools (the old `web/` folder) and
  their files, tests or scripts must never be added.
- Releases started fresh at 0.5.0. 0.4.3 and earlier stay where they were published.
- Names: people read "ENCORE. Basement Studio" (window titles, menus, About box,
  installers, Settings > Apps, packages). File and folder names leave out the
  period: `ENCORE Basement Studio.exe` / `.app`, `ENCORE Basement Studio Bridge32.exe`,
  `ENCORE Basement Studio.log`, `Program Files\ENCORE\Basement Studio`, settings in
  `ENCORE/Basement Studio` in the app data folder, songs in `Music/ENCORE Basement Studio`.
  The Linux package is `encore-basement-studio`. The CMake project and target are
  `EncoreBasementStudio`.
- Problems go to support@encoremusic.studio (Help > Report a Problem opens an
  email), not to GitHub issues.
- Docs (README, CLAUDE.md) describe this repository and what it directly uses
  or is used by, never a map of ENCORE.'s other repositories.
- Docs say what the repository does and point to the file that sets a value
  (`ci.yml`, `dependabot.yml`, a CMake file, the Pinned releases table). They
  never repeat a value from config or code, and never give an estimate or a
  measurement (how long, how fast, how often). Release notes are the
  exception: each describes its release on the day it was published.

## Layout

| Path | What |
|---|---|
| `Source/` | the app; `edit/` arrange view, mixer, piano roll, tuner; `encore/` creates the built-in plugins; `bridge/` the 32-bit plugin bridge (Windows) |
| `installer/` | `EncoreBasementStudio.iss` (Windows, Inno Setup), `linux/package.sh` (.deb and AppImage) |
| `patches/` | ENCORE.'s change to Tracktion Engine, applied by CMake at configure |
| `tools/` | `window-test.ps1` |
| `external/tracktion_engine` | Tracktion Engine, pinned to a commit: the one listed exception to releases-only (the README's Pinned releases table says why) |
| `external/encore-vst` | the plugins' sources and `shared/`, pinned to a release (the README's Pinned releases table); its `external/encore-audio-processing` and that repo's `external/JUCE` are the audio processing and JUCE Basement Studio builds with |
| `CHANGELOG.md` | every release's notes, newest first; the release job copies the version's section onto the GitHub release |
| `.github/release-notes.sh` | prints a version's notes from `CHANGELOG.md`, and fails if they're missing (CI runs it on every build) |
| `.github/encore-vst-release.sh` | downloads a file of the ENCORE. VST release the pin points to and checks its SHA-256 (the Windows installer and the Mac disk image bundle it; Linux CI loads its plugins) |
| `.github/pins/check.sh` | CI's check that every pin is a release (the README's Pinned releases table) |
| `.github/no-audio-processing.sh` | CI's check that Basement Studio keeps no audio processing of its own (its exceptions are listed in it, each with its reason) |
| `external/fst`, `external/lame` | vendored: VST2 headers (GPLv3), the MP3 encoder (LGPL) |
| `.github/sanitizers/` | the scheduled sanitizer run's exceptions (`ubsan.supp`, each with its reason; keep it short) and `report.sh` |
| `.github/workflows/ci.yml` | pull requests: Linux x64, Windows x64 and macOS (Apple Silicon), the app and the self-test, no packaging (skipped when only docs change, or only the version line and `CHANGELOG.md`); on a schedule: the self-test with sanitizers; main and manual runs: Windows, macOS, Linux x64 and arm64 with their files as artifacts, plus the Linux package tests; `v*` tags: attach main's files to the Release, no rebuild; "CI passed" is the one check to require |

## Submodules

- Two pins: encore-vst and Tracktion Engine. encore-vst decides the plugins,
  the audio processing (the encore-audio-processing it pins) and JUCE (the one
  that pins); this repo has no JUCE or audio processing of its own. The Windows
  installer bundles the ENCORE. VST release the pin points to.
- Never initialise recursively. Fetch by path, one level at a time:
  `git submodule update --init --depth 1 external/encore-vst external/tracktion_engine`,
  `git -C external/encore-vst submodule update --init --depth 1 external/encore-audio-processing`,
  `git -C external/encore-vst/external/encore-audio-processing submodule update --init --depth 1 external/JUCE`.
- The plugins come from encore-vst (`plugins/<Name>/Source/*.cpp`, `shared/`)
  and the audio processing from its encore-audio-processing (`dsp/`), both on
  the include path. Don't copy either back into this repo: change them in their
  own repo, release it, then move the pin here in a pull request.
- A JUCE change starts in encore-audio-processing and arrives here through an
  encore-vst release. Tracktion Engine must work with it: the self-test on
  Linux, Windows and macOS proves that in the pull request that moves the pin.
- `tools/sync-encore-plugins.ps1` and the copied `encore/` folder are gone on
  purpose.
- All audio processing lives in encore-audio-processing; Basement Studio keeps
  none of its own, not even a small helper: the tuner and its tunings list,
  mastering, export loudness and dither, the metronome's clicks, the drum kits,
  levels and note and key names all come from its headers. JUCE's processing
  classes and Tracktion's export loudness and dither don't stand in for them.
  When something is missing there, add it there first.
  `.github/no-audio-processing.sh` (in CI) fails on processing-shaped code in
  `Source/`. Tracktion's playback engine (mixing, plugin hosting, its meters)
  is the DAW itself.
- `ReferenceTests` (a target in `CMakeLists.txt`) builds encore-audio-processing's
  `tests/ReferenceTests.cpp` the way this repository compiles the headers; CI
  runs it on every system and it checks every result in that repo's
  `data/reference.json`.

## Pins

- What this repository uses is pinned in one place, and only to a published
  release. The Pinned releases table in the README names each pin, and CI
  (`.github/pins/check.sh`) checks that each one is a release and is exactly
  what the folder is pinned to. An exception (no usable release exists) is
  written in the table with its reason.
- Fetch pinned folders by their paths, one level at a time
  (`git -C <folder> submodule update --init --depth 1 <path>`), never with
  `--recursive`.
- Move a pin to a new release only when something changes for this
  repository's users, and pick this repository's version number by what its
  users notice, not by the number of what it uses.
- Pull request titles: "Use <repository> vX.Y.Z (was vA.B.C)" for a pull
  request that moves a pin; "Release vX.Y.Z" only for this repository's own
  version.

## Building and testing

- Actions in `ci.yml` are pinned to commits (version in a comment); Dependabot
  proposes updates. Keep new ones pinned the same way.
- Keep builds fast: Ninja, ccache/sccache in CI, no full LTO (`ENCORE_LTO` is
  OFF; THIN is ThinLTO/incremental LTCG).
- Linux: install the packages listed in the README, then
  `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`, `cmake --build build`, and
  `xvfb-run -a "build/EncoreBasementStudio_artefacts/Release/ENCORE Basement Studio" --selftest <folder>`
  (exit 0, `ALL PASSED` at the end of `selftest.log`). It must pass before a
  pull request.
- Plugin formats: VST3 and VST2 everywhere, Audio Units on macOS, LV2 and LADSPA
  on Linux, 32-bit through the bridge on Windows. `PluginFinder::isSearched` is the
  one list of what the plugin search looks for; a format compiled in but not in it
  is never found, so change both together. The self-test checks each system's set.
- macOS: CI signs the whole app ad hoc (both halves of the universal binary) and
  checks it declares `.encore` songs (`DOCUMENT_EXTENSIONS` in `CMakeLists.txt`).
- The built-in plugins are found by their identifiers (`encore:vox`, `encore:verb`,
  `encore:equalize`, `encore:master`, `encore:strum`), which songs save. encore-vst's
  `EncoreProcessorBase` makes the same identifier from the plugin's name
  ("ENCORE. Vox"), so keep the names and `Source/EncorePlugins.h` in step.

## Versioning and releases

- Semantic versioning, tags `vMAJOR.MINOR.PATCH`. MAJOR for anything that breaks
  people's saved work (songs that no longer open, or open differently); MINOR for
  new features; PATCH for fixes.

## Release process

1. The version lives in exactly one place: `project(EncoreBasementStudio VERSION x.y.z)`
   in `CMakeLists.txt`. The app, the Windows installer (`build/version.iss`), the
   Linux packages and the Mac disk image read it from there.
2. Changing it is its own small pull request ("Release vX.Y.Z"), so a person sees the
   number before it's built. It adds the release notes to `CHANGELOG.md` (see
   Release notes below); CI fails until they're there.
3. Merging builds everything on `main`: the self-tests run, and the Windows installer
   and zip, the Mac dmg and the Linux x64 and arm64 .deb and AppImage are uploaded as
   workflow artifacts.
4. After the `main` build of that commit has passed, a person publishes the release in
   GitHub. Never a Claude session. Releases, Draft a new release; tag `vX.Y.Z` (Create
   new tag) with target `main` at the "Release vX.Y.Z" commit; leave the title and the
   description empty, don't click Generate release notes and don't attach files (the
   release job fills all of them in); Publish. Never publish an old draft release:
   publishing creates its tag on whatever its target is at that moment.
5. The tag's CI run doesn't rebuild. It checks that the tag matches the version, finds
   the successful `main` build of that exact commit and attaches its files, each with a
   `.sha256` file, to the Release, and sets its title (`ENCORE. Basement Studio vX.Y.Z`)
   and notes (the version's section of `CHANGELOG.md`). If either check fails, nothing
   is attached. If that commit was docs-only (its build was skipped), run CI by hand on
   `main` (Actions, CI, Run workflow) and then re-run the release job.

- Claude sessions can't push tags: the push is refused with HTTP 403. Don't try, and
  don't look for another way (the API, a workflow, a different remote). When a
  release is due, open the version pull request, then tell your human the version
  number, why that number, and what changed since the last release, and wait for
  them to publish it.
- Tags `v*` are protected: they can't be moved or deleted (an org admin can, in an
  emergency).

## Release notes

Every release's notes use this format, so they read the same in every ENCORE.
repository:

```
<One or two sentences: what this release is.>

### What's new        (a repository's first release: "What's in it")
### Downloads         (the files attached, and what each is for)
### Built on          (exactly what the Pinned releases table pinned for this release)

Questions or problems: support@encoremusic.studio
```

- The title is `ENCORE. Basement Studio vX.Y.Z`; the tag stays `vX.Y.Z`.
- The notes describe the release as it is on the day it's published: no links
  to or mentions of later releases, and no promises ("coming soon", "will
  support"). If a limitation matters to people, state the limitation, not a plan.
- They live in `CHANGELOG.md`, newest first, one `## vX.Y.Z` section each,
  written once, in plain language, in the "Release vX.Y.Z" pull request. CI
  (`.github/release-notes.sh`) fails while the version has no section, or the
  section has no Downloads, Built on or Questions line. The release job copies
  the section onto the GitHub release, so nobody pastes notes.
- A published section isn't edited except to fix a factual error. Then fix it
  in a pull request and make the same fix on the GitHub release (Edit), so the
  two stay the same.
- "Built on" names each top-level pin's release, and what it brings in
  brackets when that matters to people (for example the JUCE version).

## Changes

- main only changes through pull requests. Never push to main directly.
