"""Translate the HPL programs of NERO0366 (HP 9825 Adventure) to C.

usage: hpl2c.py LISTINGS_DIR out.c

The listings are what the 9825 printed (listings/ADVENT.txt, A1, A2, A3):
"n: statement;statement".  HPL has no procedures as such: cll 'Name'(...)
and gsb "Name" jump to a labelled line and come back at ret.  So each
program becomes

  - one C function per routine (every label reached by cll, gsb or a
    function reference), holding the lines reachable from that label
    until ret - routines share code by falling into one another ("Spk"
    into "Spkr" into "Echo"), and each gets its own copy;
  - one C function for the program's main line, entered at the line a
    chain names.

Each line becomes "L<n>:" and gto/jmp become goto, so the control flow is
the listing's own.  cll gives the routine a new set of p-numbers (p0 = the
number of parameters); gsb lends it the caller's, as HPL does.

Expressions follow HPL: implied multiplication (2p1, 100rnd(0), (1-p1)21),
"#" for "not equal", and/or without short-circuit, evaluated left to right,
"->" assignment usable inside expressions.

The few places where the port departs from the listing are in PATCHES
below; each asserts that it applied.
"""
import os
import re
import sys

PROGS = ['ADVENT', 'A1', 'A2', 'A3']

# ---------------------------------------------------------------------------
# lexer
# ---------------------------------------------------------------------------
KEYWORDS = sorted('''gto gsb jmp cll ret if for to by next wrt prt fmt red buf ent dsp beep
stp end chain asgn open rread sread sprt getk dim fxd mod and or not int abs rnd prnd pos
len cap itf fti max min bit type flg rds'''.split(), key=len, reverse=True)
FUNCS = {'int', 'abs', 'rnd', 'prnd', 'pos', 'len', 'cap', 'itf', 'fti', 'max', 'min', 'bit', 'type', 'flg', 'rds'}


class Tok:
    def __init__(self, kind, val):
        self.kind, self.val = kind, val

    def __repr__(self):
        return '%s:%r' % (self.kind, self.val)


def lex(s):
    toks, i = [], 0
    while i < len(s):
        c = s[i]
        if c == ' ':
            i += 1
        elif c == '"':
            j, out = i + 1, []
            while True:
                if j >= len(s):
                    raise SyntaxError('unterminated string in ' + s)
                if s[j] == '"':
                    if j + 1 < len(s) and s[j + 1] == '"':
                        out.append('"')
                        j += 2
                        continue
                    break
                out.append(s[j])
                j += 1
            toks.append(Tok('str', ''.join(out)))
            i = j + 1
        elif c == "'":
            j = s.index("'", i + 1)
            toks.append(Tok('ufn', s[i + 1:j]))
            i = j + 1
        elif c == '→':
            toks.append(Tok('op', '->'))
            i += 1
        elif s.startswith('<=', i) or s.startswith('>=', i):
            toks.append(Tok('op', s[i:i + 2]))
            i += 2
        elif c in '+-*/^&(),[]=#<>':
            toks.append(Tok('op', c))
            i += 1
        elif c == '{':
            toks.append(Tok('pi', None))
            i += 1
        elif c.isdigit() or (c == '.' and i + 1 < len(s) and s[i + 1].isdigit()):
            m = re.match(r'\d*\.?\d*(e[+-]?\d+)?', s[i:])
            txt = m.group(0)
            if txt.endswith('e'):
                txt = txt[:-1]
            toks.append(Tok('num', txt))
            i += len(txt)
        elif c.isupper():
            if i + 1 < len(s) and s[i + 1] == '$':
                toks.append(Tok('svar', c))
                i += 2
            else:
                toks.append(Tok('var', c))
                i += 1
        elif c.islower():
            if c in 'rp' and i + 1 < len(s) and s[i + 1].isdigit():
                m = re.match(r'\d+', s[i + 1:])
                toks.append(Tok('reg' if c == 'r' else 'pnum', int(m.group(0))))
                i += 1 + len(m.group(0))
                continue
            for k in KEYWORDS:
                if s.startswith(k, i):
                    toks.append(Tok('kw', k))
                    i += len(k)
                    break
            else:
                raise SyntaxError('unknown word at %r' % s[i:])
        else:
            raise SyntaxError('unexpected %r in %s' % (c, s))
    return toks


