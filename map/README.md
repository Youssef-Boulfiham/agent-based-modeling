# map/ — the world the agents navigate

```
map/
├── mapgen_poc.ipynb    # proof-of-concept notebook: how the map approach was
│                       #   figured out + where you can eyeball that it works
├── map_generator.py    # THE generator. Run it -> background.png + map_overview.png
├── background.png       # navigation layer: color-coded domains + tan corridor.
│                        #   The app extracts the walkable grid from THIS image.
├── enviroment.png       # detailed art layer, drawn on top of the background (hand art)
├── map_overview.png     # annotated overview of how the ABM reads the map (generated)
└── README.md
```

`mapgen_poc.ipynb` is the proving ground. `map_generator.py` is the real thing
the project uses.

## Generate the layers

```
python3 map/map_generator.py
```

Finds the canonical map (first fully-connected, isolation-valid seed) and writes:
- `background.png` — the navigation layer the app reads.
- `map_overview.png` — annotated overview: every cell numbered by its domain id,
  a big number + name at the top-left of each domain, walkable vs blocked.

`enviroment.png` is hand art and is not generated.

## How the app uses it

On load the app reads `map/background.png`, classifies each pixel by nearest
domain color, dilates the thin corridor lines so hallways stay connected, and
downsamples to the navigation grid (`src/Env.cpp` `loadMaskFromImage`). It then
draws `background.png` + `enviroment.png` as the two visual layers. The first
top-left button cycles the view through the three layers.

`map_overview.png` reproduces that exact extraction so the overview matches what
the model sees: `x`/dark = BLOCKED (wall/void), `0` = walkable corridor (routes),
`1..7` = walkable domains.

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
