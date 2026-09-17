"""Align hand transcriptions to cells and build glyph templates.

truth.txt lines:   <page> <col> <row>: <text>
Spaces in <text> do not matter: the non-space characters are laid, in order,
onto the row's inked cells.  A row whose count of non-space characters differs
from its count of inked cells is reported and skipped.  "~" in the text stands
for a cell to ignore (typeset overlay, noise).

Writes work/templates.npz  (labels, images of every aligned sample)
"""
import os, sys, json
import numpy as np
import pagegeom as pg

EMPTY = 12.0
HERE = os.path.dirname(os.path.abspath(__file__))
TRUTH = os.environ.get("ADV_TRUTH") or os.path.join(HERE, "truth.txt" if pg.DATASET == "listing" else "truth_data.txt")


def art_flags(meta, pos):
    from columns import ART
    flags = np.zeros(len(meta), bool)
    for i, ((p, c, r, k, _), (x, y)) in enumerate(zip(meta, pos)):
        for (ax0, ay0, ax1, ay1) in ART.get(int(p), []):
            if ax0 <= x <= ax1 and ay0 <= y <= ay1:
                flags[i] = True
    return flags


def load_cells():
    d = np.load(os.path.join(pg.WORK, "cells.npz"))
    img, meta = d["img"], d["meta"]
    core = img[:, 10:54, 3:39].reshape(len(img), -1).sum(1) / 255.0
    global ART_FLAGS
    ART_FLAGS = art_flags(meta, d["pos"])
    index = {}
    for i, (p, c, r, k, _) in enumerate(meta):
        index.setdefault((int(p), int(c), int(r)), []).append(i)
    return img, meta, core, index


_ROWS = {}


def row_index(p, c, y):
    """Index of the current row of column (p, c) nearest to y (within 25 px)."""
    if (p, c) not in _ROWS:
        g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (p, c))))
        _ROWS[(p, c)] = [r["y"] for r in g["rows"]]
    ys = _ROWS[(p, c)]
    j = min(range(len(ys)), key=lambda k: abs(ys[k] - y))
    return j if abs(ys[j] - y) <= 25 else None


def parse(path):
    """(page, col, row index, text) for every transcribed row; rows are keyed
    in the file by their y position (@y) so re-segmenting does not shift them."""
    out = []
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if not line.strip() or line.startswith("#"):
            continue
        head, text = line.split(":", 1)
        p, c, r = head.split()
        p, c = int(p), int(c)
        if r.startswith("@"):
            ri = row_index(p, c, float(r[1:]))
            if ri is None:
                print("NO ROW for %s" % head)
                continue
        else:
            ri = int(r)
        text = text[1:] if text.startswith(" ") else text
        if text.startswith("{nolearn}"):
            text = text[len("{nolearn}"):].lstrip()
            nolearn[(p, c, ri)] = True
        out.append((p, c, ri, text))
    return out


nolearn = {}


ART_FLAGS = None


def skipcost(i, core):
    if core[i] < 2.0:
        return 0.0
    if ART_FLAGS is not None and ART_FLAGS[i]:
        return 0.2
    if core[i] > 650:          # far more ink than any printer glyph: typeset or art
        return 0.5
    return max(0.3, core[i] / 10.0)


SMALL = set(",.'`:;")


def takecost(ch, i, core, strict=False):
    """Placing a character on a cell with little or no ink.  Only small
    punctuation is ever printed that faintly; a letter or digit on a speck
    usually means the row is being slid over (strict, used when glyph shape
    scores are available to confirm the real placement)."""
    if core[i] >= EMPTY:
        return 0.0
    if core[i] >= 2.0:
        return 0.3 if (ch in SMALL or not strict) else 3.0
    return 1.5 if ch in SMALL else 6.0


_SCORES = None


def scores():
    """Cached glyph scores (classify.py writes work/scores.npz): cell -> row of
    class correlations.  None before the first classify run."""
    global _SCORES
    if _SCORES is None:
        path = os.path.join(pg.WORK, "scores.npz")
        cells = os.path.join(pg.WORK, "cells.npz")
        if not os.path.exists(path) or os.path.getmtime(path) < os.path.getmtime(cells):
            _SCORES = False           # missing, or stale: cells were re-cut since
        else:
            d = np.load(path)
            pos = {int(i): k for k, i in enumerate(d["idx"])}
            cidx = {str(ch): j for j, ch in enumerate(d["classes"])}
            _SCORES = (pos, cidx, d["S"])
    return _SCORES or None


