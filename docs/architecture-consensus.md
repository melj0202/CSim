# CSim architecture consensus

**Status:** Living summary for later sessions  
**Last updated:** 2026-07-30  
**Sources merged:** in-tree design notes (D-R\*, D-P\*, D-E\*, D-C1, D-F1), local code review, architecture PDFs (`grok_report_csim_arch*`), ChatGPT arch assessment, and current product code after token migration / perf / Wireworld / fade restore.

This document is the **final consensus** on what CSim is, what is intentional, what is debt, and what to do next. Prefer updating this (and the decision log) when the consensus changes, rather than re-deriving from chat history.

---

## 1. Overall assessment

CSim is a **coherent layered modular monolith**, appropriate for a small desktop cellular-automata simulator.

```
Platform → App/CellMain → Engine/Illumo → Modules → Game + Rulesets
                                         → Rendering (tokens) → OpenGL | MockBackend
```

It is an **engine-shaped application**, not a cleanly separated “engine product + game product.” That is **acceptable and intentional** at current scale.

**Verdict:** Functional, reasonably testable, better than most solo C++ graphics projects of this size. Main residual risk is **transitional / product-correctness holes**, not a need for ECS or a rewrite.

---

## 2. Strengths (keep these)

| Area | Consensus |
|------|-----------|
| **App owns composition** | `CellMain` registers modules after `Illumo::Init`; Engine does not construct Game modules (D-E1). |
| **Module lifecycle** | `Start` / `Update` / `DispatchDrawables` / `Exit` is easy to follow. |
| **Render split** | Enroll once; emit tokens per frame; `IBackend` + `MockBackend` make the path testable (D-R1–D-R8, D-R10). |
| **Rulesets** | Strategy hierarchy; pure-ish `nextState` + `evalCell`; double-buffered generation (D-P3). |
| **Scene model** | Per-frame **drawable list** only — no scene graph / EntityTable (D-E3, D-E4). |
| **Tests** | Headless `CSimTests`: rules, canvas, tokens, UI, renderer inject. |
| **Debt management** | Dead experiments archived under `archive/` rather than left half-live. |

---

## 3. Canonical architecture snapshot (code truth)

### 3.1 Packages

