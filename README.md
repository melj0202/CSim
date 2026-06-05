
# Illumo

A general-purpose 2D/3D rendering engine written in C++ with an OpenGL backend. Originally built as a Conway's Game of Life simulation, Illumo grew into a full engine rewrite focused on clean backend abstraction, a structured render pipeline, and an ECS-inspired scene system.

> **Status:** Active development. The core renderer and pipeline are functional. The infinite grid chunking system and additional backend targets (Vulkan, Metal) are in progress.

----------

## Why This Exists

The original version (CSim) worked — but every scene object made direct OpenGL draw calls. That meant the rendering API was baked into every part of the codebase. Swapping OpenGL for Vulkan or Metal would have required touching everything.

Illumo is a ground-up redesign to fix that.

----------

## Architecture (In Progress)

### Backend Abstraction

The renderer never calls OpenGL directly. Instead, draw calls are submitted as commands to a **command queue**. Each command carries a header token identifying the operation. The **Device** class reads the token and dispatches to the appropriate OpenGL call — making the backend swappable in principle without changing anything above it.

```
Scene → Render Pipeline → Render Passes → Draw Calls → Command Queue → Device → OpenGL
```

### Render Pipeline

The **RenderPipeline** holds a collection of **RenderPasses**, each of which owns a set of draw calls. This makes rendering order explicit and configurable rather than implicit in code structure.

### Scene System

Rather than a deep scene graph or manually positioned objects, Illumo uses a flat entity table — closer to an Entity Component System (ECS). The table tracks world position; everything else is referenced by handle. This keeps the scene representation cache-friendly and avoids deep inheritance hierarchies.

### Infinite Grid 

The cell simulation uses a **chunk-based spatial partitioning system** to support an effectively infinite simulation space, loading and unloading chunks based on viewport position.

----------

## Features

-   OpenGL/GLFW rendering backend
-   Structured render pipeline with explicit render passes (in progress)
-   ECS-inspired flat scene table
-   Interactive edit mode — place and erase live cells with the mouse
-   Save and load canvas state (Ctrl+S / Ctrl+O)
-   Chunk-based infinite grid (in progress)

----------

## Controls

Key / Input

Mode

Action

E

Normal

Toggle Edit Mode

Left Mouse

Edit

Place living cell

Right Mouse

Edit

Place dead cell

C

Edit

Clear canvas

Ctrl+S

Edit

Save canvas state

Ctrl+O

Edit

Load canvas state

Q / ESC

Any

Quit

----------

## Build Instructions

Requires CMake and a C++23-compatible compiler. OpenGL and GLFW must be available on your system.

bash

```bash
git clone https://github.com/melj0202/CSim
cd CSim
mkdir build && cd build
cmake ../
make
```

The compiled binaries will be in the `build/` directory.

----------

## What I Learned

-   Designing a **rendering abstraction layer** that decouples scene logic from API-specific calls
-   Implementing a **command buffer/token dispatch** pattern from first principles
-   C++ **smart pointers, ownership semantics, and move semantics** in a real codebase
-   **Spatial partitioning** via chunking for infinite simulation grids
-   Memory debugging with **Visual Studio** and **Tracy**— tracking down dangling pointers and memory leaks
-   Reading **compiler-generated assembly** to understand and optimize hot paths
-   The difference between C++ cast types (`static_cast`, `dynamic_cast`, `reinterpret_cast`) and when each is appropriate

----------

## Lineage

Illumo is the third iteration of this project:

1.  **game_of_life** — Written in C with ncurses. Terminal-rendered. The original.
2.  **CSim** — Rewritten in C++ with OpenGL/GLFW. Introduced real-time rendering.
3.  **Illumo** — Full architectural rewrite. Render pipeline abstraction, ECS scene system, chunked infinite grid.
4. 

## Future Plans
I have plans to implement the following features in the future

1. Scripting System (with Lua or John Carmack/QVM style bytecode)
2. Additional graphical effects (AA, Shadow Maps, and other Post-Processing
3. Hot Loading shaders
---------

## Third Party Dependencies

1. FreeType
2. GLEW
3. GLFW
4. GLM
5. libjpeg
6. json
7. stb
8. tracy
