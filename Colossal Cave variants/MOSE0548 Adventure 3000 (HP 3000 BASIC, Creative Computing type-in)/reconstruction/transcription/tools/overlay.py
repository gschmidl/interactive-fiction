"""Draw the fitted grid over part of a column, for checking by eye.
usage: overlay.py page col first_row nrows [ncells] [scale]"""
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw
import pagegeom as pg
from geom import page_ink


def main(page, ci, r0, n, ncells=40, scale=0.5):
    g = json.load(open(os.path.join(pg.WORK, "geom_p%d_c%d.json" % (page, ci))))
    ink = page_ink(page)
    rows = g["rows"][r0:r0 + n]
    P, E, S, XR = g["pitch"], g["edge"], g["slope"], g["xref"]
    xa, xb = int(E) - 10, int(E + ncells * P) + 10
    ya, yb = int(rows[0]) - 45, int(rows[-1]) + 45
    crop = ink[ya:yb, xa:xb]
    im = Image.fromarray((255 - np.clip(crop, 0, 1) * 255).astype(np.uint8)).convert("RGB")
    d = ImageDraw.Draw(im)
    for yc in rows:
        for k in range(ncells + 1):
            x = E + k * P
            y = yc + S * (x - XR)
            d.line([(x - xa, y - 31 - ya), (x - xa, y + 31 - ya)], fill=(255, 0, 0))
        y_l = yc + S * (E - XR)
        y_r = yc + S * (E + ncells * P - XR)
        d.line([(E - xa, y_l - ya), (E + ncells * P - xa, y_r - ya)], fill=(80, 80, 255))
    if scale != 1:
        im = im.resize((int(im.width * scale), int(im.height * scale)), Image.LANCZOS)
    out = os.path.join(pg.WORK, "ov_p%d_c%d_r%d.png" % (page, ci, r0))
    im.save(out)
    print(out, im.size)


if __name__ == "__main__":
    a = sys.argv[1:]
    main(int(a[0]), int(a[1]), int(a[2]), int(a[3]), int(a[4]) if len(a) > 4 else 40,
         float(a[5]) if len(a) > 5 else 0.5)
