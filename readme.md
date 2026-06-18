# Agent-Based Modeling Simulation

Interactive simulation framework for agent-based modeling (ABM) using C++ and SDL2.

## Overview

Simulates autonomous agents navigating a shared 2D world. Each agent exhibits behavioral rules, interacts with the environment, and is rendered in real-time.

## Final product — the runnable app

**`AgentBasedModeling.app` (the macOS Application in the project root) is the
final product** — double-click it to open the latest version. Its launcher
jumps straight to the root `./AgentBasedModeling` binary (no rebuild on launch).

The root binary must always be the newest build. After any change to `src/`,
rebuild it into the project root:

```bash
cmake --build build && cp build/AgentBasedModeling ./AgentBasedModeling
# then double-click AgentBasedModeling.app  (or run ./AgentBasedModeling)
```

The items under `testcases/` are sandboxes/proving grounds — **not** the final
product. "The application" means `AgentBasedModeling.app`, backed by the root
`AgentBasedModeling` binary.

## Quick Start

**First-time build:**
```bash
cmake -S . -B build
cmake --build build
cp build/AgentBasedModeling ./AgentBasedModeling
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

## Artefact Workflow

Every approved version pushed to `main` triggers an **artefact** — a verbatim code extraction of all functions controlling that version's behaviour. Artefacts live in `instruction_manual/` and serve as recovery documents: pasting one into a clean project reproduces the approved outcome exactly. See `CLAUDE.md` (Artefact Rule) and `instruction_manual/Programming_Agent_Workflow_Manual.docx` for the full workflow.

## Dependencies

- C++17
- SDL2
- GLM (math library)
- CMake 3.20+
