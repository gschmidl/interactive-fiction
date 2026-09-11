#!/usr/bin/env python3
r"""Mechanically convert ADVENT.FOR (DEC FORTRAN-10) to gfortran.

    ../src_original/ADVENT.FOR   ->   ../port/src/advent.f

Nothing is ever hand-edited in the generated file.  Everything this
script does is either a *mechanical* rewrite applied to every line
(below), or one of the exact-line replacements listed in patches.py,
each of which quotes the original line verbatim and says why it had to
change.  `make` regenerates advent.f from the untouched original, so the
difference between the 1977 source and what compiles is auditable line
by line -- see README.md.

The port keeps the PDP-10's word: five seven-bit characters, char k in
bits 36-7k..30-7k, held sign-extended in an INTEGER*8.  The program's
own tables fix that layout (GETIN's DATA MASKS, A5TOA1's MASK), so every
mask, SHIFT and .AND./.XOR. in the game logic keeps working untouched.
Not one line of game logic is changed by this script or by patches.py.

The mechanical rewrites are:

  * "nnn      DEC octal literal   -> its decimal value, sign-extended
                                     through bit 35
  * 'ABCDE'   literal used as a word -> its packed decimal value
  * TYPE n    DEC output statement   -> PRINT n
  * RAN       -> RAN10  (plain RAN is a gfortran intrinsic and the
                         program defines its own)

Literals are packed only outside FORMAT statements, and never inside an
OPEN.  Source form: ADVENT.FOR uses DEC's tab convention (a tab in the
label field starts the statement, a tab followed by a digit starts a
continuation line); gfortran reads that as it stands, so it is left
alone.
"""
import io, os, re, sys

TAB = chr(9)

import patches

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
SRC = os.path.join(PORT, os.pardir, 'src_original', 'ADVENT.FOR')
DST = os.path.join(PORT, 'src', 'advent.f')


# ---------------------------------------------------------------- words

def sx36(v):
    """A 36-bit word as a sign-extended 64-bit integer.

    Bit 35 is the sign on a PDP-10 and the program tests it: A5TOA1
    inserts a blank between its second and third words only IF(C.LT.0),
    which is true exactly when the word's first character is 100 octal
    or above.  Sign-extending keeps every such test working; masks are
    unaffected, because any 36-bit mask clears the extended bits."""
    v &= (1 << 36) - 1
    return v - (1 << 36) if v & (1 << 35) else v


def pack(s):
    """A character literal as a PDP-10 word: left-justified, blank-filled."""
    s = (s + '     ')[:5]
    v = 0
    for k, c in enumerate(s):
        v |= (ord(c) & 0x7f) << (29 - 7 * k)
    return sx36(v)


# ---------------------------------------------------- DEC source layout

def parse(l):
    """Split a line into (kind, head, body).

    ADVENT.FOR is written in DEC's tab form: a tab anywhere in the label
    field ends it and the statement starts after it, and a line whose
    first character is a tab followed by a non-zero digit is a
    continuation.  Lines that use spaces follow the classic column
    rules."""
    if not l.strip():
        return ('blank', l, '')
    if l[0] in 'Cc*!':
        return ('comment', l, '')
    i = l.find('\t')
    if 0 <= i <= 5:
        if i == 0 and len(l) > 1 and l[1] in '123456789':
            return ('cont', l[:2], l[2:])
        return ('init', l[:i + 1], l[i + 1:])
    if len(l) > 5 and l[5] not in ' 0':
        return ('cont', l[:6], l[6:])
    return ('init', l[:6], l[6:])


def statements(lines):
    """[(line indices, joined body)] for each statement."""
    out, idx, body = [], [], ''
    for n, l in enumerate(lines):
        kind, head, txt = parse(l)
        if kind in ('blank', 'comment'):
            continue
        if kind == 'cont' and idx:
            idx.append(n)
            body += txt
            continue
        if idx:
            out.append((idx, body))
        idx, body = [n], txt
    if idx:
        out.append((idx, body))
    return out


