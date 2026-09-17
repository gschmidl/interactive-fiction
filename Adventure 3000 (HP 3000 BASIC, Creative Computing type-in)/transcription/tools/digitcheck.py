"""Dot test one digit cell of the movement table: digitcheck.py <room> <field> <pos-from-right>"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
from PIL import Image
import pagegeom as pg
from classify import prep, SHIFTS
LIST_WORK = os.path.normpath(os.path.join(pg.HERE, "..", "work"))
FIELD_END = [41 + 4 * j for j in range(10)]


def main(room, field, npos=0):
    L = np.load(os.path.join(LIST_WORK, "cells.npz")); t = np.load(os.path.join(LIST_WORK, "templates.npz"))
    labels = np.array(t["labels"]); sel = np.isin(labels, list("0123456789"))
    XL = prep(L["img"][t["idx"][sel]]); LL = np.array(list(labels[sel]))
    D = np.load(os.path.join(pg.WORK, "cells.npz")); img, meta = D["img"], D["meta"]
    X = prep(img)
    rooms = json.load(open(os.path.join(pg.WORK, "rooms.json")))
    key = [k for k, v in rooms.items() if v == room][0]
    c, r = map(int, key.split("_"))
    idx = {int(m[3]): i for i, m in enumerate(meta) if int(m[1]) == c and int(m[2]) == r}
    cell = idx.get(FIELD_END[field - 1] - npos)
    if cell is None:
        print("no cell"); return
    obs = X[cell]
    out = []
    for ch in "0123456789":
        S = XL[LL == ch]
        m = S[:, 4:60, 3:39].mean(0)
        best, bw = -1e9, None
        for dx, dy in SHIFTS:
            w = obs[4 + dy:60 + dy, 3 + dx:39 + dx]
            v = float((w * m).sum() / (np.linalg.norm(w) * np.linalg.norm(m) + 1e-9))
            if v > best:
                best, bw = v, w
        cover = ndi.maximum_filter(m, size=3) > 0.18 * m.max()
        ink = bw > 0.25
        un = float((bw * ~cover)[ink].sum() / max(1e-9, bw[ink].sum()))
        miss = float(((m > 0.35 * m.max()) & ~(ndi.maximum_filter(bw, size=5) > 0.2)).sum() / max(1, (m > 0.35 * m.max()).sum()))
        out.append((best, ch, un, miss))
    for best, ch, un, miss in sorted(out, reverse=True)[:4]:
        print("  %s corr %.3f  stray ink %5.1f%%  missing %5.1f%%" % (ch, best, 100 * un, 100 * miss))
    Image.fromarray(255 - img[cell]).resize((42 * 8, 64 * 8), Image.LANCZOS).save(os.path.join(pg.WORK, "digit.png"))


if __name__ == "__main__":
    a = sys.argv[1:]
    main(int(a[0]), int(a[1]), int(a[2]) if len(a) > 2 else 0)
