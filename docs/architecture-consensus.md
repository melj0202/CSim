# Illumo — Architecture consensus (unified)

**Status:** Single living document — **authoritative for later sessions**  
**Last updated:** 2026-08-08

This file **merges and supersedes** scattered design memory into one coherent story. Read this first; treat external PDFs and old agenda notes as **history** (§2).

| Source | Role in this document |
|--------|------------------------|
| Desktop “Main agenda for Illumo” | Product wishlist → §7 scorecard |  
| `Illumo_Architecture_Decisions.pdf` (ChatGPT) | CA / OOP-practice design review → purpose, ownership, input, rules, SYCL |
| `Illumo_Engine_Architecture_Decisions.pdf` (ChatGPT) | Aspirational 2D engine → what we kept vs discarded |
| `gpt_illumo_arch_assessment.pdf` (ChatGPT) | Assessment of *current* tree → risks, direction |
| Grok architecture reviews + local code review | Strengths, bugs, debt |
| In-repo LaTeX design notes + decision log | Formal decision IDs (D-\*) |
| `docs/current-issues.md` | Product/correctness punch list |
| **Current code under `Illumo/Source/`** | **Wins** when anything conflicts |

**Rule:** If this document and the code disagree, **code wins** until this file is updated in the same change set.

Optional deeper reading (not required to resume work):

- `docs/latex/architecture-map.tex` → `docs/output/architecture-map.pdf` — landscape chart-only package/class map
- `docs/latex/illumo.tex` — design notes PDF entrypoint (prose chapters under `sections/`)
- `docs/latex/sections/09-design-decision-log.tex` — append-only formal decision prose
- `docs/sessions/2026-08-04-illumo-console-and-documentation.md` — this session's implementation record

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

**Illumo is a cellular-automata learning sandbox in a modular-monolith shell: App owns composition, Illumo owns services, modules drive sim/UI, rendering is enroll-once + token stream (OpenGL + Mock). Production state is an unbounded signed-coordinate `SparseCellGrid`; `CanvasView` presents a bounded camera region with RGB fade and one reusable texture/quad. The composition root injects `IBackend`; the production frame path is module dispatch → tokens; mode splash is module-owned. Dense `CellGrid`/`Canvas` remain compatibility fixtures only.**

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
- SYCL or a second compute backend before the sparse product path is correct and documented  
- Aggressive batching/instancing/render graphs without profiling evidence  

### 1.4 Allocators (from CA design PDF — retained policy)

Retain arena / stack / pool allocators as learning code and optional utilities. Do **not** force them into every service. Appropriate uses:

| Allocator | Type | Fit |
|-----------|------|-----|
| `ArenaAlloc` | bump + chained chunks, bulk `Clear` | Frame scratch, transient command data, parsers, load buffers |
| `ChainedStackAlloc` | bump + LIFO `Deallocate` (destructor runs) | Nested temporary lifetimes |
| `ChainedPoolAlloc<T>` | fixed-size free list, up to 4 chunks | Many same-sized objects with stable slots |
| `MallocAlloc` / `IAllocator` | thin `malloc`/`free` | Baseline interchangeable heap |

Headless coverage lives in `Illumo/Source/Tests/TestAllocators.cpp` (`Illumo.Alloc.*` CTest cases).

Live consumers:

| Site | Allocator | Role |
|------|-----------|------|
| `CommandLine` parse / complete / dispatch | `ArenaAlloc parseArena` | Token and chain staging for one command session |
| Nested alias expansion | `ChainedStackAlloc aliasExpandStack` | LIFO expanded text frames |
| `Renderer::RenderScene` | `ArenaAlloc frameArena` | Immediate-drawable pointer list per frame |
| `CellGameModule::LoadCellGame` | standard temporary vectors | Validated sparse/legacy load state |
| `SparseCellGrid` | standard authoritative hash map + retained inactive map, node handles, flat index/vector | Unbounded 16×16 chunks plus allocation-reusing generation output, separate stored/counting masks, and per-target candidate/halo selection |

A general-purpose allocator (mimalloc-class) is a different product; do not reinvent it.

---

## 2. Historical sources (how to read old notes)

Three generations of “design truth” got mixed in chat history. This section untangles them so later sessions do not re-implement superseded plans.

### 2.1 Desktop agenda (“Main agenda for Illumo”)

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

**Outcome today:** UI/tools/pan/most rulesets and the sparse chunk canvas are done. **SYCL remains deliberately deferred.** Full scorecard → §7.

### 2.2 CA design review (`Illumo_Architecture_Decisions.pdf`)

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

**Closer to current Illumo than the “engine passes” PDF.**

### 2.3 Engine design PDF (`Illumo_Engine_Architecture_Decisions.pdf`)

Aspirational **general 2D engine** design (July 2026). Valuable as boundary thinking; **not** the current product plan.

| Proposed | Status in Illumo |
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
- Main risks: incomplete module Start handling, Canvas dual role, and command-queue overflow
- Highest value: product correctness + one canonical Canvas model in docs — **not** ECS or multi-pass  

---

## 3. Overall assessment

