#!/usr/bin/env python3
"""Decode tapecave/DATABASE.dat into the plain-text game database.

DATABASE.dat is the Adventure game database, deliberately disguised on the
tape to look like a linkage-editor object deck so that browsing the dataset
on the mainframe would not spoil the game.  The disguise is undone by the
REVERT procedure inside the game itself (adventure.pli, "REVERT: PROCEDURE"),
which the wizard-mode DECOD command runs in place.  This is a line-for-line
transcription of that procedure:

    REVERT: PROCEDURE;
            IF SUBSTR(CARD,2,2) = 'EN'
            THEN DO;
                 COLS(4) = -COLS(4);
                 SUBSTR(CARD,33,20) = ' ';
                 END;
            ELSE DO I = 3 TO 36;
                 IF COLS(I) = 0
                 THEN SUBSTR(CARD,I*2-1,2) = ' ';
                 ELSE COLS(I) = - COLS(I);
                 END;
            SUBSTR(CARD,1,4) = ' ';
            IF SUBSTR(CARD,73,2) = 'PR'  THEN SUBSTR(CARD,73,2) = ' ';
                                         ELSE COLS(37) = -COLS(37);
            IF SUBSTR(CARD,75,2) = 'OG'  THEN SUBSTR(CARD,75,2) = ' ';
                                         ELSE COLS(38) = -COLS(38);
            IF SUBSTR(CARD,77,2) >= '00' THEN SUBSTR(CARD,77,2) = ' ';
                                         ELSE COLS(39) = -COLS(39);
            IF SUBSTR(CARD,79,2) >= '00' THEN SUBSTR(CARD,79,2) = ' ';
                                         ELSE COLS(40) = -COLS(40);
            END REVERT;

CARD is CHARACTER(80); COLS(40) is FIXED BIN(15) BASED(ADDR(CARD)), i.e. 40
signed big-endian halfwords overlaying those 80 bytes.  All positions here
are PL/I's, so 1-based, and all blanks are EBCDIC blanks (X'40') because
REVERT operates on the untranslated EBCDIC card image.

The 29 leading ESD cards are dropped: the game's own read loop skips them
("IF SUBSTR(CARD,3,1) = 'S'  /* IGNORE ESD CARD */  THEN GO TO L1002") and
they are linkage-editor padding, not game content.

Output is written as latin-1, one byte per character, so that each record is
still exactly 80 columns wide once build_object.py pads it -- the engine
parses section text with the picture (F(8),14 A(5),A(2)) and calls BUG(0) if
columns 79-80 are not blank, so a multi-byte encoding here would shift text
into them.  Only four bytes in the whole database fall outside ASCII: X'4A'
(cent sign) twice and X'5F' (logical not) twice.
"""
import os
import sys

EBCDIC = 'cp037'
BLANK = 0x40                     # EBCDIC ' '
HEX02 = 0x02                     # the "card is encoded" marker in column 1


def cols(card, i):               # COLS(i), 1-based, signed halfword
    j = (i - 1) * 2
    return int.from_bytes(card[j:j + 2], 'big', signed=True)


def negate(card, i):             # COLS(i) = -COLS(i)
    j = (i - 1) * 2
    card[j:j + 2] = ((-cols(card, i)) & 0xFFFF).to_bytes(2, 'big')


def substr(card, pos, length):   # SUBSTR(CARD,pos,length)
    return card[pos - 1:pos - 1 + length]


def blank(card, pos, length):    # SUBSTR(CARD,pos,length) = ' '
    card[pos - 1:pos - 1 + length] = bytes([BLANK]) * length


def revert(card):
    if substr(card, 2, 2) == 'EN'.encode(EBCDIC):
        negate(card, 4)
        blank(card, 33, 20)
    else:
        for i in range(3, 37):
            if cols(card, i) == 0:
                blank(card, i * 2 - 1, 2)
            else:
                negate(card, i)
    blank(card, 1, 4)
    if substr(card, 73, 2) == 'PR'.encode(EBCDIC):
        blank(card, 73, 2)
    else:
        negate(card, 37)
    if substr(card, 75, 2) == 'OG'.encode(EBCDIC):
        blank(card, 75, 2)
    else:
        negate(card, 38)
    if substr(card, 77, 2) >= '00'.encode(EBCDIC):
        blank(card, 77, 2)
    else:
        negate(card, 39)
    if substr(card, 79, 2) >= '00'.encode(EBCDIC):
        blank(card, 79, 2)
    else:
        negate(card, 40)
    return card


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', 'DATABASE.dat')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(here, 'decoded_database.txt')

    data = open(src, 'rb').read()
    if len(data) % 80:
        sys.exit(f'{src}: {len(data)} bytes is not a whole number of 80-byte records')

    kept = 0
    skipped = 0
    with open(dst, 'w', encoding='latin-1', newline='\n') as out:
        for i in range(0, len(data), 80):
            card = bytearray(data[i:i + 80])
            if substr(card, 3, 1) == 'S'.encode(EBCDIC):   # ESD card
                skipped += 1
                continue
            if card[0] == HEX02:
                revert(card)
            out.write(card.decode(EBCDIC).rstrip() + '\n')
            kept += 1
    print(f'{dst}: {kept} records ({skipped} ESD cards skipped)')


if __name__ == '__main__':
    main()