# ---------------------------------------------------------------------------
# parser
# ---------------------------------------------------------------------------
class Parser:
    def __init__(self, toks):
        self.t, self.i = toks, 0

    def peek(self, k=0):
        return self.t[self.i + k] if self.i + k < len(self.t) else None

    def at(self, kind, val=None, k=0):
        t = self.peek(k)
        return t is not None and t.kind == kind and (val is None or t.val == val)

    def take(self, kind=None, val=None):
        t = self.peek()
        if t is None or (kind and t.kind != kind) or (val is not None and t.val != val):
            raise SyntaxError('expected %s %s, got %r' % (kind, val, t))
        self.i += 1
        return t

    def done(self):
        return self.i >= len(self.t)

    # expressions -----------------------------------------------------------
    def expr(self):
        e = self.or_()
        lvs = []
        while self.at('op', '->'):
            self.take()
            lvs.append(self.lvalue())
        return ('assign', e, lvs) if lvs else e

    def or_(self):
        e = self.and_()
        while self.at('kw', 'or'):
            self.take()
            e = ('logic', 'or', e, self.and_())
        return e

    def and_(self):
        e = self.not_()
        while self.at('kw', 'and'):
            self.take()
            e = ('logic', 'and', e, self.not_())
        return e

    def not_(self):
        if self.at('kw', 'not'):
            self.take()
            return ('not', self.not_())
        return self.rel()

    def rel(self):
        e = self.add()
        if self.peek() and self.peek().kind == 'op' and self.peek().val in ('=', '#', '<', '>', '<=', '>='):
            op = self.take().val
            e = ('rel', op, e, self.add())
        return e

    def add(self):
        e = self.mul()
        while self.peek() and self.peek().kind == 'op' and self.peek().val in '+-&':
            op = self.take().val
            e = ('bin', op, e, self.mul())
        return e

    def starts_primary(self):
        t = self.peek()
        if t is None:
            return False
        if t.kind in ('num', 'var', 'reg', 'pnum', 'ufn', 'pi'):
            return True
        if t.kind == 'op' and t.val == '(':
            return True
        return t.kind == 'kw' and t.val in FUNCS

    def mul(self):
        e = self.unary()
        while True:
            t = self.peek()
            if t and ((t.kind == 'op' and t.val in '*/') or (t.kind == 'kw' and t.val == 'mod')):
                self.take()
                e = ('bin', '*' if t.val == '*' else '/' if t.val == '/' else 'mod', e, self.unary())
            elif self.starts_primary():
                e = ('bin', '*', e, self.power())          # implied multiplication
            else:
                return e

    def unary(self):
        if self.at('op', '-'):
            self.take()
            return ('neg', self.unary())
        if self.at('op', '+'):
            self.take()
            return self.unary()
        return self.power()

    def power(self):
        e = self.primary()
        if self.at('op', '^'):
            self.take()
            e = ('bin', '^', e, self.unary())
        return e

    def args(self):
        self.take('op', '(')
        a = [self.expr()]
        while self.at('op', ','):
            self.take()
            a.append(self.expr())
        self.take('op', ')')
        return a

    def subs(self):
        self.take('op', '[')
        a = [self.expr()]
        while self.at('op', ','):
            self.take()
            a.append(self.expr())
        self.take('op', ']')
        return a

    def primary(self):
        t = self.take()
        if t.kind == 'num':
            return ('num', t.val)
        if t.kind == 'str':
            return ('str', t.val)
        if t.kind == 'pi':
            return ('num', '3.14159265359')
        if t.kind == 'reg':
            return ('reg', t.val)
        if t.kind == 'pnum':
            return ('p', t.val)
        if t.kind == 'var':
            if self.at('op', '['):
                return ('aref', t.val, self.subs())
            return ('var', t.val)
        if t.kind == 'svar':
            return ('sref', t.val, self.subs() if self.at('op', '[') else [])
        if t.kind == 'ufn':
            return ('ufn', t.val, self.args() if self.at('op', '(') else [])
        if t.kind == 'op' and t.val == '(':
            e = self.expr()
            self.take('op', ')')
            return e
        if t.kind == 'kw' and t.val == 'flg':
            return ('flag', int(self.take('num').val))
        if t.kind == 'kw' and t.val in FUNCS:
            return ('fn', t.val, self.args())
        raise SyntaxError('unexpected %r' % t)

    def lvalue(self):
        t = self.take()
        if t.kind == 'var':
            if self.at('op', '['):
                return ('aref', t.val, self.subs())
            return ('var', t.val)
        if t.kind == 'svar':
            return ('sref', t.val, self.subs() if self.at('op', '[') else [])
        if t.kind == 'reg':
            return ('reg', t.val)
        if t.kind == 'pnum':
            return ('p', t.val)
        raise SyntaxError('not assignable: %r' % t)

    # io item: X[*], or an expression
    def ioitem(self):
        if self.at('var') and self.at('op', '[', 1) and self.at('op', '*', 2):
            name = self.take().val
            self.take()
            self.take()
            self.take('op', ']')
            return ('whole', name)
        return self.expr()

    def list_(self, item):
        a = [item()]
        while self.at('op', ','):
            self.take()
            a.append(item())
        return a


def split_top(s, sep):
    """Split on sep outside double quotes."""
    out, cur, q = [], [], False
    for ch in s:
        if ch == '"':
            q = not q
        if ch == sep and not q:
            out.append(''.join(cur))
            cur = []
        else:
            cur.append(ch)
    out.append(''.join(cur))
    return out


