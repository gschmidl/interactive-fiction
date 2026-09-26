#!/usr/bin/env python3
"""Lower case is taken as upper case.

    python tests\\casefold.py

The program knows its commands and answers in upper case only; the port's
PREAD folds what is typed.  So a session typed in lower case - with a
new character, instructions, commands, a quit and a resurrection - must
print exactly what the same session in upper case prints, and one with
bit 8 set on its letters and NULs mixed in must too.  Each runs in a
scratch copy (the character files go to its saves\\) with the clock held.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)

SESSION = ['abc', 'n', 'y', '', '', 'hw', 'li', 'wh', 'ms', 'hw', 'qt', 'y',
           'y', 'n']


def play(lines):
    tmp = tempfile.mkdtemp(prefix='davescave-')
    try:
        for f in ('davescave.exe', 'textfile.dad'):
            shutil.copy(os.path.join(PORT, f), tmp)
        os.makedirs(os.path.join(tmp, 'saves'))
        r = subprocess.run([os.path.join(tmp, 'davescave.exe'),
                            '--time', '120000'],
                           input=b''.join(l + b'\r\n' for l in lines),
                           capture_output=True, timeout=60, cwd=tmp)
        return r.returncode, r.stdout
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    upper = play([s.upper().encode() for s in SESSION])
    lower = play([s.lower().encode() for s in SESSION])
    high = play([bytes(c | 0x80 if c > 32 else c for c in s.encode()) + b'\0'
                 for s in SESSION])
    ok = True
    for name, got in (('lower case', lower), ('bit 8 and NULs', high)):
        same = got == upper
        ok = ok and same
        print('%-16s %s' % (name, 'same as upper case' if same else 'DIFFERS'))
    took = b'YOU HAVE TAKEN  0 HITS' in upper[1] and b'EXPEDITION ENDED' in upper[1]
    print('%-16s %s' % ('commands taken', 'yes' if took else 'NO'))
    ok = ok and took and upper[0] == 0
    print('casefold: %s' % ('ok' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
