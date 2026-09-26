#!/usr/bin/env python3
r"""Line-by-line accounting of every difference between the 1977 source
and what actually compiles.

    python tools/showdiff.py           summary
    python tools/showdiff.py -a        every changed line, with its reason

It diffs ../src_original/ADVENT.FOR against src/advent.f and labels each
change with the rule in tools/convert.py or the entry in tools/patches.py
that produced it, so that nothing in the generated file is unexplained.
"""
import difflib, io, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import patches, convert

SRC = convert.SRC
DST = convert.DST


RULES = [
    ('packed character literal -> its word value',
     lambda t: convert.re.sub(r"'([^']*)'",
                              lambda m: str(convert.pack(m.group(1))) + ' ', t)),
    ('DEC octal literal -> decimal',
     lambda t: convert.re.sub(r'"([0-7]+)',
                              lambda m: str(convert.sx36(int(m.group(1), 8))) + ' ', t)),
    ('TYPE n -> PRINT n',
     lambda t: convert.re.sub(r'(?<![A-Z0-9])TYPE(?=[ ' + chr(9) + r']+[0-9])',
                              'PRINT', t)),
    ('RAN -> RAN10 (RAN is a gfortran intrinsic)',
     lambda t: convert.re.sub(r'(?<![A-Z0-9])RAN(?![A-Z0-9])', 'RAN10', t)),
]


def mechanical(t, literals=True):
    """Apply the mechanical rewrites, reporting which ones fired.

    Character literals are packed everywhere except inside a FORMAT,
    which is why the caller tries it both ways."""
    fired = []
    for name, f in RULES:
        if not literals and name.startswith('packed'):
            continue
        u = f(t)
        if u != t:
            fired.append(name)
            t = u
    return t, fired


def cc1(t):
    """The carriage-control rewrite as it can appear on one line.

    A record starts at the FORMAT's opening paren, after any /, and at
    the beginning of a continuation line; the first blank of the record
    is the carriage control character and does not print.  Returns every
    way this line could legally have been rewritten."""
    out = []
    starts = []
    m = convert.re.match(r'^[ ' + chr(9) + r']*[0-9]*[ ' + chr(9) + r']*$',
                         t.split("'")[0].split('(')[0])
    for k, c in enumerate(t):
        if c == '/' or c == '(':
            starts.append(k + 1)
    if m:
        starts.append(0)
    for st in starts:
        r = t[st:]
        if st == 0:
            # a continuation line: tab, the continuation digit, tab
            lead = convert.re.match(r'^[ ' + chr(9) + r']*[0-9]?[ '
                                    + chr(9) + r']*', r).group(0)
        else:
            lead = convert.re.match(r'^[ ' + chr(9) + r']*', r).group(0)
        r2 = r[len(lead):]
        if r2.startswith("' "):
            e = r2.index("'", 1)
            lit = r2[2:e]
            rest = r2[e + 1:]
            if lit == '':
                if rest.startswith(','):
                    rest = rest[1:]
                out.append(t[:st] + lead + rest)
            else:
                out.append(t[:st] + lead + "'" + lit + "'" + rest)
        mx = convert.re.match(r'^([0-9]+)X', r2)
        if mx:
            n = int(mx.group(1)) - 1
            rest = r2[mx.end():]
            if n == 0:
                if rest.startswith(','):
                    rest = rest[1:]
                out.append(t[:st] + lead + rest)
            else:
                out.append(t[:st] + lead + '%dX' % n + rest)
    return out


def reason(old, new):
    """Why this line changed, checked by re-running the rules."""
    if old is None:
        if new.lstrip().startswith('CALL BLKIO') or 'STATIO' in new            or new.startswith('C  Generated'):
            return 'generated: STATIO, the complete state capture'
        if 'ADVSTA' in new:
            return 'generated: COMMON /ADVSTA/, the main program state'
        for k, v in patches.DECLS.items():
            if new in v:
                return 'declaration added (patches.DECLS)'
        return 'inserted by a patch'
    if old in patches.LINES:
        return 'patches.py'
    for v in patches.LINES.values():
        if new is not None and new in v:
            return 'patches.py (replacement line)'
    if old[:1] == chr(12):
        return 'page separator (form feed) removed'
    if new is None:
        return 'removed (routine superseded by runtime.f)'
    CC = 'carriage control (FOROTS ate the first blank of a record)'
    for lit in (True, False):
        t, fired = mechanical(old, lit)
        if t == new:
            return ' + '.join(fired) if fired else 'no change?'
        if new in cc1(t):
            return ' + '.join(fired + [CC])
    return 'UNEXPLAINED'


def main():
    all_ = '-a' in sys.argv
    a = io.open(SRC, encoding='latin-1').read().split('\n')
    b = io.open(DST, encoding='latin-1').read().split('\n')
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    counts = {}
    rows = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            continue
        olds = a[i1:i2]
        news = b[j1:j2]
        if tag == 'insert':
            olds = [None] * len(news)
        if tag == 'delete':
            news = [None] * len(olds)
        # pair each old line with whichever new line explains it, so a
        # patch that replaces one line with four does not knock the rest
        # of the hunk out of step
        free = list(news)
        for o in olds:
            hit, why = None, None
            for nw in free:
                w = reason(o, nw)
                if w != 'UNEXPLAINED':
                    hit, why = nw, w
                    break
            if hit is None:
                hit = free[0] if free else None
                why = reason(o, hit)
            else:
                free.remove(hit)
            counts[why] = counts.get(why, 0) + 1
            rows.append((i1 + 1, why, o, hit))
        for nw in free:
            if nw is None:
                continue
            why = reason(None, nw)
            counts[why] = counts.get(why, 0) + 1
            rows.append((i1 + 1, why, None, nw))
    print('ADVENT.FOR %d lines -> advent.f %d lines' % (len(a), len(b)))
    print()
    for why in sorted(counts, key=lambda w: -counts[w]):
        print('%5d  %s' % (counts[why], why))
    print()
    if all_:
        for ln, why, o, nw in rows:
            print('%-5s %s' % (ln, why))
            if o is not None:
                print('   -| %s' % o.replace('\t', '    '))
            if nw is not None:
                print('   +| %s' % nw.replace('\t', '    '))


if __name__ == '__main__':
    main()
