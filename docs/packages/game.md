# Game

The live game path is an unbounded sparse cellular-automata world plus a
bounded presentation view.

## SparseCellGrid (simulation domain)

- Authoritative signed 64-bit cell coordinates.
- Hash-map storage of non-background 16x16 chunks; there is no fixed chunk
  count or allocator-pool cap.
- Negative coordinates use centralized floor division/modulo. All byte states
  are preserved, including Brian's Brain and Wireworld values.
- Each chunk maintains compact masks for stored non-background cells and cells
  that contribute to neighbor counts. Sparse
  generations evaluate only every non-background cell plus the eight-neighbor
  birth candidates of counted cells. Exact affected chunk addresses are created
  directly through a retained generation-stamped open-addressed index; its slots
  point into contiguous scratch records carrying a 256-bit candidate mask and
  256 neighbor counts. Index and scratch capacity survive generation resets,
  avoiding per-world-cell hash nodes, sorting, binary searches, and repeated
  candidate allocation. Candidate sets with at least 16,384 cells are divided
  into retained ranges of roughly 2,048 candidate cells and evaluated through
  the reusable pool with up to four automatic workers. Each range writes
  independent result slots; small sets retain the direct serial path. Complete
  mixed worlds choose candidates or a deterministic 18x18 halo independently
  for each target from its counted-neighbor contribution work. This lets dense
  Wireworld conductors use candidates because only heads contribute neighbor
  counts. Worlds whose source chunks are all densely counted bypass candidate
  scratch construction. At 32 or more halo targets, a grid-owned reusable pool
  uses up to eight workers. Dense 18x18 halos use a rolling three-row stencil
  instead of rescanning eight neighbors per output cell. Empty results are not stored.
- Both paths write into a retained inactive chunk map. Old inactive nodes are
  extracted into a retained handle vector and reinserted with new keys/data,
  preserving transactional map comparison/swap while eliminating steady-state
  node allocation at a stable chunk-count high-water mark.
- The inactive map also remains the prior-generation baseline. Retained flat
  address sets track changed chunks and expand them by one chunk in every
  direction. Expansions of at most 64 targets are evaluated and patched locally;
  empty frontiers return immediately, while broad changes fall back to complete
  candidates or halos. Editing and ruleset-type changes repopulate or invalidate
  the frontier explicitly.
- Its revision changes only when a generation or edit changes the stored cell
  contents, allowing dependent views to skip idle resampling.
- Rulesets supply pure `nextState` and `evalCell` behavior. Each ruleset's
  complete 256x9 transition table is cached once and shared by all serial and
  worker hot loops. The old dense
  `CellGrid`/`Canvas` implementation remains only for compatibility coverage.

## CanvasView (presentation)

- Samples contiguous 16x16 world cells around the camera. Near zoom uses one
  exact texel per cell; far zoom uses a density-colored overview bounded to
  roughly four screen pixels per texel.
- Owns one reusable RGB texture and one world-space, cell-aligned quad through
  `GameVisual`.
- Uses nearest filtering so discrete cell colors stay sharp; the editor cursor
  uses the same centered cell bounds.
- CPU palette targets fade through `displayRgb`; newly revealed cells snap to
  their current color. Stable grid/camera/palette state skips resampling and
  texture upload; dirty updates remain bounded to at most one update per
  drawable submission.
- Overview sampling visits only sparse chunks intersecting the visible source
  region. The visual texel budget does not limit stored chunks or world cells.
- `CellGameModule` dispatches the view on the World layer and the cursor/splash
  on UI.

## CellGameModule

EDIT / NORMAL; simulation uses `tps` x `speedFactor`, advances at most two
generations per rendered frame, and drops excess catch-up debt. Painting,
Bresenham strokes, `setcell`, randomization, and clearing operate directly on
signed world coordinates. Startup patterns are centered around `(0, 0)` and
simulation is non-toroidal.

`status` reports output chunk nodes allocated, reused, and retained alongside
the simulation path, stored/counting cell counts, candidate/halo target counts,
changed/frontier counts, candidate work ranges, workers, and frame-step debt.

Save always writes version 2 sparse files containing the ruleset, camera, and
deterministically sorted chunks. Load validates temporary state first, reads
both version 2 and the prior dense format, imports legacy cells around the
world origin, and restores saved ruleset/camera metadata.

Wireworld retains the sticky head/empty/tail/conductor brush (`1`/`H`, `2`,
`3`/`T`, `4`).
