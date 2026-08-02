# CSim repository guidance

This file is the durable bootstrap for Codex work in this repository. Use it at
the start of every new task, then inspect the live files relevant to the request.
Do not rely on chat history as the source of truth.

## First five minutes

1. Run `git status --short` and preserve all existing user changes.
2. Read `README.md` for the current commands and
   `docs/architecture-consensus.md` for the architecture and decision history.
3. Inspect the live implementation before repeating a documentation claim.
   Code wins when code and docs disagree; update the docs in the same change
   when behavior intentionally changes.
4. Search the live tree narrowly. Exclude `build/`, `CSim/thirdparty/`,
   `archive/`, `CSim/.agents/`, `CSim/Source/.agents/`, and
   `CSim/Source/all.txt` unless the task specifically concerns generated,
   vendored, historical, or prior-agent material.
5. Keep the user's requested boundary. A review request does not authorize
   fixes, cleanup, generated-file removal, or worktree normalization.

## What this project is

CSim is a C++23 cellular-automata learning sandbox with a small modular-monolith
shell. It is an engine-shaped application, not a general-purpose game engine.
Windows, OpenGL, GLFW, and the dense-grid simulation are the current production
path. Linux and macOS contain older port scaffolding and should not be assumed
to compile or match Windows without fresh verification.

Near-term priorities are correctness, clarity, and useful CA behavior. Do not
introduce an ECS, render graph, generalized scene graph, multiple graphics
backends, infinite chunks, or SYCL merely for architectural completeness.

## Current architecture

Runtime flow:

```text
Platform entry
  -> App/CellMain                 product composition and main loop
  -> Engine/Illumo                owns services and module lifetime
  -> IModule implementations      CellGameModule; DebugModule in Debug builds
  -> Game + Rulesets              CA state, editor, simulation
  -> Rendering                    Scene list, Renderer, command tokens
  -> IBackend                     GLBackend in production, MockBackend in tests
```

Ownership and lifecycle:

- `CellMain` decides which modules ship. `Illumo` must not construct Game
  modules.
- `Illumo` owns long-lived services with `unique_ptr` and passes modules a
  non-owning `IllumoContext` pointer bag.
- Modules implement `Start`, `Update`, `DispatchDrawables`, and `Exit`.
- `Scene` is a non-owning, ordered drawable list rebuilt every frame. It is not
  a scene graph or ECS.

Rendering path:

```text
Drawable::AppendCommands(Renderer*)
  -> RenderCommand tagged-union tokens
  -> CommandQueue
  -> IBackend::SubmitCommandQueue
  -> GLBackend/GLDevice or MockBackend
```

Production drawables use the token path. Immediate `Draw()` is only the fallback
for tests or incomplete stubs. Game and rules code must not issue raw OpenGL
draw calls. Resource enrollment is rare; per-frame work emits bind, update,
uniform, state, and draw tokens. Token payload pointers must remain valid until
submission returns.

Canvas truth (verify here before trusting older notes):

- Domain: dense `lifeCanvas`, one byte per cell.
- Presentation: a CPU palette produces `targetRgb`; `displayRgb` fades toward
  it; `texCanvasBuffer` stores RGB bytes.
- GPU: one RGB display texture updated through dirty rectangles; OpenGL uses a
  PBO update path.
- `Canvas` intentionally owns domain state, visual state, dirty tracking, and
  GPU enrollment for now. Split it only when a concrete testability or scale
  need justifies the change.
- Historical R8/palette references in the decision log describe a superseded
  experiment. Current-state documentation must match the RGB-fade shader and
  `Canvas.cpp` path.

Ruleset truth:

- Active: Game of Life, Seeds, Brian's Brain, Highlife, Day & Night, Life
  Without Death, and Wireworld.
- Rule 90 and Rule 184 are stubs.
- Binary rules encode `0` as alive and `1` as dead.
- Wireworld encodes head `0`, empty `1`, tail `2`, conductor `3`.
- `RuleSet::calcGeneration` is double-buffered and toroidal. It still scans the
  full dense grid; dirty rectangles reduce visual upload work, not simulation
  complexity.

## Source map

