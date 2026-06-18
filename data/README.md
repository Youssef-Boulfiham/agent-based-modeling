# Data Directory

Storage for simulation input, output, logs, and temporary files.

**Rule: Never delete or move this folder.** It contains the project's data lifecycle.

## Structure

```
data/
├── input/          # User-supplied configuration & guidance
├── output/         # Final simulation results
├── temp/           # Working files, caches, intermediate data
└── logs/           # Research history, iteration tracking, analysis logs
```

## Folders

### `input/`
User-supplied files. Examples:
- Configuration files (world size, agent count, behavior rules)
- Map definitions
- Initial conditions
- Parameter sweeps

**Rule: Users place files here, code reads from here.**

### `output/`
Final simulation results. Check here to evaluate performance.

Examples:
- Processed statistics
- Rendered visualizations
- Aggregated metrics
- Reports

**Rule: Code writes results here. This is the source of truth for performance.**

### `temp/`
Working files, intermediate computations, caches.

Examples:
- Partial results during long runs
- Cached pathfinding grids
- Temporary analysis files

**Rule: Safe to delete at any time. Never rely on temp/ persistence.**

### `logs/`
Research history and iteration tracking.

Examples:
- Experiment logs (what was tested, why)
- Parameter sweeps history
- Iteration notes
- Analysis checkpoints

**Rule: Developers write here when exploring. Good for audit trail & reproducibility.**

## Conventions

- **No deletion of data/** folder itself
- **Cleanup temp/** before long-running processes to free space
- **Organize by experiment** — create subfolders in logs/ by date or experiment name
- **Version results** in output/ with timestamps or run IDs
- **Document inputs** with comments or metadata files

## Accessing from Code

**C++ code** reads from data/ by paths like `../data/input/config.txt` (relative to src/) or via absolute paths.

**Test code** (sandbox) outputs to `../data/testcases/sandbox/results/` for integration with main data flow, or uses isolated `testcases/sandbox/results/` for standalone testing.

See `../CLAUDE.md` for path conventions.
