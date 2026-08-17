# Illumo headless tests

Executable target: **`IllumoTests`** (alias: `IllumoRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend / Renderer | `TestMockBackend.cpp`, `TestRendererE2E.cpp` | Typed handle isolation, generations, replace/destroy, stale commands, growable queue metrics, scissor state, backend injection, styles, scene layers, texture updates |
| RuleSets | `TestRuleSets.cpp` | Pure transitions, palettes, tags, active modes, complete 256x9 transition-table equivalence and one-time caching |
| CellContext | `TestCellContext.cpp` | Sparse view construction, mode factory, env view size |
| Sparse canvas | `TestCanvasInf.cpp` | Signed negative mapping, bounded chunk visits, unbounded chunks, stable revisions, separate stored/counting masks, transactional cached totals and candidate-preference counts across edits/assignment/clear/full/frontier/swap paths, exact changed/counting masks with interior/edge target gating, adaptive frontier work selection without the former 64-target cliff, sparse frontier candidate masks and forced-complete byte identity, broad-frontier rejection, per-target candidate/halo selection in mixed worlds, dense Wireworld conductor candidates, exact repeated-neighborhood memo hits, uncached byte identity, bounded memo memory, adaptive activation, ruleset invalidation and candidate bypass, retained candidate scratch/index capacity and generation reset, retained complete-halo target/index/result capacity and generation reset, insertion-only flat-index growth, duplicate enrollment, 75% load boundary, negative-coordinate lookup, exact produced-chunk diagnostics, lazy candidate-counter reset, source-serial and direct-source target-parallel preparation across signed chunk edges/corners, exact alternating topology reuse and edit invalidation, direct broad generation without snapshot mirroring, preparation worker selection and coarse range granularity, zero-allocation steady chunk-node reuse, coarse candidate-evaluation ranges, serial/parallel candidate byte identity, retained changed-frontier zero-work still lifes, local activity/full-step identity, edit and ruleset invalidation, all shipped rules' candidate/full-chunk identity including direct counting-mask rolling rows, incremental mirror overlap/non-overlap equivalence, full-replacement node-recycling equivalence, stage-timing provenance, padded camera-cache reuse and refill boundaries, integer LOD hysteresis, exact and overview changed-bin sampling, active-fade work counts, repeated zero-speed configuration, bounded dirty upload rectangles and cap fallback, direct-upload cutoff and all-PBO-busy fallback policy, geometric texture-capacity reuse and explicit release, ordered dual-grid delta publication, shutdown draining, cursor alignment, and camera reveal |
| CellGameModule | `TestCellGameModule.cpp` | Editing/commands, non-blocking one-in-flight scheduling and debt dropping, requested/achieved TPS and rolling timing status, async pause/save/load/ruleset/manual-step drains, full-source randomization at far zoom, centered seeds, sparse v2 save/load, legacy import, validation, camera restoration |
| Compatibility domain | `TestCanvasDomain.cpp`, `TestDomainBoundary.cpp`, `TestSim.cpp` | Retained dense compatibility and legacy ruleset behavior plus dense, dense-sparse, 16,384-colony candidate, changed-frontier, Wireworld conductor candidate/halo, repeated-neighborhood memo versus uncached throughput, random/candidate regression controls, and warmed 30+300 frame-latency plus async stage reporting |
| Assets / services / UI / allocators | `TestRuntimeUtilities.cpp`, `TestServices.cpp`, `TestUITokens.cpp`, `TestAllocators.cpp`, `TestGameVisual.cpp` | Asset cache/references/fallback/reload/cancellation/shutdown, input, camera, console, presentation pacing defaults and FPS-label semantics, primitive label/panel composition, splash chrome, allocators, transforms, atlas UV/flips, painter batching, dynamic capacity, custom styles, animation modes |

CTest invokes one exact logical `Illumo.*` case per process with no OpenGL
context. Cases use a 30-second safety timeout by default; the hardware-dependent
`Illumo.Sim.FrameLatencyBench` keeps its full 30-warmup/300-generation workload
and uses a 180-second safety timeout so completion time is not treated as a
performance assertion. The suite does not replace a manual Windows/OpenGL or
native-dialog smoke test.

## Build & run

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -L Illumo --output-on-failure
```
