"""Propose a transcription for every data-page row from the nd data files.

The dump prints, in order: DUMP AITEMS, the 35 item lines, >DUMP ADESCRIP,
the 100 short descriptions, >DUMP AMESSAGE and the message file.  The nd
files hold the same text typed in (long records joined onto one line, some
records missing).  Words of the OCR'd rows are aligned to the words of that
stream (difflib over normalised words); each printed row then takes the exact
nd characters spanning its matched words.  Rows with too little matched are
left as OCR text and marked "?" for transcription by eye.

Writes work_data/proposed.txt:  <page> <col> @<y>: <text>      (?-prefixed = unmatched)
"""
import difflib, os, re
import pagegeom as pg

PROJ = os.path.normpath(os.path.join(pg.HERE, "..", ".."))
ND = os.path.join(PROJ, "data_nd")
DATA_WORK = os.path.normpath(os.path.join(pg.HERE, "..", "work_data"))

CONF = str.maketrans({"$": "s", "[": "l", "]": "l", "1": "l", "|": "l", "!": "l", "v": "u", "y": "u",
                      "q": "g", "0": "o", "5": "s", "e": "c", "a": "o", "n": "h"})


def norm(w):
    w = re.sub(r"[^A-Za-z0-9$\[\]|!]", "", w).lower()
    return w.translate(CONF)


def nd_stream():
    lines = ["DUMP AITEMS/FOOBAR"]
    lines += open(os.path.join(ND, "AITEMS"), encoding="latin-1").read().replace("\r", "").splitlines()
    lines += [">DUMP ADESCRIP/FOOBAR"]
    lines += open(os.path.join(ND, "ADESCRIP"), encoding="latin-1").read().replace("\r", "").splitlines()
    lines += [">DUMP AMESSAGE/FOOBAR"]
    lines += open(os.path.join(ND, "AMESSAGE"), encoding="latin-1").read().replace("\r", "").splitlines()
    return "\n".join(lines)


def words_with_spans(text):
    return [(m.group(0), m.start(), m.end()) for m in re.finditer(r"\S+", text)]


def main():
    nd = nd_stream()
    ndw = words_with_spans(nd)
    rows = []
    for line in open(os.path.join(DATA_WORK, "ocr_rows.txt"), encoding="utf-8"):
        head, text = line.rstrip("\n").split(": ", 1) if ": " in line else (line.rstrip("\n").rstrip(":"), "")
        rows.append((head, text))
    ocrw = []   # (norm word, row index)
    for ri, (head, text) in enumerate(rows):
        # drop the art-noise tail: text after a run of 6+ spaces followed by junk
        t = re.split(r"\s{8,}", text)[0] if text.strip() else ""
        rows[ri] = (head, t)
        for w, s, e in words_with_spans(t):
            ocrw.append((norm(w), ri))
    a = [w for w, _ in ocrw]
    b = [norm(w) for w, _, _ in ndw]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    match_of = {}
    for blk in sm.get_matching_blocks():
        for k in range(blk.size):
            match_of[blk.a + k] = blk.b + k
    # near-matches inside replace opcodes of equal length
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "replace" and (i2 - i1) == (j2 - j1):
            for k in range(i2 - i1):
                if difflib.SequenceMatcher(None, a[i1 + k], b[j1 + k]).ratio() >= 0.6:
                    match_of[i1 + k] = j1 + k
    by_row = {}
    for wi, (w, ri) in enumerate(ocrw):
        by_row.setdefault(ri, []).append(wi)
    # nd line index of every nd word, and each line's word range
    line_of = [nd.count("\n", 0, s0) for _, s0, _ in ndw]
    nd_lines = nd.split("\n")
    line_start = {}
    for j, ln in enumerate(line_of):
        line_start.setdefault(ln, j)
    prop = {}          # row index -> (text, first nd line, last nd line)
    for ri, (head, text) in enumerate(rows):
        wis = by_row.get(ri, [])
        if not wis:
            continue
        hits = [(k, match_of[w]) for k, w in enumerate(wis) if w in match_of]
        if len(hits) < max(1, 0.5 * len(wis)):
            continue
        (k1, j1), (k2, j2) = hits[0], hits[-1]
        # unmatched OCR words before the first / after the last hit: take the
        # same number of nd words, staying on that nd line
        j1 = max(j1 - k1, line_start[line_of[j1]])
        tail = len(wis) - 1 - k2
        while tail > 0 and j2 + 1 < len(ndw) and line_of[j2 + 1] == line_of[j2]:
            j2 += 1; tail -= 1
        if line_of[j1] != line_of[j2]:
            continue
        prop[ri] = (nd[ndw[j1][1]:ndw[j2][2]], line_of[j1], line_of[j2])
    # fill runs of unmatched rows 1:1 with the whole nd lines between the
    # neighbouring matched rows (record numbers, repeated maze lines)
    filled = 0
    ri = 0
    n = len(rows)
    while ri < n:
        if ri in prop or not rows[ri][1].strip():
            ri += 1
            continue
        a = ri
        while ri < n and ri not in prop and rows[ri][1].strip():
            ri += 1
        b = ri                                   # rows[a:b] unmatched
        prev = max((k for k in prop if k < a), default=None)
        nxt = ri if ri in prop else None
        if prev is None or nxt is None:
            continue
        lo, hi = prop[prev][2] + 1, prop[nxt][1]  # nd lines strictly between
        cand = [j for j in range(lo, hi) if nd_lines[j].strip()]
        # the previous matched row may be the first half of a wrapped nd line
        if len(cand) == b - a:
            for k, j in zip(range(a, b), cand):
                prop[k] = (nd_lines[j], j, j)
                filled += 1
    out, unmatched = [], 0
    for ri, (head, text) in enumerate(rows):
        if not text.strip():
            out.append("%s: " % head)
        elif ri in prop:
            out.append("%s: %s" % (head, prop[ri][0]))
        else:
            unmatched += 1
            out.append("%s: ?%s" % (head, text))
    print("gap-filled", filled)
    open(os.path.join(DATA_WORK, "proposed.txt"), "w", encoding="utf-8").write("\n".join(out) + "\n")
    print("rows", len(rows), "unmatched", unmatched)


if __name__ == "__main__":
    main()