def parse_stmt(text):
    text = text.strip()
    if text.startswith('fmt'):
        parts = [x.strip() for x in split_top(text[3:], ',')]
        items = []
        for x in parts[1:]:
            if x.startswith('"'):
                items.append(('s', x[1:-1].replace('""', '"')))
            elif x == '/':
                items.append(('/', None))
            elif re.fullmatch(r'f\d*\.\d+', x):
                items.append(('f', x))
            elif re.fullmatch(r'c\d*', x):
                items.append(('c', x))
            else:
                raise SyntaxError('format item ' + x)
        return ('fmt', int(parts[0]), items)
    p = Parser(lex(text))
    if p.done():
        return ('nop',)
    t = p.peek()
    if t.kind == 'kw':
        k = t.val
        if k in ('gto', 'gsb'):
            p.take()
            if p.at('str'):
                st = (k, 'label', p.take().val)
            else:
                sign = 1
                if p.at('op', '+') or p.at('op', '-'):
                    sign = -1 if p.take().val == '-' else 1
                    st = (k, 'rel', sign * int(p.take('num').val))
                else:
                    st = (k, 'abs', int(p.take('num').val))
        elif k == 'jmp':
            p.take()
            st = ('jmp', p.expr())
        elif k == 'cll':
            p.take()
            name = p.take('ufn').val
            st = ('cll', name, p.args() if p.at('op', '(') else [])
        elif k == 'ret':
            p.take()
            st = ('ret', None if p.done() else p.expr())
        elif k == 'if':
            p.take()
            st = ('if', p.expr())
        elif k == 'for':
            p.take()
            var = p.lvalue()
            p.take('op', '=')
            a = p.or_()
            p.take('kw', 'to')
            b = p.or_()
            c = None
            if p.at('kw', 'by'):
                p.take()
                c = p.or_()
            st = ('for', var, a, b, c)
        elif k == 'next':
            p.take()
            st = ('next', p.lvalue())
        elif k in ('wrt', 'prt', 'dsp'):
            p.take()
            st = (k, [] if p.done() else p.list_(p.expr))
        elif k == 'red':
            p.take()
            buf = p.take('str').val
            p.take('op', ',')
            st = ('red', buf, p.lvalue())
        elif k == 'ent':
            p.take()
            prompt = p.expr()
            p.take('op', ',')
            st = ('ent', prompt, p.lvalue())
        elif k in ('beep', 'stp', 'end', 'buf', 'getk', 'fxd'):
            p.take()
            if k in ('buf', 'getk', 'fxd'):
                p.i = len(p.t)
            st = (k,)
        elif k == 'chain':
            p.take()
            name = p.take('str').val
            p.take('op', ',')
            l1 = int(p.take('num').val)
            p.take('op', ',')
            l2 = int(p.take('num').val)
            st = ('chain', name, l1, l2)
        elif k == 'asgn':
            p.take()
            name = p.take('str').val
            p.take('op', ',')
            f = p.expr()
            lv = None
            if p.at('op', ','):
                p.take()
                p.expr()
                p.take('op', ',')
                lv = p.lvalue()
            st = ('asgn', name, f, lv)
        elif k == 'open':
            p.take()
            name = p.take('str').val
            p.take('op', ',')
            st = ('open', name, p.expr())
        elif k in ('rread', 'sread', 'sprt'):
            p.take()
            items = p.list_(p.ioitem)
            st = (k, items)
        elif k == 'dim':
            p.take()
            st = ('dim', p.list_(p.lvalue))
        else:
            st = ('expr', p.expr())
    else:
        st = ('expr', p.expr())
    if not p.done():
        raise SyntaxError('trailing %r' % p.t[p.i:])
    return st


# ---------------------------------------------------------------------------
# programs
# ---------------------------------------------------------------------------
class Line:
    def __init__(self, n, label, texts):
        self.n, self.label, self.texts = n, label, texts
        self.stmts = []


class Prog:
    def __init__(self, idx, name, path):
        self.idx, self.name = idx, name
        self.lines = {}
        self.labels = {}
        for raw in open(path, encoding='utf-8'):
            raw = raw.rstrip('\n')
            m = re.match(r'(\d+): ?(.*)$', raw)
            if not m:
                continue
            n, text = int(m.group(1)), m.group(2)
            label = None
            lm = re.match(r'"([^"]*)":', text)
            if lm:
                label = lm.group(1)
                text = text[lm.end():]
                if label in self.labels:
                    raise SyntaxError('%s: label %s twice' % (name, label))
                self.labels[label] = n
            texts = [t for t in split_top(text, ';')]
            ln = Line(n, label, texts)
            for t in texts:
                try:
                    ln.stmts.append(parse_stmt(t))
                except SyntaxError as e:
                    raise SyntaxError('%s line %d: %s: %s' % (name, n, t, e))
            self.lines[n] = ln
        self.maxline = max(self.lines)
        # for/next pairs: next matches the nearest for of the same variable before it
        self.fors = {}      # (line, stmt) -> id
        self.for_next = {}  # for id -> (line, stmt) of its next
        self.next_for = {}  # (line, stmt) of next -> for id
        open_fors = []
        fid = 0
        for n in sorted(self.lines):
            for si, st in enumerate(self.lines[n].stmts):
                if st[0] == 'for':
                    fid += 1
                    self.fors[(n, si)] = fid
                    open_fors.append((fid, st[1], (n, si)))
                elif st[0] == 'next':
                    for k in range(len(open_fors) - 1, -1, -1):
                        if open_fors[k][1] == st[1]:
                            f = open_fors[k][0]
                            if f not in self.for_next:
                                self.for_next[f] = (n, si)
                            self.next_for[(n, si)] = f
                            break
                    else:
                        raise SyntaxError('%s line %d: next without for' % (name, n))
        self.for_at = {v: k for k, v in self.fors.items()}

    def label(self, name):
        if name not in self.labels:
            raise SyntaxError('%s: no label "%s"' % (self.name, name))
        return self.labels[name]

    def dispatch(self, n):
        """Lines a computed jmp at line n can land on: the table of single
        gto/stp lines that follows it."""
        out = []
        k = n + 1
        while k in self.lines and len(self.lines[k].stmts) == 1 and self.lines[k].stmts[0][0] in ('gto', 'stp') \
                and self.lines[k].label is None:
            out.append(k)
            k += 1
        if not out:
            raise SyntaxError('%s line %d: computed jmp without a table' % (self.name, n))
        return out


