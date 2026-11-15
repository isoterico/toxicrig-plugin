# Second texture set: tolex, metal grilles, cloths, grunge, brick, rug, stage floor, smoke.
# All tileable. Physical scale (metres per tile) is noted; the engine sets repeat = 1/scale
# because model UVs are world metres.
import numpy as np
from PIL import Image
rng = np.random.default_rng(11)
OUT = "public/tex/"

def save(a, name, mode=None):
    a = np.clip(a, 0, 1)
    Image.fromarray((a * 255).astype(np.uint8), mode).save(OUT + name, quality=90) if name.endswith(".jpg") else \
        Image.fromarray((a * 255).astype(np.uint8), mode).save(OUT + name, optimize=True)

def vnoise(h, w, fy, fx, octaves=4, gain=0.55):
    out = np.zeros((h, w)); amp = 1
    for o in range(octaves):
        gy, gx = max(1, int(fy * 2 ** o)), max(1, int(fx * 2 ** o))
        g = rng.random((gy, gx))
        ys = np.linspace(0, gy, h, endpoint=False); xs = np.linspace(0, gx, w, endpoint=False)
        y0 = ys.astype(int); x0 = xs.astype(int)
        ty = (ys - y0)[:, None]; tx = (xs - x0)[None, :]
        ty = ty * ty * (3 - 2 * ty); tx = tx * tx * (3 - 2 * tx)
        A = g[y0 % gy][:, x0 % gx]; B = g[y0 % gy][:, (x0 + 1) % gx]; C = g[(y0 + 1) % gy][:, x0 % gx]; D = g[(y0 + 1) % gy][:, (x0 + 1) % gx]
        out += amp * ((A * (1 - tx) + B * tx) * (1 - ty) + (C * (1 - tx) + D * tx) * ty); amp *= gain
    return (out - out.min()) / (np.ptp(out) + 1e-9)

def normal_from(h, strength):
    gx = np.roll(h, -1, 1) - np.roll(h, 1, 1); gy = np.roll(h, -1, 0) - np.roll(h, 1, 0)
    n = np.dstack([-gx * strength, gy * strength, np.ones_like(h)]); n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n * 0.5 + 0.5

def worley(h, w, n_pts, seed=0):
    """Periodic Worley F1 distance (cellular pebbles)."""
    r = np.random.default_rng(seed)
    pts = r.random((n_pts, 2)) * [h, w]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    best = np.full((h, w), 1e9, np.float32); second = np.full((h, w), 1e9, np.float32)
    for py, px in pts:
        dy = np.abs(yy - py); dy = np.minimum(dy, h - dy)
        dx = np.abs(xx - px); dx = np.minimum(dx, w - dx)
        d = np.sqrt(dy * dy + dx * dx)
        second = np.minimum(second, np.maximum(best, d)); best = np.minimum(best, d)
    return best, second

# --- Tolex (pebbled vinyl). 1024 px = 0.12 m.
T = 1024
f1, f2 = worley(T, T, 2600, 4)
peb = np.clip((f2 - f1) / 9.0, 0, 1)                   # ridges between pebbles are low
h = peb ** 0.6 + vnoise(T, T, 8, 8, 3) * 0.25
save(normal_from(h, 1.6), "tolex_nrm.png", "RGB")
rough = 0.62 - 0.22 * peb + 0.08 * vnoise(T, T, 6, 6, 3)
save(np.repeat(rough[..., None], 3, 2), "tolex_rough.jpg", "RGB")
c = 0.022 + 0.012 * peb + 0.01 * vnoise(T, T, 4, 4, 3)
save(np.dstack([c, c * 0.98, c * 0.96]) ** (1 / 2.2), "tolex_col.jpg", "RGB")

# --- Hex perforated steel: RGBA, alpha = metal. Tile = 3 x sqrt(3) hex sides (s = 3.2 mm).
S = 64; Wt, Ht = 3 * S, int(round(np.sqrt(3) * S))
yy, xx = np.mgrid[0:Ht, 0:Wt].astype(np.float32) + 0.5
def hexdist(px, py, cx, cy):
    x, y = px - cx, py - cy
    return np.maximum(np.abs(y), np.maximum(np.abs(x * np.sqrt(3) / 2 + y / 2), np.abs(x * np.sqrt(3) / 2 - y / 2)))
