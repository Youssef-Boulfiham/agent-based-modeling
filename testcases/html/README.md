# HTML Sandboxes

Browser-based testcases. Self-contained HTML/JS — open in a browser, instant
visual, no compile step. Best when you want to *see* a behaviour and share it.

**Before modifying a testcase, read its own README.**

## Testcases

- **walking_behaviour/** — domein-gestuurd looppad via gangen. The agent checks
  every step which domain it must be in (set externally, stable ≥ 50 steps),
  routes there via the corridors, and once arrived either wanders (idle) or
  stands still (working). Walks a priority queue of cells one step at a time;
  never jumps. Pure HTML/JS: simulation + visualisation in one file. (ACTIVE)

  > Merges the former `walking_behaviour` and `domain_extraction` testcases —
  > both concerned the walking behaviour, so they are now one.

## Running

Most testcases are a single `index.html` — open it directly, or serve the folder:

```bash
python3 -m http.server 8732 --directory testcases/html/walking_behaviour
# open http://localhost:8732/index.html
```

In Claude Code, use the `walking-behaviour` launch config (`.claude/launch.json`).

## Adding an HTML testcase

1. New folder with `index.html` (sim + canvas viz) and a `README.md`.
2. Keep it dependency-free; embed the sim in JS so it runs offline.
3. Use a deterministic seeded RNG so runs reproduce; add a `reseed` control.
4. Show pass/fail live in the HUD (e.g. a violation counter).

**Note:** The walking behaviour testcase (`walking_behaviour/`) is locked to the
specification in `instruction_manual/`. Do not modify walking behaviour without
explicit permission — ask first.
