# Game

Cellular-automata **game domain**: cell grid, canvas presentation, game module
(modes), and editor-facing tools (brush/cursor).

## CellGrid (domain)

- Pure dense storage: one byte per cell on the **front** buffer, plus a same-sized **back** buffer for generation write (D-P5).
- Dirty-region tracking for visual rebuild / upload.
- No `Renderer`, window, camera, or render tokens.
- Rulesets take `CellGrid*`, write the next generation into the back buffer, track the dirty AABB in one pass, then `swapLifeBuffers()` when anything changed.

## Canvas (presentation over domain)

- **Extends `CellGrid`:** inherits domain storage.
- **View:** a CPU palette maps cell state into `targetRgb`; `displayRgb` eases toward those targets and packs the result into `texCanvasBuffer` (fade model kept; D-P6 tightens the hot loops).
- **GPU:** token path enrolls a mesh, canvas shader, and **RGB display texture**; changed texels upload through dirty rectangles (PBO path packs partial rects tightly — D-P6). Optional: null renderer skips GPU enroll (domain-only construction).
- `rebuildPalette(RuleSet*)` refreshes visual targets when the active ruleset changes.

## Simulation performance notes

- Default canvas size remains small (80×60); full-grid eval is still O(W×H).
- Auto multi-thread row eval engages at ≥512×512 (`RuleSet` worker override for tests).
- Headless benches: `Illumo.Sim.MicroBench`, plus correctness cases under `Illumo.Sim.*`.

See `docs/latex/sections/07-game-and-rules.tex` and
`docs/latex/sections/05-rendering-current.tex`.

## CellGameModule

EDIT / NORMAL; sim rate from `tps` × `speedFactor`; dispatches Canvas + module-owned
mode splash (`std::unique_ptr<SplashText>` — not a file-scope global).
Save/load, simulation, canvas, ruleset, and camera commands are registered here
through `CommandRegistry` rather than modeled as frame states.

Startup seeds are ruleset-aware: binary life-like rules get a centered glider;
Wireworld gets a horizontal conductor with a head+tail electron. In Wireworld
edit mode the left-paint brush is sticky (`1`/`H` head, `2` empty, `3`/`T`
tail, `4` conductor); right-click paints empty.

This package is not a generic “stdlib.” Prefer engine host code in `Engine/` and shared utilities in `Services/` or `Foundation/`.
