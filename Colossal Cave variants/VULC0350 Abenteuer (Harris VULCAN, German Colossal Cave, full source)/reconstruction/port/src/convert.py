#!/usr/bin/env python3
"""Turn ABENTEUER's Harris FORTRAN into sources gfortran will take.

    convert.py            (reads ..\\..\\src_original, writes ..\\.build\\src)

The source is the job stream J.ADV off the FAST save (fast.tap): lines 5 to
3538 are the FORTRAN - Gary Palter's portable Adventure as HCSD (Dick
Reynolds, 1977) adapted it to the Harris, with the messages in German - and
the rest is Harris assembler for the site-supplied routines, which the port
supplies itself (src\\port\\pharris.f, pharrisc.c).  Every edit below asserts
how often it matches.

What the Harris FORTRAN needs:

  INTEGER*6  a double word, 47 bits: INTEGER*8.  INTEGER*3, one 24-bit
             word: INTEGER*4.  A constant nD is a double-word one: n_8.
  MAX2 MIN2 MOD2   the double-word intrinsics: MAX, MIN, MOD; IABS: ABS.
  .AND. .OR. .XOR. .SHIFT.   bit operators on integers: IAND, IOR, IEOR,
             ISHFT (only in AND, OR, XOR, CODE1 and DATIME).
  'nnn       an octal constant.
  characters a word held three 8-bit characters, a double word six, first
             character in the high byte; the program keeps them one to a
             word (A1) and compares them with 1H constants.  Here, as in the
             other Palter ports, a character is its code in the low byte of
             its word - which A1 output prints on this little-endian machine
             - so every 1H constant and ' ' literal becomes its code, the
             input lines are unpacked by PUNPK, and CODE1 takes its literal
             as a string.
  carriage control   every output record starts with a blank that the Harris
             printed as carriage control (the 1980 session log shows it):
             FORMATs lose it here - ' TEXT' becomes 'TEXT', 1X goes, 10X
             becomes 9X - so that each record is the line that was printed.
  AND OR XOR RAN SIZE ABORT   gfortran has intrinsics of these names: the
             program's own AND, OR, XOR and RAN are renamed KAND, KOR, KXOR
             and KRAN; ADDR and SIZE (assembler) become the port's PADDR
             and SIZEOF (not LOC: the main program's LOC is a variable);
             ABORT (assembler) becomes PABORT.
  IOINIT LDCOMN SVCOMN   opened files by their Harris names (ATTACH, ASSIGN,
             GENRAT, OPEN n, IO): the port's own (pharris.f).
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original')
OUT = os.path.join(HERE, '..', '.build', 'src')

REPLACED = 'IOINIT LDCOMN SVCOMN'.split()

HEADER = re.compile(r'^ {6,}(?:(?:INTEGER|LOGICAL|REAL)(?:\s*\*\s*\d)? +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION) +([A-Z][A-Z0-9]*)')
COUNTS = []


def check(what, n, want):
    if n != want:
        sys.exit('convert: %s: %d matches, expected %d' % (what, n, want))
    COUNTS.append((what, n))


def source():
    """The FORTRAN part of J.ADV: lines 5 to 3538 (END$ ends the FORTRAN;
    the Harris ended a compilation with it)."""
    raw = open(os.path.join(ORIG, 'J.ADV.txt'), 'rb').read().decode('latin-1')
    lines = raw.split('\n')
    if not lines[3].startswith('$FORTRAN') or lines[3537].strip() != 'END$':
        sys.exit('convert: J.ADV is not laid out as expected')
    out = []
    for line in lines[4:3538]:
        line = line.rstrip()
        if line[:1] not in ('C', 'c', '*'):
            line = line[:72].rstrip()
        out.append(line)
    out[-1] = '      END'
    return out


def units():
    """(name, lines) for every program unit; the first is the main program,
    which has no PROGRAM statement"""
    got, cur, name = [], [], 'MAIN'
    for line in source():
        m = HEADER.match(line)
        if m:
            if any(x.strip() and x[:1] not in 'Cc*' for x in cur):
                sys.exit('convert: %s has no END' % name)
            name = m.group(1)
            cur.append(line)
            continue
        cur.append(line)
        if line.strip() == 'END':
            got.append((name, cur))
            cur = []
    return got


# ------------------------------------------------------------ edits
FIX = {}


def fix(unit, count, old, new):
    FIX.setdefault(unit, []).append((count, old, new))


PORT = 'C  PORT: '


def codes(values, name, cont='     1'):
    """a DATA statement for NAME with integer VALUES, within column 72"""
    out, line = [], '      DATA ' + name + '/'
    for i, v in enumerate(values):
        item = str(v) + ('/' if i == len(values) - 1 else ',')
        if len(line) + len(item) > 71:
            out.append(line)
            line = cont + '     '
        line += item
    out.append(line)
    return '\n'.join(out)


def hollerith_codes(body):
    """the codes of a list of 1Hx constants and 'x' literals"""
    return [ord(a or b) for a, b in re.findall(r"1H(.)|'(.)'", body)]


# MAIN: the site-supplied ADDR and SIZE (assembler) gave the address of a
# variable and the length of a stretch of COMMON; here PADDR and SIZEOF do
# (addr_size, below)
fix('MAIN', 1, "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),"
    "FDUMMY(10)",
    "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),"
    "FDUMMY(10)\n" + PORT + "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAIN', 1, """ 1004 READ(DBFI,1005)LOC,TEXT,KK
 1005 FORMAT(1I8,70A1,A1)""",
    """ 1004 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1005)LOC
 1005 FORMAT(I8)
      CALL PUNPK(PBUF(9:78),TEXT,70)
      KK=ICHAR(PBUF(79:79))""")
fix('MAIN', 1, """ 1043 READ(DBFI,1041)KTAB(TABNDX),(TEXT(I),I=1,5)
 1041 FORMAT(I8,5A1)""",
    """ 1043 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1041)KTAB(TABNDX)
 1041 FORMAT(I8)
      CALL PUNPK(PBUF(9:13),TEXT,5)""")
fix('MAIN', 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")
fix('MAIN', 1, """      KK='E'
      IF(K.EQ.1)KK=' '""",
    PORT + """ONE CHARACTER IN A WORD: ITS CODE (SEE PUNPK).
      KK=69
      IF(K.EQ.1)KK=32""")
fix('MAIN', 1, "      CALL LDCOMN(.TRUE.,FDUMMY,CMADRS,CMSZES)",
    PORT + "COMMAND LINE OPTIONS.\n      CALL POPTS\n"
    "      CALL LDCOMN(.TRUE.,FDUMMY,CMADRS,CMSZES)\n" + PORT +
    "FIX 1: THE SITE'S NEUSPIEL IS A GAME SAVED AT ITS FIRST COMMAND, SO\n"
    "C  ITS TURNS IS 1 AND BRING AND MAGIE MODUS, TAKEN ONLY AT TURN 0,\n"
    "C  NEVER WERE.  TAKE THEM AT THE FIRST COMMAND OF THE GAME.\n"
    "      KTURN0=0\n"
    "      IF(KFIXES().NE.0)KTURN0=TURNS")
fix('MAIN', 1, """      IF(TURNS.EQ.0.AND.WD1.EQ.CODE1('MAGIE').AND.
     1WD2.EQ.CODE1('MODUS'))CALL MAINT(CMADRS,CMSZES)
      IF(TURNS.EQ.0.AND.WD1.EQ.CODE1('BRING'))GOTO 8400""",
    PORT + """FIX 1: TURNS.EQ.KTURN0, NOT TURNS.EQ.0 (SEE KTURN0).
      IF(TURNS.EQ.KTURN0.AND.WD1.EQ.CODE1('MAGIE').AND.
     1WD2.EQ.CODE1('MODUS'))CALL MAINT(CMADRS,CMSZES)
      IF(TURNS.EQ.KTURN0.AND.WD1.EQ.CODE1('BRING'))GOTO 8400""")

for unit in ('SPEAK', 'GETIN', 'A5TOA1'):
    fix(unit, 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")
fix('MOTD', 1, "      DATA BLANK/' '/,PERIOD/'.'/",
    "      DATA BLANK/32/,PERIOD/46/")
fix('CVSTB', 1, "      DATA BLANK,MINUS,PLUS/' ','-','+'/",
    "      DATA BLANK,MINUS,PLUS/32,45,43/")
fix('CVSTB', 1, "      DATA DIGITS/1H0,1H1,1H2,1H3,1H4,1H5,1H6,1H7,1H8,1H9/",
    codes(range(48, 58), 'DIGITS'))
fix('CVLTUC', 1, """      DATA UPPER/1HA,1HB,1HC,1HD,1HE,1HF,1HG,1HH,1HI,1HJ,1HK,1HL,1HM,
     1           1HN,1HO,1HP,1HQ,1HR,1HS,1HT,1HU,1HV,1HW,1HX,1HY,1HZ/,
     2     LOWER/1Ha,1Hb,1Hc,1Hd,1He,1Hf,1Hg,1Hh,1Hi,1Hj,1Hk,1Hl,1Hm,
     3           1Hn,1Ho,1Hp,1Hq,1Hr,1Hs,1Ht,1Hu,1Hv,1Hw,1Hx,1Hy,1Hz/""",
    codes(range(65, 91), 'UPPER') + '\n' + codes(range(97, 123), 'LOWER'))
fix('HOURS', 1, """      T='EN'
      IF(D.EQ.1)T=' '""",
    PORT + """TWO CHARACTERS IN A WORD, THE FIRST IN THE LOW BYTE.
      T=%d
      IF(D.EQ.1)T=%d""" % (ord('E') | ord('N') << 8, 32 | 32 << 8))

# the terminal and the other text reads
fix('GETIN', 1, "      DIMENSION LINE(70),CHARS(5)",
    "      DIMENSION LINE(70),CHARS(5)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('GETIN', 1, """    2 READ(TTYI,3)LINE
    3 FORMAT(70A1)""",
    """    2 CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,LINE,70)""")
fix('MAINT', 1,
    "      DIMENSION HNAME(20),ABB(150),CMADRS(4,11),CMSZES(11),FDUMMY(10)",
    "      DIMENSION HNAME(20),ABB(150),CMADRS(4,11),CMSZES(11),FDUMMY(10)\n"
    + PORT + "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAINT', 1, """      READ(TTYI,2)HNAME
    2 FORMAT(20A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,HNAME,20)""")
fix('MOTD', 1, "      DIMENSION MTDTXT(100),TEXT(70)",
    "      DIMENSION MTDTXT(100),TEXT(70)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MOTD', 1, """   55 READ(TTYI,56)TEXT,K
   56 FORMAT(70A1,A1)""",
    """   55 CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,TEXT,70)
      K=ICHAR(PBUF(71:71))""")

# CODE1: the literal's characters were picked out of 24-bit words with a
# mask and a shift; every call passes a five-character literal
fix('CODE1', 1, """      INTEGER*3 CHRMSK,WORDS,WORD,CHAR,CHRSET
      DIMENSION WORDS(2)
      DIMENSION CHRSET(64)

      DATA NWORDS/2/,NCHARS/3/,CHRSIZ/8/,CHRMSK/'77600000/""",
    PORT + """THE ORIGINAL PICKED THE CHARACTERS OUT OF 24-BIT WORDS, THREE
C  TO A WORD, HIGH BYTE FIRST, WITH A MASK AND A SHIFT.  EVERY CALL PASSES
C  A FIVE CHARACTER LITERAL, SO TAKE IT AS A STRING.
      CHARACTER*(*) WORDS
      CHARACTER*5 W
      DIMENSION CHRSET(64)""")
