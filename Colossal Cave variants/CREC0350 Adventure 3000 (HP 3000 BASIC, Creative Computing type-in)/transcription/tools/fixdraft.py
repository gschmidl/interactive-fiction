"""Apply proof-reading corrections to the data draft, keeping printed spacing.

stdin lines:   <page> <col> <row>: <old> => <new>
    replaces the first occurrence of <old> in that row's text (exact match
    required, so a wrong guess about the draft fails loudly)
               <page> <col> <row>: = <full text>
    replaces the whole row text (for rows the OCR garbled beyond patching)
Rows are addressed by index in the current segmentation (as on the proof
sheets); the file keys rows by y.
"""
import json, os, sys
import pagegeom as pg

DRAFT = os.environ.get("ADV_TRUTH") or os.path.join(pg.HERE, "draft_data.txt")


def main():
    lines = open(DRAFT, encoding="utf-8").read().splitlines()
    index = {}
    for n, l in enumerate(lines):
        if l.startswith("#") or ": " not in l:
            continue
        head = l.split(": ", 1)[0]
        p, c, y = head.split()
        index[(int(p), int(c), int(y[1:]))] = n
    geo = {}
    bad = 0
    for raw in sys.stdin.read().splitlines():
        if not raw.strip() or raw.startswith("#"):
            continue
        head, edit = raw.split(":", 1)
        p, c, r = [int(v) for v in head.split()]
        if (p, c) not in geo:
            geo[(p, c)] = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (p, c))))["rows"]
        y = round(geo[(p, c)][r]["y"])
        n = index.get((p, c, y))
        if n is None:
            print("NO ROW p%d c%d r%d (y=%d)" % (p, c, r, y)); bad += 1; continue
        h, text = lines[n].split(": ", 1)
        edit = edit[1:] if edit.startswith(" ") else edit
        if edit.startswith("= "):
            text = edit[2:]
        else:
            old, new = edit.split(" => ", 1)
            if old not in text:
                print("NOT FOUND p%d c%d r%d: %r in %r" % (p, c, r, old, text)); bad += 1; continue
            text = text.replace(old, new, 1)
        lines[n] = "%s: %s" % (h, text)
    open(DRAFT, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("done, %d problems" % bad)


if __name__ == "__main__":
    main()
