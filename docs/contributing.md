# Contributing to Illumo

## Code style

1. Avoid `auto`; use explicit types.
2. Avoid namespaces.
3. Do not add recursive code.
4. Match the surrounding C++23 style and keep ownership explicit.

## Dependencies

Do not add a third-party dependency without author approval. Propose dependency
changes separately so their maintenance, license, and build impact can be
reviewed.

## Architecture boundaries

- App owns product composition; `Illumo` owns services and module lifetime.
- Game and rules code do not issue raw OpenGL calls.
- Production rendering uses `RenderCommand` tokens through `IBackend`.
- Keep `DebugModule` out of Release compilation and composition.
- Do not introduce an ECS, render graph, generalized scene graph, infinite
  chunks, or a compute backend solely for architectural completeness.

## Documentation

All first-party documentation belongs under `docs/`. Update
`architecture-consensus.md` and the relevant LaTeX chapter when behavior changes.
Append a formal decision in `latex/sections/09-design-decision-log.tex` when a
closed architecture or product-policy decision changes.
