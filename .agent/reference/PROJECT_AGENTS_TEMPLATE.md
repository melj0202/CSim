# Repository Guidance

> Replace all `TODO` items before relying on this file. Keep this file focused on durable project-wide rules; place detailed architecture in `documents/` and subsystem-specific invariants in nested `AGENTS.md` files.

## Project identity

- Project: TODO
- Purpose: TODO
- Primary users or audience: TODO
- Supported environments and target platforms: TODO
- Explicit non-goals: TODO

## Sources of truth and documentation routing

Before changing code, inspect the relevant implementation, tests, build configuration, and documentation.

Canonical project documentation:

- Architecture overview: `documents/architecture/README.md`
- Design decisions: `documents/decisions/`
- Setup/build documentation: TODO
- Public API or integration documentation: TODO

When code, tests, instructions, and documentation disagree, do not silently choose one. Identify the conflict and determine which source is stale or which behavior is unintended.

## Build and verification commands

- Configure: TODO
- Build affected targets: TODO
- Run focused tests: TODO
- Run full tests: TODO
- Formatting: TODO or `not configured`
- Static analysis/lint/type checks: TODO or `not configured`
- Sanitizers: TODO or `not configured`
- Benchmarks: TODO or `not configured`

Use the narrowest verification that proves the change, then expand when risk warrants it.

## Project-wide invariants

- TODO: State only rules every relevant future change must preserve.
- TODO: Include cross-subsystem dependency direction.
- TODO: Include ownership, lifetime, threading, or data-contract rules that are genuinely global.
- TODO: Include project identity boundaries, such as keeping general-purpose engine code independent from one application.

## Architecture policy

Preserve the existing architecture unless the task explicitly authorizes architectural change. Agents may identify and explain architectural improvements, but must not implement them without authorization.

Architecture includes subsystem boundaries, dependency direction, ownership and lifetime models, threading and synchronization rules, persistence formats, and public contracts.

Local implementation refactors that preserve these contracts may proceed when they are necessary and bounded. Broad redesign belongs in a separate design/specification task.

## Compatibility and portability

- Public compatibility commitment: TODO
- Supported file, network, protocol, or data-format compatibility: TODO
- Platform priorities: TODO
- Deliberately unsupported platforms or environments: TODO

Preserve existing external behavior by default. Breaking changes require explicit authorization and a migration plan appropriate to their impact.

## Dependencies

- Dependency policy: TODO
- Approved package or vendor mechanisms: TODO
- Licensing or deployment constraints: TODO

External libraries are acceptable when justified. Prefer a narrow project-owned boundary when replacement, testing, or platform isolation materially benefits from it. Do not add wrappers that merely duplicate an API without establishing a useful contract.

## Code and documentation expectations

- Follow existing repository naming, layout, formatting, and idioms.
- Use concise inline comments for non-obvious intent, invariants, constraints, and reasons.
- Put full subsystem behavior, architecture, workflows, and trade-offs in `documents/`.
- Update relevant documentation when behavior, public contracts, architecture, setup, or operational procedures change.
- Do not duplicate long explanations between code comments, `AGENTS.md`, and documentation; cross-reference them.

## Generated files

- Generated paths: TODO
- Generators or source-of-truth inputs: TODO

Change generator inputs rather than generated output unless the repository explicitly requires checked-in generated files to be refreshed.

## Definition of done

A change is complete when:

- The requested behavior is implemented without unrelated scope expansion.
- Affected targets build.
- Relevant tests pass or justified tests are added.
- Configured checks relevant to the change pass.
- New serious diagnostics are resolved; unrelated pre-existing issues are reported.
- Documentation is synchronized where required.
- The final diff contains no accidental or unrelated changes.
- The final handoff states what was and was not verified.

## Planning

Follow `.agent/PLANS.md` for medium, subsystem-scale, high-risk, architectural, migration, or rewrite work.

## Nested guidance

Place subsystem-only invariants in an `AGENTS.md` inside the smallest directory that must obey them. Nested instructions should specialize these project-wide rules rather than casually weakening them.

## Maintaining this file

This file governs work on the repository; it is not a live architecture summary.

Update it only when the task changes a durable invariant, convention, boundary, build command, or required workflow that future agents must obey. Do not update it merely to describe a feature, class, directory layout, or transient state. Preserve user-authored policies unless the user explicitly supersedes them.
