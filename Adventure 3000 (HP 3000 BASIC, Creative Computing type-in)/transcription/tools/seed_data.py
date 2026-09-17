"""Seed truth_data.txt from work_data/proposed.txt (nd-matched rows only)."""
import os
import pagegeom as pg
from truth import TRUTH
lines = []
for l in open(os.path.join(pg.WORK, "proposed.txt"), encoding="utf-8"):
    head, text = l.rstrip("\n").split(": ", 1) if ": " in l else (l.rstrip("\n").rstrip(":"), "")
    if "@-" in head or not text.strip() or text.startswith("?"):
        continue
    lines.append("%s: %s" % (head, text))
open(TRUTH, "w", encoding="utf-8").write("# data-file dump pages 112-122: <page> <col> @<y>: <text>\n" + "\n".join(lines) + "\n")
print("seeded", len(lines))
