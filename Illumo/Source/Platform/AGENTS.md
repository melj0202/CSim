# Platform subsystem guidance

This file specializes the repository `AGENTS.md` for
`Illumo/Source/Platform/` and applies to all platform children.

## Scope and boundaries

Platform owns OS entry points and native save/load dialogs. Keep it thin: adapt
process arguments and native UI results to App and Services contracts. Shared
runtime behavior, simulation, command parsing, rendering policy, and resource
ownership belong outside Platform.

## Required invariants

- Each entry point forwards the complete argument vector to the shared App
  contract and maps its result to the platform process exit status.
- Native dialogs return an explicit success/cancel/failure result without
  mutating game state. File validation and persistence remain in Game/Services.
- Dialog buffers, encodings, extensions, filters, parent windows, overwrite
  behavior, and cancellation must be handled deliberately and tested on the
  target OS.
- Keep platform frameworks and headers out of cross-platform interfaces.
- Do not duplicate or fork the main loop, parser, renderer, or save format by
  platform.

## Support status

Windows is the only supported and currently verified platform. Linux and
macOS are stale scaffolds whose selected source paths do not currently satisfy
the shared App/Services contracts. Do not claim portability from source
presence or CMake branches. A port becomes supported only after native
configure, compile, automated tests, startup/rendering, dialogs, and clean
shutdown are verified and the documentation is updated.

## Documentation and verification

Use `docs/packages/platform.md`, the applicable child map, and the portability
chapter. Native changes require a target-OS build and manual dialog/window
smoke; static inspection is a limitation, not validation.

Update this file only for durable cross-platform boundaries or support policy.
