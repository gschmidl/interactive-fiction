"""Find text rows and the fixed character grid of each listing column.

Output per column (.work/geom_p<page>_c<col>.json):
    slope   dy/dx of the text rows (small rotation, handled as a shear)
    pitch   character pitch in px, phase = x of cell 0's left edge
    rows    list of row centre y at x = xref
"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
from scipy.signal import find_peaks
import pagegeom as pg
from columns import COLUMNS, ART

STRIP = 60


def page_ink(page):
    path = os.path.join(pg.WORK, "p%d_ink_raw.npy" % page)
    if os.path.exists(path):
        return np.load(path).astype(np.float32)
    ink = pg.ink_map(pg.load_gray(page))
    np.save(path, ink.astype(np.float16))
    return ink


def sheared_profile(sub, slope, xref):
    """Row profile of sub after removing a shear y -> y - slope*(x - xref)."""
    h, w = sub.shape
    prof = np.zeros(h, np.float64)
    ys = np.arange(h, dtype=np.float64)
    for sx in range(0, w, STRIP):
        strip = ndi.gaussian_filter1d(sub[:, sx:sx + STRIP].sum(1).astype(np.float64), 3.0)
        dy = slope * (sx + STRIP / 2 - xref)
        prof += np.interp(ys + dy, ys, strip, left=0, right=0)
    return prof


def fit_column(page, ci):
    ink = page_ink(page)
    x0, x1, y0, y1 = COLUMNS[page][ci]
    sub = ink[y0:y1, x0:x1]
    # only use the left 40 cells for the shear and rows: long lines in the column
    # to the left can overhang into this one, and art sits on the right
    left = sub[:, : min(sub.shape[1], 1500)]
    xref = 0.0
    best = (-1, 0.0)
    for slope in np.arange(-0.02, 0.0201, 0.0005):
        p = sheared_profile(left, slope, xref)
        v = float(np.sum(np.diff(p) ** 2))
        if v > best[0]:
            best = (v, float(slope))
    slope = best[1]
    for slope2 in np.arange(slope - 0.0005, slope + 0.00051, 0.0001):
        p = sheared_profile(left, slope2, xref)
        v = float(np.sum(np.diff(p) ** 2))
        if v > best[0]:
            best = (v, float(slope2))
    slope = best[1]
    prof = ndi.gaussian_filter1d(sheared_profile(left, slope, xref), 5)
    peaks, props = find_peaks(prof, distance=40, height=prof.max() * 0.06, prominence=prof.max() * 0.04)
    rows = [float(y0 + p) for p in peaks]
    # character grid: fold the column profile of all row bands
    colprof = np.zeros(sub.shape[1], np.float64)
    for yc in rows:
        for sx in range(0, sub.shape[1], STRIP):
            yy = int(round(yc - y0 + slope * (sx + STRIP / 2 - xref)))
            a, b = max(0, yy - 22), min(sub.shape[0], yy + 22)
            colprof[sx:sx + STRIP] += sub[a:b, sx:sx + STRIP].sum(0)
    xs = np.arange(len(colprof), dtype=np.float64)
    best = (-1, 0, 0)
    for pitch in np.arange(35.0, 37.0, 0.005):
        ph = (xs % pitch) / pitch * 2 * np.pi
        c = np.sum(colprof * np.cos(ph)); s = np.sum(colprof * np.sin(ph))
        amp = np.hypot(c, s)
        if amp > best[0]:
            best = (amp, pitch, np.arctan2(s, c))
    amp, pitch, ang = best
    # ang is the phase of maximum ink; cell edges sit half a pitch away
    centre = (ang % (2 * np.pi)) / (2 * np.pi) * pitch
    edge = (centre + pitch / 2) % pitch
    return {"page": page, "col": ci, "box": [x0, x1, y0, y1], "slope": slope,
            "xref": x0 + xref, "pitch": float(pitch), "edge": float(x0 + edge),
            "rows": rows}


def main(pages):
    for page in pages:
        for ci in range(len(COLUMNS[page])):
            g = fit_column(page, ci)
            json.dump(g, open(os.path.join(pg.WORK, "geom_p%d_c%d.json" % (page, ci)), "w"), indent=1)
            d = np.diff(g["rows"])
            print("p%d c%d slope %.4f pitch %.3f edge %.1f rows %d  row gaps min %.0f med %.0f max %.0f" % (
                page, ci, g["slope"], g["pitch"], g["edge"], len(g["rows"]), d.min(), np.median(d), d.max()))


if __name__ == "__main__":
    main([int(a) for a in sys.argv[1:]] or sorted(COLUMNS))
