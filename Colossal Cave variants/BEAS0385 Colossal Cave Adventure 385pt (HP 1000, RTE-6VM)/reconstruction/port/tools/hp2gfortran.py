"""Mechanically translate HP 1000 FTN77 fixed-form source to gfortran-legacy.

HP's FORTRAN 77 compiler (FTN7X) accepts a number of extensions that gfortran
does not.  This script rewrites the ones the Adventure sources actually use:

  * `FTN77,L` / `FTN4,L` compiler-control first line   -> comment
  * `$EMA`, `$FILES`, `$ALIAS`, `$TRACE` directives    -> comment
  * `$INCLUDE FOO.INCL`                                -> include 'foo.fi'
  * program-unit "descriptors":  SUBROUTINE X(a),Text <ts>  -> SUBROUTINE X(a)
    (also the continuation form where ",Text <ts>" sits on the next card)
  * HP's partition sizing on PROGRAM NAME(pages,size)  -> PROGRAM NAME
  * octal literals  15501B                             -> decimal
    (gfortran accepts O'...' only inside DATA, and these appear in
    expressions)
  * Hollerith constants  2HAB / 24H...                 -> packed INTEGER*2
    literals, two characters per word in memory order, which is what the
    EQUIVALENCEd CHARACTER variables and A2 edit descriptors around them
    expect on a little-endian host
  * bare INTEGER / LOGICAL type specs                  -> INTEGER*2 / LOGICAL*2
    (HP defaults both to 16 bits, gfortran to 32)
  * INTEGER(4)-specific intrinsics MAX0/MIN0/IABS/FLOAT/IFIX -> the generics
  * the user-written RAND function                     -> RNDM, so it does not
    collide with gfortran's RAND intrinsic
  * the `BUFSIZ=` OPEN specifier                       -> dropped

Everything else is passed through untouched.  A handful of constructs still
need hand editing afterwards; see the port README.
"""
import re
import sys
import os

SQ = chr(39)
DQ = chr(34)
WORD = '[A-Za-z0-9_]'
NOTWORD = '(?<!' + WORD + ')'

DIRECTIVE = re.compile(r'^\s*\$(EMA|FILES|ALIAS|TRACE|CDS|LOCALITY)(?![A-Z])',
                       re.I)
INCLUDE = re.compile(r'^\s*\$INCLUDE\s+(\S+)', re.I)
CTRLLINE = re.compile(r'^\s*FTN[0-9X]*\s*(,.*)?$', re.I)
# HP's end-of-source marker, and its 16-bit IMPLICIT INTEGER default.
ENDMARK = re.compile(r'^\s*END\$\s*$', re.I)
IMPINT = re.compile(r'^(\s*IMPLICIT\s+INTEGER)(\s*\()', re.I)
UNITHEAD = re.compile(
    r'^(\s{6}\s*(?:(?:INTEGER|REAL|LOGICAL|DOUBLE\s+PRECISION|CHARACTER)'
    r'(?:\*\d+)?\s+)?(?:SUBROUTINE|FUNCTION|PROGRAM|BLOCK\s+DATA)\s+\w+)'
    r'(\s*\([^)]*\))?\s*(,.*)?$', re.I)
CONTDESC = re.compile(r'^     [^ 0]\s*,\s*[A-Za-z].*$')
OCTAL = re.compile(r'(?<![\w.' + SQ + DQ + r'])([0-7]+)B(?![\w.])')
BUFSIZ = re.compile(r',\s*BUFSIZ\s*=\s*(?:[^,()]|\([^()]*\))+', re.I)
HOLL = re.compile(r'(?<![0-9A-Za-z_' + SQ + DQ + r'])(\d+)H')
ISPROG = re.compile(r'PROGRAM\s+\w+$', re.I)
DEFINT = re.compile(r'^(?:      |     [^ 0])\s*(INTEGER|LOGICAL)'
                    r'(?=\s+(?:FUNCTION\s+)?[A-Za-z])', re.I)
ISCALL = re.compile(r'^      \s*(?:\d+\s+)?CALL(?![A-Z0-9_])', re.I)
ISCONT = re.compile(r'^     [^ 0]')

