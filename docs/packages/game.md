# Game

Cellular-automata **game domain**: canvas state, game module (modes), and editor-facing tools (brush/cursor).

## Canvas

- **Domain:** `lifeCanvas` — one byte per cell (`0` alive, `1` dead for binary CAs).
- **View:** a CPU palette maps cell state into `targetRgb`; `displayRgb` eases toward those targets and packs the result into `texCanvasBuffer`.
- **GPU:** token path enrolls a mesh, canvas shader, and **RGB display texture**; changed texels upload through dirty rectangles (PBO path in OpenGL).
- `rebuildPalette(RuleSet*)` refreshes visual targets when the active ruleset changes.

See `docs/latex/sections/07-game-and-rules.tex` and
`docs/latex/sections/05-rendering-current.tex`.

## CellGameModule

EDIT / NORMAL; sim rate from `tps` × `speedFactor`; dispatches Canvas + splash.
Save/load, simulation, canvas, ruleset, and camera commands are registered here
through `CommandRegistry` rather than modeled as frame states.

This package is not a generic “stdlib.” Prefer engine host code in `Engine/` and shared utilities in `Services/` or `Foundation/`.
