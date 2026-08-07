# Services

Cross-cutting application services:

- logging
- environment / config vars
- input
- command line / command registry
- custom allocators (`ArenaAlloc`, `ChainedStackAlloc`, `ChainedPoolAlloc`,
  `MallocAlloc` / `IAllocator`) with `Illumo.Alloc.*` tests; wired into
  console parse/dispatch (`CommandLine`), frame scratch (`Renderer`), save
  load buffers, and optional `SparseCellGrid` chunks — still not forced into
  every service
- platform-neutral `SaveLoad` API (implementations live under `Platform/`)

`Logger` is best-effort output to its available file and console sinks; its
logging calls do not provide a success/failure result for callers to handle.

`EnvVars` loads and saves the production configuration beside the executable,
not in the process working directory. CMake seeds that location from
`Illumo/envvars.json` only when no local configuration exists; explicit paths
keep file-backed tests isolated.

`CommandLine` owns general console commands and validated environment settings.
Token draw uses shared `RenderStyleId::Console` on `Renderer` and is placed on
the Scene `UI` layer by `DebugModule`.
Product modules add domain commands through `CommandRegistry`, including usage,
description, and completion metadata. `CellGameModule` therefore owns simulation,
canvas, camera, ruleset, and save/load commands without introducing Game types
into Services. The console editor uses measured caret/selection geometry, a
horizontal input viewport, multi-command chaining with `;`, alias macro management
(`alias`/`unalias`), inline ghost-text auto-suggestions, dynamic parameter syntax hints,
and futuristic glassmorphic UI styling. History lines word-wrap and scroll by
visual line so early console output is not truncated when paging upward. An
8,000-quad batch prevents long wrapped output from silently exhausting the
easy-font mesh.
