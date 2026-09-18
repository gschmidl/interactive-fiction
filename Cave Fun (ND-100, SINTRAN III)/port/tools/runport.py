"""Play typed commands into cavefun on a scratch copy of the data.

usage: python tools\runport.py [--exe EXE] [--prog PROG] [--adv FILE] KEYSFILE|-
The keys are one command per line (the adventure's name first); the output
(parity off, CR removed) goes to standard output."""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run(keys, exe=None, prog=None, adv=None, extra=(), name='CAVE-FUN'):
    """the output of the port given keys; the game file adv (default data\'s)
    is put in a scratch directory under the name the keys will type"""
    exe = exe or os.path.join(PORT, 'cavefun.exe')
    scratch = tempfile.mkdtemp(prefix='cavefun-')
    try:
        shutil.copy(adv or os.path.join(PORT, 'data', 'CAVE-FUN-MJ.ADV'),
                    os.path.join(scratch, ('CAVE-FUN-MJ' if name == 'CAVE-FUN' else name) + '.ADV'))
        args = [exe, '--data', scratch, '--raw', '--no-hold', '--prog',
                prog or os.path.join(PORT, 'data', 'ADV-INTER-CB-MJ.PROG')] + list(extra)
        p = subprocess.run(args, input=keys, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    return bytes(b & 0x7f for b in p.stdout).replace(b'\r', b'').decode('latin-1')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('keys')
    ap.add_argument('--exe')
    ap.add_argument('--prog')
    ap.add_argument('--adv')
    a = ap.parse_args()
    src = sys.stdin.read() if a.keys == '-' else open(a.keys).read()
    keys = ''.join(l.rstrip('\n') + '\r' for l in src.split('\n') if not l.startswith('#'))
    sys.stdout.write(run(keys.encode('latin-1'), a.exe, a.prog, a.adv))


if __name__ == '__main__':
    main()