def walk_expr(e, fn):
    if isinstance(e, tuple):
        fn(e)
        for x in e[1:]:
            if isinstance(x, (tuple, list)):
                walk_expr(x, fn)
    elif isinstance(e, list):
        for x in e:
            walk_expr(x, fn)


def successors(prog, n, calls):
    ln = prog.lines[n]
    out = []
    for si, st in enumerate(ln.stmts):
        walk_expr(list(st[1:]), lambda e: calls.add(e[1]) if e[0] == 'ufn' else None)
        k = st[0]
        if k == 'cll':
            calls.add(st[1])
        if k == 'gsb':
            if st[1] != 'label':
                raise SyntaxError('gsb to a line number')
            calls.add(st[2])
        if k == 'if':
            out.append(n + 1)
        elif k == 'gto':
            out.append(goto_target(prog, n, st))
            return out
        elif k == 'jmp':
            e = st[1]
            if e[0] == 'num':
                out.append(n + int(e[1]))
            else:
                out.extend(prog.dispatch(n))
            return out
        elif k in ('ret', 'stp', 'end', 'chain'):
            return out
        elif k == 'for':
            f = prog.fors[(n, si)]
            if f not in prog.for_next:
                raise SyntaxError('%s line %d: for without next' % (prog.name, n))
            out.append(prog.for_next[f][0])
        elif k == 'next':
            out.append(prog.for_at[prog.next_for[(n, si)]][0])
    out.append(n + 1)
    return out


def goto_target(prog, n, st):
    if st[1] == 'label':
        return prog.label(st[2])
    if st[1] == 'rel':
        return n + st[2]
    return st[2]


def reach(prog, entries):
    seen, calls, work = set(), set(), list(entries)
    while work:
        n = work.pop()
        if n in seen:
            continue
        if n not in prog.lines:
            raise SyntaxError('%s: control reaches missing line %d' % (prog.name, n))
        seen.add(n)
        work.extend(successors(prog, n, calls))
    return seen, calls


# ---------------------------------------------------------------------------
# the port's departures from the listing
# ---------------------------------------------------------------------------
# (program, line, statement text) -> (kind, C code); each must apply
PATCHES = [
    # The 9825 prints the player's command on its output device after
    # reading it from its own keyboard; a console has shown it already.
    ('A1', 85, "cll 'Echo'(0,0)", 'guard', 'rt_echo()'),
    ('A2', 112, "cll 'Echo'(0,0)", 'guard', 'rt_echo()'),
    ('A3', 92, "cll 'Echo'(0,0)", 'guard', 'rt_echo()'),
    # Restoring a suspended game: the console stays the output device at
    # the width asked for now.  Fix: Readrest leaves T[] holding the first
    # 32 travel entries (from SEC3H) but Q, the travel block T is supposed
    # to hold, as it was saved, so unless that was block 1 the exits of
    # the first locations visited go wrong until the game moves to another
    # block.  A new game clears Q (line 36); the fix does it here too.
    ('ADVENT', 26, "cll 'Readrest'", 'after', 'V_S = 11; V_Y = opt_width; if (opt_fixes) V_Q = 0;'),
    # "Print" holds each line of a message read from a data file (p2 is
    # the link to the next record) and joins the next one to it, so the
    # text is refilled to the printer's width.  The lines were laid out
    # by hand (hyphenated words, centred credits), so unless --reflow is
    # given or the width is too small for them, each is printed as it is.
    ('A1', 46, "if p2#0 and len(M$)<Y", 'before', 'if (!rt_reflow()) goto L47;'),
    ('A2', 72, "if p2#0 and len(M$)<Y", 'before', 'if (!rt_reflow()) goto L73;'),
    ('A3', 52, "if p2#0 and len(M$)<Y", 'before', 'if (!rt_reflow()) goto L53;'),
]


# ---------------------------------------------------------------------------
# code generation
# ---------------------------------------------------------------------------
def cstr(s):
    b = s.encode('latin-1')
    out = []
    for ch in b:
        if ch in (0x22, 0x5c, 0x3f) or ch < 0x20 or ch > 0x7e:
            out.append('\\%03o' % ch)
        else:
            out.append(chr(ch))
    return '"' + ''.join(out) + '"', len(b)


