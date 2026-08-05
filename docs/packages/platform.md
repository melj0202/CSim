# Platform

OS-specific entry points and native implementations.

| Port | Entry | Notes |
|------|--------|--------|
| `Windows/` | `WinMain.cpp` | Save/load: `WinSaveLoad.cpp` |
| `Linux/` | `_main.cpp` | Save/load: `LinuxSaveLoad.cpp` |
| `macOS/` | `Main.cpp` | Save/load: `MacSaveLoad.mm` |

Platform code should stay thin: bootstrap the process, implement `SaveLoad`, then call `App/CellMain`.

Shared game/engine code does not belong here.