fix('CODE1', 1, """      RESULT=0
      COUNT=0

      DO 10 I=1,NWORDS
         WORD=WORDS(I)

         DO 5 J=1,NCHARS
            COUNT=COUNT+1
            IF(COUNT.GT.5)GOTO 20
C           CHAR=AND(WORD,CHRMSK)
C           WORD=SHIFT(WORD,CHRSIZ)
      CHAR=WORD.AND.CHRMSK
      WORD=WORD.SHIFT.8
            DO 1 CHRIDX=1,64
C              IF(CHAR.EQ.AND(CHRSET(CHRIDX),CHRMSK))GOTO 2
                IF(CHAR.EQ.(CHRSET(CHRIDX).AND.CHRMSK)) GOTO 2
    1       CONTINUE
            CHRIDX=15
    2       RESULT=SHIFT(RESULT,6D)+CHRIDX-1
    5    CONTINUE
   10 CONTINUE

   20 CODE1=RESULT""",
    """      W=WORDS
      RESULT=0
      DO 10 I=1,5
         CHAR=ICHAR(W(I:I))
         DO 1 CHRIDX=1,64
            IF(CHAR.EQ.CHRSET(CHRIDX))GOTO 2
    1    CONTINUE
         CHRIDX=15
    2    RESULT=SHIFT(RESULT,6D)+CHRIDX-1
   10 CONTINUE
   20 CODE1=RESULT""")

