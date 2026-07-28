# CSim documentation

## Design notes (LaTeX)

Primary living document:

| File | Role |
|------|------|
| `csim-design-notes.tex` | Main driver (TOC, packages, `\input`s) |
| `sections/*.tex` | Chapters — edit these in normal work |
| `figures/` | Optional images/diagrams |
| `csim-design-notes.pdf` | Build output (generate locally) |

### Build PDF

```bash
cd docs
latexmk -pdf -interaction=nonstopmode csim-design-notes.tex
```

Or twice with:

```bash
pdflatex -interaction=nonstopmode csim-design-notes.tex
pdflatex -interaction=nonstopmode csim-design-notes.tex
```

Output: `docs/csim-design-notes.pdf`

Needs a TeX distro with `tcolorbox`, `booktabs`, `tikz`, `listings`, `hyperref`, `ulem`, … (full TeX Live works).

**Windows note:** if `pdflatex` is not on `PATH`, use e.g.  
`C:\texlive\2026\bin\windows\latexmk.exe` (adjust year) or add that `bin\windows` folder to your user `PATH`, then open a new terminal.

### What to edit when

| Situation | Edit |
|-----------|------|
| New closed choice | `sections/09-design-decision-log.tex` |
| Current render / sim architecture | `sections/05-rendering-current.tex`, `06-rendering-target.tex`, `07-game-and-rules.tex` |
| Unresolved debate | `sections/10-open-questions.tex` |
| New important path | `sections/B-file-map.tex` |
| Package layout change | `sections/03-source-layout.tex` |

### Provenance tags in the PDF

- **Status** — maturity snapshot  
- **Design decision** — closed choice (also in the log)  
- **Open** — needs author  
- **Inferred** — reverse-engineered; correct freely  

### Current architecture (short)

Token frame path is **shipped** (enroll → `AppendCommands` → submit via `IBackend`). Canvas is **R8 + palette** with dirty-rect PBOs. Rules use **double-buffer `nextState`**. Headless **`CSimTests`** covers mock backend, rules, canvas, UI tokens.

See the design notes PDF/sections for full detail and the decision log (D-R*, D-P*).
