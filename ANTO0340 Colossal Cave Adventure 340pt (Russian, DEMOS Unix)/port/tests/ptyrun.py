#!/usr/bin/env python3
"""Drive a program through a pty, one script line per prompt-settle, and
write the raw output bytes to a file.  The 1985 game reads stdin with a
bare read(0,buf,79): on a tty that returns one line, on a pipe it returns
whatever is buffered, so only a real terminal reproduces its behaviour."""
import os, pty, sys, select, termios, tty, time

prog, script, out = sys.argv[1], sys.argv[2], sys.argv[3]
cwd = sys.argv[4] if len(sys.argv) > 4 else "."
lines = open(script, "rb").read().split(b"\n")

pid, fd = pty.fork()
if pid == 0:
    os.chdir(cwd)
    os.execv(prog, [prog])

# no echo, no output post-processing: we want exactly what the program wrote
a = termios.tcgetattr(fd)
a[3] &= ~termios.ECHO
a[1] &= ~termios.OPOST
termios.tcsetattr(fd, termios.TCSANOW, a)

buf = b""
def drain(idle=0.35, limit=6.0):
    global buf
    end = time.time() + limit
    last = time.time()
    while time.time() < end and time.time() - last < idle:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                return False
            if not d:
                return False
            buf += d
            last = time.time()
    return True

alive = drain()
for ln in lines:
    if not alive:
        break
    try:
        os.write(fd, ln + b"\n")
    except OSError:
        break
    alive = drain()

os.close(fd)
try:
    os.waitpid(pid, 0)
except ChildProcessError:
    pass
open(out, "wb").write(buf)
print("wrote %d bytes to %s" % (len(buf), out))
