"""Deskewed ink thumbnail (1/6 scale) with a labelled 250-px coordinate grid, for picking column boxes."""
import sys, json, os
import numpy as np
from scipy import ndimage as ndi
from PIL import Image, ImageDraw
import pagegeom as pg

def main(page):
    a = pg.load_gray(page)
    ink = pg.ink_map(a)
    ang = pg.deskew_angle(ink, box=(500, 600, 3200, 4600))
    r = ndi.rotate(ink, ang, reshape=False, order=1)
    np.save(os.path.join(pg.WORK, "p%d_ink.npy" % page), r.astype(np.float16))
    json.dump({"angle": ang}, open(os.path.join(pg.WORK, "p%d_geom.json" % page), "w"))
    S = 6
    th = Image.fromarray((255 - np.clip(r[::S, ::S], 0, 1) * 255).astype(np.uint8)).convert("RGB")
    d = ImageDraw.Draw(th)
    for x in range(0, r.shape[1], 250):
        d.line([(x // S, 0), (x // S, th.height)], fill=(255, 0, 0) if x % 1000 == 0 else (255, 180, 180))
        if x % 500 == 0:
            d.text((x // S + 2, 2), str(x), fill=(200, 0, 0))
    for y in range(0, r.shape[0], 250):
        d.line([(0, y // S), (th.width, y // S)], fill=(0, 0, 255) if y % 1000 == 0 else (180, 180, 255))
        if y % 500 == 0:
            d.text((2, y // S + 2), str(y), fill=(0, 0, 200))
    th.save(os.path.join(pg.WORK, "p%d_grid.png" % page))
    print(page, "angle", ang, "size", r.shape)

if __name__ == "__main__":
    for p in sys.argv[1:]:
        main(int(p))
