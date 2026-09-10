#!/usr/bin/env python3
"""
gendata.py -- plays the part of Mark Niemiec's lost "F" compiler.

Reads the recovered archive sources
    ../recovered/adv.f     (vocab block: every symbol and its ordinal)
    ../recovered/loc.f     (locexec(): the 143 room blocks)
and writes data.h, the room/exit tables that newadv.c walks.

Nothing here is invented.  Where a room's behaviour is a conditional the
original source is carried through verbatim into data.h so the walker can
show it instead of guessing at it.
"""

import re
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'recovered')

# The eight compass points, in the order the source itself proves.  vehexec()'s
# raft code does
#       .X += which(fn-N, 0,1,1,1,0,-1,-1,-1);
#       .Y += which(fn-N, -1,-1,0,1,1,1,0,-1);
# which is dx/dy for N,NE,E,SE,S,SW,W,NW with y increasing southward.  That
# fixes both the order and the origin of the range label "N::NW".
COMPASS = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']

# Motion words that are not compass points.  These live in the lost
# games/f/include/vocab; the names are read off the call sites, not invented.
OTHER_DIRS = ['U', 'D', 'IN', 'OUT', 'BACK', 'CLIMB', 'JUMP', 'ENTER']


# --------------------------------------------------------------------------
# tokeniser
# --------------------------------------------------------------------------

def tokenize(src):
    """-> list of (kind, text, start, end); kinds: id num str punct"""
    toks = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c in ' \t\r\n':
            i += 1
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            i = n if j < 0 else j + 2
        elif c == '"':
            j = i + 1
            while j < n and src[j] != '"':
                j += 2 if src[j] == '\\' else 1
            toks.append(('str', src[i:j + 1], i, j + 1))
            i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and src[j] != "'":
                j += 2 if src[j] == '\\' else 1
            toks.append(('num', src[i:j + 1], i, j + 1))
            i = j + 1
        elif c.isalpha() or c == '_':
            j = i
            while j < n and (src[j].isalnum() or src[j] == '_'):
                j += 1
            toks.append(('id', src[i:j], i, j))
            i = j
        elif c.isdigit():
            j = i
            while j < n and src[j].isalnum():
                j += 1
            toks.append(('num', src[i:j], i, j))
            i = j
        elif c == '#':                       # #text / #include directive line
            j = src.find('\n', i)
            j = n if j < 0 else j
            toks.append(('punct', '#directive', i, j))
            i = j
        else:
            toks.append(('punct', c, i, i + 1))
            i += 1
    return toks


def unstring(lit):
    """F string literal -> text.  Source line breaks inside a literal are
    just wrapping and collapse to one space; \\n is a real newline."""
    body = lit[1:-1]
    body = re.sub(r'\n[ \t]*', ' ', body)
    out, i = [], 0
    while i < len(body):
        if body[i] == '\\' and i + 1 < len(body):
            c = body[i + 1]
            out.append({'n': '\n', 't': '\t', '\\': '\\', '"': '"', "'": "'",
                        '0': '\0', 'r': '\r'}.get(c, c))
            i += 2
        else:
            out.append(body[i])
            i += 1
    return ''.join(out)


# --------------------------------------------------------------------------
# adv.f: the vocab block
# --------------------------------------------------------------------------

def parse_vocab(path):
    src = open(path).read()
    i = src.index('vocab {')
    depth, j = 0, i + 6
    while True:
        if src[j] == '{':
            depth += 1
        elif src[j] == '}':
            depth -= 1
            if depth == 0:
                break
        j += 1
    body = re.sub(r'/\*.*?\*/', ' ', src[i + 7:j], flags=re.S)
    body = re.sub(r'^[ \t]*#.*$', '', body, flags=re.M)      # drop directives

    val, syms, words = 0, {}, {}
    for entry in body.split(';'):
        entry = entry.strip()
        if not entry:
            continue
        for part in (p.strip() for p in entry.split('=')):
            if not part:
                continue
            if part.startswith('"'):
                words.setdefault(val, []).append(part[1:-1])
            else:
                syms[part] = val
        val += 1
    return syms, words


# --------------------------------------------------------------------------
# loc.f: locexec()
# --------------------------------------------------------------------------

def balanced(toks, i):
    """toks[i] is '{'; return index just past its matching '}'."""
    depth = 0
    while i < len(toks):
        if toks[i][1] == '{':
            depth += 1
        elif toks[i][1] == '}':
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise SystemExit('unbalanced braces')


