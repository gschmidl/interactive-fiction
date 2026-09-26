#!/usr/bin/env python3
"""Compile and link on MVS: the subroutines, ADVWIZ, and the probe.

ADVENT and ADVENT2 are not here: neither compiler on TK5 will take a program
that big (FORTRAN G says IEY031I ROLL SIZE EXCEEDED, FORTRAN H says IEK800I
SOURCE PROGRAM IS TOO LARGE, at every region size from 1 to 8 MB).  The
original was compiled with IGIFORT - VS FORTRAN G1 - which is not on this
system.  ADVWIZ compiles clean, and it reads the whole database and runs the
whole maintenance dialogue, so it is the reference for all of that; PROBE
covers the subroutines only ADVENT calls.
"""
import os
import sys

import mkjobs

SUBS = mkjobs.SUBS


def compile_link(extra_mains=()):
    out = [mkjobs.JOB % 'MOORCL']
    out.append("""//DEL     EXEC PGM=IEFBR14
//D1       DD DSN=HERC01.MOOR.LOAD,DISP=(MOD,DELETE),UNIT=SYSDA,
//            SPACE=(TRK,(1,1,1))
//*   FORTRAN IV G takes all the subroutines in one go
//SUBS    EXEC PGM=IEYFORT,REGION=1024K,PARM='MAP,NOLIST'
//SYSPRINT DD  SYSOUT=A
//SYSPUNCH DD  DUMMY
//SYSLIN   DD  DSN=&&OBJSUB,DISP=(NEW,PASS),UNIT=SYSDA,
//             SPACE=(TRK,(30,10)),DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
""")
    for i, name in enumerate(SUBS + ['DATIMEF']):
        lead = '//SYSIN    DD ' if i == 0 else '//         DD '
        out.append('%s DSN=HERC01.MOOR.SRC(%s),DISP=SHR\n' % (lead, name))
    mains = (('ADVWIZ', 'ADVWIZ'),) + tuple(extra_mains)
    for i, (member, name) in enumerate(mains):
        out.append("""//C%-6s EXEC PGM=IEYFORT,REGION=1024K,
//             PARM='MAP,NOLIST,NAME=%s'
//SYSPRINT DD  SYSOUT=A
//SYSPUNCH DD  DUMMY
//SYSLIN   DD  DSN=&&OBJM%d,DISP=(NEW,PASS),UNIT=SYSDA,
//             SPACE=(TRK,(5,5)),DCB=(RECFM=FB,LRECL=80,BLKSIZE=3120)
//SYSIN    DD  DSN=HERC01.MOOR.SRC(%s),DISP=SHR
""" % (name[:6], name, i, member))
    first = True
    for i, (member, name) in enumerate(mains):
        out.append("""//L%-6s EXEC PGM=IEWL,REGION=1024K,PARM='XREF,LIST,LET'
//SYSLIB   DD  DSN=SYS1.FORTLIB,DISP=SHR
//SYSLMOD  DD  DSN=HERC01.MOOR.LOAD,DISP=(%s),UNIT=SYSDA,
//             SPACE=(TRK,(30,10,10))
//SYSUT1   DD  UNIT=SYSDA,SPACE=(TRK,(20,10))
//SYSPRINT DD  SYSOUT=A
//OBJMAIN  DD  DSN=&&OBJM%d,DISP=(OLD,PASS)
//OBJSUB   DD  DSN=&&OBJSUB,DISP=(OLD,PASS)
//SYSLIN   DD  *
 INCLUDE OBJMAIN
 INCLUDE OBJSUB
 ENTRY %s
 NAME %s(R)
""" % (name[:6], 'NEW,CATLG' if first else 'OLD', i, name, name))
        first = False
    return ''.join(out)


if __name__ == '__main__':
    extra = [(m, m) for m in sys.argv[1:]]
    text = compile_link(extra)
    bad = [l for l in text.split('\n') if len(l) > 71]
    if bad:
        sys.exit('card past column 71: %r' % bad[0])
    mkjobs.w('j2cl.jcl', text)
