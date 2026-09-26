#!/usr/bin/env python3
"""Play the same script twice -- once on the reference SIMH PDP-8 booting the
untouched distribution pack, once on the native port -- and diff the two
transcripts.

The reference console is a KSR-33 as far as SIMH is concerned, so it folds
everything to upper case on the way out; the port prints what the program
actually emits, which is properly capitalised English.  The comparison folds
case for that reason and for no other: every other difference is a real one.

Randomness is not reproducible between the two.  Adventure seeds its generator
from a counter that FRTS spins while it waits for the player to type, so the
seed follows the timing of the session -- it differed between two runs on the
real machine as well.  Keep the scripts clear of dwarves and the transcripts
stay comparable.

usage: python tests/regress.py [script.txt ...]
"""
import os
import shutil
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, '..', 'adventure.exe')
ORIG = os.path.join(HERE, '..', '..', 'src_original')
SIMH = os.path.join(ORIG, 'pdp8.exe')
PACK = os.path.join(ORIG, 'advent-work.rk05')

INI = """set console telnet=%d
set console telnet=nobuffer
set cpu 32K
set cpu idle
att rk0 %s
expect "." send "R FRTS\\rADVENT\\e"; continue
boot rk0
"""


def free_port():
    """SIMH wants a fixed console port and refuses to start when it is taken,
    including by a previous run still in TIME_WAIT.  Ask the OS for one."""
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    n = s.getsockname()[1]
    s.close()
    return n


def strip_telnet(b):
    out, i = bytearray(), 0
    while i < len(b):
        if b[i] == 255:
            if i + 1 < len(b) and b[i + 1] == 255:
                out.append(255)
                i += 2
                continue
            i += 3 if (i + 1 < len(b) and b[i + 1] in (251, 252, 253, 254)) else 2
            continue
        out.append(b[i])
        i += 1
    return bytes(out)


def run_reference(cmds, workdir):
    # SIMH's ATTACH cannot cope with the spaces and parentheses in the project
    # path, so give it a copy of the pack beside the .ini and a bare filename.
    local = os.path.join(workdir, 'advent-work.rk05')
    if not os.path.exists(local) or os.path.getmtime(local) < os.path.getmtime(PACK):
        shutil.copyfile(PACK, local)
    port = free_port()
    with open(os.path.join(workdir, 'ref.ini'), 'w', newline='\n') as f:
        f.write(INI % (port, 'advent-work.rk05'))

    p = subprocess.Popen([os.path.abspath(SIMH), 'ref.ini'], cwd=workdir,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        s = None
        for _ in range(40):
            try:
                s = socket.create_connection(('127.0.0.1', port), timeout=5)
                break
            except OSError:
                time.sleep(0.5)
        if s is None:
            raise RuntimeError('could not reach the reference simulator')
        s.settimeout(0.4)
        buf = bytearray()

        def pump(idle, maxwait):
            t0 = last = time.time()
            while time.time() - t0 < maxwait:
                try:
                    d = s.recv(4096)
                    if not d:
                        return
                    buf.extend(d)
                    last = time.time()
                except socket.timeout:
                    if time.time() - last > idle:
                        return

        pump(3.0, 180)                      # boot, load, first prompt
        for c in cmds:
            s.sendall((c + '\r').encode())
            pump(1.5, 120)
        return strip_telnet(bytes(buf)).decode('latin1')
    finally:
        p.kill()
        p.wait()


def run_port(cmds, workdir):
    r = subprocess.run([os.path.abspath(GAME), '-n'], cwd=workdir,
                       input=('\n'.join(cmds) + '\n').encode(),
                       stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                       timeout=600)
    return r.stdout.decode('latin1')


def normalise(text):
    text = text.replace('\r\n', '\n').replace('\r', '\n').upper()
    i = text.find('WELCOME TO ADVENTURE')
    if i >= 0:
        text = text[i:]
    lines = [l.rstrip() for l in text.split('\n')]
    while lines and lines[-1] in ('', '.'):
        lines.pop()
    return lines


def compare(name, cmds, workdir):
    ref = normalise(run_reference(cmds, workdir))
    got = normalise(run_port(cmds, workdir))
    if ref == got:
        print('PASS  %-18s %2d commands, %3d lines' % (name, len(cmds), len(ref)))
        return True
    print('FAIL  %s' % name)
    for i in range(max(len(ref), len(got))):
        a = ref[i] if i < len(ref) else '<end of transcript>'
        b = got[i] if i < len(got) else '<end of transcript>'
        if a != b:
            print('  first difference at line %d' % (i + 1))
            print('    reference: %r' % a)
            print('    port:      %r' % b)
            break
    with open(os.path.join(workdir, name + '.ref'), 'w') as f:
        f.write('\n'.join(ref))
    with open(os.path.join(workdir, name + '.port'), 'w') as f:
        f.write('\n'.join(got))
    return False


def main():
    scripts = sys.argv[1:] or [os.path.join(HERE, f)
                               for f in sorted(os.listdir(HERE))
                               if f.endswith('.txt')]
    workdir = os.path.join(HERE, 'work')
    os.makedirs(workdir, exist_ok=True)
    ok = True
    for sc in scripts:
        with open(sc) as f:
            cmds = [l.rstrip('\n') for l in f if not l.startswith('#')]
        ok &= compare(os.path.basename(sc), cmds, workdir)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
