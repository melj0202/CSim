# CSim documentation

## Start here (later sessions)

| File | Role |
|------|------|
| **`architecture-consensus.md`** | **Single unified architecture document** (read this first) — merges agenda, CA/engine PDFs, assessments, decision log, review bugs, and code truth into one story |
| `csim-design-notes.tex` / `.pdf` | Long-form LaTeX design notes + full decision log |
| `sections/*.tex` | Chapter detail |

## Design notes (LaTeX)

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
| Consensus / work order / code truth | **`architecture-consensus.md`** (primary) |
| New closed choice (formal ID) | `sections/09-design-decision-log.tex` **and** consensus doc |
| Long-form chapter detail | `sections/05-…`, `06-…`, `07-…` as needed |
| Unresolved debate | `sections/10-open-questions.tex` |

### Provenance tags in the PDF

- **Status** — maturity snapshot  
- **Design decision** — closed choice (also in the log)  
- **Open** — needs author  
- **Inferred** — reverse-engineered; correct freely  

### Current architecture (short)

See **`architecture-consensus.md`**. In brief: token renderer; dense `lifeCanvas` + RGB fade display + dirty-rect upload; double-buffer `nextState`; headless `CSimTests`.

**Rule:** if docs and code disagree, **code wins** until docs are updated in the same change.
