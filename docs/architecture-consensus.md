# CSim — Architecture consensus (unified)

**Status:** Single living document — **authoritative for later sessions**  
**Last updated:** 2026-07-30  

This file **merges and supersedes** scattered design memory into one coherent story. Read this first; treat external PDFs and old agenda notes as **history** (§2).

| Source | Role in this document |
|--------|------------------------|
| Desktop “Main agenda for CSim” | Product wishlist → §7 scorecard |
| `CSim_Architecture_Decisions.pdf` (ChatGPT) | CA / OOP-practice design review → purpose, ownership, input, rules, SYCL |
| `CSim_Engine_Architecture_Decisions.pdf` (ChatGPT) | Aspirational 2D engine → what we kept vs discarded |
| `gpt_csim_arch_assessment.pdf` (ChatGPT) | Assessment of *current* tree → risks, direction |
| Grok architecture reviews + local code review | Strengths, bugs, debt |
| In-repo LaTeX design notes + decision log | Formal decision IDs (D-\*) |
| `docs/current_issues.txt` | Product/correctness punch list |
| **Current code under `CSim/Source/`** | **Wins** when anything conflicts |

**Rule:** If this document and the code disagree, **code wins** until this file is updated in the same change set.

Optional deeper reading (not required to resume work):

- `docs/csim-design-notes.tex` / `.pdf` — long-form chapters  
- `docs/sections/09-design-decision-log.tex` — append-only formal decision prose  

---

## Table of contents

