# Tileable procedural textures for the real-time stage (numpy only).
import numpy as np
from PIL import Image
rng = np.random.default_rng(3)
def fbm(h, w, octaves=5, base=4):
    out = np.zeros((h, w))
    amp = 1.0
    for o in range(octaves):
        fy, fx = base * 2 ** o, base * 2 ** o
        g = rng.standard_normal((fy, fx))
        # periodic bilinear upsample via FFT-free approach: tile + interpolate
        ys = np.linspace(0, fy, h, endpoint=False); xs = np.linspace(0, fx, w, endpoint=False)
        y0 = np.floor(ys).astype(int); x0 = np.floor(xs).astype(int)
        ty = (ys - y0)[:, None]; tx = (xs - x0)[None, :]
        ty = ty * ty * (3 - 2 * ty); tx = tx * tx * (3 - 2 * tx)
        a = g[y0 % fy][:, x0 % fx]; b = g[y0 % fy][:, (x0 + 1) % fx]
        c = g[(y0 + 1) % fy][:, x0 % fx]; d = g[(y0 + 1) % fy][:, (x0 + 1) % fx]
        out += amp * ((a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty)
        amp *= 0.5
    return (out - out.min()) / (out.max() - out.min())

def save(arr, name, mode="RGB"):
    Image.fromarray(np.clip(arr * 255, 0, 255).astype(np.uint8), mode).save(f"public/tex/{name}")

# --- walnut: grain runs along U (horizontal). 2048 x 1024 covers 0.8 m x 0.4 m.
def afbm(h, w, fx, fy, octaves=5):
    """Anisotropic periodic value-noise fbm: fx cells across, fy cells down."""
    out = np.zeros((h, w)); amp = 1.0
    for o in range(octaves):
        gx, gy = max(1, int(fx * 2 ** o)), max(1, int(fy * 2 ** o))
        g = rng.standard_normal((gy, gx))
        ys = np.linspace(0, gy, h, endpoint=False); xs = np.linspace(0, gx, w, endpoint=False)
        y0 = np.floor(ys).astype(int); x0 = np.floor(xs).astype(int)
        ty = (ys - y0)[:, None]; tx = (xs - x0)[None, :]
        ty = ty * ty * (3 - 2 * ty); tx = tx * tx * (3 - 2 * tx)
        A = g[y0 % gy][:, x0 % gx]; B = g[y0 % gy][:, (x0 + 1) % gx]
        C = g[(y0 + 1) % gy][:, x0 % gx]; D = g[(y0 + 1) % gy][:, (x0 + 1) % gx]
        out += amp * ((A * (1 - tx) + B * tx) * (1 - ty) + (C * (1 - tx) + D * tx) * ty); amp *= 0.55
    return (out - out.min()) / (out.max() - out.min())
H, W = 1024, 2048
y = np.arange(H)[:, None] / H
warp = afbm(H, W, 2, 3, 4) * 0.9 + afbm(H, W, 1, 1, 2) * 0.6      # slow wandering of the growth rings
dens = afbm(H, W, 1, 4, 3)
phase = y * 16 + warp * 4.5 + dens * 6.0
ringf = phase % 1.0
# Growth ring profile: light early wood that darkens sharply into late wood.
late = np.clip((ringf - 0.55) / 0.45, 0, 1) ** 1.8
lines = afbm(H, W, 3, 220, 3)                                         # fine dark grain lines
lines = np.clip((lines - 0.62) * 4, 0, 1)
fig = afbm(H, W, 6, 40, 4)                                             # figure / colour variation
streaks = np.clip((afbm(H, W, 2, 10, 4) - 0.45) * 2.2, 0, 1)
v = 0.80 - 0.45 * late - 0.3 * lines + 0.45 * (fig - 0.5) - 0.38 * streaks
v = np.clip(v, 0, 1)
pores = (afbm(H, W, 40, 900, 1) > 0.86) * 1.0
dark = np.array([0.075, 0.036, 0.018]); light = np.array([0.40, 0.205, 0.10])
col = dark + (light - dark) * v[..., None]
col = col * (1 - 0.3 * pores[..., None])
save(col ** (1 / 2.2), "walnut_col.jpg")
save(np.repeat((0.30 + 0.25 * (1 - v) + 0.2 * pores)[..., None], 3, 2), "walnut_rough.jpg")
hgt = v * 0.5 - pores * 0.5
gx = np.roll(hgt, -1, 1) - np.roll(hgt, 1, 1); gy = np.roll(hgt, -1, 0) - np.roll(hgt, 1, 0)
n = np.dstack([-gx * 2.0, -gy * 2.0, np.ones_like(gx)]); n /= np.linalg.norm(n, axis=2, keepdims=True)
save(n * 0.5 + 0.5, "walnut_nrm.jpg")

# --- brushed metal: fine streaks along U.
H, W = 512, 1024
st = rng.standard_normal((H, 1)) * 0.6 + fbm(H, 8, 3, 4)[:, :1] * 0
st = np.repeat(st, W, 1) + fbm(H, W, 3, 2) * 0.8
st = (st - st.min()) / (st.max() - st.min())
save(np.repeat((0.18 + 0.22 * st)[..., None], 3, 2), "brushed_rough.jpg")
gy = np.roll(st, -1, 0) - np.roll(st, 1, 0)
n = np.dstack([np.zeros_like(gy), -gy * 0.6, np.ones_like(gy)]); n /= np.linalg.norm(n, axis=2, keepdims=True)
save(n * 0.5 + 0.5, "brushed_nrm.jpg")

# --- grille cloth: basket weave, 256 px tile = 1 cm.
T = 256
u = np.arange(T)[None, :] / T; w = np.arange(T)[:, None] / T
a = np.abs(np.sin(u * np.pi * 4)) ; b = np.abs(np.sin(w * np.pi * 4))
check = ((np.floor(u * 4) + np.floor(w * 4)) % 2)[...]
h = np.where(check > 0, a * 0.7 + 0.3 * b, b * 0.7 + 0.3 * a)
c = 0.04 + 0.12 * h
col = np.dstack([c * 1.0, c * 0.93, c * 0.85])
save(col ** (1 / 2.2), "grille_col.jpg")
gx = np.roll(h, -1, 1) - np.roll(h, 1, 1); gy = np.roll(h, -1, 0) - np.roll(h, 1, 0)
n = np.dstack([-gx * 6, -gy * 6, np.ones_like(gx)]); n /= np.linalg.norm(n, axis=2, keepdims=True)
save(n * 0.5 + 0.5, "grille_nrm.jpg")

# --- floor: dark stage boards (2048 = 2 m).
H = W = 2048
x = np.arange(W)[None, :] / W
board = np.floor(x * 14)
grain = fbm(H, W, 5, 3)
tone = (0.7 + 0.3 * ((board * 0.618) % 1))[0]
v = (0.6 + 0.4 * grain) * tone[None, :] * np.ones((H, 1))
gap = (np.abs(((x * 14) % 1) - 0.0) < 0.006) | (np.abs(((x * 14) % 1) - 1.0) < 0.006)
v = np.where(gap, 0.15, v)
c = np.dstack([v * 0.055, v * 0.05, v * 0.058])
save(c ** (1 / 2.2), "floor_col.jpg")
save(np.repeat((0.42 + 0.3 * (1 - grain))[..., None], 3, 2), "floor_rough.jpg")
print("ok")
