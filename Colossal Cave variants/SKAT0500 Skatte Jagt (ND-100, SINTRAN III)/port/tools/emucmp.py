"""Compare a port run of a command script with a RetroCore capture.

usage: python tools/emucmp.py SCRIPT.txt CAPTURE.raw [port options...]

SCRIPT has one game command per line.  CAPTURE is refcap.py output.  The
port runs with --raw so the bytes are the ND ones.  SINTRAN's own chatter
after the program has exited (the @ prompt and anything typed at it) is not
part of the comparison.
"""
import os
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
EXE = os.path.join(ROOT, 'skattejakt.exe')


def script_bytes(path):
    cmds = [l.rstrip('\r\n') for l in open(path, encoding='latin-1')]
    return b''.join(c.encode('latin-1') + b'\r' for c in cmds if not c.startswith('#'))


def compare(ref, mine):
    """None if they agree, else the offset of the first difference"""
    if ref == mine or mine == ref + b'\r\n':
        return None
    if mine.endswith(b'\r\n') and ref.startswith(mine) and ref[len(mine):len(mine) + 1] == b'@':
        return None
    k = 0
    while k < min(len(ref), len(mine)) and ref[k] == mine[k]:
        k += 1
    return k


def main():
    script, cap = sys.argv[1], sys.argv[2]
    p = subprocess.run([EXE, '--raw'] + sys.argv[3:], input=script_bytes(script), capture_output=True)
    mine = p.stdout
    ref = open(cap, 'rb').read()
    i = ref.find(b'SKAT\r\n')
    if i >= 0:
        ref = ref[i + 6:]
    k = compare(ref, mine)
    if k is None:
        print('IDENTICAL', len(mine), 'bytes')
        return 0
    print('DIFFER at byte', k, 'of', len(ref), '/', len(mine))
    print('ref :', ref[max(0, k - 200):k + 200])
    print('port:', mine[max(0, k - 200):k + 200])
    return 1


if __name__ == '__main__':
    sys.exit(main())
