# Engine

Runtime host and module system:

- `Illumo` / `IllumoContext` — composition root and frame loop services
- `IModule` — module interface
- `DebugModule` — debug overlay module
- `EntityTable` / `ModuleObject` — entity scaffolding (WIP)

Game-specific logic belongs in `Game/`, not here.