| Concern | Primary files |
|---|---|
| Composition and main loop | `CSim/Source/App/CellMain.cpp` |
| Host, services, modules | `CSim/Source/Engine/Illumo.*`, `IModule.h`, `IllumoContext.h` |
| CA modes and editor | `CSim/Source/Game/CellGameModule.*`, `CellContext.h` |
| Grid, fade, dirty upload | `CSim/Source/Game/Canvas.*`, `CSim/Shader/canvas_*` |
| CA behavior | `CSim/Source/Rulesets/*` |
| Renderer and tokens | `CSim/Source/Rendering/Renderer.h`, `RenderCommand.h`, `CommandQueue.h` |
| Real graphics execution | `CSim/Source/Rendering/OpenGL/*` |
| Headless backend | `CSim/Source/Rendering/Mock/MockBackend.h` |
| Input, console, env, logging | `CSim/Source/Services/*` |
| OS entry and native save/load | `CSim/Source/Platform/*` |
| Tests | `CSim/Source/Tests/*` |
| Canonical architecture | `docs/architecture-consensus.md` |
| Formal decisions | `docs/sections/09-design-decision-log.tex` |

## Build and verification

From the repository root on Windows:

```powershell
cmake -S CSim -B build
cmake --build build --config Release
```

The default build compiles `CSimTests.exe` and runs the `CSimTests` CTest entry
through the `CSimRunTests` `ALL` target. A failing suite must fail the build.

Focused test commands:

```powershell
cmake --build build --config Release --target CSimTests
ctest --test-dir build -C Release -R "^CSimTests$" --output-on-failure
```

`CSimRenderTests` is only a backward-compatible CMake alias. An old
`build/Debug/CSimRenderTests.exe` may remain from an earlier configuration; do
not use it as the current suite. The current executable is
`build/Release/CSimTests.exe` (or the matching active configuration).

Headless tests cover MockBackend, Renderer token flow, rulesets, CellContext,
Canvas domain/fade/dirty behavior, and CommandLine/GLString tokens. They do not
prove that the live OpenGL window, native dialogs, or non-Windows ports work.
Run a proportional manual smoke test when changing those paths.

## Change rules

- Follow `CSim/CONTRIBUTING.md`: avoid `auto`, avoid namespaces, do not add
  recursion, and do not add third-party dependencies without user approval.
- Match local formatting and existing C++23 conventions. Prefer explicit
  ownership and narrow dependencies.
- Keep Game and Rulesets independent of raw OpenGL. Preserve the token-first
  render boundary and MockBackend testability.
- Do not grow `IllumoContext` casually. If a genuinely different third module
  appears, prefer explicit constructor dependencies.
- When adding a ruleset, update the factory/known-mode logic, console help and
  validation, CMake source lists, palette behavior, and focused tests together.
- Architecture changes require an update to
  `docs/architecture-consensus.md`; closed decisions also require a new entry in
  `docs/sections/09-design-decision-log.tex`.
- Edit LaTeX/Markdown sources, not generated `.aux`, `.fls`, `.fdb_latexmk`,
  `.log`, `.toc`, PDF, source dumps, ZIPs, or build outputs unless the user
  explicitly requests regenerated artifacts.
- Update this `AGENTS.md` whenever build commands, canonical paths, or durable
  architecture facts change.

## Known hazards; do not fix opportunistically

Confirm each against the current tree before acting:

- A module whose `Start` returns early can still receive `Update` and
  `DispatchDrawables`; the game path can then dereference a null `cellContext`.
- `IllumoContextHasGameCore` omits `commandLine`, although Edit uses it.
- `LoadCellGame` writes `lifeCanvas` directly without marking visual targets
  dirty, so the display can remain stale.
- Wireworld lacks a convenient head-placement brush, and startup still seeds a
  Game-of-Life glider even in Wireworld mode.
- The console duplicates the known-ruleset list from `CellContext`.
- `CommandQueue` has a fixed capacity of 2048 commands and its overflow policy
  needs explicit handling.
- Keep sources shared by `CSim` and `CSimTests` in `CSIM_SHARED_SOURCES`, and
  put common target settings in `csim_configure_target`; preserve target-only
  behavior such as the application's Debug Tracy instrumentation.
- Keep package READMEs synchronized with the canonical architecture; do not
  let current-state summaries drift back to the archived R8-only or entity
  scaffolding descriptions.

Treat `docs/current_issues.txt` as a review snapshot, not a guarantee that every
item remains open. Reproduce or inspect before fixing.

## Generated and historical material

- `build/`, root `CMakeFiles/`, and `CMakeCache.txt`: generated configuration.
- `CSim/thirdparty/`: vendored dependencies; avoid broad searches and edits.
- `archive/`: intentionally non-live experiments and analysis output.
- `CSim/.agents/` and `CSim/Source/.agents/`: prior-agent working records.
- `CSim/Source/all.txt`, `CSim/csim_source.zip`, logs, and LaTeX auxiliaries:
  snapshots or generated artifacts, never the implementation source of truth.
