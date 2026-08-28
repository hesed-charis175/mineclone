import math
import random

from PIL import Image

TILE = 16
COLS, ROWS = 8, 8 
random.seed(42)


def noisy_tile(base_color, variation=18, speckle_color=None, speckle_chance=0.0):
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    for y in range(TILE):
        for x in range(TILE):
            r, g, b = base_color
            d = random.randint(-variation, variation)
            r = max(0, min(255, r + d))
            g = max(0, min(255, g + d))
            b = max(0, min(255, b + d))
            if speckle_color and random.random() < speckle_chance:
                r, g, b = speckle_color
            px[x, y] = (r, g, b, 255)
    return img


def grass_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    for y in range(TILE):
        for x in range(TILE):
            if y < 5:
                base = (86, 158, 58)
            elif y == 5:
                base = (74, 120, 52)
            else:
                base = (121, 85, 58)
            d = random.randint(-16, 16)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    return img


def refined_stone_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    blotch = [[random.randint(-14, 14) for _ in range(5)] for _ in range(5)]

    def blotch_at(x, y):
        bx, by = x / TILE * 4, y / TILE * 4
        x0, y0 = int(bx), int(by)
        fx, fy = bx - x0, by - y0
        a = blotch[y0][x0] * (1 - fx) + blotch[y0][x0 + 1] * fx
        b = blotch[y0 + 1][x0] * (1 - fx) + blotch[y0 + 1][x0 + 1] * fx
        return a * (1 - fy) + b * fy

    for y in range(TILE):
        for x in range(TILE):
            base = 102
            v = base + blotch_at(x, y) + random.randint(-10, 10)
            v = max(0, min(255, int(v)))
            r, g, b = v, v, v + 3
            if random.random() < 0.09:
                r, g, b = 62, 62, 66
            px[x, y] = (r, g, b, 255)

    for _ in range(2):
        cx, cy = random.randint(2, TILE - 3), random.randint(2, TILE - 3)
        length = random.randint(4, 7)
        angle = random.uniform(0, math.pi)
        for t in range(length):
            x = int(cx + math.cos(angle) * t)
            y = int(cy + math.sin(angle) * t)
            if 0 <= x < TILE and 0 <= y < TILE:
                px[x, y] = (70, 70, 74, 255)
    return img



def log_top_tile(ring_colors, band_px=2):
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    cx, cy = TILE / 2 - 0.5, TILE / 2 - 0.5
    for y in range(TILE):
        for x in range(TILE):
            dist = max(abs(x - cx), abs(y - cy))
            ring = (int(dist) // band_px) % len(ring_colors)
            base = ring_colors[ring]
            d = random.randint(-6, 6)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    return img


def log_side_tile(palette, fleck_color, fleck_chance=0.06):
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    col_base = [random.choice(palette) for _ in range(TILE)]
    for y in range(TILE):
        for x in range(TILE):
            base = col_base[x]
            d = random.randint(-14, 14)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            if random.random() < fleck_chance:
                r, g, b = fleck_color
            px[x, y] = (r, g, b, 255)
    return img


def leaves_tile(leaf_colors, density=0.12):
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    px = img.load()
    cells = 4
    cellSize = TILE / cells
    blobs = []
    for cy in range(cells):
        for cx in range(cells):
            if random.random() < density:
                continue
            bx = cx * cellSize + random.uniform(cellSize * 0.25, cellSize * 0.75)
            by = cy * cellSize + random.uniform(cellSize * 0.25, cellSize * 0.75)
            br = random.uniform(cellSize * 0.55, cellSize * 0.85)
            blobs.append((bx, by, br))
    for y in range(TILE):
        for x in range(TILE):
            inside = any(
                math.hypot(x - bx, y - by) < br for bx, by, br in blobs
            )
            if not inside:
                continue
            r, g, b = random.choice(leaf_colors)
            d = random.randint(-10, 10)
            r, g, b = (max(0, min(255, c + d)) for c in (r, g, b))
            px[x, y] = (r, g, b, 255)
    return img



def spruce_log_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    dark = (54, 38, 26)
    light = (92, 66, 44)
    x = 0
    columns = []
    while x < TILE:
        w = random.choice([1, 1, 2])
        columns.append((x, w, random.random() < 0.55))
        x += w
    for gx, gw, is_dark in columns:
        base = dark if is_dark else light
        for dx in range(gw):
            x = gx + dx
            if x >= TILE:
                continue
            for y in range(TILE):
                d = random.randint(-8, 8)
                r, g, b = (max(0, min(255, c + d)) for c in base)
                px[x, y] = (r, g, b, 255)
    return img


def birch_log_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (232, 226, 212)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-5, 5)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    num_marks = random.randint(8, 11)
    for _ in range(num_marks):
        mx = random.randint(0, TILE - 2)
        my = random.randint(0, TILE - 1)
        mw = random.randint(2, 5)
        mh = 1 if random.random() < 0.75 else 2
        shade = random.choice([(28, 26, 24), (45, 42, 40)])
        for dx in range(mw):
            for dy in range(mh):
                x, y = mx + dx, my + dy
                if 0 <= x < TILE and 0 <= y < TILE:
                    px[x, y] = shade + (255,)
    return img