for unit, op in (('AND', 'IAND'), ('OR', 'IOR'), ('XOR', 'IEOR')):
    fix(unit, 1, '      %s=A.%s.B' % (unit, unit),
        PORT + '.%s. ON INTEGERS WAS THE HARRIS\'S BIT OPERATOR.\n'
        '      K%s=%s(A,B)' % (unit, unit, op))

# RAN seeded the Harris library generator IRAN with IRANP; the port has a
# stand-in of each (pharris.f).  Renamed KRAN, it must also set KRAN.
fix('RAN', 1, "      RAN=N", PORT + "THE FUNCTION IS KRAN HERE.\n      KRAN=N")
# DATIME took the date and time apart with the bit operators
fix('DATIME', 1, """      YEAR=J(1).SHIFT.-12
      DAY=J(1).AND.'7777""",
    """      YEAR=ISHFT(J(1),-12)
      DAY=IAND(J(1),O'7777')""")
fix('BUG', 1, "      CALL ABORT", PORT + "ABORT WAS THE HARRIS'S JOB ABORT.\n"
    "      CALL PABORT")
# -u: no prime time, and a restored game need not wait LATNCY minutes
fix('START', 1, "      PTIME=AND(PRIMTM,SHIFT(1D,T/60)).NE.0",
    "      PTIME=AND(PRIMTM,SHIFT(1D,T/60)).NE.0\n" + PORT +
    "-U: NO PRIME TIME.\n      IF(KUNLIM().NE.0)PTIME=.FALSE.")
