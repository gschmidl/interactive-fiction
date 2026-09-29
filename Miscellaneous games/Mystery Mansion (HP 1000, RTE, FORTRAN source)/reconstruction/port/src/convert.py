#!/usr/bin/env python3
"""Build gfortran source for Mystery Mansion (HP 1000 RTE, FTN4) from the tape.

Reads the source &MMM from the INTEREX CSL/1000 startup tape
(archive_original/CSL-1000_Startup-Tape.zip - see hptape.py) and writes
.build/src/mmmroot.f (the main program MMM, the BLOCK DATA, MMRI, MMRL)
and .build/src/mmmseg.f (the segments).  The program is Bill Wolpert's
RTE version of Mystery Mansion, 23 July 81: a main program, a BLOCK DATA,
two subroutines and twelve overlay segments MMSA-MMSL that call one
another through EXEC 8.  The edits are here and in convert2.py.

What gfortran needs done to FTN4 (every edit asserts how often it matches):

  * columns 73-80 are the card's sequence field (22 lines have a comma or a
    period there that FTN4 never saw); gfortran's 72 columns agree;
  * HP INTEGER is 16 bits: IMPLICIT INTEGER*2 (I-N) in every unit, and
    INTEGER declarations become INTEGER*2;
  * a word holds two characters.  Hollerith constants are packed first
    character first in memory, which is what gfortran's A2 editing of an
    INTEGER*2 reads and writes on this machine; the few places that take a
    word apart with /256 and *256 - HP order, first character in the high
    byte - call CFIRST/CSECND/CJOIN/CONE instead (pmmm.f);
  * the BLOCK DATA initialises its word tables through COMPLEX arrays
    EQUIVALENCEd to them (a COMPLEX holds 8H); those DATA statements set
    the INTEGER*2 arrays instead, four words per constant;
  * octal constants nB; a repeat count on a literal in a FORMAT, 3"..."
    (written 3("...")); IF (e) n1,n2 (a two-way arithmetic IF); two
    literals meeting as "" where their comma stood in column 73;
  * the segments (PROGRAM MMSA(5) ...) become subroutines, and EXEC 8 -
    load that segment and run it, never to return - is CALL PSEG, which
    jumps back to the dispatcher in pmmmc.c with the segment's RMPAR
    parameters; EXEC 11 (the time) is CALL PTIME;
  * the FMP calls (OPEN, CREAT, READF, WRITF, POSNT, CLOSE) are the
    port's, with their optional arguments made explicit;
  * WRITE (LU, f) becomes WRITE (PU(LU), f): LU 1 is the console - whose
    output goes through a filter in pmmmc.c that does what RTE's terminal
    driver and an HP 264x terminal did (a record ending in _ leaves the
    cursor where it is; the escape sequences become ANSI ones) - LU 0 the
    bit bucket, any other LU a file in saves (pmmm.f PU);
  * every READ is one call that reads a line and takes it apart as its
    FORMAT did (convert2.py READS, pmmm.f PRDA ...).
"""

import io
import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..', '..'))
ZIP = os.path.join(GAME, 'archive_original', 'CSL-1000_Startup-Tape.zip')
OUT = os.path.join(HERE, '..', '.build', 'src')

sys.path.insert(0, HERE)
import hptape  # noqa: E402

SQ, DQ = chr(39), chr(34)


def source():
    """&MMM (file @00502 of the TF tape) as lines, columns 1-72."""
    z = zipfile.ZipFile(ZIP)
    name = [n for n in z.namelist() if n.endswith('.tf.tape')][0]
    tmp = os.path.join(OUT, '_tape.tf')
    with open(tmp, 'wb') as f:
        f.write(z.read(name))
    files = hptape.tf_files(tmp)
    os.remove(tmp)
    data = [d for n, h, d in files if n.endswith('/@00502')]
    assert len(data) == 1
    recs, _ = hptape.fmp_records(data[0])
    assert len(recs) == 9920
    out = []
    for r in recs:
        t = r.decode('latin-1').rstrip()
        if t[:1] not in ('C', '*'):
            t = t[:72].rstrip()
        out.append(t)
    return out


# ----------------------------------------------------------------------
# edits on the source as it is on the tape (whole lines, counted)
# ----------------------------------------------------------------------

