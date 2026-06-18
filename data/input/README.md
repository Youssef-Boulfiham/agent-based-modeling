# Input Directory

User-supplied configuration and guidance files for simulations.

## Purpose

Place simulation parameters, world definitions, and initial conditions here. Code reads from this directory to initialize the simulation.

## File Types

**Configuration files** (e.g., `config.json`, `simulation.yaml`)
- World size & grid resolution
- Agent count & initial positions
- Activity definitions & probability distributions
- Pathfinding parameters

**Map definitions** (e.g., `world.map`, `layout.txt`)
- Static obstacles
- Activity zones
- Spawn locations

**Initial conditions** (e.g., `agents_init.csv`)
- Agent starting positions & states
- Behavior flags
- Connection/relationship setup

**Parameter sweeps** (e.g., `experiments.txt`)
- Multiple configurations for batch runs
- Variable ranges to explore

## Conventions

- **Use clear naming** — `world_office_floor2.json` is better than `config1.txt`
- **Document assumptions** — Add comments or a `README_INPUTS.md` explaining what each file does
- **Version configs** — Use dates or experiment IDs if testing variations
- **Keep originals** — Don't modify inputs; if you need variants, copy and rename

## Example Workflow

1. User creates `world_office.json` describing office layout
2. Code loads from `data/input/world_office.json`
3. Simulation initializes world with those parameters
4. Results written to `data/output/` or `data/logs/`

## Integration

From C++ code, read files using paths like:
```cpp
std::string path = "../data/input/config.json";
```

See `../CLAUDE.md` for path conventions by file location.
