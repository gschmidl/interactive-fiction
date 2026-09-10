#!/usr/bin/env python3
"""Dump the game's message table out of the repaired core image.

The messages live in the low segment as a linked list.  Each node is one
link word followed by the text of that line packed five 7-bit characters
to a 36-bit word.  The link is +/- an offset from a fixed base; the sign
marks the first line of a message, so a message runs from a node with a
negative link through every following node with a positive one.  The
list ends with a link of -1.
"""
import os, sys

RAW = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'raw')
BASE = 33                        # file index the links are relative to
FIRST = 34                       # first node
LOST, LOSTSRC = 3674, 3684       # the word dropped in transfer

def words(path):
    data = open(path, 'rb').read()
    out = []
    for i in range(0, len(data) - len(data) % 5, 5):
        b = data[i:i+5]
        w = 0
        for j in range(5):
            w = (w << 7) | (b[j] & 0x7f)
        out.append((w << 1) | ((b[4] >> 7) & 1))
    return out

def text(w):
    return ''.join(chr((w >> (29 - 7*k)) & 0x7f) for k in range(5))

def main():
    low = words(os.path.join(RAW, 'blocke.low'))
    if text(low[LOST]) != ' nort':
        low.insert(LOST, low[LOSTSRC])

    def signed(w):
        return w - (1 << 36) if w & (1 << 35) else w

    out, node = [], FIRST
    while True:
        link = signed(low[node])
        body = ''.join(text(low[k]) for k in range(node + 1, BASE + abs(link)))
        out.append((link, body))
        if link == -1 or BASE + abs(link) <= node:
            break
        node = BASE + abs(link)

    msg = []
    for link, body in out:
        if link < 0 and msg:
            print(''.join(msg).rstrip())
            msg = []
        msg.append(body)
    if msg:
        print(''.join(msg).rstrip())

main()
