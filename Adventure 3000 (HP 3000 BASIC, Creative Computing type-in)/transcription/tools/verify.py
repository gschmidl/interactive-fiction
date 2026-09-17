"""Machine-check the transcription against the scan, cell by cell.

For every transcribed row the text is laid onto the row's character cells
(truth.align) and three kinds of disagreement are reported:

  UNUSED   a cell with real ink that no character was placed on
           -> a character missing from the transcription (or art/noise)
  BLANK    a character placed on a cell with (almost) no ink
           -> an extra character, or a printer dot drop-out
  OCR      the glyph classifier confidently reads a different character
           -> a mistyped character in the transcription

Glyph templates are rebuilt without the row being checked (leave-one-page-out),
so a typo cannot vouch for itself.

usage: verify.py [page ...]      writes work/verify.txt
"""
import os, sys, json
import numpy as np
import pagegeom as pg
import truth as T
from truth import load_cells, parse, align, HERE, TRUTH
from classify import prep, build_templates, classify

SMALL = set(",.'`:;-_")
UNUSED_INK = 60.0
BLANK_INK = 12.0


def main(pages):
    img, meta, core, index = load_cells()
    rows = parse(TRUTH)
    X = prep(img)
    # all aligned samples, remembering the page they came from
    samples = []
    aligned = {}
    for p, c, r, text in rows:
        # placement only: which cell each typed character sits on.  The glyph
        # check below uses templates that leave this page out, so a typo
        # still cannot vouch for itself.
        res = align(text, index.get((p, c, r), []), core, use_scores=True)
        aligned[(p, c, r)] = res
        if res is None or T.nolearn.get((p, c, r)):
            continue
        for ch, i in res:
            if ch != "~" and core[i] >= 4.0:
                samples.append((p, ch, i))
    out = []
    for page in pages:
        train = [(ch, i) for p, ch, i in samples if p != page]
        Xs, labels = X[[i for _, i in train]], [ch for ch, _ in train]
        if pg.DATASET == "data":
            from classify import listing_samples
            XL, LL = listing_samples()
            Xs, labels = np.concatenate([Xs, XL]), labels + LL
        classes, Tm, owner = build_templates(Xs, labels)
        cls = np.array(classes)
        prow = [(p, c, r, text) for p, c, r, text in rows if p == page]
        cells_all = sorted({i for p, c, r, _ in prow for i in index.get((p, c, r), []) if core[i] >= 5.0})
        top, score, sec, margin = classify(X[cells_all], Tm, owner, len(classes))
        pred = {i: (cls[t], s, cls[u], m) for i, t, s, u, m in zip(cells_all, top, score, sec, margin)}
        for p, c, r, text in prow:
            res = aligned[(p, c, r)]
            if res is None:
                out.append("p%d c%d r%d ALIGN FAILED: %s" % (p, c, r, text))
                continue
            got = {i: ch for ch, i in res}
            for i in index[(p, c, r)]:
                k = int(meta[i][3])
                if i in got:
                    ch = got[i]
                    if ch == "~":
                        continue
                    if core[i] < BLANK_INK and ch not in SMALL:
                        out.append("p%d c%d r%d k%d BLANK  %r on ink %.0f   | %s" % (p, c, r, k, ch, core[i], text))
                    elif i in pred:
                        o, s, o2, m = pred[i]
                        if o != ch and s >= 0.75 and m >= 0.08 and not (T.ART_FLAGS[i]):
                            out.append("p%d c%d r%d k%d OCR    %r reads %r (%.2f, margin %.2f)   | %s" % (p, c, r, k, ch, o, s, m, text))
                else:
                    if core[i] >= UNUSED_INK and not T.ART_FLAGS[i]:
                        o = pred.get(i, ("?", 0, "?", 0))
                        out.append("p%d c%d r%d k%d UNUSED ink %.0f looks like %r (%.2f)   | %s" % (p, c, r, k, core[i], o[0], o[1], text))
        print("page", page, "done;", len(out), "flags so far")
    path = os.path.join(pg.WORK, "verify.txt")
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(len(out), "flags ->", path)


if __name__ == "__main__":
    from columns import COLUMNS
    main([int(a) for a in sys.argv[1:]] or sorted(COLUMNS))