Illumo is a **coherent layered modular monolith**:

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
| **Tests** | Headless `IllumoTests`: granular process-isolated CTest cases spanning rules, Canvas, game commands/save-load, input/services, assets, tokens, UI, and renderer injection; Clang/LLVM production-line gate (D-T1). |
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
| **Foundation/** | Macros and math aliases (`MathTypes.h`). |
| **Platform/** | OS entry + native dialogs. |
| **Assets/** | Runtime asset data. |
| **Tests/** | Headless suite. |

House style (D-008 / `docs/contributing.md`): avoid `auto`; avoid namespaces (prefer static classes/structs); no recursion; third-party via PR — unless a later decision waives.

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

Product modules: `CellGameModule` always; `DebugModule` only in Debug. Release
neither compiles nor registers `DebugModule` (D-B1).

**Known hole:** `Start` can fail without preventing later `Update` / `DispatchDrawables` → §8.

### 5.4 Frame loop (production)

```
CellMain
  → Illumo::Init()                    // services + GLBackend construction
       backend = make_unique<GLBackend>(window)
       renderer = make_unique<Renderer>(..., move(backend))
  → addModule(CellGameModule)
  → addModule(DebugModule)            // Debug builds
  → StartModules()                    // erase modules whose Start returns false
  loop:
    Update(dt)   // InputManager → Camera → modules.Update
    Render()                            // single production path (D-R13)
      scene.ClearDrawables()          // Scene = per-frame FrameRenderList (D-E4)
      modules.DispatchDrawables(scene)
      renderer.BeginFrame()
      renderer.RenderScene(scene, camera)   // tokens, then optional immediate fallback
      renderer.EndFrame()                   // swap (GL)
```

There is **no** env-gated alternate product frame path. `Renderer::RenderProofQuad`
remains for headless token e2e tests only.

Startup options are parsed during `Illumo::Init` after persisted environment
values and defaults are loaded. Supplied window and canvas dimensions override
those values before the render window is constructed; `--help` and `--version`
terminate after printing their output.

Typical combined draw order (Scene layers, one main pass): World canvas;
UI splash + console + editor cursor; Debug FPS (Debug builds via DebugModule).

`RenderWindow` defaults to swap interval one. The persisted `vsync` environment
value can select synchronized or uncapped presentation and is reapplied only
when it changes, so Debug's generic `toggle vsync` command works live. The FPS
overlay labels synchronized swap-completion cadence as `Paced FPS` and reports
main-loop submissions separately; uncapped mode reports paced FPS as off rather
than presenting CPU submissions as monitor output (D-P32).

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
| **Modules** | Choose what should appear this frame; place drawables into Scene layers. |
| **Scene** | Per-frame non-owning drawable pointers in ordered layers (World → UI → Debug). One main pass. |
| **RenderStyle** | Built-in style table on `Renderer`: shader handle + `PipelineState` defaults (Canvas, UiText, Console, Shape, Sprite). |
| **Primitives / GameVisual** | Value-type shapes/sprites/text on a `GameVisual` host (D-R15). **Canvas**, **CommandLine**, **GLString**/SplashText, and **Cursor** embed/compose via `GameVisual`. |
| **Drawable** | Content handles; `bindStyle` then content tokens via `AppendCommands`. Immediate `Draw()` only if AppendCommands returns false (tests/stubs). |
| **Renderer** | Backend-neutral: owns style table; frame setup; walk layers; submit. Depends only on `IBackend*` (D-R11). |
| **IBackend** | Create resources, explicitly destroy textures, queue, submit, begin/end frame. GPU objects live in backend registries. |
| **Composition (Illumo::Init)** | Constructs the production backend via `CreateOpenGLBackend` and injects it into `Renderer` with `takeOwnership=true`; calls `ensureBuiltinStyles()`. |
| **GLBackend / GLDevice** | Real OpenGL under `Rendering/OpenGL/`: handle registries, execute tokens, PBO texture updates, bind-state tracking, blend-func-on-enable (D-R5). |
| **MockBackend** | Headless under `Rendering/Mock/`: record creates + command order; injected for tests. |

**Layers vs passes (Phase A):** layers are composition buckets on the default framebuffer (single clear). They are not multi-target GPU render passes. A future pass object would own FBO/load-store when targets diverge.

**Enroll (rare):** `enrollMesh` / `enrollShader` / `enrollTexture` → opaque `unsigned long` table IDs; built-in styles enroll their shaders once via `ensureBuiltinStyles`. Re-enrolling a texture handle replaces and destroys its prior GPU object; `releaseTexture` removes a texture before backend shutdown.
**Per frame:** `RenderCommand` tagged union (bind, uniform, update texture/buffer, draw, clear, viewport, pipeline).  
**Not current (old engine PDF):** separate Opaque/Transparent/UI pass *objects*, MeshDrawCommand-only world draws, entity mesh tables.

**Production pure-token drawables (D-R10):** Canvas, CommandLine, GLString, SplashText.

**Token migration phases 0–6:** complete for planned scope (proof → Scene → Canvas → UI → dead-path cleanup → Mock inject).

### 5.6 Sparse domain + bounded view (canonical — code wins)

The live path is intentionally a full replacement of the finite dense runtime:

| Layer | Type | Contents |
|-------|------|----------|
| **Domain** | `SparseCellGrid` | signed 64-bit cells in a hash map of non-background 16×16 chunks |
| **Simulation** | `SimulationRunner` + two `SparseCellGrid`s | one worker generation may be in flight; the main thread reads only the published grid and publishes completions at frame boundaries. Incremental completions apply `SparseGenerationDelta` before the former display grid is reused. Broad completions carry a lightweight replacement marker: the spare advances directly from the immutable published grid and updates its own authoritative nodes in place, avoiding a full snapshot and mirror pass. Overdue whole steps are dropped without a backlog, and edits, persistence, ruleset changes, manual stepping, and shutdown drain first. `SparseCellGrid::advance` retains the transactional totals/frontier/candidate/halo/node-reuse paths described below. |
| **View** | `CanvasView` | visible dimensions remain diagnostics while sampling uses a globally aligned cache padded by two chunks per side; motion inside it changes only the MVP. Near LOD is one texel/cell; far LOD is an integer density level with immediate coarsening and 80% refinement hysteresis. One-revision changed chunks map to deduplicated exact or overview cache bins; cache exit, revision gaps, resize, palette change, and replacement refill the bounded cache. Active-texel CPU RGB fades snap newly revealed cells. |
| **GPU** | `CanvasView` + `GameVisual` | one reusable nearest-filtered RGB staging texture and one world-space quad; dirty 16×16-texel tiles merge into at most eight update rectangles or their AABB. Uploads through 64 KiB are direct; larger requests use a non-waiting three-PBO/fence ring with direct fallback. |

