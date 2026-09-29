"""I vs l by geometry: ink to the right of the stem in the glyph's top dot row.

Stem = the column band with the most vertical ink; top = first row with ink
in the stem band.  In the top ~1.5 dot rows, measure ink left and right of the
stem (excluding the stem band).  I has both, l only the left flag.
Prints the distribution per typed label and writes the cells whose geometry
disagrees with the label to work*/il_geom.txt.
"""
import os, sys
import numpy as np
import pagegeom as pg
from truth import load_cells, parse, align, TRUTH


def measure(cell):
    a = cell.astype(np.float32) / 255.0
    a = a[8:58, 2:40]                         # drop vertical margins (neighbour rows)
    colsum = a.sum(0)
    sm = np.convolve(colsum, np.ones(5), "same")
    stem = int(np.argmax(sm))
    band = slice(max(0, stem - 3), stem + 4)
    rows_with = np.nonzero(a[:, band].max(1) > 0.35)[0]
    if len(rows_with) == 0:
        return None
    top = int(rows_with[0])
    win = slice(max(0, top - 2), top + 9)
    left = float(a[win, max(0, stem - 16):max(0, stem - 4)].sum())
    right = float(a[win, stem + 5:stem + 17].sum())
    return stem, top, left, right


def main():
    img, meta, core, index = load_cells()
    rows = parse(TRUTH)
    data = []
    for p, c, r, text in rows:
        res = align(text, index.get((p, c, r), []), core)
        if not res:
            continue
        for j, (ch, i) in enumerate(res):
            if ch in "Il" and core[i] >= 12:
                m = measure(img[i])
                if m:
                    ctx = "".join(cc for cc, _ in res[max(0, j - 5):j + 6])
                    data.append((ch, m, (p, c, r, int(meta[i][3])), ctx, text))
    for ch in "Il":
        R = np.array([m[3] for c_, m, *_ in data if c_ == ch])
        L = np.array([m[2] for c_, m, *_ in data if c_ == ch])
        if len(R):
            print("typed %s n=%d  right-ink pct: p5 %.2f p25 %.2f p50 %.2f p75 %.2f p95 %.2f | left p50 %.2f" % (
                ch, len(R), *np.percentile(R, [5, 25, 50, 75, 95]), np.median(L)))
    # histogram of right ink for each label
    bins = [0, 0.25, 0.5, 1, 1.5, 2, 3, 4, 6, 9, 20]
    for ch in "Il":
        R = np.array([m[3] for c_, m, *_ in data if c_ == ch])
        h, _ = np.histogram(R, bins=bins)
        print("  %s " % ch + " ".join("%s-%s:%d" % (bins[k], bins[k + 1], h[k]) for k in range(len(h))))
    thr_lo, thr_hi = 0.6, 1.6
    out = []
    for ch, (stem, top, left, right), (p, c, r, k), ctx, text in data:
        verdict = "I" if right >= thr_hi else ("l" if right <= thr_lo else "?")
        if verdict != ch:
            out.append("p%d c%d r%d k%d typed %s geom %s (right %.2f left %.2f)  ...%s...  | %s" % (p, c, r, k, ch, verdict, right, left, ctx, text[:72]))
    path = os.path.join(pg.WORK, "il_geom.txt")
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(len(out), "flags ->", path)


if __name__ == "__main__":
    main()
