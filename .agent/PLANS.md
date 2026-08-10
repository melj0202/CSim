# Planning and Execution Protocol

Planning effort must scale with blast radius. A plan is useful only when it exposes decisions, risks, sequencing, or verification that would otherwise be missed.

## Tier 0: local or trivial work

Examples: one function, a narrow bug fix, a small test, a mechanical edit with known behavior.

- Inspect the relevant code, tests, and instructions.
- Implement directly.
- Verify the affected behavior.
- Do not create a plan document.

## Tier 1: medium or multi-file work

Examples: a contained feature, a non-architectural refactor across several files, a dependency integration behind an existing boundary.

Before editing, state a concise plan containing:

- Objective and success condition.
- Files or components likely to change.
- Compatibility and behavioral constraints.
- Verification steps.
- Material uncertainty or risks.

Keep the plan short. Update it only when evidence changes the approach.

## Tier 2: subsystem-scale or high-risk work

Examples: ownership or lifetime changes within a subsystem, a new backend, a significant API extension, complex performance work, or a migration with several checkpoints.

Create an execution-plan document before implementation. Use a clear task-specific filename under an appropriate planning or documentation directory.

The plan must include:

1. Objective and measurable end state.
2. Current-state evidence and relevant documentation.
3. Scope and explicit non-goals.
4. Constraints and invariants that must remain true.
5. Proposed design and alternatives considered.
6. Affected public contracts, compatibility, and migration needs.
7. Ownership, lifetime, threading, error, and platform implications where relevant.
8. Ordered implementation milestones.
9. Build, test, static-analysis, sanitizer, and benchmark strategy as applicable.
10. Rollback or containment strategy for risky milestones.
11. Open questions and decisions requiring user input.
12. Validation results recorded as milestones complete.

Do not start implementation until the plan is coherent enough to review. Keep it synchronized with material decisions and evidence during execution.

## Tier 3: architecture, rewrite, or major migration

Examples: changing subsystem boundaries, replacing ownership or threading models, a broad compatibility break, a full rewrite, or a platform migration.

Do not treat this as an ordinary plan-mode task. Produce a detailed design/specification document first. Discuss it and obtain explicit authorization before implementation.

The design must establish:

- The problem and why the current architecture is insufficient.
- Intended end state and non-goals.
- Alternatives and why they were rejected.
- Compatibility bridges, incremental milestones, and rollback strategy.
- Data, ownership, lifetime, concurrency, platform, deployment, and performance consequences.
- Verification needed to demonstrate parity or improvement.
- Documentation and instruction changes that would follow.

Architecture proposals remain proposals until authorized.

## Discovery and delegation

Read-heavy discovery may be delegated to read-only subagents. Use them to map code paths, summarize documentation, interpret tests or logs, and review risk. The lead agent must reconcile their reports against repository evidence.

Keep design authority, implementation, and integration with the lead agent. Do not use parallel writers for overlapping code unless the user explicitly authorizes that workflow.

## Scope discipline

Do not change the goalposts during implementation. New desirable work should become a follow-up unless it is a prerequisite for correctness or the user explicitly expands scope.

If the plan reveals that the requested approach is weak or unsafe, surface the issue before investing in implementation.

## Completion

A plan is complete only when it records:

- What was implemented.
- Deviations from the original design and why.
- Verification performed and results.
- Remaining risks, unresolved questions, and follow-up work.
