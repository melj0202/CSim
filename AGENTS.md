# Illumo repository guidance

This file is the durable project-wide operating contract for work in Illumo.
Read the closest nested `AGENTS.md` before changing a subsystem. Use the live
tree and canonical documentation for implementation detail; do not treat this
file as an architecture catalog.

## Project identity and boundaries

Illumo is a C++23 cellular-automata learning sandbox with an engine-shaped
modular-monolith shell. The supported production path is Windows, GLFW,
OpenGL, and the sparse infinite canvas. It is not a general-purpose game
engine.

Do not introduce an ECS, render graph, generalized scene graph, additional
graphics backend, compute backend, or broad framework merely for architectural
completeness. Preserve public behavior and formats by default. A change to
subsystem boundaries, dependency direction, ownership, lifetime, threading,
persistence, or public contracts requires explicit authorization and a design
appropriate to its blast radius.

## Sources of truth

Inspect the applicable implementation, tests, CMake, and documentation before
editing. Route detail to these canonical sources:

- repository use and exact common commands: `README.md`;
- current architecture and decision catalog: `docs/architecture-consensus.md`;
- long-form design book and chart-only map: `docs/latex/illumo.tex` and
  `docs/latex/architecture-map.tex`;
- formal decisions: `docs/latex/sections/09-design-decision-log.tex`;
- package ownership maps: `docs/packages/`;
- coding and dependency policy: `docs/contributing.md`;
- large-work planning: `.agent/PLANS.md`;
- migration scaffolds, retained verbatim for reuse:
  `.agent/reference/PROJECT_AGENTS_TEMPLATE.md` and
  `.agent/reference/MIGRATION_GUIDE.md`.

Before substantive work, read `docs/output/illumo.pdf` when its content is not
already available. When a task authorizes documentation writes and the PDF is
absent or older than its LaTeX sources, rebuild it with `docs/build.ps1` first.
A read-only task does not authorize regenerating it.

When code, tests, CMake, instructions, and documentation disagree, identify
the conflict. The live implementation is current-state evidence, but do not
silently bless unintended behavior; correct stale documentation in the same
authorized change or report the mismatch.

## First steps and scope discipline

1. Run `git status --short` and preserve every existing user change.
2. Read `README.md`, `docs/architecture-consensus.md`, this file, and the
   closest nested guidance.
3. Search narrowly and inspect call sites, tests, configuration, ownership,
   and error paths relevant to the request.
4. Exclude generated, vendored, and historical trees unless the task concerns
   them: `build*/`, `Illumo/thirdparty/`, `archive/`, LaTeX auxiliaries,
   source dumps, ZIPs, and prior-agent records.
5. Keep the requested boundary. Review and diagnosis do not authorize fixes;
   preserve production code when the task is documentation-only.

Use `.agent/PLANS.md` for medium, subsystem-scale, high-risk, migration, or
architectural work. Keep design authority, writes, and integration with the
lead agent; read-only subagents may supply bounded evidence.

## Build and verification commands

Run from the repository root on Windows.

```powershell
cmake -S Illumo -B build
cmake --build build --config Release
ctest --test-dir build -C Release -L Illumo --output-on-failure
```

Focused test work:

```powershell
cmake --build build --config Release --target IllumoTests
ctest --test-dir build -C Release -N -L Illumo
build/Release/IllumoTests.exe --list
build/Release/IllumoTests.exe --run <exact-test-name>
```

Clang/LLVM coverage:

```powershell
cmake -S Illumo -B build-coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DILLUMO_BUILD_DOCUMENTATION=OFF -DILLUMO_ENABLE_COVERAGE=ON
cmake --build build-coverage --target IllumoCoverage
```

Documentation and formatting:

```powershell
./docs/build.ps1
clang-format -i <modified-cpp-or-header-files>
```

The default build compiles `IllumoTests`, runs every granular `Illumo.*` CTest
case through `IllumoRunTests`, and builds `IllumoDocs` when PowerShell and
`latexmk` are available. Headless tests do not prove the live OpenGL window,
native dialogs, or non-Windows ports. Use Debug or Release GUI smoke tests and
sanitizers when the affected risk requires them. No repository-wide
`clang-tidy` target is configured; use the compile database and report the
exact checks and translation units when static analysis is requested.

## Project-wide invariants

- `App` owns product composition. `Engine/Illumo` owns long-lived services and
  module lifetime; it must not construct product modules.
