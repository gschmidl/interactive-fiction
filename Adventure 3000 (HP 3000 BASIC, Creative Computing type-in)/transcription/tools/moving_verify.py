"""Check every digit of the p124 movement table against data_nd/AMOVING.

For each expected digit cell (nd's number right-aligned in its 4-cell field)
score all ten digit templates; flag a cell only when another digit beats the
expected one by a clear margin, or when ink and expectation disagree about a
cell being blank.
"""
import os
import numpy as np
import pagegeom as pg
from classify import prep, build_templates, score_matrix

PROJ = os.path.normpath(os.path.join(pg.HERE, "..", ".."))
LIST_WORK = os.path.normpath(os.path.join(pg.HERE, "..", "work"))
FIELD_END = [41 + 4 * j for j in range(10)]


def main():
    L = np.load(os.path.join(LIST_WORK, "cells.npz"))
    t = np.load(os.path.join(LIST_WORK, "templates.npz"))
    labels = np.array(t["labels"])
    sel = np.isin(labels, list("0123456789"))
    classes, T, owner = build_templates(prep(L["img"][t["idx"][sel]]), list(labels[sel]))
    cls = np.array(classes)
    D = np.load(os.path.join(pg.WORK, "cells.npz"))
    img, meta = D["img"], D["meta"]
    core = img[:, 10:54, 3:39].reshape(len(img), -1).sum(1) / 255.0
    X = prep(img)
    S = score_matrix(X, T, owner, len(classes))
    rows = {}
    for i, (p, c, r, k, _) in enumerate(meta):
        rows.setdefault((int(c), int(r)), {})[int(k)] = int(i)
    # room number -> its row
    where = {}
    for key, cells in rows.items():
        s = ""
        for k in (1, 2, 3):
            i = cells.get(k)
            if i is not None and core[i] >= 10:
                s += str(cls[int(np.argmax(S[i]))])
        if s.isdigit() and 1 <= int(s) <= 100:
            where[int(s)] = key
    nd = [l.split() for l in open(os.path.join(PROJ, "data_nd", "AMOVING"), encoding="latin-1") if l.strip()]
    flags = 0
    for room in range(1, 101):
        key = where.get(room)
        if key is None:
            print("room %d: row not found" % room); flags += 1; continue
        cells = rows[key]
        for j, num in enumerate(nd[room - 1]):
            for n, ch in enumerate(num[::-1]):          # right-aligned
                k = FIELD_END[j] - n
                i = cells.get(k)
                ink = core[i] if i is not None else 0
                if ink < 8:
                    print("room %3d field %2d cell %2d: nd says %s, cell is blank (ink %.0f)" % (room, j + 1, k, ch, ink)); flags += 1
                    continue
                sc = S[i]
                exp = float(sc[list(cls).index(ch)])
                best = int(np.argmax(sc))
                if cls[best] != ch and float(sc[best]) - exp > 0.05:
                    print("room %3d field %2d cell %2d: nd %s (%.2f) vs scan %s (%.2f)  ink %.0f  [c%d r%d]" % (
                        room, j + 1, k, ch, exp, cls[best], float(sc[best]), ink, key[0], key[1])); flags += 1
            # a digit to the left of the number must not be inked
            k = FIELD_END[j] - len(num)
            i = cells.get(k)
            if i is not None and core[i] >= 14:
                print("room %3d field %2d: ink in cell %2d left of nd's %-3s (ink %.0f) [c%d r%d]" % (room, j + 1, k, num, core[i], key[0], key[1])); flags += 1
    print("cells flagged:", flags)


if __name__ == "__main__":
    main()
