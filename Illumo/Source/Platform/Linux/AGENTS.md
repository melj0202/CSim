# Linux platform guidance

This file specializes `Illumo/Source/Platform/AGENTS.md` for Linux.

## Current status

Linux is unsupported bootstrap scaffolding, not a verified port. The selected
entry source is stale relative to the current App and Services APIs. Do not
describe it as compiling or runnable until verified on Linux.

## Port constraints

- Converge on the same `CellMain(argc, argv)` and `SysCmdLine` contracts as the
  supported path; do not revive removed service-locator APIs or fork App.
- Keep GTK use limited to native dialogs and bootstrap adaptation. Shared
  GLFW/OpenGL rendering and the common save format remain cross-platform.
- Configure GTK include, compile, and link flags through CMake target
  properties. Do not leak GTK types into Services or Game.
- Preserve exact cancel/success/failure semantics and verify new save,
  overwrite, load, long/Unicode path, and cancellation on Linux.
- Treat source-only conclusions as static findings. A support claim requires a
  native Clang/GCC configure, full build/tests, launch/render interaction,
  dialogs, and clean shutdown.

Use `docs/packages/platform-linux.md` and the portability chapter. Update both
when the port contract or verified status changes. Update this file only for
durable Linux rules.