def read_labels(toks, i):
    """Read a run of stacked labels at toks[i].  -> (labels, i) or (None, i).
    A label is IDENT ':' , or IDENT '::' IDENT ':' for a range."""
    labels = []
    while (i + 1 < len(toks) and toks[i][0] == 'id'
           and toks[i + 1][1] == ':'):
        name = toks[i][1]
        if (i + 2 < len(toks) and toks[i + 2][1] == ':'
                and i + 4 < len(toks) and toks[i + 3][0] == 'id'
                and toks[i + 4][1] == ':'):
            labels.append((name, toks[i + 3][1]))       # range label
            i += 5
        else:
            labels.append((name, None))
            i += 2
    return (labels or None), i


def split_labelled(toks):
    """Split a block body into [(labels, body_tokens)]."""
    out, i = [], 0
    while i < len(toks):
        labels, j = read_labels(toks, i)
        if labels is None:                     # stray statement before a label
            labels, j = [('<pre>', None)], i
        body, depth = [], 0
        i = j
        while i < len(toks):
            t = toks[i][1]
            if t in '({[':
                depth += 1
            elif t in ')}]':
                depth -= 1
            if depth == 0:
                # a new label may only start a fresh statement
                prev = body[-1][1] if body else None
                if prev in (None, ';', '{', '}'):
                    nl, _ = read_labels(toks, i)
                    if nl is not None:
                        break
            body.append(toks[i])
            i += 1
        out.append((labels, body))
    return out


def call_args(toks, i):
    """toks[i] is '('; -> (list of arg token-lists, index past ')')."""
    args, cur, depth = [], [], 0
    while i < len(toks):
        t = toks[i][1]
        if t in '([':
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif t in ')]':
            depth -= 1
            if depth == 0:
                if cur:
                    args.append(cur)
                return args, i + 1
        if depth == 1 and t == ',':
            args.append(cur)
            cur = []
        else:
            cur.append(toks[i])
        i += 1
    raise SystemExit('unterminated call')


# --------------------------------------------------------------------------
# A very small F evaluator.
#
# Two places in loc.f compute a destination instead of writing it down:
# TREET_0::TREET_11 does travel(D,OUT, loc-TREET_0+TREE_0), and the mallorn
# grove TREE_0::TREE_11 fills travtab[] itself with a which() lookup.  Rather
# than transcribe those maps by hand, re-execute the source's own arithmetic.
#
# Values are either ints or ('sym', name) for a symbol whose ordinal is not
# recoverable (the motion words live in the lost games/f/include/vocab).  A
# symbol may be stored whole; using one in arithmetic is an error, which is
# what keeps this from quietly inventing anything.
# --------------------------------------------------------------------------

class Unknown(Exception):
    pass


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


class Eval:
    def __init__(self, toks, syms, env):
        self.t, self.i, self.syms, self.env = toks, 0, syms, env

    def peek(self, k=0):
        return self.t[self.i + k][1] if self.i + k < len(self.t) else None

    def take(self, what=None):
        if what is not None and self.peek() != what:
            raise Unknown('expected %r got %r' % (what, self.peek()))
        v = self.t[self.i]
        self.i += 1
        return v

    @staticmethod
    def num(v):
        if isinstance(v, tuple):
            raise Unknown('symbol %s in arithmetic' % v[1])
        return v

    def primary(self):
        k, s = self.t[self.i][0], self.peek()
        if s == '(':
            self.take('(')
            v = self.expr()
            self.take(')')
            return v
        if s == '-':
            self.take()
            return -self.num(self.primary())
        if s == '!':
            self.take()
            return 0 if self.truth(self.primary()) else 1
        if s == '+':
            self.take()
            return self.primary()
        if k == 'num':
            self.take()
            return int(s, 8) if s.startswith('0') and len(s) > 1 else int(s)
        if k == 'id':
            self.take()
            if s == 'which':                      # which(n, a0, a1, ...)
                self.take('(')
                args, depth, cur = [], 1, []
                while depth:
                    tok = self.t[self.i]
                    if tok[1] in '([':
                        depth += 1
                    elif tok[1] in ')]':
                        depth -= 1
                        if depth == 0:
                            args.append(cur)
                            self.i += 1
                            break
                    if depth == 1 and tok[1] == ',':
                        args.append(cur)
                        cur = []
                    else:
                        cur.append(tok)
                    self.i += 1
                n = self.num(Eval(args[0], self.syms, self.env).expr())
                table = [Eval(a, self.syms, self.env).expr() for a in args[1:]]
                return table[n] if 0 <= n < len(table) else 0
            if s in self.env:
                return self.env[s]
            if s in self.syms:
                return self.syms[s]
            return ('sym', s)
        raise Unknown('token %r' % s)

    def truth(self, v):
        return v != 0 if not isinstance(v, tuple) else True

    def term(self):                       # * / % bind tighter than + -
        v = self.primary()
        while self.peek() in ('*', '/', '%'):
            op = self.take()[1]
            a, b = self.num(v), self.num(self.primary())
            v = (a * b if op == '*' else trunc_div(a, b) if op == '/'
                 else a - trunc_div(a, b) * b)
        return v

    def add(self):
        v = self.term()
        while self.peek() in ('+', '-'):
            op = self.take()[1]
            a, b = self.num(v), self.num(self.term())
            v = a + b if op == '+' else a - b
        return v

    def expr(self):
        v = self.add()
        while self.peek() in ('==', '!=', '<', '>', '<=', '>='):
            op = self.take()[1]
            r = self.add()
            if op in ('==', '!='):
                v = 1 if ((v == r) == (op == '==')) else 0
            else:
                a, b = self.num(v), self.num(r)
                v = 1 if {'<': a < b, '>': a > b,
                          '<=': a <= b, '>=': a >= b}[op] else 0
        return v


