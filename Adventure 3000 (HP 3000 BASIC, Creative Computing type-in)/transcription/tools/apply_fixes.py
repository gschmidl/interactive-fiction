"""Build the playable data files and program from the as-printed ones plus fixes.txt.

The as-printed transcription is never edited: this writes data_play/ and
ADVENTURE3000_fixed.BAS.  Every fix must match the printed text exactly, so a
fix that no longer applies fails loudly instead of being ignored.
"""
import os, sys
import pagegeom as pg

PROJ = os.path.normpath(os.path.join(pg.HERE, "..", ".."))
T = os.path.join(PROJ, "transcription")


def main():
    fixes = []
    for l in open(os.path.join(T, "fixes.txt"), encoding="utf-8"):
        l = l.rstrip("\n")
        if not l.strip() or l.startswith("#"):
            continue
        f, rec, before, after, why = l.split("|", 4)
        fixes.append((f.strip(), int(rec), before, after, why))
    os.makedirs(os.path.join(T, "data_play"), exist_ok=True)
    applied, bad = 0, 0
    for name in ("AITEMS", "ADESCRIP", "AMESSAGE"):
        lines = open(os.path.join(T, "data_print", name), encoding="ascii").read().split("\n")
        if lines and lines[-1] == "":
            lines.pop()
        for f, rec, before, after, why in fixes:
            if f != name:
                continue
            if lines[rec - 1] != before:
                print("FIX DOES NOT MATCH %s record %d:\n  file  %r\n  fixes %r" % (name, rec, lines[rec - 1], before))
                bad += 1
                continue
            lines[rec - 1] = after
            applied += 1
        open(os.path.join(T, "data_play", name), "w", encoding="ascii", newline="\n").write("\n".join(lines) + "\n")
    # the movement table has no text fixes yet
    src = open(os.path.join(T, "data_print", "AMOVING.txt"), encoding="ascii").read()
    open(os.path.join(T, "data_play", "AMOVING.txt"), "w", encoding="ascii", newline="\n").write(src)
    # the program
    prog = open(os.path.join(T, "ADVENTURE3000.BAS"), encoding="ascii").read().split("\n")
    idx = {}
    for n, l in enumerate(prog):
        if l[:1].isdigit():
            idx[int(l.split(" ", 1)[0])] = n
    for f, rec, before, after, why in fixes:
        if f != "PROGRAM":
            continue
        n = idx.get(rec)
        if n is None or prog[n] != before:
            print("FIX DOES NOT MATCH program line %d:\n  file  %r\n  fixes %r" % (rec, prog[n] if n is not None else None, before))
            bad += 1
            continue
        prog[n] = after
        applied += 1
    open(os.path.join(T, "ADVENTURE3000_fixed.BAS"), "w", encoding="ascii", newline="\n").write("\n".join(prog))
    print("fixes applied %d, problems %d" % (applied, bad))


if __name__ == "__main__":
    main()
