"""Shared helpers for the port's tests: running the port, and rendering a
raw console recording the way a terminal shows it."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'adventure.exe' if os.name == 'nt' else 'adventure')
REF = os.path.join(HERE, 'reference')


def run(inputs, args=(), exe=EXE, cwd=None, timeout=120):
    """run the port on the given input lines; returns (stdout text, rc)"""
    data = ''.join(line + '\n' for line in inputs)
    p = subprocess.run([exe] + list(args), input=data.encode('latin-1'),
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       cwd=cwd, timeout=timeout)
    return p.stdout.decode('latin-1').replace('\r\n', '\n'), p.returncode


def render(raw):
    """a terminal's view of a raw byte stream: CR returns to column 1, LF
    starts a new line; trailing blanks are dropped"""
    lines, col = [''], 0
    for ch in raw.decode('latin-1'):
        if ch == '\r':
            col = 0
        elif ch == '\n':
            lines.append('')
            col = 0
        elif ' ' <= ch <= '~':
            ln = lines[-1].ljust(col)
            lines[-1] = ln[:col] + ch + ln[col + 1:]
            col += 1
    return [ln.rstrip() for ln in lines]


def reference_session(name):
    """the recorded reference session NAME: (input lines, rendered output from
    the procedure's first line to the SDT line)"""
    inputs = open(os.path.join(REF, name + '.in')).read().split('\n')
    if inputs and inputs[-1] == '':
        inputs.pop()
    lines = render(open(os.path.join(REF, name + '.raw'), 'rb').read())
    first = next(i for i, ln in enumerate(lines) if ln.startswith('Adventure thru a Cave'))
    last = max(i for i, ln in enumerate(lines) if ln.startswith('NORMAL PROGRAM COMPLETION'))
    return inputs, lines[first:last + 2]


def port_lines(text):
    return [ln.rstrip() for ln in text.split('\n')]


if __name__ == '__main__':
    sys.exit('a module for the tests')
