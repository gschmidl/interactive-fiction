/* advent.c -- ADVENTURE, PL/1 VERSION 4.0, transliterated to C.
 *
 * This is the SHARE/CBT-tape PL/I Adventure (tapecave/PROGRAM.txt),
 * carried across statement by statement rather than rewritten: the
 * procedure names, the label names, the variable names and the order of
 * the code all follow the original, so this file can be read against
 * adventure.pli side by side.  PL/I's data model is preserved by
 * plisup.h (fixed-length blank-padded CHARACTER, 1-based SUBSTR, and so
 * on) rather than being converted to C strings.
 *
 * This part is the body of the PROGRAM procedure, adventure.pli lines
 * 611-3188 plus the DEALLOC epilogue at 4700.  The internal procedures
 * are in advsubs.c.
 */
#include "advent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void statics_init(void);

/* GET STRING (CARD) EDIT (...) (n F(8)) -- the fixed 8-column fields
 * the database uses throughout. */
static fixed31 cardf8(int field)          /* field 1..10 */
{
    return pl_get_f(CARD, (int)sizeof CARD, (field - 1) * 8 + 1, 8);
}

/* PUT STRING (OUTSTR) EDIT (n,' of ',m,text) (F(6),A,F(6),A); LINEOUT */
static void edit_report(fixed31 n, fixed31 m, const char *text)
{
    char buf[133];
    int p = 0;
    pl_edit_f(buf + p, 6, n);              p += 6;
    memcpy(buf + p, " of ", 4);            p += 4;
    pl_edit_f(buf + p, 6, m);              p += 6;
    memcpy(buf + p, text, strlen(text));   p += (int)strlen(text);
    pl_vassign(&OUTSTR, buf, p);
    LINEOUT();
}

void PROGRAM(void)
{
    fixed31 CLSSES_dummy;
    (void)CLSSES_dummy;

    switch (setjmp(pli_goto)) {
    case 0:            break;
    case JMP_DEALLOC:  goto DEALLOC;
    case JMP_L31:      goto L31;
    case JMP_L20000:   goto L20000;
    }

/* ON ATTENTION / ON ERROR / ON ENDFILE have no counterpart here: the
 * attention handler walked an MVS PSA->TCB->JSCB->RLGB->ECT pointer
 * chain, ON ERROR only existed to turn a PL/I condition into BUG(99),
 * and the ENDFILE flags are set by the read routines directly. */

    CLRSCRN();
    if (PLIRETV() == 0) TTYPE[0] = 'V';                  /* ON A VDU */
    pl_vassign(&OUTSTR, LIT("Initializing..."));
    LINEOUT();

    DATETIME(DATE_STG);
    THISDAY(&OUTSTR);
    LINEOUT();
    pl_assign(TSOID, sizeof TSOID, LIT("PLAYER  "));
    pl_substr_assign(MSGPRMS, sizeof MSGPRMS, 9, 8, TSOID, sizeof TSOID);
    pl_assign(PW, 4, TSOID, 4);
    TEST_WORD = -TEST_WORD;
    pl_substr_assign(USERID, sizeof USERID, 1, 4, PW, 4);
    pl_assign(PW, 4, TSOID + 4, 4);
    TEST_WORD = -TEST_WORD;
    {
        char tmp[4];
        memcpy(tmp, PW, 3);
        tmp[3] = TSOID[7];
        pl_substr_assign(USERID, sizeof USERID, 5, 4, tmp, 4);
    }
    TODAY = DATE_PIC;
    HHMM = PIC_HHMM;

    UNAUTH = OFF;

    L = INPARM.len;
    if (L > 4) {
        /* The "magic parm of the day" wizard login.  Left intact but
         * unreachable: WIZARD is forced on below, and the guard
         * SUBSTR(INPARM,5,1) > 'Z' relies on EBCDIC collation, where
         * the digits sort above the letters -- in ASCII they sort
         * below, so it would never fire here anyway. */
        if (pl_eq(INPARM.s, 4, LIT("$\242^%"))) {
            if (INPARM.s[4] > 'Z')
                YDCHR[0] = INPARM.s[4];
            if (DIGIT == DAY_N) WIZARD = ON;
        }
        if (L > 5) {
            if (INPARM.s[5] == '#') { /* DSSTATUS = 'MOD ' -- MVS only */ }
            if (INPARM.s[5] == '@') goto PREALLOC;
        }
    }
    R062A10();                  /* allocate STORAGE if not preallocated */
    R062A10();                  /* allocate OBJECT                      */
PREALLOC:

    WIZARD = ON;    /* standalone single-player port: always authorized */
    if (WIZARD) goto AUTHOK;

    /* --- the shared-mainframe login gate, skipped as above --------- */
    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    pl_substr_assign(MSGPRMS, sizeof MSGPRMS, 1, 7, ADVREC, 7);
    I = pl_index(ADVREC, sizeof ADVREC, USERID, 7);
    if (I == 0) {
        UNAUTH = ON;
        {
            char buf[133];
            int n = 0;
            memcpy(buf + n, "Userid ", 7);            n += 7;
            memcpy(buf + n, TSOID, 8);                n += 8;
            memcpy(buf + n, "is not authorized to run this program.", 37);
            n += 37;
            pl_vassign(&OUTSTR, buf, n);
        }
        LINEOUT();
        LINESKP();
    } else {
        INTEGER = (fixed15)((unsigned char)ADVREC[I + 7 - 1]);
        INTEGER = INTEGER + 1;
        ADVREC[I + 7 - 1] = (char)(INTEGER & 0xFF);
        STORAGE_write(1, ADVREC, sizeof ADVREC);
    }
    STORAGE_close();
    NOSTG = OFF;
    if (pl_eq(USERID, 7, ADVREC, 7)) goto AUTHOK;
    if (!UNAUTH) MSGPRMS[7] = 'A';
    WARNMSG(MSGPRMS);
    if (UNAUTH && PLIRETV() != 0) goto DEALLOC;   /* SIGNAL ATTENTION */

AUTHOK:
    for (J = 1; J <= 4; J++) {              /* GET TRADING HOURS */
        pl_assign(PW, 4, ADVREC + 4781 + J * 4 - 1, 4);
        if (PW[0] == ' ') continue;
        TEST_WORD = -TEST_WORD;
        pl_assign(TIMES[J], 4, PW, 4);
    }
    GETMSGS(TSOID, 'A');                                 /* LISTBC */
    OBJECT_open();

/* START NEW DATA SECTION.  SECT IS THE SECTION NUMBER. */

L1002:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[2] == 'S') goto L1002;                 /* IGNORE ESD CARD */
    if (CARD[0] == HEX02) REVERT();
    SECT = cardf8(1);
    OLDLOC = -1;
    {
        char buf[133];
        pl_assign(buf, 17, LIT("Reading section #"));
        pl_edit_f(buf + 17, 2, SECT);
        pl_vassign(&OUTSTR, buf, 19);
    }
    LINEOUT();
    switch (SECT) {
    case  0: goto L1100;
    case  1: goto L1004;
    case  2: goto L1004;
    case  3: goto L1030;
    case  4: goto L1040;
    case  5: goto L1004;
    case  6: goto L1004;
    case  7: goto L1050;
    case  8: goto L1060;
    case  9: goto L1070;
    case 10: goto L1004;
    case 11: goto L1080;
    case 12: goto L1004;
    default: BUG(9);
    }

/* SECTIONS 1, 2, 5, 6, 10.  READ MESSAGES AND SET UP POINTERS. */

L1004:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    LOC = cardf8(1);
    for (J = LINUSE + 1; J <= LINUSE + 14; J++)
        pl_assign(LINES[J], 5, CARD + 8 + (J - (LINUSE + 1)) * 5, 5);
    pl_assign(KKWORD, sizeof KKWORD, CARD + 78, 2);
    if (!pl_eq(KKWORD, sizeof KKWORD, LIT(" "))) BUG(0);
    if (LOC == -1) goto L1002;
    for (K = 1; K <= 14; K++) {
        KK = LINUSE + 15 - K;
        if (!pl_eq(LINES[KK], 5, LIT(" "))) goto L1007;
    }
    BUG(1);
L1007:
    YDPIC_SET(KK + 1);
    pl_assign(LINES[LINUSE], 5, YDCHR, 5);
    if (LOC == OLDLOC) goto L1020;
    YDPIC_SET(YDPIC + 50000);
    pl_assign(LINES[LINUSE], 5, YDCHR, 5);
    if (SECT == 12) goto L1013;
    if (SECT == 10) goto L1012;
    if (SECT ==  6) goto L1011;
    if (SECT ==  5) goto L1010;
    if (SECT ==  1) goto L1008;

    STEXT[LOC] = LINUSE;
    goto L1020;

L1008:
    LTEXT[LOC] = LINUSE;
    goto L1020;

L1010:
    if (LOC > 0 && LOC <= 100) PTEXT[LOC] = LINUSE;
    goto L1020;

L1011:
    if (LOC > RTXSIZ) BUG(6);
    RTEXT[LOC] = LINUSE;
    goto L1020;

L1012:
    CTEXT[CLSSES] = LINUSE;
    CVAL[CLSSES] = LOC;
    CLSSES = CLSSES + 1;
    goto L1020;

L1013:
    if (LOC > MAGSIZ) BUG(6);
    MTEXT[LOC] = LINUSE;

L1020:
    LINUSE = KK + 1;
    pl_assign(LINES[LINUSE], 5, LIT("-1   "));
    OLDLOC = LOC;
    if (LINUSE + 14 > LINSIZ) BUG(2);
    goto L1004;

/* SECTION 3: the travel table. */

L1030:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    LOC = cardf8(1);
    NEWLOC = cardf8(2);
    for (I = 1; I <= 8; I++) TK[I] = cardf8(2 + I);
    if (LOC == -1) goto L1002;
    if (KEY[LOC] != 0) goto L1033;
    KEY[LOC] = TRVS;
    goto L1035;
L1033:
    TRAVEL[TRVS - 1] = -TRAVEL[TRVS - 1];
L1035:
    for (L = 1; L <= 8; L++) {
        if (TK[L] == 0) goto L1039;
        TRAVEL[TRVS] = NEWLOC * 1000 + TK[L];
        TRVS = TRVS + 1;
        if (TRVS == TRVSIZ) BUG(3);
    }
L1039:
    TRAVEL[TRVS - 1] = -TRAVEL[TRVS - 1];
    goto L1030;

/* SECTION 4: the vocabulary. */

L1040:
    for (TABNDX = 1; TABNDX <= TABSIZ; TABNDX++) {
        EOCARD = (bit1)OBJECT_read(CARD);
        if (EOCARD) BUG(10);
        if (CARD[0] == HEX02) REVERT();
        KTAB[TABNDX] = cardf8(1);
        pl_assign(ATAB[TABNDX], 5, CARD + 8, 5);
        if (KTAB[TABNDX] == -1) goto L1002;
        {                                  /* ATABB(TABNDX) = ^ATABB(...) */
            int b;
            for (b = 0; b < 5; b++)
                ATAB[TABNDX][b] = (char)(~(unsigned char)ATAB[TABNDX][b]);
        }
    }
    BUG(4);

