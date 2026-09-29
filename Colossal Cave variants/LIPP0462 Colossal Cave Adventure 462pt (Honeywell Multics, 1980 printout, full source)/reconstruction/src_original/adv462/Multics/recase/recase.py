#!/usr/bin/env python3
"""Recase adventure.data (lowercase) using mixed-case reference texts.

For each message, word casing is transferred from reference texts where a
3-word window matches (case-insensitively); remaining words get sentence
capitalization.  Only letter case changes; every line keeps its length.
Usage: recase.py ADVENT_DIR in.data out.data report.txt
(ADVENT_DIR: a checkout of https://github.com/Quuxplusone/Advent)
Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.
"""
import re, sys, collections, json

ADV = sys.argv[1].rstrip('/') + '/'
WORD = re.compile(r"[A-Za-z0-9]+(?:'[A-Za-z0-9]+)*")

def c_literals(path):
    """Groups of adjacent C string literals -> list of texts."""
    src = open(path, encoding='latin-1').read()
    src = re.sub(r'/\*.*?\*/', ' ', src, flags=re.S)
    texts = []
    cur = []
    pos = 0
    lit = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
    last_end = None
    for m in lit.finditer(src):
        between = src[last_end:m.start()] if last_end is not None else 'x'
        s = m.group(1).replace('\\"', '"').replace('\\n', ' ').replace("\\'", "'").replace('\\\\', '\\')
        s = s.replace('@/', ' ')
        if last_end is not None and re.fullmatch(r'(\s|SOFT_NL|NL|@/)*', between):
            cur.append(s)
        else:
            if cur: texts.append(' '.join(cur))
            cur = [s]
        last_end = m.end()
    if cur: texts.append(' '.join(cur))
    return texts

def acode_texts(path):
    texts, cur = [], []
    for line in open(path, encoding='latin-1'):
        line = line.rstrip('\n')
        if not line.strip():
            continue
        if line[0] not in ' \t':
            if cur: texts.append(' '.join(cur)); cur = []
            continue
        t = line.strip()
        if t.startswith('%'):
            if cur: texts.append(' '.join(cur)); cur = []
            t = t[1:]
        t = t.lstrip('/').strip()
        cur.append(t)
    if cur: texts.append(' '.join(cur))
    return texts

SOURCES = {
    'woods': c_literals(ADV + 'ODWY0350/advent.c') + c_literals(ADV + 'KNUT0350/advent.w'),
    'platt': acode_texts(ADV + 'PLAT0550/ADVENTURE.ACODE') + c_literals(ADV + 'ODWY0550/advdat.c'),
}

def tokens_with_flags(text):
    """[(lower, form, initial)] where initial = first word of a sentence."""
    out = []
    prev_end = 0
    initial = True
    for m in WORD.finditer(text):
        gap = text[prev_end:m.start()]
        if out:
            g = gap.replace('"', '').replace("'", '').replace('(', '').strip()
            initial = bool(re.search(r'[.!?]\s*$', text[:m.start()].rstrip(' "\'(\n'))) if g == '' or True else False
            # sentence start if the non-space text before the word ends with . ! ?
            before = text[:m.start()].rstrip()
            before = before.rstrip('"\'(')
            before = before.rstrip()
            initial = before.endswith(('.', '!', '?'))
        out.append((m.group(0).lower(), m.group(0), initial))
        prev_end = m.end()
    return out

# n-gram index: key (w1,w2,w3) lower -> list of (source, [forms], [initials])
N = 3
index = collections.defaultdict(list)
for src, texts in SOURCES.items():
    for t in texts:
        toks = tokens_with_flags(t)
        for i in range(len(toks) - N + 1):
            key = tuple(x[0] for x in toks[i:i+N])
            index[key].append((src, [x[1] for x in toks[i:i+N]], [x[2] for x in toks[i:i+N]]))
# short texts (1-2 words) indexed whole
short_index = collections.defaultdict(list)
for src, texts in SOURCES.items():
    for t in texts:
        toks = tokens_with_flags(t)
        if 1 <= len(toks) <= 2:
            short_index[tuple(x[0] for x in toks)].append((src, [x[1] for x in toks], [x[2] for x in toks]))

