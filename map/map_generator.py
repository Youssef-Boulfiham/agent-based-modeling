#!/usr/bin/env python3
"""map_generator.py — generates the ABM map layers.

The notebook (mapgen_poc.ipynb) is the proof-of-concept where the approach was
figured out and can be eyeballed. THIS file is the actual generator the project
uses. Run it to (re)produce the map layers:

    python3 map/map_generator.py

Layers are named layer_<N>_*.png so the stacking order is obvious: 1 sits
farthest back, higher numbers paint on top. Writes (into map/):
    layer_1_background.png   the nav layer the app extracts its walkable grid from
    layer_2_map_overview.png annotated overview of how the ABM reads that grid
    layer_5_coordinates.png  reference overlay: every nav-grid cell stamped (x,y)

layer_3_enviroment.png is hand art and is not generated here.

The map: a center domain with a corridor frame, six tiled ring domains, and two
corridors per ring domain on its inward sides — validated so every domain is
reachable from every other through the corridors only.
"""
import heapq
import os
import random
from collections import deque
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
BG_OUT    = os.path.join(HERE, "layer_1_background.png")
OVR_OUT   = os.path.join(HERE, "layer_2_map_overview.png")
COORD_OUT = os.path.join(HERE, "layer_5_coordinates.png")

# ── Map model ────────────────────────────────────────────────────────────────
MAP_W, MAP_H = 110, 70
WALL, FLOOR, CORR = 0, 1, 2
WALK = {FLOOR, CORR}
DOMAINS = ["gezondheid", "kennis", "techniek", "geld", "relaties", "richting", "creativiteit"]

# domain topic -> (app domain id, RGB). Colors ARE the app palette so the app's
# pixel classifier maps each area back to the right id (src/Env.cpp).
APP = {"gezondheid": (4, (91, 143, 201)),  "kennis": (1, (176, 127, 201)),
       "techniek":   (2, (122, 138, 160)), "geld":   (6, (108, 192, 168)),
       "relaties":   (7, (201, 138,  75)), "richting": (3, (201, 161,  75)),
       "creativiteit": (5, (201,  96, 142))}
NAMES = {0: "corridor", 1: "purple", 2: "slate", 3: "gold",
         4: "blue", 5: "pink", 6: "teal", 7: "orange"}
CORRIDOR_RGB = (204, 193, 173)
VOID_RGB = (17, 15, 12)
SCALE = 48   # px per map cell in background.png (higher = sharper on zoom-in)


# ── Generator (proven layout from the PoC) ───────────────────────────────────
def _fill(g, rx, ry, rw, rh, t):
    for y in range(ry, ry + rh):
        for x in range(rx, rx + rw):
            if 0 <= x < MAP_W and 0 <= y < MAP_H:
                g[y][x] = t


def _free(g, rx, ry, rw, rh, pad):
    if rw < 6 or rh < 6:
        return False
    for y in range(ry - pad, ry + rh + pad):
        for x in range(rx - pad, rx + rw + pad):
            if not (0 <= x < MAP_W and 0 <= y < MAP_H):
                return False
            if g[y][x] != WALL:
                return False
    return True