/* SECTION 7: initial object locations. */

L1050:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    OBJ = cardf8(1);
    J = cardf8(2);
    K = cardf8(3);
    if (OBJ == -1) goto L1002;
    PLAC[OBJ] = J;
    FIXD[OBJ] = K;
    goto L1050;

/* SECTION 8: action defaults. */

L1060:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    VERB = cardf8(1);
    J = cardf8(2);
    if (VERB == -1) goto L1002;
    ACTSPK[VERB] = J;
    goto L1060;

/* SECTION 9: liquid assets and other conditions. */

L1070:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    K = cardf8(1);
    for (I = 1; I <= 9; I++) TK[I] = cardf8(1 + I);
    if (K == -1) goto L1002;
    for (I = 1; I <= 9; I++) {
        LOC = TK[I];
        if (LOC == 0) goto L1070;
        if (BITSET(LOC, K)) BUG(8);
        COND[LOC] = COND[LOC] + (fixed31)(1L << K);
    }
    goto L1070;

/* SECTION 11: hints. */

L1080:
    HNTMAX = 0;
L1081:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) BUG(10);
    if (CARD[0] == HEX02) REVERT();
    K = cardf8(1);
    for (I = 1; I <= 4; I++) TK[I] = cardf8(1 + I);
    if (K == -1) goto L1002;
    if (K == 0) goto L1081;
    if (K < 0 || K > HNTSIZ) BUG(7);
    for (I = 1; I <= 4; I++)
        HINTS[K][I] = TK[I];
    HNTMAX = (HNTMAX > K) ? HNTMAX : K;
    goto L1081;

/* FINISH CONSTRUCTING INTERNAL DATA FORMAT */

L1100:
    OBJECT_close();
    for (I = 1; I <= LOCSIZ; I++) {
        if (LTEXT[I] == 0 || KEY[I] == 0) continue;
        K = KEY[I];
        if (labs((long)TRAVEL[K]) % 1000 == 1) COND[I] = 2;
    }

    for (I = 1; I <= 100; I++) {
        K = 101 - I;
        if (FIXD[K] > 0) {
            DROP(K + 100, FIXD[K]);
            DROP(K, PLAC[K]);
        }
    }

    for (I = 1; I <= 100; I++) {
        K = 101 - I;
        FIXED[K] = FIXD[K];
        if (PLAC[K] != 0 && FIXD[K] <= 0) DROP(K, PLAC[K]);
    }


    for (I = 50; I <= MAXTRS; I++) {
        if (PTEXT[I] != 0) PROP[I] = -1;
        TALLY = TALLY - PROP[I];
    }

/* DEFINE SOME HANDY MNEMONICS.  THESE CORRESPOND TO OBJECT NUMBERS. */

    KEYS   = VOCAB("KEYS ", 1);
    LAMP   = VOCAB("LAMP ", 1);
    GRATE  = VOCAB("GRATE", 1);
    CAGE   = VOCAB("CAGE ", 1);
    ROD    = VOCAB("ROD  ", 1);
    ROD2   = ROD + 1;
    STEPS  = VOCAB("STEPS", 1);
    BIRD   = VOCAB("BIRD ", 1);
    DOOR   = VOCAB("DOOR ", 1);
    PILLOW = VOCAB("PILLO", 1);
    SNAKE  = VOCAB("SNAKE", 1);
    FISSUR = VOCAB("FISSU", 1);
    TABLET = VOCAB("TABLE", 1);
    CLAM   = VOCAB("CLAM ", 1);
    OYSTER = VOCAB("OYSTE", 1);
    MAGZIN = VOCAB("MAGAZ", 1);
    DWARF  = VOCAB("DWARF", 1);
    KNIFE  = VOCAB("KNIFE", 1);
    FOOD   = VOCAB("FOOD ", 1);
    BOTTLE = VOCAB("BOTTL", 1);
    WATER  = VOCAB("WATER", 1);
    OIL    = VOCAB("OIL  ", 1);
    MIRROR = VOCAB("MIRRO", 1);
    PLANT  = VOCAB("PLANT", 1);
    PLANT2 = PLANT + 1;
    AXE    = VOCAB("AXE  ", 1);
    DRAGON = VOCAB("DRAGO", 1);
    CHASM  = VOCAB("CHASM", 1);
    TROLL  = VOCAB("TROLL", 1);
    TROLL2 = TROLL + 1;
    BEAR   = VOCAB("BEAR ", 1);
    MESSAG = VOCAB("MESSA", 1);
    VEND   = VOCAB("VENDI", 1);
    BATTER = VOCAB("BATTE", 1);
    LADDER = VOCAB("LADDE", 1);
    BRACELET = VOCAB("BRACE", 1);
    FFIELD = VOCAB("FIELD", 1);

/* OBJECTS FROM 50 THROUGH WHATEVER ARE TREASURES.  HERE ARE A FEW. */

    NUGGET = VOCAB("GOLD ", 1);
    COINS  = VOCAB("COINS", 1);
    CHEST  = VOCAB("CHEST", 1);
    EGGS   = VOCAB("EGGS ", 1);
    TRIDNT = VOCAB("TRIDE", 1);
    VASE   = VOCAB("VASE ", 1);
    EMRALD = VOCAB("EMERA", 1);
    PYRAM  = VOCAB("PYRAM", 1);
    PEARL  = VOCAB("PEARL", 1);
    RUG    = VOCAB("RUG  ", 1);
    CHAIN  = VOCAB("CHAIN", 1);
    RUBY   = VOCAB("RUBY ", 1);
    ORAC   = VOCAB("ORAC ", 1);

/* THESE ARE MOTION-VERB NUMBERS. */

    BACK   = VOCAB("BACK ", 0);
    NULLX  = VOCAB("NULL ", 0);
    DPRSSN = VOCAB("DEPRE", 0);
    ENTRNC = VOCAB("ENTRA", 0);
    LOOK   = VOCAB("LOOK ", 0);
    CAVE   = VOCAB("CAVE ", 0);

/* The action verbs SAY/LOCK/THROW/FIND/INVENT/SUSPEND/RESTORE are
   initialised in the declarations instead -- see the note in the PL/I:
   their numbers appear as literals in SELECT statements anyway, so
   looking them up bought no independence from the database. */

    for (I = 0; I <= 4; I++)
        if (RTEXT[2 * I + 81] != 0) MAXDIE = I + 1;

/* REPORT ON AMOUNT OF ARRAYS ACTUALLY USED, TO PERMIT REDUCTIONS. */

    for (K = 1; K <= LOCSIZ; K++) {
        KK = LOCSIZ + 1 - K;
        if (LTEXT[KK] != 0) goto L1997;
    }

    OBJ = 0;
L1997:
    for (K = 1; K <= 100; K++)
        if (PTEXT[K] != 0) OBJ = OBJ + 1;

    for (K = 1; K <= TABNDX; K++)
        if (KTAB[K] / 1000 == 2) VERB = KTAB[K] - 2000;

    for (K = 1; K <= MAGSIZ; K++) {
        M = MAGSIZ + 1 - K;
        if (MTEXT[M] != 0) goto L1990;
    }

L1990:
    for (K = 1; K <= RTXSIZ; K++) {
        J = RTXSIZ + 1 - K;
        if (RTEXT[J] != 0) goto L1991;
    }

L1991:
    if (!WIZARD) goto NOREPRT;

    K = 100;
    LINESKP();
    edit_report(LINUSE, LINSIZ, " words of messages");
    edit_report(TRVS,   TRVSIZ, " travel options");
    edit_report(TABNDX, TABSIZ, " vocabulary words");
    edit_report(KK,     LOCSIZ, " locations");
    edit_report(OBJ,    K,      " objects");
    edit_report(VERB,   VRBSIZ, " action verbs");
    edit_report(J,      RTXSIZ, " rtext messages");
    edit_report(M,      MAGSIZ, " mtext messages");
    edit_report(CLSSES, CLSSIZ, " class messages");
    edit_report(HNTMAX, HNTSIZ, " hints");
    LINESKP();

/*  FINALLY, SINCE WE'RE CLEARLY SETTING THINGS UP FOR THE FIRST TIME... */

NOREPRT:
    RSPEAK(205);
    LINESKP();
    LINESKP();

/*  START-UP, DWARF STUFF */

L1:
    ITIME(&I);
    for (J = 1; J <= I; J++) {
        /* CALL RAN(1) -- commented out in the original, which is why
           the game is deterministic despite seeding off the clock */
    }
    I = RAN(1);
    HINTED[3] = YES(65, 1, 0);
    LOC = NEWLOC = 1;
    LIMIT = 330;
    if (HINTED[3]) LIMIT = 1000;

/*  CAN'T LEAVE CAVE ONCE IT'S CLOSING (EXCEPT BY MAIN OFFICE). */

L2:
    if (NEWLOC >= 9 || NEWLOC == 0 || !CLOSNG) goto L71;
    RSPEAK(130);
    NEWLOC = LOC;
    if (!PANIC) CLOCK2 = 15;
    PANIC = ON;

/* SEE IF A DWARF HAS SEEN HIM AND HAS COME FROM WHERE HE WANTS TO GO. */

L71:
    if (NEWLOC == LOC || FORCED(LOC) || BITSET(LOC, 3)) goto L74;
    for (I = 1; I <= 5; I++) {
        if (!(ODLOC[I] != NEWLOC || !DSEEN[I])) {
            NEWLOC = LOC;
            RSPEAK(2);
            goto L74;
        }
    }
L74:
    LOC = NEWLOC;

/* DWARF STUFF. */

    if (LOC == 0 || FORCED(LOC) || BITSET(NEWLOC, 3)) goto L2000;
    if (DFLAG != 0) goto L6000;
    if (LOC >= 15) DFLAG = 1;
    goto L2000;

/* WHEN WE ENCOUNTER THE FIRST DWARF, WE KILL 0, 1, OR 2 OF THE 5. */

L6000:
    if (DFLAG != 1) goto L6010;
    if (LOC < 15 || PCT(95)) goto L2000;
    DFLAG = 2;
    for (I = 1; I <= 2; I++) {
        J = 1 + RAN(5);
        if (PCT(50)) DLOC[J] = 0;
    }
    for (I = 1; I <= 5; I++) {
        if (DLOC[I] == LOC) DLOC[I] = DALTLC;
        ODLOC[I] = DLOC[I];
    }
    RSPEAK(3);
    DROP(AXE, LOC);
    goto L2000;

/* THINGS ARE IN FULL SWING.  MOVE EACH DWARF AT RANDOM. */

