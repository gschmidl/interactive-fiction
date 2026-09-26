#!/usr/bin/env python3
"""Turn DAVESCAVE.FOR into source gfortran will take, and TEXTFILE.DAD into
the file the port reads.

The source is DEC FORTRAN IV-PLUS, written with DEC's tab format and compiled
with 2-byte default integers: the VAX-11 FORTRAN IV-PLUS listing beside it
(davescave.ftl, 9-Sep-1980) is of this same source line for line, and its
storage map gives every default integer as I*2.  So the edits are:

  * tab format expanded to fixed columns (a tab in the label field starts the
    statement in column 7, or makes a following digit the continuation mark);
  * IMPLICIT INTEGER*2 (I-N) in every unit, which is what /NOI4 meant;
  * RAN(I1,I2) and SECNDS come from src/port/pdave.f - the first is FOR$IRAN
    taken instruction by instruction from the VMS run-time library;
  * the three OPENs whose NAME= is a byte array get a CHARACTER name built
    from it;
  * the main program's DATA for BNAME/CNAME/DNAME goes: START initialises the
    same common block too, and on VMS the later module's DATA is the one that
    stuck - it is START's that puts the initials in place of the extension.

Every edit is a whole line and asserts how often it matches.
"""

import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original')
OUT = os.path.join(HERE, '..', '.build')

EDITS = [
    ('      PROGRAM DAVESCAVE', ['      SUBROUTINE DAVESC'], 1),
    #  the main program's DATA for the common file names; START's wins
    ("      DATA BNAME/'R','O','O','M','F','I','L','E',",
     ['C     DATA BNAME/ROOMFILE.DAD/ - START initialises /DANDD3/ as well,',
      'C     and on VMS the later module\'s DATA was the one that stuck'], 1),
    ("      DATA CNAME/'C','H','A','R','F','I','L','E',",
     ['C     DATA CNAME/CHARFILE.DAD/'], 1),
    ("      DATA DNAME/'D','A','T','A','F','I','L','E',",
     ['C     DATA DNAME/DATAFILE.DAD/'], 1),
    ("     X  '.','D','A','D',5*0/",
     ["C    X  '.','D','A','D',5*0/"], 3),
    #  /DANDD3/ is initialised in two routines - TNAME in the main program,
    #  the three file names in START.  The VAX linker applied both to the
    #  one overlaid psect; gfortran keeps a single routine's copy of a
    #  common block, so both move to the BLOCK DATA written below.
    ("      DATA TNAME/'TEXT','FILE','.DAD',7*0/",
     ["C     DATA TNAME/'TEXT','FILE','.DAD',7*0/ - see BLOCK DATA DAVEBD"], 1),
    ("      DATA BNAME/'D','B','3',':','R','O','O','M','F','I','L',",
     ["C     DATA BNAME/'DB3:ROOMFILE.DAD'/ - see BLOCK DATA DAVEBD"], 1),
    ("      DATA CNAME/'D','B','3',':','C','H','A','R','F','I','L',",
     ["C     DATA CNAME/'DB3:CHARFILE.DAD'/"], 1),
    ("      DATA DNAME/'D','B','3',':','D','A','T','A','F','I','L',",
     ["C     DATA DNAME/'DB3:DATAFILE.DAD'/"], 1),
    ("     X  'E','.','D','A','D',0/",
     ["C    X  'E','.','D','A','D',0/"], 3),
    #  A byte compared with a character constant is its code.  BREST is the
    #  program's own typo for BRESP: an undeclared REAL that is never set, so
    #  a lower-case y at that prompt never worked on the VAX.  (Here PREAD
    #  folds it to Y before the program sees it.)
    ("      IF (BRESP.EQ.'Y'.OR.BRESP.EQ.'y')  GOTO 5",
     ["      IF (BRESP.EQ.89.OR.BRESP.EQ.121)  GOTO 5"], 1),
    ("      IF (BRESP.EQ.'Y'.OR. BREST.EQ.'y') CALL RESET(BNAME(14),BMOD)",
     ["      IF (BRESP.EQ.89.OR. BREST.EQ.121.0) CALL RESET(BNAME(14),BMOD)"],
     1),
    #  "7 is DEC's octal constant: three bells
    ('      WRITE (5,907) "7,"7,"7', ['      WRITE (5,907) 7,7,7'], 1),
    #  With 2-byte default integers a literal was INTEGER*2 too; where one
    #  is passed to a subroutine it says so here, as does IMONST*256 (the
    #  product of two INTEGER*2s on the VAX).  SFILLD takes three arguments
    #  and is given five; the VAX ignored the last two, and so does this.
    ('300   CALL SCORRS(5)', ['300   CALL SCORRS(5_2)'], 1),
    ('      CALL SADDRM(BINIT,5)', ['      CALL SADDRM(BINIT,5_2)'], 1),
    ('400   CALL SFILLD(BINIT,50,5,I1,I2)',
     ['400   CALL SFILLD(BINIT,50_2,5_2,I1,I2)'], 1),
    ('     X     0)) CALL SRMBTN(IRMCTR,50,INODE,ILINK)',
     ['     X     0)) CALL SRMBTN(IRMCTR,50_2,INODE,ILINK)'], 1),
    ('     X     0)) CALL SRMTCR(IRMCTR,50,INODE,ILINK)',
     ['     X     0)) CALL SRMTCR(IRMCTR,50_2,INODE,ILINK)'], 1),
    ('      CALL SRSTOR(IMONST*256,IDLEV)',
     ['      CALL SRSTOR(IMONST*256_2,IDLEV)'], 1),
]

