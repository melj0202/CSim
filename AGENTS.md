# Illumo repository guidance

This file is the durable bootstrap for Codex work in this repository. Use it at
the start of every new task, then inspect the live files relevant to the request.
Do not rely on chat history as the source of truth.

## First five minutes

1. Run `git status --short` and preserve all existing user changes.
2. Read `README.md` for the current commands and
   `docs/architecture-consensus.md` for the architecture and decision history.
   Before substantive work, also read `docs/output/illumo.pdf` when it exists
   and its contents are not already available in the task context. If it is
   absent or stale, rebuild it first with `docs/build.ps1` rather than relying
   on an older copy.
3. Inspect the live implementation before repeating a documentation claim.
   Code wins when code and docs disagree; update the docs in the same change
   when behavior intentionally changes.
4. Search the live tree narrowly. Exclude `build/`, `Illumo/thirdparty/`,
   `archive/`, `Illumo/.agents/`, `Illumo/Source/.agents/`, and
   `Illumo/Source/all.txt` unless the task specifically concerns generated,
   vendored, historical, or prior-agent material.
5. Keep the user's requested boundary. A review request does not authorize
   fixes, cleanup, generated-file removal, or worktree normalization.

## What this project is

Illumo is a C++23 cellular-automata learning sandbox with a small modular-monolith
shell. It is an engine-shaped application, not a general-purpose game engine.
Windows, OpenGL, GLFW, and the sparse infinite-canvas simulation are the current
production path. Linux and macOS contain older port scaffolding and should not
be assumed to compile or match Windows without fresh verification.

Near-term priorities are correctness, clarity, and useful CA behavior. Do not
introduce an ECS, render graph, generalized scene graph, multiple graphics
backends, or SYCL merely for architectural completeness.

## Current architecture

Runtime flow:

```text
Platform entry
  -> App/CellMain                 product composition and main loop
  -> Engine/Illumo                owns services and module lifetime
  -> IModule implementations      CellGameModule; DebugModule in Debug builds
  -> Game + Rulesets              CA state, editor, simulation
  -> Rendering                    Scene list, Renderer, command tokens
  -> IBackend                     CreateOpenGLBackend at Illumo::Init; MockBackend in tests
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
  -> vector CommandQueue (2,048 reserve; configurable 65,536 default ceiling)
  -> IBackend::SubmitCommandQueue
  -> GLBackend/GLDevice or MockBackend
```

Production drawables use the token path. Immediate `Draw()` is only the fallback
for tests or incomplete stubs. Game and rules code must not issue raw OpenGL
draw calls. Resource enrollment is rare; per-frame work emits bind, update,
uniform, state, and draw tokens. Token payload pointers must remain valid until
submission returns.

Backend resources use non-convertible slot+generation `MeshHandle`,
`ShaderHandle`, and `TextureHandle` values; renderer styles use the same model.
Backends allocate handles and validate replacement, destruction, queries, and
command submission. `AssetManager` caches file textures/shaders by canonical
path+options, owns one CPU file/decode worker, and performs GPU replacement only
from render-thread `pump`. Debug builds poll timestamps every 500 ms; explicit
reload remains available in all builds. The Debug renderer demo acquires both
its atlas and contract-compatible sprite shader through this managed path.

`GameVisual` is the reusable painter-correct 2D host. One stable ordered stream
spans shapes, sprites, and text; only adjacent compatible items batch. Parent and
local `Transform2D`, normalized pivots, atlas regions/flips, and bounded dynamic
quad buffers are supported. `SpriteAnimator` is passive and caller-updated.

Canvas truth (verify here before trusting older notes):

- Production domain: `SparseCellGrid` owns signed 64-bit world cells in a
  hash map of non-background 16x16 chunks with separate stored-cell occupancy
  and neighbor-counting masks. Sparse chunks accumulate non-background cells
  and neighbors of counted cells in retained
  contiguous per-chunk scratch records with fixed candidate masks and neighbor
  counts. A retained generation-stamped flat index maps chunk addresses directly
  to scratch records without sorting or binary searches. Next-generation output
  uses a retained inactive chunk map and recycled `unordered_map` node handles,
  avoiding steady-state chunk-node allocation. At 16,384 or more candidate
  cells, candidate evaluation uses the reusable worker pool with coarse ranges
  of roughly 2,048 candidate cells and up to four automatic workers; small
  candidate sets remain direct and serial. A retained changed-chunk frontier
  evaluates only changed chunks and their neighbors while that expands to at
  most 64 targets; settled worlds evaluate no chunks, and broad changes fall
  back to the complete adaptive paths. Complete mixed worlds select candidate
  or deterministic 18x18 halo evaluation independently per target from actual
  counted-neighbor contribution work; Wireworld conductors are stored but only
  heads contribute. All-dense counted chunks bypass scratch construction. At
  32 or more halo targets, a grid-owned reusable pool uses up to eight workers.
  All evaluators index a per-ruleset 256x9 transition table; dense halo targets
  count neighbors with a rolling three-row stencil. The grid is non-toroidal and its
  revision changes only when cell contents actually change.
