"""Find where the program keeps the player, the monsters and the cave, for
--debug pokes (tests\\winnable.py uses the addresses).

usage: python tools\\findvars.py [PROG]

Plays the reference world (-Z) a few keys into the game (off the bridge the
game starts on there, east and south, so that the place is not also the
river's, the road's and A and B's), taking a memory dump (#dump) at two
ORDER: prompts with a rest (V) between them: the player stays, the monsters
move.  Then goes down into the cave, the monsters cleared, and takes a dump
before and after a step each way.  Prints, in octal:

  NS, EW          the player's place (3-word reals), found by the values ?
                  showed
  GP, SKYDD, FORSV, STRENGTH
                  gold, armour, defence and strength: the reals after EW, in
                  the order line 640 sets them (checked against 100, 10, 10)
  PAMON, OFMON    the monsters' places, north-south and east-west (INTEGER
                  arrays, element 0 first): the two runs of 31 words, 0 to 86,
                  that the rest changed, told apart by which way they closed in
                  (OFMON lies below PAMON)
  PLAN            the map (INTEGER, 86 by 86, column by column: PLAN(I,J) at
                  PLAN + I + 86*J), the run of 7396 words 0 to 14
  GROTT1, GROTT2  the cave's place (reals): the two with the place of the 13
                  in PLAN (cave: the place)
  town, castle    the place of a town and of a castle (a 4 and an 8 in PLAN)
  SPE$(1)         the things that lie in the caves' rooms: 2 words each, the
                  address of the characters and their number (0 0 is "")
  G3, G4          the player's place in the caves (reals), found by the steps
"""
import os
import re
import struct
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from portfuzz import Run  # noqa: E402

PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'advenb.exe')


def real(v):
    """v as the ND-100's 48-bit real: 040000+exponent, 32-bit fraction"""
    if v == 0:
        return (0, 0, 0)
    e, m = 0, float(abs(v))
    while m >= 1:
        m /= 2
        e += 1
    while m < 0.5:
        m *= 2
        e -= 1
    f = int(m * 2 ** 32)
    return ((0o100000 if v < 0 else 0) | (0o40000 + e), f >> 16, f & 0xFFFF)


def value(w, a):
    """the real at word a of a dump, or None"""
    e, h, l = w[a:a + 3]
    if e == h == l == 0:
        return 0.0
    x = (e & 0o77777) - 0o40000
    if abs(x) > 64 or not h & 0o100000:
        return None
    return (-1 if e & 0o100000 else 1) * ((h << 16) | l) / 2 ** 32 * 2 ** x


