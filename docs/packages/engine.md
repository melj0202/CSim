# Illumo Engine

The reusable application runner, host, and source-level module contract live in
Illumo:

- `IllumoConfig` carries application name, configuration path, and fallible
  test/factory injection hooks.
- `Illumo` owns long-lived generic services and drives update/render/shutdown.
- `IllumoContext` is a frozen, non-owning service bag with no game assumptions.
- `IModule` retains `Start` / `Update` / `DispatchDrawables` / `Exit`.
- `DebugModule` is an optional generic renderer/tooling module in Debug builds.
- `IllumoApplicationDefinition` accepts declarative consumer policy while
  `RunIllumoApplication` owns logging, CLI, module registration, timing, and
  process results.

Construction loads generic defaults; `initialize()` creates the window/context,
constructs and initializes the backend exactly once, and transfers
`std::unique_ptr<IBackend>` to `Renderer`. Failures are logged and returned; the
library does not terminate the process.

Module registrations are required or optional. Optional rejection destroys the
module immediately. Required failure rolls back all accepted modules in reverse
order, with exceptions contained so every cleanup is attempted. Illumo invokes
the consumer-supplied required-module factory through the application
definition; it never includes or constructs a concrete Game type directly.
