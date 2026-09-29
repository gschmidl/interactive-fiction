"""Assemble the data files from the proof-read dump transcription and diff them against data_nd.

Rows are read in page/column/y order.  A vertical gap of about two row
pitches is a blank record.  ">DUMP name/FOOBAR" rows start a new file.
Writes ../data_print/AITEMS, ADESCRIP, AMESSAGE (as printed; no completion of
truncated records yet) and .work_data/nd_diff.txt.
"""
import difflib, json, os, re
import numpy as np
import pagegeom as pg

HERE = pg.HERE
PROJ = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(PROJ, "transcription", "data_print")
ND = os.path.join(PROJ, "data_nd")
DRAFT = os.environ.get("ADV_TRUTH") or os.path.join(HERE, "draft_data.txt")


# Records the 73-column dump device cut off.  The tail cannot be read from the
# page; it is taken from the other witnesses and marked in the report.
COMPLETE = {
    (116, 0, 2857): ("l.", 'nd + PyBasic: "locked to the wall."'),
    (118, 0, 2509): ("e.", 'nd: "blows up the staircase."'),
    (118, 0, 4488): ("ur", 'nd + PyBasic: "out of your" (the print reads "uo" where nd has "to")'),
    (118, 1, 4471): (" block.", 'PyBasic only: "a large bedrock block."; nd stops where the print stops'),
    (120, 0, 2293): ("s.", 'PyBasic only: "cover the walls."; nd has no #252 at all'),
}


def main():
    rows, completed = [], []
    for l in open(DRAFT, encoding="utf-8"):
        if l.startswith("#") or ": " not in l:
            if l.strip() and not l.startswith("#"):
                pass
            if not l.startswith("# ") and l.rstrip("\n").endswith(":") and "@" in l:
                p, c, y = l.split(":")[0].split()
                rows.append((int(p), int(c), int(y[1:]), ""))
            continue
        head, text = l.rstrip("\n").split(": ", 1)
        p, c, y = head.split()
        key = (int(p), int(c), int(y[1:]))
        if key in COMPLETE:
            text += COMPLETE[key][0]
            completed.append((key, text))
        rows.append((key[0], key[1], key[2], text))
    rows.sort()
    lines = []
    for (p, c), grp in _groupby(rows):
        ys = [y for _, _, y, _ in grp]
        pitch = float(np.median(np.diff(ys))) if len(ys) > 2 else 62.0
        for n, (_, _, y, text) in enumerate(grp):
            if n:
                gap = y - grp[n - 1][2]
                for _ in range(int(round(gap / pitch)) - 1):
                    lines.append(("p%d c%d before y%d" % (p, c, y), ""))
            lines.append(("p%d c%d y%d" % (p, c, y), text.rstrip()))
    files, cur = {}, None
    for where, text in lines:
        m = re.match(r"^\W?DUMP (\w+)/FOOBAR", text.strip())
        if m:
            cur = m.group(1); files[cur] = []; continue
        if cur:
            files[cur].append((where, text))
    os.makedirs(OUT, exist_ok=True)
    report = []
    for name, recs in files.items():
        # trailing blank lines at a file's end are page layout, not records
        while recs and not recs[-1][1]:
            recs.pop()
        open(os.path.join(OUT, name), "w", encoding="ascii", errors="replace", newline="\n").write("\n".join(t for _, t in recs) + "\n")
        nd = open(os.path.join(ND, name), encoding="latin-1").read().replace("\r", "").split("\n")
        if nd and nd[-1] == "":
            nd.pop()
        pr = [t for _, t in recs]
        report.append("===== %s: printed %d records, nd %d lines" % (name, len(pr), len(nd)))
        for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, pr, nd, autojunk=False).get_opcodes():
            if tag == "equal":
                continue
            report.append("--- %s print[%d:%d] (%s) vs nd[%d:%d]" % (tag, i1, i2, recs[i1][0] if i1 < len(recs) else "end", j1, j2))
            for t in pr[i1:i2]:
                report.append("  P| " + t)
            for t in nd[j1:j2]:
                report.append("  N| " + t)
    report.append("")
    report.append("===== records completed past the dump device's 73-column cut")
    for key, text in completed:
        report.append("  p%d c%d y%d -> %s" % (key[0], key[1], key[2], text))
        report.append("     source: %s" % COMPLETE[key][1])
    open(os.path.join(pg.WORK, "nd_diff.txt"), "w", encoding="utf-8").write("\n".join(report) + "\n")
    print("\n".join(r for r in report if r.startswith("=====")))
    print("diff lines", len(report))


def _groupby(rows):
    out, key, grp = [], None, []
    for r in rows:
        k = (r[0], r[1])
        if k != key and grp:
            out.append((key, grp)); grp = []
        key = k; grp.append(r)
    if grp:
        out.append((key, grp))
    return out


if __name__ == "__main__":
    main()
