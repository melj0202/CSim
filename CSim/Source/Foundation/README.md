# Foundation

Small, dependency-light shared pieces used across packages:

- compiler/platform macros (`MacroDefs`)
- build metadata (`BuildInfo`)
- host/sys info (`SysInfo`)
- math / container helpers (`MathTypes`, `ArrayQueue`) — note: named `MathTypes.h` so it does not shadow the system `math.h` on case-insensitive Windows
