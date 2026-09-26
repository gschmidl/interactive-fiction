# -*- coding: utf-8 -*-
"""Magazine bracket-notation listing  ->  petcat source text.
Unbracketed letters in the listing are UNSHIFTED PETSCII  -> lowercase for petcat.
[S>X] marks a SHIFTED character                            -> uppercase for petcat.
"""
import re, sys

CTRL = {
 "CD":"{down}", "CU":"{up}", "CR":"{rght}", "CL":"{left}",
 "CLS":"{clr}", "HOM":"{home}", "REV":"{rvon}", "OFF":"{rvof}",
 "DEL":"{del}", "BLK":"{blk}", "WHT":"{wht}", "RED":"{red}",
 "CYN":"{cyn}", "PUR":"{pur}", "GRN":"{grn}", "BLU":"{blu}",
 "YEL":"{yel}", "ORG":"{orng}", "BRN":"{brn}", "LTRED":"{lred}",
 "GR1":"{gry1}", "GR2":"{gry2}", "GR3":"{gry3}",
 "LTGRN":"{lgrn}", "LTBLU":"{lblu}",
 "LBR":"[", "RBR":"]",
}
TOKEN = re.compile(r"\[([^\]]*)\]")

def expand(tag):
    # [S>X] / [nS>X]
    m = re.fullmatch(r"(\d*)S>(.)", tag)
    if m:
        n = int(m.group(1) or 1)
        return m.group(2).upper() * n
    # [nSPC]
    m = re.fullmatch(r"(\d*)SPC", tag)
    if m:
        return " " * int(m.group(1) or 1)
    # [40-]  run of literal dashes
    m = re.fullmatch(r"(\d+)-", tag)
    if m:
        return "-" * int(m.group(1))
    if tag in CTRL:
        return CTRL[tag]
    raise SystemExit("unknown directive: [%s]" % tag)

def convert(line):
    out = []
    pos = 0
    for m in TOKEN.finditer(line):
        out.append(line[pos:m.start()].lower())   # plain text -> unshifted
        out.append(expand(m.group(1)))            # directive  -> literal
        pos = m.end()
    out.append(line[pos:].lower())
    return "".join(out)

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    extra = sys.argv[3:]                          # optional extra "NNN body" lines
    lines = {}
    for raw in open(src, encoding="utf-8"):
        raw = raw.rstrip("\n")
        if not raw.strip(): continue
        n, _, body = raw.partition(" ")
        lines[int(n)] = body
    for e in extra:
        n, _, body = e.partition(" ")
        lines[int(n)] = body
    with open(dst, "w", encoding="latin-1", newline="\n") as o:
        for n in sorted(lines):
            o.write("%d %s\n" % (n, convert(lines[n])))
    print("wrote", dst, len(lines), "lines")
