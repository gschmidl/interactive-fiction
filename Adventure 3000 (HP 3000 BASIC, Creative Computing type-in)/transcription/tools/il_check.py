"""Capital I versus lowercase l, decided by the one place the glyphs differ.

In this printer's font both have a stem and a full bottom bar; I has a top
bar on both sides of the stem, l only a flag on the left.  For every cell a
transcription calls I or l, the cell is aligned (small shifts) to the mean I
glyph, and the ink in the I's top-right serif region is measured against the
ink of the same cell's top-left flag.  A clearly printed right serif means I;
none at all, in a cell whose left flag printed, means l.

usage: il_check.py          (uses ADV_SET / ADV_TRUTH like the other tools)
"""
import os
import numpy as np
from scipy import ndimage as ndi
import pagegeom as pg
from truth import load_cells, parse, align, TRUTH
from classify import prep, SHIFTS, listing_samples


def mean_glyph(X, labels, ch):
    S = X[np.array(labels) == ch]
    return S[:, 4:60, 3:39].mean(0), len(S)


def main():
    img, meta, core, index = load_cells()
    X = prep(img)
    XL, LL = listing_samples()
    mI, nI = mean_glyph(XL, LL, "I")
    ml, nl = mean_glyph(XL, LL, "l")
    # regions from the templates: where I has ink and l has none (top-right
    # serif), and the top-left flag both share
    diff = (mI / mI.max()) - (ml / ml.max())
    right = diff > 0.35
    left = ((mI / mI.max()) > 0.5) & ((ml / ml.max()) > 0.5)
    left[right.nonzero()[0].max() + 3:, :] = False       # top part only
    print("templates: I n=%d, l n=%d; right-serif px %d, left-flag px %d" % (nI, nl, right.sum(), left.sum()))
    rows = parse(TRUTH)
    out = []
    for p, c, r, text in rows:
        res = align(text, index.get((p, c, r), []), core)
        if not res:
            continue
        for j, (ch, i) in enumerate(res):
            if ch not in "Il" or core[i] < 12:
                continue
            x = X[i]
            best, bw = -1e9, None
            for dx, dy in SHIFTS:
                w = x[4 + dy:60 + dy, 3 + dx:39 + dx]
                v = float((w * mI).sum())
                if v > best:
                    best, bw = v, w
            rink = float(bw[ndi.binary_dilation(right, iterations=1)].max())
            link = float(bw[left].max()) if left.any() else 1.0
            ratio = rink / max(link, 1e-3)
            verdict = "I" if ratio > 0.45 else ("l" if ratio < 0.15 and link > 0.3 else "?")
            if verdict != ch:
                ctx = "".join(cc for cc, _ in res[max(0, j - 6):j + 7])
                out.append("p%d c%d r%d k%d typed %s -> %s  (right %.2f / left %.2f = %.2f)  ...%s...  | %s" % (
                    p, c, r, int(meta[i][3]), ch, verdict, rink, link, ratio, ctx, text[:70]))
    path = os.path.join(pg.WORK, "il_check.txt")
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(len(out), "disagreements ->", path)


if __name__ == "__main__":
    main()