def acacia_log_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    plate_tones = [(150, 92, 54), (168, 104, 60), (132, 80, 46), (176, 112, 66)]
    num_seeds = 7
    seeds = [
        (random.uniform(0, TILE), random.uniform(0, TILE), random.choice(plate_tones))
        for _ in range(num_seeds)
    ]
    for y in range(TILE):
        for x in range(TILE):
            dists = sorted((math.hypot(x - sx, y - sy), tone) for sx, sy, tone in seeds)
            nearest_d, nearest_tone = dists[0]
            second_d = dists[1][0]
            on_crack = (second_d - nearest_d) < 0.9
            if on_crack:
                r, g, b = 58, 34, 20
            else:
                d = random.randint(-10, 10)
                r, g, b = (max(0, min(255, c + d)) for c in nearest_tone)
            px[x, y] = (r, g, b, 255)
    return img


def jungle_log_side_tile():
    palette = [(110, 76, 46), (90, 60, 36), (70, 46, 28), (100, 70, 42)]
    img = log_side_tile(palette, (50, 32, 18), fleck_chance=0.05)
    px = img.load()
    moss_colors = [(74, 98, 62), (62, 86, 56)]
    for _ in range(5):
        mx, my = random.randint(0, TILE - 1), random.randint(0, TILE - 1)
        mr = 1
        for dx in range(-mr, mr + 1):
            for dy in range(-mr, mr + 1):
                x, y = mx + dx, my + dy
                if 0 <= x < TILE and 0 <= y < TILE and dx * dx + dy * dy <= mr * mr + 1:
                    if random.random() < 0.45:
                        continue
                    er, eg, eb, _ = px[x, y]
                    mr_, mg_, mb_ = random.choice(moss_colors)
                    px[x, y] = ((er + mr_) // 2, (eg + mg_) // 2, (eb + mb_) // 2, 255)
    return img


def cherry_log_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (146, 100, 96)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-10, 10)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    for _ in range(4):
        mx, my = random.randint(0, TILE - 3), random.randint(0, TILE - 1)
        mw = random.randint(2, 3)
        for dx in range(mw):
            x = mx + dx
            if 0 <= x < TILE:
                px[x, my] = (86, 54, 54, 255)
    return img


def mangrove_log_side_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (108, 56, 46)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-10, 10)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    for offset in range(-TILE, TILE, 4):
        dark = random.random() < 0.7
        c = (78, 36, 30) if dark else (128, 70, 58)
        for t in range(TILE):
            y = t + offset
            if 0 <= y < TILE and random.random() < 0.75:
                jitter = random.randint(-1, 1)
                yy = max(0, min(TILE - 1, y + jitter))
                er, eg, eb, _ = px[t, yy]
                px[t, yy] = (
                    (er + c[0] * 2) // 3,
                    (eg + c[1] * 2) // 3,
                    (eb + c[2] * 2) // 3,
                    255,
                )
    return img


def grass_tuft_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    px = img.load()
    base_grey = 232
    num_blades = 6
    for _ in range(num_blades):
        bx = random.uniform(2.0, TILE - 2.0)
        height = random.uniform(TILE * 0.5, TILE * 0.95)
        lean = random.uniform(-2.5, 2.5)
        width = random.choice([1, 1, 2])
        steps = max(2, int(height))
        for t in range(steps):
            frac = t / (steps - 1)
            x = bx + lean * frac
            y = (TILE - 1) - t
            shade = int(base_grey * (0.72 + 0.28 * (1.0 - frac)))
            shade = max(0, min(255, shade + random.randint(-6, 6)))
            for dx in range(width):
                ix = int(round(x)) + dx
                iy = int(round(y))
                if 0 <= ix < TILE and 0 <= iy < TILE:
                    px[ix, iy] = (shade, shade, shade, 255)
    return img


