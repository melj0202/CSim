# Mock backend guidance

This file specializes `Illumo/Source/Rendering/AGENTS.md` for
`Illumo/Source/Rendering/Mock/`.

## Scope and boundaries

MockBackend is the deterministic headless implementation of `IBackend`. It
records semantic resource and command behavior so tests can prove the neutral
rendering contract without a window, GPU, OpenGL loader, or driver.

## Required invariants

- Include no OpenGL headers and perform no native graphics calls.
- Mirror every relevant `IBackend` token and handle contract closely enough to
  detect ordering, resource, payload, and state regressions. Reject invalid
  handles or unsupported commands explicitly rather than silently succeeding.
- Record semantic values, not concrete GL implementation details. When a
  command borrows a pointer, copy the bytes needed by later assertions during
  submission; never retain a dangling test pointer.
- Keep handle allocation deterministic and reset all mutable state between
  tests. Tests must not depend on prior enrollment or command history.
- Do not make the production interface test-specific. Add narrowly useful
  inspection to the mock implementation or test fixtures.
- A MockBackend pass proves command production and backend-neutral semantics,
  not shader compilation, pixels, driver behavior, dialogs, or native window
  lifecycle.

## Documentation and verification

Use `TestMockBackend.cpp`, `TestRendererE2E.cpp`, UI token tests, and
`docs/packages/source-layout.md`. Add exact tests whenever a command kind,
handle rule, queue error, or payload contract changes, and pair them with a
live OpenGL smoke when the real backend is affected.

Update this file only for durable MockBackend contracts.
