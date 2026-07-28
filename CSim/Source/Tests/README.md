# CSim headless tests

Executable target: **`CSimTests`** (alias: `CSimRenderTests`).

| Suite | File | Covers |
|-------|------|--------|
| MockBackend | `TestMockBackend.cpp` | Token queue, create records, proof sequence |
| Renderer E2E | `TestRendererE2E.cpp` | Inject mock into Renderer, Canvas tokens, hybrid draw |
| RuleSets | `TestRuleSets.cpp` | GoL block/blinker, Seeds, Brian's Brain, tags |
| CellContext | `TestCellContext.cpp` | Mode normalize, factory, env canvas size |
| Canvas domain | `TestCanvasDomain.cpp` | set/get, clear, fade, enroll |
| UI tokens | `TestUITokens.cpp` | CommandLine open/closed/invisible; GLString empty/FPS; combined scene |

Shared helpers: `TestHelpers.h`, `TestHarness.h` (`NullRenderWindow`, `HeadlessCanvasFixture`).

## Build & run

From the CMake build directory (repo `build/`):

```bat
cmake --build . --config Debug --target CSimTests
ctest -C Debug -R CSimTests --output-on-failure
```

Or:

```bat
Debug\CSimTests.exe
```

Exit code `0` = all checks passed. No OpenGL context is required for these tests (MockBackend).