def fold(toks, syms, env):
    """Constant-fold an expression; -> int, ('sym',n), or None."""
    # normalise '<' '=' etc. into single tokens
    merged, k = [], 0
    while k < len(toks):
        a = toks[k][1]
        b = toks[k + 1][1] if k + 1 < len(toks) else ''
        if a in ('=', '!', '<', '>') and b == '=':
            merged.append(('punct', a + '=', toks[k][2], toks[k + 1][3]))
            k += 2
            continue
        merged.append(toks[k])
        k += 1
    try:
        e = Eval(merged, syms, env)
        v = e.expr()
        return v if e.i == len(merged) else None
    except (Unknown, IndexError, KeyError, ZeroDivisionError):
        return None


def run_travtab(body, syms, env):
    """Re-execute a Go: block that fills travtab[] itself.  Understands only
    the shapes loc.f actually uses -- a plain assignment, an if on a constant
    condition, and a counted for loop -- and raises Unknown on anything else,
    so an unrecognised block falls back to being shown as source."""
    out = []

    def block(t, i, end):
        while i < end:
            i = stmt(t, i, end)
        return i

    def skip_to(t, i, ch, end):
        depth = 0
        while i < end:
            if t[i][1] in '([{':
                depth += 1
            elif t[i][1] in ')]}':
                if depth == 0 and t[i][1] == ch:
                    return i
                depth -= 1
            elif depth == 0 and t[i][1] == ch:
                return i
            i += 1
        raise Unknown('no %r' % ch)

    def paren(t, i):
        """t[i] == '('; -> (inner tokens, index past ')')"""
        depth, j = 0, i
        while j < len(t):
            if t[j][1] in '([':
                depth += 1
            elif t[j][1] in ')]':
                depth -= 1
                if depth == 0:
                    return t[i + 1:j], j + 1
            j += 1
        raise Unknown('unbalanced (')

    def braced(t, i):
        depth, j = 0, i
        while j < len(t):
            if t[j][1] == '{':
                depth += 1
            elif t[j][1] == '}':
                depth -= 1
                if depth == 0:
                    return i + 1, j, j + 1
            j += 1
        raise Unknown('unbalanced {')

    def stmt(t, i, end):
        if t[i][1] == '{':
            a, b, nxt = braced(t, i)
            block(t, a, b)
            return nxt
        if t[i][1] == 'if':
            cond, j = paren(t, i + 1)
            if (len(cond) > 2 and cond[0][0] == 'id' and cond[1][1] == '='
                    and cond[2][1] != '='):            # not "loc == TREE_4"
                v = fold(cond[2:], syms, env)      # if(k = which(...))
                if v is None:
                    raise Unknown('if assignment not constant')
                env[cond[0][1]] = v
            else:
                v = fold(cond, syms, env)
                if v is None:
                    raise Unknown('if condition not constant')
            truthy = v != 0 if not isinstance(v, tuple) else True
            j = stmt(t, j, end) if truthy else skip_stmt(t, j, end)
            if j < end and t[j][1] == 'else':
                j = skip_stmt(t, j + 1, end) if truthy else stmt(t, j + 1, end)
            return j
        if t[i][1] == 'for':
            head, j = paren(t, i + 1)
            parts, cur, depth = [], [], 0
            for tk in head:
                if tk[1] in '([':
                    depth += 1
                elif tk[1] in ')]':
                    depth -= 1
                if depth == 0 and tk[1] == ';':
                    parts.append(cur)
                    cur = []
                else:
                    cur.append(tk)
            parts.append(cur)
            if len(parts) != 3:
                raise Unknown('for head')
            var = parts[0][0][1]
            env[var] = fold(parts[0][2:], syms, env)
            guard = 0
            while fold(parts[1], syms, env):
                stmt(t, j, end)
                # step is ++var or var++
                names = [x[1] for x in parts[2] if x[0] == 'id']
                if not names:
                    raise Unknown('for step')
                env[names[0]] += -1 if any(x[1] == '-' for x in parts[2]) else 1
                guard += 1
                if guard > 10000:
                    raise Unknown('runaway for')
            return skip_stmt(t, j, end)
        # simple statement: LHS = expr ;
        semi = skip_to(t, i, ';', end)
        toks = t[i:semi]
        if toks and toks[0][1] == 'travtab':
            eq = next(k for k in range(len(toks)) if toks[k][1] == '=')
            v = fold(toks[eq + 1:], syms, env)
            if v is None:
                raise Unknown('travtab value')
            out.append(v)
        elif len(toks) >= 3 and toks[0][0] == 'id' and toks[1][1] == '=':
            env[toks[0][1]] = fold(toks[2:], syms, env)
        return semi + 1

    def skip_stmt(t, i, end):
        if t[i][1] == '{':
            return braced(t, i)[2]
        if t[i][1] in ('if', 'for'):
            _, j = paren(t, i + 1)
            j = skip_stmt(t, j, end)
            if j < end and t[j][1] == 'else':
                j = skip_stmt(t, j + 1, end)
            return j
        return skip_to(t, i, ';', end) + 1

    # the loop body may itself be "if(k = which(...)) { ... }": handle the
    # embedded assignment by rewriting it into cond + body at fold time
    block(body, 0, len(body))
    return out


