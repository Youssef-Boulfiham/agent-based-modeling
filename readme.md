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
- **Buttons** — UI controls (top bar). Labels: `Layer`, `Paths`, … `Settings`.
- **SettingsWindow** — In-game settings overlay (see below).

See `CLAUDE.md` for development guidelines and code layout rules.

## Settings menu — architecture (READ before adding settings)

The settings overlay is **one class, many panels, one data source**. This is a
deliberate game-dev pattern so settings stay simple to extend without bugs.

### 1. One class, many panels — NO window-on-window

`SettingsWindow` holds a single `Panel` enum (`NONE / MAIN / AGENTS / WORLD /
SIM / …`). Only **one** panel is active at a time. Switching panels *replaces*
the current one — there is never a settings window opened on top of another
settings window.

```
Top-bar "Settings" button → open()  → Panel::MAIN
MAIN: 10 category buttons  → click   → kills MAIN, sets Panel::AGENTS (etc.)
Sub-panel "< Back" button  → returns → Panel::MAIN
Any panel "X" / backdrop   → close() → Panel::NONE
```

To add a panel:
1. Add a value to the `Panel` enum (`SettingsWindow.h`).
2. Add a `render<Name>()` + `handle<Name>Click()` pair.
3. Wire a MAIN button index to it in `handleMainClick()`.
4. Add the dispatch cases in `render()` and `handleClick()`.

### 2. One data source — the connected pipeline (single source of truth)

`SettingsWindow` keeps a `Env* world` pointer, bound **once** in
`System::initializeSimulation()` via `settingsWindow->setWorld(simulation)`.

Every panel reads and writes the **same live `Env` / `Agent` objects** that the
simulation loop, the dev/env view, and the `ChatBox` use. No panel ever keeps
its own copy of game state. This is the rule that keeps everything in sync:

```
                ┌──────────────────────────────┐
   simulation → │   Env  (the agents, world)   │ ← ChatBox (reads/writes)
   loop         │   single source of truth     │
                └──────────────┬───────────────┘
                               │ Env*  (NOT a copy)
                        SettingsWindow panels
                  (Agents / World / Sim / … all bound here)
```

So the Agent shown in the **Agents** settings panel is the *same* Agent the
ChatBox talks to and the *same* Agent the dev view draws. Edit it in one place →
every view reflects it. **When you add a setting, bind it to `world`, never to a
snapshot** — otherwise the views drift apart and we "talk past each other".

### Where the knobs live (current)

- **Max agents** — hardcoded `System::MAX_AGENTS` (`src/include/System.h`).
  *(Currently 1 for bring-up.)* The Agents panel reads the live count from
  `Env::getActiveAgents()`, so it always matches whatever the sim is running.

## Artefact Workflow

Every approved version pushed to `main` triggers an **artefact** — a verbatim code extraction of all functions controlling that version's behaviour. Artefacts live in `instruction_manual/` and serve as recovery documents: pasting one into a clean project reproduces the approved outcome exactly. See `CLAUDE.md` (Artefact Rule) and `instruction_manual/Programming_Agent_Workflow_Manual.docx` for the full workflow.

## Dependencies

- C++17
- SDL2
- GLM (math library)
- CMake 3.20+
