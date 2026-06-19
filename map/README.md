# map/ — the world the agents navigate

## Naming convention: `layer_<N>_*.png`

The map is a **stack of layers**. The number in the filename **is the paint
order**: `layer_1` sits farthest back, every higher number paints on top of the
ones below it. So you can tell at a glance which layer is behind which.

```
layer_1_background.png    (back)  nav layer — the app extracts its walkable grid from THIS
layer_2_map_overview.png          annotated overview of how the ABM reads that grid
layer_3_enviroment.png            hand-drawn art, on top of the background
layer_5_coordinates.png   (front) reference overlay: every grid cell stamped (x,y)
```

`layer_4_*` and loose `test*.png` are scratch/experiments — not part of the
canonical stack.

```
map/
├── mapgen_poc.ipynb         # proof-of-concept notebook: how the approach was figured out
├── map_generator.py         # THE generator. Run it -> layer_1 + layer_2 + layer_5
├── layer_1_background.png    # nav layer (generated). App extracts the walkable grid from this.
├── layer_2_map_overview.png  # annotated overview (generated)
├── layer_3_enviroment.png    # detailed art (hand-drawn, NOT generated)
├── layer_5_coordinates.png   # per-cell (x,y) reference overlay (generated)
└── README.md
```

`mapgen_poc.ipynb` is the proving ground. `map_generator.py` is the real thing
the project uses.

## Generate the layers

```
python3 map/map_generator.py
```

Finds the canonical map (first fully-connected, isolation-valid seed) and writes:
- `layer_1_background.png` — the navigation layer the app reads.
- `layer_2_map_overview.png` — annotated overview: every cell numbered by its
  domain id, a big number + name at the top-left of each domain, walkable vs blocked.
- `layer_5_coordinates.png` — reference overlay: every nav-grid cell stamped with
  its own `(x,y)` in white. The cells are dimmed so the text stays legible; the
  numbers are tiny on purpose — **zoom in to read them**. Origin `(0,0)` is the
  top-left cell, `x` grows right, `y` grows down. Same cell size and dimensions
  as `layer_2_map_overview.png`, so it lines up pixel-for-pixel with the other
  layers. This is the SAME grid the agents walk (see below).

`layer_3_enviroment.png` is hand art and is not generated.

## How the app uses it

On load the app reads `map/layer_1_background.png`, classifies each pixel by
nearest domain color, dilates the thin corridor lines so hallways stay connected,
and downsamples to the navigation grid (`src/Env.cpp` `loadMaskFromImage`,
`targetCols=176`). It then draws the layers and the first top-left button cycles
the view through them.

`layer_2_map_overview.png` reproduces that exact extraction so the overview
matches what the model sees: `x`/dark = BLOCKED (wall/void), `0` = walkable
corridor (routes), `1..7` = walkable domains. `layer_5_coordinates.png` labels
that same `176 × 112` grid cell-by-cell.

## Domain colors / names

| id | name     | meaning           |
|----|----------|-------------------|
| -1 | BLOCKED  | wall / void       |
| 0  | corridor | walkable hallway  |
| 1  | purple   | walkable domain   |
| 2  | slate    | walkable domain   |
| 3  | gold     | walkable domain   |
| 4  | blue     | walkable domain (center) |
| 5  | pink     | walkable domain   |
| 6  | teal     | walkable domain   |
| 7  | orange   | walkable domain   |
</content>
</invoke>
