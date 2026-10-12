"""Append transcribed rows to truth.txt keyed by row y.

usage: addtruth.py page col < rows.txt     (rows.txt lines: <row index>: <text>)
Existing entries for the same rows are replaced.
"""
import json, os, sys
import pagegeom as pg
from truth import HERE, TRUTH


def main(page, ci, src):
    g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, ci))))
    new = {}
    for line in src.read().splitlines():
        if not line.strip():
            continue
        ri, text = line.split(":", 1)
        ri = int(ri)
        y = round(g["rows"][ri]["y"])
        new[y] = text[1:] if text.startswith(" ") else text
    path = TRUTH
    lines = open(path, encoding="utf-8").read().splitlines()
    keep = []
    for l in lines:
        if l.startswith("%d %d @" % (page, ci)):
            y = int(l.split("@", 1)[1].split(":", 1)[0])
            if any(abs(y - ny) <= 20 for ny in new):
                continue
        keep.append(l)
    for y, text in sorted(new.items()):
        keep.append("%d %d @%d: %s" % (page, ci, y, text))
    open(path, "w", encoding="utf-8").write("\n".join(keep) + "\n")
    print("added", len(new))


if __name__ == "__main__":
    main(int(sys.argv[1]), int(sys.argv[2]), sys.stdin)
