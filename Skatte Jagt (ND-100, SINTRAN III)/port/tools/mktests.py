"""Turn RetroCore captures into the regression tests in tests/ref.

usage: python tools/mktests.py

For each capture in tests/captures writes tests/ref/NAME.in (the bytes
typed) and tests/ref/NAME.ref (what SINTRAN III sent back, from the start of
the game until the game stopped or the input ran out).  tests/run.py replays
them.

  NAME.raw + tests/scripts/NAME.txt   refcap.py sessions (walk1, fuzzN)
  k_*.raw                             refkeys.py editing-key sessions
  first_session.raw                   the first session, typed by hand
"""
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
CAP = os.path.join(ROOT, 'tests', 'captures')
SCRIPTS = os.path.join(ROOT, 'tests', 'scripts')
OUT = os.path.join(ROOT, 'tests', 'ref')

KEYS = {
    'lower': [b'nei\r', b'inn\r'],
    'del': [b'NEI\r', b'INNX\x7f\r'],
    'bs': [b'NEI\r', b'INNX\x08\r'],
    'ctla': [b'NEI\r', b'INNX\x01\r'],
    'ctlq': [b'NEI\r', b'XYZ\x11INN\r'],
    'long': [b'NEI\r', b'GAAAAAAAAAAA NORDDDDDDDDD\r'],
    'empty': [b'NEI\r', b'\r', b'   \r'],
}


def script_bytes(path):
    cmds = [l.rstrip('\r\n') for l in open(path, encoding='latin-1')]
    return b''.join(c.encode('latin-1') + b'\r' for c in cmds if not c.startswith('#'))


def game_part(raw):
    """from the first output of SKAT to where the program left (SINTRAN's CR LF before @)"""
    i = raw.find(b'SKAT\r\n')
    if i >= 0:
        raw = raw[i + 6:]
    j = raw.find(b'\r\r\n@')
    return raw[:j + 3] if j >= 0 else raw


def main():
    os.makedirs(OUT, exist_ok=True)
    tests = {}
    for f in sorted(os.listdir(SCRIPTS)):
        name = f[:-4]
        cap = os.path.join(CAP, name + '.raw')
        if f.endswith('.txt') and os.path.exists(cap):
            tests[name] = (script_bytes(os.path.join(SCRIPTS, f)), game_part(open(cap, 'rb').read()))
    for k, keys in KEYS.items():
        cap = os.path.join(CAP, 'k_%s.raw' % k)
        if os.path.exists(cap):
            tests['keys_' + k] = (b''.join(keys) + b'SLUTT\rJ\r', game_part(open(cap, 'rb').read()))
    cap = os.path.join(CAP, 'esc_test.raw')
    if os.path.exists(cap):
        raw = open(cap, 'rb').read()
        i = raw.find(b'SKAT\r\n') + 6
        tests['keys_esc'] = (b'\x1b', raw[i:raw.find(b'@', i)])
    cap = os.path.join(CAP, 'first_session.raw')
    if os.path.exists(cap):
        tests['first_session'] = (b'JA\rINN\rTA NOKKEL\rTA LYKT\rINVENTAR\rUT\rSCORE\rSLUTT\rJ\r',
                                  game_part(open(cap, 'rb').read()))
    for name, (inp, ref) in sorted(tests.items()):
        open(os.path.join(OUT, name + '.in'), 'wb').write(inp)
        open(os.path.join(OUT, name + '.ref'), 'wb').write(ref)
        print('%-16s %6d bytes in, %6d bytes out' % (name, len(inp), len(ref)))


if __name__ == '__main__':
    main()
