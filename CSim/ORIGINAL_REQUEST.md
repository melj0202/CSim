# Original User Request

## Initial Request — 2026-06-12T10:30:16-04:00

An overview and fix of the CSim codebase to get it into a fully working state (compiling, linking, running, and properly rendering scene data).

Working directory: c:/Users/gravi/Source/Projects/CSim - Copy/CSim
Integrity mode: demo

## Requirements

### R1. Resolve C++ Compilation and Linker Errors
- Resolve duplicate template definitions in `Source/Util/Math.h` by adding standard header guards (`#pragma once`).
- Fix typo in `Source/System/ModuleObject.h` by replacing `Vec3` with `Vector3`.
- Prevent name conflict in `Source/System/ModuleObject.h` by making `DirtyFlags` a scoped `enum class` or renaming/scoping conflicts with `struct Transform`.
- Add `ObjectID id` member to `ModuleObject` (and update its constructors) so `EntityTable` can compile and properly retrieve object IDs.
- Fix inheritance in `Source/Core/Canvas.h` so that `Canvas` inherits from `Drawable<Canvas>` and remove the incorrect `ModuleObject()` call from the `Canvas` constructor.
- Fix include error in `Source/Rendering/Scene.h` by changing `Rendering/Camera2D.h` to `Rendering/Camera.h`, and declare `IRenderWindow* window` and `std::unordered_map<unsigned long, SceneObject*> nodeLookup` as member variables in `Scene`.

### R2. Implement Scene Frame-by-Frame Dispatch Architecture
- Implement `ClearDrawables()` and `AddDrawable(DrawableBase* drawable)` in `Scene` to manage a list of active drawables for the current frame.
- Implement `Update()` in `Scene` to clear the OpenGL color/depth buffers (using a clean color like dark gray/black) and iterate through the active drawables, calling `Draw()` on each.

### R3. Fix Main Loop Exit in CellMain.cpp
- Modify the infinite loop in `Source/System/CellMain.cpp` to check `!illumo->ShouldClose()` so the application exits cleanly when the GLFW window is closed.

## Acceptance Criteria

### Build & Run
- [ ] The CSim project compiles cleanly using CMake/MSBuild on Windows with no errors.
- [ ] The executable links and launches successfully.
- [ ] Closing the window terminates the process cleanly without hanging.

### Rendering
- [ ] The simulation window launches and renders the canvas/scene contents.
