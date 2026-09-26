"""Build MY_WORLD (ADVENTURE-MJ) on the reference machine.

usage: python tools\\rebuild.py [CMDFILE ...]   (RetroCore must not be running)

1. writes to the pack (user DNF):
     ADVENTURE-MJ:SYMB     the source as recovered: MIKAEL-6's, with its bad
                           sector read again (pascal\\recovered\\, tools\\fix_sector.py)
     AMJF:SYMB             the source with the port's fixes (pascal\\)
     AMJS:SYMB             the source as recovered with only lines 263, 289 and 741
                           mended, so that it gets past the first command: the
                           other fixes are shown against it (tests\\fixtest.py)
     MY-DATA-FILE-MJ:ADV   the world, under the name the program opens (on
                           MIKAEL-6 it is ADVENTURE-MJ:DATA)
2. starts RetroCore and runs tools\\build.cmd: ND-Pascal J (the club's, as for
   Mordor), each source in a session of its own, then NRL: SIZE 1500, the
   program, the club's EXTRA-PAS-LIB (RAN) and PASCAL-LIB-J; command files
   named after it run in the same session;
3. stops RetroCore and takes the programs, object files and listings off the
   pack: build\\ and data\\ (the fixed program), build\\original\\ and
   build\\start\\;
4. writes AMJF-REF, AMJ-REF and AMJS-REF:PROG, the programs with RAN not
   stirring in the uptime (MON 11), for repeatable reference games.
"""
import hashlib
import os
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rc  # noqa: E402
import mksymb  # noqa: E402

PORT = rc.PORT
BUILD = os.path.join(PORT, 'build')
MENDED = (263, 289, 741)


def ref_of(prog):
    """RAN ignores the uptime: its MON 11 and the COPY SD DA after it become SAA 0"""
    w = list(struct.unpack('>%dH' % (len(prog) // 2), prog))
    hits = [i for i in range(0x100, len(w) - 2) if w[i:i + 2] == [0o153011, 0o146115]]
    assert len(hits) == 1, hits
    w[hits[0]] = w[hits[0] + 1] = 0o170400
    return struct.pack('>%dH' % len(w), *w)


def startable():
    """the recovered source with the fixed source's lines MENDED in it (the two
    have the same lines, one for one)"""
    rec = mksymb.to_text(open(os.path.join(PORT, 'pascal', 'recovered', 'ADVENTURE-MJ.SYMB'), 'rb').read())
    fixed = open(os.path.join(PORT, 'pascal', 'ADVENTURE-MJ.txt'), encoding='latin-1').read()
    rec, fixed = rec.split('\n'), fixed.split('\n')
    assert len(rec) == len(fixed)
    for n in MENDED:
        rec[n - 1] = fixed[n - 1]
    return mksymb.convert('\n'.join(rec))


def prepare():
    rc.pack_write({
        'DNF/ADVENTURE-MJ:SYMB': open(os.path.join(PORT, 'pascal', 'recovered', 'ADVENTURE-MJ.SYMB'), 'rb').read(),
        'DNF/AMJF:SYMB': open(os.path.join(PORT, 'pascal', 'ADVENTURE-MJ.SYMB'), 'rb').read(),
        'DNF/AMJS:SYMB': startable(),
        'DNF/MY-DATA-FILE-MJ:ADV': open(os.path.join(PORT, 'data', 'MY-DATA-FILE-MJ.ADV'), 'rb').read(),
    })


def build():
    out = rc.session(os.path.join(rc.HERE, 'build.cmd'), os.path.join(BUILD, 'build.log'), 1200)
    print('\n'.join(l for l in out.split('\n') if 'ERRORS' in l or 'LENGTH' in l or 'FREE:' in l
                    or ' U ' in l or 'NON-STANDARD' in l))


def collect():
    refs = {}
    for prog, lst, dest, ref in (('ADVENTURE-MJ', 'AMJF', BUILD, 'AMJF-REF'),
                                 ('AMJ', 'AMJ', os.path.join(BUILD, 'original'), 'AMJ-REF'),
                                 ('AMJS', 'AMJS', os.path.join(BUILD, 'start'), 'AMJS-REF')):
        os.makedirs(dest, exist_ok=True)
        p = rc.pack_read('DNF/%s:PROG' % prog)
        open(os.path.join(dest, 'ADVENTURE-MJ.PROG'), 'wb').write(p)
        open(os.path.join(dest, 'ADVENTURE-MJ.BRF'), 'wb').write(rc.pack_read('DNF/%s:BRF' % lst))
        open(os.path.join(dest, 'ADVENTURE-MJ.LIST.txt'), 'w', encoding='latin-1', newline='\n').write(
            rc.text(rc.pack_read('DNF/%s:LIST' % lst)))
        refs['DNF/%s:PROG' % ref] = ref_of(p)
        print('%-36s %6d bytes, sha1 %s' % (os.path.relpath(os.path.join(dest, 'ADVENTURE-MJ.PROG'), PORT),
                                             len(p), hashlib.sha1(p).hexdigest()[:12]))
    rc.pack_write(refs)
    shutil.copyfile(os.path.join(BUILD, 'ADVENTURE-MJ.PROG'), os.path.join(PORT, 'data', 'ADVENTURE-MJ.PROG'))


def main():
    prepare()
    with rc.Machine():
        build()
        for cmd in sys.argv[1:]:
            rc.session(cmd, os.path.splitext(cmd)[0] + '.log', 900)
    collect()


if __name__ == '__main__':
    main()
