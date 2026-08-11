# Illumo (Cell Simulator)

An improved version of a previous Game of Life program (OpenGL / GLFW).

## Layout

```
illumo/
  docs/                 # All first-party documentation and LaTeX sources
    latex/              # Prose-book and chart-pack PDF entrypoints
    packages/           # Source-package maps formerly scattered by code
    sessions/           # Dated implementation and verification records
    history/            # Superseded/original material
  Illumo/               # Project root (CMake)
    Source/             # Application source packages
      App/              # CellMain loop
      Engine/           # Illumo runtime + modules
      Game/             # CA game domain
      Rulesets/         # Cellular automata rules
      Rendering/        # Graphics / backend interfaces
      Services/         # Log, input, env, CLI, allocators
      Foundation/       # Macros and shared helpers
      Platform/         # OS entry + native save/load
      Tests/
    Shader/             # GLSL shaders
    Assets/             # Runtime asset files (fonts, …)
    thirdparty/         # Vendored dependencies
  archive/              # Historical / non-build material
```

## Design documentation

All first-party architecture, decision, package, history, and build notes live
under `docs/`. Start with:

- `docs/README.md` — documentation map and PDF build commands
- `docs/architecture-consensus.md` — canonical current architecture
- `docs/latex/illumo.tex` — the canonical prose-book entrypoint
- `docs/latex/architecture-map.tex` — the current chart-only entrypoint
- `docs/output/*.pdf` — generated locally; never sources of truth

**Current stack (short):** reusable 2D token renderer (`AppendCommands` →
`IBackend`) with typed generational handles, painter-correct primitives,
dynamic quad buffers, primitive-composed themed UI, and asynchronous
texture/shader assets,
`SparseCellGrid` domain + revision-gated `CanvasView` RGB-fade presentation +
dirty-rect upload, retained flat-indexed chunk-local cell-candidate scratch /
per-target candidate-or-halo CA `nextState`, separate stored/counting masks,
coarse parallel evaluation, recycled transactional chunk-map nodes,
changed-region frontier stepping, cached 256x9 rule transitions and rolling-row
halo counts, headless `IllumoTests` with
`MockBackend`. `CanvasView` presents its bounded texture on a world-space
`GameVisual` sprite. Windows is the supported runtime; Linux and macOS retain
stale source/CMake scaffolding that is not currently buildable against the
shared bootstrap API.

**Architecture (single source for later sessions):** [`docs/architecture-consensus.md`](docs/architecture-consensus.md) — unified consensus (purpose, history of old plans, current renderer/sim truth, decisions, bugs, debt, work order).

Contribution rules are in [`docs/contributing.md`](docs/contributing.md).

Third-party software and font acknowledgements, license choices, and the
source/binary redistribution checklist are in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## Build (CMake)

From the repository root:

```bash
cmake -S Illumo -B build
cmake --build build --config Release
```

The default build builds `IllumoTests` and runs every registered case through
CTest. The build folder contains the executable binaries
(`build/Release/Illumo.exe`, `IllumoTests.exe` on multi-config generators).

When Windows PowerShell and `latexmk` are on `PATH`, the default build also runs
`IllumoDocs` and writes `docs/output/illumo.pdf`. Disable that optional target
at configure time with `-DILLUMO_BUILD_DOCUMENTATION=OFF`.

Headless tests (no GPU):

```bash
ctest --test-dir build -C Release -L Illumo --output-on-failure
ctest --test-dir build -C Release -N -L Illumo
# focused: build/Release/IllumoTests.exe --run Illumo.CellGame.SaveLoadRoundTrip
```

CTest registers one process-isolated entry per logical case. `IllumoTests.exe`
is shared only to avoid recompiling the same production sources for every
case; use
`IllumoTests.exe --list` to print the exact names. Each case gets its own
working directory under `build/Testing/Illumo/`.

Clang/LLVM coverage (85% production-line gate and HTML report):

```bash
cmake -S Illumo -B build-coverage -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DILLUMO_BUILD_DOCUMENTATION=OFF -DILLUMO_ENABLE_COVERAGE=ON
cmake --build build-coverage --target IllumoCoverage
```

The report measures headless-testable first-party production code. Tests,
vendored/system code, `MockBackend`, and the live OpenGL backend are excluded;
native dialogs, window behavior, and live OpenGL still require smoke testing.
See `Illumo/Source/Tests/README.md` for the exact scope and commands.

### Optimized Tracy profiling

Keep the normal Release optimization level while enabling application Tracy
instrumentation:

```bash
cmake -S Illumo -B build-profile -DILLUMO_ENABLE_TRACY=ON -DILLUMO_BUILD_DOCUMENTATION=OFF
cmake --build build-profile --config Release
```

Visual Studio: open the generated solution from the build directory, or generate with the VS generator.

## Controls

- **E** — Toggle Edit / Normal mode  
  (Simulation starts in edit mode, same as paused)
- **Left mouse** (Edit) — Place living cells
- **Right mouse** (Edit) — Place dead cells
- **C** (Edit) — Clear the cell colony
- **Q** / **ESC** — Quit
- **`** — Toggle the developer console
- **Console:** **Tab** completes commands, variables, and rulesets; **Left/Right**, **Home/End**, and **Delete** edit in place; hold **Ctrl** with Left/Right or Backspace/Delete for word edits; hold **Shift** while moving to select; **Ctrl+A** selects all

## Launch options

The executable keeps its persisted configuration in `envvars.json` beside the
executable, independent of the process working directory. A first build places
the tracked defaults there without overwriting an existing local configuration.
Command-line dimensions override the persisted values:

```text
illumo.exe [-ww width] [-wh height] [-cw canvas-width] [-ch canvas-height]
illumo.exe --help
illumo.exe --version
```

## Developer console commands

The in-app console is provided by `DebugModule`, so it is available in Debug
builds only. Type `help` for the live list or `help <command>` for details.

| Group | Commands |
|---|---|
| Simulation | `pause`, `run`, `step [count]`, `status` |
| Canvas | `clear_canvas`, `randomize [percent]`, `setcell <x> <y> <state>` |
| Rules and files | `ruleset [name]`, `save <file>`, `load <file>`, `save_dialog`, `load_dialog` |
| Camera and display | `camera [x y [zoom]]`, `camera_reset`, `fullscreen`, `fps` |
| Renderer diagnostics | `renderer_demo [on|off]`, `assets`, `asset_reload <all|path>` |
| Timing | `tps`, `speed`, `fade` |
| Environment | `get`, `set`, `toggle`, `vars [filter]` |
| Console/app | `help`, `echo`, `clear`, `close`, `quit` |

Save commands append `.illumo` when no extension is supplied. Loading validates
the save before changing the canvas and activates the ruleset stored in it.
