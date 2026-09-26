"""Read the movement table (p124) with digit-only templates and compare with nd AMOVING.

Row layout (cells from the column's cell 0):  "( nn)" room number in cells 1-3,
description from cell 8, then ten exits right-aligned in 4-cell fields ending
at cells 41, 45, ... 77.  Writes ../data_print/AMOVING.txt and prints every
disagreement with data_nd/AMOVING.
"""
import os, re
import numpy as np
import pagegeom as pg
from classify import prep, build_templates, classify

PROJ = os.path.normpath(os.path.join(pg.HERE, "..", ".."))
LIST_WORK = os.path.normpath(os.path.join(pg.HERE, "..", ".work"))


def main():
    L = np.load(os.path.join(LIST_WORK, "cells.npz"))
    t = np.load(os.path.join(LIST_WORK, "templates.npz"))
    labels = np.array(t["labels"])
    sel = np.isin(labels, list("0123456789"))
    XL = prep(L["img"][t["idx"][sel]])
    classes, T, owner = build_templates(XL, list(labels[sel]))
    cls = np.array(classes)
    D = np.load(os.path.join(pg.WORK, "cells.npz"))
    img, meta = D["img"], D["meta"]
    core = img[:, 10:54, 3:39].reshape(len(img), -1).sum(1) / 255.0
    inked = np.nonzero(core >= 12.0)[0]
    top, score, sec, margin = classify(prep(img[inked]), T, owner, len(classes))
    digit = {int(i): (str(cls[a]), float(s), float(m)) for i, a, s, m in zip(inked, top, score, margin)}
    rows = {}
    for i, (p, c, r, k, _) in enumerate(meta):
        rows.setdefault((int(c), int(r)), {})[int(k)] = int(i)
    table, doubts = {}, []
    for (c, r), cells in sorted(rows.items()):
        def field(k0, k1):
            s, low = "", []
            for k in range(k0, k1 + 1):
                i = cells.get(k)
                if i is not None and i in digit:
                    ch, sc, mg = digit[i]
                    s += ch
                    if sc < 0.8 or mg < 0.08:
                        low.append((k, ch, round(sc, 2), round(mg, 2)))
            return s, low
        room, _ = field(1, 3)
        if not room.isdigit():
            continue
        nums, lows = [], []
        for j in range(10):
            s, low = field(38 + 4 * j, 41 + 4 * j)
            nums.append(int(s) if s else None)
            lows += low
        table[int(room)] = (nums, lows, (c, r))
    nd = [list(map(int, l.split())) for l in open(os.path.join(PROJ, "data_nd", "AMOVING"), encoding="latin-1") if l.strip()]
    out = []
    bad = 0
    for room in range(1, 101):
        if room not in table:
            print("room %d not found on the page" % room); bad += 1; continue
        nums, lows, (c, r) = table[room]
        out.append(" ".join("%3d" % (n if n is not None else -1) for n in nums))
        if nums != nd[room - 1] or lows:
            flag = "DIFF" if nums != nd[room - 1] else "low-confidence"
            print("room %3d (c%d r%d) %s\n   print %s\n   nd    %s\n   doubtful cells %s" % (room, c, r, flag, nums, nd[room - 1], lows))
            bad += nums != nd[room - 1]
    os.makedirs(os.path.join(PROJ, "transcription", "data_print"), exist_ok=True)
    open(os.path.join(PROJ, "transcription", "data_print", "AMOVING.txt"), "w").write("\n".join(out) + "\n")
    print("rooms read %d, rows differing from nd: %d" % (len(table), bad))


if __name__ == "__main__":
    main()
