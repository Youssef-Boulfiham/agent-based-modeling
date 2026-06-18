# ABM Project

**Read folder README before working there.**

## Final product — MANDATORY (the runnable app)

- **`AgentBasedModeling.app` (the macOS Application bundle in the project root)
  is the final product the user double-clicks** to open the latest version.
  Its launcher (`AgentBasedModeling.app/Contents/MacOS/launcher.sh`) jumps
  straight to the root `./AgentBasedModeling` binary — it does NOT rebuild.
- **The root binary `./AgentBasedModeling` must always be the newest build, AND
  the `.app` bundle timestamp must be bumped every time.** After ANY change to
  `src/`, run the build-to-app script — NEVER stop at just the binary copy:
  ```
  ./build_app.sh
  ```
  It rebuilds, installs the root binary, AND `touch`es the `.app` bundle so
  Finder shows the newest version. The user judges freshness by the `.app`
  timestamp — if the `.app` is not touched, the user sees a stale date and
  thinks the fix never shipped. ALWAYS build until the `.app` is newest.
  (Manual equivalent: `cmake --build build && cp build/AgentBasedModeling ./AgentBasedModeling && touch AgentBasedModeling.app AgentBasedModeling.app/Contents AgentBasedModeling.app/Contents/MacOS`)
- Run from the project root — asset paths (`map/...`, `data/...`) resolve relative to root.
  The `.app` launcher `cd`s to root before running, so double-click works too.
- The HTML/cpp items under `testcases/` are sandboxes/proving grounds, **not**
  the final product. When the user says "the application" / "the app" / "open
  it to check", they mean the **`AgentBasedModeling.app` (double-click), backed
  by the root `AgentBasedModeling` binary** — never a testcase.

## Git — MANDATORY

- **NEVER commit unless the user explicitly says so.** No `git commit`, no auto-commit, ever — wait for an explicit instruction. Same for `git push`.

## Walking behaviour — VETO (Mandatory instruction)

- **`instruction_manual/Agent_Walking_Pathfinding_Manual.docx` is the source of truth.**
  - Page 1: textual specification (what the behaviour must do).
  - Page 2+: artefact v1 — verbatim code of every function controlling walking (commit 32c49e0, 2026-06-18). This is the reference implementation.
- **READ THIS DOCUMENT before touching any walking behaviour code.**
- **All implementations (testcases, ABM code) must follow this specification exactly.**
- **Any changes to walking behaviour require explicit permission.** Ask first, then implement. This includes:
  - Changes to domain, corridor, or activity logic
  - Changes to pathfinding or routing rules
  - Changes to step validation (one-cell movement)
  - Changes to how agents choose targets or navigate
- This is fundamental. Do not modify without explicit instruction.

## Structure — MANDATORY (do not move/delete)

- **map/** — the world the agents navigate (flat). `mapgen_poc.ipynb` (PoC
  notebook: how it was figured out), `map_generator.py` (THE generator — run it
  to make `background.png` + `map_overview.png`), `background.png` (nav layer the
  grid is extracted from), `enviroment.png` (hand art), `map_overview.png`
  (generated overview). See `map/README.md`. The app reads `map/...`.
- **data/** — input/ (config), output/ (results), temp/ (cache), logs/ (history)
- **testcases/** — by language: cpp/ (native sim), html/ (browser sandboxes), pygame/ (python)
- **src/** — All .cpp; **src/include/** — All .h
- **main.cpp** — Project root

## Programming Agent Workflow — MANDATORY

Full pipeline for every idea. Stages in order — none skipped, implementation never starts before approval.

| # | Stage | Rule |
|---|-------|------|
| 01 | **Idea** | Register. Give quick response: confirm what holds, flag what needs scrutiny. |
| 02 | **Deep Search** | Search full ABM codebase. Extract relevant logic, constraints, data structures. Generate new angles. |
| 03 | **Report** | Second response: findings, conflicts, new directions. No code yet. |
| 04 | **Approval** | User approves scope. Only then does implementation begin. |
| 05 | **Agent Work** | Use large model to work out full implementation. Draw on everything discussed. |
| 06 | **Keep Updated** | Notify user of anything found during implementation they would want to know. No silence on surprises. |
| 07 | **Testcases** | Write implementation in `testcases/` first (HTML sandbox or cpp). Not in main ABM yet. |
| 08 | **Iteration** | Refine together until behaviour matches the idea. Testcase is the proving ground. |
| 09 | **Push** | User instructs push to main. Agent pushes. Never automatic. |
| 10 | **Write Manual** | Write/update instruction manual `.docx` in `instruction_manual/`. |
| 11 | **Implement ABM** | Integrate approved behaviour into `src/`. Follow the manual exactly. |
| 12 | **Log Version** | Write artefact: verbatim code extraction of all behaviour-controlling functions in execution order. |
| 13 | **Build Final Product** | Rebuild the root binary: `cmake --build build && cp build/AgentBasedModeling ./AgentBasedModeling`. `AgentBasedModeling.app` (double-click) launches that fresh root binary — it is the final product the user opens to check the latest version. `testcases/` are only sandboxes. Always rebuild into root after every change. |

**Artefact (Stage 12):**
- Trace order: `Env` fields → `while` loop → `Env::step()` → `Agent::step()` → all sub-functions.
- Verbatim from the approved commit. No summaries, no pseudocode.
- Recovery guarantee: pasting the artefact into a clean project reproduces the approved outcome exactly.
- **Embedded inside the manual `.docx` (page 2+). NEVER a separate file.**
- Regenerate by running `node build_manual.js` (or equivalent build script) in `instruction_manual/`.
- Written automatically after every confirmed push to main — user does not need to ask.

See `instruction_manual/Programming_Agent_Workflow_Manual.docx` for full specification.

## Code Layout

Headers: `src/include/*.h` (`.h` lowercase only)
Includes: `#include "../include/Header.h"` (from .cpp), `#include "Header.h"` (from .h), `#include "include/Header.h"` (from main.cpp)
