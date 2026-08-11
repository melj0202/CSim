# macOS platform scaffold

macOS is not currently supported. CMake selects `Main.cpp` and
`MacSaveLoad.mm`, but the entry includes the removed `ServiceLocator.h` and
calls obsolete shared APIs. The project does not enable Objective-C++ for the
selected `.mm` file, and the shared GLFW OpenGL bootstrap has not been
validated for the required macOS context profile. `CocoaRenderWindow.*` is
unselected legacy scaffolding, not an alternate production path.

A future port should reuse the shared App, GLFW/OpenGL, renderer, and save
format contracts and pass native AppleClang configure/build, tests, Retina
rendering/input, Cocoa dialog, and shutdown checks before support is claimed.