L6010:
    DTOTAL = 0;
    ATTACK = 0;
    STICK = 0;
    for (I = 1; I <= 6; I++) {
        if (DLOC[I] == 0) continue;
        J = 1;
        KK = DLOC[I];
        KK = KEY[KK];
        if (KK == 0) goto L6016;
L6012:
        NEWLOC = (labs((long)TRAVEL[KK]) / 1000) % 1000;
        if (NEWLOC > 300 || NEWLOC < 15 || NEWLOC == ODLOC[I]
            || (J > 1 && NEWLOC == TK[J - 1]) || J >= 20
            || NEWLOC == DLOC[I] || FORCED(NEWLOC)
            || (I == 6 && BITSET(NEWLOC, 3))
            || labs((long)TRAVEL[KK]) / 1000000 == 100) goto L6014;
        TK[J] = NEWLOC;
        J = J + 1;
L6014:
        KK = KK + 1;
        if (TRAVEL[KK - 1] >= 0) goto L6012;
L6016:
        TK[J] = ODLOC[I];
        if (J >= 2) J = J - 1;
        J = 1 + RAN(J);
        ODLOC[I] = DLOC[I];
        DLOC[I] = TK[J];
        DSEEN[I] = (bit1)((DSEEN[I] && LOC >= 15)
                          || (DLOC[I] == LOC || ODLOC[I] == LOC));
        if (!DSEEN[I]) continue;
        DLOC[I] = LOC;
        if (I != 6) goto L6027;

/* THE PIRATE'S SPOTTED HIM. */

        if (LOC == CHLOC || PROP[CHEST] >= 0) continue;
        K = 0;
        for (J = 50; J <= MAXTRS; J++) {
            if (J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD]))
                goto L6020;
            if (TOTING(J)) goto L6022;
L6020:
            if (HERE(J)) K = 1;
        }
        if (TALLY == TALLY2 + 1 && K == 0 && PLACE[CHEST] == 0
            && HERE(LAMP) && PROP[LAMP] == 1) goto L6025;
        if (ODLOC[6] != DLOC[6] && PCT(20)) RSPEAK(127);
        continue;

L6022:
        RSPEAK(128);

/* DON'T STEAL CHEST BACK FROM TROLL! */

        if (PLACE[MESSAG] == 0) MOVE_(CHEST, CHLOC);
        MOVE_(MESSAG, CHLOC2);
        for (J = 50; J <= MAXTRS; J++) {
            if (J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD]))
                continue;
            if (AT(J) && FIXED[J] == 0) CARRY(J, LOC);
            if (TOTING(J)) DROP(J, CHLOC);
        }
L6024:
        DLOC[6] = CHLOC;
        ODLOC[6] = CHLOC;
        DSEEN[6] = OFF;
        continue;

L6025:
        RSPEAK(186);
        MOVE_(CHEST, CHLOC);
        MOVE_(MESSAG, CHLOC2);
        goto L6024;

/* THIS THREATENING LITTLE DWARF IS IN THE ROOM WITH HIM! */

L6027:
        DTOTAL = DTOTAL + 1;
        if (ODLOC[I] != DLOC[I]) continue;
        ATTACK = ATTACK + 1;
        if (KNFLOC >= 0) KNFLOC = LOC;
        if (RAN(1000) < 95 * (DFLAG - 2)) STICK = STICK + 1;
    }

/* NOW WE KNOW WHAT'S HAPPENING.  LET'S TELL THE POOR SUCKER ABOUT IT. */

    if (DTOTAL == 0) goto L2000;
    if (DTOTAL == 1) goto L75;
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(207);                  /* SOME THREATENING LITTLE DWARFS */
    pl_edit_f(INHIB, 1, DTOTAL);
    pl_vsubstr_assign(&OUTSTR, 11, 1, INHIB, 1);
    INHIB[0] = 'N';
    LINEOUT();
    goto L77;
L75:
    RSPEAK(4);
L77:
    if (ATTACK == 0) goto L2000;
    if (DFLAG == 2) DFLAG = 3;

/* DWARFS GET *VERY* MAD! */

    if (ATTACK == 1) goto L79;
    INHIB[0] = 'Y';
    RSPEAK(208);                /* SOME OF THEM THROW KNIVES AT YOU */
    pl_edit_f(INHIB, 1, ATTACK);
    pl_vsubstr_assign(&OUTSTR, 1, 1, INHIB, 1);
    INHIB[0] = 'N';
    LINEOUT();
    K = 6;
L82:
    if (STICK > 1) goto L83;
    RSPEAK(K + STICK);
    if (STICK == 0) goto L2000;
    goto L84;
L83:
    INHIB[0] = 'Y';
    RSPEAK(209);                          /* SOME OF THEM GET YOU! */
    pl_edit_f(INHIB, 1, STICK);
    pl_vsubstr_assign(&OUTSTR, 1, 1, INHIB, 1);
    INHIB[0] = 'N';
    LINEOUT();
L84:
    OLDLC2 = LOC;
    goto L99;

L79:
    RSPEAK(5);
    K = 52;
    goto L82;

/* DESCRIBE THE CURRENT LOCATION AND (MAYBE) GET NEXT COMMAND. */

L2000:
    if (LOC == 0) goto L99;
    KK = STEXT[LOC];
    if (ABB[LOC] % ABBNUM == 0 || KK == 0) KK = LTEXT[LOC];
    if (FORCED(LOC) || !DARK(0)) goto L2001;
    if (WZDARK && PCT(40)) goto L90;
    KK = RTEXT[16];
L2001:
    if (TOTING(BEAR)) RSPEAK(141);
    SPEAK(KK);
    K = 1;
    if (FORCED(LOC)) goto L8;
    if (LOC == 33 && PCT(25) && !CLOSNG) RSPEAK(8);

/* PRINT OUT DESCRIPTIONS OF OBJECTS AT THIS LOCATION. */

    if (DARK(0)) goto L2012;
    ABB[LOC] = ABB[LOC] + 1;
    I = ATLOC[LOC];
L2004:
    if (I == 0) goto L2012;
    OBJ = I;
    if (OBJ > 100) OBJ = OBJ - 100;
    if (OBJ == STEPS && TOTING(NUGGET)) goto L2008;
    if (PROP[OBJ] >= 0) goto L2006;
    if (CLOSED) goto L2008;
    PROP[OBJ] = 0;
    if (OBJ == RUG || OBJ == CHAIN) PROP[OBJ] = 1;
    TALLY = TALLY - 1;

/* IF REMAINING TREASURES TOO ELUSIVE, ZAP HIS LAMP. */

    if (TALLY == TALLY2 && TALLY != 0) LIMIT = (35 < LIMIT) ? 35 : LIMIT;
L2006:
    KK = PROP[OBJ];
    if (OBJ == STEPS && LOC == FIXED[STEPS]) KK = 1;
    if (OBJ == LADDER && LOC == FIXED[LADDER]) KK = KK + 1;
    PSPEAK(OBJ, KK);
L2008:
    I = LINK[I];
    goto L2004;

L2009:
    K = 54;
L2010:
    SPK = K;
L2011:
    RSPEAK(SPK);

L2012:
    VERB = 0;
    OBJ = 0;

/* CHECK IF THIS LOC IS ELIGIBLE FOR ANY HINTS. */

L2600:
    for (HINT = 4; HINT <= HNTMAX; HINT++) {
        if (!HINTED[HINT]) {
            if (!BITSET(LOC, HINT)) HINTLC[HINT] = -1;
            HINTLC[HINT] = HINTLC[HINT] + 1;
            if (HINTLC[HINT] >= HINTS[HINT][1]) goto L40000;
        }
    }

/* KICK THE RANDOM NUMBER GENERATOR JUST TO ADD VARIETY TO THE CHASE. */

L2602:
    if (!CLOSED) goto L2605;
    if (PROP[OYSTER] < 0 && TOTING(OYSTER)) PSPEAK(OYSTER, 1);
    for (I = 1; I <= 100; I++)
        if (TOTING(I) && PROP[I] < 0) PROP[I] = -1 - PROP[I];
L2605:
    WZDARK = DARK(0);
    if (KNFLOC > 0 && KNFLOC != LOC) KNFLOC = 0;
    I = RAN(1);
L2606:
    GETIN(WD1, WD1X, WD2, WD2X);

L2608:
    if (TURNS == 1500) {                     /* ALLOW 1500 TURNS */
        CLRSCRN();
        MSPEAK(1);
        if (WIZARD) {
            TURNS = TURNS + 1;
            goto L2606;
        }
        goto L20000;
    }

    if (TURNS == 0) {
        if (WIZARD) goto SKIPTST;
        if (VOCAB(WD1, -1) == 2035) WIZPROC();            /* WIZAR */
        if (WIZARD) goto L2606;
        if ((DAY_N == 0 || DAY_N == 6) && !UNAUTH) goto SKIPTST;
        if ((((pl_cmp(CHR_HHMM, 4, TIMES[1], 4) > 0 &&
               pl_cmp(CHR_HHMM, 4, TIMES[2], 4) < 0)
              || (pl_cmp(CHR_HHMM, 4, TIMES[3], 4) > 0 &&
                  pl_cmp(CHR_HHMM, 4, TIMES[4], 4) < 0))
             && !DEMOGM)
            || UNAUTH) {
            DEMOCHK();
            goto L2606;
        }
    }
SKIPTST:

    if (DEMOGM) {
        if (TURNS == 100) {          /* ALLOW 100 TURNS FOR DEMO GAME */
            CLRSCRN();
            MSPEAK(1);
            goto L20000;
        }
    }

/* EVERY INPUT, CHECK "FOOBAR" FLAG. */

    FOOBAR = (0 < -FOOBAR) ? 0 : -FOOBAR;
    TURNS = TURNS + 1;
    if (pl_eq(WD1, 5, LIT("@")) && WIZARD) {   /* DISPLAY CURRENT LOCATION */
        pl_blank(LOC_LINE, sizeof LOC_LINE);
        PLOC_SET(1, LOC);
        pl_vassign(&OUTSTR, LOC_LINE, sizeof LOC_LINE);
        LINEOUT();
        goto L2606;
    }
    if (pl_eq(WD1, 5, LIT("DLOCS")) && WIZARD) { /* DISPLAY DWARFS' LOCS */
        for (M = 1; M <= 6; M++)
            PLOC_SET(M, DLOC[M]);
        pl_vassign(&OUTSTR, LOC_LINE, sizeof LOC_LINE);
        LINEOUT();
        goto L2606;
    }
    if (VERB == SAY && !pl_eq(WD2, 5, LIT("     "))) VERB = 0;
    if (VERB == SAY) goto L4090;
    if (TALLY == 0 && LOC >= 15 && LOC != 33) CLOCK1 = CLOCK1 - 1;
    if (CLOCK1 == 0) goto L10000;
    if (CLOCK1 < 0) CLOCK2 = CLOCK2 - 1;
    if (CLOCK2 == 0) goto L11000;
    if (PROP[LAMP] == 1) LIMIT = LIMIT - 1;
    if (LIMIT <= 30 && HERE(BATTER) && PROP[BATTER] == 0
        && HERE(LAMP)) goto L12000;
    if (LIMIT == 0) goto L12400;
    if (LIMIT < 0 && LOC <= 8) goto L12600;
    if (LIMIT <= 30) goto L12200;
