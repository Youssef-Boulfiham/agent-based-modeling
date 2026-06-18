# ABM Project

**Read folder README before working there.**

## Structure — MANDATORY (do not move/delete)

- **data/** — input/ (config), output/ (results), temp/ (cache), logs/ (history)
- **testcases/** — sandbox/ (minimal sim), future: pathfinding/, agent-behavior/, etc.
- **src/** — All .cpp; **src/include/** — All .h
- **main.cpp** — Project root

## Code Layout

Headers: `src/include/*.h` (`.h` lowercase only)
Includes: `#include "../include/Header.h"` (from .cpp), `#include "Header.h"` (from .h), `#include "include/Header.h"` (from main.cpp)
