# 2026-08-04 — Illumo rename, console, UI, and documentation consolidation

## Scope

This record captures the work completed during the session that established the
Illumo product identity, expanded the developer console and its editor, repaired
window/console presentation problems, and consolidated project documentation.

It is an implementation record, not a replacement for
`docs/architecture-consensus.md` or the formal LaTeX decision log.

## Product identity

- Renamed the live first-party project identity from CSim to **Illumo**.
- Updated repository-facing paths, CMake targets, test names, runtime title,
  console title, documentation, and save extension.
- The runtime host class remains `Illumo`; the shared product/host name is
  intentional.
- Canonical test executable and CTest entry: `IllumoTests`.
- Current project saves use `.illumo`.
- Historical, generated, vendored, and archived references may retain the old
  name when rewriting them would falsify provenance.

Formal decision: D-N1.

## Build and product-composition boundary

- Windows build commands are:

  ```powershell
  cmake -S Illumo -B build
  cmake --build build --config Release
  ```

- A normal build compiles `IllumoTests` and runs its CTest entry through the
  `IllumoRunTests` `ALL` target.
- `DebugModule` remains a **Debug-only** product module:
  - `CellMain.cpp` includes and registers it only under `#ifndef NDEBUG`.
  - `Illumo/CMakeLists.txt` compiles `DebugModule.cpp` only for the Debug
    configuration.
  - Release builds contain no `DebugModule` object.
- Shared console/service code may compile in Release because logging, command
  registration, and tests use it; that does not ship the Debug overlay module.

Formal decision: D-B1.

## Developer console architecture

The console now has a deliberate two-layer command model:

1. `CommandLine` owns generic mechanics and application-wide commands.
2. `CellGameModule` registers cellular-automata domain commands through
   `CommandRegistry`.

Registry entries carry usage, description, and completion candidates. The same
metadata drives `help`, `help <command>`, and Tab completion. Registered commands
queue once and no longer fall through to a false “unknown command” error.

### Generic commands

| Group | Commands |
|---|---|
| Console/application | `help`, `echo`, `clear`, `close`, `quit` |
| Environment | `get`, `set`, `toggle`, `vars [filter]` |
| Timing | `tps`, `speed`, `fade` |
| Display | `fps`, `fullscreen` |

Numeric and boolean values are parsed and range-checked. Unknown command names
do not silently create environment variables. `vid_restart` is not advertised:
the former no-op now explains that safe OpenGL context recreation requires full
resource re-enrollment.

### Game commands

| Group | Commands |
|---|---|
| Simulation | `pause`, `run`, `step [count]`, `status` |
| Canvas | `clear_canvas`, `randomize [density-percent]`, `setcell <x> <y> <state>` |
| Rules | `ruleset [name]`, `mode [name]` |
| Files | `save <filename>`, `load <filename>`, `save_dialog`, `load_dialog` |
| Camera | `camera [x y [zoom]]`, `camera_reset` |

Ruleset completion is sourced from `CellContext` rather than duplicated in the
console. `setcell` permits numeric multi-state editing, including Wireworld
head/tail placement, although the mouse brush still lacks a convenient
multi-state workflow.

Formal decision: D-CLI1.

## Save and load behavior

The formerly unfinished save/load commands now call `CellGameModule` behavior.

- Save adds `.illumo` when appropriate, writes a bounded zero-filled ruleset
  tag, dimensions, and dense cell bytes, and reports stream failure.
- Load tries the supplied path and the `.illumo` form, validates the header,
  known ruleset, dimensions, allocation limit, and complete cell payload before
  changing the current canvas.
- A valid load activates the saved ruleset, clears the current canvas, copies
  the overlapping region row-by-row, marks cells dirty, rebuilds palette
  targets, and snaps the visible display to the loaded state.
- A dimension mismatch is reported rather than producing a flat-copy layout
  error.
- Native dialog commands use the existing platform `SaveLoad` API.

Headless tests cover the related command mechanics and Canvas refresh behavior;
native file dialogs still require a Windows manual smoke test.

## Console text editing and visuals

The command editor now supports:

- Left/Right and Home/End navigation.
- Ctrl+Left/Right word navigation.
- Backspace/Delete and Ctrl+Backspace/Delete word deletion.
- Shift selection and Ctrl+A.
- Typing over a selection.
- Command history.
- Quoted and escaped arguments.
- Command, variable, ruleset, and command-specific completion.
- A horizontally scrolling input viewport that keeps the insertion point visible.

The caret is a rendered bar positioned from measured prefix width. It is no
longer an underscore appended to the input string, eliminating the apparent
rightward cursor offset.

The console uses layered token-rendered chrome, a dedicated input row, history
viewport, scrollbar, selection highlight, and one batched update/draw. The panel
height and available history rows derive from the current window dimensions.

`stb_easy_font` expands each character into several geometry quads. The original
2,000-quad console mesh could therefore exhaust its buffer during a detailed
`help` page, truncating the final hint at text such as `Use 'h`. The console now
reserves 6,000 quads, and a focused test renders realistic long help lines and
asserts that the batch exceeds the old cap without being clipped.

Formal decision: D-UI1.

## Fullscreen behavior

Fullscreen toggling now has symmetric state transitions:

- Entering fullscreen saves the current windowed position and size, then uses
  the primary monitor’s current video mode.
- Leaving fullscreen restores the saved windowed bounds.
- Console commands accept `on`, `off`, and `toggle`, avoid redundant window
  calls, and persist the environment value.

This corrects the earlier reversed branch behavior and supports console layout
against the current window dimensions. Live monitor/context behavior remains a
manual OpenGL smoke-test concern.

## Service lifetime correction

`Logger` can mirror messages into the in-app console after `CommandLine` is
constructed. Because that pointer is non-owning, `Illumo` clears the logger
context during destruction before its owned services disappear. This prevents
late shutdown logging from using a dangling console pointer.

## Tests and verification

Added or expanded headless coverage for:

- Cursor editing, selection, word operations, quoted parsing, and completion.
- Command dispatch, validation, help metadata, variable behavior, fullscreen
  deduplication, and close/quit behavior.
- Registered-command queue execution without unknown-command fallthrough.
- Open, closed, invisible, and scrolling console token behavior.
- A detailed help page that exceeds the former 2,000-quad font limit.

During integration, two different test translation units defined incompatible
`NullRenderWindow` classes. Renaming the renderer-E2E-local fake removed that
ODR collision.

Verified in this session:

- Release application build succeeded.
- Debug application build succeeded.
- Canonical `IllumoTests` CTest entry passed.
- The default Release build automatically ran and passed `IllumoTests`.
- Release build artifacts contained no `DebugModule` object.
- `git diff --check` passed after documentation/source edits.

The compiler still reports pre-existing ignored-`nodiscard`, unused-parameter,
and Debug ASan linker warnings. They were outside this session’s requested scope.

## Documentation consolidation

All first-party documentation now lives under `docs/`:

- `architecture-consensus.md` is the current architecture authority.
- `current-issues.md` is the reproducible issue snapshot.
- `contributing.md` contains coding/dependency rules.
- `packages/` preserves useful package READMEs formerly scattered beside code.
- `sessions/` records major implementation sessions such as this one.
- `history/` preserves original requests, early notes, and superseded documents.
- `latex/illumo.tex` is the only current PDF entrypoint.
- `latex/sections/` contains long-form chapters and the append-only decision log.
- `output/` is generated and is never a source of truth.

The wrapper `docs/build.ps1` creates the output directory and invokes `latexmk`
with a quoted output-directory argument. When Windows PowerShell and `latexmk`
are available at CMake configure time, the default `cmake --build` also runs the
same wrapper through the `IllumoDocs` target. Set
`ILLUMO_BUILD_DOCUMENTATION=OFF` to opt out on a code-only machine.

Formal decision: D-DOC1.

## Remaining high-value work

1. Gate module `Update` and `DispatchDrawables` after a failed `Start`.
2. Add a mode-aware Wireworld startup seed.
3. Add an ergonomic Wireworld head/tail mouse brush.
4. Add direct save/load command integration tests around actual files and native
   dialog smoke coverage.
5. Define and enforce the 2,048-command queue overflow policy.

Large-scale architecture remains intentionally deferred: infinite chunks,
threaded/SYCL simulation, a second real renderer backend, ECS, render graphs,
and a generalized scene graph require concrete product or learning pressure.