- Production presentation: `CanvasView` samples the camera-visible region into
  an exact nearest-filtered RGB texture while one texel per cell fits. At far
  zoom it uses a revision-gated, density-colored overview capped at roughly
  four screen pixels per texel; this is a presentation cap, not a simulation
  chunk cap. It fades visible colors, snaps newly revealed cells, and owns one
  reusable RGB texture plus one world-space quad aligned to 16x16 cell bounds.
- Persistence always writes sparse version 2 and reads both that format and the
  prior dense format. Legacy `CellGrid`/`Canvas` remain compatibility fixtures,
  not a second production runtime path.

Ruleset truth:

- Active: Game of Life, Seeds, Brian's Brain, Highlife, Day & Night, Life
  Without Death, and Wireworld.
- Rule 90 and Rule 184 are stubs.
- Binary rules encode `0` as alive and `1` as dead.
- Wireworld encodes head `0`, empty `1`, tail `2`, conductor `3`.
- `RuleSet` transitions (`nextState`) build a cached 256x9 table and palette
  evaluation (`evalCell`) supplies colors. Production hot loops index the table
  instead of making virtual transition calls and use separate stored/counting masks,
  retained chunk-local candidate scratch, and a generation-stamped flat address
  index. Mixed targets independently select candidate or halo evaluation, with
  bounded worker-pool evaluation for large work sets. Both
  result paths share retained transactional chunk-map node storage; a local
  changed-region path patches the retained prior generation. Dense
  `calcGeneration` support remains only for compatibility tests.
  Headless benches: `Illumo.Sim.MicroBench`, `Illumo.Sim.SparseMicroBench`.

## Source map

| Concern | Primary files |
|---|---|
| Composition and main loop | `Illumo/Source/App/CellMain.cpp` |
| Host, services, modules | `Illumo/Source/Engine/Illumo.*`, `IModule.h`, `IllumoContext.h` |
| CA modes and editor | `Illumo/Source/Game/CellGameModule.*`, `CellContext.h` |
| Domain cell storage | `Illumo/Source/Game/SparseCellGrid.*` |
| Bounded view, fade, dirty upload | `Illumo/Source/Game/CanvasView.*`, `Illumo/Shader/canvas_*` |
| Compatibility dense storage | `Illumo/Source/Game/CellGrid.*`, `Canvas.*` |
| CA behavior | `Illumo/Source/Rulesets/*` (`nextState`/palette) |
| Renderer and tokens | `Illumo/Source/Rendering/Renderer.h` (IBackend* only), `RenderCommand.h`, `CommandQueue.h` |
| Resource handles and file assets | `Illumo/Source/Rendering/ResourceHandle*`, `AssetManager.*` |
| 2D primitives and animation | `Illumo/Source/Rendering/Primitives/*` |
| Debug renderer atlas and shader | `Illumo/Assets/RendererDemo/*` |
| Production backend factory | `Illumo/Source/Rendering/OpenGL/CreateOpenGLBackend.*` (composed in `Engine/Illumo.cpp`) |
| Real graphics execution | `Illumo/Source/Rendering/OpenGL/*` |
| Headless backend | `Illumo/Source/Rendering/Mock/MockBackend.h` |
| Input, console, env, logging | `Illumo/Source/Services/*` |
| OS entry and native save/load | `Illumo/Source/Platform/*` |
| Tests | `Illumo/Source/Tests/*` |
| Canonical architecture | `docs/architecture-consensus.md` |
| Formal decisions | `docs/latex/sections/09-design-decision-log.tex` |

## Build and verification

From the repository root on Windows:

```powershell
cmake -S Illumo -B build
cmake --build build --config Release
```

The default build compiles `IllumoTests.exe` and runs every granular `Illumo`
CTest entry through the `IllumoRunTests` `ALL` target. A failing case must fail
the build.
When Windows PowerShell and `latexmk` are found at configure time, the same
default build also runs `IllumoDocs` (`docs/build.ps1`) and writes
`docs/output/illumo.pdf`. Configure with `-DILLUMO_BUILD_DOCUMENTATION=OFF` for
a code-only machine.

Focused test commands:

```powershell
cmake --build build --config Release --target IllumoTests
ctest --test-dir build -C Release -L Illumo --output-on-failure
ctest --test-dir build -C Release -N -L Illumo
```

