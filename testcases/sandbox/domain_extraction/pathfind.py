#!/usr/bin/env python3
"""
pathfind.py — A* navigation over the extracted mask grid.

Proves the mask is usable for navigation:
  1. flood-fill: how many connected walkable components exist
  2. reachability: every domain reachable from every other domain
  3. many random domain->domain paths, logged + rendered

Walkable cell = CORRIDOR(0) or any domain(>=1). BLOCKED(-1) is wall/void.
Agents move 4-directionally (no corner cutting through walls).

Usage: python3 pathfind.py [name]   (default v13_seed1)
Writes path render + log to data/output/.
"""
import csv
import heapq
import json
import random
import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "data" / "output"
BLOCKED, CORRIDOR = -1, 0
NEI = [(-1, 0), (1, 0), (0, -1), (0, 1)]


def load(name):
    grid = np.loadtxt(OUTPUT / f"mask_{name}.csv", dtype=int, delimiter=",")
    meta = json.loads((OUTPUT / f"mask_{name}.json").read_text())
    return grid, meta


def walkable(grid, r, c):
    return 0 <= r < grid.shape[0] and 0 <= c < grid.shape[1] and grid[r, c] >= 0


def components(grid):
    """Label connected walkable components (4-conn). Returns label grid, count."""
    lab = np.full(grid.shape, -1, dtype=int)
    n = 0
    for r in range(grid.shape[0]):
        for c in range(grid.shape[1]):
            if grid[r, c] >= 0 and lab[r, c] < 0:
                q = deque([(r, c)])
                lab[r, c] = n
                while q:
                    y, x = q.popleft()
                    for dy, dx in NEI:
                        ny, nx = y + dy, x + dx
                        if walkable(grid, ny, nx) and lab[ny, nx] < 0:
                            lab[ny, nx] = n
                            q.append((ny, nx))
                n += 1
    return lab, n


def astar(grid, start, goal):
    """4-dir A* over walkable cells. Returns list of (r,c) or None."""
    def h(a, b):
        return abs(a[0] - b[0]) + abs(a[1] - b[1])
    openq = [(h(start, goal), 0, start)]
    came, g = {start: None}, {start: 0}
    while openq:
        _, cost, cur = heapq.heappop(openq)
        if cur == goal:
            path = []
            while cur is not None:
                path.append(cur)
                cur = came[cur]
            return path[::-1]
        if cost > g.get(cur, 1e18):
            continue
        for dy, dx in NEI:
            nb = (cur[0] + dy, cur[1] + dx)
            if not walkable(grid, *nb):
                continue
            ng = cost + 1
            if ng < g.get(nb, 1e18):
                g[nb] = ng
                came[nb] = cur
                heapq.heappush(openq, (ng + h(nb, goal), ng, nb))
    return None


def domain_centroids(grid, n_domains):
    """A representative walkable cell near each domain's centroid."""
    cents = {}
    for d in range(1, n_domains + 1):
        ys, xs = np.where(grid == d)
        cy, cx = int(ys.mean()), int(xs.mean())
        # snap to nearest cell that is actually this domain
        idx = np.argmin((ys - cy) ** 2 + (xs - cx) ** 2)
        cents[d] = (int(ys[idx]), int(xs[idx]))
    return cents


def render_paths(grid, paths, path_png):
    base = {BLOCKED: (20, 20, 24), CORRIDOR: (230, 222, 205)}
    rooms = [(176, 127, 201), (122, 138, 160), (201, 161, 75), (91, 143, 201),
             (201, 96, 142), (108, 192, 168), (201, 138, 75)]
    for i, rgb in enumerate(rooms, 1):
        base[i] = tuple(int(v * 0.55 + 90) for v in rgb)  # washed out so paths pop
    rows, cols = grid.shape
    img = np.zeros((rows, cols, 3), np.uint8)
    for v, rgb in base.items():
        img[grid == v] = rgb
    for path in paths:
        for (r, c) in path:
            img[r, c] = (255, 60, 40)
    Image.fromarray(img).resize((cols * 6, rows * 6), Image.NEAREST).save(path_png)


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "v13_seed1"
    grid, meta = load(name)
    n_domains = max(int(k) for k in meta["legend"] if int(k) > 0)

    lab, ncomp = components(grid)
    sizes = [int((lab == i).sum()) for i in range(ncomp)]
    main_comp = int(np.argmax(sizes))
    print(f"walkable components: {ncomp} (largest {max(sizes)} cells)")

    cents = domain_centroids(grid, n_domains)
    # connectivity matrix between domains
    reach = {}
    for a in range(1, n_domains + 1):
        for b in range(a + 1, n_domains + 1):
            p = astar(grid, cents[a], cents[b])
            reach[(a, b)] = p is not None
    n_pairs = len(reach)
    n_ok = sum(reach.values())
    print(f"domain pairs connected: {n_ok}/{n_pairs}")
    for (a, b), ok in reach.items():
        if not ok:
            print(f"  UNREACHABLE: domain {a} <-> {b}")

    # many random navigation tests
    random.seed(7)
    walk_cells = [(int(r), int(c)) for r, c in zip(*np.where(grid >= 0))]
    main_cells = [(int(r), int(c)) for r, c in zip(*np.where(lab == main_comp))]
    trials, success, lengths, sample_paths = 0, 0, [], []
    log_rows = []
    for _ in range(200):
        s, gl = random.choice(main_cells), random.choice(main_cells)
        if s == gl:
            continue
        trials += 1
        p = astar(grid, s, gl)
        if p:
            success += 1
            lengths.append(len(p))
            log_rows.append((s, gl, grid[s], grid[gl], len(p)))
            if len(sample_paths) < 12 and len(p) > 30:
                sample_paths.append(p)
    print(f"random paths: {success}/{trials} succeeded, "
          f"avg len {sum(lengths)/len(lengths):.1f}, max {max(lengths)}")

    render_paths(grid, sample_paths, OUTPUT / f"paths_{name}.png")
    with open(OUTPUT / f"pathlog_{name}.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["start_rc", "goal_rc", "start_domain", "goal_domain", "path_len"])
        for s, gl, sd, gd, L in log_rows:
            w.writerow([s, gl, sd, gd, L])

    all_pairs_ok = n_ok == n_pairs
    print(f"\nPASS={all_pairs_ok and success == trials}")
    print(f"wrote paths_{name}.png + pathlog_{name}.csv")


if __name__ == "__main__":
    main()