GENERIC = [
    (re.compile(NOTWORD + 'MAX0' + r'\s*\(', re.I), 'MAX('),
    (re.compile(NOTWORD + 'MIN0' + r'\s*\(', re.I), 'MIN('),
    (re.compile(NOTWORD + 'IABS' + r'\s*\(', re.I), 'ABS('),
    (re.compile(NOTWORD + 'FLOAT' + r'\s*\(', re.I), 'REAL('),
    (re.compile(NOTWORD + 'IFIX' + r'\s*\(', re.I), 'INT('),
    (re.compile(NOTWORD + 'RAND(?!' + WORD + ')', re.I), 'RNDM'),
]


def pack(chars):
    """Pack a character string into INTEGER*2 literals, memory order."""
    if len(chars) % 2:
        chars += ' '
    out = []
    for i in range(0, len(chars), 2):
        v = ord(chars[i]) | (ord(chars[i + 1]) << 8)
        if v > 32767:
            v -= 65536
        out.append(str(v))
    return ','.join(out)


def holleriths(line):
    """Replace nHxxxx constants, honouring the exact character count.

    Quoted strings are stepped over, so text such as "12H" inside a FORMAT
    or a game message is left alone.
    """
    out = []
    i = 0
    quote = None
    while i < len(line):
        c = line[i]
        if quote:
            out.append(c)
            i += 1
            if c == quote:
                quote = None
            continue
        if c == SQ or c == DQ:
            quote = c
            out.append(c)
            i += 1
            continue
        m = HOLL.match(line, i)
        if m:
            n = int(m.group(1))
            dstart = m.end()
            text = line[dstart:dstart + n]
            if len(text) < n:          # Hollerith ran off the end of the card
                text += ' ' * (n - len(text))
            out.append(pack(text))
            i = dstart + n
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def widths(line):
    """Spell out HP's 16-bit INTEGER/LOGICAL defaults, and degenericise."""
    m = IMPINT.match(line)
    if m:
        line = line[:m.end(1)] + '*2' + line[m.end(1):]
    m = DEFINT.match(line)
    if m:
        line = line[:m.end(1)] + '*2' + line[m.end(1):]
    for rx, rep in GENERIC:
        line = rx.sub(rep, line)
    return line


def convert(lines, incmap):
    res = []
    skip_next_desc = False
    in_call = False
    for raw in lines:
        line = raw.rstrip(chr(13)).rstrip(chr(10))
        if skip_next_desc and CONTDESC.match(line):
            res.append('C     ' + line.strip())
            skip_next_desc = False
            continue
        skip_next_desc = False
        if not line.strip():
            res.append('')
            continue
        if line[0] in 'Cc*!':
            res.append(line)
            continue
        m = INCLUDE.match(line)
        if m:
            res.append('      include ' + SQ + incmap(m.group(1)) + SQ)
            continue
        if DIRECTIVE.match(line) or CTRLLINE.match(line) or ENDMARK.match(line):
            res.append('C     ' + line.strip())
            continue
        m = UNITHEAD.match(line)
        if m:
            head = m.group(1)
            args = m.group(2) or ''
            if ISPROG.search(head):
                args = ''              # HP partition size, not arguments
            if m.group(3) is None:
                skip_next_desc = True  # descriptor may sit on the next card
            res.append(widths(head + args))
            continue
        line = BUFSIZ.sub('', line)
        # A Hollerith handed straight to a subroutine has to stay one
        # contiguous object: VOCAB reads 6HKEYS__ as ID(1:3).  gfortran
        # passes Hollerith actuals correctly, so leave those alone and only
        # repack the ones in DATA statements and expressions.
        if not ISCONT.match(line):
            in_call = bool(ISCALL.match(line))
        # Only columns 7 on are statement text.  Column 6 holds the
        # continuation marker, and a marker digit in front of a name is very
        # easy to misread as a Hollerith count -- "     2HINTLC,..." is a
        # continued COMMON list, not a two-character constant.
        head, body = line[:6], line[6:]
        if not in_call:
            body = holleriths(body)
        body = OCTAL.sub(lambda mm: str(int(mm.group(1), 8)), body)
        res.append(widths(head + body))
    return res


def main():
    src, dst = sys.argv[1], sys.argv[2]

    def incmap(f):
        return os.path.splitext(f)[0].lower() + '.fi'

    lines = open(src, encoding='latin1').read().split(chr(10))
    out = convert(lines, incmap)
    open(dst, 'w', encoding='latin1', newline=chr(10)).write(
        chr(10).join(out) + chr(10))


if __name__ == '__main__':
    main()
