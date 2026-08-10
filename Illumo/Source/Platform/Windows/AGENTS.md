# Windows platform guidance

This file specializes `Illumo/Source/Platform/AGENTS.md` for Windows.

## Scope and contracts

Windows supplies `WinMain` and native save/load dialogs for the supported
production path. It adapts Win32 process/dialog APIs to shared App and
SaveLoad contracts; it must not own game persistence or renderer policy.

- Forward the process command line to the shared argument parser without
  inventing a second grammar.
- Preserve Debug console setup only as platform bootstrap; DebugModule remains
  App/Engine composition.
- Treat dialog cancel as a normal, distinguishable outcome. Do not overwrite,
  truncate, or load a file merely because the picker opened.
- Save dialogs must allow a new destination and apply extension/overwrite
  behavior explicitly. Load dialogs must require an existing readable file.
- Use Unicode-capable APIs and bounded dynamic paths for user-selected files;
  avoid silent truncation and attach dialogs to the application window when
  the shared contract supplies an owner.
- Keep Win32 headers and handles inside this directory.

## Verification

Build Release and Debug, then manually verify startup, clean shutdown, new
save, overwrite, load, long/Unicode path, and cancellation. Use generated test
files only under `build/Testing/ManualSmoke`. Headless SaveLoad stubs do not
prove this code. Record environment and any native-dialog limitation in the
handoff and `docs/packages/platform.md` when behavior changes.

Update this file only for durable Windows platform rules.
