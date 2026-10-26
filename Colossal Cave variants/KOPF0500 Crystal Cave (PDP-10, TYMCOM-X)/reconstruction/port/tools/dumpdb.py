"""Recover CRYSTAL CAVE's data file from the core image.

On TYMCOM-X the program read its database once and was then SSAVEd, so the
tape holds the database only in parsed form.  The reader is still in the
image (000547-001351, behind ' INITIALISING...'), and this rebuilds the file
it reads, then proves it: the file is loaded again by the rules that reader
follows, the setup pass that comes after it is applied, and every array the
loading fills is compared with the image word for word.

    python tools/dumpdb.py                 # -> data/cave.dat
    python tools/dumpdb.py IMAGE.c OUT.dat

Only the 1979 image is described here; 1983 and 1984 are other compilations.

The file is Woods' twelve sections with Kopf's own details, all read off the
reader rather than assumed:

* a record is "number<TAB>text", 15A5 after the number, so a line runs to 70
  columns; section 7 carries a fourth field, the initial prop value;
* travel is "from to verb verb ...", up to 20 verbs; the reader keeps the
  verbs and destinations in two parallel arrays;
* a 0 section number (or a blank line) is skipped, and section 13 ends the
  load -- so the file ends "13", not "0";
* the setup that follows the load sets COND=2 at every location whose first
  travel verb is 1, which is undone here before section 9 is written.

Two places need more than reading back.  RTEXT 212 is defined twice: the
dragon message where it falls in order, and later "YOU ARE DEAD!", which
replaced it; the first copy is still in the message chain, unreferenced, and
goes back in as 212.  And any section 5 line numbered 100 also lands in
PTEXT(100), so the state-1 line of object 98 shows up there.

Loading this file into the program itself, with the "already set up" flag
cleared, reproduces the image's data area exactly but for seven words of
clock state.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
M36 = (1 << 36) - 1

# Base = address of element 1, read off the reader's own stores (HINTS is
# the origin of HINTS(k,c) = HINTS + HNTSIZ*c + k).  The sizes are the
# program's own bounds.
G = dict(
    room1='YOU ARE STANDING AT THE END OF A ROAD BEFORE A BARN',
    LINES=0o23661, LTEXT=0o20674, STEXT=0o21122, KEY=0o21350, COND=0o21576,
    TRAVEL=0o15322, TRAVEL2=0o17107,      # verb, destination: two arrays
    KTAB=0o53221, ATAB=0o54123, PHROG=0o14314,
    PTEXT=0o56135, RTEXT=0o23255, MTEXT=0o56075, CTEXT=0o22726, CVAL=0o22742,
    PLAC=0o22024, FIXD=0o22170, IPROP=0o22500, ACTSPK=0o22644, HINTS=0o23001,
    LOCSIZ=150, OBJSIZ=100, TABSIZ=450, RTXSIZ=260, CLSMAX=12, MAGSIZ=32,
    ACTSIZ=50, HNTSIZ=20, TRVSIZ=885, LINSIZ=11999)
END = 13        # the reader's computed GOTO ends the load on section 13;
                # a 0 (or a blank line) only makes it read the next line


def load(path):
    txt = open(path).read()
    body = re.sub(r'/\*.*?\*/', ' ', txt[txt.index('image_data[] = {') + 16:], flags=re.S)
    nums = [int(t, 8) for t in re.findall(r'\b0[0-7]*\b', body)]
    mem, i = {}, 0
    while nums[i + 1]:
        addr, cnt = nums[i], nums[i + 1]
        for k in range(cnt):
            mem[addr + k] = nums[i + 2 + k]
        i += 2 + cnt
    return mem


def signed(w):
    return w - (1 << 36) if w >> 35 & 1 else w


def a5(w):
    return ''.join(chr(w >> (29 - 7 * k) & 0o177) for k in range(5))


def enc5(s):
    w = 0
    for c in (s + '     ')[:5]:
        w = w << 7 | ord(c)
    return w << 1


class Image:
    def __init__(self, path):
        self.m = load(path)
        self.g = g = G
        first = ''.join(a5(self.m.get(g['LINES'] + k, 0)) for k in range(1, 12))
        if not first.startswith(g['room1']):
            sys.exit('%s: room 1 is not where it should be - wrong image?' % path)

    def w(self, name, i=0):
        return self.m.get(self.g[name] + i, 0)

    def arr(self, name, n):
        return [signed(self.w(name, i)) for i in range(n)]

    def L(self, k):
        return signed(self.w('LINES', k - 1))

    def hint(self, k, c):       # HINTS(k, c), c = 1..4
        return signed(self.m.get(self.g['HINTS'] + self.g['HNTSIZ'] * c + k, 0))


def walk(img):
    """LINES(k) is the index of the line after line k, negative when line k
    begins a message; LINES(end) = -1.  Returns [(k, [words-per-line])], end."""
    msgs, k = [], 1
    while True:
        p = img.L(k)
        nxt = abs(p)
        if nxt <= k:
            return msgs, k
        words = [img.w('LINES', i - 1) for i in range(k + 1, nxt)]
        if p < 0:
            msgs.append((k, [words]))
        else:
            msgs[-1][1].append(words)
        k = nxt


def text(words):
    return ''.join(a5(w) for w in words).rstrip(' ')


def forced(img, key, travel):
    """Locations the setup marks COND=2: described, with travel, whose first
    option is the forced-motion verb 1."""
    g = img.g
    ltext = img.arr('LTEXT', g['LOCSIZ'])
    return {i + 1 for i, k in enumerate(key)
            if k and ltext[i] and abs(travel[k - 1][1]) == 1}


def travel_entries(img, n):
    """[(dest, signed verb)] for TRAVEL(1..n)."""
    out = []
    for t in range(n):
        v = signed(img.w('TRAVEL', t))
        if img.g['TRAVEL2']:
            out.append((signed(img.w('TRAVEL2', t)), v))
        else:
            y, verb = divmod(abs(v), 1000)
            out.append((y, -verb if v < 0 else verb))
    return out


def dump(img):
    g = img.g
    msgs, end = walk(img)
    ordinal = {k: i for i, (k, _) in enumerate(msgs)}
    out, claimed = [], {}

    def message(num, o, what):
        if o in claimed:
            sys.exit('message %d is both %s and %s' % (o, claimed[o], what))
        claimed[o] = what
        for words in msgs[o][1]:
            out.append('%s\t%s' % (num, text(words)))

    def pointers(name, size):
        return sorted((ordinal[v], n + 1) for n, v in enumerate(img.arr(name, size)) if v)

    starts = sorted(pointers(n, g[s])[0][0] for n, s in
                    [('LTEXT', 'LOCSIZ'), ('STEXT', 'LOCSIZ'), ('PTEXT', 'OBJSIZ'),
                     ('RTEXT', 'RTXSIZ'), ('CTEXT', 'CLSMAX'), ('MTEXT', 'MAGSIZ')])
    starts.append(len(msgs))

    def text_section(sec, name, size):
        out.append(str(sec))
        ptr = pointers(name, size)
        where = {o: n for o, n in ptr}
        target = {n: o for o, n in ptr}
        n = None
        for o in range(ptr[0][0], starts[starts.index(ptr[0][0]) + 1]):
            if o in where:
                n = where[o]
            else:
                # a message the file defines again later: it took the next
                # number, which now points further on
                n += 1
                if target.get(n, -1) <= o:
                    sys.exit('message %d in section %d has no number' % (o, sec))
            message(n, o, 'section %d #%d' % (sec, n))
        out.append('-1')

    text_section(1, 'LTEXT', g['LOCSIZ'])
    text_section(2, 'STEXT', g['LOCSIZ'])

    # travel: KEY(loc) -> first entry, the last of a location negative
    key = img.arr('KEY', g['LOCSIZ'])
    last = max(key)
    while signed(img.w('TRAVEL', last - 1)) >= 0:
        last += 1
    trav = travel_entries(img, last)
    out.append('3')
    for k, loc in sorted((k, i + 1) for i, k in enumerate(key) if k):
        lines, t = [], k
        while True:
            y, v = trav[t - 1]
            if lines and lines[-1][0] == y and len(lines[-1][1]) < 20:
                lines[-1][1].append(abs(v))
            else:
                lines.append((y, [abs(v)]))
            t += 1
            if v < 0:
                break
        for y, verbs in lines:
            out.append('\t'.join(map(str, [loc, y] + verbs)))
    out.append('-1')

    out.append('4')
    for i in range(g['TABSIZ']):
        kv = signed(img.w('KTAB', i))
        if kv == -1:
            break
        word = a5(img.w('ATAB', i) ^ img.w('PHROG'))
        # A5 takes five characters whatever they are, so a comment behind a
        # tab after a short word survives as far as the fifth column
        out.append('%d\t%s' % (kv, word if '\t' in word else word.rstrip()))
    out.append('-1')

    # objects: PTEXT(obj) -> inventory line; the property messages follow it
    # and are numbered 000, 100, 200 ... by position.  The reader stores any
    # message numbered 1..100 in PTEXT, so a property line numbered 100 also
    # lands in PTEXT(100) when there is no object 100.
    heads = pointers('PTEXT', g['OBJSIZ'])
    stops = [o for o, _ in heads[1:]] + [pointers('RTEXT', g['RTXSIZ'])[0][0]]
    props = {}
    for (o, obj), stop in zip(heads, stops):
        props[obj] = list(range(o + 1, stop))
    i100 = [i for i, (o, n) in enumerate(heads) if n == 100]
    if i100 and i100[0] and (i100[0] < len(heads) - 1 or len(props[100]) > 1):
        # not object 100 but state 1 of the object before it: object 100
        # would come last, and its own state 1 would have taken PTEXT(100)
        o100, owner = heads[i100[0]][0], heads[i100[0] - 1][1]
        props[owner] += [o100] + props.pop(100)
        del heads[i100[0]]
    out.append('5')
    for o, obj in heads:
        message(obj, o, 'object %d' % obj)
        for prop, p in enumerate(props[obj]):
            message('%03d' % (100 * prop), p, 'object %d state %d' % (obj, prop))
    out.append('-1')

    text_section(6, 'RTEXT', g['RTXSIZ'])

    plac, fixd = img.arr('PLAC', g['OBJSIZ']), img.arr('FIXD', g['OBJSIZ'])
    iprop = img.arr('IPROP', g['OBJSIZ']) if g['IPROP'] else [0] * g['OBJSIZ']
    out.append('7')
    for i in range(g['OBJSIZ']):
        if plac[i] or fixd[i] or iprop[i]:
            f = [i + 1, plac[i], fixd[i]] + ([iprop[i]] if g['IPROP'] else [])
            while len(f) > 2 and f[-1] == 0:
                f.pop()
            out.append('\t'.join(map(str, f)))
    out.append('-1')

    act = img.arr('ACTSPK', g['ACTSIZ'])
    out.append('8')
    for i in range(max(i for i, v in enumerate(act) if v) + 1):
        out.append('%d\t%d' % (i + 1, act[i]))
    out.append('-1')

    cond = img.arr('COND', g['LOCSIZ'])
    for loc in forced(img, key, trav):       # undo the setup's COND=2
        if cond[loc - 1] == 2:
            cond[loc - 1] = 0
    out.append('9')
    for bit in range(36):
        locs = [i + 1 for i, c in enumerate(cond) if c >> bit & 1]
        for j in range(0, len(locs), 20):
            out.append('\t'.join(map(str, [bit] + locs[j:j + 20])))
    out.append('-1')

    cval = img.arr('CVAL', g['CLSMAX'])
    out.append('10')
    for o, n in pointers('CTEXT', g['CLSMAX']):
        message(cval[n - 1], o, 'class %d' % n)
    out.append('-1')

    out.append('11')
    for h in range(1, g['HNTSIZ'] + 1):
        row = [img.hint(h, c) for c in range(1, 5)]
        if any(row):
            out.append('\t'.join(map(str, [h] + row)))
    out.append('-1')

    text_section(12, 'MTEXT', g['MAGSIZ'])
    out.append(str(END))

    left = [i for i in range(len(msgs)) if i not in claimed]
    if left:
        sys.exit('messages not accounted for: %s' % left[:10])
    return out, len(msgs), end


def reload(lines, img):
    """Load the file by the reader's rules, then the setup's COND pass;
    return {address: word} and the counts the reader ends with."""
    g = img.g
    got, it = {}, iter(lines)

    def put(name, i, v):
        got[g[name] + i] = v & M36

    linuse, trvs, tab, seen, ncls = 1, 1, 0, set(), 0
    trav = {}
    for sec in it:
        sec = int(sec)
        if sec == END:
            break
        prev = None
        for rec in it:
            f = rec.split('\t')
            if int(f[0]) == -1:
                break
            if sec in (1, 2, 5, 6, 10, 12):
                num, txt = f[0], '\t'.join(f[1:])
                words = [enc5(txt[k:k + 5]) for k in range(0, len(txt), 5)]
                assert 0 < len(words) <= 14, rec
                start, prev = num != prev, num
                if start:
                    n = int(num)
                    if sec == 1: put('LTEXT', n - 1, linuse)
                    if sec == 2: put('STEXT', n - 1, linuse)
                    if sec == 5 and 0 < n <= 100: put('PTEXT', n - 1, linuse)
                    if sec == 6: assert n <= g['RTXSIZ']; put('RTEXT', n - 1, linuse)
                    if sec == 12: assert n <= g['MAGSIZ']; put('MTEXT', n - 1, linuse)
                    if sec == 10:
                        put('CTEXT', ncls, linuse); put('CVAL', ncls, n); ncls += 1
                nxt = linuse + 1 + len(words)
                put('LINES', linuse - 1, -nxt if start else nxt)
                for k, w in enumerate(words):
                    put('LINES', linuse + k, w)
                linuse = nxt
            elif sec == 3:
                loc, y, verbs = int(f[0]), int(f[1]), [int(x) for x in f[2:]]
                assert 0 < len(verbs) <= 20, rec
                if loc in seen:     # un-mark the previous line's last entry
                    trav[trvs - 1][1] = -trav[trvs - 1][1]
                else:
                    seen.add(loc); put('KEY', loc - 1, trvs)
                for v in verbs:
                    trav[trvs] = [y, v]; trvs += 1
                trav[trvs - 1][1] = -trav[trvs - 1][1]
            elif sec == 4:
                put('KTAB', tab, int(f[0]))
                put('ATAB', tab, enc5('\t'.join(f[1:])) ^ img.w('PHROG')); tab += 1
            elif sec == 7:
                f = [int(x) for x in f] + [0, 0, 0]
                put('PLAC', f[0] - 1, f[1])
                put('FIXD', f[0] - 1, f[2])
                if g['IPROP']: put('IPROP', f[0] - 1, f[3])
            elif sec == 8:
                put('ACTSPK', int(f[0]) - 1, int(f[1]))
            elif sec == 9:
                for loc in f[1:]:
                    a = g['COND'] + int(loc) - 1
                    assert not got.get(a, 0) >> int(f[0]) & 1, rec
                    got[a] = got.get(a, 0) + (1 << int(f[0]))
            elif sec == 11:
                k = int(f[0])
                assert 0 < k <= g['HNTSIZ'], rec
                for c in range(1, 5):
                    got[g['HINTS'] + g['HNTSIZ'] * c + k] = int(f[c]) & M36
        if sec == 4:
            put('KTAB', tab, -1)
    put('LINES', linuse - 1, -1)
    assert linuse + 14 <= g['LINSIZ'] and trvs <= g['TRVSIZ']

    for t, (y, v) in trav.items():
        if g['TRAVEL2']:
            put('TRAVEL', t - 1, v); put('TRAVEL2', t - 1, y)
        else:
            put('TRAVEL', t - 1, (1 if v >= 0 else -1) * (y * 1000 + abs(v)))
    # setup: a described location whose first travel verb is 1 gets COND=2
    for loc in range(1, g['LOCSIZ'] + 1):
        k = signed(got.get(g['KEY'] + loc - 1, 0))
        if k and got.get(g['LTEXT'] + loc - 1, 0) and abs(trav[k][1]) == 1:
            put('COND', loc - 1, 2)
    return got, linuse, trvs - 1, tab


def verify(lines, img):
    g = img.g
    got, nlines, ntrav, ntab = reload(lines, img)
    regions = [('LINES', nlines), ('LTEXT', g['LOCSIZ']), ('STEXT', g['LOCSIZ']),
               ('KEY', g['LOCSIZ']), ('COND', g['LOCSIZ']), ('TRAVEL', ntrav),
               ('TRAVEL2', ntrav), ('KTAB', ntab + 1), ('ATAB', ntab),
               ('PTEXT', g['OBJSIZ']), ('RTEXT', g['RTXSIZ']), ('MTEXT', g['MAGSIZ']),
               ('CTEXT', g['CLSMAX']), ('CVAL', g['CLSMAX']), ('PLAC', g['OBJSIZ']),
               ('FIXD', g['OBJSIZ']), ('IPROP', g['OBJSIZ']), ('ACTSPK', g['ACTSIZ'])]
    spans = [(name, g[name], n) for name, n in regions if g[name]]
    spans.append(('HINTS', g['HINTS'] + g['HNTSIZ'] + 1, 4 * g['HNTSIZ']))
    total = 0
    for name, a0, n in spans:
        bad = [a for a in range(a0, a0 + n) if got.get(a, 0) != img.m.get(a, 0)]
        if bad:
            sys.exit('%s differs from the image at %s' % (name, ', '.join('%06o' % a for a in bad[:5])))
        total += n
    return total, len(spans), ntrav, ntab


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'src', 'image.c')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, '..', 'data', 'cave.dat')
    img = Image(src)
    lines, nmsg, end = dump(img)
    words, nreg, ntrav, ntab = verify(lines, img)
    os.makedirs(os.path.dirname(os.path.abspath(dst)), exist_ok=True)
    with open(dst, 'w', newline='\r\n') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d messages (%d lines of text), %d travel options, %d words; %d records written to %s'
          % (nmsg, sum(len(t) for _, t in walk(img)[0]), ntrav, ntab, len(lines),
             os.path.normpath(dst)))
    print('reloading the file rebuilds all %d arrays, %d words, identical to the image'
          % (nreg, words))

if __name__ == '__main__':
    main()