EDITS = [
    ('FTN4,L', ['C     FTN4,L'], 1),
    # HP let a BLOCK DATA share its name with its COMMON block
    ('      BLOCK DATA MMBC', ['      BLOCK DATA MMBCBD'], 1),
    ('      END$', ['C     END$ - the end of the compiler\'s input'], 1),
    ('      PROGRAM MMM(3,100),23 JULY 81',
     ['      SUBROUTINE MMM', 'C     PROGRAM MMM(3,100),23 JULY 81'], 1),
    # the two-way arithmetic IF: IFBRK is -1 after a break, else 0
    ('      IF(IFBRK(IDMY)) 22,25',
     ['      IF(IFBRK(IDMY) .LT. 0) GOTO 22', '      GOTO 25'], 1),
    # words taken apart and put together in HP order
    ('      MSG(J)=(MSGR((J+1)/2)/256)*256+32',
     ['      MSG(J)=CONE(CFIRST(MSGR((J+1)/2)))'], 1),
    ('      MSG(J+1)=(MSGR((J+1)/2)-MSG(J)+32)*256+32',
     ['      MSG(J+1)=CONE(CSECND(MSGR((J+1)/2)))'], 1),
    ('   90 IWRD(I,J)=IWRD(2*I-1,J)-32+(IWRD(2*I,J)-32)/256',
     ['   90 IWRD(I,J)=CJOIN(IWRD(2*I-1,J),IWRD(2*I,J))'], 1),
    ('      IANS=IANS/256-60B', ['      IANS=CFIRST(IANS)-48'], 1),
    ('      IRMX=(IRM(1)/256-60B)', ['      IRMX=(CFIRST(IRM(1))-48)'], 1),
    ('      IRMY=IRM(1)-256*(IRM(1)/256)-60B',
     ['      IRMY=CSECND(IRM(1))-48'], 1),
    # a continuation card with a '/' in column 1, which FTN4 ignored
    ('/    1(J).NE.IR).OR.IC.EQ.0))IRES(J)=100*(IRES(J)/100)+IRC',
     ['     1(J).NE.IR).OR.IC.EQ.0))IRES(J)=100*(IRES(J)/100)+IRC'], 1),
    # a period after the last literal of a FORMAT: the relocatable on the
    # tape keeps it, so HP's run-time formatter passed over it
    ('     1"PEARLS IN THE OTHER".)', ['     1"PEARLS IN THE OTHER")'], 1),
    # removing articles and adjectives: I=I-1 stands inside the DO 175
    # loops, so it runs six times for every word moved up and the DO 176
    # loop wanders through the memory before IWRD before it comes back to
    # the words; gfortran does not allow a DO variable to be changed.  Here
    # I steps back once, to look at the word that moved into its place.
    ('      DO 176 I=2,8',
     ['      I=1', '17601 I=I+1', '      IF(I.GT.8)GOTO 17602'], 1),
    ('      IF(J.EQ.8.AND.K.GT.4)IWRD(K,J+1)=0\n'
     '      I=I-1\n'
     '  175 CONTINUE\n'
     '  176 CONTINUE',
     ['      IF(J.EQ.8.AND.K.GT.4)IWRD(K,J+1)=0',
      '  175 CONTINUE',
      '      I=I-1',
      '  176 GOTO 17601',
      '17602 CONTINUE'], 1),
    # Fix 1.  After a message MMSE, MMSG, MMSI and MMSK write it again on
    # the LU the game is recorded on (RECORD) - and if that was the
    # player's own terminal, again and again for ever.  MMSD tests for
    # it; here the other four do too (not with --no-fixes).
    (' 2000 IF (LU.NE.IPR(1).OR.ITST(21).EQ.0)GOTO 3000',
     [' 2000 IF (LU.NE.IPR(1).OR.ITST(21).EQ.0)GOTO 3000',
      '      IF(KFIXES().NE.0.AND.ITST(21).EQ.IPR(1))GOTO 3000'], 1),
    ('      IF(LU.NE.IPR(1).OR.ITST(21).EQ.0)GOTO 9999',
     ['      IF(LU.NE.IPR(1).OR.ITST(21).EQ.0)GOTO 9999',
      '      IF(KFIXES().NE.0.AND.ITST(21).EQ.IPR(1))GOTO 9999'], 3),
    ('      IF(IABS(IP).LT.2.AND.IC.NE.16)GOTO 10050',
     ['      IF(ABS(IP).LT.2.AND.IC.NE.16)GOTO 10050'], 1),
]
# the magic words: IVRB(k,n)=IWRD(a,I)-32+(IWRD(b,I)-32)/256
_MAGIC = re.compile(r'^([ 0-9]{5} )(IVRB\(\d,4\*I(?:\+\d)?\))=IWRD\((\d),I\)-32'
                    r'\+\(IWRD\((\d),I\)-32\)/256$')


