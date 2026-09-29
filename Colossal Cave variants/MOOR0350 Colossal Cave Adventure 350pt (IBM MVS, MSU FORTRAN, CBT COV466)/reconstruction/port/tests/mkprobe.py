#!/usr/bin/env python3
"""Generate PROBE, the test driver used to compare the port with MVS.

PROBE is not part of the game.  It loads the initialization file the way
ADVENT does - with ADVENT's own declarations and READ list, copied out of the
member so they cannot drift - and then calls the subroutines that only ADVENT
calls, printing everything.  ADVENT itself cannot be compiled on MVS 3.8j
(neither FORTRAN G nor H will take a program that big), so this is how the
rest of the program is checked against the real machine.

Two rules the FORTRAN IV G compiler imposes on the driver, neither of which
the game's own code ever runs into:
  * a function result may not appear in an input/output list - an implicitly
    typed name with a parenthesised list there is read as an array, and the
    compiler says UNDIMENSIONED - so every result goes into a temporary
    first, and the checksum is a subroutine rather than a function;
  * CARRY must not be handed PLACE(OBJ) itself, which it sets to -1 under
    its own feet, and must be given the location the object is really at:
    it walks the list of things there and nothing checks subscripts.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PDS = os.path.join(HERE, '..', '..', 'src_original',
                   'CBT.COV466.FILE119.PDS')


def advent_blocks():
    src = open(os.path.join(PDS, 'ADVENT.txt'), 'rb').read().decode('latin-1')
    lines = [l.rstrip() for l in src.split('\r\n')]
    a = next(i for i, l in enumerate(lines) if l.startswith('      IMPLICIT'))
    b = next(i for i, l in enumerate(lines)
             if l.startswith('      LOGICAL TOTING'))
    c = next(i for i, l in enumerate(lines)
             if l.startswith('      READ(DBINIT)'))
    d = c
    while not lines[d].startswith('C'):
        d += 1
    decl = lines[a:b]
    logi = lines[b:b + 2]
    read = lines[c:d]
    # The probe uses none of BLANK, SDOT or DOTBLK, and that one DATA
    # statement is the only place in the declarations where a character
    # constant goes into an integer, which the two compilers spell
    # differently.  Drop it, and insist that it is exactly one line.
    drop = [i for i, l in enumerate(decl) if "DATA BLANK/' '/" in l]
    if len(drop) != 1 or len(read) != 19 or len(decl) < 40:
        sys.exit('mkprobe: ADVENT does not look as expected')
    del decl[drop[0]]
    return decl, logi, read


BODY = """C
C  PROBE - NOT PART OF THE GAME.  IT CALLS THE ROUTINES THAT ONLY ADVENT
C  CALLS, SO THAT THEY CAN BE COMPARED WITH THE SAME ROUTINES RUNNING ON
C  MVS 3.8J, WHERE ADVENT ITSELF WILL NOT COMPILE.  SEE THE README.
C
      DIMENSION QCH(20),QS(25)
      LOGICAL QINS,QB1,QB2,QB3,QB4,QB5,QB6,QB7,QB8
      EXTERNAL RAN
C
      CALL IOINIT(0)
%(read)s
C
C  1. THE STATE AS THE INITIALIZATION FILE LEFT IT.  IF THESE MATCH, THE
C     TWO FILES - BUILT SEPARATELY ON EACH MACHINE - HOLD THE SAME GAME.
C
      WRITE(TTYO,1001)
1001  FORMAT(' PROBE 1: STATE CHECKSUMS')
      CALL QCK(LINES,9800,QS(1))
      CALL QCK(RTEXT,205,QS(2))
      CALL QCK(MTEXT,35,QS(3))
      CALL QCK(PTEXT,100,QS(4))
      CALL QCK(MTDTXT,100,QS(5))
      WRITE(TTYO,1002)(QS(QI),QI=1,5)
1002  FORMAT(' TEXT   LINES RTEXT MTEXT PTEXT MTDTXT   ',5I9)
      CALL QCK(KTAB,300,QS(1))
      CALL QCK(ATAB,300,QS(2))
      CALL QCK(TRAVEL,750,QS(3))
      CALL QCK(KEY,150,QS(4))
      WRITE(TTYO,1003)QS(1),QS(2),TABSIZ,QS(3),QS(4)
