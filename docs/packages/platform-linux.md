# Linux platform scaffold

Linux is not currently supported. CMake selects `_main.cpp` and
`LinuxSaveLoad.cpp`, but the entry includes the removed `ServiceLocator.h` and
calls obsolete `SysCmdLine` and `CellMain` signatures. These are static source
findings; no Linux build or runtime validation is claimed.

A future port should forward `argc`/`argv` through the current shared App API,
keep GTK limited to native dialog adaptation, and pass native configure,
compile, tests, GLFW/OpenGL interaction, dialog, and shutdown checks before the
support status changes.