def edit(lines):
    text = '\n'.join(lines) + '\n'
    for old, new, count in EDITS:
        pat = old + '\n'
        n = sum(1 for m in re.finditer(re.escape(pat), text)
                if m.start() == 0 or text[m.start() - 1] == '\n')
        if n != count:
            raise SystemExit('edit matched %d times, expected %d: %s'
                             % (n, count, old))
        text = re.sub(r'(?m)^' + re.escape(pat), '\n'.join(new) + '\n', text)
    lines = text.rstrip('\n').split('\n')
    n = 0
    for i, l in enumerate(lines):
        m = _MAGIC.match(l)
        if m:
            lines[i] = '%s%s=CJOIN(IWRD(%s,I),IWRD(%s,I))' % (
                m.group(1), m.group(2), m.group(3), m.group(4))
            n += 1
    if n != 12:
        raise SystemExit('magic-word lines: %d, expected 12' % n)
    return lines


# ----------------------------------------------------------------------
# statements
# ----------------------------------------------------------------------

def is_comment(l):
    return l[:1] in ('C', 'c', '*') or l.strip() == ''


def is_cont(l):
    return (not is_comment(l) and len(l) > 5 and l[:5].strip() == ''
            and l[5] not in (' ', '0'))


def statements(lines):
    """[(comment-lines, statement-lines)] in order."""
    out = []
    pend = []
    cur = None
    for l in lines:
        if is_comment(l):
            if cur is not None:
                pend.append(l)
            else:
                pend.append(l)
            continue
        if is_cont(l):
            assert cur is not None, l
            # a comment between the lines of a statement stays with it
            cur[1].extend(pend)
            pend = []
            cur[1].append(l)
            continue
        if cur is not None:
            out.append(cur)
        cur = (pend, [l])
        pend = []
    if cur is not None:
        out.append(cur)
    if pend:
        out.append((pend, []))
    return out


def joined(stmt):
    """label, text of a statement (columns 7-72 of each card)."""
    first = stmt[0]
    label = first[:5].strip()
    text = first[6:]
    for l in stmt[1:]:
        if is_comment(l):
            continue
        text += l[6:].ljust(66) if False else l[6:]
    return label, text


def cards(label, text, indent='      '):
    """A statement as fixed-form cards of at most 72 columns."""
    out = []
    first = True
    while text or first:
        room = 66
        if len(text) <= room:
            piece, text = text, ''
        else:
            # break outside a literal, at a comma or blank if possible
            cut = safe_cut(text, room)
            piece, text = text[:cut], text[cut:]
        if first:
            out.append((label.ljust(5) + ' ' + piece) if label else
                       indent + piece)
            first = False
        else:
            out.append('     &' + piece)
    return out


def safe_cut(text, room):
    """Where to end a card: after a comma, parenthesis, blank, slash or a
    closing quote outside a literal - or, if there is none, at exactly
    column 72, where a literal may go on to the next card (gfortran pads a
    card to 72 columns, so a literal must fill it)."""
    q = None
    best = None
    for i, c in enumerate(text[:room]):
        if q:
            if c == q:
                q = None
                best = i + 1
            continue
        if c in (SQ, DQ):
            q = c
            continue
        if c in ',( /':
            best = i + 1
    return best or room


# ----------------------------------------------------------------------
# Hollerith constants, octal, integer widths
# ----------------------------------------------------------------------

def pack(chars):
    """Characters -> INTEGER*2 values, first character first in memory."""
    if len(chars) % 2:
        chars += ' '
    out = []
    for i in range(0, len(chars), 2):
        v = ord(chars[i]) | (ord(chars[i + 1]) << 8)
        if v > 32767:
            v -= 65536
        out.append(str(v))
    return out


HOLL = re.compile(r'(?<![0-9A-Za-z_' + SQ + DQ + r'])(\d+)H')
OCTAL = re.compile(r'(?<![\w.' + SQ + DQ + r'])([0-7]+)B(?![\w.])')


def holleriths(text, as_list=False):
    """nHxxxx -> packed words (outside literals).  Returns text, count."""
    out = []
    i = 0
    q = None
    n = 0
    while i < len(text):
        c = text[i]
        if q:
            out.append(c)
            i += 1
            if c == q:
                q = None
            continue
        if c in (SQ, DQ):
            q = c
            out.append(c)
            i += 1
            continue
        m = HOLL.match(text, i)
        if m:
            k = int(m.group(1))
            s = text[m.end():m.end() + k].ljust(k)
            out.append(','.join(pack(s)))
            i = m.end() + k
            n += 1
            continue
        out.append(c)
        i += 1
    return ''.join(out), n


def code_only(text, fn):
    """Apply fn to the parts of text outside literals."""
    out = []
    q = None
    buf = ''
    for c in text:
        if q:
            buf += c
            if c == q:
                out.append(buf)
                buf = ''
                q = None
            continue
        if c in (SQ, DQ):
            out.append(fn(buf))
            buf = c
            q = c
            continue
        buf += c
    out.append(buf if q else fn(buf))
    return ''.join(out)


if __name__ == '__main__':
    import convert2
    convert2.main()
