# Domain Extraction & Navigation Test

Proves we can turn a floorplan PNG into a **walkability mask** (integer grid) and
navigate one agent through it along many different paths.

## What it does

1. **segment.py** — reads `data/input/v13_seed1.png`, classifies every pixel to a
   palette (void/wall, beige corridor, 7 colored room domains), downsamples to a
   cell grid by block-majority, then **carves doorways** (corridor stubs stop ~1.5
   cells short of each room in the source art — rays from corridor cells punch the
   gap so rooms become reachable).
   Writes to `data/output/`:
   - `mask_v13_seed1.csv` — integer grid, row-major, one value per 11px cell
   - `mask_v13_seed1.json` — legend, cell size, dims, per-domain cell counts
   - `mask_v13_seed1.png` — color visualization of the mask

   Cell values: `-1` BLOCKED (wall/void), `0` CORRIDOR (shared walkable),
   `1..7` DOMAIN (a specific room, walkable, activity-restricted later).

2. **pathfind.py** — loads the mask, runs:
   - connected-component flood fill (must be **1** walkable component)
   - all-domain-pairs reachability via A* (must be **21/21**)
   - 200 random walkable->walkable A* trips
   Writes `paths_v13_seed1.png` (sample routes) + `pathlog_v13_seed1.csv`.

3. **build_frontend.py** — bakes the mask into `index.html`: a self-contained
   interactive page that runs A* in-browser and animates **one agent** making
   endless random domain->domain trips. No server code needed in the page.

## Run

```bash
cd testcases/sandbox/domain_extraction
python3 segment.py          # build mask  -> data/output/
python3 pathfind.py         # prove navigation (prints PASS=True)
python3 build_frontend.py   # build index.html
python3 -m http.server 8731 # then open http://localhost:8731
```

Requires: numpy, scipy, Pillow (already in the miniforge env).

## Pass criteria (all met)

- 7 domains + corridor extracted, walls/void blocked
- mask is **1** connected walkable component (doorways carved)
- **21/21** domain pairs mutually reachable
- 200/200 random A* paths succeed (avg len ~106 cells)
- front-end: 300/300 in-browser A* trips, all 42 ordered domain pairs covered,
  agent visibly walks corridors between rooms

## Tuning

In `segment.py`: `CELL` (px per cell), `MATCH_THRESH` (palette match distance),
`carve_doorways(gap=...)` (max doorway gap in cells).
