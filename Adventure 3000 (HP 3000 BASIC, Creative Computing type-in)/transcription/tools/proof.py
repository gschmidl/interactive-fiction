"""Proof sheet: each row's scan strip with the recognised text drawn under its cells.

usage: proof.py page col row0 nrows [out]
Cells whose best match is weak or close to the runner-up are tinted:
orange = margin < 0.06, red = score < 0.65.  Rows already in truth.txt are
drawn with their transcription in blue instead of the OCR.
"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import pagegeom as pg
from geom2 import clean_ink
from rowimg import row_strip
from truth import load_cells, parse, align, HERE, TRUTH

SCALE = 0.5


def main(page, ci, r0, n, out=None):
    ink = clean_ink(page)
    g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
    ys = json.load(open(os.path.join(pg.WORK, "rowy.json"))).get("%d_%d" % (page, ci))
    ocr = np.load(os.path.join(pg.WORK, "ocr.npz"))
    img, meta, core, index = load_cells()
    truth = {(p, c, r): t for p, c, r, t in parse(TRUTH)}
    try:
        font = ImageFont.truetype("consola.ttf", 30)
    except Exception:
        font = ImageFont.load_default()
    rows = list(range(r0, min(r0 + n, len(g["rows"]))))
    strips = [row_strip(ink, g, ri, ys) for ri in rows]
    W = max(s.shape[1] for s, _ in strips) + 60
    rowh = 64 + 40
    canvas = Image.new("RGB", (W, rowh * len(rows)), (255, 255, 255))
    d = ImageDraw.Draw(canvas)
    for n_, (ri, (s, xs)) in enumerate(zip(rows, strips)):
        y = n_ * rowh
        im = Image.fromarray((255 - np.clip(s, 0, 1) * 255).astype(np.uint8)).convert("RGB")
        canvas.paste(im, (60, y))
        d.text((2, y + 18), str(ri), fill=(0, 0, 255), font=font)
        cells = index.get((page, ci, ri), [])
        key = (page, ci, ri)
        tlabels = {}
        if key in truth:
            res = align(truth[key], cells, core)
            if res:
                tlabels = {i: ch for ch, i in res}
        for i in cells:
            k = int(meta[i][3])
            if k + 1 >= len(xs):
                continue
            xa, xb = 60 + xs[k], 60 + xs[k + 1]
            if key in truth:
                ch = tlabels.get(i, " ")
                color = (0, 0, 200)
            else:
                ch = str(ocr["lab"][i])
                color = (0, 0, 0)
                if ch != " ":
                    if ocr["score"][i] < 0.65:
                        d.rectangle([xa, y + 64, xb - 1, y + rowh - 2], fill=(255, 170, 170))
                    elif ocr["margin"][i] < 0.06:
                        d.rectangle([xa, y + 64, xb - 1, y + rowh - 2], fill=(255, 215, 150))
            if ch != " ":
                d.text((xa + 5, y + 66), ch, fill=color, font=font)
    canvas = canvas.resize((int(canvas.width * SCALE), int(canvas.height * SCALE)), Image.LANCZOS)
    out = out or os.path.join(pg.WORK, "proof_p%d_c%d_r%d.png" % (page, ci, r0))
    canvas.save(out)
    print(out, canvas.size)


if __name__ == "__main__":
    a = sys.argv[1:]
    main(int(a[0]), int(a[1]), int(a[2]), int(a[3]))
