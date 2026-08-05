# Illumo headless tests

`IllumoTests.exe` is one compile-efficient runner, but it is not one CTest test.
The runner discovers every logical case and CMake registers each exact name as
its own process-isolated CTest entry. Each process uses a separate working
directory under `build/Testing/Illumo/`, so file-backed tests can run in
parallel without sharing `envvars.json`, logs, or `.illumo` saves.

From the repository root:

```powershell
cmake -S Illumo -B build -DILLUMO_BUILD_DOCUMENTATION=OFF
cmake --build build --config Release --target IllumoTests
ctest --test-dir build -C Release -N -L Illumo
ctest --test-dir build -C Release -L Illumo --output-on-failure
```

Focused execution:

```powershell
build\Release\IllumoTests.exe --list
build\Release\IllumoTests.exe --run Illumo.CellGame.SaveLoadRoundTrip
ctest --test-dir build -C Release -R '^Illumo\.CellGame\.' --output-on-failure
```

No arguments still run every case for local convenience. New tests must call
`registry.add("Illumo.<area>.<behavior>", ...)` once per logical behavior; do
not hide multiple independent behaviors behind a new suite-level CTest result.

## Coverage

Configure a Clang/LLVM build with `ILLUMO_ENABLE_COVERAGE=ON`, then build the
`IllumoCoverage` target. On Windows, an explicit SDK `rc.exe` path may be needed
when using a standalone Clang installation outside a Visual Studio shell.

```powershell
cmake -S Illumo -B build-coverage -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ `
  -DILLUMO_BUILD_DOCUMENTATION=OFF -DILLUMO_ENABLE_COVERAGE=ON
cmake --build build-coverage --target IllumoCoverage
```

The target runs every granular case, merges the per-process profiles, prints an
LLVM report, fails below 85% line coverage, and writes
`build-coverage/coverage-html/index.html`.

The denominator is first-party production code compiled by the headless target.
It excludes test code, vendored/system headers, `Rendering/Mock`, and
`Rendering/OpenGL`. Platform entry points, native dialogs, real-window behavior,
and live OpenGL require proportional manual smoke tests.
