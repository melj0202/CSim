# CSim (Cell Simulator)

An improved version of a previous Game of Life program (OpenGL / GLFW).

## Layout

```
csim/
  docs/                 # Design notes (LaTeX) — living architecture memory
  CSim/                 # Project root (CMake)
    Source/             # Application source packages
      App/              # CellMain loop
      Engine/           # Illumo runtime + modules
      Game/             # CA game domain
      Rulesets/         # Cellular automata rules
      Rendering/        # Graphics / backend interfaces
      Services/         # Log, input, env, CLI, allocators
      Foundation/       # Macros, math, sysinfo
      Assets/           # Asset loaders
      Platform/         # OS entry + native save/load
      Tests/
    Shader/             # GLSL shaders
    Assets/             # Runtime asset files (fonts, …)
    thirdparty/         # Vendored dependencies
  archive/              # Historical / non-build material
```

## Design documentation

Architecture, decision log, and performance notes live in:

- `docs/csim-design-notes.tex` (build with `pdflatex` / `latexmk` from `docs/`)
- `docs/csim-design-notes.pdf` (generated)
- `docs/README.md` — how to build and where to edit

**Current stack (short):** token renderer (`AppendCommands` → `IBackend`), Canvas as life grid + RGB fade display + dirty-rect upload, double-buffered CA `nextState`, headless `CSimTests` with `MockBackend`.

**Architecture consensus (for later sessions):** [`docs/architecture-consensus.md`](docs/architecture-consensus.md) — merged view of strengths, decisions, known issues, and recommended work order.

## Build (CMake)

From the repository root:

```bash
cmake -S CSim -B build
cmake --build build --config Release --target CSim
cmake --build build --config Release --target CSimTests
```

The build folder will contain the executable binaries (`build/Release/CSim.exe`, `CSimTests.exe` on multi-config generators).

Headless tests (no GPU):

```bash
ctest -C Release -R CSimTests --output-on-failure
# or: build/Release/CSimTests.exe
```

Visual Studio: open the generated solution from the build directory, or generate with the VS generator.

## Controls

- **E** — Toggle Edit / Normal mode  
  (Simulation starts in edit mode, same as paused)
- **Left mouse** (Edit) — Place living cells
- **Right mouse** (Edit) — Place dead cells
- **C** (Edit) — Clear the cell colony
- **Q** / **ESC** — Quit
- **Ctrl+S** (Edit) — Save canvas state
- **Ctrl+O** (Edit) — Load canvas state