L19999:
    K = 43;
    if (LIQLOC(LOC) == WATER) K = 70;
    if (pl_eq(WD1, 5, LIT("ENTER"))
        && (pl_eq(WD2, 5, LIT("STREA")) || pl_eq(WD2, 5, LIT("WATER"))))
        goto L2010;
    if (pl_eq(WD1, 5, LIT("ENTER")) && !pl_eq(WD2, 5, LIT("    ")))
        goto L2800;
    if ((!pl_eq(WD1, 5, LIT("WATER")) && !pl_eq(WD1, 5, LIT("OIL")))
        || (!pl_eq(WD2, 5, LIT("PLANT")) && !pl_eq(WD2, 5, LIT("DOOR"))))
        goto L2610;
    if (AT(VOCAB(WD2, 1))) pl_assign(WD2, 5, LIT("POUR"));
L2610:
    if (!pl_eq(WD1, 5, LIT("WEST"))) goto L2630;
    IWEST = IWEST + 1;
    if (IWEST == 10) RSPEAK(17);
L2630:
    I = VOCAB(WD1, -1);
    if (I == -1) goto L3000;
    K = I % 1000;
    KQ = (fixed15)(I / 1000);
    switch (KQ) {
    case 0: goto L8;
    case 1: goto L5000;
    case 2: goto L4000;
    case 3: goto L2010;
    default: BUG(22);
    }

/* GET SECOND WORD FOR ANALYSIS. */

L2800:
    pl_assign(WD1, 5, WD2, 5);
    pl_assign(WD1X, 5, WD2X, 5);
    pl_assign(WD2, 5, LIT("     "));
    goto L2610;

/* GEE, I DON'T UNDERSTAND. */

L3000:
    SPK = 60;
    if (PCT(20)) SPK = 61;
    if (PCT(20)) SPK = 13;
    RSPEAK(SPK);
    goto L2600;

/* ANALYSE A VERB. */

L4000:
    VERB = K;
    SPK = ACTSPK[VERB];
    if (!pl_eq(WD2, 5, LIT("     ")) && VERB != SAY
        && VERB != SUSPEND && VERB != RESTORE) goto L2800;
    if (VERB == SAY) {
        if (pl_eq(WD2, 5, LIT("     "))) goto L4080;
        else                             goto L4090;
    }
    if (OBJ != 0) goto L4090;

/* ANALYSE AN INTRANSITIVE VERB (IE, NO OBJECT GIVEN YET). */

L4080:
    switch (VERB) {
    case  1: goto L8010;       /* TAKE */
    case  2: goto L8000;       /* DROP */
    case  3: goto L8000;       /* SAY  */
    case  4: goto L8040;       /* OPEN */
    case  5: goto L2009;       /* NOTH */
    case  6: goto L8040;       /* LOCK */
    case  7: goto L9070;       /* ON   */
    case  8: goto L9080;       /* OFF  */
    case  9: goto L8000;       /* WAVE */
    case 10: goto L8000;       /* CALM */
    case 11: goto L2011;       /* WALK */
    case 12: goto L9120;       /* KILL */
    case 13: goto L9130;       /* POUR */
    case 14: goto L8140;       /* EAT  */
    case 15: goto L9150;       /* DRNK */
    case 16: goto L8000;       /* RUB  */
    case 17: goto L8000;       /* TOSS */
    case 18: goto L8180;       /* QUIT */
    case 19: goto L8000;       /* FIND */
    case 20: goto L8200;       /* INVN */
    case 21: goto L8000;       /* FEED */
    case 22: goto L9220;       /* FILL */
    case 23: goto L9230;       /* BLST */
    case 24: goto L8240;       /* SCOR */
    case 25: goto L8250;       /* FOO  */
    case 26: goto L8260;       /* BRF  */
    case 27: goto L8270;       /* READ */
    case 28: goto L8000;       /* BREK */
    case 29: goto L8000;       /* WAKE */
    case 30: goto L8300;       /* SAVE */
    case 31: goto L8310;       /* HOUR */
    case 32: goto L8320;       /* LOG  */
    case 33: goto L8330;       /* LASR */
    case 34: goto L8340;       /* RSTR */
    case 35: goto L8350;       /* WIZD */
    case 36: goto L8360;       /* ORAC */
    default: BUG(23);
    }

/* ANALYSE A TRANSITIVE VERB. */

L4090:
    switch (VERB) {
    case  1: goto L9010;       /* TAKE */
    case  2: goto L9020;       /* DROP */
    case  3: goto L9030;       /* SAY  */
    case  4: goto L9040;       /* OPEN */
    case  5: goto L2009;       /* NOTH */
    case  6: goto L9040;       /* LOCK */
    case  7: goto L9070;       /* ON   */
    case  8: goto L9080;       /* OFF  */
    case  9: goto L9090;       /* WAVE */
    case 10: goto L2011;       /* CALM */
    case 11: goto L2011;       /* WALK */
    case 12: goto L9120;       /* KILL */
    case 13: goto L9130;       /* POUR */
    case 14: goto L9140;       /* EAT  */
    case 15: goto L9150;       /* DRNK */
    case 16: goto L9160;       /* RUB  */
    case 17: goto L9170;       /* TOSS */
    case 18: goto L2011;       /* QUIT */
    case 19: goto L9190;       /* FIND */
    case 20: goto L9190;       /* INVN */
    case 21: goto L9210;       /* FEED */
    case 22: goto L9220;       /* FILL */
    case 23: goto L9230;       /* BLST */
    case 24: goto L2011;       /* SCOR */
    case 25: goto L2011;       /* FOO  */
    case 26: goto L2011;       /* BRF  */
    case 27: goto L9270;       /* READ */
    case 28: goto L9280;       /* BREK */
    case 29: goto L9290;       /* WAKE */
    case 30: goto L2011;       /* SAVE */
    case 31: goto L2011;       /* HOUR */
    case 32: goto L2011;       /* LOG  */
    case 33: goto L2011;       /* LASR */
    case 34: goto L2011;       /* RSTR */
    case 35: goto L2011;       /* WIZD */
    case 36: goto L2011;       /* ORAC */
    default: BUG(24);
    }

/* ANALYSE AN OBJECT WORD. */

L5000:
    OBJ = K;
    if (FIXED[K] != LOC && !HERE(K)) goto L5100;
L5010:
    if (!pl_eq(WD2, 5, LIT("     "))) goto L2800;
    if (VERB != 0) goto L4090;
    A5TOA1(WD1, WD1X, TKWORD, &K);
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(210);            /* WHAT DO YOU WANT TO DO WITH IT? */
    {
        char buf[133];
        int p = 0, q;
        pl_edit_a(buf, 32, OUTSTR.s, OUTSTR.len);  p = 32;
        for (q = 1; q <= K && p < 132; q++) buf[p++] = TKWORD[q][0];
        buf[p++] = '?';
        pl_vassign(&OUTSTR, buf, p);
    }
    INHIB[0] = 'N';
    LINEOUT();
    goto L2600;

L5100:
    if (K != GRATE) goto L5110;
    if (LOC == 1 || LOC == 4 || LOC == 7) K = DPRSSN;
    if (LOC > 9 && LOC < 15) K = ENTRNC;
    if (K != GRATE) goto L8;
L5110:
    if (K != DWARF) goto L5120;
    for (I = 1; I <= 5; I++)
        if (DLOC[I] == LOC && DFLAG >= 2) goto L5010;
L5120:
    if ((LIQ(0) == K && HERE(BOTTLE)) || K == LIQLOC(LOC)) goto L5010;
    if (OBJ != PLANT || !AT(PLANT2) || PROP[PLANT2] == 0) goto L5130;
    OBJ = PLANT2;
    goto L5010;
L5130:
    if (OBJ != KNIFE || KNFLOC != LOC) goto L5140;
    KNFLOC = -1;
    SPK = 116;
    goto L2011;
L5140:
    if (OBJ != ROD || !HERE(ROD2)) goto L5190;
    OBJ = ROD2;
    goto L5010;
L5190:
    if ((VERB == FIND || VERB == INVENT) && pl_eq(WD2, 5, LIT("     ")))
        goto L5010;
    A5TOA1(WD1, WD1X, TKWORD, &K);
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(220);
    {
        char buf[133];
        int p, q;
        pl_edit_a(buf, 9, OUTSTR.s, OUTSTR.len);  p = 9;
        for (q = 1; q <= K && p < 126; q++) buf[p++] = TKWORD[q][0];
        memcpy(buf + p, " here!", 6); p += 6;
        pl_vassign(&OUTSTR, buf, p);
    }
    INHIB[0] = 'N';
    LINEOUT();
    goto L2012;

/* FIGURE OUT THE NEW LOCATION */

L8:
    KK = KEY[LOC];
    NEWLOC = LOC;
    if (KK == 0) BUG(26);
    if (K == NULLX) goto L2;
    if (K == BACK)  goto L20;
    if (K == LOOK)  goto L30;
    if (K == CAVE)  goto L40;
    OLDLC2 = OLDLOC;
    OLDLOC = LOC;

L9:
    LL = (fixed31)labs((long)TRAVEL[KK]);
    if (LL % 1000 == 1 || LL % 1000 == K) goto L10;
    if (TRAVEL[KK] < 0) goto L50;
    KK = KK + 1;
    goto L9;

L10:
    LL = LL / 1000;
L11:
    NEWLOC = LL / 1000;
    K = NEWLOC % 100;
    if (NEWLOC <= 300) goto L13;
    if (PROP[K] != NEWLOC / 100 - 3) goto L16;
L12:
    if (TRAVEL[KK] < 0) BUG(25);
    KK = KK + 1;
    NEWLOC = (fixed31)(labs((long)TRAVEL[KK]) / 1000);
    if (NEWLOC == LL) goto L12;
    LL = NEWLOC;
    goto L11;

L13:
    if (NEWLOC <= 100) goto L14;
    if (TOTING(K) || (NEWLOC > 200 && AT(K))) goto L16;
    goto L12;

L14:
    if (NEWLOC != 0 && !PCT(NEWLOC)) goto L12;
L16:
    NEWLOC = LL % 1000;
    if (NEWLOC <= 300) goto L2;
    if (NEWLOC <= 500) goto L30000;
    RSPEAK(NEWLOC - 500);
    NEWLOC = LOC;
    goto L2;

/* SPECIAL MOTIONS COME HERE. */

L30000:
    NEWLOC = NEWLOC - 300;
    switch (NEWLOC - 1) {
    case 0: goto L30100;
    case 1: goto L30200;
    case 2: goto L30300;
    default: BUG(20);
    }

/* TRAVEL 301.  PLOVER-ALCOVE PASSAGE. */

L30100:
    NEWLOC = 99 + 100 - LOC;
    if (HOLDNG == 0 || (HOLDNG == 1 && TOTING(EMRALD))) goto L2;
    NEWLOC = LOC;
    RSPEAK(117);
    goto L2;

