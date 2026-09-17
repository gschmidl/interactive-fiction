"""One image per page: cleaned ink at 1/4 scale, each detected row marked at its start (index) and end (bar)."""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw
import pagegeom as pg
from columns import COLUMNS
from geom2 import clean_ink
S = 4
for page in [int(a) for a in sys.argv[1:]]:
    ink = clean_ink(page)
    im = Image.fromarray((255 - np.clip(ink[::S, ::S], 0, 1) * 200).astype(np.uint8)).convert("RGB")
    d = ImageDraw.Draw(im)
    colors = [(230, 0, 0), (0, 140, 0), (0, 0, 255)]
    for ci in range(len(COLUMNS[page])):
        g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
        for k, r in enumerate(g["rows"]):
            xa, xb = r["cells"][0], r["cells"][-1]
            ya = r["y"] + g["slope"] * (xa - g["xref"]); yb = r["y"] + g["slope"] * (xb - g["xref"])
            d.text((xa / S - 20, ya / S - 6), str(k), fill=colors[ci])
            d.line([(xb / S, yb / S - 6), (xb / S, yb / S + 6)], fill=colors[ci], width=2)
    im = im.crop((110, 140, 1640, 1150))
    im.save(os.path.join(pg.WORK, "rows_p%d.png" % page)); print(page, im.size)
