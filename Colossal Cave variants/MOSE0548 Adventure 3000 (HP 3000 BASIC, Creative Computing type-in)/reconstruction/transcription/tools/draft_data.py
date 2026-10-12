"""Draft transcription of the data-file dump rows.

Each row starts as the OCR string laid out on its cells (so the spacing is
exactly as printed; cells inside a drawing's box are blanked).  Where an OCR
word lines up with a word of the nd-seeded text for that row and the two are
the same word up to glyph confusions (s/$, l/[/1/!, u/v, g/q, e/c, ...), the
nd spelling is taken.  Anything else stays as the OCR read it: nd deviates
from the printout in places, and genuine misspellings in the original must
survive.  The draft is for proof-reading, not a result.

Writes tools/draft_data.txt in truth-file format.
"""
import difflib, json, os, re
import numpy as np
import pagegeom as pg
import truth as T
from truth import load_cells, parse, TRUTH
from match_nd import norm

OUT = os.path.join(pg.HERE, "draft_data.txt")


def main():
    img, meta, core, index = load_cells()
    ocr = np.load(os.path.join(pg.WORK, "ocr.npz"))
    lab = ocr["lab"]
    seeds = {(p, c, r): t for p, c, r, t in parse(TRUTH)}
    out = []
    for (p, c, r), cells in sorted(index.items()):
        g = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (p, c)))) if False else None
        chars = []
        for i in cells:
            ch = str(lab[i])
            if core[i] < 12.0 or (T.ART_FLAGS[i] and ocr["score"][i] < 0.75):
                ch = " "
            chars.append(ch)
        o = "".join(chars).rstrip()
        # drop a detached tail: 8+ spaces then a short scrap (art edges, specks)
        m = re.match(r"^(.*?\S)\s{8,}(\S{1,4})$", o)
        if m:
            o = m.group(1)
        seed = seeds.get((p, c, r))
        if seed:
            ow = [(mm.group(0), mm.start()) for mm in re.finditer(r"\S+", o)]
            sw = [mm.group(0) for mm in re.finditer(r"\S+", seed)]
            sm = difflib.SequenceMatcher(None, [norm(w) for w, _ in ow], [norm(w) for w in sw], autojunk=False)
            buf = list(o)
            for tag, i1, i2, j1, j2 in sm.get_opcodes():
                if tag in ("equal", "replace") and (i2 - i1) == (j2 - j1):
                    for k in range(i2 - i1):
                        w, pos = ow[i1 + k]
                        s = sw[j1 + k]
                        if len(w) == len(s) and norm(w) == norm(s):
                            buf[pos:pos + len(w)] = list(s)
            o = "".join(buf)
        ys = json.load(open(os.path.join(pg.WORK, "rowy.json")))
        gy = json.load(open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (p, c))))["rows"][r]["y"]
        out.append("%d %d @%d: %s" % (p, c, round(gy), o))
    open(OUT, "w", encoding="utf-8").write("# DRAFT data rows (OCR + nd spellings) - to be proof-read\n" + "\n".join(out) + "\n")
    print(OUT, len(out))


if __name__ == "__main__":
    main()
