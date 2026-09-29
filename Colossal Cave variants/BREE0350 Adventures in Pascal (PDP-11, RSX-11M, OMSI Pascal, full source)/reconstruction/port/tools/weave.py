#!/usr/bin/env python3
"""weave.py - turn Barry Breen's OMSI Pascal modules into Free Pascal include files.

The original modules (src_original/*.PAS) are left untouched.  Each one is
copied to port/build/gen/<module>.inc with a small set of mechanical, line
preserving substitutions, so gen/X.inc line N is always src_original/X.PAS
line N.  The substitutions are all dialect, never game logic:

  1. `PROCEDURE X(...);EXTERNAL;` declarations are blanked.  OMSI compiled
     every module separately against ADVGBL; here all modules are included
     into one program, and gen/forwards.inc (generated from the real headers)
     declares every global-level routine FORWARD instead.
  2. OMSI random-access files.  `F: FILE OF T` becomes an OMSIRT file handle
     plus an explicit buffer variable F_buf, `F^` becomes F_buf, and
     RESET/REWRITE/SEEK/GET/PUT/CLOSE on those files call OMSIRT, which
     reproduces the OMSI semantics (SEEK loads record n into F^, PUT rewrites
     it in place) and the on-disk layout (records never span a 512-byte block).
  3. `EXIT` (OMSI: leave the innermost loop) becomes BREAK.
  4. `{$C ...}` inline MACRO-11 becomes an ordinary comment; the one block
     that does something (DATIME's GTIM$ directive) is replaced by a call.
  5. Octal literals `33B` become decimal.
  6. READLN from the terminal and from the ASCII database goes through
     OMSIRT (blank-padded character arrays, end-of-file handling).
  7. A handful of exact-text patches, listed in PATCHES below with reasons.

usage: weave.py <src_original> <gendir>
"""
import os, re, sys

# ---------------------------------------------------------------- tokenizer
TOKEN = re.compile(r"""
    (?P<comment>\{[^}]*\}|\(\*.*?\*\))   # OMSI comments do not nest
  | (?P<string>'(?:[^'\n]|'')*')
  | (?P<ident>[A-Za-z][A-Za-z0-9_$]*)
  | (?P<number>[0-9]+(?:\.[0-9]+)?(?:[Ee][+-]?[0-9]+)?B?)
  | (?P<ws>\s+)
  | (?P<op>:=|<>|<=|>=|\.\.|.)
""", re.S | re.X)

def tokens(text):
    """yield (kind, text, start, end)"""
    for m in TOKEN.finditer(text):
        yield m.lastgroup, m.group(), m.start(), m.end()

def code_tokens(text):
    return [t for t in tokens(text) if t[0] not in ('comment', 'ws')]

# ---------------------------------------------------------------- structure
class Routine:
    def __init__(self, kind, name, header, start, end, directive):
        self.kind, self.name, self.header = kind, name, header
        self.start, self.end, self.directive = start, end, directive

