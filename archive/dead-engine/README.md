# Archived engine scaffolding

Moved out of the live build (2026-07-28, decision **D-E3**).

| File | What it was |
|------|-------------|
| `EntityTable.h` | Sparse handle table of `ModuleObject`s (create/remove/dirty flags) |
| `ModuleObject.h` | Mesh/texture handle + Transform bag for a hypothetical entity path |

**Why archived:** Nothing in the cell-sim frame path created or used an `EntityTable`. `Scene` / `IllumoContext` only held null pointers. The live presentation path is **drawables + tokens**, not an ECS.

**Do not re-add** unless a real feature needs a general object table; prefer extending the token drawable list first.

`ObjectID` for the lightweight scene graph now lives in `Rendering/SceneObject.h` as `uint32_t`.
