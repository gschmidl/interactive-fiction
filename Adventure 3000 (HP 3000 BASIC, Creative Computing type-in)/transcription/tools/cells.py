"""Cut every character cell out of the listing columns.

Each row's vertical position is first refined by correlating its vertical ink
profile with the column's median profile, so all glyphs of a row share one
baseline.  A cell is taken 3 px wider than its DP boundaries on each side and
+-32 px around the refined row centre (the dot glyphs, descenders and
underscores fit), resampled to 42x64 and stored as uint8.

Writes work/cells.npz:  img (N,64,42) uint8, meta (N,5) int32 = page, col, row, k, ink
"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
import pagegeom as pg
from columns import COLUMNS
from geom2 import clean_ink, masked

H, W, MARGIN = 64, 42, 3


def row_vprofile(ink, row, slope, xref, half=30):
    xs = row["cells"]
    xa, xb = int(xs[0]), int(xs[-1])
    prof = np.zeros(2 * half, np.float64)
    step = 40
    for x in range(xa, xb, step):
        yy = int(round(row["y"] + slope * (x + step / 2 - xref)))
        prof += ink[yy - half:yy + half, x:min(x + step, xb)].sum(1)
    return prof


def refine_rows(ink, g):
    profs = [row_vprofile(ink, r, g["slope"], g["xref"]) for r in g["rows"]]
    ref = np.median(np.array([p / max(p.max(), 1e-6) for p in profs]), axis=0)
    shifts = []
    for p in profs:
        best = (-1e18, 0)
        pn = p / max(p.max(), 1e-6)
        for s in range(-6, 7):
            a = pn[max(0, s):len(pn) + min(0, s)]
            b = ref[max(0, -s):len(ref) + min(0, -s)]
            v = float(np.dot(a, b))
            if v > best[0]:
                best = (v, s)
        shifts.append(best[1])
    # centre of the reference profile's ink, so y means "glyph centre"
    c = float(np.sum(np.arange(len(ref)) * ref) / ref.sum()) - len(ref) / 2
    return [r["y"] + s + c for r, s in zip(g["rows"], shifts)]


def extract(page, ci, ink, mink=None):
    g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
    ys = refine_rows(ink if mink is None else mink, g)
    imgs, meta, pos = [], [], []
    for ri, (row, yc) in enumerate(zip(g["rows"], ys)):
        xs = row["cells"]
        for k in range(len(xs) - 1):
            xa, xb = xs[k] - MARGIN, xs[k + 1] + MARGIN
            xm = (xa + xb) / 2
            ym = yc + g["slope"] * (xm - g["xref"])
            # sample on a W x H grid spanning the cell (x) and +-32 px (y)
            gx = np.linspace(xa, xb, W)
            gy = np.arange(H) - H / 2 + ym
            yy, xx = np.meshgrid(gy, gx, indexing="ij")
            patch = ndi.map_coordinates(ink, [yy, xx], order=1, mode="constant")
            imgs.append(np.clip(patch * 255, 0, 255).astype(np.uint8))
            meta.append((page, ci, ri, k, int(patch.sum() * 10)))
            pos.append((xm, ym))
    return imgs, meta, ys, pos


def main():
    all_img, all_meta, all_pos = [], [], []
    rowy = {}
    for page in sorted(COLUMNS):
        ink = clean_ink(page)
        mink = masked(ink, page)      # row heights must not be pulled by art
        for ci in range(len(COLUMNS[page])):
            imgs, meta, ys, pos = extract(page, ci, ink, mink)
            all_img += imgs
            all_meta += meta
            all_pos += pos
            rowy["%d_%d" % (page, ci)] = ys
            print(page, ci, len(imgs))
    np.savez_compressed(os.path.join(pg.WORK, "cells.npz"), img=np.array(all_img), meta=np.array(all_meta, np.int32), pos=np.array(all_pos, np.float32))
    json.dump(rowy, open(os.path.join(pg.WORK, "rowy.json"), "w"))
    print("total", len(all_img))


if __name__ == "__main__":
    main()
