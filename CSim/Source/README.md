# Source layout

Platform-independent application code lives here. OS-specific entry points and native helpers are under `Platform/`.

| Package | Role |
|---------|------|
| **App/** | Application entry after platform bootstrap (`CellMain`) |
| **Engine/** | Runtime host: modules API, Illumo loop/context, entity scaffolding |
| **Game/** | Cellular-automata game domain (canvas, game module, editor tools) |
| **Rulesets/** | CA rule implementations |
| **Rendering/** | Scene/drawables, token command path, OpenGL backend, MockBackend |
| **Services/** | Shared services: logging, env vars, input, CLI (token UI), allocators, save/load API |
| **Foundation/** | Low-level shared bits: macros, build info, sysinfo, math helpers |
| **Assets/** | Asset loaders (fonts, etc.) |
| **Platform/** | Windows / Linux / macOS ports |
| **Tests/** | Headless suite (`CSimTests`): mock backend, rules, canvas, UI tokens |

Historical / abandoned experiments: `archive/` at the repo root (not part of the build). See `docs/` for architecture notes.
