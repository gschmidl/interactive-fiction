#!/usr/bin/env python3
"""Job streams that put the original on MVS 3.8j and run it there.

    python mkjobs.py                    j1src.jcl, j1data.jcl
    python mkjobs.py add PROBE probe.f  j5add.jcl   (add/replace a member)
    python mkjobs.py wiz  answers       j3wiz.jcl   (run ADVWIZ)
    python mkjobs.py adv  cards PGM     j4adv.jcl   (run PGM, e.g. PROBE)
    python mkjobs2.py [PROBE ...]       j2cl.jcl    (compile and link)

Submit them with sub.py, which also collects the printer output.  See
..\\..\\README.md for the whole sequence and for what it verifies.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PDS = os.path.join(HERE, '..', '..', '..', 'src_original',
                   'CBT.COV466.FILE119.PDS')

JOB = ("//%-7s JOB (1),'ADVENTURE',CLASS=A,MSGCLASS=A,\n"
       "//            USER=HERC01,PASSWORD=CUL8TR\n")

SUBS = """A5TOA1 AND AT BITSET BUG CARRY CODE1 CODE2 CVLTUC CVSTB DARK
DCODE1 DROP DSTROY FORCED GET12 GETIN HERE HOURS HOURSX IOINIT JUGGLE
LIQ LIQ2 LIQLOC MAINT MOTD MOVE MSPEAK NEWHRS NEWHRX OR PCT POOF PSPEAK PUT
RAN RSPEAK SCRMBL SHIFT SPEAK START TOTING VOCAB WIZARD XOR YES YESM
YESX""".split()
MAINS = ['ADVENT', 'ADVENT2', 'ADVWIZ']
# DATIME is left out of SUBS: the reference build uses DATIMEF, a fixed clock,
# which is what the port's --date/--time does.  It also makes the assembler
# module GETDTM unreachable, so it need not be assembled.

DATIMEF = """      SUBROUTINE DATIME(D,T)
C
C  TEST BUILD ONLY: A FIXED CLOCK, SO THAT THE RANDOM NUMBER GENERATOR
C  STARTS FROM A KNOWN SEED AND THE RUN CAN BE COMPARED WITH THE PORT
C  (WHICH IS RUN WITH --date 85123 --time 1000).  THE REAL MEMBER CALLS
C  THE ASSEMBLER ROUTINE GETDTM, WHICH THIS AVOIDS NEEDING AT ALL.
C
      IMPLICIT INTEGER(A-Z)
C
      D=85123
      T=600
      RETURN
      END"""


def member(name):
    raw = open(os.path.join(PDS, name + '.txt'), 'rb').read()
    lines = raw.decode('latin-1').split('\r\n')
    while lines and not lines[-1].strip():
        lines.pop()
    return [l.rstrip() for l in lines]


def deck(text):
    """Cards for TK5's socket reader.

    The reader translates ASCII to EBCDIC with Hercules' own table, which is
    not the cp037 one the PDS members were converted *out* of: it differs for
    [ ] ^ | and has no entry at all for the two characters the conversion
    left as A2 and FF.  So encode each card the way the member really was,
    with cp037, and then pick the ASCII byte that Hercules turns into that
    EBCDIC byte (xlate.json, measured by submitting all 256 of them).  The
    cards then land on MVS byte for byte as they were on the tape.
    """
    e2a = {int(k): v for k, v in
           json.load(open(os.path.join(HERE, 'xlate.json')))['e2a'].items()}
    out = bytearray()
    for line in text.split('\n'):
        for b in line.encode('cp037'):
            if b not in e2a:
                sys.exit('deck: no ASCII byte gives EBCDIC %02X' % b)
            out.append(e2a[b])
        out.append(0x0A)
    return bytes(out[:-1]) if text.endswith('\n') else bytes(out)


def w(path, text):
    # A JCL statement ends at column 71; anything in 72 means continuation,
    # and a truncated one is a JCL error that takes a while to recognise.
    # Data cards are not JCL and may fill all 80 columns.
    bad = [l for l in text.split('\n')
           if len(l) > 71 and l[:2] == '//' and l[:3] != '//*']
    if bad:
        sys.exit('mkjobs: JCL past column 71: %r' % bad[0])
    data = deck(text)
    open(os.path.join(HERE, path), 'wb').write(data)
    print('%-14s %6d cards' % (path, data.count(b'\n')))


def srcjob():
    out = [JOB % 'MOORSRC', """//*   delete anything left from a previous run