# ------------------------------------------------------- the conversion

def convert_body(text, is_format, keep_strings):
    if is_format:
        return text
    if not keep_strings:
        text = re.sub(r"'([^']*)'", lambda m: str(pack(m.group(1))) + ' ', text)
    # DEC octal.  Decimal rather than a BOZ constant, because gfortran
    # refuses BOZ as an actual argument or in a comparison.
    text = re.sub(r'"([0-7]+)', lambda m: str(sx36(int(m.group(1), 8))) + ' ', text)
    text = re.sub(r'(?<![A-Z0-9])TYPE(?=[ 	]+[0-9])', 'PRINT', text)
    text = re.sub(r'\bRAN\b', 'RAN10', text)
    return text


# ------------------------------------------------- carriage control
# FOROTS took the first character of every formatted record as carriage
# control and did not print it: a blank meant "one new line".  Every
# record this program writes starts with a blank -- explicitly (' ',...)
# or from an nX -- so the rule here is exactly "take one blank off the
# front of every record".  Applied to the FORMAT statements, because
# gfortran has no way to put CARRIAGECONTROL='FORTRAN' on a preconnected
# unit 6.  Without it every line of the game would be indented one
# column further than the original printed it.

def cc_lines(lines):
    """Drop the carriage-control blank from every record of a FORMAT.

    Runs over the whole statement, which may be continued across lines,
    keeping a flag that says whether we are at the start of a record.
    Returns the lines, and the number of records fixed."""
    out = list(lines)
    fixed = [0]

    def fix_stmt(idx):
        at_start = True
        first = True
        for n in idx:
            kind, head, body = parse(out[n])
            t = body
            # skip up to and including the FORMAT's opening paren
            k = 0
            if first:
                k = t.upper().index('FORMAT') + 6
                while k < len(t) and t[k] != '(':
                    k += 1
                k += 1
                first = False
            res = t[:k]
            while k < len(t):
                c = t[k]
                if c == '/':
                    at_start = True
                    res += c
                    k += 1
                    continue
                if c in ' 	,':
                    res += c
                    k += 1
                    continue
                if c == ')':
                    res += t[k:]
                    k = len(t)
                    break
                if not at_start:
                    # step over one item, watching for quotes and /
                    if c == "'":
                        j = t.index("'", k + 1)
                        res += t[k:j + 1]
                        k = j + 1
                    else:
                        res += c
                        k += 1
                    continue
                # at the start of a record: take one blank off
                if c == "'":
                    j = t.index("'", k + 1)
                    lit = t[k + 1:j]
                    if not lit.startswith(' '):
                        raise SystemExit('carriage control: record starts '
                                         '%r in %r' % (lit, t))
                    lit = lit[1:]
                    if lit == '':
                        k = j + 1
                        while k < len(t) and t[k] == ',':
                            k += 1
                    else:
                        res += "'" + lit + "'"
                        k = j + 1
                    fixed[0] += 1
                    at_start = False
                    continue
                m = re.match(r'([0-9]+)X', t[k:])
                if m:
                    nx = int(m.group(1)) - 1
                    k += len(m.group(0))
                    if nx > 0:
                        res += '%dX' % nx
                    else:
                        while k < len(t) and t[k] == ',':
                            k += 1
                    fixed[0] += 1
                    at_start = False
                    continue
                raise SystemExit('carriage control: record starts %r' % t[k:])
            out[n] = head + res
    for idx, body in statements(out):
        if re.search(r'FORMAT', body.upper()):
            fix_stmt(idx)
    return out, fixed[0]


# --------------------------------------------- reading the declarations

