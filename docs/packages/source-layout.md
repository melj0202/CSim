# Source layout

Platform-independent application code lives here. OS-specific entry points and native helpers are under `Platform/`.

| Package | Role |
|---------|------|
| **App/** | Application entry after platform bootstrap (`CellMain`) |
| **Engine/** | Runtime host: module API, Illumo services/lifecycle, frozen context, Debug-only overlay |
| **Game/** | Cellular-automata game domain (canvas, game module, editor tools) |
| **Rulesets/** | CA rule implementations |
| **Rendering/** | Scene layers, drawables, built-in render styles, primitives/`GameVisual`, token command path, OpenGL backend, MockBackend |
| **Services/** | Shared services: logging, env vars, input, CLI (token UI), allocators, save/load API |
| **Foundation/** | Low-level shared bits: macros, build info, math helpers |
| **Assets/** | Runtime asset data |
| **Platform/** | Windows / Linux / macOS ports |
| **Tests/** | Headless suite (`IllumoTests`): mock backend, rules, canvas, UI tokens |

Historical / abandoned experiments live under `archive/` at the repo root and
are not part of the build. This file and all other first-party package maps live
under `docs/packages/`; source directories intentionally contain code only.