//A1      EXEC PGM=IEFBR14
//D1       DD DSN=HERC01.MOOR.SRC,DISP=(MOD,DELETE),UNIT=SYSDA,
//            SPACE=(TRK,(1)),DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
//*   the FORTRAN and the one assembler member, IEBUPDTE style
//A2      EXEC PGM=IEBUPDTE,PARM=NEW
//SYSPRINT DD  SYSOUT=A
//SYSUT2   DD  DSN=HERC01.MOOR.SRC,DISP=(NEW,CATLG),UNIT=SYSDA,
//             SPACE=(TRK,(90,10,30)),
//             DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
//SYSIN    DD  DATA,DLM='$$'
"""]
    for name in SUBS + MAINS + ['DATIME', 'GETDTM']:
        out.append('./ ADD NAME=%s\n' % name)
        out.append('\n'.join(member(name)) + '\n')
    out.append('./ ADD NAME=DATIMEF\n')
    out.append(DATIMEF + '\n')
    out.append('$$\n')
    return ''.join(out)


def datajob():
    out = [JOB % 'MOORDAT', """//A1      EXEC PGM=IEFBR14
//D1       DD DSN=HERC01.MOOR.DATA,DISP=(MOD,DELETE),UNIT=SYSDA,
//            SPACE=(TRK,(1)),DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
//A2      EXEC PGM=IEBGENER
//SYSPRINT DD  SYSOUT=A
//SYSIN    DD  DUMMY
//SYSUT2   DD  DSN=HERC01.MOOR.DATA,DISP=(NEW,CATLG),UNIT=SYSDA,
//             SPACE=(TRK,(45,15)),
//             DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
//SYSUT1   DD  DATA,DLM='$$'
"""]
    out.append('\n'.join(member('ADVTDATA')) + '\n')
    out.append('$$\n')
    return ''.join(out)


def addjob(name, path):
    """Add or replace one source member from a local file."""
    lines = [l.rstrip() for l in
             open(path, encoding='latin-1').read().split('\n')]
    while lines and not lines[-1]:
        lines.pop()
    return ''.join([JOB % 'MOORADD', """//A1      EXEC PGM=IEBUPDTE,PARM=MOD
//SYSPRINT DD  SYSOUT=A
//SYSUT1   DD  DSN=HERC01.MOOR.SRC,DISP=SHR
//SYSUT2   DD  DSN=HERC01.MOOR.SRC,DISP=SHR
//SYSIN    DD  DATA,DLM='$$'
./ REPL NAME=%s
""" % name, '\n'.join(lines) + '\n$$\n'])


def runwiz(answers):
    return ''.join([JOB % 'MOORWIZ', """//A1      EXEC PGM=IEFBR14
//D1       DD DSN=HERC01.MOOR.INI,DISP=(MOD,DELETE),UNIT=SYSDA,
//            SPACE=(TRK,(1))
//WIZ     EXEC PGM=ADVWIZ,REGION=1024K
//STEPLIB  DD  DSN=HERC01.MOOR.LOAD,DISP=SHR
//FT01F001 DD  DSN=HERC01.MOOR.DATA,DISP=SHR
//FT02F001 DD  DSN=HERC01.MOOR.INI,DISP=(NEW,CATLG),UNIT=SYSDA,
//             SPACE=(TRK,(30,10)),
//             DCB=(RECFM=VBS,BLKSIZE=13030,LRECL=13026)
//FT06F001 DD  SYSOUT=A
//FT05F001 DD  *
""", answers])


def runadv(cmds, prog='ADVENT', name='MOORADV', time=''):
    return ''.join([JOB % name, """//ADV     EXEC PGM=%s,REGION=1024K%s
//STEPLIB  DD  DSN=HERC01.MOOR.LOAD,DISP=SHR
//FT02F001 DD  DSN=HERC01.MOOR.INI,DISP=SHR
//FT06F001 DD  SYSOUT=A
//FT05F001 DD  *
""" % (prog, time), cmds])


def main():
    what = sys.argv[1] if len(sys.argv) > 1 else 'src'
    if what == 'src':
        w('j1src.jcl', srcjob())
        w('j1data.jcl', datajob())
    elif what == 'add':
        w('j5add.jcl', addjob(sys.argv[2], sys.argv[3]))
    elif what == 'wiz':
        w('j3wiz.jcl', runwiz(open(sys.argv[2], encoding='latin-1').read()))
    elif what == 'adv':
        w('j4adv.jcl',
          runadv(open(sys.argv[2], encoding='latin-1').read(), *sys.argv[3:]))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
