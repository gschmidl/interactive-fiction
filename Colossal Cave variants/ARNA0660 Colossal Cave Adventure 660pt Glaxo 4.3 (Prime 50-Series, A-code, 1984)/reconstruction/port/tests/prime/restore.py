"""Restore the SBD003 tape (mt0) into <SYS23K>GUEST23 with MAGRST, answering its prompts by rule."""
import re, sys, time
from primesh import Prime

RULES = [
    (r"tape unit:\s*$", "0"),
    (r"logical tape (number|no)[^\n]*$", sys.argv[1] if len(sys.argv) > 1 else "1"),
    (r"ready to restore[^\n]*$", "YES"),
    (r"name of (the )?tree[^\n]*$", ""),
    (r"^\s*restore\?\s*$", "yes"),
    (r"(^|\n)\s*ok, ?$", None),          # back at command level: done
    (r"(^|\n)\s*er! ?$", None),
]

p = Prime(log="restore.log")
p.login()
p.buf = ""
p.line("ASSIGN MT0")
p.expect(r"\nOK, |\nER! ", 30)
p.line("MAGRST")

last = time.time()
shown = 0
while True:
    p.read()
    if len(p.buf) > shown:
        sys.stdout.write(p.buf[shown:])
        sys.stdout.flush()
        shown = len(p.buf)
        last = time.time()
    tail = p.buf[-200:].lower()
    for rx, ans in RULES:
        if re.search(rx, tail, re.I | re.M):
            if ans is None:
                print("\n[at command level - stopping]")
                p.line("LO")
                time.sleep(2)
                p.close()
                sys.exit(0)
            print("\n[answering %r]" % ans)
            p.line(ans)
            p.buf = ""
            shown = 0
            last = time.time()
            break
    if time.time() - last > 90:
        print("\n[no output for 90s - stopping]")
        break
p.line("LO")
time.sleep(2)
p.close()