Rulesets provide stateless `nextState` and `evalCell` functions. Each ruleset's
complete 256-state by 9-neighbor transition table is built once before worker
dispatch and indexed by sparse and compatibility dense loops. Production does
not depend on `CellGrid*`; the old dense `CellGrid`/`Canvas` and their
toroidal `calcGeneration` entry remain only for compatibility tests during the
transition. The sparse map has no fixed chunk-count cap and preserves all
multi-state byte values.

Negative chunk coordinates use centralized floor division/modulo. Chunk output
is sorted by `(chunkY, chunkX)` for deterministic saves and tests. The view
visits only chunks intersecting its cache bounds. The cache is globally aligned
and extends two 16-cell chunks beyond the visible viewport on every side, so
sub-cache pan/zoom changes only the camera MVP. The grid publishes a
revision-scoped changed-chunk list for edits and completed generations. When
exactly one revision is unseen, the view maps changed chunks to deduplicated
exact-cell or overview bins. Revision gaps, cache exit, palette changes,
whole-grid replacement, LOD changes, and resize use a complete bounded refill.
Far LOD coarsens immediately to fit and refines only at 80% budget occupancy.
A retained active-texel set
makes fade ticks and zero-speed snaps proportional to colors still changing;
reapplying an unchanged zero fade speed does no scan. At far zoom it limits
the active texture to `max(CanvasX/Y, window / 4)` texels and accumulates
palette density into each overview texel. This presentation budget neither
caps chunks nor discards simulation cells. Rendering never creates per-chunk
GPU resources: only the bounded view texture and quad enter the token stream.
For small required dimension increases, the CPU and GPU texture allocation
reserves 50% headroom; nearby smooth-zoom sizes reuse it. Dirty 16x16-texel
tiles merge horizontally and vertically into at most eight rectangles when
their combined area is at most half the enclosing AABB, otherwise the AABB is
used. `GLTexture` uploads rectangles through 64 KiB directly; larger requests
select a signaled slot from a three-PBO ring without waiting and fall back to
direct upload when all slots are busy or mapping fails. A replacement keeps the
same opaque handle while `GLBackend` destroys the old texture, PBOs, and fences,
and `CanvasView` explicitly releases the handle during destruction.

Sparse stepping keeps result installation serial. Each chunk carries separate
compact masks for stored non-background cells and cells that contribute to
neighbor counts, plus cached popcounts for both masks. The authoritative and
inactive maps each maintain aggregate stored-cell, counted-cell, and
candidate-preferred-chunk totals. Edits, assignment, clear, local patches, and
complete map swaps update those totals in the same transaction as the chunk
data. `advance` therefore reports population statistics and checks whether any
chunk prefers candidate evaluation without scanning every allocated chunk. If
any source chunk has fewer than 48 counted cells, the grid
creates exact affected targets through a retained power-of-two open-addressed
index. Generation stamps make old slots logically empty without clearing the
table, and each slot points into contiguous scratch containing a 256-bit
candidate mask plus 256 neighbor counts. Target discovery checks the four edge
masks and corner bits directly. Candidate counters are initialized when a bit
is first enrolled, avoiding a 256-byte clear for every target. The direct
serial path keeps source-centric accumulation for locality. During large or
forced parallel preparation, discovery records each target's direct 3x3 source
references beside its neighbor counts. Workers claim retained coarse target
ranges and count from those references without repeated chunk-map lookups. For
direct dual-grid generations, an object-local topology epoch changes only when
chunk presence or counted edge/corner participation changes. When the same
source grid returns with the same epoch, its target index and source references
are reused exactly; only candidate masks and counts are rebuilt. An
index-aligned destination pointer also avoids output-map lookup and stale-node
scanning when the destination topology is unchanged. In-place output preserves
the node addresses that make this reuse safe. Candidate-index lookup probes for an existing target before considering
growth; only a new insertion that would cross the 75% load boundary resizes the
retained flat table. Each target independently uses those
candidates when its counted-neighbor contributions are below the calibrated
threshold, or a direct 18×18 halo otherwise. Thus mixed dense/sparse worlds do
not inherit one global decision, and dense Wireworld conductors remain candidate
work because only heads contribute neighbor counts. Worlds whose source chunks
are all densely counted bypass scratch construction. Both capacities are
retained, so a stable-width colony neither reallocates candidate records nor
sorts or binary-searches chunk addresses. Halo/core evaluation uses up to eight
reusable workers once there are at least 32 targets. Complete target discovery
uses a retained generation-stamped flat index and retained address/result
vectors rather than a generation-local hash set, target sort, and disposable
buffers. Evaluation extracts each 18-bit counted row directly from the 3x3
chunks' counting masks and reduces it through three rolling horizontal rows;
no 18x18 byte halo is materialized and cell states are read only for the target
core. Halo targets may use an exact, on-demand memo keyed by all 324 states in
that 18x18 neighborhood. The bounded four-way cache is sharded by the main and
worker slots, so evaluation needs no locks; a hash selects candidates but a
full key comparison is required for a hit. It samples 16 targets per shard,
activates at a 25% hit rate, and cools down for 32 generations after
unprofitable sampling or three active generations below 10%. Ruleset type or
transition-table revision changes invalidate every entry. Candidate-only and
sub-32-target generations bypass it. Coarse mixed/candidate
evaluation uses up to four automatic workers once there are at least 16,384
work cells. Preparation uses about eight retained ranges per worker, capped at
256 target chunks per range; evaluation ranges contain roughly 2,048 work cells.
Both avoid one atomic claim per mostly empty target chunk. All-candidate small worlds keep
the original direct serial output loop. Both
evaluators build transactionally into a retained inactive map. Nodes from the
prior inactive generation are extracted into a retained handle vector after its
aggregate statistics reset once. Sparse candidate output acquires and rekeys a
node first, then constructs the result directly in mapped storage; dense halo
output retains its complete-array bulk-copy path. Target-sized bucket headroom
is preserved because exact output reservation measured slower. Allocation occurs
only when output exceeds the previous node high-water count. A persistent `SimulationRunner` advances
the spare grid while the main thread reads the published grid. Completion
publishes only at a frame boundary with a sparse generation delta; the former
display grid consumes that delta before becoming the next worker grid. Only one
generation may be outstanding and no backlog is formed. Excess whole-step debt
is dropped while fractional remainder is retained. Pause/edit, persistence,
ruleset changes, manual stepping, and shutdown drain first. Status computes
achieved TPS from published completions and retains rolling 256-sample p50/p95/
max values for worker generations and their mirror/advance/capture stages,
cache refills, requested upload bytes, and upload rectangles. Full sparse
replacements publish only a marker; `advanceFrom` reads the published map and
writes the spare grid's map directly, so no replacement records are captured or
mirrored. Incremental catch-up uses the prior grid's own
changed-address journal and a retained incoming-address index, so an incoming
record overwrites its final state once instead of first replaying an obsolete
intermediate snapshot.

