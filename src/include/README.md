# Header Files Directory

All C++ header files for the agent-based modeling system.

**MANDATORY RULE:** All `.h` files belong here. Never place headers in `src/` root.

## File Listing

| File | Purpose | Exports |
|------|---------|---------|
| **System.h** | Application lifecycle, SDL setup | `class System` |
| **Env.h** | Simulation world, agent management | `class Env`, `struct Activity` |
| **Agent.h** | Individual agent behavior & state | `class Agent` |
| **Pathfinding.h** | A* navigation & grid | `struct WalkGrid`, pathfinding functions |
| **Statistics.h** | Real-time metrics display | `class Statistics` |
| **ChatBox.h** | Log panel & output | `class ChatBox` |
| **Buttons.h** | Control buttons & UI | `class Buttons` |

## Include Path Conventions

**From implementation files** (`src/*.cpp`):
```cpp
#include "../include/Agent.h"
```

**From other headers** (`src/include/*.h`):
```cpp
#include "Env.h"
```

**From main.cpp** (project root):
```cpp
#include "include/System.h"
```

## Header Writing Conventions

**1. Guard clause**
```cpp
#ifndef FILENAME_H
#define FILENAME_H
// ... content ...
#endif // FILENAME_H
```

**2. Forward declarations before includes**
```cpp
// Avoid circular deps — forward-declare if only using pointer/ref
struct WalkGrid;

#include <vector>
#include "Env.h"  // Only if necessary
```

**3. Minimal includes**
- Only include what's needed for the interface
- Use forward declarations for pointers & references
- Move complex includes to `.cpp` file

**4. Documentation**
```cpp
// Brief purpose description
class Agent {
public:
    // High-level behavior: agent walks path, chooses activities, meets others
    
    Agent(int id, glm::vec2 startPos, Env* env);
    virtual ~Agent();
    
    void step(float deltaTime);  // Update agent state & position
    // ...
};
```

**5. Access levels**
- `public` — External interface (minimal)
- `protected` — For subclassing (use sparingly per CLAUDE.md)
- `private` — Internal state & helpers

## Adding New Headers

1. **Create in `src/include/`**
2. **Add include guard** with uppercase filename
3. **Minimal dependencies** — Use forward declarations
4. **Document exports** — Comment what classes/functions it provides
5. **Update** `../README.md` with entry in components table
6. **Update `CLAUDE.md`** if adding new major component

## Circular Dependencies

To avoid circular includes:
- **Agent.h** uses `struct WalkGrid;` (forward decl) instead of including Pathfinding.h
- **System.h** includes Env.h (no circular — Env doesn't include System)
- **Env.h** includes Agent.h (necessary for vector<Agent*>)

Pattern: Whoever needs the full definition includes; those using pointers only forward-declare.

## No Implementation in Headers

**DON'T** put function bodies in headers:
```cpp
// WRONG
class Agent {
public:
    int getId() const { return id; }  // Don't do this
};
```

**DO** move to `.cpp`:
```cpp
// RIGHT in header
class Agent {
public:
    int getId() const;
};

// In src/Agent.cpp
int Agent::getId() const { return id; }
```

Exception: Small inline functions marked `inline` (use sparingly).

## Testing

Headers are tested in `testcases/` via implementation tests. No direct header tests.

See `../../../testcases/README.md` for test structure.
