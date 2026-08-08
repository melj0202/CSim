# Game

The live game path is an unbounded sparse cellular-automata world plus a
bounded presentation view.

## SparseCellGrid (simulation domain)

- Authoritative signed 64-bit cell coordinates.
- Hash-map storage of non-background 16x16 chunks; there is no fixed chunk
  count or allocator-pool cap.
- Negative coordinates use centralized floor division/modulo. All byte states
  are preserved, including Brian's Brain and Wireworld values.
- Each serial generation evaluates active chunks and their neighbors through a
  temporary read-only 18x18 halo, then installs deterministic sorted output.
- Rulesets supply pure `nextState` and `evalCell` behavior. The old dense
  `CellGrid`/`Canvas` implementation remains only for compatibility coverage.

## CanvasView (presentation)

- Samples the camera-visible world into the configured `CanvasX` x `CanvasY`
  bounded RGB staging buffer.
- Owns one reusable RGB texture and one screen-space quad through `GameVisual`.
- Requests linear sampling for the display texture so zoomed-out views do not
  become nearest-neighbor blocks.
- CPU palette targets fade through `displayRgb`; newly revealed cells snap to
  their current color. Texture updates are dirty and bounded to at most one
  update per drawable submission.
- `CellGameModule` dispatches the view on the World layer and the cursor/splash
  on UI.

## CellGameModule

EDIT / NORMAL; simulation uses `tps` x `speedFactor`. Painting, Bresenham
strokes, `setcell`, randomization, and clearing operate directly on signed
world coordinates. Startup patterns are centered around `(0, 0)` and
simulation is non-toroidal.

Save always writes version 2 sparse files containing the ruleset, camera, and
deterministically sorted chunks. Load validates temporary state first, reads
both version 2 and the prior dense format, imports legacy cells around the
world origin, and restores saved ruleset/camera metadata.

Wireworld retains the sticky head/empty/tail/conductor brush (`1`/`H`, `2`,
`3`/`T`, `4`).
