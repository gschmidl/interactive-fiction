#!/usr/bin/env python3
"""
check_data.py — validate V1's DATA block against what the source requires.

Line 70 of UNDERG (14-Mar-79) does:

    M%(X%,0%)=0% for X%=1% to 87% : O%(0%)=39% : MAT READ M%,O% : X%(0%)=8%

`MAT READ M%,O%` on DIM M%(87%,14), O%(45) fills M%(1..87, 1..14) and
O%(1..45) — BASIC-PLUS MAT operations skip row/column 0, which is exactly why
column 0 is zeroed separately on the same line.

So the DATA block must supply, in order:
    87 rooms x 14 values = 1218   then   45 object locations
    ------------------------------------------------------- 1263 values total

and the room rows are one-per-line at 80,90,...,500, 501..507, 510,...,870.

Empty fields count: ",,8," is three values (0, 0, 8). Getting a single comma
wrong shifts every subsequent room, so this check is worth running on any
transcription before pasting it into RSTS/E.

Usage:  python check_data.py UNDERG-V1.BAS
"""
import sys, re

EXPECTED_ROOM_LINES = (list(range(80, 501, 10)) + list(range(501, 508))
                       + list(range(510, 871, 10)))
VALS_PER_ROOM = 14
N_OBJECTS = 45

def parse(path):
    rows = {}
    for line in open(path, encoding="utf-8"):
        m = re.match(r'^\s*(\d+)\s*DATA(.*)$', line, re.I)
        if not m:
            continue
        n = int(m.group(1))
        body = m.group(2).rstrip("\n")
        # A trailing comma DOES yield a final (empty -> 0) value in BASIC DATA;
        # do not strip it. Verified empirically: stripping made every 14-value
        # room row read as 13.
        rows[n] = [p.strip() for p in body.split(",")]
    return rows

def main(path):
    rows = parse(path)
    if not rows:
        print("no DATA lines found in", path); return 1

    ok = True
    print("room rows")
    print("-" * 60)
    missing = [n for n in EXPECTED_ROOM_LINES if n not in rows]
    if missing:
        ok = False
        print("  MISSING lines:", missing)
    for n in EXPECTED_ROOM_LINES:
        if n in rows and len(rows[n]) != VALS_PER_ROOM:
            ok = False
            print("  line %-4d has %2d values, expected %d   %s"
                  % (n, len(rows[n]), VALS_PER_ROOM, ",".join(rows[n])))
    present = [n for n in EXPECTED_ROOM_LINES if n in rows]
    print("  %d of %d room rows present" % (len(present), len(EXPECTED_ROOM_LINES)))

    obj_lines = sorted(n for n in rows if n not in EXPECTED_ROOM_LINES)
    n_obj = sum(len(rows[n]) for n in obj_lines)
    print()
    print("object rows: lines %s -> %d values (expected %d)"
          % (obj_lines, n_obj, N_OBJECTS))
    if n_obj != N_OBJECTS:
        ok = False

    total = sum(len(v) for v in rows.values())
    print()
    print("total DATA values = %d (expected %d)"
          % (total, len(EXPECTED_ROOM_LINES) * VALS_PER_ROOM + N_OBJECTS))

    # Sanity: destinations must be a room number 1..87, a negative message
    # code, or 0 for "no exit".
    bad = []
    for n in present:
        for j, v in enumerate(rows[n], start=1):
            if v == "":
                continue
            try:
                iv = int(v)
            except ValueError:
                bad.append((n, j, v)); continue
            if iv > 87 or iv < -20:
                bad.append((n, j, v))
    if bad:
        ok = False
        print()
        print("values out of range (expect 0, 1..87, or a small negative):")
        for n, j, v in bad[:20]:
            print("   line %d field %d = %s" % (n, j, v))

    print()
    print("RESULT:", "OK" if ok else "PROBLEMS FOUND — do not paste yet")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "UNDERG-V1.BAS"))
