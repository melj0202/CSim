# Source-package maps

These files preserve the useful ownership notes that formerly lived as README
files beside source code. They are documentation, not build inputs.

| File | Area |
|---|---|
| `source-layout.md` | Entire `Illumo/Source` tree |
| `app.md` | Product composition and main loop |
| `engine.md` | Runtime host, context, and modules |
| `game.md` | Canvas, simulation module, editing, and domain commands |
| `services.md` | Logging, configuration, input, console, and save/load API |
| `foundation.md` | Dependency-light utilities |
| `assets.md` | Runtime files under `Illumo/Assets` (outside `Source`) |
| `platform.md` | Platform contract and port map |
| `platform-linux.md` | Linux scaffold status |
| `platform-macos.md` | macOS scaffold status |
| `tests.md` | Canonical headless suite |

The canonical architecture remains `../architecture-consensus.md`. Operational
rules live in the root and nested `AGENTS.md` hierarchy, not in these maps.
