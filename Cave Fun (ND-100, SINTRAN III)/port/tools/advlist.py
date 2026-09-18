"""List an ADVENTURE V4.2 game file (NAME:ADV) readably: words, rooms,
objects, messages and rules, the way ADV-INTER-CB-MJ reads them.

usage: python tools\advlist.py [FILE]     (default ..\src_original\CAVE-FUN-MJ.ADV)

The file: a line of 8 numbers (verbs, nouns, rooms, objects, messages,
rules, most things carried, the treasure room), max(verbs,nouns)+1 lines
"VERB,NOUN" (a leading * marks a synonym of the word above), for rooms
0..N a description (* = printed as is, else after "YOU'RE IN A ") and 11
numbers (N NE E SE S SW W NW U D exits, then 1 if lit), objects 0..N
"DESCRIPTION,WORD,ROOM,VALUE" (room -1 carried, 0 out of play), the
messages, and one line of 16 numbers per rule: verb, noun (0 any), five
(condition, parameter) pairs, four actions.  Conditions of type 0 (PAR)
hold the actions' parameters, taken in order.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
COND = {0: 'PAR', 1: 'in room', 2: 'carrying', 3: 'flag set', 4: 'flag clear', 5: 'here',
        6: 'here or carried', 7: 'not here nor carried', 8: 'not here', 9: 'in play', 10: 'out of play',
        11: 'not carried', 12: 'not in room', 13: 'random % <', 14: 'treasures in room',
        15: 'no treasures in room', 16: 'light', 17: 'dark', 18: 'moved', 19: 'not moved'}
ACT = {201: ('go to room', 1), 202: ('set flag', 1), 203: ('clear flag', 1), 204: ('swap rooms of', 2),
       205: ('clear screen', 0), 206: ('bell', 0), 207: ('put object in room', 2), 208: ('score +', 1),
       209: ('remove object', 1), 210: ('swap rooms', 2), 211: ('get object', 1), 212: ('drop object', 1),
       213: ('describe room', 0), 214: ('drop all', 0), 215: ('max carry =', 1), 216: ('treasure room =', 1),
       217: ('score = 0', 0), 218: ('treasures from room to room', 2), 999: ('END GAME', 0)}


def nums(s):
    return [int(x) for x in s.replace(' ', '').split(',') if x != '']


def load(path):
    lines = open(path, 'rb').read().decode('latin-1')
    lines = ''.join(chr(ord(c) & 0x7f) for c in lines).replace('\r', '').split('\n')
    h = nums(lines[0])
    V, N, R, O, M, C, maxl, store = h
    i = 1
    J = max(V, N)
    voc = [lines[i + k].split(',', 1) for k in range(J + 1)]
    i += J + 1
    rooms = []
    for _ in range(R + 1):
        rooms.append((lines[i], nums(lines[i + 1])))
        i += 2
    objs = []
    for _ in range(O + 1):
        parts = lines[i].rsplit(',', 2)
        desc, word = parts[0].rsplit(',', 1)
        objs.append((desc.strip('"'), word.strip('"'), int(parts[1]), int(parts[2])))
        i += 1
    msgs = lines[i:i + M]
    i += M
    rules = [nums(lines[i + k]) for k in range(C)]
    return h, voc, rooms, objs, msgs, rules


def word(voc, k, col):
    """the word itself for index k (synonyms follow it with a *)"""
    if k < 0 or k >= len(voc):
        return '?%d' % k
    w = voc[k][col] if col < len(voc[k]) else ''
    return w or '-'


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', '..', 'src_original', 'CAVE-FUN-MJ.ADV')
    h, voc, rooms, objs, msgs, rules = load(path)
    V, N, R, O, M, C, maxl, store = h
    print('verbs %d nouns %d rooms %d objects %d messages %d rules %d; carry %d; treasures to room %d'
          % tuple(h))
    print('\nWORDS (verb | noun)')
    for k, v in enumerate(voc):
        print('%3d %-12s %s' % (k, v[0], v[1] if len(v) > 1 else ''))
    dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'U', 'D']
    print('\nROOMS')
    for k, (d, e) in enumerate(rooms):
        ex = ' '.join('%s>%d' % (dirs[j], e[j]) for j in range(10) if e[j])
        print('%3d %s%s\n      %s' % (k, '' if e[10] else '[dark] ', d, ex))
    print('\nOBJECTS (room, value)')
    for k, o in enumerate(objs):
        print('%3d %-40s %-10s room %4d value %d' % (k, o[0], o[1], o[2], o[3]))
    print('\nMESSAGES')
    for k, m in enumerate(msgs, 1):
        print('%3d %s' % (k, m))
    print('\nRULES')
    for k, r in enumerate(rules, 1):
        v, n = r[0], r[1]
        head = 'AUTO %d%%' % n if v == 0 else '%s %s' % (word(voc, v, 0), word(voc, n, 1) if n else '*')
        conds, pars = [], []
        for j in range(5):
            t, p = r[2 + 2 * j], r[3 + 2 * j]
            if t == 0:
                pars.append(p)
            else:
                what = COND.get(t, '?%d' % t)
                if t in (2, 5, 6, 7, 8, 9, 10, 11) and 0 <= p < len(objs):
                    what += ' ' + objs[p][0]
                elif t in (1, 12, 14, 15):
                    what += ' %d' % p
                elif t in (3, 4, 13):
                    what += ' %d' % p
                conds.append(what)
        acts = []
        pi = 0
        for a in r[12:16]:
            if a == 0:
                continue
            if a <= 200:
                acts.append('say %d "%s"' % (a, msgs[a - 1] if a - 1 < len(msgs) else '?'))
                continue
            name, n_p = ACT.get(a, ('?%d' % a, 0))
            ps = pars[pi:pi + n_p]
            pi += n_p
            acts.append(name + ('' if not ps else ' ' + ','.join(map(str, ps))))
        print('%3d %-18s if %s\n      then %s' % (k, head, ' & '.join(conds) or '-', '; '.join(acts)))


if __name__ == '__main__':
    main()
