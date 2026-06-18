# Agent-Based Modeling Simulation

Interactive simulation framework for agent-based modeling (ABM) using C++ and SDL2.

## Overview

Simulates autonomous agents navigating a shared 2D world. Each agent exhibits behavioral rules, interacts with the environment, and is rendered in real-time.

## Quick Start

**Build & run main app:**
```bash
cd build
cmake ..
make
./AgentBasedModeling
```

**Run sandbox tests:**
```bash
cd testcases/sandbox
./build.sh
cd build && ./sandbox
```

## Project Structure

```
project-root/
├── README.md                   This file
├── CLAUDE.md                   Development guidelines & rules
├── main.cpp                    App entry point
├── CMakeLists.txt             Build configuration
├── src/                        Source code (headers in include/, impls here)
├── build/                      Build output (cmake, binaries)
├── data/                       Input/output/logs (see data/README.md)
└── testcases/                  Testing infrastructure (see testcases/README.md)
```

**Before working on any folder, read its README first.**

## Data Structure

```
data/
├── input/          # User supplies config & guidance
├── output/         # Final results (check for performance)
├── temp/           # Working files & caches (safe to delete)
└── logs/           # Research history & iteration data
```

See `data/README.md` for detailed usage.

## Architecture

- **System** — Application entry point (window, SDL, event loop)
- **Env** — Simulation state (world, agents, physics)
- **Agent** — Individual agent logic (behavior, movement)
- **Pathfinding** — A* navigation for agents
- **Statistics** — Real-time stats panel (right)
- **ChatBox** — Log output panel (bottom-left)
- **Buttons** — UI controls (top bar)

See `CLAUDE.md` for development guidelines and code layout rules.

## Dependencies

- C++17
- SDL2
- GLM (math library)
- CMake 3.20+
