# 2026-08-04 granular test coverage

## Outcome

- `IllumoTests.exe` remains one compile-efficient runner, while CTest now lists
  and invokes every logical behavior separately.
- 80 process-isolated CTest entries pass in the Windows Release and Clang/LLVM
  instrumented builds.
- The checked-in coverage target reports 87.68% line coverage, 87.50% function
  coverage, and 77.28% branch coverage across 4,872 measured lines.
- `IllumoCoverage` reruns the granular suite, enforces an 85% production-line
  gate, and writes an HTML report.

## Feature coverage added

The suite now directly covers active ruleset truth tables, Canvas domain/fade
and dirty upload behavior, CellGameModule startup/teardown, console commands,
file-backed save/load validation and restoration, camera/timing transitions,
input mappings/queues/contexts, asset enrollment, environment persistence,
logging, system argument parsing, renderer tokens, CommandLine/GLString, and
SplashText.

The test runner assigns each CTest process a separate working directory below
`build/Testing/Illumo/`, preventing parallel persistence/log tests from sharing
files.

## Coverage boundary

The automated denominator is headless-testable first-party production code. It
excludes test code, vendored/system code, `Rendering/Mock`, and
`Rendering/OpenGL`. Native dialogs, fullscreen/live-window behavior, live
OpenGL execution, and non-Windows ports still require proportional smoke tests.