fix('START', 1, "      DELAY=(D-SAVED)*1440+(T-SAVET)",
    "      DELAY=(D-SAVED)*1440+(T-SAVET)\n" + PORT +
    "-U: NO WAIT.\n      IF(KUNLIM().NE.0)DELAY=LATNCY")

# ADDR and SIZE
ADDR = re.compile(r'^      CALL ADDR\(([^,]+),CMADRS\(1,(\d+)\)\)$', re.M)
SIZE = re.compile(r'^      CMSZES\((\d+)\)=SIZE\((.+?),(.+)\)$', re.M)


def addr_size(text):
    # not LOC: in the main program LOC is the player's location
    text, n = ADDR.subn(r'      CMADRS(1,\2)=PADDR(\1)', text)
    check('addr', n, 11)
    text, n = SIZE.subn(r'      CMSZES(\1)=PADDR(\3)-PADDR(\2)+SIZEOF(\3)',
                        text)
    check('size', n, 11)
    return text


# the main program's statement functions take INTEGER*6 arguments; the
# Harris compiler made a literal argument one (BITSET(LOC,3), DARK(0),
# LIQ(0)), gfortran wants it written n_8
STMT_FNS = 'TOTING HERE AT LIQ2 LIQ LIQLOC BITSET FORCED DARK PCT'.split()
STMT_CALL = re.compile(r'\b(%s)\(([^()]*(?:\([^()]*\)[^()]*)*)\)'
                       % '|'.join(STMT_FNS))


def stmt_fn_literals(text):
    n = 0

    def one(m):
        nonlocal n
        args = m.group(2).split(',')
        new = [a + '_8' if re.fullmatch(r'-?\d+', a) else a for a in args]
        if new == args:
            return m.group(0)
        n += 1
        return '%s(%s)' % (m.group(1), ','.join(new))
    out = []
    for line in text.split('\n'):
        if line[:1] not in ('C', 'c', '*'):
            line = line[:6] + STMT_CALL.sub(one, line[6:])
        out.append(line)
    check('statement function literals', n, 19)
    return '\n'.join(out)


# ------------------------------------------------------------ FORMATs
INPUT_FORMATS = {('MAIN', '1003'), ('MAIN', '1005'), ('MAIN', '1031'),
                 ('MAIN', '1041')}


