---
name: illumo-platform-readiness-audit
description: Assess or plan Illumo portability for macOS, Linux, or another native platform without claiming unsupported validation. Use for port estimates, platform-readiness reviews, bootstrap and CMake audits, packaging plans, or native smoke-test matrices. Default to read-only; implement a port only when explicitly authorized.
---

# Illumo Platform Readiness Audit

Separate source inspection from native evidence and never turn stale scaffolding into a support claim.

## Establish the evidence level

1. Read root and platform-specific `AGENTS.md`, `README.md`, architecture guidance, CMake, App composition, target bootstrap, persistence, rendering, asset, and packaging paths.
2. Inspect `git status --short` and preserve the worktree for assessment-only requests.
3. Record host, target OS and version, compiler, SDK, architecture, graphics stack, and available native tools.
4. Label each conclusion as source-inspected, cross-compiled, natively configured, natively built, headlessly tested, launched, manually smoked, or packaged.

Windows is the only currently supported and verified production path. Treat Linux and macOS code as stale bootstrap scaffolding until native configure, compile, launch, and smoke evidence proves otherwise.

## Audit the whole native path

Inspect:

- platform entry point and App API handoff;
- CMake language, compiler, framework, library, architecture, and deployment settings;
- GLFW/OpenGL context creation, GLSL compatibility, framebuffer sizing, and high-DPI behavior;
- input mapping, Unicode/text input, mouse coordinates, fullscreen transitions, focus, and shutdown;
- native dialogs, save/load paths, writable state, environment, logging, and working-directory assumptions;
- shaders, fonts, textures, copied assets, runtime notices, and package layout;
- Debug and Release behavior, Tracy or profiling hooks, tests, sanitizers, and packaging;
- signing, notarization, bundle metadata, or distribution requirements for the target.

## Produce a narrow port plan

Preserve the existing `IBackend` and OpenGL path unless evidence requires a different product decision. Isolate target-specific code behind existing platform boundaries. Avoid speculative backend or engine generalization. Identify exact files, risks, dependencies, writable-data locations, and native validation steps for each phase.

## Require a native validation matrix

At minimum, cover native Debug and Release configure/build, labeled tests, terminal and desktop launch, canvas and fade, text and console UI, keyboard/mouse/Unicode input, camera and high-DPI behavior, fullscreen transitions, textures and shaders, representative rulesets, persistence and dialogs, clean shutdown, packaged assets and license notices, writable state and logs, and applicable signing. Add sanitizers where the native toolchain supports them.

## Report readiness honestly

Separate observed source readiness from completed native validation. List blockers, assumptions, estimate ranges, staged implementation phases, and distribution gaps. Do not describe a target as supported based on Windows builds, headless tests, cross-compilation, or source plausibility alone.