/* TRAVEL 302.  PLOVER TRANSPORT. */

L30200:
    DROP(EMRALD, LOC);
    goto L12;

/* TRAVEL 303.  TROLL BRIDGE. */

L30300:
    if (PROP[TROLL] != 1) goto L30310;
    PSPEAK(TROLL, 1);
    PROP[TROLL] = 0;
    MOVE_(TROLL2, 0);
    MOVE_(TROLL2 + 100, 0);
    MOVE_(TROLL, PLAC[TROLL]);
    MOVE_(TROLL + 100, FIXD[TROLL]);
    JUGGLE(CHASM);
    NEWLOC = LOC;
    goto L2;

L30310:
    NEWLOC = PLAC[TROLL] + FIXD[TROLL] - LOC;
    if (PROP[TROLL] == 0) PROP[TROLL] = 1;
    if (!TOTING(BEAR)) goto L2;
    RSPEAK(162);
    PROP[CHASM] = 1;
    PROP[TROLL] = 2;
    DROP(BEAR, NEWLOC);
    FIXED[BEAR] = -1;
    PROP[BEAR] = 3;
    if (PROP[SPICES] < 0) TALLY2 = TALLY2 + 1;
    OLDLC2 = NEWLOC;
    goto L99;

/* END OF SPECIALS. */

/* HANDLE "GO BACK". */

L20:
    K = OLDLOC;
    if (FORCED(K)) K = OLDLC2;
    OLDLC2 = OLDLOC;
    OLDLOC = LOC;
    K2 = 0;
    if (K != LOC) goto L21;
    RSPEAK(91);
    goto L2;

L21:
    LL = (fixed31)((labs((long)TRAVEL[KK]) / 1000) % 1000);
    if (LL == K) goto L25;
    if (LL > 300) goto L22;
    J = KEY[LL];
    if (FORCED(LL) && (labs((long)TRAVEL[J]) / 1000) % 1000 == (unsigned long)K)
        K2 = KK;
L22:
    if (TRAVEL[KK] < 0) goto L23;
    KK = KK + 1;
    goto L21;

L23:
    KK = K2;
    if (KK != 0) goto L25;
    RSPEAK(140);
    goto L2;

L25:
    K = (fixed31)(labs((long)TRAVEL[KK]) % 1000);
    KK = KEY[LOC];
    goto L9;

/* LOOK. */

L30:
    CLRSCRN();
    if (DETAIL < 3) RSPEAK(15);
    DETAIL = DETAIL + 1;
L31:
    WZDARK = OFF;
    ABB[LOC] = 0;
    goto L2;

/* CAVE. */

L40:
    if (LOC < 8)  RSPEAK(57);
    if (LOC >= 8) RSPEAK(58);
    goto L2;

/* NON-APPLICABLE MOTION. */

L50:
    SPK = 12;
    if (K >= 43 && K <= 50 && LOC <= 143) SPK = 9;
    if (K == 29 || K == 30) SPK = 9;
    if (K == 7 || K == 36 || K == 37) SPK = 10;
    if (K == 11 || K == 19) SPK = 11;
    if (VERB == FIND || VERB == INVENT) SPK = 59;
    if (K == 62 || K == 65 || K == 78) SPK = 42;
    if (K == 17) SPK = 80;
    RSPEAK(SPK);
    goto L2;

/* "YOU'RE DEAD, JIM." */

L90:
    RSPEAK(23);
    OLDLC2 = LOC;

/* OKAY, HE'S DEAD.  LET'S GET ON WITH IT. */

L99:
    if (CLOSNG) goto L95;
    YEA = YES(81 + NUMDIE * 2, 82 + NUMDIE * 2, 54);
    NUMDIE = NUMDIE + 1;
    if (NUMDIE == MAXDIE || !YEA) goto L20000;
    PLACE[WATER] = 0;
    PLACE[OIL] = 0;
    if (TOTING(LAMP)) PROP[LAMP] = 0;
    for (J = 1; J <= 100; J++) {
        I = 101 - J;
        if (!TOTING(I)) continue;
        K = OLDLC2;
        if (I == LAMP) K = 1;
        DROP(I, K);
    }
    LOC = 3;
    OLDLOC = LOC;
    goto L2000;

/* HE DIED DURING CLOSING TIME.  NO RESURRECTION. */

L95:
    RSPEAK(131);
    NUMDIE = NUMDIE + 1;
    goto L20000;

/* RANDOM INTRANSITIVE VERBS COME HERE. */

L8000:
    A5TOA1(WD1, WD1X, TKWORD, &K);
    LINESKP();
    {
        char buf[133];
        int p = 0, q;
        for (q = 1; q <= K && p < 126; q++) buf[p++] = TKWORD[q][0];
        memcpy(buf + p, " what?", 6); p += 6;
        pl_vassign(&OUTSTR, buf, p);
    }
    LINEOUT();
    OBJ = 0;
    goto L2600;

/* CARRY, NO OBJECT GIVEN YET.  OK IF ONLY ONE OBJECT PRESENT. */

L8010:
    if (ATLOC[LOC] == 0 || LINK[ATLOC[LOC]] != 0) goto L8000;
    for (I = 1; I <= 5; I++)
        if (DLOC[I] == LOC && DFLAG >= 2) goto L8000;
    OBJ = ATLOC[LOC];

/* CARRY AN OBJECT. */

L9010:
    if (TOTING(OBJ)) goto L2011;
    SPK = 25;
    if (OBJ == RUBY && !DARK(0)) {
        if (HERE(LADDER) && PROP[LADDER] == 0) goto L8331;
        RSPEAK(202);
        goto L99;
    }
    if (OBJ == PLANT && PROP[PLANT] <= 0) SPK = 115;
    if (OBJ == BEAR && PROP[BEAR] == 1) SPK = 169;
    if (OBJ == CHAIN && PROP[BEAR] != 0) SPK = 170;
    if (FIXED[OBJ] != 0) goto L2011;
    if (OBJ != WATER && OBJ != OIL) goto L9017;
    if (HERE(BOTTLE) && LIQ(0) == OBJ) goto L9018;
    OBJ = BOTTLE;
    if (TOTING(BOTTLE) && PROP[BOTTLE] == 1) goto L9220;
    if (PROP[BOTTLE] != 1) SPK = 105;
    if (!TOTING(BOTTLE)) SPK = 104;
    goto L2011;
L9018:
    OBJ = BOTTLE;
L9017:
    if (HOLDNG < 7) goto L9016;
    RSPEAK(92);
    goto L2012;
L9016:
    if (OBJ != BIRD) goto L9014;
    if (PROP[BIRD] != 0) goto L9014;
    if (!TOTING(ROD)) goto L9013;
    RSPEAK(26);
    goto L2012;
L9013:
    if (TOTING(CAGE)) goto L9015;
    RSPEAK(27);
    goto L2012;
L9015:
    PROP[BIRD] = 1;
L9014:
    if ((OBJ == BIRD || OBJ == CAGE) && PROP[BIRD] != 0)
        CARRY(BIRD + CAGE - OBJ, LOC);
    if (OBJ == BRACELET && LOC == 144) PROP[BRACELET] = 0;
    CARRY(OBJ, LOC);
    K = LIQ(0);
    if (OBJ == BOTTLE && K != 0) PLACE[K] = -1;
    goto L2009;

/* DISCARD OBJECT. */

L9020:
    if (TOTING(ROD2) && OBJ == ROD && !TOTING(ROD)) OBJ = ROD2;
    if (!TOTING(OBJ)) goto L2011;
    if (OBJ != BIRD || !HERE(SNAKE)) goto L9024;
    RSPEAK(30);
    if (CLOSED) goto L19000;
    DSTROY(SNAKE);

/* SET PROP FOR USE BY TRAVEL OPTIONS */

    PROP[SNAKE] = 1;
L9021:
    K = LIQ(0);
    if (K == OBJ) OBJ = BOTTLE;
    if (OBJ == BOTTLE && K != 0) PLACE[K] = 0;
    if (OBJ == CAGE && PROP[BIRD] != 0) DROP(BIRD, LOC);
    if (OBJ == BIRD) PROP[BIRD] = 0;
    DROP(OBJ, LOC);
    goto L2012;

L9024:
    if (OBJ != COINS || !HERE(VEND)) goto L9025;
    DSTROY(COINS);
    DROP(BATTER, LOC);
    PSPEAK(BATTER, 0);
    goto L2012;

L9025:
    if (OBJ != BIRD || !AT(DRAGON) || PROP[DRAGON] != 0) goto L9026;
    RSPEAK(154);
    DSTROY(BIRD);
    PROP[BIRD] = 0;
    if (PLACE[SNAKE] == PLAC[SNAKE]) TALLY2 = TALLY2 + 1;
    goto L2012;

L9026:
    if (OBJ != BEAR || !AT(TROLL)) goto L9027;
    RSPEAK(163);
    MOVE_(TROLL, 0);
    MOVE_(TROLL + 100, 0);
    MOVE_(TROLL2, PLAC[TROLL]);
    MOVE_(TROLL2 + 100, FIXD[TROLL]);
    JUGGLE(CHASM);
    PROP[TROLL] = 2;
    goto L9021;

L9027:
    if (OBJ == VASE && LOC != PLAC[PILLOW]) goto L9028;
    if (OBJ == BRACELET && LOC == 144) PROP[BRACELET] = 1;
    RSPEAK(54);
    goto L9021;

L9028:
    PROP[VASE] = 2;
    if (AT(PILLOW)) PROP[VASE] = 0;
    PSPEAK(VASE, PROP[VASE] + 1);
    if (PROP[VASE] != 0) FIXED[VASE] = -1;
    goto L9021;

/* SAY.  ECHO WD2 (OR WD1 IF NO WD2).  MAGIC WORDS OVERRIDE. */

L9030:
    A5TOA1(WD2, WD2X, TKWORD, &K);
    if (pl_eq(WD2, 5, LIT("     "))) A5TOA1(WD1, WD1X, TKWORD, &K);
    if (!pl_eq(WD2, 5, LIT("     "))) pl_assign(WD1, 5, WD2, 5);
    I = VOCAB(WD1, -1);
    if (I == 62 || I == 65 || I == 71 || I == 2025) goto L9035;
    if (I == 1066 || (I == 2036 && HERE(ORAC))) goto L9035;
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(221);                                     /* OKAY, ".. */
    {
        char buf[133];
        int p, q;
        pl_edit_a(buf, 7, OUTSTR.s, OUTSTR.len);  p = 7;
        for (q = 1; q <= K && p < 128; q++) buf[p++] = TKWORD[q][0];
        memcpy(buf + p, "\".", 2); p += 2;
        pl_vassign(&OUTSTR, buf, p);
    }
    INHIB[0] = 'N';
    LINEOUT();
    goto L2012;