`IllumoRenderTests` is a convenience CMake alias. The canonical executable is
`build/Release/IllumoTests.exe` (or the matching active configuration). It
supports `--list` and `--run <exact-name>`; CTest invokes one case per process
with an isolated working directory.

Headless tests cover typed/generational MockBackend resources,
Renderer/token/style/asset flow, painter-correct primitives and animation, rulesets,
CellContext, CellGameModule commands and file-backed save/load, Canvas
domain/fade/dirty behavior, input, environment/logging, SysCmdLine, and
CommandLine/GLString/SplashText tokens. `ILLUMO_ENABLE_COVERAGE=ON` adds the
Clang/LLVM `IllumoCoverage` target, an 85% production-line gate, and an HTML
report. They do not prove that the live OpenGL window, native dialogs, or
non-Windows ports work. Run a proportional manual smoke test for those paths.

## Change rules

- Follow `docs/contributing.md`: avoid `auto`, avoid namespaces, do not add
  recursion, and do not add third-party dependencies without user approval.
- **Run `clang-format`** on all modified files before finalizing tasks to conform to the Mozilla-based configuration.
- **Enforce naming conventions** on new/modified code (Classes: `PascalCase`, Functions/Variables: `camelCase`, etc.). See `docs/contributing.md` for the full mapping.
- Prefer explicit ownership and narrow dependencies.
- Keep Game and Rulesets independent of raw OpenGL. Preserve the token-first
  render boundary and MockBackend testability.
- Do not grow `IllumoContext` casually. If a genuinely different third module
  appears, prefer explicit constructor dependencies.
- When adding a ruleset, update the factory/known-mode logic, console help and
  validation, CMake source lists, palette behavior, and focused tests together.
- Architecture changes require an update to
  `docs/architecture-consensus.md`; closed decisions also require a new entry in
  `docs/latex/sections/09-design-decision-log.tex`.
- **Mandatory Pre-Completion Checklist**: Before declaring any task, feature, or behavior change complete, the agent MUST perform both steps in the same turn:
  1. **Verification**: Execute `cmake --build build --config Release` and `ctest --test-dir build -C Release -L Illumo --output-on-failure`.
  2. **Documentation Sync**: Update matching documentation files (`docs/architecture-consensus.md`, `docs/packages/*`, `docs/latex/sections/*`) in the exact same turn before presenting final completion to the user.
- Edit LaTeX/Markdown sources, not generated `.aux`, `.fls`, `.fdb_latexmk`,
  `.log`, `.toc`, PDF, source dumps, ZIPs, or build outputs unless the user
  explicitly requests regenerated artifacts.
- Update this `AGENTS.md` whenever build commands, canonical paths, or durable
  architecture facts change.

## Known hazards; do not fix opportunistically

Confirm each against the current tree before acting:

- A module whose `Start` returns early can still receive `Update` and
  `DispatchDrawables`; the game path can then dereference a null `cellContext`.
- Wireworld lacks a convenient head-placement brush, and startup still seeds a
  Game-of-Life glider even in Wireworld mode.
- `CommandQueue` reserves 2,048 commands, grows to a configurable 65,536
  default ceiling, and reports high-water/rejected counts. Do not remove the
  ceiling or hide rejection metrics.
- Keep sources shared by `Illumo` and `IllumoTests` in `ILLUMO_SHARED_SOURCES`, and
  put common target settings in `illumo_configure_target`; preserve target-only
  behavior such as the application's Debug Tracy instrumentation.
- Register each new logical behavior as its own exact `Illumo.<area>.<case>`
  entry. Do not collapse new coverage back into one monolithic CTest result.
- Keep `docs/packages/` maps synchronized with the canonical architecture; do not
  let current-state summaries drift back to the archived R8-only or entity
  scaffolding descriptions.
- Keep general console commands in `CommandLine` and domain behavior registered
  by `CellGameModule` through `CommandRegistry`; usage, descriptions, and argument
  completion data belong with the registered command.

Treat `docs/current-issues.md` as a review snapshot, not a guarantee that every
item remains open. Reproduce or inspect before fixing.

## Generated and historical material

- `build/`, root `CMakeFiles/`, and `CMakeCache.txt`: generated configuration.
- `Illumo/thirdparty/`: vendored dependencies; avoid broad searches and edits.
- `archive/`: intentionally non-live experiments and analysis output.
- `Illumo/.agents/` and `Illumo/Source/.agents/`: prior-agent working records.
- `Illumo/Source/all.txt`, `Illumo/illumo_source.zip`, logs, and LaTeX auxiliaries:
  snapshots or generated artifacts, never the implementation source of truth.