KEYWORDS = set('''IF THEN ELSE ELSEIF ENDIF GOTO GO TO DO CONTINUE CALL
RETURN END STOP PAUSE READ WRITE PRINT TYPE ACCEPT FORMAT DATA COMMON
DIMENSION IMPLICIT INTEGER LOGICAL REAL DOUBLE PRECISION COMPLEX
CHARACTER EXTERNAL INTRINSIC EQUIVALENCE PARAMETER OPEN CLOSE UNIT NAME
ACCESS SEQIN PROGRAM SUBROUTINE FUNCTION BLOCK'''.split())

# everything the main program calls: library, game subprograms, and the
# runtime routines the patches introduce
CALLED = set('''SHIFT RAN RAN10 VOCAB PUT START YES YESM YESX MAX0 MIN0 MOD
IABS BUG DROP MOVE CARRY DSTROY JUGGLE SPEAK PSPEAK RSPEAK MSPEAK GETIN
A5TOA1 MAINT MOTD POOF HOURS HOURSX NEWHRS NEWHRX CIAO DATIME WSTR TRIM
RDFRE RDMSG RDVOC RDA5 DBOPEN BOOT INITD VERBOS UNLIM CIAOSV STATIO
BLKIO'''.split())


def main_program(lines):
    """The line indices of the main program (everything before the first
    SUBROUTINE/FUNCTION header)."""
    for n, l in enumerate(lines):
        kind, head, body = parse(l)
        if kind in ('blank', 'comment'):
            continue
        u = body.upper().replace(' ', '')
        if u.startswith('SUBROUTINE') or 'FUNCTION' in u.split('(')[0]:
            return n
    return len(lines)


def split_decl(s):
    """Split a declaration list on commas that are not inside ()."""
    out, cur, d = [], '', 0
    for c in s:
        if c == '(':
            d += 1
        elif c == ')':
            d -= 1
        if c == ',' and d == 0:
            out.append(cur)
            cur = ''
        else:
            cur += c
    if cur.strip():
        out.append(cur)
    return out


def scan_decls(lines):
    """dims{name: nwords}, commons[(block, [names])] over whole source."""
    dims, commons = {}, []
    for idx, body in statements(lines):
        b = body.replace(' ', '').replace('	', '').upper()
        if b.startswith('DIMENSION'):
            for d in split_decl(b[9:]):
                m = re.match(r'^([A-Z][A-Z0-9]*)\(([0-9,]+)\)$', d)
                if m:
                    n = 1
                    for x in m.group(2).split(','):
                        n *= int(x)
                    dims[m.group(1)] = n
        elif b.startswith('COMMON/'):
            m = re.match(r'^COMMON/([A-Z0-9]+)/(.*)$', b)
            if m:
                names = [x for x in split_decl(m.group(2)) if x]
                commons.append((m.group(1), names))
    return dims, commons


def main_variables(lines, end, dims, common_names):
    """Every variable of the main program, in first-appearance order.

    IMPLICIT INTEGER(A-Z) means every name is a variable unless it is a
    keyword, a subprogram, or a statement function.  Statement-function
    definitions are skipped, so their dummy arguments do not count."""
    sfuncs = set()
    for idx, body in statements(lines[:end]):
        b = body.replace(' ', '').replace('	', '')
        m = re.match(r'^([A-Z][A-Z0-9]*)\(([A-Z][A-Z0-9]*(,[A-Z][A-Z0-9]*)*)\)=', b)
        if m and m.group(1) not in dims and m.group(1).upper() not in KEYWORDS:
            sfuncs.add(m.group(1))
    order = []
    for idx, body in statements(lines[:end]):
        b = body.replace(' ', '').replace(TAB, '')
        u = b.upper()
        if u[:6] in ('FORMAT', 'COMMON') or u[:8] == 'IMPLICIT' or u[:8] == 'EXTERNAL':
            continue
        if re.match(r'^([A-Z][A-Z0-9]*)\(([A-Z][A-Z0-9]*(,[A-Z][A-Z0-9]*)*)\)=', b) \
           and b.split('(')[0] in sfuncs:
            continue
        t = re.sub(r"'[^']*'", ' ', body)          # literals
        t = re.sub(r'\.[A-Za-z]+\.', ' ', t)       # .AND. .TRUE. ...
        for name in re.findall(r'[A-Za-z][A-Za-z0-9]*', t):
            name = name.upper()
            if name in KEYWORDS or name in CALLED or name in sfuncs:
                continue
            if name in common_names:
                continue
            if name not in order:
                order.append(name)
    return order, sfuncs


