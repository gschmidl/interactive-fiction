# -*- coding: utf-8 -*-
"""Split the readable glossary (document 0x01) into its entries.

An entry starts with the four-byte marker  03 '(' key ')' 03  and runs to the
next one.  The stream begins part-way through the entry the player invokes to
start the game -- document 0x01's first block is 08:0B and its payload opens
mid-token -- so that leading fragment is reported under the name '*'.

    python gloss.py                # summary
    python gloss.py -l             # full listing
    python gloss.py -e c           # one entry
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DOC = os.path.join(HERE, "..", "data", "doc01.bin")

MARKER = re.compile(rb"\x03\(([ -~])\)\x03")


def load(path=DOC):
    return open(path, "rb").read()


def entries(raw=None):
    """[(key, body)] in the order they appear; the lead fragment is '*'."""
    raw = load() if raw is None else raw
    flat = bytes(b & 0x7F for b in raw)
    marks = list(MARKER.finditer(flat))
    out = []
    if marks and marks[0].start() > 0:
        out.append(("*", raw[:marks[0].start()]))
    for i, m in enumerate(marks):
        end = marks[i + 1].start() if i + 1 < len(marks) else len(raw)
        out.append((m.group(1).decode("latin1"), raw[m.end():end]))
    return out


def render(body):
    """Readable text: bit 7 off, control bytes shown as <XX>."""
    out = []
    for b in body:
        c = b & 0x7F
        if b < 0x20:
            out.append("<%02X>" % b)
        elif 32 <= c < 127:
            out.append(chr(c))
        else:
            out.append("<%02X>" % b)
    return "".join(out)


def main():
    args = sys.argv[1:]
    ents = entries()
    if "-e" in args:
        want = args[args.index("-e") + 1]
        for k, body in ents:
            if k == want:
                print(render(body))
        return
    if "-l" in args:
        for k, body in ents:
            print("=" * 70)
            print("(%s)   %d bytes" % (k, len(body)))
            print("=" * 70)
            print(render(body))
            print()
        return
    print("%d entries" % len(ents))
    for k, body in ents:
        print("  (%s)  %5d bytes" % (k, len(body)))


if __name__ == "__main__":
    main()
