# Source Code Directory

C++ implementation of the agent-based modeling system.

**Before modifying files in this directory, read `CLAUDE.md` for file organization rules.**

## Structure

```
src/
├── include/        # ALL header files (.h)
│   ├── System.h
│   ├── Env.h
│   ├── Agent.h
│   ├── Pathfinding.h
│   ├── Statistics.h
│   ├── ChatBox.h
│   └── Buttons.h
├── System.cpp      # Implementations (one .cpp per .h)
├── Env.cpp
├── Agent.cpp
├── Pathfinding.cpp
├── Statistics.cpp
├── ChatBox.cpp
└── Buttons.cpp
```

**MANDATORY RULE:** Headers in `include/`, implementations in `src/` root. Never deviate.

See `include/README.md` for header-specific conventions.

## Components Overview

| Component | Purpose | Key Classes |
|-----------|---------|-------------|
| **System** | Application lifecycle, SDL window, event loop | System |
| **Env** | Simulation world, agent collection, physics | Env |
| **Agent** | Individual agent behavior, state, movement | Agent |
| **Pathfinding** | A* pathfinding grid & algorithm | WalkGrid, pathfinding functions |
| **Statistics** | Real-time metrics & right-panel UI | Statistics |
| **ChatBox** | Log output & bottom-left panel | ChatBox |
| **Buttons** | Control buttons & top bar UI | Buttons |

## Include Conventions

See `CLAUDE.md` for specific path rules:

**From `src/*.cpp`** → `#include "../include/MyHeader.h"`
```cpp
// In src/Agent.cpp
#include "../include/Env.h"
#include "../include/Pathfinding.h"
```

**From `src/include/*.h`** → `#include "OtherHeader.h"`
```cpp
// In src/include/Agent.h
#include "Env.h"
#include "Pathfinding.h"
```

**From `main.cpp`** (project root) → `#include "include/MyHeader.h"`
```cpp
// In main.cpp
#include "include/System.h"
```

## Code Style

- **No comments for obvious code** — Well-named variables explain themselves
- **Comment only the WHY** — Hidden constraints, non-obvious algorithms, workarounds
- **Const by default** — Mark parameters & members const unless mutation is required
- **Forward declarations** — Use in headers to minimize includes (see Agent.h)
- **Ownership clear** — Document who owns pointers; use RAII when possible

## Adding New Components

When adding a new component (e.g., `Behavior.h/.cpp`):

1. **Create header** in `src/include/Behavior.h`
2. **Create implementation** in `src/Behavior.cpp`
3. **Add include** to dependent files
4. **Update this README** with new component entry
5. **Add detailed documentation** in `include/Behavior.h`

## Testing

Unit tests for src/ components live in `testcases/` folders:
- `testcases/agent-behavior/` — Agent logic tests
- `testcases/pathfinding/` — A* algorithm tests
- `testcases/environment/` — Env state tests
- `testcases/integration/` — Full system tests

See `testcases/README.md` for running tests.

## Dependencies

- **SDL2** — Window, rendering (used by System, UI)
- **GLM** — Math library (used by all)
- **C++17** — Language standard

See `../CMakeLists.txt` for linking.
