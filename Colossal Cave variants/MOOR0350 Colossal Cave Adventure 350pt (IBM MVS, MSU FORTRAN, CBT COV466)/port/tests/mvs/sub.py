#!/usr/bin/env python3
"""Submit a job to TK5's socket card reader and wait for the printer output.

usage: sub.py job.jcl [jobname] [timeout]
The printer files are watched from their current length, so what this prints
is exactly what the job produced.
"""
import os
import socket
import sys
import time

TK5 = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'mvs-tk5')
PRT = [os.path.join(TK5, 'prt', n) for n in ('prt00e.txt', 'prt00f.txt')]
PORT = 3505


def size(p):
    try:
        return os.path.getsize(p)
    except OSError:
        return 0


def tail(p, start):
    with open(p, 'rb') as f:
        f.seek(start)
        return f.read().decode('latin-1')


def main():
    jcl = open(sys.argv[1], 'rb').read().replace(b'\r\n', b'\n')
    jobname = sys.argv[2] if len(sys.argv) > 2 else None
    limit = float(sys.argv[3]) if len(sys.argv) > 3 else 120.0

    marks = [size(p) for p in PRT]
    s = socket.create_connection(('127.0.0.1', PORT), 10)
    s.sendall(jcl if jcl.endswith(b'\n') else jcl + b'\n')
    s.close()

    deadline = time.time() + limit
    quiet = 0
    last = list(marks)
    while time.time() < deadline:
        time.sleep(1.5)
        now = [size(p) for p in PRT]
        if now == last:
            quiet += 1
            if quiet >= 4 and any(n > m for n, m in zip(now, marks)):
                break
        else:
            quiet = 0
            last = now
    out = []
    for p, m in zip(PRT, marks):
        t = tail(p, m)
        if t.strip():
            out.append('======== %s\n%s' % (os.path.basename(p), t))
    text = ''.join(out)
    open(os.path.join(os.path.dirname(os.path.abspath(sys.argv[1])),
                      'last.prt'), 'w', encoding='latin-1',
         newline='\n').write(text)
    sys.stdout.buffer.write(text.encode('latin-1'))
    if jobname and ('$HASP' not in text and 'IEF142I' not in text):
        sys.stderr.write('\n(no job summary seen - still running?)\n')


if __name__ == '__main__':
    main()
