# OpenGL backend guidance

This file specializes `Illumo/Source/Rendering/AGENTS.md` for
`Illumo/Source/Rendering/OpenGL/`.

## Scope and boundaries

This directory is the sole implementation boundary for command execution with
OpenGL: backend creation, command decoding, resource registries, meshes,
textures, shaders, device state, and draw submission. Do not leak GL headers,
object names, or state assumptions into neutral Rendering, Game, or Services.

## Required invariants

- Execute the token stream in order on the thread owning the active context.
  Validate command type, handle, payload size, and resource existence before
  calling GL.
- `GLDevice` registries own concrete GPU resources. Destroy each object once,
  while a valid context exists; make partial initialization and shader failure
  safe.
- Resource enrollment returns backend-neutral handles. A failed compile,
  allocation, lookup, map, or upload must not masquerade as a valid resource.
- Preserve mesh/index capacities. Reject update ranges larger than enrolled
  storage before issuing buffer writes.
- Texture uploads must preserve format, dimensions, filtering, and pixel-store
  state. Any PBO path must account for GPU/CPU synchronization and have a
  correct direct-upload fallback.
- Uniform and sampler locations must match the linked program. Treat missing
  shader files, compile errors, and link errors as initialization failures.
- Keep all raw draw, blend, viewport, clear, and resource calls here; window
  context creation remains in the shared window boundary.

## Verification

Use MockBackend tests to protect the neutral contract, then run Release and
Debug live-window smoke for startup, all production drawables, camera/edit
interaction, resizing/fullscreen when affected, and clean shutdown. Use GL
debug output or a capture tool for state/resource defects; a headless test
cannot validate them. Visually inspect shader changes and keep
`Illumo/Shader/` sources synchronized with their command payload contracts.

Relevant documentation is `docs/packages/source-layout.md`, the rendering
chapter, and the portability chapter. Update this file only for durable OpenGL
backend rules.
