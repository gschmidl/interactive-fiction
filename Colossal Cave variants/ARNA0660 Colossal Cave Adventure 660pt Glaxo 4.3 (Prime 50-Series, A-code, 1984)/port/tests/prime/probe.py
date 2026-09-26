"""Send a few lines to PRIMOS and dump everything that comes back (exploration helper).

usage: probe.py [-w SECONDS] "line1" "line2" ...
   a line of the form  @N  waits N seconds instead of sending anything
"""
import sys, time
from primesh import Prime

wait = 6.0
args = sys.argv[1:]
if args and args[0] == "-w":
    wait = float(args[1]); args = args[2:]

p = Prime(log="probe.log")
p.login()
time.sleep(0.5)
p.buf = ""
for a in args:
    if a.startswith("@"):
        time.sleep(float(a[1:]))
    else:
        print("\n>>> " + a)
        p.line(a)
    t0 = time.time()
    while time.time() - t0 < wait:
        p.read()
    sys.stdout.write(p.buf)
    p.buf = ""
p.close()
