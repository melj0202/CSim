# Tests subsystem guidance

This file specializes the repository `AGENTS.md` for `Illumo/Source/Tests/`.

## Scope and boundaries

The test target proves headless first-party behavior through the shared runner,
MockBackend, deterministic platform stubs, and isolated working directories.
It does not prove a native window, real OpenGL execution, native dialogs, or
unsupported platforms.

## Test contracts

- Register every logical behavior as its own exact `Illumo.<area>.<case>`
  entry. Do not collapse new coverage into a monolithic test result.
- Keep `--list`, `--run <exact-name>`, and generated CTest discovery in sync.
- Each CTest invocation must be process-isolated and use its assigned directory
  under `build/Testing/Illumo/`. Tests must not depend on execution order,
  repository working directory, ambient user configuration, or prior residue.
- Prefer public behavior and semantic backend actions over private layout.
  Tests named for wrapping, rendering, persistence, concurrency, or failure
  must assert that behavior rather than only exercising the path.
- Use deterministic SaveLoad replacements in the headless target. Never invoke
  native dialogs from automated tests.
- Reset MockBackend and mutable test state between cases. Any data pointer
  recorded from render commands must be copied or inspected within its valid
  submission lifetime.

## Build composition

Keep production sources shared by application and tests in
`ILLUMO_SHARED_SOURCES`; keep headless-testable application features in
`ILLUMO_HEADLESS_FEATURE_SOURCES`; keep test-only files in
`ILLUMO_TEST_SOURCES`. Preserve target-only behavior such as application Debug
Tracy and platform dialog sources.

## Verification

```powershell
cmake --build build --config Release --target IllumoTests
ctest --test-dir build -C Release -L Illumo --output-on-failure
build/Release/IllumoTests.exe --list
build/Release/IllumoTests.exe --run <exact-test-name>
```

Coverage uses the root `AGENTS.md` Clang/Ninja commands and must keep the 85%
headless-testable production-line gate. Add sanitizer or stress runs when
ownership, bounds, lifetime, or concurrency is at risk. Document manual smoke
coverage separately; never infer native or pixel correctness from headless
passes. Update `docs/packages/tests.md` when test topology changes.

Update this file only for durable test-harness contracts.
