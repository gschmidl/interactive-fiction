"""Three-scan stack around a substring of a transcribed row.
usage: witsub.py page col row "substring" [occurrence] [scale]  -> work/wit.png
The row's transcription (truth.txt) is aligned to its cells to find the substring's cells."""
import json, os, sys, subprocess
import numpy as np
from PIL import Image
import pagegeom as pg
import witness
from truth import load_cells, parse, align, HERE, TRUTH
page, ci, ri = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
sub = sys.argv[4]
occ = int(sys.argv[5]) if len(sys.argv) > 5 else 0
scale = int(sys.argv[6]) if len(sys.argv) > 6 else 3
img, meta, core, index = load_cells()
text = {(p, c, r): t for p, c, r, t in parse(TRUTH)}[(page, ci, ri)]
res = align(text, index[(page, ci, ri)], core)
compact = "".join(ch for ch in text if ch != " ")
s = "".join(ch for ch in sub if ch != " ")
pos = -1
for _ in range(occ + 1):
    pos = compact.find(s, pos + 1)
ks = [int(meta[res[j][1]][3]) for j in range(pos, pos + len(s))]
g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
r = g["rows"][ri]
c0, c1 = max(0, min(ks) - 1), min(len(r["cells"]) - 1, max(ks) + 2)
xa = r["cells"][c0] - 4; xb = r["cells"][c1] + 4
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
out = os.path.join(pg.WORK, "wit.png"); canvas.save(out); print(out, canvas.size, "cells", ks)
