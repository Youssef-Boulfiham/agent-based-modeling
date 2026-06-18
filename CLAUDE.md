# ABM Project

**Read folder README before working there.**

## Git — MANDATORY

- **NEVER commit unless the user explicitly says so.** No `git commit`, no auto-commit, ever — wait for an explicit instruction. Same for `git push`.

## Structure — MANDATORY (do not move/delete)

- **data/** — input/ (config), output/ (results), temp/ (cache), logs/ (history)
- **testcases/** — by language: cpp/ (native sim), html/ (browser sandboxes), pygame/ (python)
- **src/** — All .cpp; **src/include/** — All .h
- **main.cpp** — Project root

## Code Layout

Headers: `src/include/*.h` (`.h` lowercase only)
Includes: `#include "../include/Header.h"` (from .cpp), `#include "Header.h"` (from .h), `#include "include/Header.h"` (from main.cpp)