# ------------------------------------------------------------ generator

def contin(lead, items, per=62):
    """A declaration list, broken with DEC tab continuations."""
    out, line = [], None
    for it in items:
        if line is None:
            line = lead + it
        elif len(line) + len(it) + 1 > per:
            out.append(line + ',')
            line = '	1	' + it
        else:
            line += ',' + it
    out.append(line)
    return out


def gen_common(names, dims, per=60):
    """The COMMON /ADVSTA/ declaration, in DEC tab-continuation form."""
    out, line = [], None
    for nm in names:
        if line is None:
            line = '\tCOMMON /ADVSTA/ ' + nm
        elif len(line) + len(nm) + 1 > per:
            out.append(line + ',')
            line = '\t1\t' + nm
        else:
            line += ',' + nm
    out.append(line)
    return out


def gen_statio(blocks):
    """SUBROUTINE STATIO -- the whole persistent state, one BLKIO call per
    variable.  Generated from the declarations, so nothing can be left
    out: if a variable exists it is in a COMMON block, and every COMMON
    block is here."""
    out = []
    a = out.append
    a('C  Generated by tools/convert.py -- do not edit.')
    a('C')
    a('C  The complete state of the game, for SUSPEND and for the images')
    a('C  MAGIC MODE leaves behind.  On TOPS-10 the monitor saved the whole')
    a('C  core image and every static variable came back; here every one of')
    a('C  them lives in a COMMON block and is written out by name.  MODE 1')
    a('C  writes, 0 reads, 2 counts.')
    a('')
    a('\tSUBROUTINE STATIO(LU,MODE)')
    a('\tIMPLICIT INTEGER(A-Z)')
    for blk, names in blocks:
        decl = [n if sz == 1 else '%s(%d)' % (n, sz) for n, sz in names]
        for l in contin('	COMMON /%s/ ' % blk, decl):
            a(l)
    a('')
    n = 0
    for blk, names in blocks:
        for nm, sz in names:
            n += sz
    for blk, names in blocks:
        a('C  /%s/' % blk)
        for nm, sz in names:
            a('\tCALL BLKIO(LU,MODE,%s,%d)' % (nm, sz))
    a('\tRETURN')
    a('\tEND')
    return out, n


# ----------------------------------------------------------------- main

