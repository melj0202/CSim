# Illumo Foundation

Dependency-light supported pieces include:

- compiler/platform macros (`MacroDefs`)
- engine build/version metadata (`BuildInfo`)
- math and compact containers (`MathTypes`, `ArrayQueue`, `RollingMetric`)

The math header deliberately avoids the name `Math.h`, which shadows the CRT on
case-insensitive Windows. `BuildInfo` is an Illumo Foundation contract used by
the engine-owned system command-line parser.
