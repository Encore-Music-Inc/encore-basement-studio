# Third-party software

ENCORE. Basement Studio and ENCORE. Green Room are proprietary (see `LICENSE`).
They use the following third-party software under these terms.

| Software | Where | License used by ENCORE. |
|---|---|---|
| [JUCE](https://juce.com) 8 | `external/JUCE` | JUCE 8 Starter licence (free while revenue is under the Starter limit; move to Indie or Pro before passing it) |
| [Tracktion Engine](https://github.com/Tracktion/tracktion_engine) | `external/tracktion_engine`, and the arrange view in `Source/edit/` started from its example code | Tracktion Engine Personal licence (free under $50k revenue, one developer seat, "Powered by Tracktion Engine" shown in the app) |
| Steinberg VST3 SDK (bundled with JUCE) | inside `external/JUCE` | MIT from VST 3.8 on (older 3.7 copies are GPLv3, so JUCE must bundle 3.8 or newer) |
| [LAME](https://lame.sourceforge.io) 3.100 | `external/lame` | LGPL 2: must be a separate, replaceable library (DLL / .so / .dylib), with its source offered |
| [FST](https://git.iem.at/zmoelnig/FST) | `external/fst` | GPLv3: **can't ship in a proprietary build.** VST2 hosting and the 32-bit bridge that use it are being removed |
| lamejs 1.2.1 | `web/master/vendor/lame.min.js` | LGPL 3: shipped as its own unmodified file |
| libogg 1.3.6, libvorbis 1.3.7 | compiled into `web/master/vorbis.wasm` | BSD |
| Drum kits, sounds, word lists | `web/master/kits`, `sounds`, `words` | each folder's own license file |

Licence terms and limits change: check juce.com and engine.tracktion.com before
each release, and before revenue passes a tier's limit.
