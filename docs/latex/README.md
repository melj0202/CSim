# LaTeX book source

`illumo.tex` is the only current PDF entrypoint. It includes every chapter under
`sections/`, including the formal decision log and the 2026-08-04 session record.

Build from the repository root with the wrapper:

```powershell
.\docs\build.ps1
```

Generated files belong in `../output/`; edit `.tex` sources here. The wrapper
is also the default-build `IllumoDocs` CMake target when TeX is available.