def glass_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    px = img.load()
    for y in range(TILE):
        for x in range(TILE):
            on_border = x in (0, TILE - 1) or y in (0, TILE - 1)
            if on_border:
                px[x, y] = (232, 244, 246, 130)
            else:
                base_a = 55
                if abs((x - y)) < 2 or abs((x - y) - 8) < 2:
                    base_a = 95
                px[x, y] = (196, 226, 230, base_a)
    return img


def plank_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    for y in range(TILE):
        seam = (y % 4) == 0
        for x in range(TILE):
            base = (176, 138, 88)
            d = random.randint(-8, 8) + int(6 * math.sin(x * 0.9 + y * 0.15))
            r, g, b = (max(0, min(255, c + d)) for c in base)
            if seam:
                r, g, b = (120, 90, 56)
            px[x, y] = (r, g, b, 255)
    return img


def light_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    cx, cy = TILE / 2 - 0.5, TILE / 2 - 0.5
    for y in range(TILE):
        for x in range(TILE):
            dist = math.hypot(x - cx, y - cy) / (TILE / 2)
            t = max(0.0, 1.0 - dist)
            r = 255
            g = int(230 + 20 * t)
            b = int(150 + 90 * t)
            px[x, y] = (r, min(255, g), min(255, b), 255)
    return img


def iron_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (196, 198, 202)
    frame = (150, 153, 158)
    for y in range(TILE):
        for x in range(TILE):
            on_frame = x in (0, 1, TILE - 2, TILE - 1) or y in (0, 1, TILE - 2, TILE - 1)
            c = frame if on_frame else base
            d = random.randint(-10, 10)
            r, g, b = (max(0, min(255, ch + d)) for ch in c)
            px[x, y] = (r, g, b, 255)
    for cx, cy in [(3, 3), (TILE - 4, 3), (3, TILE - 4), (TILE - 4, TILE - 4)]:
        for dx in (-1, 0):
            for dy in (-1, 0):
                px[cx + dx, cy + dy] = (170, 172, 176, 255)
    return img


def polished_iron_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    for y in range(TILE):
        for x in range(TILE):
            base = (222, 224, 228)
            grain = int(6 * math.sin(y * 1.3))
            r, g, b = (max(0, min(255, c + grain)) for c in base)
            d = random.randint(-4, 4)
            r, g, b = (max(0, min(255, c + d)) for c in (r, g, b))
            if abs((x - y)) < 2 or abs((x - y) - 9) < 2:
                r, g, b = 250, 251, 253
            px[x, y] = (r, g, b, 255)
    return img


def clay_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (158, 164, 174)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-8, 8)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    return img


def gravel_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    pebble_tones = [(120, 118, 116), (98, 96, 94), (145, 142, 138), (80, 78, 76)]
    cell_tone = [[random.choice(pebble_tones) for _ in range(8)] for _ in range(8)]
    for y in range(TILE):
        for x in range(TILE):
            base = cell_tone[y // 2][x // 2]
            d = random.randint(-14, 14)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    return img


def ice_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (196, 224, 235)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-6, 6)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)
    for _ in range(3):
        cx, cy = random.randint(2, TILE - 3), random.randint(2, TILE - 3)
        length = random.randint(4, 8)
        angle = random.uniform(0, math.pi)
        for t in range(length):
            x = int(cx + math.cos(angle) * t)
            y = int(cy + math.sin(angle) * t)
            if 0 <= x < TILE and 0 <= y < TILE:
                px[x, y] = (225, 245, 250, 255)
    return img