1003  FORMAT(' WORDS  KTAB ATAB TABSIZ TRAVEL KEY      ',5I9)
      CALL QCK(LTEXT,150,QS(1))
      CALL QCK(STEXT,150,QS(2))
      CALL QCK(COND,150,QS(3))
      CALL QCK(ABB,150,QS(4))
      CALL QCK(ATLOC,150,QS(5))
      WRITE(TTYO,1004)(QS(QI),QI=1,5)
1004  FORMAT(' PLACES LTEXT STEXT COND ABB ATLOC       ',5I9)
      CALL QCK(PLAC,100,QS(1))
      CALL QCK(PLACE,100,QS(2))
      CALL QCK(FIXD,100,QS(3))
      CALL QCK(FIXED,100,QS(4))
      CALL QCK(LINK,200,QS(5))
      WRITE(TTYO,1005)(QS(QI),QI=1,5)
1005  FORMAT(' THINGS PLAC PLACE FIXD FIXED LINK       ',5I9)
      CALL QCK(PROP,100,QS(1))
      CALL QCK(ACTSPK,35,QS(2))
      CALL QCK(CTEXT,12,QS(3))
      CALL QCK(CVAL,12,QS(4))
      CALL QCK(HINTS,80,QS(5))
      WRITE(TTYO,1006)(QS(QI),QI=1,5)
1006  FORMAT(' MORE   PROP ACTSPK CTEXT CVAL HINTS     ',5I9)
      WRITE(TTYO,1007)MAXTRS,TALLY,TALLY2,CLSSES,HNTMAX,MAXDIE,SETUP,
     1LAMP,HOLDNG,R
1007  FORMAT(' SCALAR MAXTRS TALLY TALLY2 CLSSES HNTMAX',
     1' MAXDIE SETUP LAMP HOLDNG R',/,8X,10I7)
      WRITE(TTYO,1008)WKDAY,WKEND,HOLID,HBEGIN,HEND,SHORT,MAGIC,MAGNM,
     1LATNCY,SAVED
1008  FORMAT(' WIZARD WKDAY WKEND HOLID HBEGIN HEND SHORT',
     1' MAGIC MAGNM LATNCY SAVED',/,8X,5I8,I6,I11,I7,I7,I4)
      WRITE(TTYO,1009)KEYS,GRATE,CAGE,ROD,ROD2,STEPS,BIRD,DOOR,PILLOW,
     1SNAKE,FISSUR,TABLET,CLAM,OYSTER,MAGZIN,DWARF,KNIFE,FOOD,BOTTLE,
     2WATER,OIL,PLANT,PLANT2,AXE,MIRROR,DRAGON,CHASM,TROLL,TROLL2,BEAR,
     3MESSAG,VEND,BATTER,NUGGET,COINS,CHEST,EGGS,TRIDNT,VASE,EMRALD,
     4PYRAM,PEARL,RUG,CHAIN,SPICES,BACK,LOOK,CAVE,NULL,ENTRNC,DPRSSN,
     5SAY,LOCK,THROW,FIND,INVENT,SUSPND
1009  FORMAT(' OBJECTS AND VERBS',/,(8X,15I4))
C
C  2. THE SIXBIT PACKING, AND THE ONE ROUTINE ONLY ADVENT USES (A5TOA1).
C
      WRITE(TTYO,1010)
1010  FORMAT(/,' PROBE 2: CODE1 / DCODE1 / A5TOA1')
      QS(1)=CODE1('DWARF')
      QS(2)=CODE1('ABC  ')
      QS(3)=CODE1('>$<  ')
      QS(4)=CODE1('     ')
      QS(5)=CODE1('99999')
      QS(6)=CODE1('/7-08')
      WRITE(TTYO,1011)(QS(QI),QI=1,6)
1011  FORMAT(' CODE1 ',6I12)
      CALL DCODE1(QS(1),TEXT(1))
      CALL DCODE1(QS(6),TEXT(6))
      WRITE(TTYO,1012)(TEXT(QI),QI=1,10)
