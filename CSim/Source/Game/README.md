# Game

Cellular-automata **game domain**: canvas state, game module (modes), and editor-facing tools (brush/cursor).

## Canvas

- **Domain:** `lifeCanvas` — one byte per cell (`0` alive, `1` dead for binary CAs).
- **View:** token path enrolls mesh + **R8** cell texture + **256×1 RGB palette**; dirty-rect uploads.
- **No** dual float RGB fade buffers; `rebuildPalette(RuleSet*)` on ruleset change.

See `docs/sections/07-game-and-rules.tex` and `05-rendering-current.tex`.

## CellGameModule

EDIT / NORMAL (and save/load hooks); sim rate from `tps` × `speedFactor`; dispatches Canvas + splash.

This package is not a generic “stdlib.” Prefer engine host code in `Engine/` and shared utilities in `Services/` or `Foundation/`.
