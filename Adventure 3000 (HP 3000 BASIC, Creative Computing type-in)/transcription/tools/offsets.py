"""Distribution of glyph-centre offsets from cell centres (single-glyph ink runs only)."""
import json, os, sys
import numpy as np
import pagegeom as pg
from columns import COLUMNS
from geom2 import clean_ink, band_profile

def row_offsets(ink, g, ys, ri):
    r = g["rows"][ri]; yc = ys[ri]
    xs = r["cells"]
    xa, xb = int(xs[0]), int(xs[-1])
    p = band_profile(ink, yc, g["slope"], g["xref"], xa, xb)
    on = p > 2
    out = []
    x = 0
    while x < len(on):
        if on[x]:
            s = x
            while x < len(on) and on[x]:
                x += 1
            w = x - s
            if 18 <= w <= 34:
                c = xa + (s + x - 1) / 2
                k = max(i for i, b in enumerate(xs) if b <= c)
                if k + 1 < len(xs):
                    out.append((k, c - (xs[k] + xs[k + 1]) / 2))
        x += 1
    return out

def main(pages):
    allo = []
    bad = []
    for page in pages:
        ink = clean_ink(page)
        for ci in range(len(COLUMNS[page])):
            g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
            ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
            for ri in range(len(g["rows"])):
                offs = row_offsets(ink, g, ys, ri)
                allo += [o for _, o in offs]
                worst = [(k, round(o, 1)) for k, o in offs if abs(o) >= 8]
                if worst:
                    bad.append((page, ci, ri, worst))
    a = np.abs(np.array(allo))
    print("glyphs", len(a), "median |off| %.1f  p90 %.1f  p99 %.1f  >=8px: %d  >=12px: %d" % (
        np.median(a), np.percentile(a, 90), np.percentile(a, 99), (a >= 8).sum(), (a >= 12).sum()))
    print("rows with a glyph >=8px off:", len(bad))
    for b in bad[:60]:
        print("  p%d c%d r%d %s" % b)

if __name__ == "__main__":
    main([int(v) for v in sys.argv[1:]] or sorted(COLUMNS))
