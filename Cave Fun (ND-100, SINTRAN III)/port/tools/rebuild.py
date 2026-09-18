"""Build the adventure interpreter (and its editor) on the reference machine.

usage: python tools\\rebuild.py [CMDFILE ...]   (RetroCore must not be running)

Command files named after it are run in the same session once the build is
done (see _ND100_work\\tools\\ndsession.py), each with its log beside it.
Each program is compiled in a BASIC session of its own: the compiler carries
something over from one COMPILE to the next, and the result depends on it.

1. writes to the pack (user DNF):
     ADV-INTER-CB-MJ:SYMB   the interpreter as recovered (..\\src_original\\)
     AICF:SYMB              the interpreter with the port's fixes (basic\\)
     ADV-EDIT-CB-MJ:SYMB    the editor as recovered
     CAVE-FUN-MJ:ADV        the game with the port's fixes (data\\, tools\\fix_adv.py)
     CAVE-ORIG:ADV          the game as recovered
     LIBRARY-MJ:BRF         the club library as it survives (NILSSON-3)
     BASLIBR-H00:BRF        ND BASIC's run-time library (as the Legend port rebuilt it)
2. starts RetroCore and runs tools\\build.cmd: the BASIC compiler of January
   1985 with DEFAULT-INTEGER, then NRL (SIZE 2700) with the program, the club
   library and BASLIBR-H00, once for each program;
3. stops RetroCore and takes the programs, object files and listings off the
   pack: build\\ and data\\ (the fixed interpreter, the editor) and
   build\\original\\ (the interpreter as recovered).
"""
import hashlib
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rc  # noqa: E402

PORT = rc.PORT
BUILD = os.path.join(PORT, 'build')
ORIG = os.path.join(PORT, '..', 'src_original')


def rd(*p):
    return open(os.path.join(*p), 'rb').read()


def main():
    rc.pack_write({
        'DNF/ADV-INTER-CB-MJ:SYMB': rd(ORIG, 'ADV-INTER-CB-MJ.SYMB'),
        'DNF/AICF:SYMB': rd(PORT, 'basic', 'ADV-INTER-CB-MJ.SYMB'),
        'DNF/ADV-EDIT-CB-MJ:SYMB': rd(ORIG, 'ADV-EDIT-CB-MJ.SYMB'),
        'DNF/CAVE-FUN-MJ:ADV': rd(PORT, 'data', 'CAVE-FUN-MJ.ADV'),
        'DNF/CAVE-ORIG:ADV': rd(ORIG, 'CAVE-FUN-MJ.ADV'),
        'DNF/LIBRARY-MJ:BRF': rd(BUILD, 'LIBRARY-MJ.BRF'),
        'DNF/BASLIBR-H00:BRF': rd(BUILD, 'BASLIBR-H00.BRF'),
    })
    with rc.Machine():
        out = rc.session(os.path.join(rc.HERE, 'build.cmd'), os.path.join(BUILD, 'build.log'), 1200)
        for cmd in sys.argv[1:]:
            rc.session(cmd, os.path.splitext(cmd)[0] + '.log', 900)
    print('\n'.join(l for l in out.split('\n')
                    if 'COMPILED' in l or 'FREE:' in l or ' U ' in l or 'ERROR' in l.upper()))
    os.makedirs(os.path.join(BUILD, 'original'), exist_ok=True)
    for prog, brf, lst, dest, name in (
            ('ADV-INTER-CB-MJ', 'AICF', 'AICF', BUILD, 'ADV-INTER-CB-MJ'),
            ('AIC', 'AIC', 'AIC', os.path.join(BUILD, 'original'), 'ADV-INTER-CB-MJ'),
            ('ADV-EDIT-CB-MJ', 'AEC', 'AEC', BUILD, 'ADV-EDIT-CB-MJ')):
        p = rc.pack_read('DNF/%s:PROG' % prog)
        open(os.path.join(dest, name + '.PROG'), 'wb').write(p)
        open(os.path.join(dest, name + '.BRF'), 'wb').write(rc.pack_read('DNF/%s:BRF' % brf))
        listing = rc.text(rc.pack_read('DNF/%s:LIST' % lst))
        open(os.path.join(dest, name + '.LIST.txt'), 'w', encoding='latin-1', newline='\n').write(listing)
        errors = [l for l in listing.split('\n') if l.startswith('*** ERROR')]
        if errors:
            sys.exit('%s:LIST:\n%s' % (lst, '\n'.join(errors)))
        print('%-45s %6d bytes, sha1 %s' % (os.path.relpath(os.path.join(dest, name + '.PROG'), PORT),
                                             len(p), hashlib.sha1(p).hexdigest()[:12]))
    for name in ('ADV-INTER-CB-MJ', 'ADV-EDIT-CB-MJ'):
        shutil.copyfile(os.path.join(BUILD, name + '.PROG'), os.path.join(PORT, 'data', name + '.PROG'))


if __name__ == '__main__':
    main()