def parse_module(text):
    """Find the global-level routines of a module.

    Returns (routines, main_start): every PROCEDURE/FUNCTION declared at the
    outermost level (with its header text and whether it is EXTERNAL/FORWARD
    or has a body), and the offset of the main BEGIN if the module has one.
    """
    toks = code_tokens(text)
    pos = 0
    routines = []
    main_start = None

    def up(i): return toks[i][1].upper() if i < len(toks) else ''

    def skip_header(i):
        # PROCEDURE name [(params)] [: type] ;
        depth = 0
        while i < len(toks):
            t = toks[i][1]
            if t == '(': depth += 1
            elif t == ')': depth -= 1
            elif t == ';' and depth == 0:
                return i + 1
            i += 1
        raise SyntaxError('unterminated header')

    def skip_compound(i):
        # toks[i] is BEGIN; return index after matching END
        depth = 0
        while i < len(toks):
            u = up(i)
            if u in ('BEGIN', 'CASE'): depth += 1
            elif u == 'END':
                depth -= 1
                if depth == 0: return i + 1
            i += 1
        raise SyntaxError('unterminated BEGIN')

    def parse_block(i, level):
        # declarations ... BEGIN ... END
        nonlocal main_start
        while i < len(toks):
            u = up(i)
            if u in ('PROCEDURE', 'FUNCTION'):
                hstart = toks[i][2]
                name = toks[i + 1][1]
                j = skip_header(i)
                hend = toks[j - 1][3]
                d = up(j)
                if d in ('EXTERNAL', 'FORWARD'):
                    # directive ;
                    end = toks[j + 1][3]
                    if level == 0:
                        routines.append(Routine(u, name, text[hstart:hend], hstart, end, d))
                    i = j + 2
                else:
                    k = parse_block(j, level + 1)
                    # trailing ;
                    assert toks[k][1] == ';', (name, toks[k])
                    if level == 0:
                        routines.append(Routine(u, name, text[hstart:hend], hstart, toks[k][3], None))
                    i = k + 1
            elif u == 'RECORD':
                # skip to matching END (records may hold CASE variants; none here)
                depth = 0
                while True:
                    uu = up(i)
                    if uu == 'RECORD': depth += 1
                    elif uu == 'END':
                        depth -= 1
                        if depth == 0: break
                    i += 1
                i += 1
            elif u == 'BEGIN':
                if level == 0: main_start = toks[i][2]
                return skip_compound(i)
            else:
                i += 1
        return i

    parse_block(0, 0)
    return routines, main_start

