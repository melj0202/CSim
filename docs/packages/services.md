# Services

Cross-cutting application services:

- logging
- environment / config vars
- input
- command line / command registry
- custom allocators
- platform-neutral `SaveLoad` API (implementations live under `Platform/`)

`CommandLine` owns general console commands and validated environment settings.
Product modules add domain commands through `CommandRegistry`, including usage,
description, and completion metadata. `CellGameModule` therefore owns simulation,
canvas, camera, ruleset, and save/load commands without introducing Game types
into Services. The console editor uses measured caret/selection geometry, a
horizontal input viewport, multi-command chaining with `;`, alias macro management
(`alias`/`unalias`), inline ghost-text auto-suggestions, dynamic parameter syntax hints,
and futuristic glassmorphic UI styling. A 6,000-quad batch prevents long output
from silently exhausting the easy-font mesh.
