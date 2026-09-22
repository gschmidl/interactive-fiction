"""Diff a PRIMOS transcript of DUNGEON.SEG against the C port fed the same lines.

usage: cmp.py cmdfile primos_transcript
"""
import difflib, subprocess, sys, re

EXE = r"D:\SynologyDrive\_\RECONSTRUCTIONS\Dungeon by Jonathan Reed (Prime 50-Series, PL1G, full source)\port\dungeon.exe"

cmds = open(sys.argv[1], "rb").read()
prim = open(sys.argv[2], encoding="latin-1").read()

# the PRIMOS capture starts with the SEG command and ends at the PRIMOS prompt
prim = re.sub(r"^SEG DUNGEON\n", "", prim)
prim = re.sub(r"\nOK, *$", "\n", prim)

port = subprocess.run([EXE], input=cmds, capture_output=True).stdout.decode("latin-1")
port = port.replace("\r\n", "\n")

def norm(t):
    return [l.rstrip() for l in t.strip("\n").split("\n")]

a, b = norm(prim), norm(port)
if a == b:
    print("IDENTICAL (%d lines)" % len(a))
else:
    print("differs: %d PRIMOS lines, %d port lines" % (len(a), len(b)))
    for line in difflib.unified_diff(a, b, "primos", "port", lineterm="", n=1):
        print(line)
