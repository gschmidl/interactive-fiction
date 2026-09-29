"""Generate the BASIC/3000 entry script for the program.

The interpreter's input buffer stops at 131 characters, so statements longer
than that go in as the author's own LIST wrote them: broken at a comma outside
a string literal, with "&" continuing the line.

usage: gen_program.py [fixed|print]      (default: fixed = with the fixes applied)
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.normpath(os.path.join(HERE, "..", "transcription"))
LIMIT = 126          # leave room for the "&"


def split_statement(line):
    """Break one statement into rows of <= LIMIT chars, at commas outside quotes."""
    if len(line) <= LIMIT:
        return [line]
    # positions just after a comma that is not inside a string
    cuts, inq = [], False
    for i, ch in enumerate(line):
        if ch == '"':
            inq = not inq
        elif ch == "," and not inq:
            cuts.append(i + 1)
    rows, start = [], 0
    while len(line) - start > LIMIT:
        here = [c for c in cuts if start < c <= start + LIMIT]
        if not here:
            raise SystemExit("cannot split line safely: %r" % line[:60])
        cut = here[-1]
        rows.append(line[start:cut] + "&")
        start = cut
    rows.append(line[start:])
    return rows


def main(which="fixed"):
    src = os.path.join(T, "ADVENTURE3000_fixed.BAS" if which == "fixed" else "ADVENTURE3000.BAS")
    prog = [l.rstrip() for l in open(src, encoding="ascii").read().split("\n") if l.strip()]
    rows, nsplit = [], 0
    for line in prog:
        parts = split_statement(line)
        if len(parts) > 1:
            nsplit += 1
        rows += parts
    name = "ADV3000" if which == "fixed" else "ADV3000P"
    out = ["#BASIC"] + rows + ["SAVE " + name, "EXIT", "#MPE", "LISTF %s,2" % name]
    path = os.path.join(HERE, "enter_%s.txt" % name)
    open(path, "w", encoding="ascii", newline="\n").write("\n".join(out) + "\n")
    print("%d statements -> %d rows (%d split), longest row %d -> %s"
          % (len(prog), len(rows), nsplit, max(len(r) for r in rows), os.path.basename(path)))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "fixed")