def fmt_tokens(s):
    toks, i = [], 0
    while i < len(s):
        c = s[i]
        if c == "'":
            j = i + 1
            while True:
                j = s.index("'", j)
                if j + 1 < len(s) and s[j + 1] == "'":
                    j += 2
                    continue
                break
            toks.append(s[i:j + 1])
            i = j + 1
        elif c in '/,':
            toks.append(c)
            i += 1
        elif c == ' ':
            i += 1
        else:
            j = i
            while j < len(s) and s[j] not in "/,'":
                j += 1
            toks.append(s[i:j].replace(' ', ''))
            i = j
    return toks


def drop_cc(body, where):
    """take the carriage-control character off every record of an output
    FORMAT's item list"""
    toks = fmt_tokens(body)
    out = []
    start = True
    for t in toks:
        if t == '/':
            out.append(t)
            start = True
            continue
        if t == ',':
            out.append(t)
            continue
        if start:
            start = False
            if t.startswith("'"):
                if t[1] != ' ':
                    sys.exit('convert: %s: a record starts with %s' % (where, t))
                t = "'" + t[2:]
                if t == "''":
                    if out and out[-1] == ',':
                        out.pop()
                    continue
            elif re.fullmatch(r'\d*X', t):
                n = int(t[:-1] or 1)
                if n == 1:
                    if out and out[-1] == ',':
                        out.pop()
                    continue
                t = '%dX' % (n - 1)
            else:
                sys.exit('convert: %s: a record starts with %s' % (where, t))
        out.append(t)
    # tidy the separators left behind: none at the start or the end, none
    # doubled, none next to a slash
    res = []
    for t in out:
        if t == ',' and (not res or res[-1] in (',', '/')):
            continue
        if t == '/' and res and res[-1] == ',':
            res.pop()
        res.append(t)
    while res and res[-1] == ',':
        res.pop()
    return ''.join(res)


def statements(lines):
    """(first index, last index) of each statement, continuations joined"""
    i = 0
    while i < len(lines):
        if lines[i][:1] in ('C', 'c', '*') or not lines[i].strip():
            i += 1
            continue
        j = i
        while j + 1 < len(lines) and len(lines[j + 1]) > 5 and \
                lines[j + 1][:1] not in ('C', 'c', '*') and \
                lines[j + 1][5] not in (' ', '0'):
            j += 1
        yield i, j
        i = j + 1


def emit(label, text):
    """a FORMAT statement in fixed form, continued within column 72"""
    first = '%-5s FORMAT(%s)' % (label, text)
    out = [first[:72]]
    rest = first[72:]
    while rest:
        out.append('     1' + rest[:66])
        rest = rest[66:]
    return out


def formats(name, lines, written):
    out = list(lines)
    n = 0
    for i, j in reversed(list(statements(lines))):
        label = lines[i][:5].strip()
        text = lines[i][6:72] + ''.join(l[6:72] for l in lines[i + 1:j + 1])
        m = re.match(r'\s*FORMAT\s*\((.*)\)\s*$', text)
        if not m or not label:
            continue
        if (name, label) in INPUT_FORMATS or label not in written:
            continue
        new = drop_cc(m.group(1), '%s %s' % (name, label))
        out[i:j + 1] = emit(label, new)
        n += 1
    return out, n


WRITE = re.compile(r'WRITE\s*\(\s*(?:TTYO|3)\s*,\s*(\d+)\s*\)')


# ------------------------------------------------------------ generic
def generic(name, text):
    text, n = re.subn(r'IMPLICIT INTEGER\*6\(A-Z\)', 'IMPLICIT INTEGER*8(A-Z)',
                      text)
    text = re.sub(r'\bINTEGER\*6 FUNCTION', 'INTEGER*8 FUNCTION', text)
    text = re.sub(r'\bINTEGER\*3\b', 'INTEGER*4', text)
    text = re.sub(r'\b(MAX2|MIN2|MOD2)\(', lambda m: m.group(1)[:3] + '(', text)
    text = re.sub(r'\bIABS\(', 'ABS(', text)
    # nD constants and the renamed functions: in statements only, outside
    # quoted strings (which may run on over a continuation line)
    out = []
    quoted = False
    for line in text.split('\n'):
        if line[:1] in ('C', 'c', '*') or not line.strip():
            out.append(line)
            continue
        head, body = line[:6], line[6:]
        pieces = []
        i = 0
        seg = ''
        while i < len(body):
            c = body[i]
            if quoted:
                seg += c
                if c == "'":
                    if i + 1 < len(body) and body[i + 1] == "'":
                        seg += "'"
                        i += 2
                        continue
                    quoted = False
                    pieces.append((True, seg))
                    seg = ''
            else:
                if c == "'":
                    pieces.append((False, seg))
                    seg = c
                    quoted = True
                else:
                    seg += c
            i += 1
        pieces.append((quoted, seg))
        new = ''
        for inq, s in pieces:
            if not inq:
                s = re.sub(r'(?<![A-Z0-9.])(\d+)D\b', r'\1_8', s)
                s = re.sub(r'\b(AND|OR|XOR)\(', r'K\1(', s)
                s = re.sub(r'\bRAN\(', 'KRAN(', s)
            new += s
        out.append(head + new)
    return '\n'.join(out)


