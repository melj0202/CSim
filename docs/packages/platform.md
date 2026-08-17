# Platform

OS-specific entry points and native implementations.

| Port | Entry | Current status |
|------|--------|----------------|
| `Windows/` | `WinMain.cpp` | Supported production path; native dialogs in `WinSaveLoad.cpp` |
| `Linux/` | `_main.cpp` | Unsupported stale scaffold; entry references removed APIs and does not compile against the current shared bootstrap |
| `macOS/` | `Main.cpp` | Unsupported stale scaffold; entry references removed APIs and Objective-C++ is not enabled for `MacSaveLoad.mm` |

Platform code should stay thin: bootstrap the process, implement `SaveLoad`, then call `App/CellMain`.

Shared game/engine code does not belong here. Source presence and a CMake
branch are not support evidence: a port requires a native configure/build,
tests, startup/render interaction, dialogs, and clean shutdown.
