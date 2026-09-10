#!/usr/bin/env python3
"""Play the test scripts and compare with the transcripts from the pack.

The .real files were captured by playing the same commands on the
original TOPS-20 system running under a DECsystem-10 simulator.  The
comparison folds case, because that machine's terminal echoed what was
typed in upper case and this one does not.
"""
import difflib
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(os.path.dirname(HERE), "bin", "adv751.exe")


def norm(text):
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = [l.rstrip() for l in text.split("\n")]
    while lines and lines[0].startswith("R GAME"):
        lines.pop(0)
    while lines and not lines[-1]:
        lines.pop()
    return [l.upper() for l in lines]


def main():
    scripts = sorted(f for f in os.listdir(HERE) if f.endswith(".in"))
    failed = 0
    # GRIPE and SUSPEND write files where the game is run, so run it
    # somewhere it can litter freely.
    scratch = tempfile.mkdtemp(prefix="adv751-")
    for name in scripts:
        stem = name[:-3]
        real = os.path.join(HERE, stem + ".real")
        if not os.path.exists(real):
            continue
        with open(os.path.join(HERE, name), "rb") as fp:
            script = fp.read()
        run = subprocess.run([EXE, "--player", "OPERATOR", "--no-delays"],
                             input=script, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, timeout=600,
                             cwd=scratch)
        mine = norm(run.stdout.decode("latin1"))
        theirs = norm(open(real, "rb").read().decode("latin1"))
        if mine == theirs:
            print("%-14s ok   (%d lines)" % (stem, len(theirs)))
        else:
            failed += 1
            print("%-14s DIFFERS" % stem)
            for line in list(difflib.unified_diff(theirs, mine, "pack", "port",
                                                  n=2, lineterm=""))[:40]:
                print("   " + line)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
