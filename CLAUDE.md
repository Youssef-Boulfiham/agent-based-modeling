# ABM Project

**Read folder README before working there.**

## Git — MANDATORY

- **NEVER commit unless the user explicitly says so.** No `git commit`, no auto-commit, ever — wait for an explicit instruction. Same for `git push`.

## Walking behaviour — VETO (Mandatory instruction)

- **The walking behaviour is specified in `instruction_manual/Agent_Walking_Pathfinding_Manual.docx`.** This is the source of truth.
- **All implementations (testcases, ABM code) must follow this specification exactly.**
- **Any changes to walking behaviour require explicit permission.** Ask first, then implement. This includes:
  - Changes to domain, corridor, or activity logic
  - Changes to pathfinding or routing rules
  - Changes to step validation (one-cell movement)
  - Changes to how agents choose targets or navigate
- This is fundamental. Do not modify without explicit instruction.

## Structure — MANDATORY (do not move/delete)

- **data/** — input/ (config), output/ (results), temp/ (cache), logs/ (history)
- **testcases/** — by language: cpp/ (native sim), html/ (browser sandboxes), pygame/ (python)
- **src/** — All .cpp; **src/include/** — All .h
- **main.cpp** — Project root

## Code Layout

Headers: `src/include/*.h` (`.h` lowercase only)
Includes: `#include "../include/Header.h"` (from .cpp), `#include "Header.h"` (from .h), `#include "include/Header.h"` (from main.cpp)
