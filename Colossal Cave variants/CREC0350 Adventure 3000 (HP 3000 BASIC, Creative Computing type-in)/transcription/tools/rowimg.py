"""Render listing rows for reading, with a tick above every cell boundary (tall every 5).

usage: rowimg.py page col row0 nrows [scale] [out]
Each row is straightened (sheared back) and cut from its own cell boundaries,
so the ticks line up with the cells the recogniser sees.
"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
from PIL import Image, ImageDraw
import pagegeom as pg
from geom2 import clean_ink


def row_strip(ink, g, ri, ys, ncells=73):
    row = g["rows"][ri]
    xs = row["cells"]
    yc = ys[ri]
    x0 = xs[0]
    pitch = g["pitch"]
    width = int(ncells * pitch) + 8
    gx = x0 - 4 + np.arange(width)
    gy = np.arange(-32, 32)
    yy = yc + gy[:, None] + g["slope"] * (gx[None, :] - g["xref"])
    xx = np.broadcast_to(gx[None, :], yy.shape)
    strip = ndi.map_coordinates(ink, [yy, xx], order=1, mode="constant")
    return strip, [x - x0 + 4 for x in xs]


def main(page, ci, r0, n, scale=0.55, out=None):
    ink = clean_ink(page)
    g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
    ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (page, ci)]
    rows = range(r0, min(r0 + n, len(g["rows"])))
    strips = [row_strip(ink, g, ri, ys) for ri in rows]
    W = max(s.shape[1] for s, _ in strips)
    tick = 12
    H = sum(s.shape[0] + tick for s, _ in strips)
    canvas = Image.new("RGB", (W + 40, H), (255, 255, 255))
    d = ImageDraw.Draw(canvas)
    y = 0
    for ri, (s, xs) in zip(rows, strips):
        for j, x in enumerate(xs):
            d.line([(40 + x, y + (0 if j % 5 == 0 else 6)), (40 + x, y + tick - 1)],
                   fill=(255, 0, 0) if j % 5 == 0 else (255, 150, 150))
        d.text((2, y + tick + 20), str(ri), fill=(0, 0, 255))
        im = Image.fromarray((255 - np.clip(s, 0, 1) * 255).astype(np.uint8))
        canvas.paste(im.convert("RGB"), (40, y + tick))
        y += s.shape[0] + tick
    if scale != 1:
        canvas = canvas.resize((int(canvas.width * scale), int(canvas.height * scale)), Image.LANCZOS)
    out = out or os.path.join(pg.WORK, "row_p%d_c%d_r%d.png" % (page, ci, r0))
    canvas.save(out)
    print(out, canvas.size)


if __name__ == "__main__":
    a = sys.argv[1:]
    main(int(a[0]), int(a[1]), int(a[2]), int(a[3]), float(a[4]) if len(a) > 4 else 0.55)
