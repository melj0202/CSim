# Illumo headless tests

Executable target: **`IllumoTests`** (alias: `IllumoRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend / Renderer | `TestMockBackend.cpp`, `TestRendererE2E.cpp` | Token queue, backend injection, styles, scene layers, texture updates |
| RuleSets | `TestRuleSets.cpp` | Pure transitions, palettes, tags, active modes, complete 256x9 transition-table equivalence and one-time caching |
| CellContext | `TestCellContext.cpp` | Sparse view construction, mode factory, env view size |
| Sparse canvas | `TestCanvasInf.cpp` | Signed negative mapping, bounded chunk visits, unbounded chunks, stable revisions, separate stored/counting masks, transactional cached totals and candidate-preference counts across edits/assignment/clear/full/frontier/swap paths, adaptive frontier work selection beyond the former 64-target cliff, sparse frontier candidate masks and forced-complete byte identity, broad-frontier rejection, per-target candidate/halo selection in mixed worlds, dense Wireworld conductor candidates, retained candidate scratch/index capacity and generation reset, retained complete-halo target/index/result capacity and generation reset, flat-index growth and negative-coordinate lookup, lazy candidate-counter reset, source-serial and target-parallel preparation across signed chunk edges/corners, preparation worker selection, zero-allocation steady chunk-node reuse, coarse candidate-range granularity, serial/parallel candidate byte identity, retained changed-frontier zero-work still lifes, local activity/full-step identity, edit and ruleset invalidation, all shipped rules' candidate/full-chunk identity including direct counting-mask rolling rows, sparse wide-colony selection, dense parallel fallback, edge/corner halos, multi-state transitions, changed-tile near-view sampling for edits/removals/generations, full-resample fallbacks for revision gaps/overview/grid replacement, active-fade work counts, repeated zero-speed configuration, geometric texture-capacity reuse and explicit release, revision-gated upload, cursor alignment, and camera reveal |
| CellGameModule | `TestCellGameModule.cpp` | Editing/commands, one-step guarantee and measured second-step frame budget, requested/achieved TPS and timing status, full-source randomization at far zoom, centered seeds, sparse v2 save/load, legacy import, validation, camera restoration |
| Compatibility domain | `TestCanvasDomain.cpp`, `TestDomainBoundary.cpp`, `TestSim.cpp` | Retained dense compatibility and legacy ruleset behavior plus dense, dense-sparse, 16,384-colony candidate, changed-frontier, and Wireworld conductor candidate/halo microbenchmark reporting |
| Services / UI / allocators | `TestServices.cpp`, `TestUITokens.cpp`, `TestAllocators.cpp`, `TestGameVisual.cpp` | Input, camera, console, allocators, primitive composition |

CTest invokes one exact logical `Illumo.*` case per process with no OpenGL
context. The suite does not replace a manual Windows/OpenGL smoke test.

## Build & run

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -L Illumo --output-on-failure
```
