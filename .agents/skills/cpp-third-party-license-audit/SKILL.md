---
name: cpp-third-party-license-audit
description: Audit third-party C++ libraries, headers, fonts, assets, and build dependencies for licensing and redistribution completeness. Use when reviewing vendored code, dependency changes, THIRD_PARTY_NOTICES.md, runtime license packaging, or release compliance. This workflow is engineering evidence, not legal advice; remediation, dependency changes, commits, and pushes require authorization.
---

# C++ Third-Party License Audit

Trace actual dependency use and package contents. Do not guess legal obligations from a library name alone.

## Build the dependency inventory

1. Read repository guidance, `README.md`, `THIRD_PARTY_NOTICES.md`, CMake, package or vendor manifests, asset directories, and packaging logic.
2. Trace the actual build graph and runtime copy steps rather than relying only on directory names.
3. Include libraries, header-only code, embedded source, fonts, images, shaders derived from third parties, command-line tools, optional features, and dynamically loaded components.
4. Record name, version or revision, source, purpose, linkage or embedding form, license identifier, and repository evidence.

Classify each item as active runtime, compiled or header source, build/test tool, optional or platform-specific, dormant vendored material, first-party, or uncertain.

## Determine evidence and obligations

Inspect bundled license files and authoritative upstream sources when version, provenance, or terms are ambiguous. Check required license-text retention, attribution, copyright notices, source or offer requirements, font or asset terms, and redistribution restrictions. Preserve license text verbatim. Keep dormant vendored-source notices distinct from licenses required in runtime packages.

This audit supplies engineering evidence, not legal advice. Flag ambiguity for counsel or the dependency owner instead of inventing a conclusion.

## Remediate only when authorized

- Add missing source license files verbatim.
- Update the central notice with accurate names, versions, sources, roles, and license references.
- Update CMake or packaging steps so required notices accompany runtime artifacts.
- Treat adding, replacing, or removing a dependency as a separate decision requiring licensing, maintenance, build, and deployment assessment.
- Do not commit or push without explicit authorization.

## Verify package completeness

Test from a fresh output directory when possible, including a renamed destination that exposes hard-coded paths. Build Release and run relevant tests. Enumerate packaged notices, compute hashes when exact identity matters, and reconcile active versus dormant dependencies. Run `git diff --check` and inspect the complete diff for altered legal text or accidental generated files.

## Report the audit

Provide an inventory, evidence-backed gaps, package impact, changes made, verification, and remaining legal uncertainty. Distinguish repository compliance from what a specific release artifact actually contains.