def neutral(form):
    """The case information a sentence-initial token carries: only non-trivial caps."""
    if form[1:] != form[1:].lower():      # e.g. XYZZY, Y2 fine, McDonald
        return form
    return None

def recase_message(text, prefer):
    """text: message lines joined with \n.  Returns (newtext, n_matched, n_words)."""
    toks = list(WORD.finditer(text))
    low = [m.group(0).lower() for m in toks]
    votes = [collections.Counter() for _ in toks]
    matched = [False] * len(toks)
    def add(i, src, form, init):
        w = 3 if src == prefer else 1
        if init:
            f = neutral(form)
            if f is None:
                matched[i] = True     # we know the word, not its mid-sentence case
                return
            form = f
        votes[i][form] += w
        matched[i] = True
    if len(toks) <= 2 and tuple(low) in short_index:
        for src, forms, inits in short_index[tuple(low)]:
            for j in range(len(toks)):
                add(j, src, forms[j], inits[j])
    for i in range(len(toks) - N + 1):
        for src, forms, inits in index.get(tuple(low[i:i+N]), []):
            for j in range(N):
                add(i + j, src, forms[j], inits[j])
    res = list(text)
    for i, m in enumerate(toks):
        w = m.group(0)
        if votes[i]:
            form = votes[i].most_common(1)[0][0]
        else:
            form = w.lower()
            if form == 'i' or re.fullmatch(r"i'(m|ll|ve|d)", form):
                form = 'I' + form[1:]
        # sentence start in our own text
        before = text[:m.start()].rstrip().rstrip('"\'(').rstrip()
        if before == '' or before.endswith(('.', '!', '?')):
            form = form[0].upper() + form[1:]
        assert form.lower() == w.lower()
        res[m.start():m.end()] = list(form)
    return ''.join(res), sum(matched), len(toks)

def main():
    inp, outp, rep = sys.argv[2:5]
    lines = open(inp).read().split('\n')
    # locate sections
    sec = None
    groups = []          # (section, key, [line indexes])
    cur = None
    i = 0
    while i < len(lines):
        l = lines[i]
        if sec is None:
            if l.strip():
                sec = int(l[:8])
            i += 1
            continue
        if l[:8].strip() == '-1':
            sec = None; cur = None; i += 1; continue
        if sec in (1, 2, 5, 6, 10, 12):
            num = l[:8].strip()
            if sec == 5:
                # object header: number followed by name; prop lines 0,100,... start new paragraph
                key = (sec, 'obj', num) if (l[8:9] != '' and not num.lstrip('-').isdigit() or True) else None
                if cur and cur[0] == 5 and cur[1] == num:
                    cur[2].append(i)
                else:
                    cur = [5, num, [i]]
                    groups.append(cur)
            else:
                if cur and cur[0] == sec and cur[1] == num:
                    cur[2].append(i)
                else:
                    cur = [sec, num, [i]]
                    groups.append(cur)
        i += 1
    report = []
    objnum = None
    for g in groups:
        sec, num, idxs = g
        text = '\n'.join(lines[k][8:] for k in idxs)
        n = int(num)
        if sec == 5:
            prefer = 'platt' if (n >= 65 and len(num) <= 3 and n < 100 and lines[idxs[0]][8:9] not in ' ') else None
        if sec == 1 or sec == 2:
            prefer = 'platt' if n >= 141 else 'woods'
        elif sec == 6:
            prefer = 'platt' if n >= 202 else 'woods'
        elif sec == 5:
            pass
        else:
            prefer = 'woods'
        if sec == 5:
            # object number lines have text starting right after; prop lines are 0/100/200...
            # decide per group: header lines (object number) set current object
            if lines[idxs[0]][8:].startswith(('*',)) or not (n % 100 == 0):
                objnum = n
            prefer = 'platt' if objnum and objnum >= 65 else 'woods'
        new, nm, nw = recase_message(text, prefer)
        for k, t in zip(idxs, new.split('\n')):
            assert len(t) == len(lines[k][8:])
            lines[k] = lines[k][:8] + t
        report.append((sec, num, nm, nw, new))
    open(outp, 'w').write('\n'.join(lines))
    with open(rep, 'w') as f:
        for sec, num, nm, nw, new in report:
            f.write('%d %s %d/%d\n%s\n\n' % (sec, num, nm, nw, new))

main()
