# Illumo TestSupport guidance

This file specializes the root and Rendering guidance for
`Illumo/TestSupport/`.

## Scope and boundaries

`Illumo::TestSupport` is a separate, test-only CMake interface. Its
`MockBackend` is the deterministic headless implementation of `IBackend` used
to prove the public renderer contract without a window, GPU, loader, or driver.
These headers are not part of the `Illumo::Illumo` production API.

## Required invariants

- Include no OpenGL headers and perform no native graphics calls.
- Mirror relevant `IBackend` token and handle behavior closely enough to catch
  ordering, resource, payload, and state regressions.
- Record semantic values, not GL implementation details. Copy borrowed payload
  bytes needed by later assertions during synchronous submission.
- Keep handle allocation deterministic and reset mutable state between tests.
- Do not make production interfaces test-specific; put narrow inspection in
  TestSupport or target-owned fixtures.
- Never grant a production consumer this include root.

A MockBackend pass does not prove shader compilation, pixels, driver behavior,
dialogs, or native window lifecycle. Pair OpenGL changes with proportional live
Windows smoke validation.