def sandstone_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    band_tones = [(214, 195, 152), (198, 178, 134), (224, 206, 166)]
    for y in range(TILE):
        band = band_tones[(y // 3) % len(band_tones)]
        for x in range(TILE):
            d = random.randint(-8, 8)
            r, g, b = (max(0, min(255, c + d)) for c in band)
            px[x, y] = (r, g, b, 255)
    return img


def magma_tile():
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()
    base = (40, 22, 20)
    for y in range(TILE):
        for x in range(TILE):
            d = random.randint(-6, 6)
            r, g, b = (max(0, min(255, c + d)) for c in base)
            px[x, y] = (r, g, b, 255)

    red_tones = [(190, 40, 18), (210, 50, 20), (170, 30, 14)]
    hot_tones = [(255, 130, 30), (255, 90, 20)] 

    def draw_crack(x, y, length, angle, depth=0):
        for t in range(length):
            ix, iy = int(round(x)), int(round(y))
            if 0 <= ix < TILE and 0 <= iy < TILE:
                color = random.choice(red_tones)
                if random.random() < 0.12:
                    color = random.choice(hot_tones)
                px[ix, iy] = color + (255,)
            angle += random.uniform(-0.35, 0.35)
            x += math.cos(angle)
            y += math.sin(angle)
            if depth < 1 and t > 2 and random.random() < 0.15:
                branch_angle = angle + random.choice([-1, 1]) * random.uniform(0.8, 1.4)
                draw_crack(x, y, max(2, length - t - 1), branch_angle, depth + 1)

    for _ in range(7):
        cx, cy = random.randint(0, TILE - 1), random.randint(0, TILE - 1)
        length = random.randint(6, 11)
        angle = random.uniform(0, math.pi * 2)
        draw_crack(cx, cy, length, angle)

    for _ in range(6):
        cx, cy = random.randint(0, TILE - 1), random.randint(0, TILE - 1)
        length = random.randint(3, 5)
        angle = random.uniform(0, math.pi * 2)
        draw_crack(cx, cy, length, angle)

    return img


def ore_tile(vein_colors, base=(58, 58, 64), cluster_count=4, base_variation=8):
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 255))
    px = img.load()

    num_seeds = 9
    stone_seeds = []
    for _ in range(num_seeds):
        d = random.randint(-base_variation, base_variation)
        tone = tuple(max(0, min(255, c + d)) for c in base)
        stone_seeds.append((random.uniform(0, TILE), random.uniform(0, TILE), tone))

    for y in range(TILE):
        for x in range(TILE):
            dists = sorted((math.hypot(x - sx, y - sy), tone) for sx, sy, tone in stone_seeds)
            nearest_d, nearest_tone = dists[0]
            second_d = dists[1][0]
            on_crack = (second_d - nearest_d) < 0.6
            if on_crack:
                dark_stone = tuple(max(0, min(255, c - 26)) for c in nearest_tone)
                vc = random.choice(vein_colors)
                r, g, b = (
                    int(dark_stone[i] * 0.55 + vc[i] * 0.45) for i in range(3)
                )
            else:
                jd = random.randint(-6, 6)
                r, g, b = (max(0, min(255, c + jd)) for c in nearest_tone)
            px[x, y] = (r, g, b, 255)

    for _ in range(cluster_count):
        ccx, ccy = random.randint(1, TILE - 2), random.randint(1, TILE - 2)
        vein_seeds = []
        num_vein_seeds = random.randint(2, 3)
        for _ in range(num_vein_seeds):
            sx = ccx + random.uniform(-1.2, 1.2)
            sy = ccy + random.uniform(-1.2, 1.2)
            vein_seeds.append((sx, sy))
        reach = 2
        for dy in range(-reach, reach + 1):
            for dx in range(-reach, reach + 1):
                x, y = ccx + dx, ccy + dy
                if not (0 <= x < TILE and 0 <= y < TILE):
                    continue
                nearest = min(math.hypot(x - sx, y - sy) for sx, sy in vein_seeds)
                if nearest < 1.6:
                    color = random.choice(vein_colors)
                    d = random.randint(-8, 8)
                    r, g, b = (max(0, min(255, c + d)) for c in color)
                    px[x, y] = (r, g, b, 255)
        highlight = tuple(min(255, c + 55) for c in random.choice(vein_colors))
        px[ccx, ccy] = highlight + (255,)
    return img


def coal_ore_tile():
    return ore_tile([(26, 26, 28), (16, 16, 18), (36, 36, 38)],
                    base=(52, 52, 56), cluster_count=5)


def iron_ore_tile():
    return ore_tile([(196, 168, 132), (214, 186, 148), (176, 148, 112)],
                    base=(60, 58, 56), cluster_count=4)


def copper_ore_tile():
    return ore_tile([(198, 120, 70), (218, 140, 88), (170, 100, 56)],
                    base=(58, 56, 54), cluster_count=4)


def gold_ore_tile():
    return ore_tile([(246, 208, 60), (255, 224, 92), (216, 178, 40)],
                    base=(58, 56, 54), cluster_count=4)


def diamond_ore_tile():
    return ore_tile([(140, 224, 232), (172, 242, 248), (108, 200, 212)],
                    base=(52, 54, 58), cluster_count=4)


def emerald_ore_tile():
    return ore_tile([(40, 196, 110), (62, 220, 132), (24, 168, 92)],
                    base=(52, 54, 58), cluster_count=4)


