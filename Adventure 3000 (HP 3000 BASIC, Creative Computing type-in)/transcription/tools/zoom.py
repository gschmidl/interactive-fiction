"""Full-resolution crop of rows: zoom.py page col cell0 ncells row [row ...] (rows by index)."""
import json, os, sys
import numpy as np
from PIL import Image
import pagegeom as pg
from geom2 import clean_ink
from rowimg import row_strip
page, ci, c0, nc = [int(v) for v in sys.argv[1:5]]
rows = [int(v) for v in sys.argv[5:]]
ink = clean_ink(page)
g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
strips = []
for ri in rows:
    s, xs = row_strip(ink, g, ri, ys, ncells=73)
    a = int(xs[min(c0, len(xs) - 1)]) - 4
    b = int(xs[min(c0 + nc, len(xs) - 1)]) + 4
    strips.append(s[:, max(0, a):b])
W = max(s.shape[1] for s in strips)
canvas = np.zeros((sum(s.shape[0] for s in strips), W))
y = 0
for s in strips:
    canvas[y:y + s.shape[0], :s.shape[1]] = s; y += s.shape[0]
out = os.path.join(pg.WORK, "zoom.png")
Image.fromarray((255 - np.clip(canvas, 0, 1) * 255).astype(np.uint8)).save(out)
print(out, canvas.shape)
