# Illumo library headless tests

`IllumoTests` covers the reusable library. Each `Illumo.*` case is an isolated
CTest process; `Illumo.PublicHeaders.ConsumerSmoke` separately compiles the
complete supported public header tree without Illumo private include paths.

```powershell
cmake -S Illumo -B build-illumo -DILLUMO_BUILD_DOCUMENTATION=OFF
cmake --build build-illumo --config Release --target IllumoTests
build-illumo/Release/IllumoTests.exe --list
ctest --test-dir build-illumo -C Release -L Illumo --output-on-failure
```

The root workspace adds `IllumoGameTests` and the `IllumoWorkspace` aggregate
label. Coverage excludes tests, vendored/system code, and the live OpenGL
backend.
