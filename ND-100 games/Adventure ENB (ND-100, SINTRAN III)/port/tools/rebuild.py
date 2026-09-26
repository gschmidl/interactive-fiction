"""Build ADVENTURE-ENB on the reference machine.

usage: python tools\\rebuild.py [CMDFILE ...]   (RetroCore must not be running)

1. writes to the pack (user DNF):
     ADVENTURE-ENB:SYMB   the program as recovered (..\\src_original\\)
     AENBF:SYMB           the program with the port's fixes (basic\\)
     AENBS:SYMB           the program as recovered with only the three error
                          handlers mended (131, 150, 2970), so that it starts
                          without the lost instruction files: the port's other
                          fixes are shown against it (tests\\fixtest.py)
     LIBRARY-MJ:BRF       the club library as it survives (NILSSON-3)
     BASLIBR-H00:BRF      ND BASIC's run-time library (as the Legend port rebuilt it)
2. starts RetroCore and runs tools\\build.cmd: the BASIC compiler of January
   1985 (TABLE-SIZES 1500,35000, numbers real as the program's % marks want),
   each program in a BASIC session of its own, then NRL (SIZE 2700) with the
   program, the club library and BASLIBR-H00; command files named after it
   are run in the same session (their logs beside them);
3. stops RetroCore and takes the programs, object files and listings off the
   pack: build\\ and data\\ (the fixed program), build\\original\\ and
   build\\start\\;
4. writes AENBF-REF, AENB-REF and AENBS-REF:PROG to the pack: the programs with
   RANDOM not stirring in the uptime (MON 11), so that the reference machine
   plays the same game for the same typing (NOTES.md).
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
ORIG = os.path.join(PORT, '..', 'src_original')


def rd(*p):
    return open(os.path.join(*p), 'rb').read()


def ref_of(prog):
    """RANDOM ignores the uptime: its MON 11 and the COPY SD DA after it become
    SAA 0 (as the Legend port's reference programs)"""
    w = list(struct.unpack('>%dH' % (len(prog) // 2), prog))
    seq = [0o153011, 0o146115, 0o171756, 0o006600]
    hits = [i for i in range(0x100, len(w) - 4) if w[i:i + 4] == seq]
    assert len(hits) == 1, hits
    w[hits[0]] = w[hits[0] + 1] = 0o170400
    return struct.pack('>%dH' % len(w), *w)


HANDLERS = (131, 136, 137, 138, 150, 211, 212, 2970, 3041, 3042)


def startable():
    """the recovered source with the fixed program's lines HANDLERS in it"""
    def lines(text):
        return {int(l.split(' ', 1)[0]): l for l in text.split('\n') if l[:1].isdigit()}
    orig = lines(mksymb.to_text(rd(ORIG, 'ADVENTURE-ENB.SYMB')))
    fixed = lines(open(os.path.join(PORT, 'basic', 'ADVENTURE-ENB.txt'), encoding='latin-1').read())
    for n in HANDLERS:
        orig[n] = fixed[n]
    return mksymb.convert('\n'.join(orig[n] for n in sorted(orig)) + '\n')


def main():
    rc.pack_write({
        'DNF/ADVENTURE-ENB:SYMB': rd(ORIG, 'ADVENTURE-ENB.SYMB'),
        'DNF/AENBF:SYMB': rd(PORT, 'basic', 'ADVENTURE-ENB.SYMB'),
        'DNF/AENBS:SYMB': startable(),
        'DNF/LIBRARY-MJ:BRF': rd(BUILD, 'LIBRARY-MJ.BRF'),
        'DNF/BASLIBR-H00:BRF': rd(BUILD, 'BASLIBR-H00.BRF'),
    })
    with rc.Machine():
        out = rc.session(os.path.join(rc.HERE, 'build.cmd'), os.path.join(BUILD, 'build.log'), 1200)
    print('\n'.join(l for l in out.split('\n')
                    if 'COMPILED' in l or 'FREE:' in l or ' U ' in l or 'ERROR' in l.upper()))
    os.makedirs(os.path.join(BUILD, 'original'), exist_ok=True)
    refs = {}
    os.makedirs(os.path.join(BUILD, 'start'), exist_ok=True)
    for prog, lst, dest in (('ADVENTURE-ENB', 'AENBF', BUILD), ('AENB', 'AENB', os.path.join(BUILD, 'original')),
                            ('AENBS', 'AENBS', os.path.join(BUILD, 'start'))):
        p = rc.pack_read('DNF/%s:PROG' % prog)
        open(os.path.join(dest, 'ADVENTURE-ENB.PROG'), 'wb').write(p)
        open(os.path.join(dest, 'ADVENTURE-ENB.BRF'), 'wb').write(rc.pack_read('DNF/%s:BRF' % lst))
        listing = rc.text(rc.pack_read('DNF/%s:LIST' % lst))
        open(os.path.join(dest, 'ADVENTURE-ENB.LIST.txt'), 'w', encoding='latin-1', newline='\n').write(listing)
        errors = [l for l in listing.split('\n') if l.startswith('*** ERROR')]
        if errors:
            sys.exit('%s:LIST:\n%s' % (lst, '\n'.join(errors)))
        refs['DNF/%s-REF:PROG' % lst] = ref_of(p)
        print('%-40s %6d bytes, sha1 %s' % (os.path.relpath(os.path.join(dest, 'ADVENTURE-ENB.PROG'), PORT),
                                             len(p), hashlib.sha1(p).hexdigest()[:12]))
    rc.pack_write(refs)
    shutil.copyfile(os.path.join(BUILD, 'ADVENTURE-ENB.PROG'), os.path.join(PORT, 'data', 'ADVENTURE-ENB.PROG'))
    if sys.argv[1:]:
        with rc.Machine():
            for cmd in sys.argv[1:]:
                rc.session(cmd, os.path.splitext(cmd)[0] + '.log', 900)


if __name__ == '__main__':
    main()