The inactive map also preserves the prior generation for changed-region
stepping. Edits and committed generations record exact per-chunk state-change
and counting-change bitmasks in retained generation-stamped flat sets. On the
next step, each changed chunk enrolls itself; a neighboring chunk is enrolled
only when a counting-status change touches the shared edge or corner. A
state-only change cannot affect a neighbor's transition. Sparse local sources build
exact candidate masks only for those targets and choose candidates or halos from
counted-neighbor contribution work. The grid compares target/source bookkeeping,
candidate construction, neighbor contributions, and evaluation cells with a
complete-path estimate derived from cached population totals. It patches the
retained prior map only when the frontier estimate is no greater; broad changes
use the complete adaptive path. An empty frontier returns without visiting any
chunk. Tracking retains at most 4,096 changed chunks, bounding bookkeeping
without reintroducing the former 64-target selection cliff. Ruleset type changes
invalidate current chunks before the next step.

### 5.7 Rules and encoding

**Active rules** (factory / AllSets): Game of Life, Seeds, Brian's Brain, Highlife, Day & Night, Life Without Death, **Wireworld**.  
**Stubs:** Rule 90 / 184 (identity `nextState`).

```
class RuleSet {
  virtual unsigned char nextState(unsigned char cell,
                                  unsigned char aliveNeighbors) const;
  virtual void evalCell(const unsigned char& target,
                        unsigned char dest[3]) const; // palette colors
};
```

