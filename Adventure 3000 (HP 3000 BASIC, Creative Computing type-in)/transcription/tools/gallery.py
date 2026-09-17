"""Contact sheet of listing spans for review.

usage: gallery.py spec.txt out.png
spec lines: page col row k0 k1 [label...]
Each entry: the 600-dpi scan (grey, untouched) of cells k0..k1 at 2x, with the
transcription's characters drawn under the cells they were aligned to.
"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import pagegeom as pg
import witness
from truth import load_cells, parse, align, HERE, TRUTH

SCALE = 2


def main(spec, outp):
    img, meta, core, index = load_cells()
    truth = {(p, c, r): t for p, c, r, t in parse(TRUTH)}
    font = ImageFont.truetype("consola.ttf", 26)
    small = ImageFont.truetype("consola.ttf", 18)
    tiles = []
    pages = {}
    for line in open(spec, encoding="utf-8"):
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split()
        p, c, r, k0, k1 = [int(v) for v in parts[:5]]
        label = " ".join(parts[5:])
        if p not in pages:
            pages[p] = witness.reading(p, "tif")
        A = pages[p]
        g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (p, c))))
        ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))["%d_%d" % (p, c)]
        row = g["rows"][r]
        cells = row["cells"]
        k1 = min(k1, len(cells) - 2)
        xa, xb = cells[k0] - 4, cells[k1 + 1] + 4
        yc = ys[r] + g["slope"] * ((xa + xb) / 2 - g["xref"])
        crop = A[int(yc - 40):int(yc + 40), int(xa):int(xb)]
        im = Image.fromarray(crop.astype(np.uint8)).resize((crop.shape[1] * SCALE, crop.shape[0] * SCALE), Image.LANCZOS).convert("RGB")
        text = truth.get((p, c, r), "")
        res = align(text, index[(p, c, r)], core) if text else None
        band = Image.new("RGB", (im.width, 40), (255, 255, 255))
        d = ImageDraw.Draw(band)
        if res:
            for ch, i in res:
                k = int(meta[i][3])
                if k0 <= k <= k1:
                    x = (cells[k] - xa) * SCALE
                    d.text((x + 16, 4), ch, fill=(0, 0, 200), font=font)
        dd = ImageDraw.Draw(im)
        for k in range(k0, k1 + 2):
            x = (cells[k] - xa) * SCALE
            dd.line([(x, 0), (x, 10)], fill=(255, 0, 0))
        head = Image.new("RGB", (im.width, 24), (255, 255, 255))
        ImageDraw.Draw(head).text((2, 2), "p%d c%d r%d k%d-%d  %s" % (p, c, r, k0, k1, label), fill=(0, 0, 0), font=small)
        tile = Image.new("RGB", (im.width, head.height + im.height + band.height + 8), (255, 255, 255))
        tile.paste(head, (0, 0)); tile.paste(im, (0, head.height)); tile.paste(band, (0, head.height + im.height))
        tiles.append(tile)
    per = 8
    base, ext = os.path.splitext(outp)
    for s in range(0, len(tiles), per):
        chunk = tiles[s:s + per]
        W = max(t.width for t in chunk)
        H = sum(t.height for t in chunk)
        sheet = Image.new("RGB", (W, H), (255, 255, 255))
        y = 0
        for t in chunk:
            sheet.paste(t, (0, y)); y += t.height
        name = "%s_%d%s" % (base, s // per, ext)
        sheet.save(name)
        print(name, sheet.size, len(chunk))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
