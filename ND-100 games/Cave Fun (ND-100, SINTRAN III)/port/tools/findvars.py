"""Find where a build of the interpreter keeps ROOM, OROOM and its arrays.

usage: python tools\\findvars.py [PROG [ADVFILE]]

Plays a few moves with --debug, dumping memory in between (#dump), and
works out from the differences:
  ROOM, OROOM   the room the player is in, and the one last described
  ADV           ADV(0,0): ADV(I,J) is at ADV + 351*J + I (by columns; I a
                room, an object or a rule, J 0-10 a room's exits and light,
                11-12 an object's place and value, 13-28 a rule)
  BEEN, SF      BEEN(0) and SF(0): rooms seen, and the special flags
All octal.  tests\\fixtest.py uses them to put the player where a fix shows.
"""
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from runport import run, PORT  # noqa: E402


def find(prog=None, adv=None, name='CAVE-FUN'):
    tmp = tempfile.mkdtemp(prefix='cavefun-vars-')
    d = [os.path.join(tmp, 'm%d.bin' % i) for i in range(4)]
    keys = [name, '#dump ' + d[0], 'N', '#dump ' + d[1], 'GET LAMP', '#dump ' + d[2],
            'S', 'SW', 'GET MATCHES', 'ON LAMP', '#dump ' + d[3], 'END', 'N']
    run(''.join(k + '\r' for k in keys).encode('latin-1'), prog=prog, adv=adv, extra=['--debug'],
        name=name)
    m = [struct.unpack('>65536H', open(f, 'rb').read()) for f in d]
    room = [a for a in range(65536) if m[0][a] == 1 and m[1][a] == 3]
    exits = [3, 0, 2, 6, 0, 4, 5, 0, 0, 0, 1]              # room 1: N NE E SE S SW W NW U D, light
    adv = [a - 1 for a in range(65536 - 3510) if all(m[0][a + 351 * k] == exits[k] for k in range(11))]
    been = [a - 3 for a in range(2, 65536) if m[0][a] == 0 and m[1][a] == 1 and m[0][a - 2] == 1]
    lamp_on = [a - 1 for a in range(1, 65536) if m[2][a] == 0 and m[3][a] == 1]
    assert len(room) == 2 and len(adv) == 1 and len(been) == 1, (room, adv, been)
    sf = [a for a in lamp_on if 351 <= been[0] - a <= 400]    # the array DIMmed before BEEN
    assert len(sf) == 1, sf
    lamp_place = adv[0] + 351 * 11 + 1
    assert m[1][lamp_place] == 3 and m[2][lamp_place] == 0xFFFF
    return {'ROOM': room[0], 'OROOM': room[1], 'ADV': adv[0], 'BEEN': been[0], 'SF': sf[0]}


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else None
    adv = sys.argv[2] if len(sys.argv) > 2 else None
    for k, v in find(prog, adv).items():
        print('%-6s %06o' % (k, v))


if __name__ == '__main__':
    main()
