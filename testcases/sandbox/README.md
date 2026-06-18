# Sandbox Mode

Lightweight testing environment for ABM concepts — stripped-down C++ simulation with zero external dependencies.

**Before modifying sandbox code, read this file.**

## Purpose

- **Proof-of-concept testing** — Validate ideas before main integration
- **Isolated debugging** — Test algorithms without SDL/graphics overhead
- **Rapid iteration** — Build & run in seconds, no dependencies
- **Reproducible scenarios** — Fixed random seeds, deterministic output
- **Concept validation** — Test core simulation logic in isolation

## When to Use Sandbox

**DO use sandbox for:**
- Testing new pathfinding algorithms
- Validating agent behavior logic
- Profiling core simulation (without UI)
- Creating reproducible test scenarios
- Debugging complex interactions

**DON'T use sandbox for:**
- Testing UI components (use full app)
- Testing SDL/graphics rendering
- Integration testing with main app
- Testing file I/O beyond logging

For UI & integration tests, use full app or create specialized test suites in `testcases/` folders.

## Structure

```
sandbox/
├── README.md              # This file
├── sandbox.cpp            # Standalone simulation engine (~280 lines)
├── build.sh               # Compilation script
├── build/                 # Compiled binary + results
│   ├── sandbox            # Executable
│   └── results/           # Simulation logs
└── results/               # Alternate output location
```

## Building

```bash
cd testcases/sandbox
./build.sh
```

**Requirements:** C++11 compiler (g++, clang), standard library only.

**No external dependencies:** No SDL, GLM, or other libraries. Just stdlib.

**Output:** `build/sandbox` executable, logs in `build/results/`.

## Running

```bash
cd testcases/sandbox/build
./sandbox
```

**Output:** `results/sandbox_log_YYYYMMDD_HHMMSS.txt`

**Typical run:** 5 seconds (312 frames at 60 FPS), 10 agents, 100×100 world.

## What's in the Sandbox

### SandboxAgent
Minimal agent implementation:
- **Position & velocity** — 2D coordinate tracking
- **Activity state** — "idle" or "moving"
- **Target seeking** — Move toward random destination
- **Timestep** — Delta-time based physics

### SandboxEnv
Simplified world management:
- **2D space** — Configurable (default 100×100)
- **Agent collection** — Vector of agents
- **Frame loop** — Step simulation at fixed timestep
- **File logging** — Log state every N frames
- **No physics** — Agents pass through each other

### Simulation Loop
```
1. Spawn agents at random positions
2. Every ~30 frames: assign random target to idle agents
3. Agent::step() updates position, activity, status
4. Every 60 frames: log full state to file
5. Repeat until duration expires (default 5 seconds)
```

## Output Format

Log file contains:
```
=== Sandbox Simulation Started ===
Environment: 100 x 100
Agents: 10
---
Frame 1 (t=0.02s)
  Agent 0: pos=(47.64, 5.13) activity=idle status=active
  Agent 1: pos=(3.34, 23.25) activity=idle status=active
  ...
Frame 61 (t=0.98s)
  Agent 0: pos=(47.15, 5.99) activity=moving status=active
  ...
---
=== Simulation Complete ===
Total frames: 312
Total time: 4.99 seconds
Agents: 10
```

Parse with: `cat results/sandbox_log_*.txt`

## Extending the Sandbox

### Add New Behaviors
1. **Modify SandboxAgent** — Add new activity types, state transitions
2. **Modify SandboxEnv::step()** — Add world rules (e.g., collision, grouping)
3. **Add logging** — Track new metrics in output

### Example: Add Collision Avoidance
```cpp
// In SandboxEnv::step()
for (auto agent : agents) {
    // Check distance to other agents
    for (auto other : agents) {
        if (distance < MIN_DISTANCE) {
            // Steer away
        }
    }
    agent->step(deltaTime);
}
```

### Example: Test Activity Probability
```cpp
// In SandboxAgent::step()
if (needs_activity_change) {
    float rand = distribution(rng);
    if (rand < 0.3f) activity = "eating";
    else if (rand < 0.6f) activity = "working";
    else activity = "socializing";
}
```

### Example: Track Metrics
```cpp
// In SandboxEnv::logStats()
logFile << "Agent distance traveled: " << agent->getDistanceTraveled() << std::endl;
```

## Test Scenarios

### Scenario 1: Movement & Speed
**Test:** Do agents move at correct velocity?
- Modify agent speed, check position delta in logs
- Expected: position change = speed × deltaTime

### Scenario 2: Behavior Transitions
**Test:** Do agents transition between states correctly?
- Add idle → moving → idle → moving cycle
- Expected: Log shows transitions at correct frames

### Scenario 3: Activity Distribution
**Test:** Are activities distributed with correct probability?
- Weight pathfinding = 40%, socializing = 60%
- Run many frames, count occurrences
- Expected: Ratio ~40:60 in logs

### Scenario 4: Scalability
**Test:** Performance with many agents?
- Change agent count to 100, 1000, 10000
- Check total runtime in logs
- Expected: Linear or near-linear scaling

## Modifying Simulation Parameters

Edit `sandbox.cpp` to change:

```cpp
// In main():
SandboxEnv env(100.0f, 100.0f, 10);  // width, height, agent count
env.run(5.0f);                        // duration in seconds
```

## Debugging Tips

**Understand log format:** First few frames show agent spawning. Later frames show movement behavior.

**Check for anomalies:** Agents stuck in corners? Teleporting? Check position deltas in consecutive frames.

**Profile agent count:** Default 10 agents. Increase to find performance cliff.

**Add debug output:** Modify logStats() to log additional fields (velocity, acceleration, etc.).

**Seed for reproducibility:** RNG seeded from system time. Add fixed seed for deterministic runs:
```cpp
rng = std::mt19937(12345);  // Fixed seed
```

## Integration with Main App

Sandbox logic is independent from main app. When validated in sandbox:
1. Migrate tested behavior to `src/Agent.cpp`
2. Test in full app with UI
3. Check for regressions with full system

Sandbox is validation step, not replacement for main app testing.
