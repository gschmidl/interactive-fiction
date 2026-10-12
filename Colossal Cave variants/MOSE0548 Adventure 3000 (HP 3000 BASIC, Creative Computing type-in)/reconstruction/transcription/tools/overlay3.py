"""Overlay the DP cell boundaries: overlay3.py page col row0 nrows cell0 ncells [scale]"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw
import pagegeom as pg
from geom2 import clean_ink
page, ci, r0, n, c0, nc = [int(v) for v in sys.argv[1:7]]
scale = float(sys.argv[7]) if len(sys.argv) > 7 else 1.0
g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
ink = clean_ink(page)
rows = g["rows"][r0:r0 + n]
S, XR = g["slope"], g["xref"]
xs = [c for r in rows for c in r["cells"][c0:c0 + nc + 1]]
xa, xb = int(min(xs)) - 8, int(max(xs)) + 8
ya = int(min(r["y"] + S * (x - XR) for r in rows for x in (xa, xb))) - 40
yb = int(max(r["y"] + S * (x - XR) for r in rows for x in (xa, xb))) + 40
crop = ink[ya:yb, xa:xb]
im = Image.fromarray((255 - np.clip(crop, 0, 1) * 255).astype(np.uint8)).convert("RGB")
d = ImageDraw.Draw(im)
for r in rows:
    for x in r["cells"][c0:c0 + nc + 1]:
        y = r["y"] + S * (x - XR)
        d.line([(x - xa, y - 28 - ya), (x - xa, y + 28 - ya)], fill=(255, 0, 0))
if scale != 1:
    im = im.resize((int(im.width * scale), int(im.height * scale)), Image.LANCZOS)
out = os.path.join(pg.WORK, "ov3.png"); im.save(out); print(out, im.size)
