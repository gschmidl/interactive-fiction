#!/usr/bin/env python3
"""check_data.py - does the port build the same database the author did?

Runs the ported ADVFLS and 100FLS on the ASCII databases in a scratch
directory and compares every file they write with the binary file of the same
name that came off the DECUS tape (archive_original/).

A tape file is whole 512-byte blocks and RSX did not clear the tail of the
last one, so after the last record the program wrote it still holds whatever
that disk block held before.  The comparison therefore covers exactly the
records the port wrote - and insists the port wrote as many blocks as the
author's run did.

Also checks that data/ holds the tape's files unchanged.
"""
import filecmp, os, shutil, subprocess, sys, tempfile

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TAPE = os.path.join(os.path.dirname(PORT), 'archive_original')
RECSIZE = {'ADVTXT': 74, 'KATAB': 56, 'ADVDAT': 2, 'ADVENT': 2}

def records(path, size):
    d = open(path, 'rb').read()
    rpb = 512 // size
    return len(d), [d[b + k * size: b + (k + 1) * size] for b in range(0, len(d), 512) for k in range(rpb)]

def compare(mine, tape, size):
    la, a = records(mine, size)
    lb, b = records(tape, size)
    used = len(a)
    while used and not a[used - 1].strip(b'\0'): used -= 1
    diff = [i + 1 for i, (x, y) in enumerate(zip(a, b)) if x != y]
    real = [i for i in diff if i <= used]
    ok = la == lb and not real
    print('  %-11s %6d bytes  %5d records written, all identical%s' % (
        os.path.basename(mine), la, used,
        '' if not diff else '; tape has %d stale record(s) after them' % len(diff))
        if ok else '  %-11s DIFFERS: sizes %d/%d, first differing records %s' % (os.path.basename(mine), la, lb, real[:8]))
    return ok

def main():
    work = tempfile.mkdtemp(prefix='advpas-data-')
    ok = True
    try:
        for f in ('ADVENTURE.DAT', 'ADVENTURE.100'):
            shutil.copy(os.path.join(PORT, 'data', f), work)
        env = dict(os.environ, ADVPAS_DATA=work, ADVPAS_SAVE=os.path.join(work, 'save'))
        for exe in ('advfls.exe', '100fls.exe'):
            p = subprocess.run([os.path.join(PORT, exe)], env=env, capture_output=True, text=True)
            if p.returncode:
                print(exe, 'failed:', p.stderr); return 1
        print('rebuilt from ADVENTURE.DAT / ADVENTURE.100, against the tape:')
        for name in ('ADVTXT.DTA', 'KATAB.DTA', 'ADVDAT.DTA', 'ADVENT.DTA', 'ADVTXT.100', 'ADVDAT.100'):
            ok &= compare(os.path.join(work, name), os.path.join(TAPE, name.lower()), RECSIZE[name.split('.')[0]])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('data/ against the tape:')
    for name in ('ADVENTURE.DAT', 'ADVTXT.DTA', 'KATAB.DTA', 'ADVDAT.DTA', 'ADVENT.DTA', 'ADVTXT.100', 'ADVDAT.100', 'ADVWIZ.DTA'):
        same = filecmp.cmp(os.path.join(PORT, 'data', name), os.path.join(TAPE, name.lower()), shallow=False)
        print('  %-14s %s' % (name, 'identical' if same else 'DIFFERS'))
        ok &= same
    print('  ADVENTURE.100  rebuilt by tools/mk100.py (the tape\'s copy is ADVENTURE.DAT again)')
    print('PASS' if ok else 'FAIL')
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
