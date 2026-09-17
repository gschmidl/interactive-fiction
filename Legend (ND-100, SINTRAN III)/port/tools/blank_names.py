"""Take the club members' own names out of LEGEND's player files.

usage: python tools\\blank_names.py [--check]      (--check: report, write nothing)

Each player in SPELARE-n-LU.DATA carries, besides the character's name
(MEDL$, the one the game shows in a room and DODA takes), the name of the
member who made him: NAM$, asked for as "Vad heter du?" and printed by
VILKA.  Some twenty of the club's members wrote their real names there in
1986-87.  This puts "Namnl|s" - the game's own word for a player who gave
no name, and already in these files - in every one of those fields, in
..\\src_original\\data (the recovered files, which this repository
publishes) and so in everything built from them.  A field the game left
empty or NUL is not touched: that is not a name.

The character names stay: they are inventions (Djingis Klan, Tant Olga,
Biff Burkenzon), except two members who took their own name, and those
lose the surname.  The signature (up to four letters, PLAY$) stays.

What follows a file's end mark (027) is also cleared.  SINTRAN allocates
these files by the page and never shortens them, so the slack holds
whatever stood there before - in SPELARE-2 a whole deleted player, with
the name of a member who appears nowhere in the live records.  The game
reads none of it: it reads the count on the first line and that many
records.

The unedited files are on the author's floppies in
_ND100_work\\files\\lundin, which this repository does not publish.
Everything else the name scan found is the authors' own credit: "(C) A
Hedstr|m & M Lundin 84-87", the game's joke about "Eru Iluvatar alias
Magnus Lundin, creator av detta spel", and P{r Anders Nilsson's header in
his own mode file.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from convert_players import FIELDS, parity  # noqa: E402

PORT = os.path.dirname(HERE)
SRC = os.path.join(PORT, '..', 'src_original', 'data')

BLANK = 'Namnl|s'                      # what LEGEND itself writes (line 12520)
CHARACTER_NAMES = {'STEFAN KJELLBERT': 'STEFAN', 'OLA RANINGE': 'OLA'}
NAM = FIELDS.index('NAM')


def read(path):
    """(the file's lines, its end mark or b'', its length on the disk)"""
    raw = open(path, 'rb').read()
    low = bytes(b & 0x7f for b in raw)
    end = low.find(b'\x17')
    mark = b'\x17' if end >= 0 else b''
    text = low[:end] if end >= 0 else low.rstrip(b'\0')
    return text.decode('latin-1').split('\r\n'), mark, len(raw)


def blank(lines):
    """Blank NAM$ in every record, and drop what follows the last one;
    returns what changed."""
    count = int(lines[0])
    at, changed = 1, []
    for _ in range(count):
        rec = dict(zip(FIELDS, lines[at:at + len(FIELDS)]))
        if rec['NAM'] == BLANK and rec['MU'] == '' and lines[at + 6] == '\0':
            at += 1                    # v9.11's extra empty HEISSE$ (SPELARE-4)
            rec = dict(zip(FIELDS, lines[at:at + len(FIELDS)]))
        if rec['MEDL'] in CHARACTER_NAMES:
            changed.append('%s -> %s (the character)' % (rec['MEDL'], CHARACTER_NAMES[rec['MEDL']]))
            lines[at] = CHARACTER_NAMES[rec['MEDL']]
        if rec['NAM'] not in ('', '\0', BLANK):
            changed.append('%s -> %s' % (rec['NAM'], BLANK))
            lines[at + NAM] = BLANK
        at += len(FIELDS)
    if len(lines) > at + 1:
        # Players the members removed are still there, after the count the
        # game reads: it writes the file from the beginning and leaves the
        # rest of it standing.  SPELARE-1 keeps six of them.
        changed.append('%d lines of deleted players dropped' % (len(lines) - at - 1))
        del lines[at:]
        lines.append('')               # the last record ends with CR LF too
    return changed


def main():
    check = '--check' in sys.argv
    for n in range(1, 13):
        path = os.path.join(SRC, 'SPELARE-%d-LU.DATA' % n)
        if not os.path.exists(path):
            continue
        lines, mark, size = read(path)
        changed = blank(lines)
        new = parity('\r\n'.join(lines)) + mark
        if len(new) > size:
            raise SystemExit('%s: %d bytes will not fit in %d' % (path, len(new), size))
        new += b'\0' * (size - len(new))          # the slack, cleared
        old = open(path, 'rb').read()
        print('SPELARE-%-2d %-11s %s' % (
            n, '%d names' % len(changed) if changed else 'no names',
            'as it should be' if new == old else
            ('would be rewritten' if check else 'rewritten')))
        for c in changed:
            print('           ', c)
        if not check and new != old:
            open(path, 'wb').write(new)
    return 0


if __name__ == '__main__':
    sys.exit(main())
