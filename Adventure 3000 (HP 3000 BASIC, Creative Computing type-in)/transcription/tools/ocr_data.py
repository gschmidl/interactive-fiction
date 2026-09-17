"""First-pass OCR of the data-file dump pages with the listing's glyph templates.

Same printer, same font: the templates learned from the verified listing
(work/templates.npz over work/cells.npz) classify the data cells
(work_data/cells.npz).  Empty cells become spaces.  A vertical gap of about
two row pitches between rows is a blank line in the dump.

Writes work_data/ocr_rows.txt:   <page> <col> @<y>: <text>   (blank lines as "<page> <col> @-: ")
"""
import json, os
import numpy as np
import pagegeom as pg
from classify import prep, build_templates, classify

PROJ_T = os.path.normpath(os.path.join(pg.HERE, ".."))
LIST_WORK = os.path.join(PROJ_T, "work")
DATA_WORK = pg.WORK


def core_of(img):
    return img[:, 10:54, 3:39].reshape(len(img), -1).sum(1) / 255.0


def main():
    L = np.load(os.path.join(LIST_WORK, "cells.npz"))
    t = np.load(os.path.join(LIST_WORK, "templates.npz"))
    XL = prep(L["img"])
    classes, T, owner = build_templates(XL[t["idx"]], list(t["labels"]))
    cls = np.array(classes)

    D = np.load(os.path.join(DATA_WORK, "cells.npz"))
    img, meta = D["img"], D["meta"]
    core = core_of(img)
    inked = np.nonzero(core >= 12.0)[0]
    top, score, sec, margin = classify(prep(img[inked]), T, owner, len(classes))
    lab = np.full(len(img), " ", dtype="<U1")
    lab[inked] = cls[top]
    lab[inked[score < 0.45]] = "?"
    rowy = json.load(open(os.path.join(DATA_WORK, "rowy.json")))
    rows = {}
    for i, (p, c, r, k, _) in enumerate(meta):
        rows.setdefault((int(p), int(c), int(r)), []).append((int(k), lab[i]))
    out = []
    for (p, c) in sorted({(p, c) for p, c, _ in rows}):
        g = json.load(open(os.path.join(DATA_WORK, "cells_p%d_c%d.json" % (p, c))))
        ys = [row["y"] for row in g["rows"]]
        pitch = np.median(np.diff(ys)) if len(ys) > 2 else 62.0
        for r in range(len(ys)):
            if r > 0:
                gap = ys[r] - ys[r - 1]
                for _ in range(int(round(gap / pitch)) - 1):
                    out.append("%d %d @-: " % (p, c))
            cells = sorted(rows.get((p, c, r), []))
            text = "".join(ch for _, ch in cells).rstrip()
            out.append("%d %d @%d: %s" % (p, c, round(ys[r]), text))
    open(os.path.join(DATA_WORK, "ocr_rows.txt"), "w", encoding="utf-8").write("\n".join(out) + "\n")
    print("rows", sum(1 for l in out if "@-" not in l), "blank lines", sum(1 for l in out if "@-" in l))


if __name__ == "__main__":
    main()
