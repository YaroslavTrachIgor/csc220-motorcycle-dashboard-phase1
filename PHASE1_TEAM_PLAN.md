# Phase I — Team roles, rubric alignment, and remaining work

**Course:** CSC 220 — Operating Systems and Systems Programming (Spring ’26)  
**Assignment:** *Motorcycle Dashboard* — *OS Project _ Motorcycle _ Phase I.pdf*  

The official **grading rubric** lives on Canvas. This document translates the **published Phase I specification** into actionable tasks. **Every team should cross-check each row against the actual rubric** and add or remove items if the instructor’s sheet differs.

---

## Partner responsibilities (baseline)

### Murat — reflection, comments, and headers (primary)

| Area | What “done” looks like |
|------|-------------------------|
| **Reflection document** | Standalone write-up per instructor format (often: what you built, how threads interact, what was hard, what you’d change for Phase II, honest division of labor). Submit wherever the syllabus specifies (Canvas upload, etc.). |
| **File headers** | At top of each `.c` / `.h`: project/course, filename, one-line purpose, optional `@author` / date if required. |
| **Comments** | Brief block comments above each subsystem’s `*_thread` and any non-obvious logic (RPM/temp coupling, fuel formula, UTF-8 width padding, signal demo loop). Use `/* ... */` or consistent `//` style per course style guide. |
| **Readable identifiers** | Spot-check that names match responsibilities (already mostly good); fix any unclear locals. |

### Aidan — logging (primary)

| Area | What “done” looks like |
|------|-------------------------|
| **Logging design** | Decide what to log (e.g. subsystem name, event type, key numbers: RPM, speed, fuel, ECU zone changes). Avoid spamming the same line every tick unless the rubric asks for a trace. |
| **Implementation** | Small `log.h` / `log.c` (or similar): levels or categories, timestamp, single writer or documented interleaving. **Do not** send routine logs to the same stream as the live dashboard unless the assignment allows a split (e.g. dashboard = stdout, logs = stderr or log file). |
| **Phase I constraints** | Spec: simulation threads should not own “display.” Logging is a gray area: prefer **stderr** or **`fopen` append to a file** from a dedicated path, and document that **multiple pthreads writing logs without locks** can interleave bytes—acceptable for Phase I if that limitation is stated in the README or reflection. |
| **Build switch** | Optional `-DENABLE_LOG` or env var `BAZOOKI_LOG=1` so graders can run with logs on/off. |

---

## Rubric themes (from Phase I PDF) vs. current codebase

These are the main **grading dimensions** implied by the assignment text. Tick them off against Canvas.

| Theme | Expectation (from spec) | Status (high level) |
|-------|-------------------------|---------------------|
| **Threaded design** | Engine, Motion, Fuel, ECU, Dashboard each as `pthread`, concurrent | Implemented |
| **Shared state** | One coherent `system_state` / `g_state` all subsytems read/write | Implemented |
| **Layering** | Simulation → ECU → Dashboard; no simulation logic inside dashboard | Implemented; only `dashboard.c` uses `printf` for UI |
| **Dashboard contents** | Engine, RPM + zone, temp + class, times, speed, fuel + low warning, distances, signals, headlight | Implemented |
| **Motion model** | Speed changes in a clear, non-random pattern; distance tied to speed | Implemented (50↔70 mph cycle); **README no longer spells out the formula** — see tasks |
| **Fuel model** | Consumption tied to speed/RPM / idle lowest | Implemented |
| **ECU role** | Classify RPM zone, temp class, derived/high-level state; **no** dashboard printing | Partially: classification yes; **spec examples** (low-fuel speed cap, REDLINE stress / shutdown) **not** implemented |
| **README / documentation** | Architecture, how pieces connect; **distance vs. speed relationship explained** | Architecture table present; **explicit formula / units paragraph missing** |
| **Extensibility** | Structure ready for Phase II sync / rules | Reasonable; could name more extension points in comments |
| **Naming / constants** | Clear constants and variable names | Mostly good; headers can document macro groups |
| **Git / collaboration** | Repo history, partners can contribute | In progress |

---

## Remaining tasks — Murat

- [ ] **Reflection document:** draft, peer review with team, submit per syllabus.
- [ ] **File headers:** add to `include/*.h`, `src/*.c` (template agreed by team).
- [ ] **Comments:** `engine_thread`, `motion_thread`, `fuel_thread`, `ecu_thread`, `dashboard_thread`; document why `setlocale` exists in `main.c` (UTF-8 display width).
- [ ] **README — motion & distance:** add a short subsection (2–4 sentences) stating internal units (miles vs. displayed km), update interval (1 s), and formula **Δdistance = speed<sub>mph</sub> × (1/3600)** miles per second (or equivalent clear statement). This matches the spec requirement to explain the relationship.
- [ ] **README — optional:** one paragraph on **what is intentionally not in Phase I** (no mutexes, log interleaving, engine always ON in demo) so graders see you understood the boundaries.
- [ ] **Rubric pass:** open Canvas rubric and map each criterion to a file/section; add anything missing (e.g. “demo video,” “cover page,” “peer evaluation”) if listed.

---

## Remaining tasks — Aiden

- [ ] **Logging spec:** 1-page max internal note or README subsection: what is logged, at what rate, where output goes.
- [ ] **Implement logger:** `log_info(subsystem, fmt, ...)`, timestamps, compile-time or runtime enable.
- [ ] **Integrate calls:** at least ECU (zone changes, optional low-fuel flag), Motion (speed direction flip), Fuel (crossing low threshold)—enough to prove logging works without flooding.
- [ ] **Verify:** run under Docker Ubuntu from README; confirm logs don’t break dashboard (TTY).
- [ ] **Rubric pass:** if rubric has “testing,” “trace,” or “observability,” align log examples with those words.

---

## Joint / whoever has bandwidth

- [ ] **ECU “stretch” behaviors (only if rubric weights them):** e.g. cap max speed when fuel is low; track REDLINE dwell time and set a `engine_stress` or log event (Phase II can add real shutdown).
- [ ] **Engine OFF demo:** optional `volatile` flag toggled later from Phase III input; for Phase I, could document “engine remains ON for continuous dashboard demo.”
- [ ] **Short screen recording** if the rubric requires a demo video.
- [ ] **Contributors / `PHASE1_TEAM_PLAN.md`:** keep GitHub contributors and this plan updated if roles shift.

---

## Quick reference — who owns what

| Murat | Aiden |
|-------|--------|
| Reflection | Logging design + implementation |
| Headers & comments | Logger integration & verification |
| README gaps (distance/speed text, rubric cross-check) | Docker + log path smoke test |

Questions about interpretation of the PDF or rubric → **instructor or TA**, not guesswork.