def load(path):
    b = open(path, 'rb').read()
    return struct.unpack('>%dH' % (len(b) // 2), b)


def dumps(prog, work):
    d0, d1 = os.path.join(work, 'd0.bin'), os.path.join(work, 'd1.bin')
    keys = 'N\r\x1bC\x1bB?\r#dump %s\rV\r#dump %s\rS' % (d0.replace('\\', '/'), d1.replace('\\', '/'))
    out = subprocess.run([EXE, '--prog', prog, '--raw', '--no-hold', '-Z', '1', '--debug'],
                         input=keys.encode('latin-1'), stdout=subprocess.PIPE, timeout=60).stdout
    text = bytes(b & 0x7f for b in out).decode('latin-1')
    m = re.search(r'koordinater \{r +(\d+) *, *(\d+)', text)
    return int(m.group(1)), int(m.group(2)), load(d0), load(d1)


def outside(prog, work):
    ns, ew, w0, w1 = dumps(prog, work)
    found = {}
    for name, v in (('NS', ns), ('EW', ew)):
        pat = real(v)
        hits = [i for i in range(len(w0) - 3) if tuple(w0[i:i + 3]) == pat and tuple(w1[i:i + 3]) == pat]
        assert len(hits) == 1, (name, hits)
        found[name] = hits[0]
    for k, name in enumerate(('GP', 'SKYDD', 'FORSV', 'STRENGTH')):
        found[name] = found['EW'] + 3 * (k + 1)
    assert [value(w1, found[n]) for n in ('GP', 'SKYDD', 'FORSV')] == [100, 10, 10]
    runs, i = [], 0
    while i < len(w0) - 31:
        if w0[i] == 0 and all(0 <= x <= 86 for x in w0[i:i + 31]) and w0[i:i + 31] != w1[i:i + 31] \
                and sum(1 for a, b in zip(w0[i:i + 31], w1[i:i + 31]) if a != b) >= 5:
            runs.append(i)
            i += 31
        else:
            i += 1
    # PAMON follows the player north and south, OFMON east and west

    def closer(i, v):
        return sum(1 for a, b in zip(w0[i + 1:i + 31], w1[i + 1:i + 31]) if abs(b - v) < abs(a - v))
    runs.sort(key=lambda i: closer(i, ew) - closer(i, ns))
    found['PAMON'], found['OFMON'] = runs
    i = 0
    while i < len(w0) - 7396:
        n = 0
        while i + n < len(w0) and w0[i + n] <= 14:
            n += 1
        if n == 7396:
            found['PLAN'] = i
        i += n + 1
    plan = found['PLAN']
    cave = [(k % 86, k // 86) for k in range(7396) if w0[plan + k] == 13]
    assert len(cave) == 1, cave
    g1, g2 = real(cave[0][0]), real(cave[0][1])
    hits = [a for a in range(len(w0) - 6) if tuple(w0[a:a + 3]) == g1 and tuple(w0[a + 3:a + 6]) == g2]
    assert len(hits) == 1, hits
    found['GROTT1'], found['GROTT2'] = hits[0], hits[0] + 3
    found['cave'] = cave[0]
    for name, what in (('town', 4), ('castle', 8)):
        found[name] = next((k % 86, k // 86) for k in range(7396)
                           if w1[plan + k] == what and 1 < k % 86 < 85 and 1 < k // 86 < 85)
    # a string is the address of its characters and their number; SPE$(1) is
    # the DATA constant "en glad hund"
    text = struct.pack('>%dH' % len(w0), *w0)
    at = text.find(b'en glad hund') // 2
    hits = [i for i in range(len(w0) - 2) if w0[i] == at and w0[i + 1] == 12 and w0[i + 2] != 0o177777]
    assert len(hits) == 1, hits
    found['SPE$(1)'] = hits[0]
    return found


def inside(prog, work, v):
    """down into the cave, a dump, a step north (or south), a dump, a step east
    (or west), a dump: G3 and G4 are the reals that moved by the steps"""
    r = Run([EXE, '--prog', prog, '--raw', '--no-hold', '-Z', '1', '--debug'])

    def say(keys):
        since = r.size
        r.send(keys)
        r.settle(since)
        out = r.text()[since:]
        while True:                                  # a monster: fight it
            seg = r.tail(600)
            seg = seg[seg.rfind('ORDER:', 0, len(seg) - 6):]
            if seg.endswith('ORDER:') and ('Pl|tsligt' in seg or 'Det kan du inte g|ra nu' in seg):
                keys = 'D'
            elif re.search(r"'RETURN' ?f\|r att forts\{tta (sl\}ss|striden)$", seg):
                keys = '\r'
            else:
                return out
            since = r.size
            r.send(keys)
            r.settle(since)

    def poke(a, words):
        say('#poke %o %s\r' % (a, ' '.join('%o' % x for x in words)))

    def dump(name):
        # the port echoes the line before it reads the Return, then writes the
        # file and says so: a slow file system outlasts say()'s quiet spell
        path = os.path.join(work, name)
        since = r.size
        say('#dump %s\r' % path.replace('\\', '/'))
        end = time.time() + 20
        while 'written' not in r.text()[since:] and time.time() < end:
            time.sleep(0.05)
        return load(path)

    try:
        say('N\r')
        poke(v['PAMON'], [0] * 31)
        poke(v['OFMON'], [0] * 31)
        poke(v['FORSV'], real(1000))
        poke(v['SKYDD'], real(1000))
        poke(v['STRENGTH'], real(10000))
        poke(v['NS'], real(v['cave'][0]))
        poke(v['EW'], real(v['cave'][1]))
        assert 'nere i grottorna' in say('N')
        w = [dump('c0.bin')]
        steps = []
        for there, back, moved in (('\x1bA', '\x1bB', r'H\{r \{r'), ('\x1bC', '\x1bD', r'Du \{r vid')):
            if re.search(moved, say(there)):
                steps.append(1)
            else:
                assert re.search(r'Du \{r vid', say(back))
                steps.append(-1)
            w.append(dump('c%d.bin' % len(w)))
    finally:
        r.p.kill()
    for name, (a, b), step in (('G3', (w[0], w[1]), steps[0]), ('G4', (w[1], w[2]), steps[1])):
        hits = [i for i in range(len(a) - 3) if value(a, i) is not None and value(b, i) is not None
                and 1 <= value(a, i) <= 15 and value(b, i) - value(a, i) == step]
        assert len(hits) == 1, (name, hits)
        v[name] = hits[0]
    return v


def find(prog):
    work = tempfile.mkdtemp(prefix='advenbvars')
    return inside(prog, work, outside(prog, work))


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else os.path.join(PORT, 'data', 'ADVENTURE-ENB.PROG')
    for name, a in find(prog).items():
        print('%-10s %s' % (name, a if isinstance(a, tuple) else '%o' % a))


if __name__ == '__main__':
    main()