- `IllumoContext` is a non-owning pointer bag frozen after engine startup.
  Modules whose `Start` returns `false` are removed before update or draw.
- Runtime window, input, module, rendering, and OpenGL work is main-thread
  affine unless a documented subsystem contract explicitly provides workers.
- Game and Rulesets do not issue raw OpenGL calls or depend on OpenGL types.
- Production drawables append `RenderCommand` tokens to the backend-neutral
  `Renderer`; `IBackend` executes them. Any pointer carried by a command must
  remain valid until synchronous queue submission returns.
- `Scene` is a non-owning ordered list rebuilt each frame, not a scene graph or
  ECS. Rendering resource handles remain backend-neutral and registry-owned.
- `SparseCellGrid` is the production domain: signed 64-bit coordinates,
  non-background 16x16 sparse chunks, and non-toroidal evolution.
  `CanvasView` is a bounded world-space presentation. Legacy `CellGrid` and
  `Canvas` remain compatibility fixtures, not a second production path.
- Save writes preserve sparse format version 2 and loads remain compatible
  with both version 2 and the legacy dense format unless migration is
  explicitly authorized.
- Ruleset transitions and palettes must remain deterministic. Update factory
  selection, known-mode validation, console help/completion, source lists, and
  focused tests together when ruleset availability changes.
- Windows is the only supported and currently verified platform. Linux and
  macOS are stale bootstrap scaffolds and must not be described as supported
  until they configure, compile, launch, and pass platform smoke tests.
- Keep generic engine/services independent of game-domain policy. Keep raw
  platform and OpenGL details behind their existing boundaries.
- Do not add or replace a third-party dependency without user approval and a
  licensing, maintenance, build, and deployment assessment.

## Code, documentation, and generated material

Follow `docs/contributing.md`: avoid `auto`, namespaces, and recursion; use the
documented names; run the Mozilla-based `clang-format` on every modified C++ or
header file. Prefer explicit ownership and narrow dependencies. Owning types
must define or delete copy/move operations deliberately.

Use comments for non-obvious intent and invariants. Put current architecture,
workflows, rationale, and diagrams under `docs/`; cross-reference rather than
duplicating long narratives in guidance. Architecture changes update
`docs/architecture-consensus.md` and the matching LaTeX chapter. A closed
decision also receives a new formal decision-log entry.

Edit source inputs, not generated outputs: do not hand-edit CMake/build output,
LaTeX auxiliaries, PDFs, source dumps, ZIPs, or copied runtime assets. Dated
session records and superseded decisions are provenance; preserve them rather
than rewriting history when current implementation changes.

## Definition of done

A change is complete only when its scope is reviewed for accidental edits and:

- affected targets build and relevant exact tests pass;
- behavior changes pass the full Release build and labeled CTest suite above;
- configured formatting, analysis, sanitizer, coverage, benchmark, or manual
  checks relevant to the risk pass, with environment and limitations reported;
- new serious diagnostics are resolved and unrelated pre-existing failures are
  recorded without opportunistic fixes;
- matching canonical documentation is synchronized in the same change;
- the final handoff states what was verified, what was not, and why.

## Nested guidance

Subsystem rules live in:

- `Illumo/Source/App/AGENTS.md`, `Illumo/Source/Engine/AGENTS.md`, and
  `Illumo/Source/Foundation/AGENTS.md`;
- `Illumo/Source/Game/AGENTS.md` and `Illumo/Source/Rulesets/AGENTS.md`;
- `Illumo/Source/Services/AGENTS.md` and `Illumo/Source/Tests/AGENTS.md`;
- `Illumo/Source/Platform/AGENTS.md` plus
  `Illumo/Source/Platform/Windows/AGENTS.md`,
  `Illumo/Source/Platform/Linux/AGENTS.md`, and
  `Illumo/Source/Platform/macOS/AGENTS.md`;
- `Illumo/Source/Rendering/AGENTS.md` plus
  `Illumo/Source/Rendering/OpenGL/AGENTS.md`,
  `Illumo/Source/Rendering/Mock/AGENTS.md`, and
  `Illumo/Source/Rendering/Primitives/AGENTS.md`.

Child guidance specializes this file and does not weaken it.

## Maintaining guidance

Update this file or a nested `AGENTS.md` only when a task changes a durable
invariant, convention, boundary, command, or required workflow. Do not use
guidance as a class catalog, issue tracker, session log, or current-state
architecture narrative. Preserve user-authored policy unless explicitly
superseded.
