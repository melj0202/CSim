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

Architecture, decision log, and the render-token migration plan live in:

- `docs/csim-design-notes.tex` (build with `pdflatex` / `latexmk` from `docs/`)
- `docs/README.md` — how to build and where to edit

Use that document across chat sessions so design intent is not lost.

## Build (CMake)

From the repository root:

```bash
cmake -S CSim -B build
cmake --build build
```

The build folder will contain the executable binaries.

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
