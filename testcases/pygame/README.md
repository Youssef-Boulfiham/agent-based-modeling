# Pygame Sandboxes

Python/pygame testcases. For behaviours that are quickest to prototype in Python
or that need pygame's input/rendering loop.

**Status: ready, none yet.** This folder is a placeholder for the third sandbox
language alongside `html/` and `cpp/`.

## When to use pygame here

- Quick numeric/algorithm prototypes that benefit from Python's stdlib.
- Behaviours needing real-time keyboard/mouse interaction in a window.
- Bridging logic between the HTML proof and the C++ implementation.

## Adding a pygame testcase

1. New folder with a `main.py` and a `README.md`.
2. Keep deps minimal: `pip install pygame` only.
3. Use a fixed RNG seed for reproducibility.
4. Mirror the HUD idea from `html/` — show pass/fail (e.g. a violation counter)
   on screen.

```bash
cd testcases/pygame/<testcase>
python3 main.py
```
