# Release configuration menu and finite toroidal worlds

Status: authorized for implementation on 2026-08-17.

## Objective and measurable end state

Ship a configuration menu in both Release and Debug builds so ordinary users
can change the active ruleset, world topology, simulation timing, fade speed,
VSync, and fullscreen mode without the Debug-only console. A finite world is a
centered rectangle of whole 16x16 chunks whose opposite edges are adjacent.
`0 / 0` and `inf / inf` select the existing infinite, non-toroidal sparse
world. Mixed finite/infinite dimensions are invalid.

The feature is complete when:

- F1 opens a primitive-composed settings overlay in Release and Debug;
- the overlay supports keyboard entry and Apply/Cancel/Exit without a retained
  widget system;
- positive chunk dimensions produce deterministic toroidal editing,
  rendering, evolution, save/load, and asynchronous publication;
- zero or `inf` dimensions preserve the existing optimized infinite path;
- changing topology starts a fresh world instead of ambiguously folding old
  cells into new coordinates;
- sparse save version 3 preserves topology while version 2 and legacy dense
  files remain readable;
- focused tests and the complete Release suite pass.

## Current-state evidence

- `SparseCellGrid` currently stores signed 64-bit cells in unbounded 16x16
  chunks and its optimized candidate/halo/frontier paths assume adjacent chunk
  addresses never wrap.
- `CanvasView` is a bounded camera presentation over that unbounded domain.
  `CanvasX` and `CanvasY` are initial texture-capacity hints in cells, not world
  dimensions.
- `CellGameModule` owns ruleset selection, editing, timing, persistence, and
  its existing UI drawables. `DebugModule` is not compiled into Release.
- `GameVisual` and `UiTheme` are the supported primitive-composed product UI
  path.
- `EnvVars` persists configuration beside the executable. VSync is observed on
  the next swap; fullscreen changes through `IRenderWindow`.

## Scope

### In scope

- New `WorldChunksX` and `WorldChunksY` configuration values.
- A sparse finite-torus generation path that is selected only for finite
  topology.
- Canonical wrapping for domain reads, edits, and simulation, presentation
  clipped to the centered finite rectangle, and full bounded cache refreshes
  after finite-world changes.
- An F1 settings menu with ruleset, width/height in chunks, TPS, speed factor,
  fade speed, VSync, fullscreen, Apply, Cancel, and Exit.
- Save format version 3 with topology metadata and backward-compatible reads.
- Focused tests, README/package/architecture/design-book updates, and durable
  guidance updates for the new topology and persistence contract.

### Non-goals

- Replacing the existing optimized infinite candidate/halo/frontier pipeline.
- A generalized widget toolkit, retained UI tree, or independent renderer.
- Arbitrary cell-sized finite worlds; finite dimensions are whole chunks.
- Converting cells when topology changes. The reset is deliberate and visible.
- Claiming non-Windows runtime support.

## Constraints and invariants

- Infinite mode keeps byte-for-byte behavior and does not pay finite-world
  normalization in its generation hot path.
- Finite coordinates have one canonical chunk/cell representation. Both main
  and spare grids always use identical topology.
- Background state remains implicit. Candidate discovery includes every stored
  cell and all wrapped neighbors of neighbor-counting cells.
- Only one simulation generation may be outstanding. Topology/ruleset changes
  drain the worker before mutating or replacing grids.
- Command payload pointers and menu visual storage remain valid through
  synchronous renderer submission.
- Invalid text, mixed infinite/finite dimensions, non-finite numbers, and
  out-of-range values do not partially apply settings.

## Proposed design

### Topology value and canonicalization

`SparseCellGrid` stores two signed 64-bit chunk extents. Both zero means
infinite; both positive means finite toroidal. Finite chunk coordinates are
centered around chunk zero using `minimum = -(extent / 2)`. Cell coordinates
wrap with floor-modulo into that canonical chunk rectangle. Reads and edits
canonicalize at the public grid boundary. Chunk import validates canonical
addresses.

The infinite `advanceImpl` path stays unchanged. A finite-only sparse candidate
path groups candidates by canonical target chunk, enrolls every stored cell,
adds wrapped Moore-neighbor contributions from counted cells, evaluates the
existing transition table, and installs results through retained next-map node
storage. This is proportional to stored cells and their local candidates, not
to the full finite rectangle. It may use the existing bounded candidate worker
pool for large workloads.

`visitChunksInBounds` intersects camera requests with the canonical finite
chunk rectangle. `CanvasView` fills all other cache samples with the background
palette color, including density-LOD samples that touch a boundary. Simulation
still wraps opposite-edge neighbors. Finite worlds use a complete bounded
`CanvasView` refill after a revision instead of the infinite-world changed-bin
shortcut.

### Runtime reconfiguration

`CellContext` constructs both sparse grids with the configured topology and
provides a reset operation that allocates replacement grids first, then points
`CanvasView` at the new published grid. `CellGameModule` drains the runner,
applies the ruleset, resets only when topology changes, reseeds, resets the
camera, rebuilds presentation, and persists settings. Failure leaves the old
world and settings intact.

### Menu

`ConfigurationMenu` is a Game-owned drawable/input controller containing one
screen-space `GameVisual` and a small draft settings value. It is not a widget
tree. F1 opens or cancels it. Up/Down select rows; Left/Right cycle choices;
typing and Backspace edit numeric/topology fields; Enter activates toggles or
Apply/Cancel/Exit; Escape cancels. Mouse row selection and action activation use
simple panel hit tests. Exit requests window closure so the App loop performs
normal module and renderer shutdown.

