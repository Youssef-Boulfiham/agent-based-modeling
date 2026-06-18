#!/usr/bin/env python3
"""
segment.py — Extract walkable areas from a floorplan PNG into an integer mask grid.

Input : data/input/<image>.png  (colored room domains + beige corridors on dark void)
Output: data/output/
    mask_<name>.csv      integer grid, row-major, one cell per CELL pixels
    mask_<name>.json     metadata (palette, cell size, grid dims, domain stats)
    mask_<name>.png      color visualization of the extracted mask

Mask cell values:
    -1  BLOCKED   void / wall (impassable)
     0  CORRIDOR  shared walkable (any agent)
   1..N DOMAIN    a specific room (walkable, activity-restricted later)

The grid is what the pathfinding layer reads: it says which cells are walkable
and under which "domain" they fall, so agents can be restricted to their domain
plus the shared corridors.
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
INPUT = ROOT / "data" / "input"
OUTPUT = ROOT / "data" / "output"

# --- palette (from pixel inspection of v13_seed1.png) ---------------------
VOID = (17, 15, 12)
CORRIDOR_RGB = (204, 193, 173)          # beige shared hallway
ROOM_RGB = [
    (176, 127, 201),  # purple
    (122, 138, 160),  # slate
    (201, 161, 75),   # gold
    (91, 143, 201),   # blue
    (201, 96, 142),   # pink
    (108, 192, 168),  # teal
    (201, 138, 75),   # orange
]

BLOCKED, CORRIDOR = -1, 0
MATCH_THRESH = 55.0   # max RGB distance to count as a palette fill
CELL = 11             # source pixels per mask cell


def classify_pixels(im):
    """Per-pixel label: BLOCKED, CORRIDOR(0), or domain 1..N (nearest palette fill)."""
    h, w, _ = im.shape
    flat = im.reshape(-1, 3).astype(np.float32)

    # palette[0] = corridor, palette[1..N] = rooms
    palette = np.array([CORRIDOR_RGB] + ROOM_RGB, dtype=np.float32)
    labels = np.arange(palette.shape[0])      # 0=corridor, 1..N=rooms

    # nearest palette entry + its distance
    d = np.linalg.norm(flat[:, None, :] - palette[None, :, :], axis=2)
    nearest = d.argmin(axis=1)
    mindist = d.min(axis=1)

    out = labels[nearest].astype(np.int32)
    out[mindist > MATCH_THRESH] = BLOCKED     # void, walls, white frame/title
    return out.reshape(h, w)


def downsample_majority(px_labels, cell):
    """Collapse each cell-sized block to its majority label (ties -> BLOCKED)."""
    h, w = px_labels.shape
    rows, cols = h // cell, w // cell
    grid = np.full((rows, cols), BLOCKED, dtype=np.int32)
    n_classes = px_labels.max() + 1
    for r in range(rows):
        for c in range(cols):
            block = px_labels[r * cell:(r + 1) * cell, c * cell:(c + 1) * cell].ravel()
            walk = block[block >= 0]
            # a cell is walkable only if it is mostly (>=40%) walkable pixels
            if walk.size < 0.40 * block.size:
                continue
            grid[r, c] = np.bincount(walk, minlength=n_classes).argmax()
    return grid


def carve_doorways(grid, gap=3):
    """Stitch corridor stubs to rooms across the rendered void gap.

    In the source art a corridor stub points at a room but stops ~1.5 cells
    short (a visual threshold, not a wall). For each corridor cell we ray-cast
    the 4 cardinal directions up to `gap` cells; if the ray reaches a domain
    cell passing only through BLOCKED cells, those gap cells are carved to
    CORRIDOR -- creating a real, walkable doorway. Rays only start from
    corridors (which only exist at stub tips), so no room-to-room wall is
    ever breached.
    """
    rows, cols = grid.shape
    carved = 0
    corridor_cells = list(zip(*np.where(grid == CORRIDOR)))
    for r, c in corridor_cells:
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            blocked_run = []
            for step in range(1, gap + 1):
                ny, nx = r + dy * step, c + dx * step
                if not (0 <= ny < rows and 0 <= nx < cols):
                    break
                v = grid[ny, nx]
                if v == BLOCKED:
                    blocked_run.append((ny, nx))
                    continue
                if v >= 1 and blocked_run:        # hit a domain through a gap
                    for by, bx in blocked_run:
                        grid[by, bx] = CORRIDOR
                        carved += 1
                break                              # stop ray on any non-blocked
    return carved


def render(grid, path):
    """Save a color PNG of the mask for visual verification."""
    colormap = {BLOCKED: (20, 20, 24), CORRIDOR: (230, 222, 205)}
    for i, rgb in enumerate(ROOM_RGB, start=1):
        colormap[i] = rgb
    rows, cols = grid.shape
    img = np.zeros((rows, cols, 3), dtype=np.uint8)
    for val, rgb in colormap.items():
        img[grid == val] = rgb
    scale = 6
    Image.fromarray(img).resize((cols * scale, rows * scale), Image.NEAREST).save(path)


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "v13_seed1"
    src = INPUT / f"{name}.png"
    im = np.array(Image.open(src).convert("RGB"))

    px = classify_pixels(im)
    grid = downsample_majority(px, CELL)
    carved = carve_doorways(grid)
    print(f"doorways carved: {carved} cells")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    np.savetxt(OUTPUT / f"mask_{name}.csv", grid, fmt="%d", delimiter=",")

    n_domains = int(grid.max())
    stats = {int(d): int((grid == d).sum()) for d in range(1, n_domains + 1)}
    meta = {
        "source": str(src.relative_to(ROOT)),
        "cell_px": CELL,
        "rows": int(grid.shape[0]),
        "cols": int(grid.shape[1]),
        "legend": {"-1": "BLOCKED", "0": "CORRIDOR", **{str(i): f"domain_{i}" for i in range(1, n_domains + 1)}},
        "domain_colors": {str(i): list(c) for i, c in enumerate([CORRIDOR_RGB] + ROOM_RGB)},
        "cells": {
            "blocked": int((grid == BLOCKED).sum()),
            "corridor": int((grid == CORRIDOR).sum()),
            "domains": stats,
        },
    }
    (OUTPUT / f"mask_{name}.json").write_text(json.dumps(meta, indent=2))
    render(grid, OUTPUT / f"mask_{name}.png")

    print(f"grid {grid.shape[0]}x{grid.shape[1]} cell={CELL}px")
    print(f"domains found: {n_domains}")
    print(f"corridor cells: {meta['cells']['corridor']}  blocked: {meta['cells']['blocked']}")
    for d, n in stats.items():
        print(f"  domain {d}: {n} cells")
    print(f"wrote mask_{name}.csv / .json / .png to {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