# ---------------------------------------------------------------- patches
# (module, exact old text, new text, reason).  Every patch must apply exactly once.
PATCHES = [
    ('DATIME',
     "{$C\tGTIM$C  TIMBUF\n\tMOV\tTIMBUF+G.TIMO,MO(%6)\n\tMOV\tTIMBUF+G.TIDA,DAY(%6)\n\tMOV\tTIMBUF+G.TIYR,YEAR(%6)    }",
     "OMSIGTIM(MO,DAY,YEAR); { was inline MACRO-11:\n\tGTIM$C  TIMBUF / MOV TIMBUF+G.TIMO,MO(%6)\n\tMOV TIMBUF+G.TIDA,DAY(%6)\n\tMOV TIMBUF+G.TIYR,YEAR(%6)    }",
     "the RSX GTIM$ executive directive (month, day, year-1900)"),
    ('ASK',
     "FUNCTION ASK(I,J,K: INTEGER; PROCEDURE SPK): BOOLEAN;",
     "FUNCTION ASK(I,J,K: INTEGER; SPK: OMSISPK): BOOLEAN;",
     "pre-ISO procedural parameter without a parameter list"),
    ('ASK',
     "TYPE ARR5=ARRAY[1..5] OF INTEGER;",
     "{ TYPE ARR5=ARRAY[1..5] OF INTEGER; - see glue.inc }",
     "ARR5 was declared in each separately compiled module that used it; one program needs it once"),
    ('ADVGBL',
     "WKDAY,WKEND,HOLID:ARRAY[1..2] OF INTEGER;",
     "WKDAY,WKEND,HOLID:OMSIDBL;",
     "OMSI matched array types by structure, Free Pascal by name: one named type for the hours masks"),
    ('HOURS',
     "TYPE	DBL=ARRAY[1..2] OF INTEGER;",
     "TYPE	DBL=OMSIDBL;",
     "same - the hours masks"),
    ('PRIMET',
     "TYPE	DBL=ARRAY[1..2] OF INTEGER;",
     "TYPE	DBL=OMSIDBL;",
     "same - the hours masks"),
    ('START',
     "    PRIMTM:ARRAY[1..2] OF INTEGER;",
     "    PRIMTM:OMSIDBL;",
     "same - the hours masks"),
    ('ADVINI',
     "PROCEDURE GETDT;\nBEGIN",
     "PROCEDURE GETDT; VAR I:INTEGER; { own counter }\nBEGIN",
     "Free Pascal wants a FOR counter local; OMSI let a nested procedure use its parent's"),
    ('GETIN',
     "PROCEDURE GETWORD(VAR X: BOOLEAN; VAR WORDN: WORD);\nBEGIN",
     "PROCEDURE GETWORD(VAR X: BOOLEAN; VAR WORDN: WORD); VAR J:INTEGER; { own counter }\nBEGIN",
     "Free Pascal wants a FOR counter local; OMSI let a nested procedure use its parent's"),
    ('ADVFLS',
     "    LINES:ARRAY[1..72] OF CHAR;\n\nPROCEDURE PUTTXT;",
     "    LINES:LINE;\n\nPROCEDURE PUTTXT;",
     "assigned to ADVTXT^.TXT, which is a LINE: types match by name in Free Pascal"),
    ('100FLS',
     "    LINES:ARRAY[1..72] OF CHAR;\n\nPROCEDURE PUTTXT;",
     "    LINES:LINE;\n\nPROCEDURE PUTTXT;",
     "assigned to ADVTXT^.TXT, which is a LINE: types match by name in Free Pascal"),
    ('ADVFLS',
     "PROCEDURE NEWREC;\nBEGIN",
     "PROCEDURE NEWREC; VAR J:INTEGER; { own counter }\nBEGIN",
     "Free Pascal wants a FOR counter local; OMSI let a nested procedure use its parent's"),
    ('TRAVEL',
     "WHILE (LINK2^.VERBVAL<>K) AND (LINK2<>NIL) DO",
     "WHILE (LINK2<>NIL) AND (LINK2^.VERBVAL<>K) DO",
     "the one place that reads through NIL before testing for it.  OMSI evaluates both "
     "operands and an RSX task may read location 0, so the original never noticed; the "
     "value read cannot matter because the other operand is already false.  Windows faults."),
    ('TRAVEL',
     "\t    ELSE IF I<>LINK1^.NEWLOC[2] THEN BEGIN\n",
     "\t    ELSE IF (LINK1=NIL) OR (I<>LINK1^.NEWLOC[2]) THEN BEGIN { port: NIL guard }\n",
     "BACK with no way back: BACKUP returns LINK1=NIL and the original then compares I with "
     "whatever an RSX task has at location 6.  It is never the wanted location, so the "
     "answer is message 140 either way; Windows faults on the read."),
    ('TRAVEL',
     "\tNEWLOC:=LINK1^.NEWLOC[2];\n\tOLDLC2:=OLDLOC;",
     "\tNEWLOC:=LINK1^.NEWLOC[2]; IF (NEWLOC=0) AND OMSIFIXES THEN NEWLOC:=-1; { FIX 1 }\n\tOLDLC2:=OLDLOC;",
     "FIX 1 (game logic, off with --no-fixes): the database still sends the two fatal falls "
     "(locations 20 and 21) to location 0, the FORTRAN convention for dead; this Pascal "
     "version uses -1 everywhere, so the player landed in 'location 0' among the objects "
     "that do not exist yet instead of dying"),
    ('TAKE',
     "\t    OBJ:=ATLOC[LOCATION]^.OBJ;\n",
     "\t    OBJ:=ATLOC[LOCATION]^.OBJ; IF (OBJ>100) AND OMSIFIXES THEN OBJ:=OBJ-100; { FIX 2 }\n",
     "FIX 2 (game logic, off with --no-fixes): a bare TAKE where the only thing present is the "
     "far end of a two-place object (the stone steps, the grate...) picked up its list entry "
     "OBJ+100 and indexed PLACE/FIXED/PROP with it, off the end of the arrays.  WHATSHERE "
     "already folds OBJ>100 back; TAKEIT forgot to (so did the FORTRAN)"),
    ('MAGICM',
     "\tREADLN(MAGICWORD);\n",
     "\tREADLN(MAGICWORD); IF OMSIFIXES THEN CAPS(MAGICWORD); { FIX 3 }\n",
     "FIX 3 (game logic, off with --no-fixes): maintenance mode stored a new magic word exactly "
     "as typed, but GETIN capitalises whatever the player types before WIZARD compares it, so a "
     "word entered in lower case could never be matched again and only POOF got the wizards "
     "back in.  An RSX terminal of 1980 usually upper-cased input by itself; a PC does not"),
    ('ASK',
     "\tMSPEAK(17);\n",
     "\tMSPEAK(17); OMSIWIZWORD(MAGICWORD); { port: -u says the magic word }\n",
     "port -u wizard hint (src/wizhint.inc), prints only with -u: the magic word, as WIZARD asks for it"),
    ('ASK',
     "\t    WRTMAGIC(MAGICWORD);\n",
     "\t    WRTMAGIC(MAGICWORD); OMSIWIZREPLY(MAGICWORD,MAGICNUMBER); { port: -u says the reply }\n",
     "port -u wizard hint (src/wizhint.inc), prints only with -u: the reply to the challenge "
     "WRTMAGIC has just written, from CHKMAGIC's own loop"),
    ('ADVINI',
     "\tWRITE(CHR(33B),'<');\n",
     "\tWRITE(CHR(33B),'<'); OMSIVT100; { port: a VT100 has auto wrap off }\n",
     "port terminal handling, no game logic: every VT100 line is written as 72 columns but a "
     "double-width line holds 40; a VT100 shipped with auto wrap off, a present-day terminal "
     "wraps the trailing blanks into a blank row under every big line and splits double-height "
     "text in two.  OMSIVT100 turns auto wrap off for the session (and back on at exit)"),
    ('MAIN',
     "BEGIN {MAIN PROGRAM}\n",
     "BEGIN OMSIDEBUG:=ADVDEBUG; {MAIN PROGRAM} { port: hand OMSIRT the --debug handler }\n",
     "port test bench only: registers src/debug.inc; does nothing unless run with --debug"),
    ('SAVNAM',
     "PROCEDURE SAVNAME(VAR SAV:INTEGER);\n",
     "PROCEDURE SAVNAME(VAR SAV:INTEGER); VAR SAVX:INTEGER; { FOR counter, see below }\n",
     "Free Pascal will not take a VAR parameter as a FOR counter"),
    ('SAVNAM',
     "    FOR SAV:=1 TO 3 DO BEGIN\n",
     "    FOR SAVX:=1 TO 3 DO BEGIN SAV:=SAVX; { was FOR SAV:=1 TO 3 }\n",
     "Free Pascal will not take a VAR parameter as a FOR counter"),
    ('WIZMAG',
     "TYPE ARR5=ARRAY[1..5] OF INTEGER;",
     "{ TYPE ARR5=ARRAY[1..5] OF INTEGER; - see glue.inc }",
     "ARR5 was declared in each separately compiled module that used it; one program needs it once"),
]

