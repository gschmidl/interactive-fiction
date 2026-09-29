"""Prompt-synchronised play of DUNGEON.SEG: never send a line before the game has asked.

usage: syncplay.py cmdfile out.txt
Each command is sent only after one of the game's prompts appears; a line
"@N" in the command file waits N seconds and reports whether anything arrived
(used to tell "waiting for input" apart from "answered").
"""
import re, sys, time
from primesh import Prime

PROMPT = r"(Command\?|instructions\?|wierd positive number\.|Short or long version\?|" \
         r"What direction\?|Play again\?|With what\?|try again:|Invalid entry, try again:|" \
         r"wish to remember\?|At what coordinates\?|first  :|second :|third  :|" \
         r"How many gold pieces do you wish to hide\?|number wand will you use\?|" \
         r"number potion will you drink\?|wish to forget\?|to continue \*\*\*|to begin \*\*\*) ?"

cmds = [l.rstrip("\n") for l in open(sys.argv[1], encoding="latin-1")]
out = sys.argv[2]

p = Prime(log=out + ".raw")
p.login()
p.buf = ""
p.line("A *>SEG_GAMES")
p.expect(r"\nOK, |\nER! ", 30)
p.line("SEG DUNGEON")
text = []
for c in cmds:
    if c.startswith("@"):
        secs = float(c[1:])
        before = len(p.buf)
        t0 = time.time()
        while time.time() - t0 < secs:
            p.read()
        got = p.buf[before:]
        print("[waited %gs, %d new bytes] %r" % (secs, len(got), got[-200:]))
        continue
    try:
        text.append(p.expect(PROMPT, 30))
    except TimeoutError as e:
        print("[no prompt before sending %r] tail=%r" % (c, p.buf[-200:]))
        text.append(p.buf); p.buf = ""
    p.line(c)
t0 = time.time()
while time.time() - t0 < 6:
    p.read()
text.append(p.buf)
p.close()
open(out, "w", encoding="latin-1", newline="").write("".join(text))
print("".join(text)[-800:])
