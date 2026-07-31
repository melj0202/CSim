# Archived engine scaffolding

Moved out of the live build (2026-07-28).

| File | Decision | What it was |
|------|----------|-------------|
| `EntityTable.h` | D-E3 | Sparse handle table of `ModuleObject`s |
| `ModuleObject.h` | D-E3 | Mesh/texture handle + Transform bag |
| `SceneObject.h` | D-E4 | Parent/child node graph for a general scene tree |
| `RenderableObject.h` | D-E4 | Unused mesh/texture transform bag on Scene |

**Why archived:** The live path is **Scene = drawable list + camera + window**, then tokens. Entity tables and scene graphs were never wired into the CA frame.

**Do not re-add** unless a real feature needs them; prefer extending the token drawable list first.