def apply_patches(module, text, log):
    for mod, old, new, why in PATCHES:
        if mod != module: continue
        n = text.count(old)
        if n != 1:
            raise SystemExit('patch for %s (%s) matches %d times' % (mod, why, n))
        assert old.count('\n') == new.count('\n'), 'patch must preserve line count: ' + why
        text = text.replace(old, new)
        log.append('%s: patched - %s' % (module, why))
    return text

# ---------------------------------------------------------------- transforms
# identifiers that are reserved words in Free Pascal but were free in OMSI Pascal
RENAME = {'OBJECT': 'OBJECT_'}

def blank(s):
    """same-shape filler: keeps newlines (and so line numbers), blanks the rest"""
    return ''.join(c if c == '\n' else ' ' for c in s)

def transform(module, text, filevars, log):
    text = apply_patches(module, text, log)
    routines, main_start = parse_module(text)

    # 1. blank EXTERNAL declarations (leave a marker comment on the first line)
    edits = []
    for r in routines:
        if r.directive in ('EXTERNAL', 'FORWARD'):
            seg = text[r.start:r.end]
            first, nl, rest = seg.partition('\n')
            mark = '{%s %s}' % (r.directive[:3].lower(), r.name)
            first = (mark + ' ' * len(first))[:max(len(first), len(mark))]
            edits.append((r.start, r.end, first + nl + blank(rest)))
    for s, e, new in sorted(edits, reverse=True):
        text = text[:s] + new + text[e:]

    # token-level rewrites (never inside comments or strings)
    out = []
    toks = list(tokens(text))
    i = 0
    n_exit = n_oct = 0
    renamed = set()
    def nxt(j):
        j += 1
        while j < len(toks) and toks[j][0] in ('ws', 'comment'): j += 1
        return j
    while i < len(toks):
        kind, s, a, b = toks[i]
        U = s.upper()
        if kind == 'comment' and s.startswith('{$'):
            out.append('{ ' + s[2:])           # same length: '{$' -> '{ '
        elif kind == 'number' and U.endswith('B'):
            dec = str(int(s[:-1], 8))
            out.append(dec + ' ' * (len(s) - len(dec)) if len(dec) <= len(s) else dec)
            n_oct += 1
        elif kind == 'ident' and U in RENAME:
            out.append(RENAME[U]); renamed.add(U)
        elif kind == 'ident' and U == 'EXIT':
            out.append('BREAK'); n_exit += 1
        elif kind == 'ident' and U in filevars and toks[i + 1][1] == '^' if i + 1 < len(toks) else False:
            out.append(s + '_buf'); i += 1     # F^ -> F_buf
        elif kind == 'ident' and U in ('RESET', 'REWRITE', 'SEEK', 'GET', 'PUT', 'CLOSE', 'READLN', 'READ'):
            j = nxt(i)
            if j < len(toks) and toks[j][1] == '(':
                k = nxt(j)
                arg = toks[k][1].upper() if k < len(toks) else ''
                if U == 'READ':
                    out.append('OMSIRDINTF' if arg == 'TXTFILE' else s)
                elif U == 'READLN':
                    if arg == 'TXTFILE':
                        out.append('OMSIRDLNF')
                    else:
                        out.append('OMSIRDLN')
                elif arg in filevars:
                    if U in ('RESET', 'REWRITE'):
                        # RESET(F,'name') -> OMSIRESET(F,F_buf,SIZEOF(F_buf),'name')
                        out.append('OMSI' + U)
                        out.append(text[toks[i][3]:toks[k][3]])      # "(F"
                        name = toks[k][1]
                        out.append(',%s_buf,SIZEOF(%s_buf)' % (name, name))
                        i = k
                    else:
                        out.append('OMSI' + U)
                elif arg == 'TXTFILE' and U == 'RESET':
                    out.append('OMSIRESETTEXT')
                else:
                    out.append(s)
            else:
                out.append(s)
        else:
            out.append(s)
        i += 1
    text = ''.join(out)

    # 2b. file declarations:  A,B:FILE OF T;  ->  A,B:OMSIFILE; A_buf,B_buf:T;
    def decl(m):
        names = [x.strip() for x in m.group(2).split(',')]
        return '%s%s:OMSIFILE; %s:%s;' % (m.group(1), ','.join(names),
                                           ','.join(x + '_buf' for x in names), m.group(3))
    text = re.sub(r'(?im)^(\s*(?:VAR\s+)?)([A-Z0-9_]+(?:\s*,\s*[A-Z0-9_]+)*)\s*:\s*FILE\s+OF\s+([A-Z0-9_]+)\s*;', decl, text)

    if renamed: log.append('%s: renamed %s' % (module, ', '.join(sorted(renamed))))
    if n_exit: log.append('%s: %d EXIT -> BREAK' % (module, n_exit))
    if n_oct: log.append('%s: %d octal literal(s)' % (module, n_oct))
    return text, routines, main_start

