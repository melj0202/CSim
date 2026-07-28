# Unit Tests

## CSimRenderTests (Phase 6 + Renderer E2E)

Headless token/backend tests:

| File | Coverage |
|------|----------|
| `TestMockBackend.cpp` | MockBackend unit: lifecycle, creates, submit snapshots, proof sequence |
| `TestRendererE2E.cpp` | **Inject** MockBackend into `Renderer`; `RenderScene` + token drawable; **Canvas** tokens; `RenderProofQuad` |

Uses `NullRenderWindow` (no real GLFW window). Does not construct `GLBackend` in e2e paths.

### Build & run

From the CMake build directory (repo `build/`):

```bat
cmake --build . --config Debug --target CSimRenderTests
ctest -C Debug -R MockBackend --output-on-failure
```

Or:

```bat
Debug\CSimRenderTests.exe
```

Exit code `0` = all checks passed.
