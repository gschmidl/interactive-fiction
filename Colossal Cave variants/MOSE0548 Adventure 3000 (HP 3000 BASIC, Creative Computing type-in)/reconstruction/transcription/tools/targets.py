"""Resolve every line-number reference in ADVENTURE3000.BAS.

References checked (outside string literals):
  GOTO n, GOSUB n, THEN n, ELSE n
  GOTO/GOSUB expr OF n,n,...
  RESTORE n
  ON END #f THEN n
  CONVERT a TO b,n        (error-exit line)
Also reports lines that nothing jumps to and that do not follow a statement
which can fall through (dead code), and REM-only targets (legal in HP BASIC,
but worth knowing).
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, "..", "ADVENTURE3000.BAS"))


def strip_strings(s):
    out, q = [], False
    for ch in s:
        if ch == '"':
            q = not q
            out.append('"')
        else:
            out.append(" " if q else ch)
    return "".join(out)


def main():
    lines = {}
    order = []
    for raw in open(SRC, encoding="ascii"):
        m = re.match(r"(\d+) (.*)$", raw.rstrip("\n"))
        n = int(m.group(1)); lines[n] = m.group(2); order.append(n)
    refs = []   # (from, kind, to)
    for n in order:
        body = lines[n]
        if body.startswith("REM"):
            continue
        s = strip_strings(body)
        for m in re.finditer(r"\b(GOTO|GOSUB)\s+([^,]*?)\s+OF\s+([\d,\s]+)", s):
            for t in re.findall(r"\d+", m.group(3)):
                refs.append((n, m.group(1) + " OF", int(t)))
        s2 = re.sub(r"\b(GOTO|GOSUB)\s+[^,]*?\s+OF\s+[\d,\s]+", " ", s)
        for m in re.finditer(r"\b(GOTO|GOSUB|THEN|ELSE)\s+(\d+)\b", s2):
            refs.append((n, m.group(1), int(m.group(2))))
        for m in re.finditer(r"\bRESTORE\s+(\d+)", s2):
            refs.append((n, "RESTORE", int(m.group(1))))
        for m in re.finditer(r"\bCONVERT\s+.*?\bTO\s+[A-Z]\d?\s*,\s*(\d+)", s2):
            refs.append((n, "CONVERT", int(m.group(1))))
    missing = [(f, k, t) for f, k, t in refs if t not in lines]
    print("statements %d, references %d, missing %d" % (len(order), len(refs), len(missing)))
    for f, k, t in missing:
        print("  MISSING  line %d: %s %d   | %s" % (f, k, t, lines[f]))
    rem_targets = sorted({(t, k) for f, k, t in refs if t in lines and lines[t].startswith("REM") and k != "RESTORE"})
    print("jumps to REM lines: %d" % len(rem_targets))
    restore_bad = [(f, t) for f, k, t in refs if k == "RESTORE" and t in lines and not lines[t].startswith("DATA")]
    for f, t in restore_bad:
        print("  RESTORE %d from line %d is not a DATA line: %s" % (t, f, lines[t]))
    # unreachable: not a jump target and previous statement cannot fall through
    targets = {t for _, _, t in refs}
    # GOTO/GOSUB ... OF falls through when the index is out of range
    nofall = re.compile(r"^(GOTO\s+\d+$|RETURN|STOP$|END$|ELSE GOTO)")
    dead = []
    for i, n in enumerate(order[1:], 1):
        prev = lines[order[i - 1]]
        if n not in targets and nofall.match(prev) and not lines[n].startswith("REM"):
            if not lines[n].startswith("DATA") and not lines[n].startswith("DEF") and not lines[n].startswith("FNEND"):
                dead.append(n)
    print("statements after an unconditional jump that nothing reaches: %s" % dead)


if __name__ == "__main__":
    main()
