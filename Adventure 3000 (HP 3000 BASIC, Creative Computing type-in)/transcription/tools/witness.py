"""Crop the same spot from the other scans of a page (second witnesses).

The original 600-dpi scan (reading orientation) is registered to the
"better scan" and to the 800-ppi microfilm by matching a grid of patches at
1/4 scale and fitting an affine map; the fit is cached per page.

usage: witness.py page col row cell0 ncells [nrows]
    writes work/witness.png: original / better / microfilm, one above the other
"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
from scipy.signal import fftconvolve
from PIL import Image
import pagegeom as pg
from geom2 import clean_ink
from rowimg import row_strip

S = 4


def gray_small(page, source):
    a = pg.load_gray(page, source)
    if source == "better" and a.shape[0] > a.shape[1]:
        pass
    return a, a[::S, ::S]


def reading(page, source):
    """Grayscale in reading orientation for any source (better scan pages
    are sometimes stored upright, sometimes sideways)."""
    im = Image.open(pg.scan_path(page, source)).convert("L")
    if im.height > im.width:
        im = im.rotate(-90, expand=True)
    return np.asarray(im, np.float32)


def fit(page, source):
    cache = os.path.join(pg.WORK, "reg_p%d_%s.json" % (page, source))
    if os.path.exists(cache):
        return json.load(open(cache))
    A = reading(page, "tif"); B = reading(page, source)
    sa = A[::S, ::S]
    # scale factor from image widths is only approximate; search it
    best_s = None
    pts_a, pts_b = [], []
    ha, wa = sa.shape
    for fy in (0.25, 0.5, 0.75):
        for fx in (0.2, 0.4, 0.6, 0.8):
            y, x = int(ha * fy), int(wa * fx)
            patch = 255 - sa[y - 60:y + 60, x - 90:x + 90]
            if patch.std() < 8:
                continue
            patch = patch - patch.mean()
            bestv = None
            for scale in np.arange(0.85, 1.45, 0.01) if source == "mf" else np.arange(0.95, 1.06, 0.005):
                sb = ndi.zoom(B, 1.0 / (S * scale), order=1) if bestv is None or True else None
                break
            pts_a.append((x * S, y * S))
    # simpler robust approach: global scale by width ratio, then per-patch translation
    ratio = B.shape[1] / A.shape[1]
    candidates = np.arange(ratio * 0.9, ratio * 1.1, 0.005)
    sb_cache = {}
    results = []
    for (xa, ya) in pts_a:
        pa = 255 - A[ya - 240:ya + 240, xa - 360:xa + 360][::S, ::S]
        pa = pa - pa.mean()
        results.append((xa, ya, pa))
    best = None
    for sc in candidates:
        key = round(sc, 4)
        sb = 255 - ndi.zoom(B, 1.0 / (S * sc), order=1)
        score, matches = 0.0, []
        for (xa, ya, pa) in results:
            cy, cx = int(ya / S), int(xa / S)
            win = sb[max(0, cy - 160):cy + 160, max(0, cx - 220):cx + 220]
            if win.shape[0] <= pa.shape[0] or win.shape[1] <= pa.shape[1]:
                continue
            num = fftconvolve(win - win.mean(), pa[::-1, ::-1], mode="valid")
            k = np.unravel_index(np.argmax(num), num.shape)
            v = num[k] / (np.linalg.norm(pa) * (win.std() * np.sqrt(pa.size) + 1e-6))
            score += v
            by = (max(0, cy - 160) + k[0] + pa.shape[0] / 2) * S * sc
            bx = (max(0, cx - 220) + k[1] + pa.shape[1] / 2) * S * sc
            matches.append((xa, ya, bx, by, v))
        if best is None or score > best[0]:
            best = (score, sc, matches)
    score, sc, matches = best
    M = np.array([[m[0], m[1], 1.0] for m in matches])
    bx = np.array([m[2] for m in matches]); by = np.array([m[3] for m in matches])
    cx, *_ = np.linalg.lstsq(M, bx, rcond=None)
    cy, *_ = np.linalg.lstsq(M, by, rcond=None)
    res = {"ax": list(cx), "ay": list(cy), "scale": sc, "score": score,
           "resid": float(np.sqrt(np.mean((M @ cx - bx) ** 2 + (M @ cy - by) ** 2)))}
    json.dump(res, open(cache, "w"))
    return res


def crop_other(page, source, x0, y0, x1, y1, out_h):
    r = fit(page, source)
    B = reading(page, source)
    corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
    bx = [r["ax"][0] * x + r["ax"][1] * y + r["ax"][2] for x, y in corners]
    by = [r["ay"][0] * x + r["ay"][1] * y + r["ay"][2] for x, y in corners]
    c = B[int(min(by)):int(max(by)), int(min(bx)):int(max(bx))]
    im = Image.fromarray(np.clip(c, 0, 255).astype(np.uint8))
    return im.resize((int(im.width * out_h / max(1, im.height)), out_h), Image.LANCZOS)


def main(page, ci, ri, c0, nc, nrows=1):
    g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
    ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
    rows = g["rows"][ri:ri + nrows]
    xa = rows[0]["cells"][min(c0, len(rows[0]["cells"]) - 1)] - 6
    xb = max(r["cells"][min(c0 + nc, len(r["cells"]) - 1)] for r in rows) + 6
    ya = ys[ri] - 34 + g["slope"] * (xa - g["xref"])
    yb = ys[ri + nrows - 1] + 34 + g["slope"] * (xb - g["xref"])
    A = reading(page, "tif")
    ca = A[int(min(ya, yb)):int(max(ya, yb)), int(xa):int(xb)]
    H = ca.shape[0] * 2
    ims = [Image.fromarray(ca.astype(np.uint8)).resize((ca.shape[1] * 2, H), Image.LANCZOS)]
    for src in ("better", "mf"):
        try:
            ims.append(crop_other(page, src, xa, min(ya, yb), xb, max(ya, yb), H))
        except Exception as e:
            print(src, "failed:", e)
    W = max(i.width for i in ims)
    canvas = Image.new("L", (W, sum(i.height + 8 for i in ims)), 255)
    y = 0
    for i in ims:
        canvas.paste(i, (0, y)); y += i.height + 8
    out = os.path.join(pg.WORK, "witness.png")
    canvas.save(out)
    print(out, canvas.size)


if __name__ == "__main__":
    a = [int(v) for v in sys.argv[1:]]
    main(*a)