1012  FORMAT(' DCODE1 >',10A1,'<')
      QS(11)=CODE1('MAGIC')
      QS(12)=CODE1('WORD ')
      QS(13)=CODE1('HERE ')
      QS(14)=CODE1('X    ')
      QINS=.TRUE.
      CALL QBLANK(QCH)
      CALL A5TOA1(QS(11),QS(12),QS(13),QINS,QCH,QLEN)
      WRITE(TTYO,1013)QLEN,(QCH(QI),QI=1,20)
1013  FORMAT(' A5TOA1 T',I3,' >',20A1,'<')
      QINS=.FALSE.
      CALL QBLANK(QCH)
      CALL A5TOA1(QS(11),QS(12),QS(13),QINS,QCH,QLEN)
      WRITE(TTYO,1014)QLEN,(QCH(QI),QI=1,20)
1014  FORMAT(' A5TOA1 F',I3,' >',20A1,'<')
      QINS=.TRUE.
      CALL QBLANK(QCH)
      CALL A5TOA1(QS(14),QS(4),QS(4),QINS,QCH,QLEN)
      WRITE(TTYO,1015)QLEN,(QCH(QI),QI=1,20)
1015  FORMAT(' A5TOA1 X',I3,' >',20A1,'<')
C
C  3. BIT FIDDLING, INCLUDING NEGATIVE VALUES.
C
      WRITE(TTYO,1020)
1020  FORMAT(/,' PROBE 3: SHIFT / AND / OR / XOR')
      DO 1021 QI=1,6
      QV=1
      IF(QI.EQ.2)QV=-1
      IF(QI.EQ.3)QV=1023
      IF(QI.EQ.4)QV=-1023
      IF(QI.EQ.5)QV=2147483647
      IF(QI.EQ.6)QV=-2147483647
      QS(1)=SHIFT(QV,1)
      QS(2)=SHIFT(QV,7)
      QS(3)=SHIFT(QV,31)
      QS(4)=SHIFT(QV,-1)
      QS(5)=SHIFT(QV,-7)
      QS(6)=SHIFT(QV,-31)
      WRITE(TTYO,1022)QV,(QS(QJ),QJ=1,6)
1022  FORMAT(' SHIFT',I12,' ->',6I12)
1021  CONTINUE
      QS(1)=AND(-1,255)
      QS(2)=OR(-256,15)
      QS(3)=XOR(-1,255)
      QS(4)=AND(12345,54321)
      QS(5)=OR(12345,54321)
      QS(6)=XOR(12345,54321)
      WRITE(TTYO,1023)(QS(QI),QI=1,6)
1023  FORMAT(' BITS ',6I12)
C
C  4. THE RANDOM NUMBER GENERATOR, FROM SEEDS SET HERE SO THAT NO CLOCK
C     IS INVOLVED.
C
      WRITE(TTYO,1030)
1030  FORMAT(/,' PROBE 4: RAN / PCT')
      R=10805
      DO 1031 QI=1,10
1031  QS(QI)=RAN(1000)
      WRITE(TTYO,1032)(QS(QI),QI=1,10)
1032  FORMAT(' RAN(1000)',10I5)
      R=10805
      DO 1033 QI=1,10
1033  QS(QI)=RAN(8)
      WRITE(TTYO,1034)(QS(QI),QI=1,10)
1034  FORMAT(' RAN(8)   ',10I5)
      R=1
      DO 1035 QI=1,10
1035  QS(QI)=RAN(100)
      WRITE(TTYO,1036)(QS(QI),QI=1,10)
1036  FORMAT(' RAN(100) ',10I5)
      R=7
      QN=0
      DO 1037 QI=1,100
1037  IF(PCT(25))QN=QN+1
      WRITE(TTYO,1038)QN,R
1038  FORMAT(' PCT(25) OF 100 =',I4,'  R NOW',I9)
C
C  5. THE PREDICATES AND THE OBJECT LISTS, ON THE STATE AS LOADED.
C
      WRITE(TTYO,1040)
1040  FORMAT(/,' PROBE 5: PREDICATES')
      LOC=1
      QB1=DARK(0)
      QB2=FORCED(1)
      QB3=FORCED(8)
      QB4=BITSET(1,0)
      QB5=BITSET(21,3)
      QB6=TOTING(LAMP)
      QB7=HERE(LAMP)
      QB8=AT(GRATE)
      WRITE(TTYO,1041)QB1,QB2,QB3,QB4,QB5,QB6,QB7,QB8