#  In every OPEN: NAME= is FILE= and TYPE= is STATUS=, and the name is kept
#  in a byte array (TNAME in an INTEGER*4 one), so it becomes a CHARACTER.
NOPEN = 9

HEADER = re.compile(r'^ {6}(?:(?:INTEGER|LOGICAL|REAL)(?:\*\d)? +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION|BLOCK DATA) *([A-Z0-9]*)')


def detab(line):
    """DEC tab format to fixed columns."""
    if line[:1] in ('C', 'c', '*', '!'):
        return line.replace('\t', ' ')
    if '\t' not in line[:6]:
        # ordinary fixed columns (a continuation mark already in column 6);
        # a tab further on is only white space
        return line[:6] + line[6:].replace('\t', ' ')
    head, _, rest = line.partition('\t')
    label = head.strip()
    if rest[:1].isdigit() and rest[:1] != '0' and not label:
        return '     ' + rest[0] + rest[1:].replace('\t', ' ')
    return (label.ljust(5) + ' ' + rest.replace('\t', ' '))[:200]


def textfile():
    """TEXTFILE.DAD is one PDP-11 FORTRAN unformatted record in segments of
    126 bytes: a control word (1 first, 0 middle, 2 last) and 124 data bytes.
    The port reads it as gfortran's own unformatted record."""
    raw = open(os.path.join(ORIG, 'textfile.dad'), 'rb').read()
    data, pos, ctls = bytearray(), 0, []
    while pos < len(raw):
        ctl = struct.unpack('<H', raw[pos:pos + 2])[0]
        ctls.append(ctl)
        take = min(124, 1000 - len(data))
        data += raw[pos + 2:pos + 2 + take]
        pos += 126
        if ctl == 2:
            break
    if ctls[0] != 1 or ctls[-1] != 2 or any(c for c in ctls[1:-1]):
        sys.exit('convert: textfile.dad segments %r' % ctls)
    if len(data) != 1000:
        sys.exit('convert: textfile.dad gave %d bytes, not 1000' % len(data))
    rec = struct.pack('<i', 1000) + bytes(data) + struct.pack('<i', 1000)
    return rec, len(ctls)


