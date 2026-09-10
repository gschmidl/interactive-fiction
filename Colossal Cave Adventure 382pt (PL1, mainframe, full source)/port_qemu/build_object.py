#!/usr/bin/env python3
"""Build the fixed 80-byte-record OBJECT database file from decoded_database.txt.

decoded_database.txt is produced by decode_database.py, one line per card with
trailing blanks stripped.  Every line is padded back out to exactly 80 columns
here; the engine reads OBJECT with ENVIRONMENT(F RECSIZE(80)) and parses
section text as (F(8),14 A(5),A(2)), calling BUG(0) if columns 79-80 of a text
card are not blank -- so the file has to be latin-1 (one byte per character)
and no line may run past column 80.
"""
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'decoded_database.txt')
dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(here, 'OBJECT')

text = open(src, encoding='latin-1', newline='\n').read()
lines = text.split('\n')
if lines and lines[-1] == '':
    lines.pop()                      # trailing newline, not a record

out = bytearray()
for n, line in enumerate(lines, 1):
    line = line.rstrip('\r')
    if len(line) > 80:
        sys.exit(f'{src}:{n}: record is {len(line)} columns, limit is 80')
    out += line.ljust(80).encode('latin-1')

with open(dst, 'wb') as f:
    f.write(out)
print(f'{dst}: {len(lines)} records, {len(out)} bytes')