def cnum(txt):
    if txt.startswith('.'):
        txt = '0' + txt
    if re.fullmatch(r'\d+', txt):
        return txt + '.0'
    return txt


class Gen:
    def __init__(self, progs, string_arrays):
        self.progs = progs
        self.sarr = string_arrays
        self.tmp = 0
        self.patched = set()

    def t(self):
        self.tmp += 1
        return '_t%d' % self.tmp

    # types and purity --------------------------------------------------------
    def typ(self, e):
        k = e[0]
        if k == 'str' or k == 'sref':
            return 's'
        if k == 'fn' and e[1] in ('cap', 'fti'):
            return 's'
        if k == 'bin' and e[1] == '&':
            return 's'
        if k == 'assign':
            return self.typ(e[1])
        return 'n'

    def impure(self, e):
        found = []

        def f(x):
            if x[0] in ('ufn', 'assign') or (x[0] == 'fn' and x[1] == 'rnd'):
                found.append(1)
        walk_expr(e, f)
        return bool(found)

    # expressions -------------------------------------------------------------
    def seq(self, parts, combine):
        """Evaluate parts (list of (expr, type)) left to right, then combine(names)."""
        if not any(self.impure(e) for e, _ in parts) or len(parts) < 2:
            return combine([self.ex(e) for e, _ in parts])
        names, decls = [], []
        for e, ty in parts:
            nm = self.t()
            decls.append('%s %s = %s;' % ('double' if ty == 'n' else 'SV', nm, self.ex(e)))
            names.append(nm)
        return '({ %s %s; })' % (' '.join(decls), combine(names))

    def subs3(self, subs):
        s = [x for x in subs] + [('num', '0')] * (3 - len(subs))
        return s

    def ex(self, e):
        k = e[0]
        if k == 'num':
            return cnum(e[1])
        if k == 'str':
            s, n = cstr(e[1])
            return 'rt_lit(%s, %d)' % (s, n)
        if k == 'var':
            return 'V_%s' % e[1]
        if k == 'reg':
            return 'R[%d]' % e[1]
        if k == 'p':
            return 'p[%d]' % e[1]
        if k == 'flag':
            if e[1] != 13:
                raise SyntaxError('flag %d' % e[1])
            return 'flg13'
        if k == 'aref':
            if len(e[2]) != 1:
                raise SyntaxError('2-d numeric array')
            return '(*rt_aptr(&NA_%s, %s))' % (e[1], self.ex(e[2][0]))
        if k == 'sref':
            subs = e[2]
            parts = [(x, 'n') for x in self.subs3(subs)]
            return self.seq(parts, lambda a: 'rt_sget(&S_%s, %d, %s)' % (e[1], len(subs), ', '.join(a)))
        if k == 'neg':
            return '(-%s)' % self.ex(e[1])
        if k == 'not':
            return '(%s == 0 ? 1.0 : 0.0)' % self.ex(e[1])
        if k == 'logic':
            op = '&&' if e[1] == 'and' else '||'
            return self.seq([(e[2], 'n'), (e[3], 'n')],
                            lambda a: '((%s != 0) %s (%s != 0) ? 1.0 : 0.0)' % (a[0], op, a[1]))
        if k == 'rel':
            op = {'=': '==', '#': '!='}.get(e[1], e[1])
            if self.typ(e[2]) == 's':
                if self.typ(e[3]) != 's':
                    raise SyntaxError('string compared with number')
                return self.seq([(e[2], 's'), (e[3], 's')],
                                lambda a: '(rt_scmp(%s, %s) %s 0 ? 1.0 : 0.0)' % (a[0], a[1], op))
            return self.seq([(e[2], 'n'), (e[3], 'n')], lambda a: '(%s %s %s ? 1.0 : 0.0)' % (a[0], op, a[1]))
        if k == 'bin':
            op = e[1]
            if op == '&':
                return self.seq([(e[2], 's'), (e[3], 's')], lambda a: 'rt_cat(%s, %s)' % (a[0], a[1]))
            if op == 'mod':
                return self.seq([(e[2], 'n'), (e[3], 'n')], lambda a: 'rt_mod(%s, %s)' % (a[0], a[1]))
            if op == '^':
                return self.seq([(e[2], 'n'), (e[3], 'n')], lambda a: 'pow(%s, %s)' % (a[0], a[1]))
            return self.seq([(e[2], 'n'), (e[3], 'n')], lambda a: '(%s %s %s)' % (a[0], op, a[1]))
        if k == 'fn':
            name, args = e[1], e[2]
            sig = {'int': ('n',), 'abs': ('n',), 'rnd': ('n',), 'prnd': ('n', 'n'), 'pos': ('s', 's'),
                   'len': ('s',), 'cap': ('s',), 'itf': ('s',), 'fti': ('n',), 'max': ('n', 'n'),
                   'min': ('n', 'n'), 'bit': ('n', 'n'), 'type': ('n',), 'rds': ('s',)}[name]
            if len(args) != len(sig):
                raise SyntaxError('%s takes %d arguments' % (name, len(sig)))
            for a, ty in zip(args, sig):
                if self.typ(a) != ty:
                    raise SyntaxError('%s: argument type' % name)
            if name == 'rds':
                return 'rt_rds()'
            cfn = {'int': 'rt_int', 'abs': 'fabs', 'rnd': 'rt_rnd', 'prnd': 'rt_prnd', 'pos': 'rt_pos',
                   'len': 'rt_len', 'cap': 'rt_cap', 'itf': 'rt_itf', 'fti': 'rt_fti', 'max': 'rt_max',
                   'min': 'rt_min', 'bit': 'rt_bit', 'type': 'rt_type'}[name]
            return self.seq(list(zip(args, sig)), lambda a: '%s(%s)' % (cfn, ', '.join(a)))
        if k == 'ufn':
            return self.call(e[1], e[2])
        if k == 'assign':
            return self.assign(e[1], e[2])
        raise SyntaxError('cannot generate %r' % (e,))

    def call(self, name, args, frame=None):
        fn = self.routine_name(self.cur, name)
        f = self.t()
        body = ['double %s[16] = {0};' % f, '%s[0] = %d;' % (f, len(args))]
        for i, a in enumerate(args):
            if self.typ(a) != 'n':
                raise SyntaxError('string parameter')
            body.append('%s[%d] = %s;' % (f, i + 1, self.ex(a)))
        body.append('%s(%s);' % (fn, f))
        return '({ %s })' % ' '.join(body)

    def store(self, lv, val):
        k = lv[0]
        if k == 'var':
            return 'V_%s = %s;' % (lv[1], val)
        if k == 'reg':
            return 'R[%d] = %s;' % (lv[1], val)
        if k == 'p':
            return 'p[%d] = %s;' % (lv[1], val)
        if k == 'aref':
            return '*rt_aptr(&NA_%s, %s) = %s;' % (lv[1], self.ex(lv[2][0]), val)
        if k == 'sref':
            subs = lv[2]
            parts = [(x, 'n') for x in self.subs3(subs)]
            return self.seq(parts, lambda a: 'rt_sset(&S_%s, %d, %s, %s)' % (lv[1], len(subs), ', '.join(a), val)) + ';'
        raise SyntaxError('store to %r' % (lv,))

    def assign(self, e, lvs):
        ty = self.typ(e)
        for lv in lvs:
            if (lv[0] == 'sref') != (ty == 's'):
                raise SyntaxError('assignment type mismatch')
        nm = self.t()
        decl = '%s %s = %s;' % ('double' if ty == 'n' else 'SV', nm, self.ex(e))
        return '({ %s %s %s; })' % (decl, ' '.join(self.store(lv, nm) for lv in lvs), nm)

    def numptr(self, lv):
        k = lv[0]
        if k == 'var':
            return '&V_%s' % lv[1]
        if k == 'reg':
            return '&R[%d]' % lv[1]
        if k == 'p':
            return '&p[%d]' % lv[1]
        if k == 'aref':
            return 'rt_aptr(&NA_%s, %s)' % (lv[1], self.ex(lv[2][0]))
        raise SyntaxError('not a numeric variable: %r' % (lv,))

    def strtarget(self, lv):
        subs = lv[2]
        s3 = self.subs3(subs)
        return '&S_%s, %d, %s' % (lv[1], len(subs), ', '.join(self.ex(x) for x in s3))

    # statements -------------------------------------------------------------
    def routine_name(self, prog, label):
        n = prog.label(label)
        return 'r_%s_%d' % (prog.name, n)

    def stmts(self, prog, n, func):
        ln = prog.lines[n]
        out = []
        for si, st in enumerate(ln.stmts):
            text = ln.texts[si].strip()
            guard, before, after = None, None, None
            for pp in PATCHES:
                if pp[0] == prog.name and pp[1] == n and pp[2] == text:
                    self.patched.add(pp[:3])
                    if pp[3] == 'guard':
                        guard = pp[4]
                    elif pp[3] == 'before':
                        before = pp[4]
                    else:
                        after = pp[4]
            code = self.stmt(prog, n, si, st, func)
            if guard:
                code = 'if (%s) { %s }' % (guard, code)
            if before:
                out.append('    ' + before + '  /* port */')
            shown = text.replace('*/', '* /').replace('→', '->')
            out.append('    ' + code + ('  /* %s */' % shown if text else ''))
            if after:
                out.append('    ' + after + '  /* port */')
        return out

    def stmt(self, prog, n, si, st, func):
        k = st[0]
        if k == 'nop':
            return ';'
        if k == 'gto':
            return 'goto L%d;' % goto_target(prog, n, st)
        if k == 'gsb':
            return '%s(p);' % self.routine_name(prog, st[2])
        if k == 'jmp':
            e = st[1]
            if e[0] == 'num':
                return 'goto L%d;' % (n + int(e[1]))
            cases = ' '.join('case %d: goto L%d;' % (t, t) for t in prog.dispatch(n))
            return '{ long _j = %d + rt_round(%s); switch (_j) { %s default: rt_fail("jmp outside its table", %d, %d); } }' % (
                n, self.ex(e), cases, prog.idx, n)
        if k == 'cll':
            return '(void)%s;' % self.call(st[1], st[2])
        if k == 'ret':
            if func == 'main':
                return 'rt_fail("ret in the main program", %d, %d);' % (prog.idx, n)
            return 'return %s;' % (self.ex(st[1]) if st[1] else '0')
        if k == 'if':
            return 'if (%s == 0) goto L%d;' % (self.ex(st[1]), n + 1)
        if k == 'for':
            f = prog.fors[(n, si)]
            var, a, b, c = st[1], st[2], st[3], st[4]
            v = self.ex(var)
            return '%s _lim%d = %s; _stp%d = %s; if (_stp%d >= 0 ? %s > _lim%d : %s < _lim%d) goto F%d_x; F%d_b:;' % (
                self.store(var, self.ex(a)), f, self.ex(b), f, self.ex(c) if c else '1.0', f, v, f, v, f, f, f)
        if k == 'next':
            f = prog.next_for[(n, si)]
            v = self.ex(st[1])
            lab = 'F%d_x:;' % f if prog.for_next[f] == (n, si) else ''
            return '%s += _stp%d; if (_stp%d >= 0 ? %s <= _lim%d : %s >= _lim%d) goto F%d_b; %s' % (
                v, f, f, v, f, v, f, f, lab)
        if k in ('wrt', 'prt'):
            args = st[1]
            if k == 'wrt' and args and args[0][0] == 'str':
                m = re.fullmatch(r'M\.(\d+)', args[0][1])
                if not m:
                    raise SyntaxError('wrt to %s' % args[0][1])
                code = ['rt_wrt_begin(%s);' % m.group(1)]
                for a in args[1:]:
                    code.append('rt_wrt_%s(%s);' % ('str' if self.typ(a) == 's' else 'num', self.ex(a)))
                code.append('rt_wrt_end();')
                return ' '.join(code)
            items = args[1:] if k == 'wrt' else args
            if not items:
                return 'rt_out_blank();'
            if len(items) != 1 or self.typ(items[0]) != 's':
                raise SyntaxError('%s of other than one string' % k)
            return 'rt_out(%s);' % self.ex(items[0])
        if k == 'dsp':
            return 'rt_dsp(%s);' % self.ex(st[1][0])
        if k == 'fmt':
            items = []
            for kind, val in st[2]:
                if kind == 's':
                    items.append("{'s', %s}" % cstr(val)[0])
                else:
                    items.append("{'%s', 0}" % kind)
            items.append('{0, 0}')
            return '{ static const FmtItem _f[] = {%s}; rt_fmt(%d, _f); }' % (', '.join(items), st[1])
        if k == 'red':
            if st[1] != 'M' or st[2] != ('sref', 'M', []):
                raise SyntaxError('red other than buffer M into M$')
            return 'rt_red(&S_M);'
        if k == 'ent':
            lv = st[2]
            if lv[0] == 'sref':
                return 'rt_ent_str(%s, %s);' % (self.ex(st[1]), self.strtarget(lv))
            return 'rt_ent_num(%s, %s);' % (self.ex(st[1]), self.numptr(lv))
        if k in ('beep', 'buf', 'getk', 'fxd'):
            return ';'
        if k in ('stp', 'end'):
            return 'rt_stop();'
        if k == 'chain':
            if st[2] != 0:
                raise SyntaxError('chain of part of a program')
            return 'rt_chain(%d, %d);' % (PROGS.index(st[1]), st[3])
        if k == 'asgn':
            call = 'rt_asgn("%s", %s)' % (st[1], self.ex(st[2]))
            if st[3]:
                return self.store(st[3], call)
            return 'if (%s) rt_fail("no file %s", %d, %d);' % (call, st[1], prog.idx, n)
        if k == 'open':
            return 'rt_open("%s", %s);' % (st[1], self.ex(st[2]))
        if k in ('rread', 'sread'):
            items = st[1]
            fexpr = self.ex(items[0])
            code = []
            if k == 'rread':
                code.append('rt_rread(%s, %s);' % (fexpr, self.ex(items[1])))
                items = items[2:]
            else:
                items = items[1:]
            for it in items:
                if it[0] == 'whole':
                    code.append('rt_rd_narr(%s, &NA_%s);' % (fexpr, it[1]))
                elif it[0] == 'sref' and not it[2] and it[1] in self.sarr:
                    code.append('rt_rd_sarr(%s, &S_%s);' % (fexpr, it[1]))
                elif it[0] == 'sref':
                    code.append('rt_rd_str(%s, %s);' % (fexpr, self.strtarget(it)))
                else:
                    code.append('rt_rd_num(%s, %s);' % (fexpr, self.numptr(it)))
            return ' '.join(code)
        if k == 'sprt':
            items = st[1]
            fexpr = self.ex(items[0])
            code = []
            for it in items[1:]:
                if it == ('str', 'end'):
                    code.append('rt_wr_eof(%s);' % fexpr)
                elif it[0] == 'whole':
                    code.append('rt_wr_narr(%s, &NA_%s);' % (fexpr, it[1]))
                elif it[0] == 'sref' and not it[2] and it[1] in self.sarr:
                    code.append('rt_wr_sarr(%s, &S_%s);' % (fexpr, it[1]))
                elif self.typ(it) == 's':
                    code.append('rt_wr_str(%s, %s);' % (fexpr, self.ex(it)))
                else:
                    code.append('rt_wr_num(%s, %s);' % (fexpr, self.ex(it)))
            code.append('rt_wr_done(%s);' % fexpr)
            return ' '.join(code)
        if k == 'dim':
            code = []
            for it in st[1]:
                if it[0] == 'sref':
                    if len(it[2]) == 1:
                        code.append('rt_sdim(&S_%s, -1, %s);' % (it[1], self.ex(it[2][0])))
                    else:
                        code.append('rt_sdim(&S_%s, %s, %s);' % (it[1], self.ex(it[2][0]), self.ex(it[2][1])))
                elif it[0] == 'aref':
                    code.append('rt_ndim(&NA_%s, %s);' % (it[1], self.ex(it[2][0])))
            return ' '.join(code) or ';'
        if k == 'expr':
            return '(void)%s;' % self.ex(st[1])
        raise SyntaxError('statement %r' % (st,))

    def function(self, prog, kind, entry_lines, name):
        self.cur = prog
        lines, calls = reach(prog, entry_lines)
        fors = sorted({prog.fors[(n, si)] for n in lines for si, st in enumerate(prog.lines[n].stmts) if st[0] == 'for'})
        out = []
        if kind == 'main':
            out.append('static void %s(int entry)\n{' % name)
            out.append('    double *p = pmain;')
        else:
            out.append('static double %s(double *p)\n{' % name)
        out.append('    size_t _fm = rt_mark();')
        for f in fors:
            out.append('    double _lim%d, _stp%d;' % (f, f))
        if kind == 'main':
            cases = ' '.join('case %d: goto L%d;' % (n, n) for n in sorted(entry_lines))
            out.append('    switch (entry) { %s default: rt_fail("no such entry", %d, entry); }' % (cases, prog.idx))
        else:
            out.append('    goto L%d;' % entry_lines[0])
        for n in sorted(lines):
            ln = prog.lines[n]
            out.append('L%d: rt_prog = %d; rt_line = %d; rt_release(_fm);%s' % (
                n, prog.idx, n, '  /* "%s" */' % ln.label if ln.label else ''))
            out.extend(self.stmts(prog, n, kind))
        out.append('    rt_fail("ran off the end of a routine", %d, -1);' % prog.idx)
        if kind != 'main':
            out.append('    return 0;')
        out.append('}')
        return '\n'.join(out), calls