def dest_ids(toks, syms, lo, hi):
    """Location symbols mentioned in an expression.  A leading '.' makes it a
    state variable (.STONE_DOOR is a flag, not the room STONE_DOOR)."""
    out = []
    for k, t in enumerate(toks):
        if t[0] != 'id' or not (lo <= syms.get(t[1], -1) <= hi):
            continue
        if k and toks[k - 1][1] == '.':
            continue
        if t[1] not in out:
            out.append(t[1])
    return out


def raw(src, body):
    """Original source text of a token run, re-indented for display."""
    if not body:
        return ''
    text = src[body[0][2]:body[-1][3]]
    lines = [l.rstrip() for l in text.split('\n')]
    strip = min((len(l) - len(l.lstrip()) for l in lines[1:] if l.strip()),
                default=0)
    return '\n'.join([lines[0]] + [l[strip:] if l.strip() else '' for l in lines[1:]])


# --------------------------------------------------------------------------

def main():
    syms, words = parse_vocab(os.path.join(SRC, 'adv.f'))
    FIRST_LOC, LAST_LOC = syms['FIRST_LOC'], syms['LAST_LOC']
    FIRST_CAVE = syms['FIRST_CAVE']
    nloc = LAST_LOC - FIRST_LOC + 1

    # canonical name per location ordinal (prefer the one loc.f uses)
    locname = {}
    for name, v in syms.items():
        if FIRST_LOC <= v <= LAST_LOC:
            cur = locname.get(v)
            if cur is None or name.startswith(('FIRST_', 'LAST_', 'START_',
                                               'SCORE_', 'RESTART_', 'LAMP_',
                                               'MOVE_OBJ')) is False and \
               cur.startswith(('FIRST_', 'LAST_', 'START_', 'SCORE_',
                               'RESTART_', 'LAMP_', 'MOVE_OBJ')):
                locname[v] = name
            elif cur is None:
                locname[v] = name
    for v in range(FIRST_LOC, LAST_LOC + 1):
        locname.setdefault(v, 'LOC_%d' % (v - FIRST_LOC))

    src = open(os.path.join(SRC, 'loc.f')).read()
    toks = tokenize(src)

    # locate locexec's switch(loc, fn) { ... }
    k = next(i for i, t in enumerate(toks)
             if t[1] == 'locexec' and toks[i + 1][1] == '(')
    k = next(i for i in range(k, len(toks))
             if toks[i][1] == 'switch' and toks[i + 2][1] == 'loc')
    k = next(i for i in range(k, len(toks)) if toks[i][1] == '{')
    end = balanced(toks, k)
    inner = toks[k + 1:end - 1]

    rooms = {}
    globals_ = None
    i = 0
    while i < len(inner):
        labels, j = read_labels(inner, i)
        if labels is None:
            i += 1
            continue
        assert inner[j][1] == '{', inner[j]
        e = balanced(inner, j)
        body = inner[j + 1:e - 1]
        names = []
        for a, b in labels:
            if b is None:
                names.append(a)
            else:
                for v in range(syms[a], syms[b] + 1):
                    names.append(locname[v])
        blk = split_labelled(body)
        if names == ['default']:
            globals_ = blk
        else:
            for nm in names:
                rooms[nm] = blk
        i = e

    print('rooms with a block: %d   locations declared: %d' % (len(rooms), nloc))

    # ---- turn each block into room data ----------------------------------
    motion_words = {}          # name -> id

    def wordid(nm):
        return motion_words.setdefault(nm, len(motion_words))

    def expand(a, b):
        if b is None:
            return [a]
        if a in COMPASS and b in COMPASS:
            return COMPASS[COMPASS.index(a):COMPASS.index(b) + 1]
        if a in syms and b in syms:
            return [locname[v] for v in range(syms[a], syms[b] + 1)]
        raise SystemExit('cannot expand range %s::%s' % (a, b))

    def plain_msg(body):
        """body is exactly one string literal statement?"""
        if len(body) == 2 and body[0][0] == 'str' and body[1][1] == ';':
            return unstring(body[0][1])
        if (len(body) == 5 and body[0][1] == 'putstr' and body[1][1] == '('
                and body[2][0] == 'str' and body[3][1] == ')'
                and body[4][1] == ';'):
            return unstring(body[2][1])
        return None

    def parse_travel(body, locval):
        """-> list of exits, or None if there is no plain travel() call."""
        for p in range(len(body)):
            if body[p][1] == 'travel' and body[p + 1][1] == '(':
                args, _ = call_args(body, p + 1)
                break
        else:
            return None
        exits, pending = [], []
        for a in args:
            v = fold(a, syms, {'loc': locval})       # computed destination?
            if isinstance(v, int) and FIRST_LOC <= v <= LAST_LOC and len(a) > 1:
                for w, wneg in pending:
                    exits.append(dict(word=w, wneg=wneg, dest=locname[v],
                                      dneg=False, cond=None, alts=[]))
                pending = []
                continue
            neg = a[0][1] == '-'
            core = a[1:] if neg else a
            if len(core) == 1 and core[0][0] == 'id':
                nm = core[0][1]
                v = syms.get(nm)
                if v is not None and FIRST_LOC <= v <= LAST_LOC:
                    for w, wneg in pending:
                        exits.append(dict(word=w, wneg=wneg, dest=locname[v],
                                          dneg=neg, cond=None, alts=[]))
                    pending = []
                else:
                    pending.append((nm, neg))
                continue
            # an expression: a conditional destination
            alts = [locname[syms[n]]
                    for n in dest_ids(core, syms, FIRST_LOC, LAST_LOC)]
            for w, wneg in pending:
                exits.append(dict(word=w, wneg=wneg, dest=None, dneg=False,
                                  cond=raw(src, a), alts=alts))
            pending = []
        return exits

    def parse_furnish(body):
        for p in range(len(body)):
            if body[p][1] == 'furnish' and body[p + 1][1] == '(':
                args, _ = call_args(body, p + 1)
                return [a[0][1] for a in args if len(a) == 1 and a[0][0] == 'id']
        return None

    def parse_travtab(body, locval):
        """The mallorn grove fills travtab[] instead of calling travel().
        -> list of exits, or None if the block is not of that shape."""
        if not any(t[1] == 'travtab' for t in body):
            return None
        try:
            vals = run_travtab(body, syms, {'loc': locval})
        except (Unknown, IndexError, KeyError, StopIteration):
            return None
        exits, pending = [], []
        for v in vals:
            if isinstance(v, tuple):                 # a motion word
                pending.append(v[1])
            elif FIRST_LOC <= v <= LAST_LOC:
                for w in pending:
                    exits.append(dict(word=w, wneg=False, dest=locname[v],
                                      dneg=False, cond=None, alts=[]))
                pending = []
            elif v == 0:                             # end-of-table marker
                pending = []
            else:
                return None
        return exits

    def build(blk, locval):
        r = dict(look=None, brief=None, inread=None, exits=[], furnish=[],
                 acts=[], enter=[])
        for labels, body in blk:
            names = []
            for a, b in labels:
                names += expand(a, b)
            text = plain_msg(body)
            source = raw(src, body)
            for nm in names:
                if nm == 'Look' and text is not None:
                    r['look'] = text
                elif nm == 'Brief' and text is not None:
                    r['brief'] = text
                elif nm == 'Inread' and text is not None:
                    r['inread'] = text
                elif nm == 'Furnish':
                    r['furnish'] = parse_furnish(body) or []
                elif nm == 'Go':
                    ex = parse_travel(body, locval)
                    if ex is None:
                        ex = parse_travtab(body, locval)
                    if ex is None:
                        r['acts'].append((nm, None, source))
                    else:
                        r['exits'] += ex
                        if len(body) > 1 and body[0][1] != 'travel':
                            r['acts'].append((nm, None, source))
                elif nm in ('Enter1', 'Enter2'):
                    r['enter'].append((nm, text, source))
                else:
                    r['acts'].append((nm, text, source))
        return r

    built = {nm: build(blk, syms[nm]) for nm, blk in rooms.items()}
    gbl = build(globals_, -1)

    # a room-level label naming a motion word becomes an exit override
    known_dirs = set(COMPASS) | set(OTHER_DIRS)
    stv = set(n for n, v in syms.items()
              if syms['FIRST_STV'] <= v <= syms['LAST_STV'])
    for nm, r in built.items():
        keep = []
        for lbl, text, source in r['acts']:
            if lbl in known_dirs or lbl in stv:
                r['exits'].append(dict(word=lbl, wneg=False, dest=None,
                                       dneg=False, cond=source, ovr=True,
                                       msg=text,
                                       alts=[locname[syms[n]] for n in dest_ids(
                                           tokenize(source), syms,
                                           FIRST_LOC, LAST_LOC)]))
            else:
                keep.append((lbl, text, source))
        r['acts'] = keep
        for e in r['exits']:
            e.setdefault('msg', None)
            e.setdefault('ovr', False)
            wordid(e['word'])
        # a label for one direction overrides the room's travel() table for it,
        # the way the original switch(loc, fn) dispatch does
        r['exits'].sort(key=lambda e: 0 if e['ovr'] else 1)

    # ---- emit data.h -----------------------------------------------------
    def cstr(s):
        if s is None:
            return 'NULL'
        out = ['"']
        for ch in s:
            if ch == '"':
                out.append('\\"')
            elif ch == '\\':
                out.append('\\\\')
            elif ch == '\n':
                out.append('\\n"\n        "')
            elif ch == '\t':
                out.append('\\t')
            elif ord(ch) < 32 or ord(ch) > 126:
                out.append('\\%03o' % ord(ch))
            else:
                out.append(ch)
        out.append('"')
        t = ''.join(out)
        tail = '"\n        ""'
        while t.endswith(tail):                  # trailing newline: no empty piece
            t = t[:-len(tail)] + '"'
        return t

    order = [locname[v] for v in range(FIRST_LOC, LAST_LOC + 1)]
    index = {nm: i for i, nm in enumerate(order)}

    o = []
    w = o.append
    w('/* data.h -- generated by gendata.py from the recovered adv.f / loc.f.')
    w(' * Do not edit; edit gendata.py. */')
    w('')
    w('#define NROOMS %d' % nloc)
    w('#define FIRST_CAVE %d' % (FIRST_CAVE - FIRST_LOC))
    w('#define START_LOC %d' % (syms['START_LOC'] - FIRST_LOC))
    w('#define RESTART_LOC %d' % (syms['RESTART_LOC'] - FIRST_LOC))
    w('')
    w('typedef struct { const char *word; short dest;')
    w('                 unsigned char wneg, dneg, ovr;')
    w('                 const char *cond, *msg; const short *alts; short nalts; } Exit;')
    w('typedef struct { const char *label, *msg, *src; } Act;')
    w('typedef struct { const char *name, *look, *brief, *inread;')
    w('                 const Exit *ex; short nex; const Act *ac; short nac;')
    w('                 const char *const *furn; short nfurn;')
    w('                 const Act *ent; short nent; unsigned char lit, built; } Room;')
    w('')

    for nm in order:
        r = built.get(nm)
        if not r:
            continue
        i = index[nm]
        for k2, e in enumerate(r['exits']):
            if e['alts']:
                w('static const short alt_%d_%d[] = {%s};' %
                  (i, k2, ','.join(str(index[a]) for a in e['alts'])))
        if r['exits']:
            w('static const Exit ex_%d[] = {' % i)
            for k2, e in enumerate(r['exits']):
                w('  {%s,%d,%d,%d,%d,%s,%s,%s,%d},' % (
                    cstr(e['word']),
                    index[e['dest']] if e['dest'] else -1,
                    1 if e['wneg'] else 0, 1 if e['dneg'] else 0,
                    1 if e['ovr'] else 0,
                    cstr(e['cond']), cstr(e['msg']),
                    ('alt_%d_%d' % (i, k2)) if e['alts'] else 'NULL',
                    len(e['alts'])))
            w('};')
        if r['acts']:
            w('static const Act ac_%d[] = {' % i)
            for lbl, text, source in r['acts']:
                w('  {%s,%s,%s},' % (cstr(lbl), cstr(text), cstr(source)))
            w('};')
        if r['enter']:
            w('static const Act en_%d[] = {' % i)
            for lbl, text, source in r['enter']:
                w('  {%s,%s,%s},' % (cstr(lbl), cstr(text), cstr(source)))
            w('};')
        if r['furnish']:
            w('static const char *const fu_%d[] = {%s};' %
              (i, ','.join(cstr(x) for x in r['furnish'])))
        w('')

    w('static const Room rooms[NROOMS] = {')
    for nm in order:
        r = built.get(nm)
        i = index[nm]
        if not r:
            w('  {%s,NULL,NULL,NULL,NULL,0,NULL,0,NULL,0,NULL,0,0,0},' % cstr(nm))
            continue
        w('  {%s,%s,%s,%s,%s,%d,%s,%d,%s,%d,%s,%d,%d,1},' % (
            cstr(nm), cstr(r['look']), cstr(r['brief']), cstr(r['inread']),
            ('ex_%d' % i) if r['exits'] else 'NULL', len(r['exits']),
            ('ac_%d' % i) if r['acts'] else 'NULL', len(r['acts']),
            ('fu_%d' % i) if r['furnish'] else 'NULL', len(r['furnish']),
            ('en_%d' % i) if r['enter'] else 'NULL', len(r['enter']),
            1 if 'LIGHT' in r['furnish'] else 0))
    w('};')
    w('')
    w('/* the global "under construction" default, verbatim from loc.f */')
    ucm = next((t for l, t, s in gbl['acts'] if l == 'Look' and t), None)
    if ucm is None:
        for labels, body in globals_:
            if any(a == 'Look' for a, b in labels):
                m = re.search(r'putstr\((".*?")\)', raw(src, body), re.S)
                ucm = unstring(m.group(1)) if m else None
    w('static const char under_construction[] = %s;' % cstr(ucm))
    w('')
    w('static const char *const compass[] = {%s};' %
      ','.join(cstr(c) for c in COMPASS))
    w('#define NCOMPASS %d' % len(COMPASS))

    open(os.path.join(HERE, 'data.h'), 'w').write('\n'.join(o) + '\n')

    # ---- report ----------------------------------------------------------
    nex = sum(len(r['exits']) for r in built.values())
    ncond = sum(1 for r in built.values() for e in r['exits'] if e['dest'] is None)
    nlook = sum(1 for r in built.values() if r['look'])
    print('data.h written: %d exits (%d plain, %d guarded), %d Look texts, '
          '%d motion words' % (nex, nex - ncond, ncond, nlook, len(motion_words)))
    nolook = [nm for nm in order if built.get(nm) and not built[nm]['look']]
    print('Look is not a plain string in %d rooms: %s'
          % (len(nolook), ' '.join(nolook)))
    missing = [nm for nm in order if nm not in built]
    print('locations with no block (fall to "under construction"): %d' % len(missing))
    if missing:
        print('   ' + ' '.join(missing))


if __name__ == '__main__':
    main()
