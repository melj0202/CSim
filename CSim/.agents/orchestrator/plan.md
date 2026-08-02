# Project Plan: CSim Codebase Fixes

## Architecture
CSim is a simulation environment utilizing OpenGL for rendering. It consists of:
- `Source/Platform` for platform-specific windowing (e.g. GLFW, GTK, Cocoa).
- `Source/Core` containing `Canvas`, `CellGameModule`, etc.
- `Source/Rendering` containing `Scene`, `Camera`, etc.
- `Source/System` containing `ModuleObject`, `CellMain`, etc.
- `Source/Util` containing mathematical helpers like `Math.h`.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | M1: Compilation & Linker Fixes | Fix Math.h templates, ModuleObject Vector3/DirtyFlags/id, Canvas inheritance/constructor, Scene includes/members. | None | PLANNED |
| 2 | M2: Frame-by-Frame Dispatch | Implement ClearDrawables(), AddDrawable(), and Update() in Scene. | M1 | PLANNED |
| 3 | M3: Main Loop & Clean Exit | Modify CellMain.cpp loop to check should-close status. | M1 | PLANNED |
| 4 | M4: End-to-End Verification | Build CSim, run executable, verify rendering, exit without hanging. | M1, M2, M3 | PLANNED |

## Code Layout
- `Source/Util/Math.h` - Template math functions.
- `Source/System/ModuleObject.h` / `ModuleObject.cpp` - Base simulation objects.
- `Source/Core/Canvas.h` / `Canvas.cpp` - Canvas drawable object.
- `Source/Rendering/Scene.h` / `Scene.cpp` - Main scene graph and render management.
- `Source/System/CellMain.cpp` - Entry point and main loop.
- `CMakeLists.txt` - Build configuration.