centres = [(0, 0), (Wt, 0), (0, Ht), (Wt, Ht), (Wt / 2, Ht / 2)]
d = np.min([hexdist(xx, yy, cx, cy) for cx, cy in centres], axis=0)
AP = S * np.sqrt(3) / 2
hole = d < AP * 0.8
alpha = np.where(hole, 0.0, 1.0)
edge = np.clip((d - AP * 0.8) / (AP * 0.15), 0, 1)
val = 0.18 + 0.25 * edge
img = np.dstack([val, val, val * 1.02, alpha])
save(img, "hex.png", "RGBA")

# --- Silver grille cloth (American): woven, horizontal silver threads with black weft. Tile = 8 mm.
T = 256
y = np.arange(T)[:, None] / T; x = np.arange(T)[None, :] / T
warp = (np.sin(x * np.pi * 2 * 16) * 0.5 + 0.5)          # vertical threads
weft = (np.sin(y * np.pi * 2 * 16) * 0.5 + 0.5)
over = ((np.floor(x * 16) + np.floor(y * 16)) % 2)
silver = 0.55 + 0.35 * warp * (over > 0)
band = (np.floor(y * 16) % 4 == 0)
col = np.where(band, 0.12 + 0.2 * weft, silver * (0.7 + 0.3 * weft))
holes = ((np.sin(x * np.pi * 2 * 16) > 0.86) & (np.sin(y * np.pi * 2 * 16) > 0.86))
a = np.where(holes, 0.15, 1.0)
save(np.dstack([col, col * 0.99, col * 0.96, a]), "silvercloth.png", "RGBA")
save(normal_from(warp * 0.5 + weft * 0.5, 3.0), "cloth_nrm.png", "RGB")

# --- Dark basket-weave cloth (British). Tile = 8 mm.
check = ((np.floor(x * 8) + np.floor(y * 8)) % 2)
a1 = np.abs(np.sin(x * np.pi * 8)); b1 = np.abs(np.sin(y * np.pi * 8))
hh = np.where(check > 0, a1 * 0.7 + 0.3 * b1, b1 * 0.7 + 0.3 * a1)
c = 0.05 + 0.13 * hh
holes = (a1 < 0.12) & (b1 < 0.12)
save(np.dstack([c * 1.05, c * 0.95, c * 0.82, np.where(holes, 0.25, 1.0)]), "brit_cloth.png", "RGBA")

# --- Grunge for roughness variation (fingerprints, dust). 2048 = 1 m.
T = 1024
g = vnoise(T, T, 3, 3, 6, 0.6)
spots = np.clip((vnoise(T, T, 24, 24, 3) - 0.62) * 4, 0, 1)
streak = vnoise(T, T, 1, 30, 4)
grunge = np.clip(0.5 + 0.35 * (g - 0.5) + 0.3 * spots - 0.15 * (streak - 0.5), 0, 1)
save(np.repeat(grunge[..., None], 3, 2), "grunge.jpg", "RGB")

# --- Orange-peel paint normal for pedals. 512 = 4 cm.
T = 512
peel = vnoise(T, T, 40, 40, 3, 0.5)
save(normal_from(peel, 0.9), "peel_nrm.png", "RGB")

