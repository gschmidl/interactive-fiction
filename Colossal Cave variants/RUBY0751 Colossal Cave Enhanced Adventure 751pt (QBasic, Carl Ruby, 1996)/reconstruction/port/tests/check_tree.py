r"""Static checks of the tree make_tree.py builds (../ADV751 unless given):

1. make_tree's cutting loses nothing: the entries put back together give each dump again, byte
   for byte (ROOM5's terminator is the one "//q").
2. Every file ADV751.BAS can open is there: the names written into the program, and the ones
   it builds while running - a room's long description for every room the travel table (from
   REFILL.BAS) and the program's own moves can reach, BRIDGE0/1, CHUTE1/2, PLANT1-3, the
   hints, the books and papers READ opens, CABINET, CHANGE\CLOAK.

    python check_tree.py [TREE]
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, ".."))
import make_tree                                                    # noqa: E402

TREE = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "ADV751"))
CRLF = b"\r\n"
failed = []


def check(ok, what):
    if not ok:
        failed.append(what)
        print("FAIL", what)


# 1. round trip
for dump in ("rooms.txt", "texts.txt", "1timers.txt"):
    raw = open(os.path.join(make_tree.SRC, dump), "rb").read()
    again = b""
    for i, (lines, after) in enumerate(make_tree.entries(dump)):
        end = b"//q" if (dump, i) == ("rooms.txt", 4) else b"//"
        again += b"".join(line + CRLF for line in lines + [end] + after)
    check(again == raw, "%s does not come back byte for byte" % dump)

# 2. what the program opens
program = os.path.join(TREE, "ADV751.BAS")         # the collection's copy has only ADV751.EXE,
if not os.path.exists(program):                      # compiled from the port's ADV751.BAS
    program = os.path.join(HERE, "..", "ADV751", "ADV751.BAS")
src = open(program, "rb").read().decode("cp437")
want = set()
for line in src.split("\r\n"):
    for m in re.finditer(r'(?:^\d+ |THEN |: )M\$ = (C1\$ \+ )?"([^"]+)": (?:[A-Z]+ = \d+: )*'
                         r'(?:DI\(\d+, \d+\) = \d+: )?(?:NI = \d+: )?GOSUB (89[05]0)', line):
        prefix, name, sub = m.groups()
        if sub == "8950":
            want.add("CHANGE/" + name)
        else:
            want.add(("1TIMERS/" if prefix else "") + name.replace("\\", "/") + ".TXT")
want |= {"BRIDGE0.TXT", "BRIDGE1.TXT", "CHUTE1.TXT", "CHUTE2.TXT", "CASABLNC.TXT",
         "PLANT1.TXT", "PLANT2.TXT", "PLANT3.TXT", "POWDER.TXT", "ALT139.TXT",
         "1TIMERS/CABINET.TXT", "CHANGE/CLOAK", "CHANGE/PLANT1", "CHANGE/PLANT2", "CHANGE/PLANT3",
         "HINTS/ALT1.TXT", "RMS/ALT27.TXT", "RMS/ALT139.TXT", "RMS/ALT199.TXT"}
want |= {"HINTS/%s%d.TXT" % (kind, n) for kind in ("ASK", "HINT") for n in range(2, 7)}
want |= {w + ".TXT" for w in ("LEAFLET", "POSTER", "BOOK", "CARD", "SHIELD", "DOCUMENT", "PAPER")}
want = {("CHANGE/CONTENTE" if w == "CHANGE/CONTENTED" else w) for w in want}

# rooms: DI from REFILL.BAS's DATA (P > 290 uses row P - 50), plus every room the program
# moves the player to itself
refill = open(os.path.join(make_tree.SRC, "REFILL.BAS"), "rb").read().decode("cp437").split("\r\n")
rows = [[int(v) for v in m.group(2).split(",")]
        for m in (re.match(r"(\d+) DATA ([\d,]+)$", l) for l in refill)
        if m and 20001 <= int(m.group(1)) <= 20296]
check(len(rows) == 246, "REFILL.BAS has %d travel rows, not 246" % len(rows))
exits = {p: [q for q in rows[(p - 50 if p > 290 else p) - 1] if 0 < q < 500]
         for p in list(range(1, 247)) + list(range(291, 297))}
todo = [1] + [int(n) for n in re.findall(r"\bP = (\d+)(?=\s*:|\s*$|\s+GOTO)", src) if 0 < int(n) < 300]
seen = set()
while todo:
    p = todo.pop()
    if p not in seen:
        seen.add(p)
        todo += exits.get(p, [])
for p in sorted(seen):
    if (p < 63 or p == 88 or 120 < p <= 221) and p not in (126, 135, 136):
        want.add("RMS/ROOM%d.TXT" % p)

for rel in sorted(want):
    check(os.path.isfile(os.path.join(TREE, *rel.split("/"))), "missing " + rel)
check(os.path.isdir(os.path.join(TREE, "GAMES")), "missing GAMES\\")
check(os.path.isfile(os.path.join(TREE, "VAR")), "missing VAR (run REFILL.BAS)")

# every .TXT the reader (line 8900) opens must end its text with a "//" line
for rel in sorted(w for w in want if w.endswith(".TXT") and w != "CASABLNC.TXT"):
    path = os.path.join(TREE, *rel.split("/"))
    if os.path.isfile(path):
        check(b"//" in open(path, "rb").read().split(CRLF), rel + " has no // line")

print("%d files the program can open, %d rooms reachable: %s" % (
    len(want), len(seen), "all there" if not failed else "%d problems" % len(failed)))
sys.exit(1 if failed else 0)
