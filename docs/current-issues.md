# Illumo current issues

This is a review snapshot, not proof that every item remains open. Reproduce or
inspect an item against the live tree before fixing it.

Last reviewed: 2026-08-04

## Open correctness issues

| Priority | Issue | Evidence and consequence |
|---|---|---|
| 1 | Failed module startup is not gated | `CellGameModule::Start` can return without creating `cellContext`, while later `Update` and `DispatchDrawables` paths still assume it exists. `DebugModule` has the same class of lifecycle risk. |
| 2 | Wireworld has no convenient head/tail mouse brush | `setcell` can place numeric states, but normal paint input still favors conductor/empty editing. |
| 3 | Startup seed is not ruleset-aware | The default Game-of-Life glider becomes five electron heads under Wireworld rather than a useful wire. |

## Coverage gaps

- Manually smoke-test native save/load dialogs, fullscreen transitions, and live
  OpenGL console presentation after relevant changes; MockBackend cannot prove
  those platform paths.

## Structural risks

- `CommandQueue` has a fixed 2,048-command capacity and needs an explicit
  overflow policy rather than silent loss.
- Per-command token payload pointers must remain valid until submission returns.
- Dense simulation still scans the full grid; visual dirty rectangles reduce
  upload work, not simulation complexity.

## Resolved during the 2026-08-04 session

- Split the headless suite into 80 separately named, process-isolated CTest
  entries and added an 85% LLVM production-line coverage gate.
- Added explicit Wireworld neighbor truth tables and direct file-backed
  save/load round-trip, corruption, size-overlap, and command tests.
- Covered the game module, InputManager, AssetManager, backend-token conversion,
  SysCmdLine, SplashText, environment, logger, and command registry paths.
- Renamed the live product, targets, tests, runtime title, and saves to Illumo.
- Kept `DebugModule` out of Release compilation and composition.
- Added advanced console editing, selection, history, completion, measured caret
  placement, horizontal input scrolling, and improved panel visuals.
- Fixed detailed help truncation by increasing the easy-font UI mesh from 2,000
  to 6,000 quads and adding a regression test.
- Replaced duplicated/hard-coded help with registry metadata and one known-ruleset
  source.
- Implemented validated generic, simulation, canvas, ruleset, file, camera, and
  display commands.
- Connected and hardened save/load; loads now refresh visual targets immediately.
- Fixed fullscreen enter/exit state restoration.
- Added required `commandLine`/registry context validation.
- Prevented shutdown logging from retaining a dangling console pointer.
- Consolidated first-party documentation under `docs/` with one current LaTeX
  entrypoint.

## Behaviors that are intentional

- Wireworld encodes head `0`, empty `1`, tail `2`, conductor `3`.
- Binary rules encode alive as `0` and dead as `1`.
- Canvas owns dense domain state, RGB targets/fade state, dirty tracking, and GPU
  enrollment until scale or testability creates a concrete split trigger.
- Scene is a rebuilt, ordered drawable list—not a scene graph or ECS.
- Production drawables use render tokens; immediate `Draw()` remains only for
  tests or incomplete stubs.