| Encoding | Values |
|----------|--------|
| Binary / life-like | `0` = alive, `1` = dead; neighbor count treats value `0` as live |
| Multi-state (e.g. Brian's Brain) | `≥2` additional states (e.g. dying = 2) |
| **Wireworld** | `0` head, `1` empty, `2` tail, `3` conductor (head = 0 reuses head-neighbor counting) |

**Generation path:**

1. Read the cached stored/counting totals and candidate-preferred chunk count.
   If the retained changed frontier is empty, return immediately without
   visiting allocated chunks. Otherwise enroll each changed chunk and only the
   neighbors touched by its exact counting-change edge/corner bits. For
   sparse local sources, build target-local candidate masks and select candidate
   or halo evaluation independently.
2. Compare exact frontier preparation/evaluation work units with a complete-path
   estimate derived from cached totals. Evaluate and patch the frontier when it
   is no more expensive; otherwise use the complete path. Retain at most 4,096
   changed addresses between generations.
3. Use the cached candidate-preferred count to select the complete
   adaptive or all-dense path.
4. If any source chunk is counting-sparse, derive affected target addresses
   from source counting-mask edges/corners and insert them into the retained
   generation-stamped flat index. Initialize neighbor counts only as candidate
   bits are enrolled. Build serial scratch source-by-source, or prepare large
   and explicitly parallel workloads target-by-target through the reusable
   pool. Select candidate or direct 18×18 halo evaluation separately
   for each target from its actual counted-neighbor contribution work. Large
   mixed work sets use coarse work-count ranges in the worker pool.
5. If every source chunk is counting-dense, bypass candidate scratch. Build the
   expanded target set in its retained generation-stamped flat index, retain
   address/result vector capacity, and evaluate serially or through the bounded
   pool for 32+ targets. Construct rolling neighbor rows directly from chunk
   counting masks without a temporary 18×18 byte halo.
6. For at least 32 halo targets, sample an exact sharded 18×18-state memo. Use
   it only after the observed hit rate pays for key construction, and bypass or
   cool it down on unique neighborhoods. Invalidate on transition changes.
7. Build the complete next map serially using retained buckets and recycled
   chunk nodes. Construct sparse candidate output directly in mapped node
   storage; keep dense halo output as a bulk array copy. Compare to the
   authoritative map, then swap transactionally only when contents differ.
   Empty space is implicit, so births never wrap.

Before evaluation, the active ruleset lazily materializes all 2,304 transition
results. Hot loops share that immutable table, eliminating virtual dispatch and
repeated rule branches after the first use. Rules stay free of rendering and
input. `evalCell` supplies palette/RGB colors only.

**Optional cleanup (from CA PDF, not required for correctness):** collapse life-like rules into one family + JSON birth/survive tables:

```json
{ "family": "life_like", "birth": [3], "survive": [2, 3] }
```

JSON describes **data**, not executable behavior. Multi-state (Wireworld, Brian's Brain) stay separate families.

### 5.8 Simulation vs rendering

1. Simulation advances on a **tps × speedFactor** clock. One due generation is
   guaranteed; a measured 4 ms slice gates the optional second generation.
2. Rendering runs every frame.  
3. Double-buffer prevents reading a half-written generation.  
4. Visual fade is **presentation**, not simulation state.  
5. Modes in `CellGameModule`: `NORMAL | EDIT | EXIT` (enum + switch; save/load are registered commands, not frame states).

Fixed-tick **mesh transform interpolation** (engine PDF) is **not** required for the CA product.

### 5.9 Input

Intended flow:

```
GLFW callbacks / poll → InputManager → module / controller logic
```

**Today:** InputManager holds key/mouse state and contexts; CellGameModule / DebugModule consume it. Not every behavior is extracted into tiny controller classes—acceptable.

**D-E2:** InputManager must not depend on Game types.  
Callbacks should record events/state, not own game policy long-term (CA design PDF — still the direction of travel).

### 5.10 Developer console

The Debug-only console separates general tooling from product behavior:

- `CommandLine` owns help, environment-variable inspection/editing, validated
  finite timing/display settings, console history, alias macro management, multi-command chaining, and application exit.
- `CellGameModule` registers simulation, canvas, camera, ruleset, and save/load
  commands through `CommandRegistry`; registry metadata drives help and Tab
  completion.
- Registered commands execute from the queue without falling through as unknown.
- Save always writes version 2 sparse records (magic/version, ruleset, camera,
  deterministic sorted chunks). Load validates into temporary state, accepts
  both version 2 and the prior dense format, imports legacy cells centered at
  the origin, then restores ruleset/camera and rebuilds the bounded view.
- `vid_restart` is not advertised: safely recreating an OpenGL context requires a
  complete resource re-enrollment design, so the old no-op now reports that limit.
- Editing supports measured caret placement, selection, Home/End, Delete,
  Ctrl+word movement/deletion, Ctrl+A, quoted arguments, history, and horizontal
  input scrolling. The caret is rendered as a glowing dual-layer geometry bar at the measured insertion point.
- Multi-command chaining splits on `;` (preserving quotes and escape sequences).
- Alias macro management (`alias`, `unalias`) expands user-defined command shortcuts (with recursion capped at depth 8) and integrates aliases into auto-completion.
- Inline ghost-text auto-suggestions display faint completion candidates after the caret; pressing Right-Arrow or Tab accepts the ghost text.
- Window mode supports switching between top-mounted and floating modes (via `console_mode [floating|mounted|toggle]` or double-clicking the console title bar). In floating mode, title-bar dragging repositions the window across the screen, and dragging the bottom-right corner grip handle (or running `console_size <W> <H> | reset`) dynamically resizes the console window with real-time UI bounds clipping (D-UI3).
- Dynamic parameter syntax hints dynamically render usage instructions in the status bar while typing known commands.
- Utility commands include `repeat <N> <command>`, `history [filter|clear]`, and the `sysinfo` telemetry dashboard. The simulation-provided `status` command reports simulation, canvas, ruleset, and camera state.
- Console chrome is a refined tactical glass HUD: multi-layer drop shadow and outer neon halo, bezel inset panel, title badge + status chip header, recessed history well with sparse scanlines and a slow beam sweep, elevated command dock with prompt chip, breathing laser caret, gradient scrollbar thumb, and single-batch quad rendering (8,000 UI quads, heap-backed). History output word-wraps to the panel width and scrolls by visual lines so long help text remains fully readable when paging to the oldest entries; PageUp/wheel/scrollbar limits share the same floating-aware layout metrics as the draw path.
- `Logger` may mirror output into the console while services are alive. The host
  clears that non-owning logger context before destroying the services.

### 5.11 Window / platform boundaries

- Semantic window ops (`shouldClose`, poll, swap, title, dimensions) justify thin wrappers even if one-liners — they hide GLFW types from the main loop.  
- OpenGL calls stay under `Rendering/OpenGL/` (+ window bootstrap).  
- Interfaces only where multiple implementations or third-party volatility are real (`IBackend`, `IRenderWindow`).  
- macOS Metal myths: GLFW can create a no-client-API window and expose native handles; do not invent a Cocoa window path “because Metal.”
- Fullscreen transitions preserve the windowed position and dimensions, enter
  the primary monitor at its current video mode, and restore the saved windowed
  bounds on exit.

---

## 6. Locked decisions (catalog)

Full formal prose also lives in `docs/latex/sections/09-design-decision-log.tex`. Append new IDs there **and** update this table.

### 6.1 Structure and style

| ID | Decision |
|----|----------|
| **D-001** | Package rename: Core→Game, System→{App,Engine,Services}, Init+Util→Foundation, AssetLoaders→Assets. |
| **D-002** | Math aliases in `MathTypes.h` (not `Math.h` — Windows CRT collision). |
| **D-004** | Modules: Start / Update / DispatchDrawables / Exit. |
| **D-005** | Each CA variant is a `RuleSet` subclass (factory in CellContext; registry later optional). |
| **D-006** | Modes = `CellState` enum in module (not archived State class hierarchy). |
| **D-008** | House style: avoid `auto`; no namespaces; no recursion; third-party via PR. |
| **D-N1** | Adopt Illumo as the project, product, CMake-target, runtime, and save-format name; keep the existing `Illumo` host class. |
| **D-B1** | `DebugModule` is Debug-build product composition only; Release must neither compile nor register it. |
| **D-CLI1** | Services own generic console mechanics; `CellGameModule` registers domain commands and help/completion metadata. |
| **D-UI1** | Console editing and caret placement use measured text geometry; one enlarged batch must fit a full help page. |
| **D-UI2** | Console history wraps and scrolls by visual lines using shared mounted/floating layout metrics. |
| **D-UI3** | Console can be mounted or floating; floating mode supports title-bar drag and corner resize. |
| **D-DOC1** | All first-party project documentation lives under `docs/`, with one current LaTeX entrypoint. |
| **D-T1** | One exact CTest entry per logical behavior; shared filtered runner only for compile efficiency; Clang/LLVM `IllumoCoverage` enforces at least 85% headless-testable production line coverage. |

### 6.2 Rendering (D-R\*)

| ID | Decision |
|----|----------|
| **D-003 / D-R\*** | Token submission via IBackend + RenderCommand; game must not permanently depend on raw GL for draw. |
| **D-R1** | `RenderCommand` = simple tagged union (C-style), not byte-stream compiler / `std::variant`. |
| **D-R2** | Each drawable emits tokens via `AppendCommands`; Renderer owns frame setup + submit. |
| **D-R3** | Opaque `unsigned long` handles + backend registries for v1; strong typedefs deferred. |
| **D-R4** | Migration phases 1–6 done; shutdown remains the default bulk-destruction point, while D-P23 explicitly replaces/releases the resizeable canvas texture; Windows GL primary. |
| **D-R5** | On blend enable, always set `glBlendFunc` (console panel transparency). |
| **D-R6** | Archive dead render queue / SceneObject graph helpers; Scene + Renderer is the path. |
| **D-R7** | MockBackend + headless tests; no Vulkan/Metal yet. |
| **D-R8** | Inject `IBackend*` into Renderer always; production composition owns backend via `CreateOpenGLBackend` + ownership transfer; tests inject MockBackend. |
| **D-R9** | String-named uniforms = debt if a second *real* GPU API appears. |
| **D-R10** | Production drawables pure-token; hybrid `Draw()` for tests/stubs only. |
| **D-R11** | Construct concrete backend at composition root (`Illumo::Init` / `CreateOpenGLBackend`); `Renderer` never includes OpenGL types. |
| **D-R12** | CommandQueue fixed 2048 capacity; overflow drops + logs once per frame. |
| **D-R13** | Single production frame path in `Illumo::Render`; `RenderProofQuad` is test-only. |
| **D-R14** | Scene layers (World/UI/Debug) + Renderer-owned built-in `RenderStyle` table. One main pass; layers ≠ GPU render passes. |
| **D-R15** | Render primitives (`Shape`/`Sprite`/`Text`) composed on a `GameVisual` host; product drawables embed/compose via GameVisual. |
| **D-007** | Enroll resources outside the per-frame stream (frame queue = bind/draw/update). |
| **D-WW1** | Wireworld: ruleset-aware seed + sticky head/tail/conductor brush keys. |
| **D-C2** | `CellGrid` domain + `Canvas` presentation; rulesets depend only on `CellGrid`. |
| **D-C3** | Replace the finite production path with signed-coordinate `SparseCellGrid` chunks plus bounded `CanvasView`; retain dense types only as compatibility fixtures. |
| **D-C4** | `CanvasView` is a nearest-filtered, world-space quad with exact cell texels at normal zoom and cursor-aligned world-cell editing. |
| **D-C5** | At far zoom, `CanvasView` uses a revision-gated density overview capped at roughly four screen pixels per texel; this visual budget does not cap sparse simulation chunks. |

### 6.3 Performance (D-P\*)

| ID | Decision | Note |
|----|----------|------|
| **D-P1** | Dirty visual path — idle frames skip full recolor/upload. | Still current |
| **D-P2** | UI batch: CommandLine one packed update; GLString geometry cache. | Still current |
| **D-P3** | Double-buffer `calcGeneration` + sparse dirty AABB. | Still current; refined by D-P5 |
| **D-P4** | Originally: R8 + palette + dirty-rect PBO; drop dual float RGB. | **Partially superseded:** dirty-rect PBO + bind tracker kept; **RGB fade display restored** as live presentation (see §5.6) |
| **D-P5** | Single-pass dirty AABB + `CellGrid` front/back swap (no full memcpy). | 2026-08-06 |
| **D-P6** | Fade loop hoists + packed dirty-rect PBO staging (keep CPU RGB fade). | 2026-08-06 |
| **D-P7** | Optional row-parallel `calcGeneration` (≥512² auto, override for tests). | 2026-08-06 |
| **D-P8** | Reusable bounded worker pool for large sparse target sets; normal mode drops excess catch-up debt. | Worker pool current; scheduling refined by D-P26 |
| **D-P9** | Occupancy-masked cell candidates for sparse chunks; full chunk halos remain the dense fallback. | 2026-08-08 |
| **D-P10** | Replace per-world-cell candidate hash nodes with retained contiguous per-chunk masks and neighbor counts. | 2026-08-08 |
| **D-P11** | Replace candidate-address sorting and binary searches with a retained generation-stamped open-addressed flat index. | 2026-08-08 |
| **D-P12** | Reuse next-generation chunk-map buckets and nodes transactionally through a retained inactive map and node handles. | 2026-08-08 |
| **D-P13** | Parallelize large candidate evaluation with retained coarse ranges balanced by candidate-cell count. | 2026-08-08 |
| **D-P14** | Retain changed chunks and patch only their neighbor frontier; settled worlds perform no simulation evaluation. | 2026-08-08 |
| **D-P15** | Track stored and neighbor-counting masks separately and select candidate versus halo work independently per target chunk. | 2026-08-08 |
| **D-P16** | Cache all 256x9 rule transitions and reduce dense halos with a rolling three-row neighbor stencil. | 2026-08-09 |
| **D-P17** | Maintain transactional chunk population aggregates so settled stepping and complete-path selection do not scan all allocated chunks. | 2026-08-09 |
| **D-P18** | Replace the 64-target frontier cutoff with bounded adaptive work comparison and per-target candidate/halo frontier evaluation. | 2026-08-09 |
| **D-P19** | Derive candidate boundaries from masks, initialize counters lazily, and use target-owned worker preparation for large workloads. | 2026-08-09 |
| **D-P20** | Retain complete-halo targets/index/results and build rolling neighbor rows directly from chunk counting masks. | 2026-08-09 |
| **D-P21** | Publish revision-scoped changed chunks for dirty-tile sampling and retain active fading texels. | Refined for every LOD by D-P24 |
| **D-P22** | Gate the optional second synchronous generation by measured frame cost and report requested versus achieved TPS. | Superseded by D-P26 |
| **D-P23** | Grow canvas texture capacity geometrically and destroy replaced/released GL textures and PBOs. | Refined by D-P25 |
| **D-P24** | Separate the visible viewport from a padded aligned camera cache with stable integer LOD and incremental overview bins. | 2026-08-09 |
| **D-P25** | Merge dirty texel tiles into bounded rectangles and use direct small uploads plus a non-waiting three-PBO/fence ring. | 2026-08-09 |
| **D-P26** | Publish one evidence-gated worker generation from dual sparse grids at frame boundaries with no backlog. | 2026-08-09 |
| **D-P27** | Retain exact state/counting change masks and enroll neighboring frontier targets only at affected edges or corners. | 2026-08-11 |
| **D-P28** | Use bounded, sharded, evidence-adaptive exact memoization for repeated 18×18 halo states. | 2026-08-11 |
| **D-P29** | Eliminate duplicate inactive-delta snapshots, skip superseded incremental mirror writes, and rebuild broad mirrors with recycled chunk nodes. | 2026-08-11 |
| **D-P30** | Link candidate targets to source chunks during discovery and prepare them through retained coarse ranges. | 2026-08-11 |
| **D-P31** | Probe duplicate candidate enrollments before growth checks and construct sparse results directly in recycled nodes. | 2026-08-16 |
| **D-P32** | Default to configurable synchronized presentation and distinguish paced swap cadence from CPU submissions. | 2026-08-16 |

### 6.4 Engine shape (D-E\*, D-C\*, D-F\*)

| ID | Decision |
|----|----------|
| **D-E1** | Module registration lives in App; Engine knows only `IModule`. |
| **D-E2** | InputManager has no Game types. |
| **D-E3** | EntityTable archived; cells are not entities. |
| **D-E4** | Scene is drawable list only (no graph). |
| **D-E5** | Freeze IllumoContext; validate at Start; third module → explicit deps. |
| **D-C1** | Canvas dual role intentional until scale forces split. |
| **D-C2** | **Refines D-C1:** extract `CellGrid` domain; `Canvas` extends it for view/GPU. |
| **D-F1** | MacroDefs / Windows.h include toxicity deferred until real pain. |

---

## 7. Early agenda vs today

| Agenda item | Status |
|-------------|--------|
| Render text / fonts | **Done** (FreeType + GLString / SplashText) |
| Command line | **Done** (validated built-ins plus module-registered simulation, canvas, camera, ruleset, and file commands) |
| Console on GL screen | **Done** (token UI, advanced editing, measured caret, scrolling, full-help capacity test) |
| More rulesets | **Partly** — Wireworld live; 90/184 stubs |
| Infinite 16×16 chunk canvas | **Done** — sparse signed-coordinate chunks with separate stored/counting masks, per-target candidates or dense 18×18 halos, bounded view, editing, camera, persistence, and tests |
| Mouse pan | **Done** (camera controls) |
| SYCL / large-grid parallel | **Not done** — serial full-grid; deferred |

**Reading of the agenda:** UI/tools first (mostly finished); **scale** (chunks + parallel) remains the open endgame — after product correctness.

---

## 8. Known product / correctness issues

From local code review / `docs/current-issues.md` (fix when touching related code; not architecture rewrites). Architecture itself is sound for the normal paint/sim loop.

### 8.1 Bugs (high signal)

| # | Severity | Issue |
|---|----------|--------|
| 1 | ~~bug~~ | **Resolved 2026-08-06:** `StartModules` erases modules that fail `Start`; `CellGameModule` / `DebugModule` also early-return from `Update` / `DispatchDrawables` when core state is missing. |
| 2 | ~~bug~~ | **Resolved 2026-08-06:** Wireworld left-paint uses a sticky brush selected with `1`/`H` (head), `2` (empty), `3`/`T` (tail), `4` (conductor); right-click still clears to empty. |
| 3 | ~~bug~~ | **Resolved 2026-08-06:** Startup seed is ruleset-aware — GoL-family glider for binary rules; Wireworld plants a horizontal conductor with a head+tail electron. |

### 8.2 Closed test gaps

Wireworld now has explicit two-head birth and three-head no-birth truth-table
coverage. File-backed save/load tests cover round trips, ruleset restoration,
dimension overlap, missing/truncated/invalid files, extension fallback, and
dialog cancellation. Native dialog UI still needs a platform smoke test.

### 8.3 Assessment-only risks (structural)

From `gpt_illumo_arch_assessment.pdf` and later boundary-consolidation work:

- Command queue **2048** capacity: overflow now **logs once per frame** and tracks drop counts (D-R12); raw pointer payloads must still stay alive until submit  
- CMake source/config duplication was resolved on 2026-08-02 with shared source lists and a common target-configuration helper; the repository root forwards to the live `Illumo/CMakeLists.txt` entrypoint
- Runtime configuration is one `envvars.json` beside the executable; CMake seeds
  it from the tracked `Illumo/envvars.json` only when absent, so launch working
  directories cannot select or overwrite a different configuration.
- Docs historically described **R8+palette** while code used **RGB fade** — this consensus file + §5.6 is the resolution; keep LaTeX chapters aligned  
- ~~Renderer.h constructed GLBackend~~ — **resolved D-R11:** composition root injects `IBackend` via `CreateOpenGLBackend`; `Renderer.h` no longer includes OpenGL types  
- Hybrid token + immediate Draw path remains for stubs  
- Native file dialogs and live OpenGL/fullscreen behavior still require manual
  smoke tests; headless MockBackend coverage does not prove them.
- Deferred boundary work (do when it hurts): further Canvas→CanvasRenderer
  extraction, capability-oriented module contexts instead of the frozen bag
  (D-E5), rename `Scene` → `FrameRenderList` only if the name causes real
  confusion, multi-library CMake split, logger/SaveLoad global removal.

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
| Command queue overflow | **D-R12:** log once per frame + drop counters; capacity still fixed at 2048 |
| CMake duplication | Resolved 2026-08-02 with shared source lists/settings; no forced package libraries |
| Renderer ↔ backend | **D-R11:** `CreateOpenGLBackend` at composition; Renderer is `IBackend*`-only; Mock inject for tests |
| Sparse sim + bounded view memory | Sparse simulation scales with stored and counted cells; mixed targets independently use candidates or halos, dense counted chunks use at most eight reusable workers, and presentation scales with the configured visible view |
| MacroDefs + Windows.h | D-F1 deferred |
| IllumoContext growth | Frozen; third module = explicit deps |
| Life-like JSON family collapse | Optional cleanup of repetitive RuleSet classes |
| GPU/SYCL acceleration | Optional after bounded CPU parallel benchmark / product need; CPU sparse stepping is the production baseline |
| Resource destroy/reload | Shutdown-only for v1 (D-R4); hot-reload later |

---

## 10. Recommended work order

### A. Correctness first

1. Failed `Start` → do not run that module’s Update/Dispatch — **done 2026-08-06**.
2. Wireworld seed + mouse head-placement UX — **done 2026-08-06**.
3. Keep this file and LaTeX “current state” sections aligned with **RGB fade CanvasView** (no stale dense-production or GPU-R8 claims).

### B. Hygiene / boundary consolidation

4. CMake source/config consolidation — **completed 2026-08-02**.
5. Command-queue overflow policy (log once per frame) — **D-R12, done 2026-08-06**.
6. Backend injection at composition root — **D-R11, done 2026-08-06**.

### C. Boundary consolidation (2026-08-07 arc — done)

7. `SparseCellGrid` domain + bounded `CanvasView` production migration — **D-C3**.
8. Single production frame path; remove `UseTokenProof` product bypass — **D-R13**.
9. Mode splash module ownership (no file-scope `stateSplash` global).

### D. Only if product or learning goals require it

10. Further renderer slimming or GPU sparse stepping if profiling requires it.
11. Bounded CPU chunk parallelism is production; GPU simulation remains optional.
12. Non-string uniforms / second real backend (OpenGL factory already at composition).
13. Data-driven life-like rule family (JSON birth/survive).
14. Focused controllers (camera / console) if input coupling becomes painful.
15. Narrow `IllumoContext` into capability bags only when a third module needs different deps (D-E5).

### Explicitly deferred (engine PDF + consensus)

- Full Scene/World abstractions, general ECS, cached queries  
- Render graphs, multi-backend, aggressive batching/instancing  
- Multithreaded command generation  

---

## 11. Core design principles (merged)

1. **CA learning sandbox first** — not a general engine product.  
2. **Ownership explicit** — Illumo owns services; App owns module set; context does not own.  
3. **Boundaries over cleverness** — abstract volatility (GLFW, GL, future compute); keep the stable sparse-domain/bounded-view split explicit.
4. **Sim produces complete state; render observes** — double-buffer; no draw mid-generation.  
5. **Tokens for draw submission** — enroll once, emit commands, backend executes.  
6. **Parallelize data transformations last** — deterministic serial ownership first; adaptive cell candidates and bounded CPU chunk evaluation are justified only by measured pressure.
7. **Archive experiments** — don’t leave half-live ECS/graph/passes in the hot path.  
8. **Simplest architecture that preserves the boundaries you care about** (engine PDF principle, applied to the CA product).  
9. **Code wins over docs** — update this file when consensus shifts.  
10. **One documentation tree** — current prose, LaTeX, decisions, package maps,
    and session records live under `docs/`; generated PDF output is not source.

---

## 12. Open questions (remaining)

Most design questions from the LaTeX open list are **resolved** (see §6). Still open or only lightly decided:

| Topic | Working answer |
|-------|----------------|
| Resource ownership long-term | Bulk destruction remains at shutdown; the resizeable canvas texture has explicit replace/release lifecycle (D-P23). Refcounts / generational handles remain deferred until hot-reload needs them. |
| Linux/macOS parity | Freeze until Windows token path solid — Windows path is solid; parity still optional. |
| Tracy CI policy | Debug-oriented; no strict CI policy yet. |
| When to introduce chunks / SYCL | After correctness + serial benchmarks, or as an explicit learning goal. |

Resolved highlights (do not re-open without a new decision ID):

- Token payload shape → D-R1  
- Handles → D-R3  
- Who emits tokens → D-R2  
- Scene graph leftovers → D-E4  
- IllumoContext growth → D-E5  
- Canvas domain vs view → D-C1 refined by D-C2 (`CellGrid` + `Canvas`)  
- String uniforms → D-R9 debt  
- Backend injection → D-R11  
- Command queue overflow → D-R12  
- Single production frame path → D-R13  
- Canvas upload dirty rects → D-P1 / PBO path  
- Module registration → D-E1  
- InputManager Game deps → D-E2  
- EntityTable → D-E3 archived  
- CPU color fade → **restored** (RGB display)  
- Wireworld → implemented  
- MacroDefs toxicity → D-F1 deferred  
- Project/product naming → D-N1 (`Illumo`)

---

## 13. How to use this document

| When | Action |
|------|--------|
| New chat / new session | Read **this file first**. |
| Deep dive | Then LaTeX design notes / decision log. |
| Architecture change | Update **this file** + append a decision log entry in `docs/latex/sections/09-design-decision-log.tex`. |
| Code lands | Update “code truth” sections here if behavior changed. |
| Significant session | Add a dated implementation/verification record under `docs/sessions/`. |
| Rebuild the PDF | Run `docs/build.ps1`; the default Windows CMake build runs `IllumoDocs` when PowerShell and `latexmk` are available. |
| Old PDFs / agenda | Treat as **history** (§2); do not re-implement superseded engine plans by default. |
| Bug triage | §8 first; architecture is not the problem until proven otherwise. |

---

### Appendix A — Source map (where code lives)

| Concern | Typical location |
|---------|------------------|
| Frame loop / composition | `Source/App/CellMain.cpp` |
| Host / services / modules | `Source/Engine/Illumo.*` |
| CA module / modes | `Source/Game/CellGameModule.*`, `CellContext.*` |
| Sparse domain cell storage | `Source/Game/SparseCellGrid.*` |
| Bounded view + fade + GPU enroll | `Source/Game/CanvasView.*` |
| Compatibility dense storage | `Source/Game/CellGrid.*`, `Source/Game/Canvas.*` |
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
