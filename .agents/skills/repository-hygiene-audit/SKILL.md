---
name: repository-hygiene-audit
description: Audit this repository for tracked generated output, private data, credentials, machine-local paths, logs, caches, archives, and accidental artifacts. Use for repository cleanup, pre-publication hygiene, or requests to find sloppy leftovers or private material. Audit read-only first; untracking, deletion, staging, committing, and pushing require explicit authorization.
---

# Repository Hygiene Audit

Find material repository risks without treating pattern matches as proof or deleting user data.

## Inventory safely

1. Inspect `git status --short`, `.gitignore`, and `git ls-files` before scanning.
2. Audit tracked state first. Include ignored, generated, vendored, archived, or historical directories only when needed to determine ownership or intent.
3. Do not stage, untrack, delete, normalize, clean, or rewrite files during an audit.
4. Never print or quote suspected credential, token, key, or connection-string values. Prefer filename-only or redacting scanners; report paths, line numbers, secret types, and non-sensitive identifiers only.

## Scan bounded risk categories

Look for:

- build output, binaries, coverage reports, generated documentation, and LaTeX auxiliaries;
- logs, crash dumps, Tracy captures, caches, temporary files, and IDE state;
- source dumps, archives, screenshots, copied artifacts, and prior-agent records;
- environment files, credentials, access tokens, private keys, connection strings, and account identifiers;
- absolute machine-local paths, usernames, home directories, and workspace-specific configuration;
- unexpectedly large files, duplicate dependencies, and copied runtime assets;
- tracked files contradicted by ignore policy.

A filename or regular-expression match is only a candidate. Inspect context, tracking state, provenance, and whether the material is a required source, test fixture, license, runtime asset, or historical record before classifying it. If contextual inspection could disclose a secret, use a redacted tool or stop and report the limitation.

## Classify findings

For each finding, state:

- tracked, untracked, ignored, staged, or historical state;
- intended role and evidence;
- privacy, security, size, reproducibility, or maintenance risk;
- recommended action and whether a local copy must be preserved.

Do not remove canonical source inputs, necessary runtime assets, third-party license texts, vendored sources, formal decisions, or provenance merely because they resemble generated or historical material.

## Remediate only with authorization

When cleanup is explicitly requested, enumerate exact targets first. Update `.gitignore` for durable local-only categories. Use `git rm --cached` only when untracking is authorized and preserve required local files. Treat credential rotation, history rewriting, dependency removal, commits, and pushes as separate operations requiring explicit intent.

## Verify and report

Use `git check-ignore`, `git status`, staged name-status checks, targeted rescans, and a complete diff review. Confirm essential sources and assets remain. Explain every material deletion or untracking action and whether it is recoverable. Separate audit findings from changes actually made, and state whether anything was staged, committed, or pushed.
