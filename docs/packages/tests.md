# Illumo headless tests

Executable target: **`IllumoTests`** (alias: `IllumoRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend / Renderer | `TestMockBackend.cpp`, `TestRendererE2E.cpp` | Typed handle isolation, generations, replace/destroy, stale commands, growable queue metrics, scissor state, backend injection, styles, scene layers, texture updates |
| RuleSets | `TestRuleSets.cpp` | Pure transitions, palettes, tags, active modes, complete 256x9 transition-table equivalence and one-time caching |
| CellContext | `TestCellContext.cpp` | Sparse view construction, mode factory, env view size |
| Sparse canvas | `TestCanvasInf.cpp` | Signed negative mapping, bounded chunk visits, unbounded chunks, stable revisions, separate stored/counting masks, per-target candidate/halo selection in mixed worlds, dense Wireworld conductor candidates, retained candidate scratch/index capacity and generation reset, flat-index growth and negative-coordinate lookup, zero-allocation steady chunk-node reuse, coarse candidate-range granularity, serial/parallel candidate byte identity, retained changed-frontier zero-work still lifes, local activity/full-step identity, edit and ruleset invalidation, all shipped rules' candidate/full-chunk identity (including rolling halo counts), sparse wide-colony selection, dense parallel fallback, edge/corner halos, multi-state transitions, adaptive overview, revision-gated upload, cursor alignment, fade and camera reveal |
| CellGameModule | `TestCellGameModule.cpp` | Editing/commands, responsive two-step timing, full-source randomization at far zoom, centered seeds, sparse v2 save/load, legacy import, validation, camera restoration |
| Compatibility domain | `TestCanvasDomain.cpp`, `TestDomainBoundary.cpp`, `TestSim.cpp` | Retained dense compatibility and legacy ruleset behavior plus dense, dense-sparse, 16,384-colony candidate, changed-frontier, and Wireworld conductor candidate/halo microbenchmark reporting |
| Assets / services / UI / allocators | `TestRuntimeUtilities.cpp`, `TestServices.cpp`, `TestUITokens.cpp`, `TestAllocators.cpp`, `TestGameVisual.cpp` | Asset cache/references/fallback/reload/cancellation/shutdown, input, camera, console, primitive label/panel composition, splash chrome, allocators, transforms, atlas UV/flips, painter batching, dynamic capacity, custom styles, animation modes |

CTest invokes one exact logical `Illumo.*` case per process with no OpenGL
context. The suite does not replace a manual Windows/OpenGL smoke test.

## Build & run

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -L Illumo --output-on-failure
```