# --- Brick wall. 1024 x 1024 = 1.2 m.
T = 1024
rows = 16; bh = T // rows; bw = bh * 2.1
yy, xx = np.mgrid[0:T, 0:T].astype(np.float32)
r = (yy // bh).astype(int)
xs = (xx + (r % 2) * bw / 2) % T
bx = xs % bw; by = yy % bh
mortar = (bx < 5) | (by < 5)
brick_id = (r * 97 + (xs // bw).astype(int) * 13) % 17
tone = 0.6 + 0.4 * ((brick_id * 0.37) % 1)
n = vnoise(T, T, 30, 30, 4)
colb = np.dstack([0.18 * tone, 0.075 * tone, 0.05 * tone]) * (0.75 + 0.5 * n[..., None])
colm = np.array([0.09, 0.085, 0.08]) * np.ones((T, T, 3)) * (0.7 + 0.5 * n[..., None])
col = np.where(mortar[..., None], colm, colb)
soot = vnoise(T, T, 2, 2, 4)
col *= (0.55 + 0.45 * soot[..., None])
save(col ** (1 / 2.2), "brick_col.jpg", "RGB")
hgt = np.where(mortar, 0.0, 0.6 + 0.4 * n)
save(normal_from(hgt, 2.0), "brick_nrm.jpg", "RGB")

# --- Rug (worn oriental style, original pattern). 1024 x 1536 = whole rug.
H, W = 1536, 1024
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
u = xx / W; v = yy / H
border = np.minimum(np.minimum(u, 1 - u) * W, np.minimum(v, 1 - v) * H)
red = np.array([0.30, 0.035, 0.03]); navy = np.array([0.03, 0.04, 0.10]); cream = np.array([0.55, 0.45, 0.32]); gold = np.array([0.45, 0.28, 0.08])
col = np.ones((H, W, 3)) * red
cu, cv = u - 0.5, (v - 0.5) * 1.5
rr = np.sqrt(cu ** 2 + cv ** 2); th = np.arctan2(cv, cu)
med = (np.cos(th * 8) * 0.06 + 0.22 > rr)
col = np.where(med[..., None], navy, col)
ring = (np.abs(rr - (np.cos(th * 8) * 0.06 + 0.22)) < 0.012)
col = np.where(ring[..., None], cream, col)
inner = (np.cos(th * 6) * 0.03 + 0.1 > rr)
col = np.where(inner[..., None], red * 1.3, col)
motif = (np.sin(u * 60) * np.sin(v * 90) > 0.75) & ~med
col = np.where(motif[..., None], gold, col)
for lo, hi, cc in ((10, 60, navy), (60, 72, cream), (72, 110, red * 0.8), (110, 118, gold)):
    m = (border >= lo) & (border < hi)
    col = np.where(m[..., None], cc, col)
bm = (border >= 10) & (border < 60) & (np.sin((u + v) * 160) > 0.6)
col = np.where(bm[..., None], gold, col)
col = np.where((border < 10)[..., None], cream * 0.8, col)
fiber = vnoise(H, W, 300, 200, 2)
wear = vnoise(H, W, 3, 2, 5)
col = col * (0.75 + 0.35 * fiber[..., None]) * (0.6 + 0.5 * wear[..., None])
save(col ** (1 / 2.2), "rug_col.jpg", "RGB")
save(normal_from(fiber, 2.5), "rug_nrm.jpg", "RGB")

# --- Stage floor: worn black-stained planks. 2048 = 2 m.
T = 2048
x = np.arange(T)[None, :] / T
plank = np.floor(x * 12)
grain = vnoise(T, T, 2, 60, 5) * np.ones((T, 1))
tone = (0.75 + 0.25 * ((plank * 0.618) % 1))
scuff = vnoise(T, T, 6, 6, 5)
v = (0.55 + 0.45 * grain) * tone * (0.6 + 0.6 * scuff)
gap = (np.abs((x * 12) % 1) < 0.004) | (np.abs((x * 12) % 1) > 0.996)
v = np.where(np.broadcast_to(gap, v.shape), 0.05, v)
col = np.dstack([v * 0.07, v * 0.058, v * 0.05])
save(col ** (1 / 2.2), "stage_col.jpg", "RGB")
save(np.repeat((0.35 + 0.45 * scuff)[..., None], 3, 2), "stage_rough.jpg", "RGB")
save(normal_from(grain * 0.4 + np.broadcast_to(gap, v.shape) * -1.0, 1.5), "stage_nrm.jpg", "RGB")

# --- Smoke sprite (RGBA, soft). 512.
T = 512
yy, xx = np.mgrid[0:T, 0:T] / T - 0.5
rad = np.sqrt(xx ** 2 + yy ** 2)
n = vnoise(T, T, 4, 4, 6)
a = np.clip((0.5 - rad) * 2.2, 0, 1) ** 1.5 * (0.3 + 0.7 * n)
save(np.dstack([np.ones_like(a), np.ones_like(a), np.ones_like(a), a]), "smoke.png", "RGBA")
print("ok")
