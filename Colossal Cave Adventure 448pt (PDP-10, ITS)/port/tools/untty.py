#!/usr/bin/env python3
r"""Reconstruct exact columns from an ITS console capture.

ITS drives the console as a printing terminal on which LINE FEED moves
down without returning the carriage.  It therefore never re-sends a
line's leading blanks: it backspaces to the column it wants, emits LF,
and starts printing there.  Read as text, such a log loses every leading
blank -- fatal for FT01.DAT, whose records are read with fixed FORTRAN
formats (I7, I8+18A4, 11I7, I7+A5).

Replaying the byte stream through a cursor model recovers the columns
exactly:  printable advances the cursor, BS backs up one, CR homes to
column 0, LF ends the line and *keeps* the column, TAB moves to the next
multiple of 8.
"""
import io, sys

def untty(data):
    lines, buf, col = [], [], 0
    for b in data:
        c = b if isinstance(b, int) else ord(b)
        if c == 0o12:                      # LF: end line, keep column
            lines.append(''.join(buf).rstrip())
            buf = []
            continue
        if c == 0o15:                      # CR: home
            col = 0
            continue
        if c == 0o10:                      # BS
            col = max(0, col - 1)
            continue
        if c == 0o11:                      # TAB
            col = (col // 8 + 1) * 8
            continue
        if c == 0o14:                      # FF: page break, treat as LF
            lines.append(''.join(buf).rstrip())
            buf = []
            continue
        if c < 32 or c > 126:
            continue
        while len(buf) < col:
            buf.append(' ')
        if col < len(buf):
            buf[col] = chr(c)
        else:
            buf.append(chr(c))
        col += 1
    if buf:
        lines.append(''.join(buf).rstrip())
    return lines

if __name__ == '__main__':
    data = io.open(sys.argv[1], 'rb').read()
    out = untty(data)
    io.open(sys.argv[2], 'w', encoding='latin-1', newline='\n').write('\n'.join(out) + '\n')
    print('%s -> %s : %d lines' % (sys.argv[1], sys.argv[2], len(out)))
