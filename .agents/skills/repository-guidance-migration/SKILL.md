---
name: repository-guidance-migration
description: Migrate or audit this repository's root and nested AGENTS.md guidance from supplied policy, templates, or migration instructions. Use when asked to restructure agent guidance, propagate durable rules, reconcile guidance with the live repository, or produce a guidance-compliance report. Do not trigger for ordinary code or documentation edits.
---

# Repository Guidance Migration

Treat supplied policy, templates, and migration instructions as authoritative within the user's stated scope.

## Build an evidence map

1. Read every supplied template or migration guide completely.
2. Inspect the existing root and nested `AGENTS.md` hierarchy, `.agent/PLANS.md`, canonical documentation, CMake, tests, and relevant live implementation.
3. Run `git status --short` first and preserve unrelated user work.
4. Map each supplied rule to its destination, supporting repository evidence, and any conflict with current guidance or code.

## Maintain the hierarchy

- Put durable project-wide operating rules in root `AGENTS.md`.
- Put subsystem-specific specialization in the closest nested `AGENTS.md`.
- Never let child guidance silently weaken project-wide safety or architectural boundaries.
- Keep current architecture, class catalogs, historical decisions, and long rationale in canonical documentation rather than duplicating them as instructions.
- Exclude transient task state, issue lists, dated session detail, generated output, and unsupported claims.
- Preserve user-authored policy unless the user explicitly supersedes it.
- Keep repository skills under `.agents/skills`; never install or copy them into personal skill directories without explicit authorization.

## Perform the migration

1. Verify referenced paths, targets, commands, platform claims, and subsystem ownership against the live tree.
2. Remove stale or duplicated instructions only when the supplied policy authorizes replacement.
3. Cross-reference canonical documentation for descriptive detail.
4. If guidance exposes a code or documentation mismatch, synchronize canonical sources only when those writes are also authorized; otherwise report it.
5. Edit source-of-truth files, never generated PDFs, build output, or copied artifacts.

## Validate completeness

- Account for every supplied policy item as retained, specialized, relocated, superseded, or intentionally omitted with a reason.
- Check expected guidance files, stale paths, placeholders, contradictory platform statements, duplicate rules, and broken routing references.
- Confirm a future agent can find the right instruction from the repository root and subsystem path.
- Run proportional documentation or repository checks, `git diff --check`, and a complete diff review.
- Do not infer compliance from a file's existence; verify its content and interaction with parent guidance.

## Report the result

Summarize changed guidance, preserved policy, reconciled conflicts, validation, and unresolved mismatches. Do not commit, push, or modify personal configuration unless explicitly requested.