def main():
    src, outpath = sys.argv[1], sys.argv[2]
    progs = [Prog(i, name, os.path.join(src, name + '.txt')) for i, name in enumerate(PROGS)]
    # string arrays: dimensioned with two sizes
    sarr = set()
    for pr in progs:
        for ln in pr.lines.values():
            for st in ln.stmts:
                if st[0] == 'dim':
                    for it in st[1]:
                        if it[0] == 'sref' and len(it[2]) == 2:
                            sarr.add(it[1])
    # entries of each main program: the lines chains go to
    entries = {0: {0}}
    for pr in progs:
        for ln in pr.lines.values():
            for st in ln.stmts:
                if st[0] == 'chain':
                    entries.setdefault(PROGS.index(st[1]), set()).add(st[3])
    g = Gen(progs, sarr)
    funcs, protos = [], []
    for pr in progs:
        g.cur = pr
        code, calls = g.function(pr, 'main', sorted(entries.get(pr.idx, set())), 'm_%s' % pr.name)
        funcs.append(code)
        done = set()
        todo = sorted(calls)
        while todo:
            lab = todo.pop()
            if lab in done:
                continue
            done.add(lab)
            nm = g.routine_name(pr, lab)
            code, more = g.function(pr, 'routine', [pr.label(lab)], nm)
            funcs.append(code)
            protos.append('static double %s(double *p);  /* %s "%s" */' % (nm, pr.name, lab))
            todo.extend(sorted(more - done))
    for pp in PATCHES:
        if pp[:3] not in g.patched:
            raise SystemExit('patch did not apply: %r' % (pp[:3],))
    out = ['/* generated by hpl2c.py from the NERO0366 listings - do not edit */',
           '#include "rt.h"', '#include <math.h>', '#include <string.h>', '',
           'int rt_prog, rt_line;',
           'const char *const game_progs[] = {%s};' % ', '.join('"%s"' % p for p in PROGS),
           'static double pmain[16];',
           'static double R[64];',
           'static double %s;' % ', '.join('V_%s' % c for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'),
           'static NArr %s;' % ', '.join('NA_%s' % c for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'),
           'static HSVar %s;' % ', '.join('S_%s' % c for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'),
           '']
    out += protos
    out.append('')
    out += funcs
    out.append('void game_run(int prog, int line)\n{\n    switch (prog) {')
    for pr in progs:
        if pr.idx in entries:
            out.append('    case %d: m_%s(line); break;' % (pr.idx, pr.name))
    out.append('    default: rt_fail("no such program", prog, line);\n    }\n}')
    with open(outpath, 'w', encoding='ascii', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    print('hpl2c: %d functions' % len(funcs))


if __name__ == '__main__':
    main()
