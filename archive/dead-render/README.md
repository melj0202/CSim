# Archived dead render paths

Moved during token-renderer Phase 5 cleanup (2026-07-28).

| File | Why archived |
|------|----------------|
| `RenderQueue.*` | Pre-token drawable list on the window; Scene + Renderer won. |
| `Drawable.cpp` | Immediate GL compile helpers on DrawableBase; resources now enroll via IBackend. |
| `RenderContext.h` | Unused static GL handle bag. |
| `Transform.h` | Empty stub. |
| `CellCommandLine.*` | Older static console; replaced by `Services/CommandLine`. |

Do not re-add without an explicit decision log entry.
