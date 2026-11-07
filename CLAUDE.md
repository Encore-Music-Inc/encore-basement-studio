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
- The repo will be public. Outside contributions are not accepted
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

## Layout

| Path | What |
|---|---|
| `Source/` | the app; `edit/` arrange view, mixer, piano roll, tuner; `encore/` creates the built-in plugins; `bridge/` the 32-bit plugin bridge (Windows) |
| `installer/` | `EncoreBasementStudio.iss` (Windows, Inno Setup), `linux/package.sh` (.deb and AppImage) |
| `patches/` | ENCORE.'s change to Tracktion Engine, applied by CMake at configure |
| `tools/` | `tunings.py` (writes `Source/edit/Tunings.h`), `window-test.ps1` |
| `external/tracktion_engine` | Tracktion Engine, pinned to commit `a5e4499`: the one listed exception to releases-only (no Tracktion release supports JUCE 9 yet) |
| `external/encore-vst` | the plugins' sources and `shared/`, pinned to release `v0.6.0`; its `external/encore-audio-processing` (`v1.0.1`) and that repo's `external/JUCE` (`9.0.5`) are the audio processing and JUCE Basement Studio builds with |
| `.github/pins/check.sh` | CI's check that every pin is a release (the README's Pinned releases table) |
| `external/fst`, `external/lame` | vendored: VST2 headers (GPLv3), the MP3 encoder (LGPL) |
| `.github/sanitizers/` | the weekly sanitizer run's exceptions (`ubsan.supp`, each with its reason; keep it short) and `report.sh` |
| `.github/workflows/ci.yml` | pull requests: Linux x64, Windows x64 and macOS (Apple Silicon), the app and the self-test, no packaging; weekly: the self-test with sanitizers; main and manual runs: Windows, macOS, Linux x64 and arm64 with their files as artifacts, plus the Linux package tests; `v*` tags: attach main's files to the Release, no rebuild; "CI passed" is the one check to require |

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

## How ENCORE.'s repositories fit together

```
encore-audio-processing ──▶ JUCE                        (the JUCE version for all of ENCORE.)
encore-vst              ──▶ encore-audio-processing     (brings JUCE with it)
encore-basement-studio  ──▶ encore-vst                  (brings the sound engine and JUCE with it)
                        ──▶ Tracktion Engine
encore-os               ──▶ encore-vst and encore-basement-studio releases
Green Room (encoremusic)──▶ master.wasm from encore-audio-processing, by its SHA-256
```

- Each repository pins each thing it uses in one place, and only to a
  published release. The Pinned releases table in the README names them, and
  CI (`.github/pins/check.sh`) checks that each one is a release and is exactly
  what the folder is pinned to. An exception (no usable release exists) is
  written in the table with its reason.
- Nothing is pinned twice. A repository that uses encore-vst uses encore-vst's
  sound engine and JUCE, never its own copy; one that uses
  encore-audio-processing uses its JUCE.
- Fetch pinned folders by their paths, one level at a time
  (`git -C <folder> submodule update --init --depth 1 <path>`), never with
  `--recursive`: some pinned repositories have test-only folders of their own.
- Changes travel up one repository at a time, each through a release:
  encore-audio-processing, then encore-vst, then encore-basement-studio, then
  encore-os (Green Room beside them). A repository moves to a new release only
  when something changes for its own users, and picks its own version number
  by what its users notice, not by the number of what it uses.
- Pull request titles: "Use <repository> vX.Y.Z (was vA.B.C)" in the repository
  that moves a pin; "Release vX.Y.Z" only for the repository's own version.
- After this repository publishes a release: encore-os pins the new release for its next image.

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
   number before it's built.
3. Merging builds everything on `main`: the self-tests run, and the Windows installer
   and zip, the Mac dmg and the Linux x64 and arm64 .deb and AppImage are uploaded as
   workflow artifacts.
4. A person publishes the release in GitHub (Releases, Draft a new release, a new tag
   `vX.Y.Z` on that `main` commit, Publish). Never a Claude session.
5. The tag's CI run doesn't rebuild. It checks that the tag matches the version, finds
   the successful `main` build of that exact commit and attaches its files, each with a
   `.sha256` file, to the Release. If either check fails, nothing is attached.
   If that commit was docs-only (its build was skipped), run CI by hand on `main`
   (Actions, CI, Run workflow) and then re-run the release job.

- Claude sessions can't push tags: the push is refused with HTTP 403. Don't try, and
  don't look for another way (the API, a workflow, a different remote). When a
  release is due, open the version pull request, then tell your human the version
  number, why that number, and what changed since the last release, and wait for
  them to publish it.
- Tags `v*` should be protected (a tag ruleset, as on encore-vst): they can't be
  moved or deleted (an org admin can, in an emergency).

## Changes

- main only changes through pull requests. Never push to main directly.
