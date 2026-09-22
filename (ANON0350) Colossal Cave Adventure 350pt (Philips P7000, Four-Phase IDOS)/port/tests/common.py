"""Shared helpers for the port's tests: running the port in transcript mode
on a scratch copy of the disc pack."""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'advent.exe' if os.name == 'nt' else 'advent')
REF = os.path.join(HERE, 'reference')


def scratch_pack(tmp, name='advent.pack'):
    """a pack path in TMP; the port makes it from p7000.pack on first use"""
    return os.path.join(tmp, name)


def run(inputs, args=(), pack=None, timeout=120):
    """run the port on the given input lines; returns (stdout, stderr, rc).
    Without PACK, a fresh scratch pack is used and thrown away."""
    data = ''.join(line + '\n' for line in inputs).encode('latin-1')
    tmp = None
    if pack is None:
        tmp = tempfile.mkdtemp(prefix='advp7k-')
        pack = scratch_pack(tmp)
    try:
        p = subprocess.run([EXE, '--pack=' + pack] + list(args), input=data,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    finally:
        if tmp:
            for f in os.listdir(tmp):
                os.remove(os.path.join(tmp, f))
            os.rmdir(tmp)
    return (p.stdout.decode('latin-1').replace('\r\n', '\n'),
            p.stderr.decode('latin-1').replace('\r\n', '\n'), p.returncode)


def reference(name):
    """(input lines, expected output or None) of the recorded session NAME"""
    inputs = open(os.path.join(REF, name + '.in')).read().split('\n')
    if inputs and inputs[-1] == '':
        inputs.pop()
    out = os.path.join(REF, name + '.out')
    if not os.path.exists(out):
        return inputs, None
    with open(out, newline='') as f:
        return inputs, f.read()


if __name__ == '__main__':
    sys.exit('a module for the tests')