def lithium_ore_tile():
    return ore_tile([(178, 120, 226), (202, 152, 242), (150, 96, 202)],
                    base=(56, 54, 60), cluster_count=4)


def uranium_ore_tile():
     return ore_tile([(152, 232, 60), (182, 255, 92), (122, 202, 40)],
                    base=(44, 50, 40), cluster_count=5)



TREE_SPECIES = {
    "oak": dict(
        log_top_rings=[(200, 166, 118), (172, 136, 92)],
        log_top_band=2,
        side_func=lambda: log_side_tile(
            [(120, 88, 55), (98, 70, 44), (84, 58, 36), (106, 76, 48)], (58, 40, 26)),
        leaf_colors=[(58, 122, 46), (72, 140, 56), (46, 104, 38)],
    ),
    "spruce": dict(
        log_top_rings=[(120, 92, 66), (98, 72, 50)],
        log_top_band=1,  # tight rings — dense conifer trunk
        side_func=spruce_log_side_tile,
        leaf_colors=[(38, 84, 58), (28, 68, 46), (48, 96, 66)],
    ),
    "birch": dict(
        log_top_rings=[(226, 220, 206), (206, 198, 182)],
        log_top_band=2,
        side_func=birch_log_side_tile,
        leaf_colors=[(150, 190, 70), (168, 206, 88), (134, 176, 58)],
    ),
    "acacia": dict(
        log_top_rings=[(150, 96, 62), (126, 78, 48)],
        log_top_band=3,  # wide rings — thick trunk
        side_func=acacia_log_side_tile,
        leaf_colors=[(150, 150, 60), (168, 168, 74), (132, 132, 50)],
    ),
    "jungle": dict(
        log_top_rings=[(150, 108, 70), (122, 84, 50)],
        log_top_band=3,  # wide rings — big canopy trunk
        side_func=jungle_log_side_tile,
        leaf_colors=[(30, 110, 40), (22, 90, 32), (40, 128, 50)],
    ),
    "cherry": dict(
        log_top_rings=[(150, 100, 96), (124, 78, 76)],
        log_top_band=2,
        side_func=cherry_log_side_tile,
        leaf_colors=[(244, 178, 202), (250, 200, 216), (232, 156, 184)],
    ),
    "mangrove": dict(
        log_top_rings=[(120, 66, 56), (96, 50, 42)],
        log_top_band=1,  # tight rings — fibrous, thin
        side_func=mangrove_log_side_tile,
        leaf_colors=[(34, 96, 62), (26, 80, 50), (44, 112, 72)],
    ),
}

SPECIES_START_INDEX = {
    "oak": 5,
    "spruce": 18,
    "birch": 21,
    "acacia": 24,
    "jungle": 27,
    "cherry": 30,
    "mangrove": 33,
}

tiles = {
    0: noisy_tile((92, 168, 62), variation=20),
    1: grass_side_tile(),
    2: noisy_tile((134, 96, 60), variation=16),
    3: refined_stone_tile(),
    4: noisy_tile((219, 205, 154), variation=12),
    8: glass_tile(),
    9: plank_tile(),
    10: light_tile(),
    11: iron_tile(),
    12: polished_iron_tile(),
    13: clay_tile(),
    14: gravel_tile(),
    15: ice_tile(),
    16: sandstone_tile(),
    17: magma_tile(),
    36: coal_ore_tile(),
    37: iron_ore_tile(),
    38: copper_ore_tile(),
    39: gold_ore_tile(),
    40: diamond_ore_tile(),
    41: emerald_ore_tile(),
    42: lithium_ore_tile(),
    43: uranium_ore_tile(),
    44: grass_tuft_tile(),
}

for name, spec in TREE_SPECIES.items():
    start = SPECIES_START_INDEX[name]
    tiles[start] = log_top_tile(spec["log_top_rings"], spec["log_top_band"])
    tiles[start + 1] = spec["side_func"]()
    tiles[start + 2] = leaves_tile(spec["leaf_colors"])

atlas = Image.new("RGBA", (TILE * COLS, TILE * ROWS), (0, 0, 0, 0))
for idx, tile_img in tiles.items():
    col = idx % COLS
    row = idx // COLS
    atlas.paste(tile_img, (col * TILE, row * TILE))

atlas.save("assets/atlas.png")
print("[INFO] Wrote assets/atlas.png", atlas.size, f"({len(tiles)} tiles defined)")