L9035:
    pl_assign(WD2, 5, LIT("     "));
    OBJ = 0;
    if (I == 1066) WD1[4] = '?';                        /* ORAC? */
    goto L2630;

/* LOCK, UNLOCK, NO OBJECT GIVEN. */

L8040:
    SPK = 28;
    if (HERE(CLAM)) OBJ = CLAM;
    if (HERE(OYSTER)) OBJ = OYSTER;
    if (AT(DOOR)) OBJ = DOOR;
    if (AT(GRATE)) OBJ = GRATE;
    if (OBJ != 0 && HERE(CHAIN)) goto L8000;
    if (HERE(CHAIN)) OBJ = CHAIN;
    if (OBJ == 0) goto L2011;

/* LOCK, UNLOCK OBJECT. */

L9040:
    if (OBJ == CLAM || OBJ == OYSTER) goto L9046;
    if (OBJ == DOOR) SPK = 111;
    if (OBJ == DOOR && PROP[DOOR] == 1) SPK = 54;
    if (OBJ == CAGE) SPK = 32;
    if (OBJ == KEYS) SPK = 55;
    if (OBJ == GRATE || OBJ == CHAIN) SPK = 31;
    if (SPK != 31 || !HERE(KEYS)) goto L2011;
    if (OBJ == CHAIN) goto L9048;
    if (!CLOSNG) goto L9043;
    K = 130;
    if (!PANIC) CLOCK2 = 15;
    PANIC = ON;
    goto L2010;

L9043:
    K = 34 + PROP[GRATE];
    PROP[GRATE] = 1;
    if (VERB == LOCK) PROP[GRATE] = 0;
    K = K + 2 * PROP[GRATE];
    goto L2010;

/* CLAM/OYSTER. */

L9046:
    K = 0;
    if (OBJ == OYSTER) K = 1;
    SPK = 124 + K;
    if (TOTING(OBJ)) SPK = 120 + K;
    if (!TOTING(TRIDNT)) SPK = 122 + K;
    if (VERB == LOCK) SPK = 61;
    if (SPK != 124) goto L2011;
    DSTROY(CLAM);
    DROP(OYSTER, LOC);
    DROP(PEARL, 105);
    goto L2011;

/* CHAIN. */

L9048:
    if (VERB == LOCK) goto L9049;
    SPK = 171;
    if (PROP[BEAR] == 0) SPK = 41;
    if (PROP[CHAIN] == 0) SPK = 37;
    if (SPK != 171) goto L2011;
    PROP[CHAIN] = 0;
    FIXED[CHAIN] = 0;
    if (PROP[BEAR] != 3) PROP[BEAR] = 2;
    FIXED[BEAR] = 2 - PROP[BEAR];
    goto L2011;

L9049:
    SPK = 172;
    if (PROP[CHAIN] != 0) SPK = 34;
    if (LOC != PLAC[CHAIN]) SPK = 173;
    if (SPK != 172) goto L2011;
    PROP[CHAIN] = 2;
    if (TOTING(CHAIN)) DROP(CHAIN, LOC);
    FIXED[CHAIN] = -1;
    goto L2011;

/* LIGHT LAMP */

L9070:
    if (!HERE(LAMP)) goto L2011;
    SPK = 184;
    if (LIMIT < 0) goto L2011;
    PROP[LAMP] = 1;
    RSPEAK(39);
    if (WZDARK) goto L2000;
    goto L2012;

/* LAMP OFF */

L9080:
    if (!HERE(LAMP)) goto L2011;
    PROP[LAMP] = 0;
    RSPEAK(40);
    if (DARK(0)) RSPEAK(16);
    goto L2012;

/* WAVE.  NO EFFECT UNLESS WAVING ROD AT FISSURE. */

L9090:
    if ((!TOTING(OBJ)) && (OBJ != ROD || !TOTING(ROD2))) SPK = 29;
    if (OBJ != ROD || !AT(FISSUR) || !TOTING(OBJ) || CLOSNG) goto L2011;
    PROP[FISSUR] = 1 - PROP[FISSUR];
    PSPEAK(FISSUR, 2 - PROP[FISSUR]);
    goto L2012;

/* ATTACK. */

L9120:
    for (I = 1; I <= 5; I++)
        if (DLOC[I] == LOC && DFLAG >= 2) goto L9122;
    I = 0;
L9122:
    if (OBJ != 0) goto L9124;
    if (I != 0) OBJ = DWARF;
    if (HERE(SNAKE)) OBJ = OBJ * 100 + SNAKE;
    if (AT(DRAGON) && PROP[DRAGON] == 0) OBJ = OBJ * 100 + DRAGON;
    if (AT(TROLL)) OBJ = OBJ * 100 + TROLL;
    if (HERE(BEAR) && PROP[BEAR] == 0) OBJ = OBJ * 100 + BEAR;
    if (OBJ > 100) goto L8000;
    if (OBJ != 0) goto L9124;

/* CAN'T ATTACK BIRD BY THROWING AXE. */

    if (HERE(BIRD) && VERB != THROW) OBJ = BIRD;

/* CLAM AND OYSTER BOTH TREATED AS CLAM FOR INTRANSITIVE CASE. */

    if (HERE(CLAM) || HERE(OYSTER)) OBJ = 100 * OBJ + CLAM;
    if (OBJ > 100) goto L8000;
L9124:
    if (OBJ != BIRD) goto L9125;
    SPK = 137;
    if (CLOSED) goto L2011;
    DSTROY(BIRD);
    PROP[BIRD] = 0;
    if (PLACE[SNAKE] == PLAC[SNAKE]) TALLY2 = TALLY2 + 1;
    SPK = 45;
L9125:
    if (OBJ == 0) SPK = 44;
    if (OBJ == CLAM || OBJ == OYSTER) SPK = 150;
    if (OBJ == SNAKE) SPK = 46;
    if (OBJ == DWARF) SPK = 49;
    if (OBJ == DWARF && CLOSED) goto L19000;
    if (OBJ == DRAGON) SPK = 167;
    if (OBJ == TROLL) SPK = 157;
    if (OBJ == BEAR) SPK = 165 + (PROP[BEAR] + 1) / 2;
    if (OBJ != DRAGON || PROP[DRAGON] != 0) goto L2011;

/* FUN STUFF FOR DRAGON. */

    RSPEAK(49);
    VERB = 0;
    OBJ = 0;
    GETIN(WD1, WD1X, WD2, WD2X);
    if (!pl_eq(WD1, 5, LIT("Y")) && !pl_eq(WD1, 5, LIT("YES"))) goto L2608;
    PSPEAK(DRAGON, 1);
    PROP[DRAGON] = 2;
    PROP[RUG] = 0;
    K = (PLAC[DRAGON] + FIXD[DRAGON]) / 2;
    MOVE_(DRAGON + 100, -1);
    MOVE_(RUG + 100, 0);
    MOVE_(DRAGON, K);
    MOVE_(RUG, K);
    for (OBJ = 1; OBJ <= 100; OBJ++)
        if (PLACE[OBJ] == PLAC[DRAGON] || PLACE[OBJ] == FIXD[DRAGON])
            MOVE_(OBJ, K);
    LOC = K;
    K = NULLX;
    goto L8;

/* POUR. */

L9130:
    if (OBJ == BOTTLE || OBJ == 0) OBJ = LIQ(0);
    if (OBJ == 0) goto L8000;
    if (!TOTING(OBJ)) goto L2011;
    SPK = 78;
    if (OBJ != OIL && OBJ != WATER) goto L2011;
    PROP[BOTTLE] = 1;
    PLACE[OBJ] = 0;
    SPK = 77;
    if (!(AT(PLANT) || AT(DOOR))) goto L2011;

    if (AT(DOOR)) goto L9132;
    SPK = 112;
    if (OBJ != WATER) goto L2011;
    PSPEAK(PLANT, PROP[PLANT] + 1);
    PROP[PLANT] = (PROP[PLANT] + 2) % 6;
    PROP[PLANT2] = PROP[PLANT] / 2;
    K = NULLX;
    goto L8;

L9132:
    PROP[DOOR] = 0;
    if (OBJ == OIL) PROP[DOOR] = 1;
    SPK = 113 + PROP[DOOR];
    goto L2011;

/* EAT. */

L8140:
    if (!HERE(FOOD)) goto L8000;
L8142:
    DSTROY(FOOD);
    SPK = 72;
    goto L2011;

L9140:
    if (OBJ == FOOD) goto L8142;
    if (OBJ == BIRD || OBJ == SNAKE || OBJ == CLAM || OBJ == OYSTER
        || OBJ == DWARF || OBJ == DRAGON || OBJ == TROLL
        || OBJ == BEAR) SPK = 71;
    goto L2011;

/* DRINK. */

L9150:
    if (OBJ == 0 && LIQLOC(LOC) != WATER
        && (LIQ(0) != WATER || !HERE(BOTTLE))) goto L8000;
    if (OBJ != 0 && OBJ != WATER) SPK = 110;
    if (SPK == 110 || LIQ(0) != WATER || !HERE(BOTTLE)) goto L2011;
    PROP[BOTTLE] = 1;
    PLACE[WATER] = 0;
    SPK = 74;
    goto L2011;

/* RUB.  YIELDS VARIOUS SNIDE REMARKS. */

L9160:
    if (OBJ != LAMP) SPK = 76;
    goto L2011;

/* THROW. */

L9170:
    if (TOTING(ROD2) && OBJ == ROD && !TOTING(ROD)) OBJ = ROD2;
    if (!TOTING(OBJ)) goto L2011;
    if (OBJ >= 50 && OBJ <= MAXTRS && AT(TROLL)) goto L9178;
    if (OBJ == FOOD && HERE(BEAR)) goto L9177;
    if (OBJ != AXE) goto L9020;
    for (I = 1; I <= 5; I++) {
        /* NEEDN'T CHECK DFLAG IF AXE IS HERE. */
        if (DLOC[I] == LOC) goto L9172;
    }
    SPK = 152;
    if (AT(DRAGON) && PROP[DRAGON] == 0) goto L9175;
    SPK = 158;
    if (AT(TROLL)) goto L9175;
    if (HERE(BEAR) && PROP[BEAR] == 0) goto L9176;
    OBJ = 0;
    goto L9120;

L9172:
    SPK = 48;
    if (RAN(3) == 0) goto L9175;
    DSEEN[I] = OFF;
    DLOC[I] = 0;
    SPK = 47;
    DKILL = DKILL + 1;
    if (DKILL == 1) SPK = 149;
L9175:
    RSPEAK(SPK);
    DROP(AXE, LOC);
    K = NULLX;
    goto L8;

/* THIS'LL TEACH HIM TO THROW THE AXE AT THE BEAR! */

L9176:
    SPK = 164;
    DROP(AXE, LOC);
    FIXED[AXE] = -1;
    PROP[AXE] = 1;
    JUGGLE(BEAR);
    goto L2011;

/* BUT THROWING FOOD IS ANOTHER STORY. */

L9177:
    OBJ = BEAR;
    goto L9210;

