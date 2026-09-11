#!/usr/bin/env python3
r"""The acceptance test for SUSPEND: a suspended and resumed game must
continue exactly as an unbroken one would.

  A:  play turns 1..N, SUSPEND, restore, play turns N+1..M
  B:  play turns 1..M straight through

With the clock frozen the game is deterministic (RAN is seeded from
DATIME and from nothing else), so from turn N+1 on the two transcripts
have to agree line for line.  They are compared by longest common
suffix: everything from the first command after the resume to the final
score must be identical.

The one thing that legitimately differs is the resume itself.  The
original comes back at label 8305, which does START, then K=NULL and
GOTO 8 -- a null move, so the room is described again.  That extra
description belongs to the original's design, not to the state capture,
and it is the only text before the common suffix.

The test then goes further than the transcript: both runs SUSPEND again
at turn M and the two images are compared word by word, with the
differing words named from src/state.map.  Anything left over there
would be a variable the capture had missed.

usage:  python tools/savetest.py [-v]
"""
import io, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'bin', 'advent350.exe')
BIN = os.path.join(PORT, 'bin')
TMP = os.path.join(PORT, 'bin', '_test')

DATE = '06-JAN-2007'
TIME0 = '1200'
TIME1 = '1400'        # 120 minutes later: LATNCY is 90, so a resume is
                      # allowed without -u

FIRST = ['NO', 'IN', 'TAKE LAMP', 'TAKE KEYS', 'OUT', 'SOUTH', 'SOUTH',
         'SOUTH', 'OPEN GRATE', 'DOWN', 'WEST', 'ON', 'WEST']
REST = ['WEST', 'TAKE CAGE', 'WEST', 'TAKE ROD', 'INVENTORY', 'SCORE',
        'NO', 'EAST', 'EAST', 'LOOK']


def run(args, lines, when):
    env = dict(os.environ)
    env['ADVENT_DATE'] = DATE
    env['ADVENT_TIME'] = when
    p = subprocess.run([EXE] + args, input='\n'.join(lines) + '\n',
                       capture_output=True, text=True, cwd=BIN, env=env)
    return [l.rstrip() for l in p.stdout.replace('\r\n', '\n').split('\n')]


def suffix(a, b):
    n = 0
    while n < len(a) and n < len(b) and a[-1 - n] == b[-1 - n]:
        n += 1
    return n


def statemap():
    out = []
    for l in io.open(os.path.join(PORT, 'src', 'state.map')):
        off, sz, blk, nm = l.split()
        out.append((int(off), int(sz), blk, nm))
    return out


def words(path):
    d = io.open(path, 'rb').read()
    body = d[24:-16]                       # magic(16) + version(8) ... count+sum
    return [int.from_bytes(body[i:i+8], 'little', signed=True)
            for i in range(0, len(body), 8)]


def main():
    verbose = '-v' in sys.argv
    os.makedirs(TMP, exist_ok=True)
    a1 = os.path.join(TMP, 'a1.sav')
    a2 = os.path.join(TMP, 'a2.sav')
    b2 = os.path.join(TMP, 'b2.sav')
    for f in (a1, a2, b2):
        if os.path.exists(f):
            os.remove(f)
    end = ['SUSPEND', 'Y']
    rc = 0

    # A: play turns 1..N and suspend
    t1 = run(['-s', a1], FIRST + end, TIME0)
    if not os.path.exists(a1):
        print('FAIL: SUSPEND wrote no file')
        return 1
    # ... resume, play on, and suspend again.  Both runs write their
    # second image to the same name, so that the wording CIAO prints is
    # the same in both transcripts.
    m = os.path.join(TMP, 'm.sav')
    ta = run(['-s', m, a1], REST + end, TIME1)
    os.replace(m, a2)
    # B: the same game without the interruption
    tb = run(['-s', m], FIRST + REST + end, TIME0)
    os.replace(m, b2)

    # how much of B is the first N turns: the common prefix with A's
    # first leg, which diverges where A was offered the suspension
    p = 0
    while p < len(t1) and p < len(tb) and t1[p] == tb[p]:
        p += 1
    cont = tb[p:]
    n = suffix(ta, tb)
    print('turns 1..N produce %d identical lines' % p)
    print('B has %d lines after that; A and B share their last %d lines'
          % (len(cont), n))
    ok = n >= len(cont)
    print('the whole continuation is identical after the resume:', ok)
    if not ok:
        rc = 1
        print('--- B, from turn N+1 ---')
        print(chr(10).join(cont[:20]))
        print('--- A, the same stretch ---')
        print(chr(10).join(ta[len(ta)-len(cont):][:20]))
    extra = ta[:len(ta) - n]
    print('A prints %d extra lines at the resume (label 8305 null move):'
          % len(extra))
    for l in extra:
        print('   | %s' % l)

    # the images themselves
    wa, wb = words(a2), words(b2)
    if len(wa) != len(wb):
        print('FAIL: images differ in size')
        return 1
    diff = [i for i in range(len(wa)) if wa[i] != wb[i]]
    mp = statemap()
    named = {}
    for i in diff:
        for off, sz, blk, nm in mp:
            if off <= i < off + sz:
                k = '%s/%s' % (blk, nm)
                named[k] = named.get(k, 0) + 1
                break
    print('state at turn M: %d of %d words differ between the two runs'
          % (len(diff), len(wa)))
    for k in sorted(named):
        print('   %-16s %d word(s)' % (k, named[k]))
    return rc


if __name__ == '__main__':
    sys.exit(main())