| Package | Role |
|---------|------|
| **App/** | Product composition (`CellMain`). |
| **Engine/** | Host: Illumo, modules, frozen `IllumoContext` bag. |
| **Game/** | Domain + presentation for the CA (`Canvas`, `CellGameModule`, `CellContext`). |
| **Rulesets/** | CA rules (GoL family, Wireworld, …). |
| **Rendering/** | Scene list, drawables, Renderer, tokens, OpenGL, Mock. |
| **Services/** | Log, env, input, console UI, save/load API. |
| **Foundation/** | Macros, math aliases, sysinfo. |
| **Platform/** | OS entry + native dialogs. |
| **Tests/** | Headless suite. |

### 3.2 Frame loop (production)

```
CellMain
  → Illumo::Init()          // services only
  → addModule(CellGameModule) [+ DebugModule in Debug]
  → StartModules()
  loop:
    Update(dt)              // Input → Camera → modules.Update
    Render()
      scene->ClearDrawables()
      modules.DispatchDrawables(scene)
      renderer->BeginFrame / RenderScene / EndFrame
```

### 3.3 Canvas presentation model (canonical — **code wins over old notes**)

**Current code (post fade restore):**

| Layer | State |
|-------|--------|
| **Domain** | `lifeCanvas` — dense `unsigned char[width * height]` (not chunked) |
| **View** | CPU palette → `targetRgb` / `displayRgb` fade; `texCanvasBuffer` RGB |
| **GPU** | RGB display texture; dirty-rect `UpdateTexture` (PBO path); canvas shaders sample `uDisplayTexture` |

**Not current:** GPU R8 cell texture + palette texture as the live presentation path (that was an intermediate design; fade brought back CPU RGB display).

**Decision D-C1:** Keep Canvas as intentional domain+view+GPU monolith until pure-sim tests or large grids force a `LifeGrid` / `CanvasView` split.

### 3.4 Rules / encoding (selected)

- Binary CAs: `0` = alive, `1` = dead (neighbor count treats `0` as “alive”).
- **Wireworld:** `0` head, `1` empty, `2` tail, `3` conductor (head = 0 reuses head-neighbor counting).
- Generation always walks the **full** grid; dirty AABB is for **visual** upload sparsity.

### 3.5 Production drawables (D-R10)

Pure-token when contributing: **Canvas, CommandLine, GLString, SplashText**.  
Immediate `Draw()` hybrid remains for **tests/stubs only**.

---

## 4. Locked decisions (short index)

| ID | Decision |
|----|----------|
| D-R1…D-R8 | Token migration foundation (union, emitters, handles, phases, mock inject, …) |
| D-R9 | String-named uniforms = debt if a second *real* GPU API appears |
| D-R10 | Production drawables are pure-token |
| D-P1…D-P4 | Dirty visual path, UI batch, double-buffer sim, dirty-rect uploads |
| D-E1 | Module registration in App |
| D-E2 | InputManager has no Game types |
| D-E3 | EntityTable archived |
| D-E4 | Scene is drawable list only |
| D-E5 | Freeze `IllumoContext`; validate at `Start`; third module → explicit deps |
| D-C1 | Canvas dual role intentional for now |
| D-F1 | MacroDefs/`Windows.h` include toxicity deferred until it hurts |

Full prose: `docs/sections/09-design-decision-log.tex`.

---

## 5. Known product / correctness issues (fix when touching related code)

From the 2026-07-30 local code review (still open unless noted fixed in git history):

1. **Partial module `Start`** — validation can return early without `cellContext`, but `Update` / `DispatchDrawables` still dereference → crash risk. Need a started-OK guard (or remove failed modules from the loop).
2. **`LoadCellGame`** — writes `lifeCanvas` without `markCellsDirty` → stale fade/display until something else dirties the grid.
3. **`IllumoContextHasGameCore`** — omits `commandLine` while `Edit` uses `commandLine->isOpen`.
4. **Wireworld editor** — paint is conductor/empty only; no head placement tool.
5. **Startup seed** — GoL glider (value 0) is wrong / misleading under `WIREWORLD`.
6. **Duplicate ruleset name lists** — console allow-list vs `CellContext::IsKnownModeString` can drift.
7. **Test gaps** — Wireworld 2-head birth / 3-head no-birth; load→dirty; incomplete-Start safety.

These are **higher priority than** backend purity or Canvas splits.

---

## 6. Architectural debt (do when it hurts)

| Item | Severity | Notes |
|------|----------|--------|
| String-named uniforms | Low until 2nd GPU API | D-R9 |
| Hybrid Draw path | Low | Keep for tests; production already tokens |
| Command queue 2048 silent drop | Low–medium | Log/assert if ever near cap |
| CMake source list duplication (CSim vs CSimTests) | Medium maintainability | Shared library / object target later |
| `Renderer.h` includes GL backend types | Medium purity | Factory / pimpl later |
| Full-grid sim + ~24 B/cell float fade | Scale wall | Redesign if large/infinite canvas becomes a product goal |
| MacroDefs + Windows.h | Deferred D-F1 | Split only if toxicity shows |
| IllumoContext bag growth | Frozen D-E5 | Don’t grow for convenience |

---

## 7. Explicit non-goals (for now)

- Full ECS / EntityTable revival  
- Scene graph / hierarchical transforms for cells  
- Second real graphics API (Vulkan/Metal) as a near-term project  
- Perfect Linux/macOS parity before Windows remains solid  
- General-purpose AAA renderer  

---

## 8. Recommended work order (consensus)

### Fix first (correctness)

1. Module Start failure → do not run Update/Dispatch for that module; fix game-core check (`commandLine`).  
2. Load path → `markCellsDirty` + visual rebuild.  
3. Wireworld seed + head placement UX.  
4. **Align design notes** with RGB-fade Canvas (remove stale “GPU R8 + palette texture” as *current* truth where still present).  

### Then hygiene

5. Single source of truth for known ruleset names (console + factory).  
6. Shared CMake target for common sources.  
7. Command-queue overflow policy (log/assert).  

### Only if product needs it

8. Canvas split (`LifeGrid` / `CanvasView`).  
9. Large-grid / high-TPS sim redesign.  
10. Backend factory + non-string uniforms.  

---

## 9. How to use this document

- **Starting a new chat / session:** read this first, then `docs/csim-design-notes.pdf` (or `.tex` sections) for detail.  
- **Changing architecture:** update this consensus **and** append a decision log entry; do not leave two conflicting “current” stories.  
- **Code truth vs docs:** if they disagree, **code wins** until docs are updated in the same change set.  

---

## 10. One-line summary

**Keep the modular monolith, pure-token frame path, and dense-grid Canvas; fix Start/load/Wireworld correctness and docs drift; defer multi-backend, ECS, and MacroDefs splits until real pain.**
