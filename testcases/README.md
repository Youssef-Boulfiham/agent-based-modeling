# Test Cases & Sandboxes

Testing & experimentation space for the agent-based modeling system.

**Before working on any test folder, read its README first.**

## Structure — by language

Testcases are organised by the sandbox technology they run in. Depending on what
a testcase needs (quick visual proof, native perf, scripting) you build it in
the matching folder.

```
testcases/
├── README.md       # This file
├── html/           # Browser sandboxes — self-contained HTML/JS, instant visual
│   └── walking_behaviour/   # Domein-gestuurd looppad via gangen (ACTIVE)
├── pygame/         # Python/pygame sandboxes (ready, none yet)
└── cpp/            # Native C++ sandboxes — no SDL/GLM, stdlib only
    └── sandbox.cpp          # Minimal POC simulator
```

## Which folder?

| Need                                   | Folder    |
|----------------------------------------|-----------|
| Instant visual proof, no build, share  | `html/`   |
| Python scripting / quick numeric proto | `pygame/` |
| Native perf, core-logic close to `src/`| `cpp/`    |

A single behaviour usually lives in **one** folder — the one that proves it best.
Walking behaviour is visual & interactive, so it lives in `html/`.

## Testing philosophy

- **Isolation** — each testcase exercises one component without the full system.
- **Speed** — runs in seconds, no heavy deps.
- **Reproducibility** — fixed seeds, deterministic runs (`reseed` to explore).
- **Documentation** — each folder explains what it tests and how to read results.

## Active testcases

### html/walking_behaviour — domein-gestuurd looppad via gangen
Every step the agent checks which **domain** it must be in (set by an external
controller, stable ≥ 50 steps). Not there yet → it routes **via the corridors
(gangen)** to that domain. Already there → activity decides: **idle** (wander the
room) or **working** (stand still); en route it is **move to domain**. Walks a
priority queue one cell per step; hard rule: never jumps. **Domain drives the
walk; the three activities are the loop-state.** See `html/walking_behaviour/README.md`.

### cpp/sandbox.cpp — minimal POC simulator
Stripped-down C++ sim (no SDL/GLM) for validating core logic and profiling.
See `cpp/README.md`.

## Integration with main code

Testcases are decoupled from the main app but exercise the same ideas. Validated
behaviour migrates into `src/` (e.g. `src/Agent.cpp`). The walking behaviour is
specified in `instruction_manual/` — that 1-A4 is the source of truth.