L9178:
    SPK = 159;

/* SNARF A TREASURE FOR THE TROLL. */

    DROP(OBJ, 0);
    MOVE_(TROLL, 0);
    MOVE_(TROLL + 100, 0);
    DROP(TROLL2, PLAC[TROLL]);
    DROP(TROLL2 + 100, FIXD[TROLL]);
    JUGGLE(CHASM);
    goto L2011;

/* QUIT. */

L8180:
    CLRSCRN();
    GAVEUP = YES(22, 54, 54);
L8185:
    if (GAVEUP) goto L20000;
    goto L2012;

/* FIND. */

L9190:
    if (AT(OBJ) || (LIQ(0) == OBJ && AT(BOTTLE))
        || K == LIQLOC(LOC)) SPK = 94;
    for (I = 1; I <= 5; I++)
        if (DLOC[I] == LOC && DFLAG >= 2 && OBJ == DWARF) SPK = 94;
    if (CLOSED) SPK = 138;
    if (TOTING(OBJ)) SPK = 24;
    goto L2011;

/* INVENTORY. */

L8200:
    SPK = 98;
    for (I = 1; I <= 100; I++) {
        if (I == BEAR || !TOTING(I)) continue;
        if (SPK == 98) RSPEAK(99);
        BLKLIN = OFF;
        PSPEAK(I, -1);
        BLKLIN = ON;
        SPK = 0;
    }
    if (TOTING(BEAR)) SPK = 141;
    goto L2011;

/* FEED. */

L9210:
    if (OBJ != BIRD) goto L9212;
    SPK = 100;
    goto L2011;

L9212:
    if (OBJ != SNAKE && OBJ != DRAGON && OBJ != TROLL) goto L9213;
    SPK = 102;
    if (OBJ == DRAGON && PROP[DRAGON] != 0) SPK = 110;
    if (OBJ == TROLL) SPK = 182;
    if (OBJ != SNAKE || CLOSED || !HERE(BIRD)) goto L2011;
    SPK = 101;
    DSTROY(BIRD);
    PROP[BIRD] = 0;
    TALLY2 = TALLY2 + 1;
    goto L2011;

L9213:
    if (OBJ != DWARF) goto L9214;
    if (!HERE(FOOD)) goto L2011;
    SPK = 103;
    DFLAG = DFLAG + 1;
    goto L2011;

L9214:
    if (OBJ != BEAR) goto L9215;
    if (PROP[BEAR] == 0) SPK = 102;
    if (PROP[BEAR] == 3) SPK = 110;
    if (!HERE(FOOD)) goto L2011;
    DSTROY(FOOD);
    PROP[BEAR] = 1;
    FIXED[AXE] = 0;
    PROP[AXE] = 0;
    SPK = 168;
    goto L2011;

L9215:
    SPK = 14;
    goto L2011;

/* FILL.  BOTTLE MUST BE EMPTY, AND SOME LIQUID AVAILABLE. */

L9220:
    if (OBJ == VASE) goto L9222;
    if (OBJ != 0 && OBJ != BOTTLE) goto L2011;
    if (OBJ == 0 && !HERE(BOTTLE)) goto L8000;
    SPK = 107;
    if (LIQLOC(LOC) == 0) SPK = 106;
    if (LIQ(0) != 0) SPK = 105;
    if (SPK != 107) goto L2011;
    PROP[BOTTLE] = (COND[LOC] % 4) / 2;
    PROP[BOTTLE] = PROP[BOTTLE] * 2;
    K = LIQ(0);
    if (TOTING(BOTTLE)) PLACE[K] = -1;
    if (K == OIL) SPK = 108;
    goto L2011;

L9222:
    SPK = 29;
    if (LIQLOC(LOC) == 0) SPK = 144;
    if (LIQLOC(LOC) == 0 || !TOTING(VASE)) goto L2011;
    RSPEAK(145);
    PROP[VASE] = 2;
    FIXED[VASE] = -1;
    goto L9024;

/* BLAST. */

L9230:
    if (PROP[ROD2] < 0 || !CLOSED) goto L2011;
    BONUS = 133;
    if (LOC == 115) BONUS = 134;
    if (HERE(ROD2)) BONUS = 135;
    RSPEAK(BONUS);
    goto L20000;

/* SCORE. */

L8240:
    SCORNG = ON;
    goto L20000;

L8241:
    SCORNG = OFF;
    CLRSCRN();
    INHIB[0] = 'Y';
    RSPEAK(213);                  /* IF YOU WERE TO QUIT NOW ... */
    pl_edit_f(INSTR, 4, SCORE);
    pl_edit_f(INSTR + 4, 4, MXSCOR);
    pl_vsubstr_assign(&OUTSTR, 41, 4, INSTR, 4);
    pl_vsubstr_assign(&OUTSTR, 63, 4, INSTR + 4, 4);
    INHIB[0] = 'N';
    LINEOUT();
    GAVEUP = YES(143, 54, 54);
    goto L8185;

/* FEE FIE FOE FOO (AND FUM). */

L8250:
    K = VOCAB(WD1, 3);
    SPK = 42;
    if (FOOBAR == 1 - K) goto L8252;
    if (FOOBAR != 0) SPK = 151;
    goto L2011;

L8252:
    FOOBAR = K;
    if (K != 4) goto L2009;
    FOOBAR = 0;
    if (PLACE[EGGS] == PLAC[EGGS]
        || (TOTING(EGGS) && LOC == PLAC[EGGS])) goto L2011;

/* BRING BACK TROLL IF WE STEAL THE EGGS BACK FROM HIM. */

    if (PLACE[EGGS] == 0 && PLACE[TROLL] == 0 && PROP[TROLL] == 0)
        PROP[TROLL] = 1;
    K = 2;
    if (HERE(EGGS)) K = 1;
    if (LOC == PLAC[EGGS]) K = 0;
    MOVE_(EGGS, PLAC[EGGS]);
    PSPEAK(EGGS, K);
    goto L2012;

/* BRIEF. */

L8260:
    SPK = 156;
    ABBNUM = 10000;
    DETAIL = 3;
    goto L2011;

/* READ. */

L8270:
    if (HERE(MAGZIN)) OBJ = MAGZIN;
    if (HERE(TABLET)) OBJ = OBJ * 100 + TABLET;
    if (HERE(MESSAG)) OBJ = OBJ * 100 + MESSAG;
    if (CLOSED && TOTING(OYSTER)) OBJ = OYSTER;
    if (OBJ > 100 || OBJ == 0 || DARK(0)) goto L8000;

L9270:
    if (DARK(0)) goto L5190;
    if (OBJ == MAGZIN) SPK = 190;
    if (OBJ == TABLET) SPK = 196;
    if (OBJ == MESSAG) SPK = 191;
    if (OBJ == OYSTER && HINTED[2] && TOTING(OYSTER)) SPK = 194;
    if (OBJ != OYSTER || HINTED[2] || !TOTING(OYSTER) || !CLOSED)
        goto L2011;
    HINTED[2] = YES(192, 193, 54);
    goto L2012;

/* BREAK. */

L9280:
    if (OBJ == MIRROR) SPK = 148;
    if (OBJ == VASE && PROP[VASE] == 0) goto L9282;
    if (OBJ != MIRROR || !CLOSED) goto L2011;
    RSPEAK(197);
    goto L19000;

L9282:
    SPK = 198;
    if (TOTING(VASE)) DROP(VASE, LOC);
    PROP[VASE] = 2;
    FIXED[VASE] = -1;
    goto L2011;

/* WAKE.  ONLY USE IS TO DISTURB THE DWARFS. */

L9290:
    if (OBJ != DWARF || !CLOSED) goto L2011;
    RSPEAK(199);
    goto L19000;

/* SUSPEND. */

L8300:
    if (DEMOGM) {
        RSPEAK(201);
    } else {
        if (SUSPEND_N == 10) {     /* LIMIT OF TEN SUSPENDS PER GAME */
            MSPEAK(40);
            goto L2012;
        }
        CIAO();
        MSPEAK(41);                      /* NO ROOM FOR SUSPEND */
        LINESKP();
    }
    goto L2012;

/* HOURS.  REPORT CURRENT NON-PRIME-TIME HOURS. */

L8310:
    MSPEAK(6);
    HOURFMT();
    goto L2012;

/* LOG.  TOGGLE LOGGING EITHER ON OR OFF */

L8320:
    LOGON = (bit1)!LOGON;
    if (LOGON) MSPEAK(42);                            /* LOG ON  */
    else       MSPEAK(43);                            /* LOG OFF */
    LINESKP();
    goto L2012;

L8330:
    if (!TOTING(RUBY)) goto L2011;
L8331:
    if (HERE(LADDER) && PROP[LADDER] == 0) {
        PROP[LADDER] = 2;
        SPK = 225;
    }
    if (HERE(FFIELD) && PROP[FFIELD] == 0) {
        PROP[FFIELD] = 1;
        SPK = 226;
    }
    RSPEAK(SPK);
    goto L2000;

L8340:
    PUTBACK();
    goto L31;

L8350:
    if (TTYPE[0] != 'V') goto L2011;
    WIZPROC();
    goto L2012;

L8360:
    if (!HERE(ORAC)) {         /* LET ORAC PROVIDE ASSISTANCE (IF   */
        SPK = 29;              /* PIRATE HASN'T STOLEN HIM)         */
        goto L2011;
    }
    INHIB[0] = 'Y';
    RSPEAK(228);
    INHIB[0] = 'N';
    switch (LOC) {
    case  41: pl_assign(JUNK2, 5, LIT("south")); break;
    case  42: pl_assign(JUNK2, 5, LIT("east!")); break;
    case  43: pl_assign(JUNK2, 5, LIT("south")); break;
    case  44: pl_assign(JUNK2, 5, LIT("south")); break;
    case  50: pl_assign(JUNK2, 5, LIT("south")); break;
    case  52: pl_assign(JUNK2, 5, LIT("north")); break;
    case  55: pl_assign(JUNK2, 5, LIT("east!")); break;
    case  57: pl_assign(JUNK2, 5, LIT("east!")); break;
    case 141: pl_assign(JUNK2, 5, LIT("TELEP")); break; /* BEAM ME UP */
    case 144: pl_assign(JUNK2, 5, LIT("TRICK")); break; /* GET ME OUT */
    default:  pl_assign(JUNK2, 5, LIT("     ")); break;
    }
    if (JUNK2[0] == 'T' && !TOTING(BRACELET))
        pl_assign(JUNK2, 5, LIT("     "));
    if (pl_eq(JUNK2, 5, LIT("TELEP"))) {
        LOC = 144;                            /* BACK TO TELEPORT ROOM */
        goto L2000;
    }
    if (pl_eq(JUNK2, 5, LIT("TRICK"))) {
        if (PROP[LADDER] == 2) LOC = 106;  /* ESCAPE IF LADDER MELTED */
        else                   LOC = 142;  /* ELSE JUST A TELEPORT    */
        goto L2000;
    }
    if (pl_eq(JUNK2, 5, LIT("     "))) {
        RSPEAK(227);
    } else {
        pl_vsubstr_assign(&OUTSTR, 55, 5, JUNK2, 5);
        LINEOUT();
    }
    goto L2012;

