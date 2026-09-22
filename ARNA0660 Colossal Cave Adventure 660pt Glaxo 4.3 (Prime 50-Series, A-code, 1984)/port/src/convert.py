#!/usr/bin/env python3
"""Turn Mike Arnautov's A-code executive - EXECUTIVE.F77 and its include file
EXECUTIVE.INS.F77, Prime F77, compiled `-Ints -Logs -Big` (see
EXECUTIVE.BUILD.CPL) - into gfortran source.

    convert.py <ADVENTURE4 directory> <output directory>

writes executive.f and executive.ins.  The files are read as PRIMOS wrote them
(characters with the top bit set, DC1 + count for a run of blanks, NULs to pad
a line to a word).  Every edit asserts how often it matches, and the counts
are printed.

What the Prime F77 dialect needs:

  -Ints      INTEGER is two bytes: IMPLICIT INTEGER*2, INTEGER -> INTEGER*2.
             A constant written with six digits (001000, 000001, 000500,
             000010) is a long one on the Prime, which is why the record keys
             (class * 1000 * 1000 + line) come out right; gfortran's
             constants are all four bytes, which gives the same.
  -Logs      LOGICAL is two bytes too: LOGICAL -> LOGICAL*2.  With
             -fno-align-commons the COMMON blocks are laid out halfword for
             halfword as on the Prime, and that matters: the A-code reads
             past the end of arrays - EVAL(1500) at the start is OBJVAL(500),
             which is PLACEBIT(98) - and the port reads what the Prime did.
  PARAMETER  without parentheses.
  $INSERT    -> INCLUDE; SYSCOM>KEYS.F is only for OPENDB (below).
  $n         an alternate return in an argument list -> *n.
  /*         a comment to the end of the line -> !.
  :100000    an octal constant: the top bit of a halfword, -32768 (a
             halfword constant: OR wants both arguments of one kind).
  RS LS RT   Prime's shift intrinsics: the run-time has them.
  RAND       the executive's own INTEGER FUNCTION RAND(IX) would be taken
             for gfortran's intrinsic of that name: renamed KRAND.
  OPENDB     read the four ADVINIT files with SRCH$$/PRWF$$ into COMMON,
             the text in pieces because it is bigger than a segment: the
             whole routine is replaced by the run-time's.
  MOVE       the PMA routine copying a text record into CHARACTER*140 ->
             PMOVE, which unpacks the halfwords into characters.
  IOA$ NLEN$A TSRC$$   -> PIOA PNLEN PTSRC (the run-time's).
  PRWF$$     RESTORE writes nothing to the save file to touch its date:
             dropped.

What the port changes:

  - units 1 (the terminal, both ways) -> 5 and 6;
  - the saved games go to saves\\, and RECL= (in halfwords on the Prime,
    bytes here) is dropped;
  - the terminal is read by the run-time's PREAD: at the end of piped input
    the program stops, where the original kept reading its last line again,
    and --echo shows every line read, as the Prime's terminal did (for
    comparing transcripts);
  - the main program first reads the port's options (POPTS).
"""

import os
import re
import sys


def detext(b):
    out = bytearray()
    i = 0
    while i < len(b):
        c = b[i]
        if c == 0x91 and i + 1 < len(b):        # DC1 + count: blanks
            out += b' ' * b[i + 1]
            i += 2
            continue
        if c == 0:                              # pad to a halfword
            i += 1
            continue
        out.append(c & 0x7F)
        i += 1
    return out.decode('ascii')


class Edits:
    def __init__(self):
        self.counts = []

    def sub(self, name, pattern, repl, text, count, flags=re.M):
        new, n = re.subn(pattern, repl, text, flags=flags)
        if n != count:
            raise SystemExit('%s: %d matches, expected %d' % (name, n, count))
        self.counts.append((name, n))
        return new

    def lit(self, name, old, new, text, count=1):
        n = text.count(old)
        if n != count:
            raise SystemExit('%s: %d matches, expected %d' % (name, n, count))
        self.counts.append((name, n))
        return text.replace(old, new)


def convert_ins(e, ins):
    ins = e.sub('nolist', r'^      (NO)?LIST\n', '', ins, 2)
    ins = e.lit('implicit', 'IMPLICIT INTEGER(A-Y)',
                'IMPLICIT INTEGER*2(A-Y)', ins)
    ins = e.sub('logs', r'^(      )LOGICAL ', r'\1LOGICAL*2 ', ins, 1)
    ins = e.sub('parameter', r'^(\s+PARAMETER)\s+(\S.*?)\s*$', r'\1 (\2)',
                ins, 20)
    return ins


