"""Recover ADVENTURE's data file from the core image.

On TYMCOM-X the program read its database once and was then SSAVEd, so the
tape holds the database only in parsed form.  This rebuilds the data file in
the layout Woods' FORTRAN reader expects (sections 1-12, "-1" after each,
"0" at the end) from src/image.c, and then proves it: the file is loaded
again by the same rules the reader in the image follows, and every array
that loading fills is compared with the image word for word.

    python tools/dumpdb.py                 # -> data/advent382.dat
    python tools/dumpdb.py IMAGE.c OUT.dat

Only the image is read.  The layout of the file is the one thing taken from
outside it; every text, number and ordering written comes from the image.

Array bases were found from the program's own indexed references (a
FORTRAN array X(1) at A is addressed as A-1(index)).  They are given for the
1979 compilation; the 1978 one has every one of them 5 words higher.  The
shift is found from the message table itself, not assumed.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
M36 = (1 << 36) - 1

# 1979 addresses (base = address of element 1)
LINES = 0o21313                          # message text, a chain (see walk())
LTEXT, STEXT, KEY, COND = 0o16533, 0o16761, 0o17207, 0o17435
TRAVEL = 0o15011
KTAB, ATAB = 0o45326, 0o46230
PTEXT, MTEXT = 0o50245, 0o50202
RTEXT, CTEXT, CVAL = 0o20707, 0o20421, 0o20435
ACTSPK, HINTS = 0o20337, 0o20521
PLAC, FIXD = 0o47671, 0o50035            # the live PLACE/FIXED copies (017663,
                                         # 020027) hold the same 200 words
PHROG = 0o14020                          # vocabulary XOR key, a literal
LOCSIZ, OBJSIZ, TABSIZ, RTXSIZ = 150, 100, 450, 208
CLSMAX, MAGSIZ, ACTSIZ, HNTSIZ = 12, 35, 35, 20


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
        # LINES(1) is the negative pointer in front of room 1's first line
        first = 'YOU ARE STANDING AT THE END OF A ROAD'
        room1 = [a for a, w in self.m.items()
                 if a5(w) == first[:5] and signed(self.m.get(a - 1, 0)) < 0
                 and ''.join(a5(self.m.get(a + k, 0)) for k in range(8)).startswith(first)]
        if len(room1) != 1:
            sys.exit('cannot find the start of the message table')
        self.d = room1[0] - 1 - LINES

    def w(self, base, i=0):
        return self.m.get(base + self.d + i, 0)

    def arr(self, base, n):
        return [signed(self.w(base, i)) for i in range(n)]

    def L(self, k):
        return signed(self.w(LINES, k - 1))


def walk(img):
    """LINES(k) is the index of the line after line k, negative when line k
    begins a message; LINES(end) = -1.  Returns [(k, [words-per-line])], end."""
    msgs, k = [], 1
    while True:
        p = img.L(k)
        nxt = abs(p)
        if nxt <= k:
            return msgs, k
        words = [img.w(LINES, i - 1) for i in range(k + 1, nxt)]
        if p < 0:
            msgs.append((k, [words]))
        else:
            msgs[-1][1].append(words)
        k = nxt


def text(words):
    s = ''.join(a5(w) for w in words)
    t = s.rstrip(' ')
    return t if -(-len(t) // 5) == len(words) else s   # keep a word of blanks


def dump(img):
    msgs, end = walk(img)
    ordinal = {k: i for i, (k, _) in enumerate(msgs)}
    out, claimed = [], {}

    def message(num, o, what):
        if o in claimed:
            sys.exit('message %d is both %s and %s' % (o, claimed[o], what))
        claimed[o] = what
        for words in msgs[o][1]:
            out.append('%s\t%s' % (num, text(words)))

    def pointers(base, size):
        return sorted((ordinal[v], n + 1) for n, v in enumerate(img.arr(base, size)) if v)

    def text_section(sec, base, size):
        out.append(str(sec))
        for o, n in pointers(base, size):
            message(n, o, 'section %d #%d' % (sec, n))
        out.append('-1')

    text_section(1, LTEXT, LOCSIZ)
    text_section(2, STEXT, LOCSIZ)

    # travel: KEY(loc) -> first entry; Y*1000+verb, last of a location negative
    out.append('3')
    for k, loc in sorted((k, i + 1) for i, k in enumerate(img.arr(KEY, LOCSIZ)) if k):
        lines, t = [], k
        while True:
            v = signed(img.w(TRAVEL, t - 1))
            y, verb = divmod(abs(v), 1000)
            if lines and lines[-1][0] == y:
                lines[-1][1].append(verb)
            else:
                lines.append((y, [verb]))
            t += 1
            if v < 0:
                break
        for y, verbs in lines:
            out.append('\t'.join(map(str, [loc, y] + verbs)))
    out.append('-1')

    out.append('4')
    for i in range(TABSIZ):
        kv = signed(img.w(KTAB, i))
        if kv == -1:
            break
        out.append('%d\t%s' % (kv, a5(img.w(ATAB, i) ^ img.w(PHROG)).rstrip()))
    out.append('-1')

    # objects: PTEXT(obj) -> inventory line; the property messages follow it
    # and are numbered 000, 100, 200 ... by position.  PTEXT(100) is not an
    # object: the reader tests 0 < N <= 100, so every property line numbered
    # 100 also lands there and the last one wins.
    heads = pointers(PTEXT, OBJSIZ - 1)
    stops = [o for o, _ in heads[1:]] + [pointers(RTEXT, RTXSIZ)[0][0]]
    out.append('5')
    for (o, obj), stop in zip(heads, stops):
        message(obj, o, 'object %d' % obj)
        for prop, p in enumerate(range(o + 1, stop)):
            message('%03d' % (100 * prop), p, 'object %d state %d' % (obj, prop))
    out.append('-1')

    text_section(6, RTEXT, RTXSIZ)

    plac, fixd = img.arr(PLAC, OBJSIZ), img.arr(FIXD, OBJSIZ)
    out.append('7')
    for i in range(OBJSIZ):
        if plac[i] or fixd[i]:
            out.append('\t'.join(map(str, [i + 1, plac[i]] + ([fixd[i]] if fixd[i] else []))))
    out.append('-1')

    act = img.arr(ACTSPK, ACTSIZ)
    out.append('8')
    for i in range(max(i for i, v in enumerate(act) if v) + 1):
        out.append('%d\t%d' % (i + 1, act[i]))
    out.append('-1')

    cond = img.arr(COND, LOCSIZ)
    out.append('9')
    for bit in range(36):
        locs = [i + 1 for i, c in enumerate(cond) if c >> bit & 1]
        if locs:
            out.append('\t'.join(map(str, [bit] + locs)))
    out.append('-1')

    cval = img.arr(CVAL, CLSMAX)
    out.append('10')
    for o, n in pointers(CTEXT, CLSMAX):
        message(cval[n - 1], o, 'class %d' % n)
    out.append('-1')

    out.append('11')
    for h in range(HNTSIZ):
        row = [signed(img.w(HINTS, c * HNTSIZ + h)) for c in range(4)]
        if any(row):
            out.append('\t'.join(map(str, [h + 1] + row)))
    out.append('-1')

    text_section(12, MTEXT, MAGSIZ)
    out.append('0')

    left = [i for i in range(len(msgs)) if i not in claimed]
    if left:
        sys.exit('messages not accounted for: %s' % left[:10])
    return out, len(msgs), end


def reload(lines, img):
    """Load the file by the reader's rules; return {address: word}."""
    got, it = {}, iter(lines)

    def put(base, i, v):
        got[base + img.d + i] = v & M36

    linuse, trvs, tab, seen, ncls = 1, 1, 0, set(), 0
    for sec in it:
        sec = int(sec)
        if sec == 0:
            break
        prev = None
        for rec in it:
            if rec == '-1':
                break
            f = rec.split('\t')
            if sec in (1, 2, 5, 6, 10, 12):
                num, txt = f[0], '\t'.join(f[1:])
                words = [enc5(txt[k:k + 5]) for k in range(0, len(txt), 5)]
                start, prev = num != prev, num
                if start:
                    n = int(num)
                    if sec == 1: put(LTEXT, n - 1, linuse)
                    if sec == 2: put(STEXT, n - 1, linuse)
                    if sec == 5 and 0 < n <= 100: put(PTEXT, n - 1, linuse)
                    if sec == 6: put(RTEXT, n - 1, linuse)
                    if sec == 12: put(MTEXT, n - 1, linuse)
                    if sec == 10:
                        put(CTEXT, ncls, linuse); put(CVAL, ncls, n); ncls += 1
                nxt = linuse + 1 + len(words)
                put(LINES, linuse - 1, -nxt if start else nxt)
                for k, w in enumerate(words):
                    put(LINES, linuse + k, w)
                linuse = nxt
            elif sec == 3:
                loc, y, verbs = int(f[0]), int(f[1]), [int(x) for x in f[2:]]
                if loc in seen:     # un-mark the previous line's last entry
                    a = TRAVEL + img.d + trvs - 2
                    got[a] = -signed(got[a]) & M36
                else:
                    seen.add(loc); put(KEY, loc - 1, trvs)
                for v in verbs:
                    put(TRAVEL, trvs - 1, y * 1000 + v); trvs += 1
                put(TRAVEL, trvs - 2, -(y * 1000 + verbs[-1]))
            elif sec == 4:
                put(KTAB, tab, int(f[0]))
                put(ATAB, tab, enc5(f[1]) ^ img.w(PHROG)); tab += 1
            elif sec == 7:
                put(PLAC, int(f[0]) - 1, int(f[1]))
                put(FIXD, int(f[0]) - 1, int(f[2]) if len(f) > 2 else 0)
            elif sec == 8:
                put(ACTSPK, int(f[0]) - 1, int(f[1]))
            elif sec == 9:
                for loc in f[1:]:
                    a = COND + img.d + int(loc) - 1
                    got[a] = got.get(a, 0) | 1 << int(f[0])
            elif sec == 11:
                for c in range(4):
                    put(HINTS, c * HNTSIZ + int(f[0]) - 1, int(f[1 + c]))
        if sec == 4:
            put(KTAB, tab, -1)
    put(LINES, linuse - 1, -1)
    return got, linuse, trvs - 1, tab


