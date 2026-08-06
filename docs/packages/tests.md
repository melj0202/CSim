# Illumo headless tests

Executable target: **`IllumoTests`** (alias: `IllumoRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend | `TestMockBackend.cpp` | Token queue, create records, proof sequence |
| Renderer E2E | `TestRendererE2E.cpp` | Inject mock into Renderer, Canvas RGB-display tokens, hybrid draw |
| RuleSets | `TestRuleSets.cpp` | GoL block/blinker, Seeds, Brian's Brain, tags |
| CellContext | `TestCellContext.cpp` | Mode normalize, factory, env canvas size |
| Canvas domain | `TestCanvasDomain.cpp` | set/get, clear, palette/fade, dirty-rect RGB upload, enroll |
| UI tokens + console | `TestUITokens.cpp` | CommandLine editing/completion/validation/dispatch/help; command chaining and aliases; ghost text and parameter hints; repeat execution and history search; open/closed/invisible tokens; full-help mesh-capacity regression; GLString empty/FPS; combined scene |

Shared helpers: `TestHelpers.h`, `TestHarness.h` (`NullRenderWindow`, `HeadlessCanvasFixture`).

## Build & run

From the repository root, a normal default build now builds and runs the
current suite automatically:

```bat
cmake --build build --config Release
```

From the CMake build directory (repo `build/`), the explicit commands remain:

```bat
cmake --build . --config Release --target IllumoTests
ctest -C Release -R IllumoTests --output-on-failure
```

Or:

```bat
Release\IllumoTests.exe
```

Exit code `0` = all checks passed. No OpenGL context is required for these tests (`MockBackend`).
