# LaTeX book source

PDF entrypoints:

| Source | Output | Role |
|--------|--------|------|
| `illumo.tex` | `../output/illumo.pdf` | Design notes, decisions, prose chapters under `sections/` |
| `architecture-map.tex` | `../output/architecture-map.pdf` | Landscape multi-page chart pack only (layers, ownership, frame loop, class maps) |

Build from the repository root with the wrapper (builds both):

```powershell
.\docs\build.ps1
```

Generated files belong in `../output/`; edit `.tex` sources here. The wrapper
is also the default-build `IllumoDocs` CMake target when TeX is available.