1041  FORMAT(' LOC 1  DARK',L2,' FORCED1',L2,' FORCED8',L2,
     1' BIT1.0',L2,' BIT21.3',L2,' TOT',L2,' HERE',L2,' AT',L2)
      LOC=3
      QB1=DARK(0)
      QS(1)=LIQ(0)
      QS(2)=LIQLOC(1)
      QS(3)=LIQLOC(3)
      QS(4)=LIQLOC(14)
      QS(5)=LIQ2(0)
      QS(6)=LIQ2(1)
      QS(7)=LIQ2(2)
      WRITE(TTYO,1042)QB1,(QS(QI),QI=1,7)
1042  FORMAT(' LOC 3  DARK',L2,' LIQ',I4,' LIQLOC 1/3/14',3I4,
     1' LIQ2 0/1/2',3I4)
      WRITE(TTYO,1043)(QI,ATLOC(QI),QI=1,12)
1043  FORMAT(' ATLOC ',12(I4,':',I3))
C
C  6. MOVING THINGS ABOUT.
C
      WRITE(TTYO,1050)
1050  FORMAT(/,' PROBE 6: CARRY / DROP / MOVE / JUGGLE / DSTROY / PUT')
      QL=PLACE(LAMP)
      CALL CARRY(LAMP,QL)
      QL=PLACE(KEYS)
      CALL CARRY(KEYS,QL)
      CALL DROP(LAMP,1)
      CALL MOVE(FOOD,7)
      CALL JUGGLE(BOTTLE)
      CALL DSTROY(MAGZIN)
      QP=PUT(GRATE,8,1)
      WRITE(TTYO,1051)HOLDNG,PLACE(LAMP),PLACE(KEYS),PLACE(FOOD),
     1PLACE(BOTTLE),PLACE(MAGZIN),PLACE(GRATE),QP
1051  FORMAT(' HOLDNG',I3,' PLACE LAMP/KEYS/FOOD/BOTTLE/MAGZIN/GRATE',
     16I5,'  PUT',I4)
      WRITE(TTYO,1052)ATLOC(1),ATLOC(3),ATLOC(7),ATLOC(8),
     1LINK(LAMP),LINK(KEYS),LINK(FOOD),LINK(BOTTLE)
1052  FORMAT(' ATLOC 1/3/7/8',4I5,'  LINK LAMP/KEYS/FOOD/BOTTLE',4I5)
C
C  7. THE VOCABULARY.
C
      WRITE(TTYO,1060)
1060  FORMAT(/,' PROBE 7: VOCAB')
      QS(11)=CODE1('ENTER')
      QS(1)=VOCAB(QS(11),-1)
      QS(11)=CODE1('LAMP ')
      QS(2)=VOCAB(QS(11),-1)
      QS(11)=CODE1('TAKE ')
      QS(3)=VOCAB(QS(11),-1)
      QS(11)=CODE1('DIG  ')
      QS(4)=VOCAB(QS(11),-1)
      QS(11)=CODE1('XYZZY')
      QS(5)=VOCAB(QS(11),-1)
      QS(11)=CODE1('FOOBA')
      QS(6)=VOCAB(QS(11),-1)
      QS(11)=CODE1('QQQQQ')
      QS(7)=VOCAB(QS(11),-1)
      QS(11)=CODE1('WEST ')
      QS(8)=VOCAB(QS(11),-1)
      WRITE(TTYO,1061)(QS(QI),QI=1,8)
1061  FORMAT(' ENTER LAMP TAKE DIG XYZZY FOOBAR QQQQQ WEST',8I7)
      QS(11)=CODE1('STEPS')
      QS(1)=VOCAB(QS(11),1)
      QS(2)=VOCAB(QS(11),0)
      QS(11)=CODE1('WATER')
      QS(3)=VOCAB(QS(11),1)
      QS(11)=CODE1('OIL  ')
      QS(4)=VOCAB(QS(11),1)
      WRITE(TTYO,1062)(QS(QI),QI=1,4)
1062  FORMAT(' STEPS AS OBJECT/MOTION, WATER, OIL',4I7)
C
C  8. NUMBERS OUT OF SIXBIT TEXT.  (THE CASE FOLDER, CVLTUC, IS TESTED
C     THROUGH GETIN IN PROBE 10, WHERE THE CARDS ARE IN LOWER CASE - IT
C     CANNOT BE TESTED FROM A DATA STATEMENT BECAUSE THE LETTER CODES
C     ARE NOT THE SAME ON THE TWO MACHINES.)
C
      WRITE(TTYO,1070)
