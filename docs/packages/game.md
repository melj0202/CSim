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
  that contribute to neighbor counts, plus cached counts for both masks. The
  authoritative and retained inactive maps maintain aggregate stored-cell,
  counted-cell, and candidate-preferred-chunk totals transactionally across
  edits, assignment, clear, local frontier patches, and complete map swaps.
  Settled stepping and complete-path selection therefore do not scan all
  allocated chunks. Sparse
  generations evaluate only every non-background cell plus the eight-neighbor
  birth candidates of counted cells. Exact affected chunk addresses are created
  directly through a retained generation-stamped open-addressed index; its slots
  point into contiguous scratch records carrying a 256-bit candidate mask and
  256 neighbor counts. Edge and corner participation is derived directly from
  counting-mask words rather than by rescanning counted cells, and each
  neighbor counter is initialized only when its candidate bit is first set.
  Index and scratch capacity survive generation resets,
  avoiding per-world-cell hash nodes, sorting, binary searches, and repeated
  candidate allocation. Serial preparation remains source-centric for cache
  locality. Large or explicitly parallel preparation is target-centric, so
  independent scratch records can use the same reusable pool without shared
  writes. Candidate sets with at least 16,384 cells are divided
  into retained ranges of roughly 2,048 candidate cells and evaluated through
  the reusable pool with up to four automatic workers. Each range writes
  independent result slots; small sets retain the direct serial path. Complete
  mixed worlds choose candidates or a deterministic 18x18 halo independently
  for each target from its counted-neighbor contribution work. This lets dense
  Wireworld conductors use candidates because only heads contribute neighbor
  counts. Worlds whose source chunks are all densely counted bypass candidate
  scratch construction. At 32 or more halo targets, a grid-owned reusable pool
  uses up to eight workers. Complete-halo target addresses use another retained
  generation-stamped flat index, and their target/result vectors retain their
  high-water capacity instead of rebuilding a hash set, sorting, and allocating
  disposable buffers. Dense evaluation extracts 18-bit counted rows directly
  from the nine chunks' counting masks and reduces them through a rolling
  three-row stencil; it does not materialize an 18x18 byte halo or rescan cell
  states for counts. Empty results are not stored.
- Both paths write into a retained inactive chunk map. Old inactive nodes are
  extracted into a retained handle vector and reinserted with new keys/data,
  preserving transactional map comparison/swap while eliminating steady-state
  node allocation at a stable chunk-count high-water mark.
- The inactive map also remains the prior-generation baseline. Retained flat
  address sets track changed chunks and expand them by one chunk in every
  direction. Up to 4,096 changed addresses are retained. Sparse local sources
  build candidate masks only for frontier targets and choose candidate or halo
  evaluation independently. Exact local target/source bookkeeping, candidate,
  neighbor-contribution, and evaluation work is compared with a complete-path
  estimate derived from cached population totals. Cheaper frontiers patch the
  retained map; broad changes fall back to complete candidates or halos. Empty
  frontiers return immediately. Editing and ruleset-type changes repopulate or
  invalidate the frontier explicitly.
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
  `GameVisual`. Small capacity increases reserve 50% headroom so nearby zoom
  changes do not reallocate. Re-enrollment preserves the handle while the
  backend destroys its prior GL texture/PBO pair; view destruction explicitly
  releases the texture.
- Uses nearest filtering so discrete cell colors stay sharp; the editor cursor
  uses the same centered cell bounds.
- CPU palette targets fade through `displayRgb`; newly revealed cells snap to
  their current color. A retained active-texel set makes each fade tick and
  zero-speed snap visit only colors still changing; repeated unchanged
  `setFadeSpeed(0)` calls are constant-time. Stable grid/camera/palette state
  skips resampling and texture upload. At exact-cell zoom, a one-revision grid
  change publishes current or removed chunks and resamples only their visible
  16x16 tiles. Revision gaps, overview density, palette/camera changes, and
  whole-grid replacement fall back to a complete bounded resample. Dirty
  updates remain bounded to at most one update per drawable submission.
- Overview sampling visits only sparse chunks intersecting the visible source
  region. The visual texel budget does not limit stored chunks or world cells.
- `CellGameModule` dispatches the view on the World layer and the cursor/splash
  on UI.

## CellGameModule

EDIT / NORMAL; simulation uses `tps` x `speedFactor` and guarantees one due
generation. It starts an optional second synchronous generation only when the
first generation's measured cost projects both inside a 4 ms simulation slice,
then drops excess catch-up debt. Painting, Bresenham strokes, `setcell`,
randomization, and clearing operate directly on signed world coordinates.
Startup patterns are centered around `(0, 0)` and simulation is non-toroidal.

`status` reports output chunk nodes allocated, reused, and retained alongside
the simulation path, stored/counting cell counts, candidate-preferred chunk and
candidate/halo target counts, changed/frontier source and target counts,
frontier/complete work estimates, candidate work ranges, preparation/evaluation
workers, fading texels, last sampled/faded texel work, and frame-step debt.
It also separates requested and recently achieved TPS and reports the longest
step, total frame simulation time, and second-step budget deferral.

Save always writes version 2 sparse files containing the ruleset, camera, and
deterministically sorted chunks. Load validates temporary state first, reads
both version 2 and the prior dense format, imports legacy cells around the
world origin, and restores saved ruleset/camera metadata.

Wireworld retains the sticky head/empty/tail/conductor brush (`1`/`H`, `2`,
`3`/`T`, `4`).
