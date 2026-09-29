"""Grid overlay for a cell window: overlay2.py page col row0 nrows cell0 ncells [scale]"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw
import pagegeom as pg
from geom import page_ink
page, ci, r0, n, c0, nc = [int(v) for v in sys.argv[1:7]]
scale = float(sys.argv[7]) if len(sys.argv) > 7 else 1.0
g = json.load(open(os.path.join(pg.WORK, "geom_p%d_c%d.json" % (page, ci))))
ink = page_ink(page)
rows = g["rows"][r0:r0 + n]
P, E, S, XR = g["pitch"], g["edge"], g["slope"], g["xref"]
xa, xb = int(E + c0 * P) - 6, int(E + (c0 + nc) * P) + 6
ya = int(min(rows) + S * (xa - XR)) - 40; yb = int(max(rows) + S * (xb - XR)) + 40
ya, yb = min(ya, yb) - 20, max(ya, yb) + 20
crop = ink[ya:yb, xa:xb]
im = Image.fromarray((255 - np.clip(crop, 0, 1) * 255).astype(np.uint8)).convert("RGB")
d = ImageDraw.Draw(im)
for yc in rows:
    for k in range(c0, c0 + nc + 1):
        x = E + k * P; y = yc + S * (x - XR)
        d.line([(x - xa, y - 30 - ya), (x - xa, y + 30 - ya)], fill=(255, 0, 0))
    for off in (-30, 30):
        d.line([(xa - xa, yc + S * (xa - XR) + off - ya), (xb - xa, yc + S * (xb - XR) + off - ya)], fill=(120, 120, 255))
if scale != 1:
    im = im.resize((int(im.width * scale), int(im.height * scale)), Image.LANCZOS)
out = os.path.join(pg.WORK, "ov2.png"); im.save(out); print(out, im.size)
