#!/usr/bin/env python3
"""Pull a program's printed output out of a Hercules 1403 printer file.

Hercules writes the printer with CRLF line ends (TK5CRLF=CRLF), a lone CR
where the carriage control said to return without advancing, and a form feed
at a page break.  The program's own output is what lies between the two
markers given; everything either side is the JES2 job log and the separator
banners.  Lines come back with the carriage-control column already consumed
by the printer, so they are compared against the port's output with its one
leading blank removed.
"""
import sys


def lines(path):
    raw = open(path, 'rb').read().decode('latin-1')
    raw = raw.replace('\r\n', '\n').replace('\r', '').replace('\x0c', '\n')
    return [l.rstrip() for l in raw.split('\n')]


def extract(path, first, last):
    out = lines(path)
    try:
        i = next(k for k, l in enumerate(out) if l == first)
        j = next(k for k, l in enumerate(out) if l == last and k > i)
    except StopIteration:
        sys.exit('prtnorm: %r / %r not found in %s' % (first, last, path))
    return out[i:j + 1]


def port(path):
    raw = open(path, 'rb').read().decode('latin-1')
    raw = raw.replace('\r\n', '\n')
    return [(l[1:] if l[:1] == ' ' else l).rstrip() for l in raw.split('\n')]


if __name__ == '__main__':
    print('\n'.join(extract(sys.argv[1], sys.argv[2], sys.argv[3])))