/* HINTS */

L40000:
    switch (HINT - 4) {
    case 0: goto L40400;        /* CAVE  */
    case 1: goto L40500;        /* BIRD  */
    case 2: goto L40600;        /* SNAKE */
    case 3: goto L40700;        /* MAZE  */
    case 4: goto L40800;        /* DARK  */
    case 5: goto L40900;        /* WITT  */
    case 6: goto L41000;        /* RUBY  */
    default: BUG(27);
    }

L40010:
    HINTLC[HINT] = 0;
    if (!YES(HINTS[HINT][3], 0, 54)) goto L2602;
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(214);
    pl_edit_f(KKWORD, 2, HINTS[HINT][2]);
    pl_vsubstr_assign(&OUTSTR, 56, 2, KKWORD, 2);
    INHIB[0] = 'N';
    LINEOUT();
    HINTED[HINT] = YES(175, HINTS[HINT][4], 54);
    if (HINTED[HINT] && LIMIT > 30)
        LIMIT = LIMIT + 30 * HINTS[HINT][2];
L40020:
    HINTLC[HINT] = 0;
L40030:
    goto L2602;

/* NOW FOR THE QUICK TESTS. */

L40400:
    if (PROP[GRATE] == 0 && !HERE(KEYS)) goto L40010;
    goto L40020;

L40500:
    if (HERE(BIRD) && TOTING(ROD) && OBJ == BIRD) goto L40010;
    goto L40030;

L40600:
    if (HERE(SNAKE) && !HERE(BIRD)) goto L40010;
    goto L40020;

L40700:
    if (ATLOC[LOC] == 0 && ATLOC[OLDLOC] == 0
        && ATLOC[OLDLC2] == 0 && HOLDNG > 1) goto L40010;
    goto L40020;

L40800:
    if (PROP[EMRALD] != -1 && PROP[PYRAM] == -1) goto L40010;
    goto L40020;

L40900:
    goto L40010;

L41000:
    if (HERE(RUBY) && !TOTING(RUBY)) goto L40010;
    goto L40020;

/* CAVE CLOSING AND SCORING */

L10000:
    PROP[GRATE] = 0;
    PROP[FISSUR] = 0;
    for (I = 1; I <= 6; I++)
        DSEEN[I] = OFF;
    MOVE_(TROLL, 0);
    MOVE_(TROLL + 100, 0);
    MOVE_(TROLL2, PLAC[TROLL]);
    MOVE_(TROLL2 + 100, FIXD[TROLL]);
    JUGGLE(CHASM);
    if (PROP[BEAR] != 3) DSTROY(BEAR);
    PROP[CHAIN] = 0;
    FIXED[CHAIN] = 0;
    PROP[AXE] = 0;
    FIXED[AXE] = 0;
    RSPEAK(129);
    CLOCK1 = -1;
    CLOSNG = ON;
    goto L19999;

/* SET UP THE STORAGE ROOM. */

L11000:
    PROP[BOTTLE] = PUT_(BOTTLE, 115, 1);
    PROP[PLANT]  = PUT_(PLANT, 115, 0);
    PROP[OYSTER] = PUT_(OYSTER, 115, 0);
    PROP[LAMP]   = PUT_(LAMP, 115, 0);
    PROP[ROD]    = PUT_(ROD, 115, 0);
    PROP[DWARF]  = PUT_(DWARF, 115, 0);
    LOC = 115;
    OLDLOC = 115;
    NEWLOC = 115;

/* LEAVE THE GRATE WITH NORMAL (NON-NEGATIVE PROPERTY). */

    FOO = PUT_(GRATE, 116, 0);
    PROP[SNAKE]  = PUT_(SNAKE, 116, 1);
    PROP[BIRD]   = PUT_(BIRD, 116, 1);
    PROP[CAGE]   = PUT_(CAGE, 116, 0);
    PROP[ROD2]   = PUT_(ROD2, 116, 0);
    PROP[PILLOW] = PUT_(PILLOW, 116, 0);

    PROP[MIRROR] = PUT_(MIRROR, 115, 0);
    FIXED[MIRROR] = 116;

    for (I = 1; I <= 100; I++)
        if (TOTING(I)) DSTROY(I);

    RSPEAK(132);
    CLOSED = ON;
    goto L2;

/* THE LAMP GIVING OUT. */

L12000:
    RSPEAK(188);
    PROP[BATTER] = 1;
    if (TOTING(BATTER)) DROP(BATTER, LOC);
    LIMIT = LIMIT + 2500;
    LMWARN = OFF;
    goto L19999;

L12200:
    if (LMWARN || !HERE(LAMP)) goto L19999;
    LMWARN = ON;
    SPK = 187;
    if (PLACE[BATTER] == 0) SPK = 183;
    if (PROP[BATTER] == 1) SPK = 189;
    RSPEAK(SPK);
    goto L19999;

L12400:
    LIMIT = -1;
    PROP[LAMP] = 0;
    if (HERE(LAMP)) RSPEAK(184);
    goto L19999;

L12600:
    RSPEAK(185);
    GAVEUP = ON;
    goto L20000;

/* OH DEAR, HE'S DISTURBED THE DWARFS. */

L19000:
    RSPEAK(136);

/* EXIT CODE / SCORING. */

L20000:
    SCORE = 0;
    MXSCOR = 0;

/* FIRST TALLY UP THE TREASURES. */

    for (I = 50; I <= MAXTRS; I++) {
        if (PTEXT[I] != 0) {
            K = 12;
            if (I == CHEST) K = 14;
            if (I > CHEST) K = 16;
            if (PROP[I] >= 0) SCORE = SCORE + 2;
            if (PLACE[I] == 3 && PROP[I] == 0) SCORE = SCORE + K - 2;
            MXSCOR = MXSCOR + K;
        }
    }

/* NOW LOOK AT HOW HE FINISHED AND HOW FAR HE GOT. */

    SCORE = SCORE + (MAXDIE - NUMDIE) * 10;
    MXSCOR = MXSCOR + MAXDIE * 10;
    if (!(SCORNG || GAVEUP)) SCORE = SCORE + 4;
    MXSCOR = MXSCOR + 4;
    if (DFLAG != 0) SCORE = SCORE + 25;
    MXSCOR = MXSCOR + 25;
    if (CLOSNG) SCORE = SCORE + 25;
    MXSCOR = MXSCOR + 25;
    if (!CLOSED) goto L20020;
    if (BONUS == 0) SCORE = SCORE + 10;
    if (BONUS == 135) SCORE = SCORE + 25;
    if (BONUS == 134) SCORE = SCORE + 30;
    if (BONUS == 133) SCORE = SCORE + 45;
L20020:
    MXSCOR = MXSCOR + 45;

/* DID HE COME TO WITT'S END AS HE SHOULD? */

    if (PLACE[MAGZIN] == 108) SCORE = SCORE + 1;
    MXSCOR = MXSCOR + 1;

/* ROUND IT OFF. */

    SCORE = SCORE + 2;
    MXSCOR = MXSCOR + 2;

/* DEDUCT POINTS FOR HINTS. */

    for (I = 1; I <= HNTMAX; I++)
        if (HINTED[I]) SCORE = SCORE - HINTS[I][2];

/* RETURN TO SCORE COMMAND IF THAT'S WHERE WE CAME FROM. */

    if (SCORNG) goto L8241;

/* THAT SHOULD BE GOOD ENOUGH.  LET'S TELL HIM ALL ABOUT IT. */

    LINESKP();
    LINESKP();
    LINESKP();
    M = PIC_HH * 60 + PIC_MM;
    {                              /* TIME_CHR = TIME(): writes through
                                    * the overlay into DATE_STG(9:17) */
        char t9[9];
        PLI_TIME(t9);
        TIME_CHR_SET(t9);
    }
    ELAPSED = PIC_HH * 60 + PIC_MM + ELAPSED - M;
    INHIB[0] = 'Y';
    RSPEAK(215);                                 /* YOU SCORED ... */
    pl_edit_f(INSTR,      4, SCORE);
    pl_edit_f(INSTR +  4, 4, MXSCOR);
    pl_edit_f(INSTR +  8, 4, TURNS);
    pl_edit_f(INSTR + 12, 4, ELAPSED);
    pl_vsubstr_assign(&OUTSTR, 11, 4, INSTR,      4);
    pl_vsubstr_assign(&OUTSTR, 33, 4, INSTR +  4, 4);
    pl_vsubstr_assign(&OUTSTR, 44, 4, INSTR +  8, 4);
    pl_vsubstr_assign(&OUTSTR, 57, 4, INSTR + 12, 4);
    INHIB[0] = 'N';
    LINEOUT();

    for (I = 1; I <= CLSSES; I++)
        if (CVAL[I] >= SCORE) goto L20210;
    LINESKP();
    RSPEAK(216);                     /* YOU JUST WENT OFF MY SCALE! */
    goto L25000;

L20210:
    SPEAK(CTEXT[I]);
    if (I == CLSSES - 1) goto L20220;
    K = CVAL[I] + 1 - SCORE;
    pl_assign(KKWORD, 2, LIT("s."));
    if (K == 1) pl_assign(KKWORD, 2, LIT(". "));
    LINESKP();
    INHIB[0] = 'Y';
    RSPEAK(217);          /* FOR NEXT HIGHER RATING YOU NEED ... */
    pl_edit_f(JUNK1, 3, K);
    pl_vsubstr_assign(&OUTSTR, 44, 3, JUNK1, 3);
    pl_vsubstr_assign(&OUTSTR, 58, 2, KKWORD, 2);
    INHIB[0] = 'N';
    LINEOUT();
    goto L25000;

L20220:
    LINESKP();
    RSPEAK(218);
    /* TO ACHIEVE THE NEXT HIGHER RATING WOULD BE A NEAT TRICK! */
    LINESKP();
    RSPEAK(219);                              /* CONGRATULATIONS!! */

L25000:
    LINESKP();
    goto DEALLOC;

DEALLOC:                              /* DEALLOCATE FILES AND STOP. */
    R062A10();                        /* DDNAME = 'STORAGE ' */
    R062A10();                        /* DDNAME = 'OBJECT  ' */
    plat_exit(0);
}

int main(int argc, char **argv)
{
    int i, n = 0;
    statics_init();
    advars_init();

    INPARM.len = 0;
    for (i = 1; i < argc; i++) {
        int l = (int)strlen(argv[i]);
        if (n + l > (int)sizeof INPARM.s) l = (int)sizeof INPARM.s - n;
        if (l <= 0) break;
        memcpy(INPARM.s + n, argv[i], (size_t)l);
        n += l;
    }
    INPARM.len = n;

    setvbuf(stdout, NULL, _IOFBF, 8192);
    PROGRAM();
    return 0;
}
