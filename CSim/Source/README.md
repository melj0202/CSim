# Source layout

Platform-independent application code lives here. OS-specific entry points and native helpers are under `Platform/`.

| Package | Role |
|---------|------|
| **App/** | Application entry after platform bootstrap (`CellMain`) |
| **Engine/** | Runtime host: modules API, Illumo loop/context, entity scaffolding |
| **Game/** | Cellular-automata game domain (canvas, game module, editor tools) |
| **Rulesets/** | CA rule implementations |
| **Rendering/** | Render types, scene/drawables, backend interfaces, OpenGL |
| **Services/** | Shared services: logging, env vars, input, CLI, allocators, save/load API |
| **Foundation/** | Low-level shared bits: macros, build info, sysinfo, math helpers |
| **Assets/** | Asset loaders (fonts, etc.) |
| **Platform/** | Windows / Linux / macOS ports |
| **Tests/** | Unit tests (not yet fully wired into CMake) |

Historical / abandoned experiments: `archive/` at the repo root (not part of the build).
