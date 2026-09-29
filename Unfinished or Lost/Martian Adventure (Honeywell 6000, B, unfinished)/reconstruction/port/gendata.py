#!/usr/bin/env python3
"""
gendata.py -- the missing compiler.

Brad Templeton's 1978 B program (jmc/mars/game) opened two binary files,
"jmc/mars/desc" and "jmc/mars/rooms", that had been produced from the text
sources by a separate tool.  Neither binary survived, and neither did the
tool.  This script does that job: it reads the surviving text sources
verbatim and emits data.h for mars.c.

It also stands in for the compiler Mark Niemiec wrote for the adventure
definition language, of which jmc/mars/math is the only surviving program.

Nothing here invents content.  The only editorial act is documented in
restore_dropped_lines() and is reported on stderr.
"""
import os, re, sys, difflib

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, '..', 'src_original', 'mars')

def rd(name):
    with open(os.path.join(SRC, name), 'rb') as f:
        return f.read().decode('latin-1')

# ---------------------------------------------------------------- Mars map

# The index order is fixed by the fall-through chain in game.v2.txt:
#   d/down->10  up/u->9  nw->8  west/w->7  sw->6  s->5  se->4  east/e->3
#   ne->2  nort/n->1     ... then q = char(rvec, po)
DIRS  = ['', 'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'U', 'D']
DIRRE = re.compile(r'(NE|NW|SE|SW|N|S|E|W|U|D)\s*(\d+)')

def parse_goto():
    rooms = {}
    for ln in rd('goto').split('\n'):
        if not ln.strip():
            continue
        m = re.match(r'^(\d{3})\s+(\d+)(.*)$', ln)
        if not m:
            sys.exit('goto: cannot parse %r' % ln)
        flags, num, rest = m.group(1), int(m.group(2)), m.group(3)
        ex = [0] * 11
        seen = 0
        for m2 in DIRRE.finditer(rest):
            d, t = m2.group(1), m2.group(2)
            ex[DIRS.index(d)] = int(t)
            seen += 1
        if seen != len(re.findall(r'\d+', rest)):
            sys.exit('goto: exit mismatch on room %d' % num)
        rooms[num] = (flags, ex)
    return rooms

def parse_snames():
    out, cur = {}, None
    for ln in rd('snames').split('\n'):
        m = re.match(r'^(\d{3})\s+(\d+)\s*$', ln)
        if m:
            cur = int(m.group(2))
            out[cur] = [m.group(1), '']
        elif cur is not None and ln.strip():
            out[cur][1] = ln.strip()
    return out

def parse_numbered(text):
    """Files whose format is a bare room number on a line, then its lines."""
    out, cur = {}, None
    for ln in text.split('\n'):
        if re.fullmatch(r'\d+\s*', ln):
            cur = int(ln.strip())
            out[cur] = []
        elif cur is not None:
            out[cur].append(ln)
    for k in out:
        while out[k] and not out[k][-1].strip():
            out[k].pop()
    return out

# ------------------------------------------------- indat line restoration

PROPER = ['Mars', 'Martian', 'Diemos', 'Deimos', 'Gmog', 'Mwa', 'Makwu',
          'Earth', 'Olympics', 'Diemosian', 'Demosian', 'Mother Earth']

def to_v2_case(line, after_sentence_end):
    """Reproduce the hand pass that turned indat.v1 (all caps) into indat.v2.

    Reading v2 against v1 line by line, the rule the author used was:
    lowercase everything, then capitalise the first letter of a room, the
    first letter after a line that ended a sentence, and the first letter
    after .!? followed by *two* spaces -- a single space after a full stop
    was left lowercase (rooms 27, 28, 31).  Proper nouns were kept."""
    s = line.lower()
    s = re.sub(r'([.!?]  )([a-z])', lambda m: m.group(1) + m.group(2).upper(), s)
    if after_sentence_end and s[:1].isalpha():
        s = s[0].upper() + s[1:]
    for w in sorted(PROPER, key=len, reverse=True):
        s = re.sub(r'\b' + w.lower().replace(' ', r'\s+') + r'\b', w, s)
    return s

def restore_dropped_lines(v1, v2):
    """indat.v2 lost five whole lines that indat.v1 still has.  Put them back.

    Room 4 is NOT restored: there v1 carries two lines of a superseded
    rewording that the author deliberately deleted when making v2."""
    notes = []
    for rm in sorted(set(v1) & set(v2)):
        if rm == 4:
            continue
        a = [x.lower().rstrip() for x in v1[rm]]
        b = [x.lower().rstrip() for x in v2[rm]]
        if a == b:
            continue
        sm = difflib.SequenceMatcher(None, b, a, autojunk=False)
        newb = []
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == 'insert':
                for j in range(j1, j2):
                    prev = newb[-1].rstrip() if newb else ''
                    txt = to_v2_case(v1[rm][j],
                                     not newb or prev[-1:] in '.!?')
                    newb.append(txt)
                    notes.append((rm, len(newb), txt))
            else:
                newb += v2[rm][i1:i2]
        v2[rm] = newb
    return notes

# ------------------------------------------- the adventure-language parser

QUOTE = '"'
BQ    = chr(96)

class Tok:
    def __init__(s, k, v, ln): s.k, s.v, s.ln = k, v, ln
    def __repr__(s): return '%s(%r)' % (s.k, s.v)

def lex(src):
    i, ln, out = 0, 1, []
    while i < len(src):
        c = src[i]
        if c == '\n':
            ln += 1; i += 1; continue
        if c in ' \t\r':
            i += 1; continue
        if src.startswith('/*', i):
            j = src.find('*/', i + 2)
            i = len(src) if j < 0 else j + 2
            continue
        if c == QUOTE or c == BQ:
            j = src.index(c, i + 1)
            body = src[i + 1:j]
            ln += body.count('\n')
            body = '\n'.join(x.lstrip('\t') for x in body.split('\n'))
            out.append(Tok('str' if c == QUOTE else 'bq', body, ln))
            i = j + 1; continue
        if src.startswith('==', i) or src.startswith('!=', i):
            out.append(Tok('op', src[i:i + 2], ln)); i += 2; continue
        if c.isalpha() or c in '_.':
            j = i + 1
            while j < len(src) and (src[j].isalnum() or src[j] in '_.'):
                j += 1
            out.append(Tok('id', src[i:j], ln)); i = j; continue
        if c.isdigit():
            j = i
            while j < len(src) and src[j].isdigit():
                j += 1
            out.append(Tok('num', src[i:j], ln)); i = j; continue
        out.append(Tok('p', c, ln)); i += 1
    out.append(Tok('eof', '', ln))
    return out

class Prog:
    """Emits the little bytecode that mars.c interprets."""
    OPS = dict(END=0, PRINT=1, PROMPT=2, MOVE=3, SET=4, GETSTR=5,
               JNEVAR=6, JNEARG=7, JEQARG=8, JMP=9)

    def __init__(s):
        s.code, s.strs, s.vars, s.rooms = [], [], [], []

    def sid(s, t):
        if t not in s.strs: s.strs.append(t)
        return s.strs.index(t)

    def vid(s, t):
        t = t.lstrip('.')
        if t not in s.vars: s.vars.append(t)
        return s.vars.index(t)

    def rid(s, t):
        if t not in s.rooms: s.rooms.append(t)
        return s.rooms.index(t)

    def emit(s, op, *a):
        at = len(s.code)
        s.code.append(s.OPS[op])
        s.code += list(a)
        return at

VALS = {'OPEN': 1, 'YES': 1, 'CLOSED': 0, 'NO': 0}

class Parser:
    def __init__(s, toks, prog):
        s.t, s.i, s.p = toks, 0, prog

    def pk(s, n=0): return s.t[s.i + n]
    def nx(s): s.i += 1; return s.t[s.i - 1]

    def want(s, k, v=None):
        t = s.nx()
        if t.k != k or (v is not None and t.v != v):
            sys.exit('math: line %d: expected %s %s, got %r' % (t.ln, k, v, t))
        return t

    def parse(s):
        rooms = []
        while s.pk().k != 'eof':
            name = s.want('id').v
            s.want('p', ':'); s.want('p', '{')
            r = dict(name=name, long=None, brief=None, clauses=[])
            while not (s.pk().k == 'p' and s.pk().v == '}'):
                keys = []
                while (s.pk().k == 'id' and s.pk(1).k == 'p'
                       and s.pk(1).v == ':'):
                    keys.append(s.nx().v); s.nx()
                if keys in (['long'], ['brief']):
                    r[keys[0]] = s.want('str').v
                    s.semi(); continue
                if not keys:
                    sys.exit('math: line %d: clause without key' % s.pk().ln)
                pc = s.stmt()
                s.p.emit('END')
                r['clauses'].append((keys, pc))
            s.want('p', '}')
            rooms.append(r)
        return rooms

    def semi(s):
        if s.pk().k == 'p' and s.pk().v == ';':
            s.nx()

    def stmt(s):
        """Compile one statement; return its entry pc."""
        t = s.pk()
        if t.k == 'p' and t.v == '{':
            s.nx()
            at = len(s.p.code)
            while not (s.pk().k == 'p' and s.pk().v == '}'):
                s.stmt()
            s.nx(); s.semi()
            return at
        if t.k == 'str':
            s.nx(); at = s.p.emit('PRINT', s.p.sid(t.v)); s.semi(); return at
        if t.k == 'bq':
            s.nx(); at = s.p.emit('PROMPT', s.p.sid(t.v)); s.semi(); return at
        if t.k == 'id' and t.v == 'if':
            return s.ifstmt()
        if t.k == 'id' and s.pk(1).k == 'p' and s.pk(1).v == '=':
            s.nx(); s.nx(); val = s.nx()
            at = s.p.emit('SET', s.p.vid(t.v), VALS.get(val.v, 1))
            s.semi(); return at
        if t.k == 'id':
            s.nx()
            at = (s.p.emit('GETSTR') if t.v == 'getstr'
                  else s.p.emit('MOVE', s.p.rid(t.v)))
            s.semi(); return at
        sys.exit('math: line %d: unexpected %r' % (t.ln, t))

    def ifstmt(s):
        s.nx(); s.want('p', '(')
        lhs = s.nx()
        if lhs.k == 'id' and lhs.v == 'getarg':
            s.want('p', '('); s.want('num'); s.want('p', ')')
            op = s.want('op').v
            rhs = s.nx().v
            s.want('p', ')')
            # jump past the then-branch when the condition is false
            at = s.p.emit('JEQARG' if op == '!=' else 'JNEARG',
                          s.p.sid(rhs), 0)
            slot = at + 2
        else:
            op = s.want('op').v
            rhs = s.nx().v
            s.want('p', ')')
            if op != '==':
                sys.exit('math: only == is used on variables in this source')
            at = s.p.emit('JNEVAR', s.p.vid(lhs.v), VALS.get(rhs, 1), 0)
            slot = at + 3
        s.stmt()
        if s.pk().k == 'id' and s.pk().v == 'else':
            j = s.p.emit('JMP', 0)
            s.p.code[slot] = len(s.p.code)
            s.nx(); s.stmt()
            s.p.code[j + 1] = len(s.p.code)
        else:
            s.p.code[slot] = len(s.p.code)
        return at

# ------------------------------------------------------------- C emission

def cstr(x):
    out = []
    for ch in x:
        if ch == '"':    out.append('\\"')
        elif ch == '\\': out.append('\\\\')
        elif ch == '\n': out.append('\\n')
        elif 32 <= ord(ch) < 127: out.append(ch)
        else: out.append('\\%03o' % ord(ch))
    return '"' + ''.join(out) + '"'

def main():
    goto, sn = parse_goto(), parse_snames()
    ln_ = parse_numbered(rd('lnames'))
    d1  = parse_numbered(rd('indat.v1.txt'))
    d2  = parse_numbered(rd('indat.v2.txt'))
    notes = restore_dropped_lines(d1, d2)

    maxr = max(max(goto), max(sn))
    o = ['/* generated by gendata.py -- do not edit */']
    o.append('#define MARS_MAXROOM %d' % maxr)

    o.append('static const short mars_exits[MARS_MAXROOM+1][11] = {')
    for r in range(maxr + 1):
        ex = goto[r][1] if r in goto else [0] * 11
        o.append('  {%s},' % ','.join(str(x) for x in ex))
    o.append('};')

    def bitrow(name, pred):
        o.append('static const unsigned char %s[MARS_MAXROOM+1] = {' % name)
        o.append('  ' + ','.join('1' if pred(r) else '0'
                                 for r in range(maxr + 1)))
        o.append('};')

    bitrow('mars_brief',  lambda r: r in sn and sn[r][0][0] == '1')
    bitrow('mars_incity', lambda r: r in sn and sn[r][0][2] == '1')
    bitrow('mars_known',  lambda r: r in goto or r in sn)
    bitrow('mars_mapped', lambda r: r in goto)

    o.append('static const char *const mars_short[MARS_MAXROOM+1] = {')
    for r in range(maxr + 1):
        o.append('  %s,' % (cstr(sn[r][1]) if r in sn else '0'))
    o.append('};')

    o.append('static const char *const mars_long[MARS_MAXROOM+1] = {')
    for r in range(maxr + 1):
        o.append('  %s,' % (cstr('\n'.join(ln_[r])) if r in ln_ else '0'))
    o.append('};')

    nd = max(d2)
    o.append('#define DEI_MAXROOM %d' % nd)
    o.append('static const char *const dei_long[DEI_MAXROOM+1] = {')
    for r in range(nd + 1):
        o.append('  %s,' % (cstr('\n'.join(d2[r])) if r in d2 else '0'))
    o.append('};')
    rset = set(r for r, _, _ in notes)
    o.append('static const unsigned char dei_restored[DEI_MAXROOM+1] = {')
    o.append('  ' + ','.join('1' if r in rset else '0' for r in range(nd + 1)))
    o.append('};')

    # ---- the adventure language
    prog = Prog()
    mrooms = Parser(lex(rd('math.v2.txt')), prog).parse()
    defined = {r['name']: i for i, r in enumerate(mrooms)}
    for r in mrooms:
        prog.rid(r['name'])

    o.append('#define MATH_NROOM %d' % len(mrooms))
    o.append('#define MATH_NREF  %d' % len(prog.rooms))
    o.append('static const short math_ref[MATH_NREF] = {')
    o.append('  ' + ','.join(str(defined.get(n, -1)) for n in prog.rooms))
    o.append('};')
    o.append('static const char *const math_refname[MATH_NREF] = {')
    o.append('  ' + ','.join(cstr(n) for n in prog.rooms))
    o.append('};')
    o.append('#define MATH_NSTR %d' % max(1, len(prog.strs)))
    o.append('static const char *const math_str[MATH_NSTR] = {')
    for t in prog.strs or ['']:
        o.append('  %s,' % cstr(t))
    o.append('};')
    o.append('#define MATH_NVAR %d' % max(1, len(prog.vars)))
    o.append('static const char *const math_varname[MATH_NVAR] = {')
    o.append('  ' + ','.join(cstr(v) for v in (prog.vars or [''])))
    o.append('};')
    o.append('static const short math_code[%d] = {' % max(1, len(prog.code)))
    for i in range(0, len(prog.code), 16):
        o.append('  ' + ','.join(str(x) for x in prog.code[i:i + 16]) + ',')
    o.append('};')

    nclause = sum(len(r['clauses']) for r in mrooms)
    o.append('typedef struct { const char *w; short pc; } MClause;')
    o.append('static const MClause math_clause[%d] = {' % max(1, nclause))
    for r in mrooms:
        for keys, pc in r['clauses']:
            o.append('  {%s,%d},' % (cstr('|' + '|'.join(keys) + '|'), pc))
    o.append('};')
    o.append('typedef struct { const char *name,*lng,*brf; short c0,cn; } MRoom;')
    o.append('static const MRoom math_room[MATH_NROOM] = {')
    k = 0
    for r in mrooms:
        o.append('  {%s,%s,%s,%d,%d},' % (
            cstr(r['name']), cstr(r['long'] or ''),
            cstr(r['brief'] or ''), k, len(r['clauses'])))
        k += len(r['clauses'])
    o.append('};')

    with open(os.path.join(HERE, 'data.h'), 'w') as f:
        f.write('\n'.join(o) + '\n')

    # ---- report to stderr
    w = sys.stderr.write
    w('mars   : %d rooms with exits, %d short names, %d long descriptions\n'
      % (len(goto), len(sn), len(ln_)))
    miss = sorted(r for r in sn if r not in ln_ and sn[r][0][0] != '1')
    w('         listed as having a long description, but none was written: %s\n'
      % ','.join(map(str, miss)))
    tgt = set()
    for r in goto:
        tgt |= set(x for x in goto[r][1] if x)
    w('         exits point at rooms with no entry of their own: %s\n'
      % ','.join(map(str, sorted(t for t in tgt if t not in goto))))
    w('         has a short name but no exits: %s\n'
      % ','.join(map(str, sorted(set(sn) - set(goto)))))
    w('deimos : %d descriptions, no connection table survives\n' % len(d2))
    for rm, i, t in notes:
        w('         restored room %d line %d from indat.v1: %s\n'
          % (rm, i, t.strip()[:58]))
    w('math   : %d rooms, %d clauses, %d bytecode words\n'
      % (len(mrooms), nclause, len(prog.code)))
    und = [n for n in prog.rooms if n not in defined]
    w('         referenced but never defined: %s\n' % ','.join(und))
    w('         variables: %s\n' % ','.join(prog.vars))

main()
