# macOS platform guidance

This file specializes `Illumo/Source/Platform/AGENTS.md` for macOS.

## Current status

macOS is unsupported bootstrap scaffolding, not a verified port. Its selected
entry source is stale relative to shared App/Services APIs, and Objective-C++
language enablement must be established before the selected `.mm` source can
be treated as buildable. `CocoaRenderWindow` is inactive legacy scaffolding,
not the production window path.

## Port constraints

- Reuse the shared `CellMain(argc, argv)`, GLFW window, OpenGL backend, parser,
  and save format. Do not fork the main loop or renderer.
- Keep Cocoa and Objective-C++ details in this directory and enable OBJCXX
  explicitly in CMake when compiling `.mm` files.
- Apply any required macOS OpenGL context/profile hints through the shared
  window abstraction without changing behavior on other platforms.
- Preserve exact cancel/success/failure semantics and verify new save,
  overwrite, load, long/Unicode path, and cancellation with native panels.
- Do not claim support without a native AppleClang configure, full build/tests,
  launch/render interaction, dialogs, and clean shutdown.

Use `docs/packages/platform-macos.md` and the portability chapter. Update both
when the port contract or verified status changes. Update this file only for
durable macOS rules.