def verify(lines, img):
    got, nlines, ntrav, ntab = reload(lines, img)
    regions = [('LINES', LINES, nlines), ('LTEXT', LTEXT, LOCSIZ), ('STEXT', STEXT, LOCSIZ),
               ('KEY', KEY, LOCSIZ), ('TRAVEL', TRAVEL, ntrav), ('KTAB', KTAB, ntab + 1),
               ('ATAB', ATAB, ntab), ('PTEXT', PTEXT, OBJSIZ), ('RTEXT', RTEXT, RTXSIZ),
               ('PLAC', PLAC, OBJSIZ), ('FIXD', FIXD, OBJSIZ), ('ACTSPK', ACTSPK, ACTSIZ),
               ('COND', COND, LOCSIZ), ('CTEXT', CTEXT, CLSMAX), ('CVAL', CVAL, CLSMAX),
               ('HINTS', HINTS, 4 * HNTSIZ), ('MTEXT', MTEXT, MAGSIZ)]
    total = 0
    for name, base, n in regions:
        a0 = base + img.d
        bad = [a for a in range(a0, a0 + n) if got.get(a, 0) != img.m.get(a, 0)]
        if bad:
            sys.exit('%s differs from the image at %s' % (name, ', '.join('%06o' % a for a in bad[:5])))
        total += n
    return total, len(regions)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'src', 'image.c')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, '..', 'data', 'advent382.dat')
    img = Image(src)
    lines, nmsg, end = dump(img)
    words, nreg = verify(lines, img)
    os.makedirs(os.path.dirname(os.path.abspath(dst)), exist_ok=True)
    with open(dst, 'w', newline='\r\n') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d messages (%d lines of text), %d records written to %s'
          % (nmsg, sum(len(t) for _, t in walk(img)[0]), len(lines), os.path.normpath(dst)))
    print('image offset %+d; reloading the file rebuilds all %d arrays, %d words, '
          'identical to the image' % (img.d, nreg, words))


if __name__ == '__main__':
    main()
