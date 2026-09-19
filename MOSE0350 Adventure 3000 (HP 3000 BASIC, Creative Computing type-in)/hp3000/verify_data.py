"""Check the data files on the HP 3000 against the local copies, byte for byte.

Writes a BASIC program that walks each file and prints, per record, its length
and a position-weighted checksum of its character codes; then compares that
with the same checksum computed here.  A checksum this way catches a wrong
byte, a wrong order and a missing record, including in the movement table
whose records are binary.

usage: verify_data.py [play|print]
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.normpath(os.path.join(HERE, "..", "transcription"))
FILES = ("AITEMS", "ADESCRIP", "AMESSAGE", "AMOVING")


def checksum(s):
    return sum((i + 1) * b for i, b in enumerate(s))


def local_records(src):
    out = {}
    for name in ("AITEMS", "ADESCRIP", "AMESSAGE"):
        lines = open(os.path.join(src, name), "rb").read().decode("ascii").split("\n")
        if lines and lines[-1] == "":
            lines.pop()
        out[name] = [l.encode("ascii") for l in lines]
    rows = [l.split() for l in open(os.path.join(src, "AMOVING.txt"), encoding="ascii") if l.strip()]
    out["AMOVING"] = [bytes(int(v) for v in row) for row in rows]
    return out


def main(which="play"):
    src = os.path.join(T, "data_play" if which == "play" else "data_print")
    want = local_records(src)
    script = ["#BASIC", "10 DIM A$[120]"]
    n = 10
    for fi, name in enumerate(FILES, start=1):
        n += 10
        script.append("%d FILES %s" % (5, name) if False else "5 FILES AITEMS,ADESCRIP,AMESSAGE,AMOVING")
        script.pop()
        break
    script = ["#BASIC",
              "5 FILES AITEMS,ADESCRIP,AMESSAGE,AMOVING",
              "10 DIM A$[120]",
              "20 FOR F=1 TO 4",
              '30 PRINT "FILE";F',
              "40 GOSUB 100",
              "50 NEXT F",
              '60 PRINT "ALL DONE"',
              "70 STOP",
              "100 READ #F,1",
              "110 ON END #F THEN 200",
              "120 READ #F;A$",
              "130 S=0",
              "140 FOR I=1 TO LEN(A$)",
              "150 S=S+I*NUM(A$[I,I])",
              "160 NEXT I",
              "170 PRINT LEN(A$);S",
              "180 GOTO 120",
              "200 PRINT -1;-1",
              "210 RETURN",
              "220 END",
              "RUN"]
    path = os.path.join(HERE, "dump_data.txt")
    open(path, "w", encoding="ascii", newline="\n").write("\n".join(script) + "\n")
    print("running the dump on the machine ...")
    r = subprocess.run([sys.executable, "-u", os.path.join(HERE, "basic_session.py"), path],
                       capture_output=True, text=True, timeout=1800)
    text = r.stdout
    # parse: "FILE n" then "len sum" lines until "-1 -1"
    got, cur = {}, None
    for line in text.splitlines():
        line = line.strip()
        m = re.match(r"^FILE\s+(\d+)$", line)
        if m:
            cur = FILES[int(m.group(1)) - 1]
            got[cur] = []
            continue
        m = re.match(r"^(-?\d+)\s+(-?\d+)\s*$", line)
        if m and cur:
            a, b = int(m.group(1)), int(m.group(2))
            if a == -1:
                cur = None
            else:
                got[cur].append((a, b))
    ok = True
    for name in FILES:
        w = [(len(r), checksum(r)) for r in want[name]]
        g = got.get(name, [])
        if g == w:
            print("%-9s %4d records: identical on the machine" % (name, len(w)))
        else:
            ok = False
            print("%-9s DIFFERS: machine %d records, local %d" % (name, len(g), len(w)))
            for i in range(max(len(g), len(w))):
                gi = g[i] if i < len(g) else None
                wi = w[i] if i < len(w) else None
                if gi != wi:
                    print("   record %d: machine %s, local %s" % (i + 1, gi, wi))
                    if sum(1 for j in range(max(len(g), len(w))) if (g[j] if j < len(g) else None) != (w[j] if j < len(w) else None)) > 12 and i > 10:
                        print("   ...")
                        break
    print("VERIFIED" if ok else "MISMATCH")
    if not ok:
        open(os.path.join(HERE, "dump_data.raw"), "w", encoding="utf-8").write(text)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "play")
