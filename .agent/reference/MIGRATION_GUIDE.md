# Migrating an Existing Repository `AGENTS.md`

The target structure is not a smaller file at any cost. It is a clearer separation of authority:

- Root `AGENTS.md`: durable rules that apply across the repository.
- Nested `AGENTS.md`: durable rules that apply only inside one subsystem or directory.
- Architecture documents: what the system currently does and how its pieces fit together.
- Decision records: why a consequential choice was made, alternatives considered, and consequences.
- `.agent/PLANS.md`: how large or risky work is planned and tracked.

## Audit each existing paragraph

Ask these questions in order:

1. Must every future change in this scope obey it?
   - Keep it in the closest applicable `AGENTS.md`.
2. Does it describe the current implementation, class layout, file tree, or feature state?
   - Move it into architecture documentation or leave it discoverable from code.
3. Does it explain why a consequential choice exists?
   - Move it into a decision record and keep only the resulting invariant in `AGENTS.md`.
4. Is it a repeated procedure with clear inputs and outputs?
   - Consider a skill later; do not keep a long workflow in every instruction context.
5. Is it stale, duplicated, vague, or unenforceable?
   - Correct or remove it.

## Replace broad self-modification permission

Avoid instructions such as "update AGENTS.md whenever the code changes." Replace them with:

> Update this file only when the task changes a durable invariant, convention, boundary, or required workflow that future agents must obey. Do not update it merely to describe implementation details or transient project state. Preserve user-authored policy unless explicitly superseded.

## Split by scope

A useful placement rule:

- Project identity, build entry points, global compatibility commitments, and cross-subsystem boundaries: root `AGENTS.md`.
- Renderer, networking, scene, platform, test, or tooling invariants: the relevant nested `AGENTS.md`.
- Class relationships, data flows, current pipelines, and diagrams: architecture documents.
- Rationale for handles, dependency choices, threading models, or compatibility bridges: decision records.

## Resolve conflicts deliberately

When code, tests, instructions, and documentation disagree, do not silently choose the most convenient source. Identify the conflict and determine whether the implementation is wrong, the documentation is stale, or an instruction has been superseded.

## Keep the root authoritative

Nested guidance should specialize the root rather than casually weaken project-wide invariants. Use an override only when the exception is intentional and documented.