1070  FORMAT(/,' PROBE 8: CVSTB')
      QS(11)=CODE1('     ')
      QS(12)=CODE1('123  ')
      QS(1)=CVSTB(QS(12),QS(11))
      QS(12)=CODE1('-45  ')
      QS(2)=CVSTB(QS(12),QS(11))
      QS(12)=CODE1('+7   ')
      QS(3)=CVSTB(QS(12),QS(11))
      QS(12)=CODE1('12A45')
      QS(4)=CVSTB(QS(12),QS(11))
      QS(5)=CVSTB(QS(11),QS(11))
      QS(12)=CODE1('99999')
      QS(6)=CVSTB(QS(12),QS(12))
      WRITE(TTYO,1071)(QS(QI),QI=1,6)
1071  FORMAT(' CVSTB',6I12)
C
C  9. THE MESSAGES, INCLUDING MULTI-LINE AND MULTI-PROPERTY ONES.
C
      WRITE(TTYO,1080)
1080  FORMAT(/,' PROBE 9: RSPEAK / MSPEAK / PSPEAK / SPEAK')
      CALL RSPEAK(1)
      CALL RSPEAK(2)
      CALL RSPEAK(60)
      CALL RSPEAK(61)
      CALL RSPEAK(201)
      CALL MSPEAK(1)
      CALL MSPEAK(31)
      CALL PSPEAK(LAMP,0)
      CALL PSPEAK(LAMP,1)
      CALL PSPEAK(GRATE,0)
      CALL PSPEAK(GRATE,1)
      CALL SPEAK(LTEXT(1))
      CALL SPEAK(STEXT(1))
      CALL SPEAK(LTEXT(141))
      CALL SPEAK(LTEXT(142))
      CALL SPEAK(CTEXT(1))
      CALL SPEAK(CTEXT(10))
C
C  10. THE PARSER, ON CARDS FROM THE TERMINAL FILE.
C
      WRITE(TTYO,1090)
1090  FORMAT(/,' PROBE 10: GETIN')
      DO 1091 QI=1,6
      CALL GETIN(QW1,QW1X,QW2,QW2X,.TRUE.)
      QS(1)=VOCAB(QW1,-1)
      WRITE(TTYO,1092)QW1,QW1X,QW2,QW2X,QS(1)
1092  FORMAT(' GETIN',4I12,'  VOCAB',I7)
1091  CONTINUE
C
      WRITE(TTYO,1099)
1099  FORMAT(/,' PROBE DONE')
      STOP
      END
      SUBROUTINE QBLANK(QCH)
C
C  TWENTY BLANKS, WITHOUT SAYING WHAT A BLANK IS: DCODE1 OF ZERO GIVES
C  FIVE OF THEM IN WHATEVER THIS MACHINE'S CHARACTER CODE IS.
C
      IMPLICIT INTEGER(A-Z)
      DIMENSION QCH(20)
C
      DO 10 I=1,4
10    CALL DCODE1(0,QCH(5*I-4))
      RETURN
      END
      SUBROUTINE QCK(ARR,N,QOUT)
C
C  A CHECKSUM THAT CANNOT OVERFLOW ON EITHER MACHINE.
C
      IMPLICIT INTEGER(A-Z)
      DIMENSION ARR(N)
C
      QOUT=17
      DO 10 QI=1,N
10    QOUT=MOD(QOUT*31+ARR(QI),1000003)
      RETURN
      END
"""


def main():
    decl, logi, read = advent_blocks()
    text = ('\n'.join(decl) + '\n' + '\n'.join(logi) + '\n' +
            BODY % {'read': '\n'.join(read)})
    bad = [l for l in text.split('\n') if len(l) > 72]
    if bad:
        sys.exit('mkprobe: past column 72: %r' % bad[0])
    open(os.path.join(HERE, 'probe.f'), 'w', encoding='latin-1',
         newline='\n').write(text)
    print('probe.f: %d lines' % text.count('\n'))


if __name__ == '__main__':
    main()
