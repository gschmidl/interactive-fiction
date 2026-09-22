#!/usr/bin/env python3
"""A game's state by name: a saved game, or NEUSPIEL.DAT.

A save is the main program's eleven stretches of COMMON one after
another, laid out as its declarations say (src\\neuspiel.py reads them from
the converted main.f).  State gives every element by name - s['LOC'],
s['PLACE', 50] - and has the program's own CARRY, DROP and MOVE, so that
a test can set a game up the way the program would have left it.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
MAIN_F = os.path.join(PORT, '.build', 'src', 'main.f')
sys.path.insert(0, os.path.join(PORT, 'src'))
import neuspiel  # noqa: E402


def names(main_f=MAIN_F):
    """(byte offset, size, name, index) of every element saved, in file
    order; index is 0 for a variable that is not an array"""
    lay, ranges = neuspiel.layout(neuspiel.statements(main_f))
    out = []
    at = 0
    for first, i, last, j in ranges:
        blk, _, pb0, k0, _ = lay[first]
        _, _, pb1, k1, _ = lay[last]
        size0 = 8 if k0 == 'INTEGER*6' else 4
        size1 = 8 if k1 == 'INTEGER*6' else 4
        start, end = pb0 + size0 * (i - 1), pb1 + size1 * j
        for n, (b, _, pb, k, cnt) in sorted(lay.items(), key=lambda x: x[1][2]):
            if b != blk:
                continue
            e = 8 if k == 'INTEGER*6' else 4
            for x in range(cnt):
                p = pb + e * x
                if start <= p < end:
                    out.append((at + p - start, e, n, x + 1 if cnt > 1 else 0))
        at += end - start
    return out


class State:
    def __init__(self, path, main_f=MAIN_F):
        self.data = bytearray(open(path, 'rb').read())
        self.at = {(n, i): (o, s) for o, s, n, i in names(main_f)}
        if sum(s for o, s in self.at.values()) != len(self.data):
            raise ValueError('%s is not a game of this build' % path)

    def _key(self, key):
        return key if isinstance(key, tuple) else (key, 0)

    def __getitem__(self, key):
        o, s = self.at[self._key(key)]
        return struct.unpack('<q' if s == 8 else '<i', self.data[o:o + s])[0]

    def __setitem__(self, key, value):
        o, s = self.at[self._key(key)]
        self.data[o:o + s] = struct.pack('<q' if s == 8 else '<i', value)

    def save(self, path):
        open(path, 'wb').write(self.data)

    # the program's CARRY, DROP and MOVE (an OBJECT over 100 is the
    # second place of object OBJECT-100)
    def carry(self, obj, where):
        if obj <= 100:
            if self['PLACE', obj] == -1:
                return
            self['PLACE', obj] = -1
            self['HOLDNG'] = self['HOLDNG'] + 1
        if self['ATLOC', where] == obj:
            self['ATLOC', where] = self['LINK', obj]
            return
        t = self['ATLOC', where]
        while self['LINK', t] != obj:
            t = self['LINK', t]
        self['LINK', t] = self['LINK', obj]

    def drop(self, obj, where):
        if obj <= 100:
            if self['PLACE', obj] == -1:
                self['HOLDNG'] = self['HOLDNG'] - 1
            self['PLACE', obj] = where
        else:
            self['FIXED', obj - 100] = where
        if where <= 0:
            return
        self['LINK', obj] = self['ATLOC', where]
        self['ATLOC', where] = obj

    def move(self, obj, where):
        frm = self['PLACE', obj] if obj <= 100 else self['FIXED', obj - 100]
        if 0 < frm <= 300:
            self.carry(obj, frm)
        self.drop(obj, where)

    def here(self, where):
        """the objects at a place, as the program lists them"""
        out, o = [], self['ATLOC', where]
        while o:
            out.append(o)
            o = self['LINK', o]
        return out