def convert_main(e, src):
    # OPENDB goes: the run-time reads the four files
    src = e.sub('opendb', r'^C\*\n      SUBROUTINE OPENDB\n.*?^      END\n', '',
                src, 1, re.M | re.S)
    src = e.lit('insert', '$INSERT EXECUTIVE.INS.F77',
                "      INCLUDE 'executive.ins'", src, 15)
    src = e.sub('altreturn', r'([(,]\s*)\$(\d+)', r'\1*\2', src, 17)
    src = e.sub('comment', r'\s*/\*', '   !', src, 4)
    src = e.lit('octal', ':100000', '(-32767_2-1_2)', src)
    src = e.lit('intfunc', '      INTEGER FUNCTION ', '      INTEGER*2 FUNCTION ',
                src, 2)
    src = e.sub('logs', r'^(      )LOGICAL ', r'\1LOGICAL*2 ', src, 6)
    src = e.sub('krand', r'\bRAND\b(?=\s*[(=])', 'KRAND', src, 4)
    src = e.lit('svar', '      INTEGER WORD1,WORD2', '      INTEGER*2 WORD1,WORD2',
                src)
    # -Ints holds in the three routines without the include file too:
    # KRAND's IX is a halfword (its callers pass halfwords)
    src = e.sub('ints', r'^(      (?:INTEGER\*2 FUNCTION|SUBROUTINE) \w+(?:\(.*\))?\n)'
                r'(?!      INCLUDE)', r'\1      IMPLICIT INTEGER*2 (I-N)\n', src, 3)
    src = e.lit('units', 'DATA INUNIT, OUTUNIT, DBI, FREEZER, INHAND /1, 1, 11, 12, -1/',
                'DATA INUNIT, OUTUNIT, DBI, FREEZER, INHAND /5, 6, 11, 12, -1/',
                src)
    src = e.sub('ioa', r'\bIOA\$\(', 'PIOA(', src, 4)
    src = e.lit('nlen', 'NLEN$A(', 'PNLEN(', src)
    src = e.lit('tsrc', 'TSRC$$(', 'PTSRC(', src)
    src = e.lit('prwf', '      CALL PRWF$$(2,FREEZER-4,LOC(MAXREC),1,000000,I,J)\n',
                '', src)
    src = e.lit('move', 'CALL MOVE(TEXTBUFFER(POSN+1)', 'CALL PMOVE(TEXTBUFFER(POSN+1)',
                src)
    src = e.lit('savefile', 'FILE=Z$FULLKEY, RECL=MAXREC,',
                "FILE='saves/'//Z$FULLKEY,", src, 2)
    # the terminal's input: the run-time's PREAD reads a line as the
    # format did (A139, A1, A12), ends the program at the end of piped
    # input, and with --echo shows the line as the Prime's terminal did
    src = e.lit('read-line', "READ (INUNIT, '(A139)', END=2) Z$LINE",
                'CALL PREAD(Z$LINE, 139)', src)
    src = e.lit('read-answer', "READ (INUNIT, '(A1)', END=7120, ERR=7120) Z$RESP",
                'CALL PREAD(Z$RESP, 1)', src)
    src = e.lit('read-name', "READ (INUNIT,'(A12)')Z$KEY",
                'CALL PREAD(Z$KEY, 12)', src, 2)
    # WHERE of something that is not an object prints its glitch and
    # returns without setting WHERE: the caller got what the Prime's I/O
    # library left in the A register.  In gfortran the entry points share
    # one result, so it would be the last REF/EVAL - and WAKE HOUSE, whose
    # A-code asks WHERE of a place, then answered differently from the
    # Prime (session 7).  0 answers as the Prime did.
    src = e.lit('where0', "4000  FORMAT (' Glitch! Bad WHERE - key = ',I4,"
                "' on record ',F9.3,\n     + ' after loc ',I4)\n      RETURN\n",
                "4000  FORMAT (' Glitch! Bad WHERE - key = ',I4,"
                "' on record ',F9.3,\n     + ' after loc ',I4)\n"
                "      WHERE = 0\n      RETURN\n", src)
    # EXEC 9: -u makes every restore come after a long wait
    src = e.lit('exec9', '      CALL SETVAL (WORD2,999)\n',
                '      CALL SETVAL (WORD2,999)\n      IF (KUNLIM() .NE. 0) RETURN\n',
                src)
    # the port's options, first thing
    src = e.lit('popts', "      INTEGER*4 KEY\n      WRITE (OUTUNIT,",
                "      INTEGER*4 KEY\n      CALL POPTS\n      WRITE (OUTUNIT,",
                src)
    return src


def main():
    srcdir, outdir = sys.argv[1], sys.argv[2]
    rd = lambda n: detext(open(os.path.join(srcdir, n), 'rb').read())
    ins = rd('EXECUTIVE.INS.F77')
    src = rd('EXECUTIVE.F77')
    e = Edits()
    # three of the author's lines run past column 72; the Prime read only
    # 1-72 (one of them loses a comma of a FORMAT, and gfortran reads it
    # the same way), so only lines made here have to fit
    original = set(ins.split('\n')) | set(src.split('\n'))
    original |= set(l.replace('/*', '!') for l in original)
    ins = convert_ins(e, ins)
    src = convert_main(e, src)
    for name, text in (('executive.ins', ins), ('executive.f', src)):
        for n, l in enumerate(text.split('\n'), 1):
            if len(l) > 72 and l[:1] not in 'C*!' and l not in original \
                    and not re.sub(r'\s*!.*', '', l) in \
                    set(re.sub(r'\s*/\*.*', '', o) for o in original):
                raise SystemExit('%s:%d: past column 72: %r' % (name, n, l))
        with open(os.path.join(outdir, name), 'w', newline='\n') as f:
            f.write(text)
    print('convert: ' + ', '.join('%s %d' % c for c in e.counts))


if __name__ == '__main__':
    main()
