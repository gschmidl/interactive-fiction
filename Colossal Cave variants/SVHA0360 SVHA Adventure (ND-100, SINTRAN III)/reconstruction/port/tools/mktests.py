"""Turn RetroCore captures into the regression tests in tests/ref.

usage: python tools/mktests.py

For each capture in tests/captures writes tests/ref/NAME.in (the bytes
typed), tests/ref/NAME.ref (what SINTRAN III sent back from the start of the
game until it stopped or the input ran out) and, for sessions whose random
numbers matter, tests/ref/NAME.clock (the -Z value that shows the program the
minute it was started at on the reference).

  NAME.raw + NAME.raw.json + tests/scripts/NAME.txt   svhacap.py sessions
  k_*.raw                                            svhakeys.py sessions
  first_session.raw                                  the first session, typed by hand
"""
import json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import svhakeys

ROOT = os.path.join(HERE, '..')
CAP = os.path.join(ROOT, 'tests', 'captures')
SCRIPTS = os.path.join(ROOT, 'tests', 'scripts')
OUT = os.path.join(ROOT, 'tests', 'ref')


def script_bytes(path):
    cmds = [l.rstrip('\r\n') for l in open(path, encoding='latin-1')]
    return b''.join(c.encode('latin-1') + b'\r' for c in cmds if not c.startswith('#'))


def game_part(raw):
    """from the first output of SVH to where the program left (CR LF before @)"""
    i = raw.find(b'SVH\r\n')
    if i >= 0:
        raw = raw[i + 5:]
    j = raw.find(b'\r\n@')
    return raw[:j + 2] if j >= 0 else raw


def main():
    os.makedirs(OUT, exist_ok=True)
    tests = {}
    for f in sorted(os.listdir(SCRIPTS)):
        name = f[:-4]
        cap = os.path.join(CAP, name + '.raw')
        if f.endswith('.txt') and os.path.exists(cap):
            clock = json.load(open(cap + '.json'))['clock']
            tests[name] = (script_bytes(os.path.join(SCRIPTS, f)), game_part(open(cap, 'rb').read()), clock)
    for k, keys in svhakeys.TESTS.items():
        cap = os.path.join(CAP, 'k_%s.raw' % k)
        if os.path.exists(cap):
            tests['keys_' + k] = (b''.join(keys) + b'QUIT\rYES\r', game_part(open(cap, 'rb').read()), None)
    cap = os.path.join(CAP, 'first_session.raw')
    if os.path.exists(cap):
        tests['first_session'] = (b'NO\rIN\rTAKE LAMP\rINVENTORY\rSCORE\rQUIT\rYES\r',
                                  game_part(open(cap, 'rb').read()), None)
    for name, (inp, ref, clock) in sorted(tests.items()):
        open(os.path.join(OUT, name + '.in'), 'wb').write(inp)
        open(os.path.join(OUT, name + '.ref'), 'wb').write(ref)
        if clock:
            open(os.path.join(OUT, name + '.clock'), 'w').write('%d\n' % clock)
        print('%-16s %6d bytes in, %6d bytes out%s' % (name, len(inp), len(ref),
                                                      ', clock %d' % clock if clock else ''))


if __name__ == '__main__':
    main()
