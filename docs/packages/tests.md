# Illumo headless tests

Executable target: **`IllumoTests`** (alias: `IllumoRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend / Renderer | `TestMockBackend.cpp`, `TestRendererE2E.cpp` | Token queue, backend injection, styles, scene layers, texture updates |
| RuleSets | `TestRuleSets.cpp` | Pure transitions, palettes, tags, active modes |
| CellContext | `TestCellContext.cpp` | Sparse view construction, mode factory, env view size |
| Sparse canvas | `TestCanvasInf.cpp` | Signed negative mapping, chunk boundaries, unbounded chunks, serial halos, multi-state transitions, bounded view, linear display filtering, fade and camera reveal |
| CellGameModule | `TestCellGameModule.cpp` | Editing/commands, centered seeds, sparse v2 save/load, legacy import, validation, camera restoration |
| Compatibility domain | `TestCanvasDomain.cpp`, `TestDomainBoundary.cpp`, `TestSim.cpp` | Retained dense compatibility and legacy ruleset behavior |
| Services / UI / allocators | `TestServices.cpp`, `TestUITokens.cpp`, `TestAllocators.cpp`, `TestGameVisual.cpp` | Input, camera, console, allocators, primitive composition |

CTest invokes one exact logical `Illumo.*` case per process with no OpenGL
context. The suite does not replace a manual Windows/OpenGL smoke test.

## Build & run

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -L Illumo --output-on-failure
```