0. [One-line summary](#0-one-line-summary)  
1. [Project purpose and scope](#1-project-purpose-and-scope)  
2. [Historical sources (how to read old notes)](#2-historical-sources-how-to-read-old-notes)  
3. [Overall assessment](#3-overall-assessment)  
4. [Strengths (keep these)](#4-strengths-keep-these)  
5. [Canonical architecture (code truth)](#5-canonical-architecture-code-truth)  
6. [Locked decisions (catalog)](#6-locked-decisions-catalog)  
7. [Early agenda vs today](#7-early-agenda-vs-today)  
8. [Known product / correctness issues](#8-known-product--correctness-issues)  
9. [Architectural debt](#9-architectural-debt-do-when-it-hurts)  
10. [Recommended work order](#10-recommended-work-order)  
11. [Core design principles](#11-core-design-principles-merged)  
12. [Open questions (remaining)](#12-open-questions-remaining)  
13. [How to use this document](#13-how-to-use-this-document)  

---

## 0. One-line summary

**CSim is a cellular-automata learning sandbox in a modular-monolith shell: App owns composition, Illumo owns services, modules drive sim/UI, rendering is enroll-once + token stream (OpenGL + Mock), Canvas is a dense grid with RGB fade display. Keep that shape. Fix Start/load/Wireworld correctness and docs drift. Defer ECS, multi-pass pipelines, multi-backend, infinite chunks, and SYCL until product pain or learning goals demand them.**

---

## 1. Project purpose and scope

### 1.1 Primary purpose (still true)

From the CA design review and the original spirit of the repo:

- **Practice OOP and systems programming**, not ship a commercial game engine.
- The main learning sandbox is **cellular automata** (GoL family, multi-state rules, editor, camera, console).
- Custom allocators, Tracy, backend interfaces, etc. are **useful experiments**, not requirements that must dominate the design.
- Do **not** rewrite everything for an imagined perfect architecture. Refactor where coupling produces **real friction**.

### 1.2 What the project grew into

It is no longer “one Game of Life file.” It is a **small CA framework / app**:

- Multiple rule sets, edit/run modes, save/load hooks  
- Token-based OpenGL presentation + headless tests  
- Engine-shaped packaging (App / Engine / Game / Rendering / Services)

That growth is **acceptable**. Architecture should reflect **actual families of behavior** (CA + UI + host), not a fantasy multi-genre engine.

### 1.3 Explicit non-goals (for now)

- Production AAA renderer or full multiplayer engine  
- Full ECS / archetypes / cached structural queries  
- Scene/World graph as the primary cell path  
- Second real graphics API (Vulkan/Metal) as a near-term deliverable  
- Perfect Linux/macOS parity before Windows remains solid  
- Infinite canvas + SYCL **before** dense-grid product paths are correct and documented  
- Aggressive batching/instancing/render graphs without profiling evidence  

### 1.4 Allocators (from CA design PDF — retained policy)

Retain arena / stack / pool allocators as learning code and optional utilities. Do **not** force them into every service. Appropriate uses:

| Allocator | Fit |
|-----------|-----|
| Arena | Frame scratch, transient command data, parsers, load buffers |
| Stack | Nested temporary lifetimes (LIFO free) |
| Pool | Many same-sized objects with stable slots |

A general-purpose allocator (mimalloc-class) is a different product; do not reinvent it.

---

## 2. Historical sources (how to read old notes)

Three generations of “design truth” got mixed in chat history. This section untangles them so later sessions do not re-implement superseded plans.

### 2.1 Desktop agenda (“Main agenda for CSim”)

Early product wishlist:

| Item | Intent |
|------|--------|
| On-screen text / fonts | UI |
| In-game command line | Tools |
| Console drawn in the GL window | UI |
| More rule sets | Content |
| Infinite canvas in **16×16 chunks** + hash map | Scale |
| Mouse drag pan | UX |
| SYCL (or similar) for large cell counts | Scale / learning |

**Outcome today:** UI/tools/pan/most rulesets largely done. **Chunk infinite canvas + SYCL not done** (deliberately deferred). Full scorecard → §7.

### 2.2 CA design review (`CSim_Architecture_Decisions.pdf`)

Themes that **still guide** the project:

| Theme | Guidance |
|-------|----------|
| Ownership | Explicit Engine/App object; reduce uncontrolled global lookup |
| Input | Thin layer: GLFW callbacks → InputManager → application logic |
| Boundaries | Contain GLFW/OpenGL behind wrappers; no GL in sim/UI domain |
| Sim vs view | Separate simulation from presentation; double-buffer generations |
| Rules | Families (life-like / elementary / multi-state); JSON for **data**, not executable behavior |
| Parallel | Parallelize **simulation data** last—after serial works |
| SYCL | Optional compute backend; never a hard engine dependency |
| Refactor plan | Ownership → input extract → controllers → contain GL → rule families → double-buffer → then parallel |

**Principle:** *abstract real volatility; hard-code stable structure; ownership explicit.*

**Closer to current CSim than the “engine passes” PDF.**

### 2.3 Engine design PDF (`CSim_Engine_Architecture_Decisions.pdf`)

Aspirational **general 2D engine** design (July 2026). Valuable as boundary thinking; **not** the current product plan.

| Proposed | Status in CSim |
|----------|----------------|
| Illumo owns services; context as view | **Kept** |
| Backend owns GPU resources + ID tables | **Kept** (IBackend / GL registries) |
| Modules as subsystems | **Kept** |
| RenderPipeline with Opaque / Transparent / UI / Debug **passes** | **Not built** |
| MeshDrawCommand-style typed pass queues | **Not built** — we use tagged-union `RenderCommand` stream |
| Minimal EntityTable (transform, mesh, texture) | **Archived** (D-E3) |
| Transform hierarchies + fixed-tick mesh interpolation | **Not needed** for CA; cell **fade** is the visual interpolation |
| Full ECS (sparse-set sketches) | **Deferred forever-until-pain** |
| Multi-backend, render graphs, heavy batching | **Explicitly deferred** in that PDF too |

**Do not** re-introduce pass objects or entity mesh tables “because the engine PDF said so.” Token stream + drawable list is the shipped boundary.

### 2.4 Later assessments (Grok + ChatGPT on the *current* tree)

Agreement across reviews:

- Layered modular monolith is **appropriate**  
- App owns modules; token path + MockBackend are real strengths  
- Scene-as-list is correct; dead graph/EntityTable should stay gone  
- Main risks: incomplete module Start handling, Canvas dual role, partial backend neutrality, CMake duplication, docs lagging code (especially Canvas presentation)  
- Highest value: product correctness + one canonical Canvas model in docs — **not** ECS or multi-pass  

---

## 3. Overall assessment

CSim is a **coherent layered modular monolith**:

```
Platform
  → App/CellMain          (product composition)
  → Engine/Illumo         (owns services + module list)
  → Modules               (CellGameModule, DebugModule)
  → Game + Rulesets       (CA domain / rules)
  → Rendering             (Scene list, drawables, Renderer, tokens)
  → IBackend              (GLBackend | MockBackend)
```

It is an **engine-shaped application**, not a cleanly separated “engine product + game product.” That is **intentional at current scale**.

**Verdict:** Functional and reasonably testable. Residual risk is mostly **product correctness holes** and **documented debt**, not a need for ECS or a full rewrite.

---

## 4. Strengths (keep these)

| Area | Consensus |
|------|-----------|
| **App owns composition** | `CellMain`: `Init` → `addModule` → `StartModules` (D-E1). Engine does not construct Game modules. |
| **Module lifecycle** | `Start` / `Update` / `DispatchDrawables` / `Exit` is clear. |
| **Render split** | Enroll once; emit tokens per frame; backend executes (D-R1–D-R8, D-R10). |
| **Rulesets** | Strategy hierarchy; pure `nextState` + `evalCell`; double-buffered generation (D-P3). |
| **Scene model** | Per-frame drawable list only (D-E3, D-E4). |
| **Tests** | Headless `CSimTests`: rules, canvas, tokens, UI, renderer inject. |
| **Debt hygiene** | Dead experiments under `archive/` rather than half-live. |

---

## 5. Canonical architecture (code truth)

### 5.1 Packages

| Package | Role |
|---------|------|
| **App/** | Product composition (`CellMain`). |
| **Engine/** | Host: Illumo, modules, frozen `IllumoContext`. |
| **Game/** | CA domain + presentation (`Canvas`, `CellGameModule`, `CellContext`). |
| **Rulesets/** | CA rules (GoL family, Wireworld, …). |
| **Rendering/** | Scene list, drawables, Renderer, tokens, OpenGL, Mock. |
| **Services/** | Log, env, input, console UI, save/load API, allocators. |
| **Foundation/** | Macros, math aliases (`MathTypes.h`), sysinfo. |
| **Platform/** | OS entry + native dialogs. |
| **Assets/** | Asset loaders. |
| **Tests/** | Headless suite. |

House style (D-008 / CONTRIBUTING): avoid `auto`; avoid namespaces (prefer static classes/structs); no recursion; third-party via PR — unless a later decision waives.

### 5.2 Ownership and lifetime

- **Illumo** owns long-lived services with `unique_ptr` (window, renderer, camera, env, input, scene, command line, …).  
- **IllumoContext** is a **non-owning** pointer bag for modules (D-E5: frozen for the two shipped modules; validate at `Start`).  
- **App** decides which modules exist.  
- Do not grow the bag for convenience; a third module with different needs should take **explicit constructor dependencies**.  
- Prefer explicit references for domain logic over “look up everything from the bag.”

### 5.3 Module contract

```
class IModule {
  virtual void Start(IllumoContext* context) = 0;
  virtual void Update(double dt) = 0;
  virtual void DispatchDrawables(Scene* scene) = 0;
  virtual void Exit() = 0;
};
```

| Hook | Responsibility |
|------|----------------|
| Start | Bind inputs, allocate game state; bail if context incomplete |
| Update | Simulation / input |
| DispatchDrawables | Contribute what should draw this frame |
| Exit | Teardown module-owned state |

Shipped modules: `CellGameModule` (always), `DebugModule` (`#ifndef NDEBUG`).

**Known hole:** `Start` can fail without preventing later `Update` / `DispatchDrawables` → §8.

### 5.4 Frame loop (production)

```
CellMain
  → Illumo::Init()                    // services only
  → addModule(CellGameModule)
  → addModule(DebugModule)            // Debug builds
  → StartModules()
  loop:
    Update(dt)   // InputManager → Camera → modules.Update
    Render()
      scene.ClearDrawables()
      modules.DispatchDrawables(scene)
      renderer.BeginFrame()
      renderer.RenderScene(scene, camera)   // tokens, then optional immediate fallback
      renderer.EndFrame()                   // swap (GL)
```

Typical draw order from the game module: canvas → console → FPS → splash.

### 5.5 Rendering architecture (shipped)

**Mental model:** Game never issues draw `gl*` for the live path. It enrolls resources and emits **tokens**.

```
Game / Rulesets / UI
    |  (no gl* for draw submission)
    v
Drawable::AppendCommands(Renderer*)
    |  enroll handles + push tokens
    v
CommandQueue<RenderCommand>     // tagged union payloads
    |
    v
IBackend::SubmitCommandQueue
    |
    +-- GLBackend + GLDevice   (handle resolve, PBO upload, bind tracker)
    +-- MockBackend            (tests)
```

| Piece | Job |
|-------|-----|
| **Modules** | Choose what should appear this frame. |
| **Scene** | Ordered list of drawable pointers for this frame only. |
| **Drawable** | Prefer `AppendCommands(Renderer*)` → tokens. Immediate `Draw()` only if AppendCommands returns false (tests/stubs). |
| **Renderer** | Frame setup tokens; collect drawable tokens; submit; then hybrid immediate list. |
| **IBackend** | Create resources, queue, submit, begin/end frame. |
| **GLBackend / GLDevice** | Real OpenGL: handle registries, execute tokens, PBO texture updates, bind-state tracking, blend-func-on-enable (D-R5). |
| **MockBackend** | Headless: record creates + command order. |

**Enroll (rare):** `enrollMesh` / `enrollShader` / `enrollTexture` → opaque `unsigned long` table IDs.  
**Per frame:** `RenderCommand` tagged union (bind, uniform, update texture/buffer, draw, clear, viewport, pipeline).  
**Not current (old engine PDF):** separate Opaque/Transparent/UI pass *objects*, MeshDrawCommand-only world draws, entity mesh tables.

**Production pure-token drawables (D-R10):** Canvas, CommandLine, GLString, SplashText.

**Token migration phases 0–6:** complete for planned scope (proof → Scene → Canvas → UI → dead-path cleanup → Mock inject).

### 5.6 Canvas presentation model (canonical — code wins)

**Important history:** D-P4 briefly moved presentation to **GPU R8 state texture + 256×1 palette** (snap colors, drop CPU fade). Fade was later **restored**. Intermediate LaTeX notes that still describe “R8-only current” are **stale**.

**Code truth after fade restore:**

| Layer | Contents |
|-------|----------|
| **Domain** | `lifeCanvas` — dense `unsigned char[width × height]` (**not** chunked) |
| **View** | CPU palette → `targetRgb` / `displayRgb` fade; `texCanvasBuffer` RGB bytes |
| **GPU** | RGB display texture; dirty-rect `UpdateTexture` (PBO); shader samples `uDisplayTexture` |

**What survived from D-P4:** dirty-rect uploads, PBO path in `GLTexture::UpdateSubImage`, bind-state tracker, dirty flags for idle frames (D-P1).  
**What was rolled back:** exclusive R8+palette GPU path as the live presentation; dual float RGB staging + fade are back.

**Decision D-C1:** Keep Canvas as intentional domain + view + GPU enroll **monolith** until pure-sim tests or large grids force `LifeGrid` / `CanvasView`.

**Scale walls:**

- `calcGeneration` always walks the **full** grid (rect args ignored).  
- Neighbor counts: scalar Moore/toroidal.  
- Change detection may do a second full pass for dirty AABB (helps sparse **upload**, still O(W×H) sim).  
- Per cell: 1 B life + ~24 B float RGB + 3 B display tex. Default 80×60 is fine; large/infinite needs redesign.

### 5.7 Rules and encoding

**Active rules** (factory / AllSets): Game of Life, Seeds, Brian's Brain, Highlife, Day & Night, Life Without Death, **Wireworld**.  
**Stubs:** Rule 90 / 184 (identity `nextState`).

```
class RuleSet {
  virtual unsigned char nextState(unsigned char cell,
                                  unsigned char aliveNeighbors) const;
  virtual void evalCell(const unsigned char& target,
                        unsigned char dest[3]) const; // palette colors
  void calcGeneration(...); // double-buffer: nextGen then write-back
};
```

| Encoding | Values |
|----------|--------|
| Binary / life-like | `0` = alive, `1` = dead; neighbor count treats value `0` as live |
| Multi-state (e.g. Brian's Brain) | `≥2` additional states (e.g. dying = 2) |
| **Wireworld** | `0` head, `1` empty, `2` tail, `3` conductor (head = 0 reuses head-neighbor counting) |

**Generation path (D-P3):**

1. Count neighbors from raw `lifeCanvas` (toroidal).  
2. Write `nextState` into scratch `nextGen`.  
3. If any cell changed: copy back, mark sparse dirty AABB for visuals. Still life → no dirty.

Rules stay free of rendering and input. `evalCell` supplies palette/RGB colors only.

**Optional cleanup (from CA PDF, not required for correctness):** collapse life-like rules into one family + JSON birth/survive tables:

```json
{ "family": "life_like", "birth": [3], "survive": [2, 3] }
```

JSON describes **data**, not executable behavior. Multi-state (Wireworld, Brian's Brain) stay separate families.

### 5.8 Simulation vs rendering

1. Simulation advances on a **tps × speedFactor** clock (accumulator + step cap).  
2. Rendering runs every frame.  
3. Double-buffer prevents reading a half-written generation.  
4. Visual fade is **presentation**, not simulation state.  
5. Modes in `CellGameModule`: `NORMAL | EDIT | SAVE | LOAD | EXIT` (enum + switch; old State classes archived).

Fixed-tick **mesh transform interpolation** (engine PDF) is **not** required for the CA product.

### 5.9 Input

Intended flow:

```
GLFW callbacks / poll → InputManager → module / controller logic
```

**Today:** InputManager holds key/mouse state and contexts; CellGameModule / DebugModule consume it. Not every behavior is extracted into tiny controller classes—acceptable.

**D-E2:** InputManager must not depend on Game types.  
Callbacks should record events/state, not own game policy long-term (CA design PDF — still the direction of travel).

### 5.10 Window / platform boundaries

- Semantic window ops (`shouldClose`, poll, swap, title, dimensions) justify thin wrappers even if one-liners — they hide GLFW types from the main loop.  
- OpenGL calls stay under `Rendering/OpenGL/` (+ window bootstrap).  
- Interfaces only where multiple implementations or third-party volatility are real (`IBackend`, `IRenderWindow`).  
- macOS Metal myths: GLFW can create a no-client-API window and expose native handles; do not invent a Cocoa window path “because Metal.”

---

## 6. Locked decisions (catalog)

Full formal prose also lives in `docs/sections/09-design-decision-log.tex`. Append new IDs there **and** update this table.

### 6.1 Structure and style

| ID | Decision |
|----|----------|
| **D-001** | Package rename: Core→Game, System→{App,Engine,Services}, Init+Util→Foundation, AssetLoaders→Assets. |
| **D-002** | Math aliases in `MathTypes.h` (not `Math.h` — Windows CRT collision). |
| **D-004** | Modules: Start / Update / DispatchDrawables / Exit. |
| **D-005** | Each CA variant is a `RuleSet` subclass (factory in CellContext; registry later optional). |
| **D-006** | Modes = `CellState` enum in module (not archived State class hierarchy). |
| **D-008** | House style: avoid `auto`; no namespaces; no recursion; third-party via PR. |

### 6.2 Rendering (D-R\*)

| ID | Decision |
|----|----------|
| **D-003 / D-R\*** | Token submission via IBackend + RenderCommand; game must not permanently depend on raw GL for draw. |
| **D-R1** | `RenderCommand` = simple tagged union (C-style), not byte-stream compiler / `std::variant`. |
| **D-R2** | Each drawable emits tokens via `AppendCommands`; Renderer owns frame setup + submit. |
| **D-R3** | Opaque `unsigned long` handles + backend registries for v1; strong typedefs deferred. |
| **D-R4** | Migration phases 1–6 done; destroy resources at shutdown only for v1; Windows GL primary. |
| **D-R5** | On blend enable, always set `glBlendFunc` (console panel transparency). |
| **D-R6** | Archive dead render queue / SceneObject graph helpers; Scene + Renderer is the path. |
| **D-R7** | MockBackend + headless tests; no Vulkan/Metal yet. |
| **D-R8** | Inject `IBackend*` into Renderer for tests; production owns GLBackend. |
| **D-R9** | String-named uniforms = debt if a second *real* GPU API appears. |
| **D-R10** | Production drawables pure-token; hybrid `Draw()` for tests/stubs only. |
| **D-007** | Enroll resources outside the per-frame stream (frame queue = bind/draw/update). |

### 6.3 Performance (D-P\*)

| ID | Decision | Note |
|----|----------|------|
| **D-P1** | Dirty visual path — idle frames skip full recolor/upload. | Still current |
| **D-P2** | UI batch: CommandLine one packed update; GLString geometry cache. | Still current |
| **D-P3** | Double-buffer `calcGeneration` + sparse dirty AABB. | Still current |
| **D-P4** | Originally: R8 + palette + dirty-rect PBO; drop dual float RGB. | **Partially superseded:** dirty-rect PBO + bind tracker kept; **RGB fade display restored** as live presentation (see §5.6) |

### 6.4 Engine shape (D-E\*, D-C\*, D-F\*)

| ID | Decision |
|----|----------|
| **D-E1** | Module registration lives in App; Engine knows only `IModule`. |
| **D-E2** | InputManager has no Game types. |
| **D-E3** | EntityTable archived; cells are not entities. |
| **D-E4** | Scene is drawable list only (no graph). |
| **D-E5** | Freeze IllumoContext; validate at Start; third module → explicit deps. |
| **D-C1** | Canvas dual role intentional until scale forces split. |
| **D-F1** | MacroDefs / Windows.h include toxicity deferred until real pain. |

---

## 7. Early agenda vs today

| Agenda item | Status |
|-------------|--------|
| Render text / fonts | **Done** (FreeType + GLString / SplashText) |
| Command line | **Done** (commands can still grow) |
| Console on GL screen | **Done** (token UI) |
| More rulesets | **Partly** — Wireworld live; 90/184 stubs |
| Infinite 16×16 chunk canvas | **Not done** — dense grid by design for now |
| Mouse pan | **Done** (camera controls) |
| SYCL / large-grid parallel | **Not done** — serial full-grid; deferred |

**Reading of the agenda:** UI/tools first (mostly finished); **scale** (chunks + parallel) remains the open endgame — after product correctness.

---

## 8. Known product / correctness issues

From local code review / `docs/current_issues.txt` (fix when touching related code; not architecture rewrites). Architecture itself is sound for the normal paint/sim loop.

### 8.1 Bugs (high signal)

| # | Severity | Issue |
|---|----------|--------|
| 1 | bug | **Partial Start still crashes later.** If `IllumoContextHasGameCore` fails, `Start` returns without `cellContext`, but `Update` / `DispatchDrawables` still use it → null deref. Same class of hole in DebugModule. |
| 2 | bug | **`LoadCellGame` skips dirty/fade.** Writes `lifeCanvas` directly; never `markCellsDirty()`. Display can stay stale until something else dirties the grid. |
| 3 | bug | **`IllumoContextHasGameCore` omits `commandLine`**, but Edit always uses `ic->commandLine->isOpen`. Latent under product Init (always wired); real if bag incomplete. |
| 4 | bug | **Wireworld editor can’t place heads.** Paint is conductor/empty only; no head/tail tool. |
| 5 | bug | **Startup seed is always a GoL glider (value 0).** Under WIREWORLD that plants five electron heads, not a useful wire. |

### 8.2 Suggestions

| # | Issue |
|---|--------|
| 6 | Console ruleset list still duplicated vs `CellContext::IsKnownModeString` (drift risk — already hit with WIREWORLD). |
| 7 | Wireworld tests miss 2-head birth / 3-head no-birth; no load→dirty coverage. |

### 8.3 Assessment-only risks (structural)

From `gpt_csim_arch_assessment.pdf`:

- Command queue **2048** capacity with **silent drop**; raw pointer payloads must stay alive until submit  
- CMake source-list duplication (CSim vs CSimTests); empty package CMake files  
- Docs historically described **R8+palette** while code used **RGB fade** — this consensus file + §5.6 is the resolution; keep LaTeX chapters aligned  
- Renderer.h still includes GL types for production construction  
- Hybrid token + immediate Draw path remains for stubs  

### 8.4 Looks fine (do not “fix” as bugs)

- Wireworld encoding (head = 0) and head-neighbor counting  
- Fade order for paint/sim path  
- `rebuildPalette` → full dirty refresh on ruleset switch  
- SceneObject removal has no leftover live call sites  
- Pure-token production drawables  

---

## 9. Architectural debt (do when it hurts)

| Item | Notes |
|------|--------|
| String uniforms | D-R9 — GL-shaped until second real backend |
| Hybrid Draw path | Tests/stubs only; production is tokens |
| Command queue silent overflow | Prefer log/assert if near cap |
| CMake duplication | Shared object/library target later |
| Renderer.h includes GL types | Factory / pimpl later |
| Full-grid sim + float fade memory | Scale wall for large canvases |
| MacroDefs + Windows.h | D-F1 deferred |
| IllumoContext growth | Frozen; third module = explicit deps |
| Life-like JSON family collapse | Optional cleanup of repetitive RuleSet classes |
| Chunk + SYCL | Optional after serial benchmark / product need |
| Resource destroy/reload | Shutdown-only for v1 (D-R4); hot-reload later |
| Incomplete experimental stubs | e.g. leftover GLBuffer experiments |

---

## 10. Recommended work order

### A. Correctness first

1. Failed `Start` → do not run that module’s Update/Dispatch; include `commandLine` in game-core check.  
2. Load path → `markCellsDirty` + visual rebuild.  
3. Wireworld seed + head placement UX.  
4. Keep this file and LaTeX “current state” sections aligned with **RGB fade Canvas** (no stale “GPU R8 as current” claims).  

### B. Hygiene

5. Single source of truth for known ruleset names.  
6. Shared CMake target for common sources.  
7. Command-queue overflow policy (log/assert).  

### C. Only if product or learning goals require it

8. Canvas split (`LifeGrid` / `CanvasView`).  
9. Chunked / infinite canvas (agenda 16×16 map).  
10. Threaded or SYCL simulation backend behind a narrow interface (serial benchmark first).  
11. Backend factory + non-string uniforms.  
12. Data-driven life-like rule family (JSON birth/survive).  
13. Focused controllers (camera / console) if input coupling becomes painful.  

### Explicitly deferred (engine PDF + consensus)

- Full Scene/World abstractions, general ECS, cached queries  
- Render graphs, multi-backend, aggressive batching/instancing  
- Multithreaded command generation  

---

## 11. Core design principles (merged)

1. **CA learning sandbox first** — not a general engine product.  
2. **Ownership explicit** — Illumo owns services; App owns module set; context does not own.  
3. **Boundaries over cleverness** — abstract volatility (GLFW, GL, future compute); hard-code stable structure (module loop, dense grid for now).  
4. **Sim produces complete state; render observes** — double-buffer; no draw mid-generation.  
5. **Tokens for draw submission** — enroll once, emit commands, backend executes.  
6. **Parallelize data transformations last** — serial correctness first; chunks/SYCL optional.  
7. **Archive experiments** — don’t leave half-live ECS/graph/passes in the hot path.  
8. **Simplest architecture that preserves the boundaries you care about** (engine PDF principle, applied to the CA product).  
9. **Code wins over docs** — update this file when consensus shifts.  

---

## 12. Open questions (remaining)

Most design questions from the LaTeX open list are **resolved** (see §6). Still open or only lightly decided:

| Topic | Working answer |
|-------|----------------|
| Resource ownership long-term | Destroy only at shutdown for v1 (D-R4). Refcounts / generational handles later if hot-reload needs them. |
| Linux/macOS parity | Freeze until Windows token path solid — Windows path is solid; parity still optional. |
| Tracy CI policy | Debug-oriented; no strict CI policy yet. |
| Name “Illumo” permanence | Engine codename; no rename required. |
| When to introduce chunks / SYCL | After correctness + serial benchmarks, or as an explicit learning goal. |

Resolved highlights (do not re-open without a new decision ID):

- Token payload shape → D-R1  
- Handles → D-R3  
- Who emits tokens → D-R2  
- Scene graph leftovers → D-E4  
- IllumoContext growth → D-E5  
- Canvas domain vs view → D-C1  
- String uniforms → D-R9 debt  
- Canvas upload dirty rects → D-P1 / PBO path  
- Module registration → D-E1  
- InputManager Game deps → D-E2  
- EntityTable → D-E3 archived  
- CPU color fade → **restored** (RGB display)  
- Wireworld → implemented  
- MacroDefs toxicity → D-F1 deferred  

---

## 13. How to use this document

| When | Action |
|------|--------|
| New chat / new session | Read **this file first**. |
| Deep dive | Then LaTeX design notes / decision log. |
| Architecture change | Update **this file** + append a decision log entry in `09-design-decision-log.tex`. |
| Code lands | Update “code truth” sections here if behavior changed. |
| Old PDFs / agenda | Treat as **history** (§2); do not re-implement superseded engine plans by default. |
| Bug triage | §8 first; architecture is not the problem until proven otherwise. |

---

### Appendix A — Source map (where code lives)

| Concern | Typical location |
|---------|------------------|
| Frame loop / composition | `Source/App/CellMain.cpp` |
| Host / services / modules | `Source/Engine/Illumo.*` |
| CA module / modes | `Source/Game/CellGameModule.*`, `CellContext.*` |
| Grid + fade + GPU enroll | `Source/Game/Canvas.*` |
| Rules | `Source/Rulesets/*` |
| Tokens / Renderer | `Source/Rendering/Renderer.*`, `RenderCommand.*` |
| GL execute | `Source/Rendering/OpenGL/*` |
| Mock | `Source/Rendering/Mock/*` |
| Console | `Source/Services/CommandLine.*` (UI drawable may sit in Rendering) |
| Dead experiments | `archive/dead-engine/`, `archive/dead-render/`, `archive/old-states/` |

### Appendix B — What not to do next

- Do not reintroduce EntityTable / SceneObject graph “for completeness.”  
- Do not build Opaque/Transparent/UI pass objects unless profiling or a real multi-genre product appears.  
- Do not make SYCL a hard dependency of Illumo or Game.  
- Do not document R8-only presentation as current without verifying shaders + Canvas.cpp.  
- Do not expand IllumoContext casually for a third module.  

---

*End of unified consensus. Prefer this one coherent story over re-deriving from multiple ChatGPT PDFs, agendas, and chat threads.*
