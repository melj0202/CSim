# Engine

Runtime host and module system:

- `Illumo` — owns long-lived services and drives the frame loop
- `IllumoContext` — frozen, non-owning service bag for the two shipped modules
- `IModule` — module interface
- `DebugModule` — debug overlay module

Product composition belongs to `App/CellMain.cpp`. `EntityTable`,
`ModuleObject`, and the unused scene-graph scaffolding are archived under the
repository `archive/` tree; they are not part of the live engine path.

Game-specific logic belongs in `Game/`, not here.