def _center(r):
    rx, ry, rw, rh = r
    return (rx + rw // 2, ry + rh // 2)


def _line(g, x1, y1, x2, y2):
    if x1 == x2:
        for y in range(min(y1, y2), max(y1, y2) + 1):
            if g[y][x1] == WALL:
                g[y][x1] = CORR
    else:
        for x in range(min(x1, x2), max(x1, x2) + 1):
            if g[y1][x] == WALL:
                g[y1][x] = CORR


def _tap(g, a, maxbend):
    """Carve from one anchor to the nearest corridor, max `maxbend` bends."""
    st0 = (a, (0, 0), 0)
    if not (0 <= a[0] < MAP_W and 0 <= a[1] < MAP_H) or g[a[1]][a[0]] not in (WALL, CORR):
        return False
    oq = [(0, st0)]; came = {st0: None}; g0 = {st0: 0}
    while oq:
        cost, st = heapq.heappop(oq); (cx, cy), pd, bends = st
        if g[cy][cx] == CORR and pd != (0, 0):
            cur = st
            while cur is not None:
                x, y = cur[0]
                if g[y][x] == WALL:
                    g[y][x] = CORR
                cur = came[cur]
            return True
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = cx + dx, cy + dy
            if not (0 <= nx < MAP_W and 0 <= ny < MAP_H) or g[ny][nx] not in (WALL, CORR):
                continue
            nd = (dx, dy); nb = bends + (1 if pd != (0, 0) and nd != pd else 0)
            if nb > maxbend:
                continue
            ns = ((nx, ny), nd, nb); ng = cost + (0.3 if g[ny][nx] == CORR else 1.0)
            if ns not in g0 or ng < g0[ns]:
                g0[ns] = ng; came[ns] = st; heapq.heappush(oq, (ng, ns))
    return False


def generate_layout(seed):
    rng = random.Random(seed)
    g = [[WALL] * MAP_W for _ in range(MAP_H)]
    cxm, cym = MAP_W // 2, MAP_H // 2
    cw, chh = 36, 24; cx0 = cxm - cw // 2; cy0 = cym - chh // 2
    _fill(g, cx0, cy0, cw, chh, FLOOR)
    rooms = [{"room": (cx0, cy0, cw, chh), "topic": DOMAINS[0]}]
    ux, uy = cx0 + cw // 2, cy0 + chh // 2
    Rt, Rb = cy0 - 4, cy0 + chh + 3; Cl, Cr = cx0 - 5, cx0 + cw + 4
    # center: corridors from all sides (frame + a stub per side)
    _line(g, Cl, Rt, Cr, Rt); _line(g, Cl, Rb, Cr, Rb); _line(g, Cl, Rt, Cl, Rb); _line(g, Cr, Rt, Cr, Rb)
    _line(g, ux, cy0 - 1, ux, Rt); _line(g, ux, cy0 + chh, ux, Rb)
    _line(g, cx0 - 1, uy, Cl, uy); _line(g, cx0 + cw, uy, Cr, uy)

    def jit():
        return rng.randint(0, 3)

    def box(x0, y0, x1, y1):
        bw = (x1 - x0) - jit(); bh = (y1 - y0) - jit(); return (x0, y0, max(7, bw), max(7, bh))

    M = 1
    slots = [box(M, M, ux - 1, Rt - 1), box(ux + 2, M, MAP_W - M, Rt - 1),
             box(M, Rb + 2, ux - 1, MAP_H - M), box(ux + 2, Rb + 2, MAP_W - M, MAP_H - M),
             box(M, Rt + 1, Cl - 1, Rb), box(Cr + 2, Rt + 1, MAP_W - M, Rb)]
    for si, (rx, ry, bw, bh) in enumerate(slots):
        if not _free(g, rx, ry, bw, bh, 1):
            return None
        _fill(g, rx, ry, bw, bh, FLOOR)
        rooms.append({"room": (rx, ry, bw, bh), "topic": DOMAINS[1 + si]})

    def clamp(v, lo, hi):
        return max(lo, min(v, hi))

    # per ring domain: 2 corridors on the inward sides, toward the center
    for n in rooms[1:]:
        rx, ry, rw, rh = n["room"]
        outL = rx <= 2; outR = rx + rw >= MAP_W - 3; outT = ry <= 2; outB = ry + rh >= MAP_H - 3
        sides = {}
        if not outT: sides["N"] = (clamp(cxm, rx + 1, rx + rw - 2), ry - 1)
        if not outB: sides["S"] = (clamp(cxm, rx + 1, rx + rw - 2), ry + rh)
        if not outL: sides["W"] = (rx - 1, clamp(cym, ry + 1, ry + rh - 2))
        if not outR: sides["E"] = (rx + rw, clamp(cym, ry + 1, ry + rh - 2))

        def towards(s):
            a = sides[s]; return (a[0] - cxm) ** 2 + (a[1] - cym) ** 2

        made = 0
        for s in sorted(sides, key=towards):
            if made >= 2:
                break
            if _tap(g, sides[s], 3):
                made += 1
        if made < 2:
            return None
    return {"grid": g, "rooms": rooms}


def _astar(g, s, goal):
    if s == goal:
        return True
    seen = {s}; oq = [(0, s)]
    while oq:
        _, c = heapq.heappop(oq); cx, cy = c
        if c == goal:
            return True
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = cx + dx, cy + dy
            if 0 <= nx < MAP_W and 0 <= ny < MAP_H and g[ny][nx] in WALK and (nx, ny) not in seen:
                seen.add((nx, ny)); heapq.heappush(oq, (abs(nx - goal[0]) + abs(ny - goal[1]), (nx, ny)))
    return False


def validate(m):
    """Empty list = every domain reachable from every other through corridors only."""
    g = m["grid"]; rooms = m["rooms"]; n = len(rooms)
    for i in range(n):
        for j in range(n):
            if i == j:
                continue
            g2 = [row[:] for row in g]
            for k, o in enumerate(rooms):
                if k in (i, j):
                    continue
                ox, oy, ow, oh = o["room"]
                for y in range(oy, oy + oh):
                    for x in range(ox, ox + ow):
                        g2[y][x] = WALL
            if not _astar(g2, _center(rooms[i]["room"]), _center(rooms[j]["room"])):
                return ["%s->%s unreachable" % (rooms[i]["topic"], rooms[j]["topic"])]
    return []


def canonical_map():
    """First seed that produces a fully-connected, isolation-valid map."""
    for seed in range(400):
        m = generate_layout(seed)
        if m and not validate(m):
            return m, seed
    raise RuntimeError("no valid map found in 400 seeds")


# ── Layer: background.png ─────────────────────────────────────────────────────
def write_background(m):
    g = m["grid"]
    idg = [[-1 if g[y][x] == WALL else 0 for x in range(MAP_W)] for y in range(MAP_H)]
    for n in m["rooms"]:
        rx, ry, rw, rh = n["room"]; did = APP[n["topic"]][0]
        for y in range(ry, ry + rh):
            for x in range(rx, rx + rw):
                if g[y][x] == FLOOR:
                    idg[y][x] = did
    colormap = {0: CORRIDOR_RGB}
    for _t, (i, c) in APP.items():
        colormap[i] = c
    # Render at logical resolution, then upscale with NEAREST: every cell stays a
    # flat hard-edged block (no interpolation blur) at SCALE x the source pixels.
    base = Image.new("RGB", (MAP_W, MAP_H))
    bpx = base.load()
    for y in range(MAP_H):
        for x in range(MAP_W):
            v = idg[y][x]
            bpx[x, y] = VOID_RGB if v < 0 else colormap[v]
    img = base.resize((MAP_W * SCALE, MAP_H * SCALE), Image.NEAREST)
    img.save(BG_OUT)


# ── Layer: map_overview.png (mirrors the app's grid extraction) ──────────────
# PALETTE: domain id -> (name, RGB).
_APP_BY_ID = {0: CORRIDOR_RGB}
for _t, (_i, _c) in APP.items():
    _APP_BY_ID[_i] = _c
PALETTE = {i: (NAMES[i], _APP_BY_ID[i]) for i in range(8)}
BLOCKED = -1
MATCH_MAX_SQ = 60 * 60
# Scaled with SCALE (16->48, 3x) so the extracted grid is byte-identical to the
# old 16px render: cell stays 0.625 logical cells, dilation stays 0.375 logical
# cells. Must match src/Env.cpp loadMaskFromImage (targetCols=176, R=18).
TARGET_COLS = 176
DILATE_PX = 18
CELL = 48   # px per grid cell in map_overview.png (higher = sharper text/lines)


def _classify(r, g, b):
    best, best_sq = BLOCKED, MATCH_MAX_SQ + 1
    for i, (_, (pr, pg, pb)) in PALETTE.items():
        sq = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
        if sq < best_sq:
            best_sq, best = sq, i
    return best if best_sq <= MATCH_MAX_SQ else BLOCKED


def _extract_grid():
    im = Image.open(BG_OUT).convert("RGB")
    W, H = im.size; px = im.load()
    cell = max(1, W // TARGET_COLS)
    cols, rows = W // cell, H // cell
    cls = [[BLOCKED] * W for _ in range(H)]
    corr = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            d = _classify(*px[x, y]); cls[y][x] = d
            if d == 0:
                corr[y][x] = 1
    R = DILATE_PX
    tmp = [[0] * W for _ in range(H)]
    for y in range(H):
        rc = corr[y]
        for x in range(W):
            tmp[y][x] = 1 if any(rc[xx] for xx in range(max(0, x - R), min(W, x + R + 1))) else 0
    for y in range(H):
        for x in range(W):
            corr[y][x] = 1 if any(tmp[yy][x] for yy in range(max(0, y - R), min(H, y + R + 1))) else 0
    grid = [[BLOCKED] * cols for _ in range(rows)]
    for gy in range(rows):
        for gx in range(cols):
            ys, ye = gy * cell, min(H, gy * cell + cell)
            xs, xe = gx * cell, min(W, gx * cell + cell)
            has_corr = False; count = [0] * 8
            for y in range(ys, ye):
                if has_corr:
                    break
                for x in range(xs, xe):
                    if corr[y][x]:
                        has_corr = True; break
                    dv = cls[y][x]
                    if 1 <= dv <= 7:
                        count[dv] += 1
            if has_corr:
                grid[gy][gx] = 0
            else:
                best = max(range(1, 8), key=lambda d: count[d])
                grid[gy][gx] = best if count[best] > 0 else BLOCKED
    return grid


def _font(sz, bold=False):
    cands = (["/System/Library/Fonts/Supplemental/Arial Bold.ttf"] if bold else []) + \
            ["/System/Library/Fonts/Supplemental/Arial.ttf", "/System/Library/Fonts/Helvetica.ttc"]
    for p in cands:
        if os.path.exists(p):
            return ImageFont.truetype(p, sz)
    return ImageFont.load_default()


def _halo(d, xy, s, fnt, fill, anchor=None, hw=3):
    x, y = xy
    for dx in range(-hw, hw + 1):
        for dy in range(-hw, hw + 1):
            if dx or dy:
                d.text((x + dx, y + dy), s, font=fnt, fill=(0, 0, 0), anchor=anchor)
    d.text((x, y), s, font=fnt, fill=fill, anchor=anchor)


def _topleft(g, rows, cols, target):
    seen = [[False] * cols for _ in range(rows)]; best = None
    for sy in range(rows):
        for sx in range(cols):
            if g[sy][sx] != target or seen[sy][sx]:
                continue
            comp = []; dq = deque([(sy, sx)]); seen[sy][sx] = True
            while dq:
                y, x = dq.popleft(); comp.append((y, x))
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < rows and 0 <= nx < cols and not seen[ny][nx] and g[ny][nx] == target:
                        seen[ny][nx] = True; dq.append((ny, nx))
            if best is None or len(comp) > len(best):
                best = comp
    if not best:
        return None
    return min(x for _, x in best), min(y for y, _ in best)


def write_overview(g):
    # No title, no legend, no margins: image is exactly cols*CELL x rows*CELL so
    # it shares the aspect of layer_1_background / layer_3_enviroment and lines up
    # when the app cycles the layers. `g` is the extracted nav grid.
    rows, cols = len(g), len(g[0])
    im = Image.new("RGB", (cols * CELL, rows * CELL), VOID_RGB); d = ImageDraw.Draw(im)
    f_cell = _font(max(9, CELL - 9)); f_big = _font(int(CELL * 2.2), bold=True)
    for y in range(rows):
        for x in range(cols):
            v = g[y][x]; walk = v >= 0
            x0, y0 = x * CELL, y * CELL
            col = PALETTE[v][1] if walk else VOID_RGB
            d.rectangle([x0, y0, x0 + CELL - 1, y0 + CELL - 1], fill=col, outline=(0, 0, 0))
            d.text((x0 + 2, y0 + 1), str(v) if walk else "x",
                   fill=(0, 0, 0) if walk else (85, 85, 85), font=f_cell)
    for vid in range(1, 8):
        c = _topleft(g, rows, cols, vid)
        if not c:
            continue
        minx, miny = c
        _halo(d, (minx * CELL + CELL * 0.6, miny * CELL + CELL * 0.6),
              f"{vid}  {PALETTE[vid][0]}", f_big, (255, 255, 255), anchor="la")
    im.save(OVR_OUT)


# ── Layer: layer_5_coordinates.png (per-cell nav-grid coordinates) ───────────
def write_coordinates(g):
    # Reference overlay: every nav-grid cell stamped with its own (x, y) — the
    # SAME grid the agents walk (src/Env.cpp loadMaskFromImage, targetCols=176).
    # Same CELL and dimensions as the overview, so it aligns pixel-for-pixel with
    # the other layers. Cells are dimmed to keep the white (x,y) text legible;
    # zoom in to read each coordinate. Origin (0,0) = top-left, x→right, y↓down.
    rows, cols = len(g), len(g[0])
    im = Image.new("RGB", (cols * CELL, rows * CELL), VOID_RGB); d = ImageDraw.Draw(im)
    f = _font(max(9, int(CELL * 0.23)), bold=True)
    for y in range(rows):
        for x in range(cols):
            v = g[y][x]; walk = v >= 0
            x0, y0 = x * CELL, y * CELL
            base = PALETTE[v][1] if walk else VOID_RGB
            dim = tuple(c // 3 for c in base)  # dim the cell so coord text reads
            d.rectangle([x0, y0, x0 + CELL - 1, y0 + CELL - 1], fill=dim, outline=(0, 0, 0))
            _halo(d, (x0 + CELL // 2, y0 + CELL // 2), f"{x},{y}", f,
                  (255, 255, 255), anchor="mm", hw=2)
    im.save(COORD_OUT)


def main():
    m, seed = canonical_map()
    write_background(m)
    print("wrote", os.path.relpath(BG_OUT), " (seed", seed, "grid", MAP_W, "x", MAP_H, ")")
    grid = _extract_grid()
    write_overview(grid)
    print("wrote", os.path.relpath(OVR_OUT))
    write_coordinates(grid)
    print("wrote", os.path.relpath(COORD_OUT))


if __name__ == "__main__":
    main()