Opening the menu drains the outstanding simulation and suppresses editing,
camera movement, and stepping until it closes. Apply validates the complete
draft transactionally. The menu reports validation errors in place. Opening
uses a short eased panel reveal with staggered rows; selection changes glide the
highlight between rows, and value edits/cycles trigger a brief accent pulse.
These animations are scalar state on the existing controller and never block
input.

### Persistence

Version 3 sparse files use a new `ILLUMO3` magic and append signed 64-bit chunk
width/height metadata before the chunk count. Version 3 load validates the
topology and canonical chunk addresses before replacing live state. Version 2
loads as infinite. Legacy dense files retain the existing centered import into
an infinite world. All new saves use version 3.

## Alternatives considered

- Modify every infinite candidate/halo/frontier address operation to wrap.
  Rejected because it risks regressions in the heavily optimized default path
  and creates duplicate-target corner cases throughout several algorithms.
- Evaluate every cell in the finite rectangle. Rejected because a large sparse
  torus should remain sparse in cost and storage.
- Keep topology only in `envvars.json`. Rejected because loading a finite save
  as infinite changes its behavior.
- Fold existing cells into new dimensions. Rejected because collisions make
  the result order-dependent for multi-state rules; resetting is explicit and
  deterministic.

## Compatibility, ownership, threading, and errors

- Existing version 2 and legacy saves remain readable; version 3 is a forward
  format change for older executables.
- Infinite-mode public behavior, signed coordinate reach, and save import are
  preserved.
- `CellContext` continues to own both grids, the view, and the ruleset.
  `ConfigurationMenu` is owned by `CellGameModule`.
- Menu and topology application are main-thread affine. The simulation runner
  is drained before replacement; no worker observes deleted grids.
- Allocation or parsing failure is reported in the menu and cannot partially
  update environment settings, fullscreen state, topology, or live cells.

## Ordered implementation milestones

1. Add topology value, wrapping access, the finite sparse generation branch,
   and domain tests.
2. Add context reset and version 3 persistence with compatibility tests.
3. Add the menu drawable/input controller and module integration.
4. Synchronize current documentation, formal decision log, build/source maps,
   and guidance.
5. Format, build Release and Debug, run focused and full tests, review the
   final diff, rebuild PDFs, and inspect affected rendered pages.

## Verification strategy

- Exact tests for topology validation, negative/positive wrapping, edit alias
  identity, births and oscillators across horizontal/vertical/corner seams,
  multi-state Wireworld seams, infinite regression, and direct dual-grid
  identity.
- Exact menu tests for defaults, `0`/`inf`, mixed-mode rejection, numeric
  ranges, Apply/Cancel, rendered primitive tokens, and module pause/reset.
- Save/load tests for finite version 3 round trip, version 2 compatibility,
  invalid topology/chunk rejection, and camera/ruleset restoration.
- Release build and full labeled CTest suite; Debug application build because
  the menu coexists with `DebugModule`.
- Manual Windows GUI smoke remains required for pixel layout, real keyboard and
  mouse interaction, VSync, fullscreen, and native OpenGL behavior.

No performance improvement is claimed. A focused Release micro-benchmark will
compare finite sparse seam evolution with an equivalently populated infinite
case to expose gross regressions; the separate infinite path is the containment
boundary.

## Rollback and containment

The new finite branch is selected only when both extents are positive. Removing
the menu settings or setting both extents to zero returns to the unchanged
infinite path. Version 3 loading is isolated behind its magic/version branch;
version 2 and legacy readers remain available.

## Decisions and remaining questions

- The user selected finite toroidal behavior and explicitly authorized
  `0 / 0` or `inf / inf` as infinite on 2026-08-17.
- The user approved the proposed first-menu setting set and F1 access.
- The user clarified on 2026-08-17 that finite presentation must not repeat
  canonical aliases: camera space outside the centered world stays blank.
- Mixed finite/infinite dimensions are rejected to keep topology coherent.
- Finite dimensions are capped at 1,000,000 chunks per axis to keep cell-period
  arithmetic comfortably within signed 64-bit range.
- No unresolved design question blocks implementation.

## Validation results

- Release application and test targets built successfully. The full labeled
  suite passed 172/172 tests, including the topology, bounded finite
  presentation, menu, module, and
  persistence cases added here.
- The Debug application target built successfully, proving that the Release
  menu composes with `DebugModule`.
- The focused Release micro-benchmark evolved 1,024 cells for 100 generations:
  finite torus 13.274 ms total; infinite topology 0.067 ms total. The resulting
  cells were identical away from seams. This is containment evidence, not a
  performance claim.
- `docs/build.ps1` rebuilt both PDFs successfully. The affected Illumo book
  pages and architecture-map pages were rendered to images and visually
  checked after correcting two layout defects.
- Existing third-party/compiler/documentation warnings remain: vendored
  FreeType conversions, unused `RenderWindow` parameters, a `SysCmdLine`
  size conversion, the Debug ASAN/incremental-link warning, locale fallback,
  and older LaTeX box warnings outside this change.
- A real Windows OpenGL GUI smoke was attempted, but the Windows app-control
  approval timed out before a target window was returned. Pixel-level menu
  appearance, physical keyboard/mouse interaction, VSync, and fullscreen still
  require that manual check.