ART_TAKE = 2.5     # a character sitting inside a drawing's box is unlikely


def takecost_scored(ch, i, core, sc):
    """Ink-based cost, plus how unlike the glyph the cell looks.  The shape
    term is weighted by how much ink there is to judge: a normalised
    correlation on a near-empty cell is noise."""
    base = takecost(ch, i, core, strict=sc is not None)
    if ART_FLAGS is not None and ART_FLAGS[i]:
        base += ART_TAKE
    if sc is None or core[i] < 5.0:
        return base
    pos, cidx, S = sc
    if i not in pos or ch not in cidx:
        return base
    w = min(1.0, (core[i] - 5.0) / 40.0)
    if ch in SMALL:
        w *= 0.5
    return base + w * 4.0 * (1.0 - float(S[pos[i], cidx[ch]]))


def align(text, cells, core, use_scores=True):
    """Lay the non-space characters onto the row's cells in order.  Empty cells
    are free to pass over (spaces); inked cells may be passed over as noise at
    a cost that grows with their ink; a character may sit on a faint or even
    blank cell (printer dots can drop out entirely) at a cost.  When glyph
    scores are available, placing a character on a cell costs by how unlike
    that glyph the cell looks, which pins every row to its true cells.
    Returns list of (char, cell) or None."""
    sc = scores() if use_scores else None
    chars = [ch for ch in text if ch != " "]
    cand = list(cells)
    m, n = len(chars), len(cand)
    if m > n:
        return None
    INF = 1e18
    D = np.full((m + 1, n + 1), INF)
    B = np.zeros((m + 1, n + 1), np.int8)
    D[0, 0] = 0
    for j in range(1, n + 1):
        D[0, j] = D[0, j - 1] + skipcost(cand[j - 1], core)
    for i in range(1, m + 1):
        for j in range(i, n + 1):
            skip = D[i, j - 1] + skipcost(cand[j - 1], core)
            take = D[i - 1, j - 1] + takecost_scored(chars[i - 1], cand[j - 1], core, sc)
            if take <= skip:
                D[i, j], B[i, j] = take, 1
            else:
                D[i, j], B[i, j] = skip, 0
    # anything after the last character (neighbouring columns, art) is cheap
    tail = np.zeros(n + 1)
    for j in range(n - 1, -1, -1):
        tail[j] = tail[j + 1] + 0.05 * skipcost(cand[j], core)
    jbest = int(np.argmin(D[m, :] + tail))
    limit = 25 + (1.2 * m if sc is not None else 0)
    if D[m, jbest] + tail[jbest] > limit:
        return None
    out, i, j = [], m, jbest
    while i > 0:
        if B[i, j] == 1:
            out.append((chars[i - 1], cand[j - 1])); i -= 1; j -= 1
        else:
            j -= 1
    return out[::-1]


def main():
    img, meta, core, index = load_cells()
    truth = parse(TRUTH)
    labels, samples, bad = [], [], 0
    for p, c, r, text in truth:
        cells = index.get((p, c, r), [])
        res = align(text, cells, core)
        if res is None:
            bad += 1
            inked = [(int(meta[i][3]), int(core[i])) for i in cells if core[i] >= 5.0]
            print("MISMATCH p%d c%d r%d: %d chars; inked cells %s" % (p, c, r, len([ch for ch in text if ch != " "]), inked))
            print("    text: %r" % text)
            continue
        if nolearn.get((p, c, r)):
            continue
        for ch, i in res:
            if ch == "~" or core[i] < 4.0:
                continue
            labels.append(ch)
            samples.append(i)
    np.savez_compressed(os.path.join(pg.WORK, "templates.npz"), labels=np.array(labels), idx=np.array(samples, np.int64))
    from collections import Counter
    cnt = Counter(labels)
    print("rows %d (bad %d), samples %d, classes %d" % (len(truth), bad, len(samples), len(cnt)))
    print(" ".join("%s:%d" % (k, v) for k, v in sorted(cnt.items())))


if __name__ == "__main__":
    main()