def main():
    lines = io.open(SRC, encoding='latin-1').read().split('\n')
    # The -10's line printer page separators: a form feed in column 1
    # of a comment line; gfortran reads it as part of the label field.
    nff = len([l for l in lines if l[:1] == chr(12)])
    lines = [l[1:] if l[:1] == chr(12) else l for l in lines]
    end = main_program(lines)
    dims, commons = scan_decls(lines)
    common_names = set()
    for blk, names in commons:
        common_names.update(names)
    mvars, sfuncs = main_variables(lines, end, dims, common_names)

    # order: arrays as declared, then scalars, so the image is readable
    arrays = [v for v in mvars if v in dims]
    scalars = [v for v in mvars if v not in dims]
    state = arrays + scalars

    # the COMMON blocks of the finished program, with sizes
    blocks = []
    seen = set()
    for blk, names in commons:
        if blk in seen:
            continue
        seen.add(blk)
        blocks.append((blk, [(n, dims.get(n, 1)) for n in names]))
    blocks.append(('MOTCOM', [('MSG', 100)]))
    blocks.append(('RANCOM', [('R', 1)]))
    blocks.append(('ADVSTA', [(n, dims.get(n, 1)) for n in state]))

    # ---- line by line
    fmtlines, keep = set(), set()
    for idx, body in statements(lines):
        if re.search(r'\bFORMAT\b', body.upper()):
            fmtlines.update(idx)
        if re.search(r'\b(OPEN|CLOSE)\s*\(', body.upper()):
            keep.update(idx)

    used = {}
    out = []
    decls = {k: v[:] for k, v in patches.DECLS.items()}
    cur = '@MAIN'
    skip_to_end = False
    for n, l in enumerate(lines):
        kind, head, body = parse(l)
        u = body.strip().upper()
        if skip_to_end:
            if kind == 'init' and u == 'END':
                skip_to_end = False
            continue
        if kind == 'init' and u in patches.DROP_ROUTINES:
            skip_to_end = True
            used[u] = used.get(u, 0) + 1
            continue
        if kind == 'init':
            m = re.match(r'^(?:INTEGER |LOGICAL |REAL )?(?:SUBROUTINE|FUNCTION) *'
                         r'([A-Z][A-Z0-9]*)', u)
            if m:
                cur = m.group(1)
        if l in patches.LINES:
            used[l] = used.get(l, 0) + 1
            out.extend(patches.LINES[l])
        else:
            out.append(head + convert_body(body, n in fmtlines, n in keep))
        if kind == 'init' and u.startswith('IMPLICIT') and cur in decls:
            out.extend(decls.pop(cur))
        if l == patches.STATE_ANCHOR:
            used[l] = used.get(l, 0) + 1
            out.extend(gen_common(state, dims))

    # ---- FOROTS carriage control
    out, ncc = cc_lines(out)

    # ---- the generated state routine
    statio, nwords = gen_statio(blocks)
    out.append('C  ' + '-' * 66)
    out.extend(statio)

    io.open(DST, 'w', encoding='latin-1', newline='\n').write('\n'.join(out))


    # a map of the saved image, so that a difference between two saves
    # can be named rather than counted
    man = []
    off = 0
    for blk, names in blocks:
        for nm, sz in names:
            man.append('%6d %5d %-8s %s' % (off, sz, blk, nm))
            off += sz
    io.open(os.path.join(PORT, 'src', 'state.map'), 'w',
            newline=chr(10)).write(chr(10).join(man) + chr(10))
    # ---- report and self-check
    miss = [k for k in patches.LINES if k not in used]
    miss += [k for k in patches.DROP_ROUTINES if k not in used]
    if patches.STATE_ANCHOR not in used:
        miss.append(patches.STATE_ANCHOR)
    if decls:
        print('DECLARATIONS NOT PLACED: %s' % ' '.join(decls))
        sys.exit(1)
    if miss:
        print('PATCHES THAT MATCHED NOTHING:')
        for m in miss:
            print('   %r' % m)
        sys.exit(1)
    dup = [k for k, v in used.items() if v > 1 and k not in patches.MULTI]
    if dup:
        print('PATCHES THAT MATCHED MORE THAN ONCE:')
        for d in dup:
            print('   %r x%d' % (d, used[d]))
        sys.exit(1)
    for nm in patches.STATE_MUST_HAVE:
        if nm not in state:
            print('state capture is missing %s' % nm)
            sys.exit(1)
    print('ADVENT.FOR -> src/advent.f   %d lines in, %d out'
          % (len(lines), len(out)))
    print('  %d page separators (form feeds) removed' % nff)
    print('  %d carriage-control blanks removed from FORMAT records' % ncc)
    print('  %d exact-line patches, %d routines dropped'
          % (len(patches.LINES), len(patches.DROP_ROUTINES)))
    print('  main-program state: %d arrays, %d scalars'
          % (len(arrays), len(scalars)))
    print('  saved image: %d words in %d common blocks'
          % (nwords, len(blocks)))
    if '-l' in sys.argv:
        print('  arrays : ' + ' '.join(arrays))
        print('  scalars: ' + ' '.join(scalars))


if __name__ == '__main__':
    main()
