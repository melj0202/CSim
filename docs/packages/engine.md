# Engine

Runtime host and module system:

- `Illumo` — owns long-lived services and drives the frame loop
- `IllumoContext` — frozen, non-owning service bag for the two shipped modules
- `IModule` — module interface (`Start` / `Update` / `DispatchDrawables` /
  `Exit`); failed `Start` removes the module from the host list
- `DebugModule` — debug overlay module, compiled and registered only in Debug

`Illumo::Init` constructs the production `GLBackend` and injects it into
`Renderer` as `std::unique_ptr<IBackend>` (D-R11). `Illumo::Render` has a single
production path: clear frame list → module drawables → token submit (D-R13);
there is no env-gated alternate product frame path. The host never hard-codes
game modules; product composition belongs to `App/CellMain.cpp`.

`EntityTable`, `ModuleObject`, and the unused scene-graph scaffolding are
archived under the repository `archive/` tree; they are not part of the live
engine path.

Game-specific logic belongs in `Game/`, not here.
