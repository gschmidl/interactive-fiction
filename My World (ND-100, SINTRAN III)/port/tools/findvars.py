"""Find where MY_WORLD keeps the player's room and the things, for --debug
pokes (tests\\fixtest.py and tests\\winnable.py use the addresses).

usage: python tools\\findvars.py [PROG]

Starts the game in the reference world (-Z), takes a memory dump (#dump),
goes north and takes another.  Prints, in octal:

  SAK       SAK[1].RUM, the first thing's room: SAK is an array of records of
            115 words, the room last (value, weight, room), so SAK[K].RUM is
            at SAK + 115*(K-1); found as the 22 rooms the world file gives
  RUMNR     the player's room: the word that went from a room to the room
            north of it (by the world file's exits)
  BEAR ... BOLA
            the BOOLEANs, one word each, the other way round from their
            declaration: found by ROST and SNAKE, TRUE at the start, near RUMNR
"""
import os
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'myworld.exe')
DATA = os.path.join(PORT, 'data')
SIZE = 115


def world():
    """rooms' exits and the things' rooms, from the world file"""
    t = bytes(b & 0x7f for b in open(os.path.join(DATA, 'MY-DATA-FILE-MJ.ADV'), 'rb').read())
    lines = t.decode('latin-1').split('\x17')[0].replace('\r\n', '\n').lstrip(' ').split('\n')
    nrum, nsak = [int(x) for x in lines[0].split(',')[:2]]
    i, exits, rooms = 1, {}, []
    for r in range(1, nrum + 1):
        i += 1
        while not lines[i].startswith('#'):
            i += 1
        exits[r] = [int(x) for x in lines[i + 1].split(',')]
        i += 3
    for s in range(nsak):
        rooms.append(int(lines[i + 3].split(',')[0]))
        i += 4
    return exits, rooms


def load(path):
    b = open(path, 'rb').read()
    return struct.unpack('>%dH' % (len(b) // 2), b)


def find(prog):
    work = tempfile.mkdtemp(prefix='myworldvars')
    d0, d1 = os.path.join(work, 'd0.bin'), os.path.join(work, 'd1.bin')
    keys = '#dump %s\rN\r#dump %s\rQUIT\r' % (d0.replace('\\', '/'), d1.replace('\\', '/'))
    subprocess.run([EXE, '--data', DATA, '--prog', prog, '--raw', '--no-hold', '-Z', '1', '--debug'],
                   input=keys.encode('latin-1'), stdout=subprocess.PIPE, timeout=60)
    w0, w1 = load(d0), load(d1)
    exits, rooms = world()
    found = {}
    hits = [i for i in range(len(w0) - SIZE * len(rooms))
            if all(w0[i + SIZE * k] == rooms[k] for k in range(len(rooms)))]
    assert len(hits) == 1, hits
    found['SAK'] = hits[0]
    hits = [i for i in range(len(w0)) if 1 <= w0[i] <= len(exits) and w1[i] == exits[w0[i]][0] != 0
            and w1[i] != w0[i]]
    assert len(hits) == 1, [(oct(i), w0[i], w1[i]) for i in hits]
    found['RUMNR'] = hits[0]
    # the BOOLEANs D\D ... BEAR lie the other way round: BEAR, ROST, SNAKE,
    # BRIDGE, SLUTA, ORC, GATE, BOLA, MOVED, D\D; at the start only ROST and
    # SNAKE are TRUE
    near = range(found['RUMNR'] - 64, found['RUMNR'] + 64)
    hits = [i for i in near if list(w0[i:i + 8]) == [0, 1, 1, 0, 0, 0, 0, 0]]
    assert len(hits) == 1, hits
    for k, name in enumerate(('BEAR', 'ROST', 'SNAKE', 'BRIDGE', 'SLUTA', 'ORC', 'GATE', 'BOLA')):
        found[name] = hits[0] + k
    return found


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else os.path.join(DATA, 'ADVENTURE-MJ.PROG')
    for name, a in find(prog).items():
        print('%-6s %o' % (name, a))


if __name__ == '__main__':
    main()
