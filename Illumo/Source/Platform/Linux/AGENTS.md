# Linux platform guidance

This file specializes `Illumo/Source/Platform/AGENTS.md` for Linux.

Linux sources are unverified scaffolding. They must use the same generic
application runner and SaveLoad contract as Windows and must not include game
types. Do not describe Linux as supported until native CMake, compiler, GLFW,
OpenGL, GTK dialog, runtime, and shutdown checks pass.
