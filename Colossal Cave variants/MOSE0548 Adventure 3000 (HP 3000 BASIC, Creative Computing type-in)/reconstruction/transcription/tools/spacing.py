"""Check the spacing the transcription claims against the printed columns.

The aligner only places non-space characters.  Inside string literals and REM
text the number of spaces is part of the program's output, so for those
stretches the gap between consecutive characters in the typed text must equal
the gap between their cells on the page.  Outside strings HP BASIC's LIST
regenerates the spacing (and indents FOR bodies), so it is not checked there.

A row that does not start with a line number continues the previous row of
the same column; an open string carries over.

usage: spacing.py            writes .work/spacing.txt
"""
import os, re
import numpy as np
import pagegeom as pg
from truth import load_cells, parse, align, HERE, TRUTH


def significant(text, in_string):
    """Per character of text: True where spacing matters.  Returns (flags, in_string_at_end).
    On the data-file dump pages every character of a record is data."""
    if pg.DATASET == "data":
        return [True] * len(text), False
    flags = [False] * len(text)
    i = 0
    m = re.match(r"\s*\d+\s+REM", text)
    rem_from = m.end() if m else None
    q = in_string
    while i < len(text):
        ch = text[i]
        if rem_from is not None and i >= rem_from:
            flags[i] = True
        elif ch == '"':
            flags[i] = True
            q = not q
        elif q:
            flags[i] = True
        i += 1
    return flags, q


def main():
    img, meta, core, index = load_cells()
    rows = parse(TRUTH)
    rows.sort(key=lambda t: (t[0], t[1], t[2]))
    out = []
    state = {}
    checked = 0
    import truth as _T
    skipped = []
    for p, c, r, text in rows:
        if _T.nolearn.get((p, c, r)):
            # original scan unreliable for this row (checked on the other scans)
            skipped.append("p%d c%d r%d" % (p, c, r))
            state[(p, c, r)] = False
            continue
        numbered = re.match(r"\s*\d+\s", text) is not None
        in_str = False if numbered else state.get((p, c, r - 1), False)
        flags, end_state = significant(text, in_str)
        state[(p, c, r)] = end_state
        res = align(text, index.get((p, c, r), []), core)
        if res is None:
            out.append("p%d c%d r%d  no alignment | %s" % (p, c, r, text))
            continue
        pos = [j for j, ch in enumerate(text) if ch != " "]
        ks = [int(meta[i][3]) for _, i in res]
        for a in range(1, len(pos)):
            ja, jb = pos[a - 1], pos[a]
            # only gaps wholly inside a significant stretch
            if not all(flags[j] for j in range(ja, jb + 1)):
                continue
            checked += 1
            typed = jb - ja
            measured = ks[a] - ks[a - 1]
            if typed != measured:
                lo, hi = max(0, ja - 12), min(len(text), jb + 12)
                out.append("p%d c%d r%d  %r..%r typed gap %d, printed gap %d  (cells %d-%d)  | ...%s..." % (
                    p, c, r, text[ja], text[jb], typed, measured, ks[a - 1], ks[a], text[lo:hi]))
    path = os.path.join(pg.WORK, "spacing.txt")
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print("gaps checked", checked, "mismatches", len(out), "->", path)
    print("rows skipped (nolearn, verified on the other scans):", " ".join(skipped))


if __name__ == "__main__":
    main()
