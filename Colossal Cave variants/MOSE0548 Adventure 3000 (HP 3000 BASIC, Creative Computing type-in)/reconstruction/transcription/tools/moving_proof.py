"""Proof sheets for the movement table: printed digits with nd's numbers under them.

usage: moving_proof.py <col> <row0> <nrows>
The strip is taken from the untouched scan and contrast-stretched locally, so
faint rows stay readable; nd's ten numbers are drawn in blue right-aligned
under the same fields.
"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import pagegeom as pg
import witness

PROJ = os.path.normpath(os.path.join(pg.HERE, "..", ".."))
FIELD_END = [41 + 4 * j for j in range(10)]


def main(ci, r0, n):
    A = witness.reading(124, "tif")
    g = json.load(open(os.path.join(pg.WORK, "cells_p124_c%d.json" % ci)))
    ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["124_%d" % ci]
    nd = [l.split() for l in open(os.path.join(PROJ, "data_nd", "AMOVING"), encoding="latin-1") if l.strip()]
    font = ImageFont.truetype("consola.ttf", 26)
    rows = list(range(r0, min(r0 + n, len(g["rows"]))))
    tiles = []
    for r in rows:
        cells = g["rows"][r]["cells"]
        if len(cells) <= FIELD_END[-1] + 1:
            continue
        # room number, from the "( nn)" field, straight off the OCR pass
        xa, xb = cells[1] - 8, cells[FIELD_END[-1] + 1] + 8
        yc = ys[r]
        crop = A[int(yc - 30):int(yc + 32), int(xa):int(xb)].astype(np.float32)
        lo, hi = np.percentile(crop, 2), np.percentile(crop, 65)
        im = Image.fromarray(np.clip((crop - lo) / max(hi - lo, 1) * 255, 0, 255).astype(np.uint8)).convert("RGB")
        t = Image.new("RGB", (im.width, im.height + 32), (255, 255, 255))
        t.paste(im, (0, 0))
        tiles.append((r, t, cells, xa))
    W = max(t.width for _, t, _, _ in tiles)
    sheet = Image.new("RGB", (W, sum(t.height for _, t, _, _ in tiles)), (255, 255, 255))
    d = ImageDraw.Draw(sheet)
    y = 0
    for r, t, cells, xa in tiles:
        sheet.paste(t, (0, y))
        # which room is this row? read the digits under "( nn)" from nd by matching position
        y += t.height
    # second pass: draw nd numbers using the room number decoded from the OCR table
    tab = json.load(open(os.path.join(pg.WORK, "rooms.json"))) if os.path.exists(os.path.join(pg.WORK, "rooms.json")) else {}
    y = 0
    for r, t, cells, xa in tiles:
        room = tab.get("%d_%d" % (ci, r))
        if room:
            for j, num in enumerate(nd[int(room) - 1]):
                xr = cells[FIELD_END[j] + 1] - xa
                w = d.textlength(num, font=font)
                d.text((xr - w - 4, y + t.height - 30), num, fill=(0, 0, 220), font=font)
            d.text((2, y + t.height - 30), "#%s" % room, fill=(0, 150, 0), font=font)
        y += t.height
    out = os.path.join(pg.WORK, "moving_c%d_r%d.png" % (ci, r0))
    sheet.save(out)
    print(out, sheet.size)


if __name__ == "__main__":
    main(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]))
