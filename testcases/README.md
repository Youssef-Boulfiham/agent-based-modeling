# Test Cases & Sandbox

Testing & experimentation space for agent-based modeling system.

**Before working on any test folder, read its README first.**

## Structure

```
testcases/
├── README.md                # This file
├── sandbox/                 # POC & concept testing (ACTIVE)
│   ├── README.md            # Sandbox documentation
│   ├── sandbox.cpp          # Simplified simulation (no SDL/GLM)
│   ├── build.sh             # Compile script
│   ├── build/               # Binary output
│   └── results/             # Simulation logs
├── pathfinding/             # A* algorithm tests (PLANNED)
├── agent-behavior/          # Agent logic tests (PLANNED)
├── environment/             # World state tests (PLANNED)
├── statistics/              # Metrics tests (PLANNED)
├── ui-elements/             # UI component tests (PLANNED)
└── integration/             # End-to-end tests (PLANNED)
```

## Testing Philosophy

**Isolation** — Each test folder tests one component in isolation from full system complexity.

**Speed** — Tests compile & run in seconds, not minutes. No SDL, no graphics overhead.

**Reproducibility** — Tests use fixed seeds, log all inputs, and can be re-run identically.

**Documentation** — Each folder documents what it tests, how to run it, how to interpret results.

## Active: Sandbox Mode

`sandbox/` is a minimal C++ simulator (zero external dependencies).

**Use it to:**
- Validate concepts before main integration
- Debug core algorithms (movement, activity selection, etc.)
- Test mathematical models
- Create deterministic scenarios
- Profile performance without UI overhead

**Why isolated?**
- Main system has SDL/graphics overhead
- Sandbox is pure simulation logic
- Fast iteration & debugging
- Clear cause-effect for algorithm changes

**Run:**
```bash
cd testcases/sandbox
./build.sh
cd build && ./sandbox
```

Output: `results/sandbox_log_YYYYMMDD_HHMMSS.txt`

See `sandbox/README.md` for details.

## Planned: Specialized Test Suites

### pathfinding/
Tests A* algorithm, grid generation, pathfinding performance.

### agent-behavior/
Tests individual agent behavior: activity selection, movement, state transitions.

### environment/
Tests world state management, agent spawning, activity zones, physics.

### statistics/
Tests metrics calculation, real-time stats, panel rendering.

### ui-elements/
Tests UI components: buttons, panels, text rendering, interaction.

### integration/
End-to-end system tests combining all components with SDL.

## Adding New Test Suites

When adding a test folder (e.g., `pathfinding/`):

1. **Create folder** with `README.md` explaining:
   - What component is tested
   - Why testing in isolation matters
   - How to run tests
   - Expected output format
   - Passing criteria

2. **Create test executable** (C++, minimal deps):
   ```cpp
   // testcases/pathfinding/pathfinding_tests.cpp
   #include <iostream>
   // ... minimal headers ...
   
   int main() {
       // Run tests, log results
   }
   ```

3. **Create build script** similar to `sandbox/build.sh`:
   ```bash
   g++ -std=c++11 -O2 pathfinding_tests.cpp -o build/pathfinding_tests
   ```

4. **Create results directory** for output logs.

5. **Document expected output** so tests can be validated.

## Test Output Convention

All test suites output logs to `results/` subdirectory:
```
testcases/sandbox/results/sandbox_log_YYYYMMDD_HHMMSS.txt
testcases/pathfinding/results/pathfinding_log_YYYYMMDD_HHMMSS.txt
```

Each log includes:
- Test configuration (what's being tested)
- Test inputs (parameters, initial conditions)
- Frame-by-frame state (if iterative)
- Final results (pass/fail, metrics)
- Timing info (how long the test took)

## Running All Tests

Future: Add top-level script to run all testcases:
```bash
./run_all_tests.sh
```

For now, run manually:
```bash
cd testcases/sandbox && ./build.sh && cd build && ./sandbox
# ... more tests as they're added ...
```

## Integration with Main Code

Test suites are **decoupled** from main app but **test the same logic**:
- Sandbox tests Agent/Env core (simplified, no SDL)
- Main app uses full Agent/Env with UI

Changes to `src/` should trigger test validation:
1. Modify core logic in `src/Agent.cpp`, etc.
2. Re-run relevant test suite
3. Verify behavior unchanged or improved
4. If regression, fix before committing

## Benchmarking & Metrics

Use test suites to track performance:
- Save results with timestamps
- Compare against baselines
- Identify slow commits
- Validate optimizations

Example workflow:
```bash
cd testcases/sandbox
./build.sh && cd build && ./sandbox
# Review results/sandbox_log_*.txt
# Note metrics: frames, time, agents processed
# Run again after code change
# Compare performance delta
```

## Debugging Failed Tests

If a test fails:
1. **Read the results log** — First line of investigation
2. **Check test configuration** — Are inputs correct?
3. **Verify expectations** — Is the test criteria correct?
4. **Isolate the issue** — Modify test to narrow down cause
5. **Document finding** — Log in `data/logs/` for future reference

See `CLAUDE.md` for rule about reading READMEs first.