def main():
    raw = open(os.path.join(ORIG, 'davescave.for'),
               encoding='latin-1').read().replace('\r\n', '\n')
    lines = [detab(l).rstrip() for l in raw.split('\n')]
    while lines and not lines[-1]:
        lines.pop()
    #  One statement line reaches column 73 once its tab is expanded - the
    #  comma that ends line 662's computed GO TO list - and the compiler
    #  read it (the listing shows the statement with no diagnostic), so the
    #  port is compiled -ffixed-line-length-80.  Nothing else passes 72.
    long = [i for i, l in enumerate(lines, 1)
            if l[:1] not in ('C', 'c', '*', '!') and l[72:].strip()]
    if long != [662]:
        sys.exit('convert: lines past column 72: %r' % long)
    text = '\n'.join(lines) + '\n'

    for old, new, count in EDITS:
        pat = old + '\n'
        got = text.count(pat)
        if got != count:
            sys.exit('convert: %r matched %d times, expected %d'
                     % (old, got, count))
        text = text.replace(pat, '\n'.join(new) + '\n')

    #  On VMS unit 5 was the terminal both ways, and the program writes all
    #  its output there; gfortran's unit 5 is standard input only.  Every
    #  record starts with a blank carriage control character (none is 0, 1
    #  or +), which the port prints, as the collection's other source ports
    #  do.
    text, n = re.subn(r'WRITE *\( *5 *,', 'WRITE (6,', text)
    if n != 125:
        sys.exit('convert: %d WRITEs to unit 5, expected 125' % n)

    #  Terminal input.  The program knows its commands and answers in upper
    #  case only (two capital letters, Y and N), as upper-case terminals
    #  sent them; so every READ from the terminal takes its line from the
    #  port's PREAD - bit 8 masked off, NULs dropped, lower case folded to
    #  upper - and reads its items from that line with its own FORMAT.  At
    #  a terminal the input never ends; piped input does, and PREAD then
    #  stops the game quietly.  (One more READ is in a comment.)
    text, n = re.subn(r'(?m)^      READ *\( *5 *,([0-9]+)\)',
                      r'      CALL PREAD(PLINE)\n      READ (PLINE,\1)', text)
    if n != 15:
        sys.exit('convert: %d terminal READs, expected 15' % n)

    #  BMOD='A' and BMOD=' ': a byte assigned a character is its code.
    text, n = re.subn(r"(?m)^(      BMOD=)'(.)'", lambda m: m.group(1) +
                      str(ord(m.group(2))), text)
    if n != 3:
        sys.exit('convert: %d BMOD assignments, expected 3' % n)

    #  OPEN statements.
    def fixopen(m):
        s = m.group(0)
        s = re.sub(r'NAME=TNAME', 'FILE=PNAMEI(TNAME)', s)
        s = re.sub(r'NAME=([BCD]NAME)', r'FILE=PNAMEB(\1)', s)
        return s.replace('TYPE=', 'STATUS=')
    text, n = re.subn(r'(?m)^[ 0-9]{5} OPEN \(UNIT=\d,NAME=\w+,TYPE=[^\n]*',
                      fixopen, text)
    if n != NOPEN:
        sys.exit('convert: %d OPEN statements, expected %d' % (n, NOPEN))

    #  IAND/IOR/IEOR take arguments of one kind in gfortran; FORTRAN IV-PLUS
    #  mixed INTEGER*2 and INTEGER*4 freely, so both are made default INTEGER.
    #  Values here are 16 bit patterns and small masks, so the low 16 bits of
    #  every result are what they were.
    def wrapargs(text, name):
        out, i, n = [], 0, 0
        key = name + '('
        while True:
            j = text.find(key, i)
            if j < 0 or (j > 0 and (text[j - 1].isalnum() or text[j - 1] == '_')):
                if j < 0:
                    out.append(text[i:])
                    return ''.join(out), n
                out.append(text[i:j + len(key)])
                i = j + len(key)
                continue
            k, depth, args, start = j + len(key), 1, [], j + len(key)
            while depth:
                c = text[k]
                if c == '(':
                    depth += 1
                elif c == ')':
                    depth -= 1
                elif c == ',' and depth == 1:
                    args.append(text[start:k])
                    start = k + 1
                k += 1
            args.append(text[start:k - 1])
            out.append(text[i:j] + key + ','.join('INT(%s)' % a.strip()
                                                  for a in args) + ')')
            i, n = k, n + 1
    total = 0
    for name in ('IAND', 'IOR', 'IEOR'):
        text, n = wrapargs(text, name)
        total += n

    #  Two byte default integers in every unit, and the port's own RAN,
    #  SECNDS and file-name functions, declared after the unit's IMPLICITs.
    lines = text.split('\n')
    out, units, pending = [], 0, False
    decls = ['      EXTERNAL RAN,SECNDS', '      REAL RAN,SECNDS',
             '      CHARACTER*260 PNAMEB,PNAMEI', '      CHARACTER*132 PLINE']
    for line in lines:
        if pending and not line.startswith('      IMPLICIT') and \
                line[:1] not in ('C', 'c', '*', '!'):
            out += decls
            pending = False
        out.append(line)
        if HEADER.match(line):
            units += 1
            out.append('      IMPLICIT INTEGER*2 (I-N)')
            pending = True
    text = '\n'.join(out)
    print('convert: %d IAND/IOR/IEOR calls made one kind' % total)

    #  /DANDD3/ as the VAX linker left it: START's file names (it was
    #  linked after the main program, so where both initialised the same
    #  bytes START's won) and the main program's TNAME.
    text += '\n'.join([
        '      BLOCK DATA DAVEBD',
        '      BYTE BNAME(17),CNAME(17),DNAME(17)',
        '      INTEGER*4 TNAME(10)',
        '      COMMON /DANDD3/BNAME,CNAME,DNAME,TNAME',
        "      DATA BNAME/'D','B','3',':','R','O','O','M','F','I','L',",
        "     X  'E','.','D','A','D',0/",
        "      DATA CNAME/'D','B','3',':','C','H','A','R','F','I','L',",
        "     X  'E','.','D','A','D',0/",
        "      DATA DNAME/'D','B','3',':','D','A','T','A','F','I','L',",
        "     X  'E','.','D','A','D',0/",
        "      DATA TNAME/'TEXT','FILE','.DAD',7*0/",
        '      END', ''])

    os.makedirs(os.path.join(OUT, 'src'), exist_ok=True)
    with open(os.path.join(OUT, 'src', 'davescave.f'), 'w',
              newline='\n') as f:
        f.write(text)
    rec, nseg = textfile()
    with open(os.path.join(OUT, 'textfile.dad'), 'wb') as f:
        f.write(rec)
    print('convert: davescave.f, %d units; textfile.dad from %d segments'
          % (units, nseg))


if __name__ == '__main__':
    main()
