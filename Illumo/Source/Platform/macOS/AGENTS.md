# macOS platform guidance

This file specializes `Illumo/Source/Platform/AGENTS.md` for macOS.

macOS sources are unverified scaffolding. They must use the same generic
application runner and SaveLoad contract as Windows and must not include game
types. Do not describe macOS as supported until native CMake/Objective-C++,
compiler, window/context, Cocoa dialog, rendering, and shutdown checks pass.
