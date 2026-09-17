"""Three-scan stack of cells c0..c1 of one row: wit.py page col row c0 c1 [scale] -> work/wit.png"""
import json, os, sys
import numpy as np
from PIL import Image
import pagegeom as pg
import witness
page, ci, ri, c0, c1 = [int(v) for v in sys.argv[1:6]]
scale = int(sys.argv[6]) if len(sys.argv) > 6 else 4
g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
r = g["rows"][ri]
xa = r["cells"][c0] - 6; xb = r["cells"][min(c1, len(r["cells"]) - 1)] + 6
yc = ys[ri] + g["slope"] * ((xa + xb) / 2 - g["xref"])
ya, yb = yc - 36, yc + 36
A = witness.reading(page, "tif")
ca = A[int(ya):int(yb), int(xa):int(xb)]
H = ca.shape[0] * scale
ims = [Image.fromarray(ca.astype(np.uint8)).resize((ca.shape[1] * scale, H), Image.LANCZOS)]
for src in ("better", "mf"):
    ims.append(witness.crop_other(page, src, xa, ya, xb, yb, H))
W = max(i.width for i in ims)
canvas = Image.new("L", (W, sum(i.height + 6 for i in ims)), 255)
y = 0
for i in ims:
    canvas.paste(i, (0, y)); y += i.height + 6
out = os.path.join(pg.WORK, "wit.png"); canvas.save(out); print(out, canvas.size)
