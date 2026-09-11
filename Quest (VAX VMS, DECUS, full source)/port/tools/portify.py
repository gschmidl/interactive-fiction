"""Turn the detabbed DECUS sources into something gfortran will build.

Run after detab.py, over a fresh copy -- see build_sources.sh.  Every edit
is asserted to apply exactly the number of times it is expected to, so a
change in the input cannot slip through silently.  The categories are:

  * VMS run-time calls          -> the equivalents in vmsrt.c
  * VAX FORTRAN extensions      -> standard FORTRAN
        unit'record             -> REC=
        A<n> / I<n>             -> fixed widths, or a call to the C writer
        '+' / '$' carriage ctl  -> TTYRAW (print here, move nothing)
        indexed I/O (KEY=...)   -> the layer in keyed.c
  * five PROGRAMs               -> five SUBROUTINEs the dispatcher calls
  * names that clash with a gfortran intrinsic, a Fortran keyword, or a
    routine of the same name in another one of the five images
  * the VMS file specifications -> QPATH, which prefixes the data directory

Note for anyone rebuilding this: gfortran must be given -fno-pad-source.
QUEST continues its long message strings from one line to the next, and
VMS files are variable length records, so the literal picks up at the
start of the continuation.  gfortran's default is to pad every fixed-form
line out to the full line length first, which buries each continuation
under sixty spaces.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(os.path.dirname(HERE), 'src')

edits = 0


def sub(text, old, new, count=1, where=''):
    global edits
    n = text.count(old)
    if n != count:
        sys.exit('%s: expected %d occurrence(s) of\n---\n%s\n---\ngot %d'
                 % (where, count, old, n))
    edits += n
    return text.replace(old, new)


def resub(text, pat, new, count, where=''):
    global edits
    out, n = re.subn(pat, new, text)
    if n != count:
        sys.exit('%s: pattern %r matched %d times, expected %d'
                 % (where, pat, n, count))
    edits += n
    return out


def drop_lines(text, pat, count, where=''):
    global edits
    keep, n = [], 0
    for line in text.split('\n'):
        if re.search(pat, line):
            n += 1
            continue
        keep.append(line)
    if n != count:
        sys.exit('%s: %r matched %d lines, expected %d' % (where, pat, n, count))
    edits += n
    return '\n'.join(keep)


def fix_rec(text, count, name):
    """READ(24'K+1,2) VAR  ->  READ(24,2,REC=K+1) VAR.

    The record expression can itself contain commas inside parentheses
    (READ(21'DICE(1,50),2) MORAL), so the separating comma is found by
    balancing brackets rather than by a regular expression."""
    global edits
    out, pos, n = [], 0, 0
    for m in re.finditer(r"\b(READ|WRITE)\((\d+)'", text):
        i, depth, comma = m.end(), 0, -1
        while i < len(text):
            ch = text[i]
            if ch == '(':
                depth += 1
            elif ch == ')':
                if depth == 0:
                    break
                depth -= 1
            elif ch == ',' and depth == 0 and comma < 0:
                comma = i
            i += 1
        if comma < 0:
            sys.exit('%s: cannot split %r' % (name, text[m.start():i + 1]))
        rec = text[m.end():comma].strip()
        fmt = text[comma + 1:i].strip()
        out.append(text[pos:m.start()])
        out.append('%s(%s,%s,REC=%s)' % (m.group(1), m.group(2), fmt, rec))
        pos = i + 1
        n += 1
    out.append(text[pos:])
    if n != count:
        sys.exit('%s: %d unit-quote-record statements, expected %d' % (name, n, count))
    edits += n
    return ''.join(out)


def common(text, name, c):
    text = sub(text, "INCLUDE 'QSTCOM.FOR'", "INCLUDE 'qstcom.inc'",
               c['qstcom'], name)
    for pat, k in ((r"INCLUDE '\(\$FORIOSDEF\)'", 'foriosdef'),
                   (r"INCLUDE '\(\$SSDEF\)'", 'ssdef'),
                   # LIB$ESTABLISH armed a handler whose only job was to
                   # mail the author an error code; there is nobody to mail.
                   (r'CALL LIB\$ESTABLISH', 'establish')):
        if c.get(k):
            text = drop_lines(text, pat, c[k], name)

    for old, new, k in (
        ('LIB$LEN(', 'LEN(', 'lib_len'),
        ('CALL LIB$SET_SCROLL(', 'CALL SETSCROLL(', 'set_scroll'),
        ('CALL LIB$ERASE_PAGE(1,1)', 'CALL CLRSCR', 'erase_page'),
        ('CALL LIB$DAY(', 'CALL LIBDAY(', 'lib_day'),
        ('CALL LIB$PUT_COMMON(', 'CALL PUTCOMMON(', 'put_common'),
        ('CALL LIB$GET_COMMON(', 'CALL GETCOMMON(', 'get_common'),
        ('CALL LIB$RUN_PROGRAM(', 'CALL RUNPROG(', 'run_program'),
        ('CALL SYS$DELPRC(,)', 'CALL DELPRC', 'delprc'),
        ('CALL BAS$SLEEP(%VAL(I))', 'CALL BASSLP(I)', 'bas_sleep'),
        ('CALL FOR$EXIT', 'CALL FOREXIT', 'for_exit'),
        ('SECNDS(0.0)', 'VSECND(0.0)', 'secnds'),
        ('RAN(SEED1)', 'VAXRAN(SEED1)', 'ran'),
        ('CALL TIME(TIM)', 'CALL VMSTIM(TIM)', 'time'),
        # VAX FORTRAN .OR. on two INTEGERs is a bitwise OR
        ('SEED1=SEED1.OR.1', 'SEED1=IOR(SEED1,1)', 'seedor'),
    ):
        if c.get(k):
            text = sub(text, old, new, c[k], name)

    for pat, new, k in (
        (r'\bCALL SLEEP\(', 'CALL QSLEEP(', 'call_sleep'),
        (r'\bSUBROUTINE SLEEP\b', 'SUBROUTINE QSLEEP', 'def_sleep'),
        (r'\bCALL RENAME\b', 'CALL QRENAME', 'call_rename'),
        (r'\bSUBROUTINE RENAME\b', 'SUBROUTINE QRENAME', 'def_rename'),
        (r'\bCALL KILL\b(?!PLAYER)', 'CALL QKILL', 'call_kill'),
        (r'\bSUBROUTINE KILL\b(?!PLAYER)', 'SUBROUTINE QKILL', 'def_kill'),
        (r'\bCALL PRINT\b(?!_)', 'CALL QPRINT', 'call_print'),
        (r'\bSUBROUTINE PRINT\b(?!_)', 'SUBROUTINE QPRINT', 'def_print'),
    ):
        if c.get(k):
            text = resub(text, pat, new, c[k], name)

    if c.get('rec'):                       # READ(24'K,2) -> READ(24,2,REC=K)
        text = fix_rec(text, c['rec'], name)
    return text


def load(name):
    return open(os.path.join(SRC, name), encoding='latin1').read()


def save(name, t):
    open(os.path.join(SRC, name), 'w', encoding='latin1', newline='\n').write(t)
    print('%-10s ok' % name)


# ====================================================================
def do_lib():
    name = 'lib.f'
    t = load(name)
    t = common(t, name, dict(
        qstcom=28, foriosdef=5, ssdef=1, lib_len=4, erase_page=1, lib_day=1,
        put_common=2, get_common=1, run_program=1, delprc=1, bas_sleep=1,
        for_exit=1, ran=1, secnds=1, seedor=1, call_sleep=12,
        def_sleep=1, rec=3))

    # ---- terminal input ------------------------------------------------
    t = sub(t, """      SUBROUTINE INPUTNUMBER(I)
      INTEGER I

1     READ(5,2,ERR=5) I
2     FORMAT(I)
      RETURN

5     CALL SINGLE(7)""", """      SUBROUTINE INPUTNUMBER(I)
      INTEGER I,IERR

1     CALL TTYGETNUM(I,IERR)
      IF(IERR.EQ.0)RETURN

5     CALL SINGLE(7)""", 1, name)

    t = sub(t, """      I=LEN(STRING)
1     READ(5,2,ERR=5) STRING
2     FORMAT(A<I>)
      IF(FLAG.NE.0)CALL UPPERCASE(STRING)""", """      I=LEN(STRING)
1     CALL TTYGET(STRING)
      IF(FLAG.NE.0)CALL UPPERCASE(STRING)""", 1, name)

    # ---- ('+',...,$): print here, move the cursor nowhere --------------
    t = sub(t, """      J=LENGTH(STRING)
      IF(J.NE.0)THEN
        WRITE(6,1) STRING
1       FORMAT('+',A<J>,$)
        ENDIF""", """      J=LENGTH(STRING)
      IF(J.NE.0)CALL TTYRAW(STRING(1:J))""", 1, name)

    t = sub(t, """      I=8
      WRITE(6,1) I
1     FORMAT('+_',A1$)""", """      I=8
      CALL TTYRAW('_'//CHAR(I))""", 1, name)

    # A1 editing on an INTEGER*4 emits its low order byte.
    t = sub(t, """      WRITE(6,1) I
1     FORMAT('+',A1$)
      RETURN
      END""", """      CALL TTYRAW(CHAR(MOD(I,256)))
      RETURN
      END""", 1, name)

    t = sub(t, """      DATA BLANKS/'          '/
      J=IPOSITION(NUMBER)
      I=DIGITS-J
      WRITE(6,1) BLANKS(1:I),NUMBER
1     FORMAT('+',A<I>,I<J>,$)""", """      CHARACTER B*32
      DATA BLANKS/'          '/
      J=IPOSITION(NUMBER)
      I=DIGITS-J
      WRITE(B,'(I32)') NUMBER
      IF(I.GT.0)CALL TTYRAW(BLANKS(1:I))
      CALL TTYRAW(B(33-J:32))""", 1, name)

    t = sub(t, """      SUBROUTINE OUTNUM(I)
      INTEGER I

      J=IPOSITION(I)
      WRITE(6,1) I
1     FORMAT('+',I<J>$)""", """      SUBROUTINE OUTNUM(I)
      INTEGER I
      CHARACTER B*32

      J=IPOSITION(I)
      WRITE(B,'(I32)') I
      CALL TTYRAW(B(33-J:32))""", 1, name)

    # ---- carriage-controlled empty records ----------------------------
    t = sub(t, "      WRITE(6,FMT='(//)')\n      CALL TRIMMER(NAME)",
            "      CALL TTYNL(3)\n      CALL TRIMMER(NAME)", 1, name)
    t = sub(t, "          CALL TRIMMER(RECORD)\n          WRITE(6,FMT='()')",
            "          CALL TRIMMER(RECORD)\n          CALL TTYNL(1)", 1, name)
    t = sub(t, "      LIFE=0\n      WRITE(6,FMT='(///////)')",
            "      LIFE=0\n      CALL TTYNL(8)", 1, name)

    # ---- CHARACTER.DTA: RMS indexed -> keyed.c -------------------------
    t = sub(t, "1     WRITE(21,IOSTAT=IOS) PLAYER",
            "1     CALL KWRITE(PLAYER,IOS)", 1, name)

    t = sub(t, """      SUBROUTINE OPENCHARFILE

      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]CHARACTER.DTA',
     +ACCESS='KEYED',STATUS='UNKNOWN',ORGANIZATION='INDEXED',RECL=63,
     +FORM='UNFORMATTED',KEY=(1:15:CHARACTER,26:37:CHARACTER),SHARED)

      RETURN
      END""", """      SUBROUTINE OPENCHARFILE
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      CALL KOPEN
      KEYOPN=.TRUE.
      RETURN
      END""", 1, name)

    t = sub(t, "1     READ(21,IOSTAT=IOS,KEY=KEY1,KEYID=0) PLAYER",
            "1     CALL KREAD_KEY(PLAYER,KEY1,0,0,IOS)", 1, name)
    t = sub(t, "1     READ(21,IOSTAT=IOS,KEY=KEY1,KEYID=0)\n",
            "1     CALL KFIND_KEY(KEY1,0,0,IOS)\n", 2, name)
    t = sub(t, "3     REWRITE(21,IOSTAT=IOS) PLAYER",
            "3     CALL KREWRITE(PLAYER,IOS)", 1, name)
    t = sub(t, "3     DELETE(21,IOSTAT=IOS)", "3     CALL KDELETE(IOS)", 1, name)
    t = sub(t, "1     READ(21,IOSTAT=IOS,KEY=USER,KEYID=1) PLAYER(1:37)",
            "1     CALL KREAD_KEY(PLAYER(1:37),USER,1,0,IOS)", 1, name)
    t = sub(t, """2     READ(21,IOSTAT=IOS,END=50) PLAYER(1:37)
      IF(USER.NE.PLAYER(26:37))GOTO 50""", """2     CALL KREAD_NEXT(PLAYER(1:37),IOS)
      IF(IOS.LT.0)GOTO 50
      IF(USER.NE.PLAYER(26:37))GOTO 50""", 1, name)
    t = sub(t, """      ERR=0
      READ(21,ERR=1,KEY=KEY1,KEYID=0)
      RETURN""", """      ERR=0
      CALL KFIND_KEY(KEY1,0,0,IOS)
      IF(IOS.NE.0)GOTO 1
      RETURN""", 1, name)

    t = sub(t, """      SUBROUTINE CLOSEFILE(I)
      INTEGER I

      CLOSE(UNIT=I)
      RETURN
      END""", """      SUBROUTINE CLOSEFILE(I)
      INTEGER I,J
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      IF(I.EQ.21.AND.KEYOPN)THEN
        CALL KCLOSE
        KEYOPN=.FALSE.
      ELSE
        CLOSE(UNIT=I,IOSTAT=J)
        ENDIF
      RETURN
      END



*****
*
*  CLOSEALL shuts every unit an image had open.  On VMS a chained-to image
*  simply started with none of them open; here the dispatcher calls this
*  on the way out of CHAIN.
*
*****

      SUBROUTINE CLOSEALL
      INTEGER I,J
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      KEYOPN=.FALSE.
      DO I=21,27
        CLOSE(UNIT=I,IOSTAT=J)
      ENDDO
      CLOSE(UNIT=99,IOSTAT=J)
      RETURN
      END""", 1, name)

    # ---- the VMS file specifications ----------------------------------
    t = sub(t, """      OPEN(UNIT=23,FILE='BSU$USER_2:[00CKKELLE.QUEST]MAGIC.DTA',
     +STATUS='OLD',FORM='FORMATTED',ACCESS='DIRECT',ORGANIZATION=
     +'RELATIVE',RECL=54,READONLY)""", """      CHARACTER QFN*256
      CALL QPATH('magic.dta',QFN)
      OPEN(UNIT=23,FILE=QFN,STATUS='OLD',FORM='FORMATTED',
     +ACCESS='DIRECT',RECL=54,ACTION='READ')""", 1, name)

    t = sub(t, """      SUBROUTINE DEATH
      INCLUDE 'qstcom.inc'
      CHARACTER MORAL*80""", """      SUBROUTINE DEATH
      INCLUDE 'qstcom.inc'
      CHARACTER MORAL*80,QFN*256""", 1, name)
    t = sub(t, """      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]MORAL.DTA',
     +STATUS='OLD',ACCESS='DIRECT',ORGANIZATION='RELATIVE',RECL=80,
     +FORM='FORMATTED',CARRIAGECONTROL='LIST',READONLY)""", """      CALL QPATH('moral.dta',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',ACCESS='DIRECT',RECL=80,
     +FORM='FORMATTED',ACTION='READ')""", 1, name)

    t = sub(t, """      SUBROUTINE GETTEMPCORE(RECORD)
      INCLUDE 'qstcom.inc'
      CHARACTER RECORD*(*),CLEAR*1""", """      SUBROUTINE GETTEMPCORE(RECORD)
      INCLUDE 'qstcom.inc'
      CHARACTER RECORD*(*),CLEAR*1,QFN*256""", 1, name)
    t = sub(t, """        OPEN(UNIT=27,FILE='BSU$USER_2:[00CKKELLE.QUEST]FATAL.DAT',
     +STATUS='OLD',ACCESS='APPEND')""", """        CALL QPATH('fatal.dat',QFN)
        OPEN(UNIT=27,FILE=QFN,STATUS='UNKNOWN',POSITION='APPEND')""", 1, name)

    # QUEST_ERROR: the VAX condition handler.  Nothing establishes it any
    # more, but the routine is kept, and its report now goes to the
    # terminal the same way every other message does.
    t = sub(t, """      INTEGER*4 MECHARGS(*),SIGARGS(*)""",
            """      INTEGER*4 MECHARGS(*),SIGARGS(*)
      CHARACTER QFN*256""", 1, name)
    t = sub(t, """      OPEN(UNIT=99,FILE='SYS$OUTPUT',STATUS='OLD')
      J=IPOSITION(SIGARGS(2))""", """      J=IPOSITION(SIGARGS(2))""", 1, name)
    t = sub(t, """        WRITE(99,1) SIGARGS(2)
1       FORMAT(////' Jim, the wizard, appears before you.',//,
     +' Alas!! The very fabric of my world has changed and I cannot
     + seem to',/,' find out why. Would you MAIL this number: ',
     +I<J>,' to 00CKKELLEY?',///,' 			He
     + vanishes........')
        OPEN(UNIT=27,FILE='BSU$USER_2:[00CKKELLE.QUEST]ERROR.DAT',
     +ACCESS='APPEND',STATUS='OLD')""", """        CALL FORMAT(5,' Jim, the wizard, appears before you.!/!/')
        CALL FORMAT(0,' Alas!! The very fabric of my world has changed')
        CALL FORMAT(0,' and I cannot!/ seem to find out why.')
        CALL FORMAT(0,' Would you MAIL this number: ')
        CALL OUTNUM(SIGARGS(2))
        CALL FORMAT(0,' to 00CKKELLEY?!/!/!/ !_!_!_He vanishes........')
        CALL QPATH('error.dat',QFN)
        OPEN(UNIT=27,FILE=QFN,STATUS='UNKNOWN',POSITION='APPEND')""", 1, name)
    t = sub(t, """      SUBROUTINE LOCATEMAGIC(J)
      INCLUDE 'qstcom.inc'
      CHARACTER ITEMNAME*10,ITEMDESCRIPT*36
      INTEGER K1,K2,K3,K4""", """      SUBROUTINE LOCATEMAGIC(J)
      INCLUDE 'qstcom.inc'
      CHARACTER ITEMNAME*10,ITEMDESCRIPT*36
      INTEGER K1,K2,K3,K4
      LOGICAL SAVINGTHROW""", 1, name)

    t = sub(t, "12      FORMAT(1X,A12,2X,A6,2X,I)",
            "12      FORMAT(1X,A12,2X,A6,2X,I11)", 1, name)
    save(name, t)


# ====================================================================
def do_quest():
    name = 'quest.f'
    t = load(name)
    t = common(t, name, dict(qstcom=1, establish=1, lib_day=1, time=1))
    t = sub(t, '      PROGRAM QUEST\n', '      SUBROUTINE QUEST\n', 1, name)
    t = sub(t, "      CHARACTER SETUP*12,START_TIME*5,END_TIME*5",
            "      CHARACTER SETUP*12,START_TIME*5,END_TIME*5,QFN*256", 1, name)
    t = sub(t, """      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]ACCESS.FIL',
     +STATUS='OLD',FORM='FORMATTED',CARRIAGECONTROL='LIST',READONLY)""",
            """      CALL QPATH('access.fil',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',FORM='FORMATTED',ACTION='READ')""",
            1, name)
    t = sub(t, "      WRITE(5,FMT='(1X,A12)') SETUP",
            "      CALL TTYREC(ICHAR(' '),SETUP)", 1, name)
    t = sub(t, "      CALL BAS$SLEEP(%VAL(4))", "      CALL BASSLP(4)", 1, name)
    t = sub(t, """      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
      END""", """      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
      RETURN
      END""", 1, name)
    save(name, t)


# ====================================================================
def do_quest1():
    name = 'quest1.f'
    t = load(name)
    t = common(t, name, dict(qstcom=10, foriosdef=3, establish=1, set_scroll=6, seedor=1,
                             time=1, secnds=1, call_sleep=8, call_rename=1,
                             def_rename=1, call_kill=1, def_kill=1))
    t = sub(t, '      PROGRAM QUEST1\n', '      SUBROUTINE QUEST1\n', 1, name)

    # LIB$SCREEN_INFO is not in the distribution; it came from LIBRARY.OLB.
    # Only B is used, to pick the ANSI rather than the VT52 "erase to end
    # of line", so SCRINF reports an ANSI terminal.
    t = sub(t, '      CALL LIB$SCREEN_INFO(A,B,C,D)',
            '      CALL SCRINF(A,B,C,D)', 1, name)

    t = sub(t, """      WRITE(6,1) NAME,CLASS1,(STATS(I),I=1,6),CHARLVL,EXPERIENCE,
     +USERNAME
1     FORMAT(' ',A15,5X,A1,5X,6(I3,1X),2X,I3,2X,I7,2X,A12)""",
            """      WRITE(QREC,1) NAME,CLASS1,(STATS(I),I=1,6),CHARLVL,EXPERIENCE,
     +USERNAME
1     FORMAT(A15,5X,A1,5X,6(I3,1X),2X,I3,2X,I7,2X,A12)
      CALL TTYREC(ICHAR(' '),QREC)""", 2, name)

    t = sub(t, """      CHARACTER CLASS1*1

      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]CHARACTER.DTA',
     +STATUS='UNKNOWN',FORM='UNFORMATTED',SHARED)""", """      CHARACTER CLASS1*1,QREC*78
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      CALL KOPEN
      KEYOPN=.TRUE.
      CALL KSEQ_REWIND""", 1, name)
    t = sub(t, "5       READ(21,IOSTAT=IOS) PLAYER",
            "5       CALL KREAD_NEXT(PLAYER,IOS)", 1, name)

    t = sub(t, """      CHARACTER CLASS1*1,USER*12

      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]CHARACTER.DTA',
     +ACCESS='KEYED',ORGANIZATION='INDEXED',STATUS='UNKNOWN',RECL=63,
     +FORM='UNFORMATTED',SHARED,KEY=(1:15:CHARACTER,26:37:CHARACTER))""",
            """      CHARACTER CLASS1*1,USER*12,QREC*78
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      CALL KOPEN
      KEYOPN=.TRUE.""", 1, name)
    t = sub(t, "5       READ(21,IOSTAT=IOS,KEY=USER,KEYID=1) PLAYER",
            "5       CALL KREAD_KEY(PLAYER,USER,1,0,IOS)", 1, name)
    t = sub(t, """7     READ(21,IOSTAT=IOS,END=2) PLAYER
      IF(USER.NE.PLAYER(26:37))IOS=1""", """7     CALL KREAD_NEXT(PLAYER,IOS)
      IF(IOS.LT.0)GOTO 2
      IF(USER.NE.PLAYER(26:37))IOS=1""", 1, name)
    t = drop_lines(t, r'^ *UNLOCK\(UNIT=21\)$', 2, name)

    # ---- SORT ----------------------------------------------------------
    t = sub(t, "17    READ(21,IOSTAT=IOS,KEYGE='               ',KEYID=0) PLAYER",
            "17    CALL KREAD_KEY(PLAYER,'               ',0,1,IOS)", 1, name)
    t = resub(t, r'DELETE\(UNIT=21,ERR=7\)', 'CALL KDELETE(IOS)', 3, name)
    t = sub(t, "7     READ(21,IOSTAT=IOS) PLAYER",
            "7     CALL KREAD_NEXT(PLAYER,IOS)", 1, name)
    t = sub(t, "      INTEGER LEVEL(400),TEMP1,EXP(400),DATT,J",
            "      INTEGER LEVEL(400),TEMP1,EXP(400),DATT,J\n"
            "      CHARACTER QREC*61", 1, name)
    t = sub(t, """        WRITE(6,12) I,NAME1(I),EXP(I),LEVEL(I),USERNAME2(I),CLASS1(I)
12      FORMAT(1X,I3,2X,A15,5X,I7,5X,I2,2X,A12,7X,A1)""",
            """        WRITE(QREC,12) I,NAME1(I),EXP(I),LEVEL(I),USERNAME2(I),
     +CLASS1(I)
12      FORMAT(I3,2X,A15,5X,I7,5X,I2,2X,A12,7X,A1)
        CALL TTYREC(ICHAR(' '),QREC)""", 1, name)

    t = sub(t, "      WRITE(6,FMT='(//)')\n      IF(I.LT.0.OR.I.GT.20)RETURN",
            "      CALL TTYNL(3)\n      IF(I.LT.0.OR.I.GT.20)RETURN", 1, name)
    t = sub(t, "      CALL EXITR\n      END", "      CALL EXITR\n      RETURN\n      END",
            1, name)
    save(name, t)


# ====================================================================
def do_quest2():
    name = 'quest2.f'
    t = load(name)
    t = common(t, name, dict(qstcom=6, establish=1, time=1, secnds=1, seedor=1))
    t = sub(t, "      INCLUDE 'qstcom.inc'\n      EXTERNAL QUEST_ERROR",
            "      SUBROUTINE QUEST2\n      INCLUDE 'qstcom.inc'\n"
            "      EXTERNAL QUEST_ERROR", 1, name)
    t = sub(t, """      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
      END""", """      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
      RETURN
      END""", 1, name)
    save(name, t)


# ====================================================================
def do_quest3():
    name = 'quest3.f'
    t = load(name)
    t = common(t, name, dict(qstcom=41, establish=1, time=1, secnds=1, seedor=1,
                             call_sleep=16, call_print=17, def_print=1, rec=11))
    t = sub(t, '      PROGRAM QUEST3\n', '      SUBROUTINE QUEST3\n', 1, name)
    t = sub(t, "      CALL RUNIT\n\n      END", "      CALL RUNIT\n\n      RETURN\n      END",
            1, name)

    t = sub(t, """      OPEN(UNIT=24,FILE='BSU$USER_2:[00CKKELLE.QUEST]DUNGEON.DTA',
     +STATUS='OLD',ACCESS='DIRECT',ORGANIZATION='RELATIVE',RECL=4,FORM=
     +'FORMATTED',CARRIAGECONTROL='LIST',READONLY)""", """      CHARACTER QFN*256
      CALL QPATH('dungeon.dta',QFN)
      OPEN(UNIT=24,FILE=QFN,STATUS='OLD',ACCESS='DIRECT',RECL=4,
     +FORM='FORMATTED',ACTION='READ')""", 1, name)

    t = sub(t, """      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]DUNNAM.DTA',
     +STATUS='OLD',FORM='FORMATTED',CARRIAGECONTROL='LIST',READONLY)""",
            """      CHARACTER QFN*256
      CALL QPATH('dunnam.dta',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',FORM='FORMATTED',ACTION='READ')""",
            1, name)

    t = sub(t, """      OPEN(UNIT=22,FILE='BSU$USER_2:[00CKKELLE.QUEST]MON.DTA',
     +STATUS='OLD',FORM='FORMATTED',CARRIAGECONTROL='LIST',READONLY)""",
            """      CHARACTER QFN*256
      CALL QPATH('mon.dta',QFN)
      OPEN(UNIT=22,FILE=QFN,STATUS='OLD',FORM='FORMATTED',ACTION='READ')""",
            1, name)

    # VAX FORTRAN branches to ERR= on end of file when there is no END=;
    # gfortran wants to be told.
    t = sub(t, "        READ(22,1,ERR=2) MONSTER(I),MONSTERSTAT(I,1),MONSTERSTAT(I,2)",
            "        READ(22,1,ERR=2,END=2) MONSTER(I),MONSTERSTAT(I,1),\n"
            "     +MONSTERSTAT(I,2)", 1, name)
    t = sub(t, "          READ(21,FMT='(A37)',ERR=2) DUNNAM(I)",
            "          READ(21,FMT='(A37)',ERR=2,END=2) DUNNAM(I)", 1, name)

    t = sub(t, """      SUBROUTINE FOUNTAIN
      INCLUDE 'qstcom.inc'""", """      SUBROUTINE FOUNTAIN
      INCLUDE 'qstcom.inc'
      LOGICAL SAVINGTHROW""", 1, name)

    t = sub(t, """      SUBROUTINE CAST(DAMAGE,WHERE)
      INCLUDE 'qstcom.inc'
      INTEGER DAMAGE,WHERE""", """      SUBROUTINE CAST(DAMAGE,WHERE)
      INCLUDE 'qstcom.inc'
      INTEGER DAMAGE,WHERE
      LOGICAL MONSAV""", 1, name)

    t = sub(t, "      WRITE(6,FMT='(/)')", "      CALL TTYNL(2)", 1, name)
    t = sub(t, "      WRITE(6,FMT='(//)')", "      CALL TTYNL(3)", 1, name)
    t = resub(t, r"WRITE\(6,FMT='\(\)'\)", "CALL TTYNL(1)", 2, name)
    save(name, t)


# ====================================================================
def do_dndop():
    name = 'dndop.f'
    t = load(name)
    t = common(t, name, dict(qstcom=8, delprc=1, rec=11))

    t = sub(t, "      INCLUDE 'qstcom.inc'\n\n      CALL USERINFO(UIC,USERNAME)",
            "      SUBROUTINE DNDOP\n      INCLUDE 'qstcom.inc'\n"
            "      CHARACTER QFN*256,QREC(24)*132\n\n"
            "      CALL USERINFO(UIC,USERNAME)", 1, name)

    # names that also exist in QUEST3 or are Fortran keywords
    t = resub(t, r'\bSUBROUTINE CHARACTER\b', 'SUBROUTINE DNDCHR', 1, name)
    t = resub(t, r'\bCALL CHARACTER\b', 'CALL DNDCHR', 1, name)
    t = resub(t, r'\bSUBROUTINE BUILDMAP\b', 'SUBROUTINE OPBLDMAP', 1, name)
    t = resub(t, r'\bCALL BUILDMAP\b', 'CALL OPBLDMAP', 1, name)
    t = resub(t, r'\bSUBROUTINE OPENDUNGEON\b(?!1)', 'SUBROUTINE OPENDUN0', 1, name)
    t = resub(t, r'\bCALL OPENDUNGEON\b(?!1)', 'CALL OPENDUN0', 1, name)
    t = resub(t, r'\bSUBROUTINE OPENDUNGEON1\b', 'SUBROUTINE OPENDUN1', 1, name)
    t = resub(t, r'\bCALL OPENDUNGEON1\b', 'CALL OPENDUN1', 2, name)

    # ---- the operator's character dump: a multi record write -----------
    t = sub(t, "        WRITE(5,100) NAME,USERNAME,SECRETNAME,UIC,RUN,LIFE,",
            "        CALL QCLEAR(QREC,24)\n"
            "        WRITE(QREC,100) NAME,USERNAME,SECRETNAME,UIC,RUN,LIFE,",
            1, name)
    t = sub(t, "1WISH,AGE,MOVES,SOLVED\n100   FORMAT(",
            "1WISH,AGE,MOVES,SOLVED\n        CALL QEMIT(QREC,24)\n100   FORMAT(",
            1, name)

    t = sub(t, """      WRITE(5,5) K
3     FORMAT(I4)
5     FORMAT(' Old location: ',I4,/,
     +' New location: '$)""", """      WRITE(QREC(1),5) K
3     FORMAT(I4)
5     FORMAT(' Old location: ',I4)
      CALL TTYREC(ICHAR(' '),QREC(1))
      CALL TTYREC(ICHAR('$'),' New location: ')""", 1, name)
    t = sub(t, """      SUBROUTINE CHANGE
      INCLUDE 'qstcom.inc'""", """      SUBROUTINE CHANGE
      INCLUDE 'qstcom.inc'
      CHARACTER QREC(1)*132""", 1, name)

    t = sub(t, """      WRITE(5,1) LEVELLENGTH,LEVELWIDTH,STAIRSUPX,STAIRSUPY,
     +STAIRSDOWNX,STAIRSDOWNY""", """      CALL QCLEAR(QREC,12)
      WRITE(QREC,1) LEVELLENGTH,LEVELWIDTH,STAIRSUPX,STAIRSUPY,
     +STAIRSDOWNX,STAIRSDOWNY
      CALL QEMIT(QREC,12)""", 1, name)
    t = sub(t, """      DO 12 I1=1,LEVELLENGTH
      WRITE(5,13) (MAP(I1,K), K=1,LEVELWIDTH)
13    FORMAT(1X,<LEVELWIDTH>(I4,1X))""", """      DO 12 I1=1,LEVELLENGTH
      CALL QCLEAR(QREC,1)
      WRITE(QREC,13) (MAP(I1,K), K=1,LEVELWIDTH)
13    FORMAT(21(I4,1X))
      CALL QEMIT(QREC,1)""", 1, name)
    t = sub(t, """      SUBROUTINE LIST_LEVEL
      INCLUDE 'qstcom.inc'""", """      SUBROUTINE LIST_LEVEL
      INCLUDE 'qstcom.inc'
      CHARACTER QREC(12)*132""", 1, name)

    # ---- MAPPED.DAT, and the two dungeon file opens --------------------
    t = sub(t, """      OPEN(UNIT=22,FILE='BSU$USER_2:[00CKKELLE.QUEST]MAPPED.DAT',
     +STATUS='NEW')""", """      CALL QPATH('mapped.dat',QFN)
      OPEN(UNIT=22,FILE=QFN,STATUS='UNKNOWN')""", 1, name)
    t = sub(t, """      CHARACTER ROW(4)*127,WEST(8)*1,SAVE*127
      CHARACTER NORTH(8)*7,NAMES(6)*20""", """      CHARACTER ROW(4)*127,WEST(8)*1,SAVE*127
      CHARACTER NORTH(8)*7,NAMES(6)*20,QFN*256""", 1, name)
    t = sub(t, """        WRITE(22,17) ROW(1),ROW(2),ROW(3),ROW(4)
17      FORMAT(' ',A<I1>,/' ',A<I2>,/' ',A<I3>,/' ',A<I4>)""",
            """        WRITE(22,17) ROW(1)(1:I1),ROW(2)(1:I2),ROW(3)(1:I3),
     +ROW(4)(1:I4)
17      FORMAT(' ',A,/' ',A,/' ',A,/' ',A)""", 1, name)

    t = sub(t, """      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]DUNGEON.DTA',
     +STATUS='OLD',ACCESS='DIRECT',ORGANIZATION='RELATIVE',
     +FORM='FORMATTED',CARRIAGECONTROL='LIST',RECL=4)""", """      CHARACTER QFN*256
      CALL QPATH('dungeon.dta',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',ACCESS='DIRECT',
     +FORM='FORMATTED',RECL=4)""", 1, name)
    t = sub(t, """      OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]DUNGEON.DTA',
     +STATUS='OLD',ACCESS='DIRECT',ORGANIZATION='RELATIVE',
     +FORM='FORMATTED',CARRIAGECONTROL='LIST',RECL=4,READONLY)""", """      CHARACTER QFN*256
      CALL QPATH('dungeon.dta',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',ACCESS='DIRECT',
     +FORM='FORMATTED',RECL=4,ACTION='READ')""", 1, name)
    save(name, t)


if __name__ == '__main__':
    do_lib()
    do_quest()
    do_quest1()
    do_quest2()
    do_quest3()
    do_dndop()
    print('%d edits applied' % edits)
