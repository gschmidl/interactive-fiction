"""Build DOD on the reference machine.

usage: python tools\\rebuild.py [CMDFILE ...]   (RetroCore must not be running)

1. writes to the pack (user DNF):
     DOD-ENB:SYMB         the program as recovered (..\\src_original\\)
     DODF:SYMB            the program with the port's fixes (basic\\)
     LIBRARY-MJ:BRF       the club library as it survives (NILSSON-3)
     BASLIBR-H00:BRF      ND BASIC's run-time library (as the Legend port rebuilt it)
2. starts RetroCore and runs tools\\build.cmd: the BASIC compiler of January
   1985 (TABLE-SIZES 1500,35000), each program in a BASIC session of its own,
   then NRL (SIZE 2700) with the program, the club library and BASLIBR-H00;
   command files named after it are run in the same session;
3. stops RetroCore and takes the programs, object files and listings off the
   pack: build\\ and data\\ (the fixed program) and build\\original\\ (the
   recovered source, which the compiler compiles with errors: lines 570-590
   twice, line 3120 missing);
4. writes DODF-REF, DODF-R01 ... DODF-R12 and the same of DOD to the pack:
   the programs with RANDOM not stirring in the uptime (MON 11), each with a
   seed of its own (tools\\refprog.py), so that the reference machine plays
   the same game for the same typing.
"""
import hashlib
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rc  # noqa: E402
from refprog import SEEDS, name, ref_of  # noqa: E402

PORT = rc.PORT
BUILD = os.path.join(PORT, 'build')
ORIG = os.path.join(PORT, '..', 'src_original')
NAME = 'DOD-ENB'


def rd(*p):
    return open(os.path.join(*p), 'rb').read()


def prepare():
    rc.pack_write({
        'DNF/DOD-ENB:SYMB': rd(ORIG, 'DOD-ENB.SYMB'),
        'DNF/DODF:SYMB': rd(PORT, 'basic', 'DOD-ENB.SYMB'),
        'DNF/LIBRARY-MJ:BRF': rd(BUILD, 'LIBRARY-MJ.BRF'),
        'DNF/BASLIBR-H00:BRF': rd(BUILD, 'BASLIBR-H00.BRF'),
    })


def build():
    out = rc.session(os.path.join(rc.HERE, 'build.cmd'), os.path.join(BUILD, 'build.log'), 1200)
    print('\n'.join(l for l in out.split('\n')
                    if 'COMPILED' in l or 'FREE:' in l or ' U ' in l or 'ERROR' in l.upper()))


def collect():
    os.makedirs(os.path.join(BUILD, 'original'), exist_ok=True)
    refs = {}
    for prog, lst, dest in ((NAME, 'DODF', BUILD), ('DOD', 'DOD', os.path.join(BUILD, 'original'))):
        p = rc.pack_read('DNF/%s:PROG' % prog)
        open(os.path.join(dest, NAME + '.PROG'), 'wb').write(p)
        open(os.path.join(dest, NAME + '.BRF'), 'wb').write(rc.pack_read('DNF/%s:BRF' % lst))
        listing = rc.text(rc.pack_read('DNF/%s:LIST' % lst))
        open(os.path.join(dest, NAME + '.LIST.txt'), 'w', encoding='latin-1', newline='\n').write(listing)
        errors = [l for l in listing.split('\n') if l.startswith('*** ERROR')]
        if errors and lst == 'DODF':
            sys.exit('%s:LIST:\n%s' % (lst, '\n'.join(errors)))
        for e in errors:
            print('  (as recovered) %s' % e)
        for seed in range(SEEDS + 1):
            refs['DNF/%s:PROG' % name(lst, seed)] = ref_of(p, seed)
        print('%-32s %6d bytes, sha1 %s' % (os.path.relpath(os.path.join(dest, NAME + '.PROG'), PORT),
                                             len(p), hashlib.sha1(p).hexdigest()[:12]))
    rc.pack_write(refs)
    shutil.copyfile(os.path.join(BUILD, NAME + '.PROG'), os.path.join(PORT, 'data', NAME + '.PROG'))


def main():
    prepare()
    with rc.Machine():
        build()
        for cmd in sys.argv[1:]:
            rc.session(cmd, os.path.splitext(cmd)[0] + '.log', 900)
    collect()


if __name__ == '__main__':
    main()