def chrsets(text, name):
    m = re.search(r"      DIMENSION CHRSET\(64\)\n(?:\s*\n)*"
                  r"      DATA CHRSET/.*?/\n", text, re.S)
    if not m:
        return text, 0
    vals = hollerith_codes(m.group(0).split('DATA CHRSET', 1)[1])
    if len(vals) != 64:
        sys.exit('convert: %s: CHRSET has %d entries' % (name, len(vals)))
    return text.replace(m.group(0), '      DIMENSION CHRSET(64)\n' +
                        codes(vals, 'CHRSET') + '\n'), 1


TYPE = re.compile(r"      DATA \(\(TYPE\(I,J\),J=1,10\),I=1,3\)\n(.*?/)\n", re.S)


def types(text, name):
    m = TYPE.search(text)
    if not m:
        return text, 0
    vals = hollerith_codes(m.group(1))
    if len(vals) != 30:
        sys.exit('convert: %s: TYPE has %d entries' % (name, len(vals)))
    table = [vals[i * 10 + j] for j in range(10) for i in range(3)]
    return text.replace(m.group(0), codes(table, 'TYPE') + '\n'), 1


def main():
    os.makedirs(OUT, exist_ok=True)
    seen = set()
    n_fmt = n_chr = n_type = 0
    for name, lines in units():
        if name in REPLACED:
            continue
        if name in seen:
            sys.exit('convert: %s defined twice' % name)
        seen.add(name)
        text = '\n'.join(lines) + '\n'
        for count, old, new in FIX.get(name, ()):
            n = text.count(old + '\n')
            check('%s: %s' % (name, old.strip().split('\n')[0][:30]), n,
                  count)
            text = text.replace(old + '\n', new + '\n')
        if name == 'MAIN':
            text = addr_size(text)
        text, k = chrsets(text, name)
        n_chr += k
        text, k = types(text, name)
        n_type += k
        text = generic(name, text)
        if name == 'MAIN':
            text = stmt_fn_literals(text)
        written = set(WRITE.findall(text))
        lines, k = formats(name, text.rstrip('\n').split('\n'), written)
        n_fmt += k
        text = '\n'.join(lines) + '\n'
        if name in ('AND', 'OR', 'XOR', 'RAN'):
            k = len(re.findall(r'^      INTEGER\*8 FUNCTION K%s\(' % name,
                               text, re.M))
            check('rename %s' % name, k, 1)
        for i, l in enumerate(text.split('\n')):
            if len(l) > 72 and l[:1] not in ('C', 'c', '*'):
                sys.exit('convert: %s line %d past column 72:\n%s'
                         % (name, i + 1, l))
        fname = {'AND': 'kand', 'OR': 'kor', 'XOR': 'kxor', 'RAN': 'kran'}.get(
            name, name.lower())
        open(os.path.join(OUT, fname + '.f'), 'w', encoding='latin-1',
             newline='\n').write(text)
    check('CHRSET tables', n_chr, 3)
    check('TYPE tables', n_type, 2)
    check('output FORMATs', n_fmt, N_OUTPUT_FORMATS)
    print('convert: %d units -> %s' % (len(seen), os.path.normpath(OUT)))
    print('convert: ' + ', '.join('%s %d' % c for c in COUNTS
                                  if not c[0].startswith(('MAIN:', 'CVSTB:'))))


N_OUTPUT_FORMATS = 38


if __name__ == '__main__':
    main()
