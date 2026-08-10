# App subsystem guidance

This file specializes the repository `AGENTS.md` for `Illumo/Source/App/`.

## Scope and boundaries

`App` is the product-composition root and owns the application main loop. It
selects the modules that ship, wires them to `Illumo`, advances the frame loop,
and translates process arguments into startup configuration. It does not own
engine services, cellular-automata state, rendering resources, or platform UI.

- Depend on Engine module/context contracts and public service interfaces.
- Keep OS entry points and native dialogs in `Platform/`.
- Keep rules, editing, persistence behavior, and drawables in `Game/`.
- Do not issue OpenGL calls or construct a concrete rendering backend here.

## Required invariants

- `CellMain` is the single product composition point. `Illumo` must remain
  product-agnostic and must not choose `CellGameModule` or `DebugModule`.
- Register production modules before the run loop. Compose `DebugModule` only
  in Debug builds, matching its conditional CMake compilation.
- Preserve startup and shutdown ordering: initialize the host, start accepted
  modules, run update/draw while the window remains active, then shut down the
  host exactly once.
- Pass the platform-provided argument vector to `SysCmdLine`; do not introduce
  platform-specific parsing in App.
- Keep the frame order visible and main-thread affine. Do not hide rendering,
  input polling, or module updates behind implicit background work.
- Treat startup failures as failures; do not continue into a partially
  initialized loop or report success after required composition fails.

## Ownership and compatibility

App owns the host object it creates and must make destruction and exceptional
paths explicit. Modules are transferred to Engine ownership through the
existing module API. Changes to executable arguments, startup defaults, module
selection, or shutdown behavior are user-visible contracts and require README,
architecture, and focused-test updates.

## Documentation and verification

- Current architecture: `docs/architecture-consensus.md` and
  `docs/packages/app.md`.
- Runtime loop: `docs/latex/sections/04-runtime-loop.tex`.
- Build both Release and Debug when composition or conditional modules change.
- Exercise startup, clean shutdown, and relevant command-line paths manually;
  headless tests do not prove the native window loop.

Update this file only for durable App boundaries or workflow changes.
