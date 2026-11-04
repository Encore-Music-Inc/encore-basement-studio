# Notes for Claude (and whoever is driving it)

Written 2026-11-04 by Jimmy's Claude for the ENCORE. team as the project moves over to
Denny. Read this before changing anything. Jimmy decides anything this file doesn't cover.

## Before any code: the handoff checklist

Confirm these with your human at the start of the first session, and remind them until each
one is done:

1. **Contributor agreement signed.** Every person who writes code here signs a copyright
   assignment (or contributor agreement) to Encore Studio *before* their first commit. Without
   it they own their part and the company can't keep the code closed. Jimmy provides it.
2. **Tracktion Engine seat (free).** Basement Studio uses Tracktion Engine under its free
   Personal licence (individual use, under $50k revenue, "Powered by Tracktion Engine" shown
   in the app). Sign up at https://engine.tracktion.com/ (Pricing, then **Get License**
   under Personal). Anyone changing Basement's C++ (`Source/`, `CMakeLists.txt`, `encore/`)
   needs their own seat; ask Tracktion through that site whether a second developer can
   take their own Personal licence or needs Indie ($35/seat/month, 12-month term). Green Room
   (`web/`) and the Next.js work don't touch Tracktion.
3. **JUCE (free).** The repo is on **JUCE 9**. JUCE 9's free **Starter** tier covers all
   features up to $20,000 revenue (then Indie, then Pro). Each developer building the C++
   makes an account at https://account.juce.com/ and gets Starter from
   https://juce.com/get-juce/ (download at https://juce.com/download/).
4. **ASIO licence (free, signed once).** Windows builds use Steinberg's ASIO SDK (bundled in
   JUCE at `external/JUCE/modules/juce_audio_devices/native/asio/`, switched on by
   `JUCE_ASIO=1` in `CMakeLists.txt`). It is GPLv3 *or* Steinberg's own ASIO licence, and a
   closed build needs the Steinberg licence. It costs nothing, but a signed agreement has to be
   countersigned by Steinberg **before** a Windows build with ASIO is published. Start at
   https://www.steinberg.net/developers/ (the proprietary SDK page; Steinberg staff on the
   forum have linked it as https://www.steinberg.net/developers/prorietary-sdk/, with that
   spelling). Developers on forums.steinberg.net report replies taking weeks, so sign early,
   keep a copy of the countersigned agreement, and add an ASIO row to
   `THIRD-PARTY-NOTICES.md` once it's signed. Don't turn ASIO off to dodge this: without it
   Windows loses low-latency recording.
5. **Access.** The repos are private: jimmyrlondo/encore-studio (this one),
   jimmyrlondo/sick-and-sad (the VST plugins), jimmyrlondo/encore-studio-prototype (Next.js).
   jimmyrlondo/encore-vst is public: downloads only, no code.

## The rules every change keeps

- **Everything here is proprietary and closed source.** Never add an open-source licence,
  "open source" wording, or AGPL/GPL notices to code or copy. `LICENSE`,
  `THIRD-PARTY-NOTICES.md` and `web/docs/LICENSING-NOTE-FOR-ENCORE.md` have the details.
- **No copyleft dependencies.** No GPL or AGPL code at all, and LGPL only as a separate,
  replaceable file or library (that's how LAME and lamejs are used). MIT, BSD, Apache, ISC,
  zlib and OFL are fine. Check before adding any package, including npm packages in the
  Next.js app.
- **Show "Powered by Tracktion Engine"** in Basement Studio (About box and start screen): a
  condition of the free licence.
- **Revenue caps.** The day ENCORE. earns money, tell Jimmy to check the JUCE and Tracktion
  tiers. Both count the whole company's revenue.
- **No secrets in the repo.** Keys and credentials live in the site's own configuration.
- **Brand:** always write **ENCORE.** (capitals and the period) in anything people see. No
  em dashes in user-facing copy (Jimmy reads them as AI-written); use commas, periods or
  parentheses.

## Where things are

| Folder | What |
|---|---|
| `Source/`, `CMakeLists.txt`, `external/` | ENCORE. Basement Studio, the desktop DAW (C++, JUCE + Tracktion Engine) |
| `encore/` | The ENCORE. plugins' code built into Basement (copied from sick-and-sad with `tools/sync-encore-plugins.ps1`). `encore/shared/dsp/` is also the source of Green Room's `master.wasm` |
| `web/master/` | ENCORE. Green Room, the browser tools (plain HTML/JS, no build step). Start with `README.md` and `INTEGRATION.md` there |
| `web/docs/` | `GREEN-ROOM-TOOLS-INVENTORY.md` (every tool, what it uses, what needs a backend), `NEXTJS-CONVERSION-GUIDE.md`, `UX-REVIEW-2026-11.md`, `LICENSING-NOTE-FOR-ENCORE.md` |
| `installer/` | Windows, Mac, Linux and Raspberry Pi packaging |
| `os/` (branch `encore-os`) | ENCORE. OS, the Raspberry Pi distribution (not merged yet) |

## Working on Green Room

- Run it: serve `web/master/` over http (`python -m http.server 8123`) and open
  `http://localhost:8123/`.
- Tests: the 28 `test-*.mjs` suites in `web/master/` (`node test-node.mjs` and so on; the list
  is in `web/tools/deploy-londo.sh`). Run them all before you push.
- `master.wasm` and `vorbis.wasm` are committed. Rebuild `master.wasm` with
  `web/master/build.ps1` (needs Emscripten; the path in the script is Jimmy's machine). It
  builds from `encore/shared/dsp/`, including `lite/JuceLite.h`, the browser-only JUCE
  stand-in. Don't delete that folder.
- Deploy (Jimmy's machine today): `bash web/tools/deploy-londo.sh` publishes the committed
  files to londo.tech/encore-studio (a noindex preview). `ENCORE_DEST=encore-studio-preview`
  publishes a try-out copy instead.
- The tools must fit one screen (tabs and side panels, no long scrolling pages).
  `metronome.html` is the reference layout.

## To do: "Report a Problem" has nowhere to go

Help > Report a Problem (and the "Report a problem" link on the start screen) still opens a
new GitHub issue on jimmyrlondo/encore-studio. That repo is private now, so users get a 404.
Point it at ENCORE.'s existing support form instead (Jimmy says ENCORE. already has forms;
ask him for the right one's URL). The places to change:

- `Source/MainComponent.h`, `showReport()` (about line 4095): builds the
  `github.com/<repo>/issues/new` URL with a title and body, and its status message says
  "Drag it into the GitHub issue".
- `Source/Hub.h` (about line 1041): the start screen's `issueLink`.
- `Source/Support.h`, `ReportComponent`: the intro text says it "opens a new GitHub issue".
- `support::repo()`, if nothing else uses it afterwards.

The report is a zip on the user's Desktop (log, audio setup, optional song arrangement, no
recordings). If the form can't take an attachment, have the app say where to email the zip,
or add an upload to the form. Keep the "what happened" text pre-filled if the form accepts
URL parameters.

## CI

GitHub Actions minutes are limited on private repos (Mac minutes count 10x). `build.yml` runs
only for a `v*` tag or by hand (`gh workflow run build.yml --ref <branch>`). Don't add push or
pull-request triggers without asking Jimmy.

## Releases

The old releases (Basement Studio 0.1.0 to 0.4.3, plugins 0.4.0) were published under the
AGPLv3 and are now hidden as drafts. Don't publish them again, and don't bundle them into new
builds. Downloads are paused until the first proprietary build, and the Basement Studio and
VST pages say so.
