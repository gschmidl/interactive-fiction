"""Compare the English image with the original in every room of the game.

    python deepcheck.py [COMMANDS-PER-ROOM] [SEEDS]

check.py compares with the recorded sessions, but they stay near the start.
Here both images are suspended with the lamp lit, the suspended games are poked
into each room in turn (LOC, NEWLOC and the OLDLOCs), and the same random
commands from the original's word table, put into English, are typed into both.
The Norwegian transcript, translated by message, must be the English one.  At
the end it lists the messages no room ever printed.
"""
import os
import random
import re
import struct
import subprocess
import sys
import tempfile

import check

LOCATIONS = (0o22163, 0o22167, 0o22171, 0o22201, 0o63171)
ROOMS = 184
CLOCK = ['-Z', '1789632000']
# a word that ends or suspends the game is no use here
AVOID = {'SLUTT', 'SPAR', 'UTSETT', 'PAUSE', 'SELVMO'}
START_NO = ['NEI', 'INN', 'TA LYKT', 'TA N\\KLER', 'TENN', 'SPAR', 'JA']


def run(args, typed):
    p = subprocess.run([check.EXE, '--raw', '-u'] + CLOCK + args, input='\r'.join(typed).encode('latin-1') + b'\r',
                       stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=60)
    return check.lines_of(p.stdout)


def poke(path, out, room):
    data = bytearray(open(path, 'rb').read())
    for a in LOCATIONS:
        struct.pack_into('>H', data, 0x200 + 2 * a, room)
    open(out, 'wb').write(data)


def main():
    per_room = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    seeds = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    objects = [w for w, k in check.vocabulary(check.ORIG) if 1000 <= k < 2000]
    verbs = [w for w, k in check.vocabulary(check.ORIG) if 2000 <= k < 3000 and w not in AVOID]
    motions = [w for w, k in check.vocabulary(check.ORIG) if k < 1000]
    bad = 0
    printed = set()
    en_lines = {}
    for idx, text in check.MSG_EN.items():
        en_lines.setdefault(check.norm(text[0]) if text else '', idx)
    with tempfile.TemporaryDirectory() as tmp:
        run(['-s', tmp, '--prog', check.ORIG], START_NO + ['no'])
        typed = [' '.join(check.english_word(x) for x in c.split(' ')) for c in START_NO]
        run(['-s', tmp, '--prog', check.ENGL], typed + ['en'])
        for seed in range(seeds):
            for room in range(1, ROOMS + 1):
                rnd = random.Random(room * 1000 + seed)
                cmds = []
                for _ in range(per_room):
                    r = rnd.random()
                    if r < .35:
                        cmds.append(rnd.choice(motions))
                    elif r < .75:
                        cmds.append(rnd.choice(verbs) + ' ' + rnd.choice(objects))
                    elif r < .85:
                        cmds.append(rnd.choice(objects))
                    elif r < .95:
                        cmds.append(rnd.choice(verbs))
                    else:
                        cmds.append(rnd.choice(['J', 'NEI']))
                poke(os.path.join(tmp, 'no.PROG'), os.path.join(tmp, 'pno.PROG'), room)
                poke(os.path.join(tmp, 'en.PROG'), os.path.join(tmp, 'pen.PROG'), room)
                no = run(['-s', tmp, 'pno'], cmds)
                en = run(['-s', tmp, 'pen'], [' '.join(check.english_word(x) for x in c.split(' ')) for c in cmds])
                expect = []
                for kind, item in check.translate_transcript(no, check.MSG_NO, check.MSG_EN):
                    if kind == 'msg':
                        expect += item
                    else:
                        expect.append(check.translate_line(item))
                got = [check.norm(x) for x in en]
                for x in got:
                    if x in en_lines:
                        printed.add(en_lines[x])
                if [x.upper() for x in expect] != [x.upper() for x in got]:
                    bad += 1
                    for i, (a, b) in enumerate(zip(expect, got)):
                        if a.upper() != b.upper():
                            print('room %d seed %d line %d:\n  expected %s\n  got      %s' % (room, seed, i, a, b))
                            break
                    else:
                        print('room %d seed %d: %d lines against %d' % (room, seed, len(expect), len(got)))
    print('\n%d of %d runs differ' % (bad, ROOMS * seeds))
    missing = [i for i, t in check.MSG_EN.items() if i not in printed and t and not t[0].startswith('>$<') and not t[0].startswith('*')]
    print('%d of %d messages seen; not seen:' % (len(check.MSG_EN) - len(missing), len(check.MSG_EN)))
    for i in missing:
        print('  #%d %s' % (i, check.MSG_EN[i][0][:68]))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