FILE_DECL = re.compile(r'(?im)^\s*(?:VAR\s+)?([A-Z0-9_]+(?:\s*,\s*[A-Z0-9_]+)*)\s*:\s*FILE\s+OF\s+[A-Z0-9_]+\s*;')

def main():
    src, gen = sys.argv[1], sys.argv[2]
    os.makedirs(gen, exist_ok=True)
    mods = sorted(f[:-4] for f in os.listdir(src) if f.upper().endswith('.PAS'))
    texts = {m: open(os.path.join(src, m + '.PAS'), encoding='latin-1', newline='').read() for m in mods}
    # every typed-file variable declared anywhere
    filevars = set()
    for m, t in texts.items():
        stripped = ''.join(s if k != 'comment' else blank(s) for k, s, a, b in tokens(t))
        for mm in FILE_DECL.finditer(stripped):
            filevars.update(x.strip().upper() for x in mm.group(1).split(','))
    log = ['typed files: ' + ', '.join(sorted(filevars))]
    info = {}
    for m in mods:
        new, routines, main_start = transform(m, texts[m], filevars, log)
        assert new.count('\n') == texts[m].count('\n'), m
        info[m] = (routines, main_start)
        with open(os.path.join(gen, m.lower() + '.inc'), 'w', encoding='latin-1', newline='') as fh:
            fh.write(new)
    # report + forwards
    programs = [m for m in mods if info[m][1] is not None]
    log.append('programs (have a main block): ' + ', '.join(programs))
    defined = {}
    for m in mods:
        for r in info[m][0]:
            if r.directive is None:
                defined.setdefault(r.name.upper(), []).append((m, r))
    dup = {k: [x[0] for x in v] for k, v in defined.items() if len(v) > 1}
    if dup: log.append('routines defined in more than one module: %r' % dup)
    for m in mods:
        ext = [r.name.upper() for r in info[m][0] if r.directive == 'EXTERNAL']
        missing = [e for e in ext if e not in defined]
        if missing: log.append('%s: EXTERNAL with no definition anywhere: %s' % (m, missing))
    # check EXTERNAL headers agree with the definitions
    norm = lambda h: re.sub(r'\s+', '', re.sub(r'\{[^}]*\}', '', h)).upper()
    for m in mods:
        for r in info[m][0]:
            if r.directive == 'EXTERNAL' and r.name.upper() in defined:
                dm, dr = defined[r.name.upper()][0]
                if norm(r.header) != norm(dr.header):
                    log.append('%s: EXTERNAL %s differs from %s: %s  vs  %s' % (m, r.name, dm, ' '.join(r.header.split()), ' '.join(dr.header.split())))
    game = [m for m in mods if m not in programs or m == 'MAIN']
    with open(os.path.join(gen, 'forwards.inc'), 'w', encoding='latin-1', newline='\n') as fh:
        fh.write('{ generated by weave.py: every global-level routine of the game modules }\n')
        for m in game:
            # a routine the module itself declared FORWARD is defined with an
            # abbreviated header (PROCEDURE SEARCH; {VAR LINK1:KEYLINK}) - the
            # parameters are on the FORWARD declaration
            own = {r.name.upper(): r.header for r in info[m][0] if r.directive == 'FORWARD'}
            for r in info[m][0]:
                if r.directive is None:
                    header = own.get(r.name.upper(), r.header)
                    fh.write('%s FORWARD; { %s }\n' % (' '.join(header.split()), m))
    with open(os.path.join(gen, 'modules.inc'), 'w', encoding='latin-1', newline='\n') as fh:
        fh.write('{ generated by weave.py: every game module except MAIN }\n')
        for m in game:
            if m not in ('MAIN', 'ADVGBL'):
                fh.write('{$I %s.inc}\n' % m.lower())
    with open(os.path.join(gen, 'weave.log'), 'w', encoding='latin-1', newline='\n') as fh:
        fh.write('\n'.join(log) + '\n')
    print('\n'.join(log))
    print('game modules:', ' '.join(game))

if __name__ == '__main__':
    main()
