#!/usr/bin/env python3
"""A transcript of a game, the commands where the program read them.

    python tests\\transcript.py [options for abenteuer] < commands.txt

Runs abenteuer.exe in a scratch copy of the port (so that a SICHR or a
wizard's maintenance cannot touch the real NEUSPIEL.DAT or saves\\), with
gfortran's output unbuffered, and sends one command at a time: when the
program has been quiet for a moment it is waiting for input, and the
command is written into the transcript as "> command".  Used by the other
tests; as a program, it prints the transcript.
"""
import os
import queue
import shutil
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
QUIET = 0.25


def scratch(files=('abenteuer.exe', 'ADV.DATA', 'ADV1980.DATA', 'NEUSPIEL.DAT')):
    tmp = tempfile.mkdtemp(prefix='abenteuer-')
    for f in files:
        shutil.copy(os.path.join(PORT, f), tmp)
    return tmp


def run(commands, args=(), where=None, quiet=QUIET):
    """the transcript and the exit code of a game played in WHERE (a
    scratch copy made and removed here if None).  A command may be a
    function of the transcript so far, which returns the command."""
    own = where is None
    if own:
        where = scratch()
    env = dict(os.environ, GFORTRAN_UNBUFFERED_ALL='1')
    p = subprocess.Popen([os.path.join(where, 'abenteuer.exe')] + list(args),
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, env=env)
    q = queue.Queue()

    def reader():
        while True:
            b = p.stdout.read1(4096)
            if not b:
                q.put(None)
                return
            q.put(b)
    threading.Thread(target=reader, daemon=True).start()
    out = bytearray()
    done = False

    def drain():
        nonlocal done
        while not done:
            try:
                b = q.get(timeout=quiet)
            except queue.Empty:
                return
            if b is None:
                done = True
                return
            out.extend(b)
    try:
        for c in commands:
            drain()
            if done:
                break
            if callable(c):
                c = c(out.decode('latin-1'))
            out.extend(b'> ' + c.encode('latin-1') + b'\n')
            try:
                p.stdin.write(c.encode('latin-1') + b'\n')
                p.stdin.flush()
            except OSError:
                break
        if not done:
            p.stdin.close()
            while not done:
                drain()
        code = p.wait(timeout=30)
    finally:
        if p.poll() is None:
            p.kill()
        if own:
            shutil.rmtree(where, ignore_errors=True)
    text = out.decode('latin-1').replace('\r\n', '\n')
    return '\n'.join(l.rstrip() for l in text.split('\n')), code


def main():
    commands = [l.rstrip('\r\n') for l in sys.stdin]
    text, code = run(commands, sys.argv[1:])
    sys.stdout.write(text)
    print('[exit %d]' % code)


if __name__ == '__main__':
    main()
