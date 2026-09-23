/* advsubs.c -- the internal procedures of adventure.pli (lines
 * 3189-4699), transliterated in the same order.
 *
 *   TOTING(OBJ)  = TRUE IF THE OBJ IS BEING CARRIED
 *   HERE(OBJ)    = TRUE IF THE OBJ IS AT "LOC" (OR IS BEING CARRIED)
 *   AT(OBJ)      = TRUE IF ON EITHER SIDE OF TWO-PLACED OBJECT
 *   LIQ(DUMMY)   = OBJECT NUMBER OF LIQUID IN BOTTLE
 *   LIQLOC(LOC)  = OBJECT NUMBER OF LIQUID (IF ANY) AT LOC
 *   BITSET(L,N)  = TRUE IF COND(L) HAS BIT N SET (BIT 0 IS UNITS BIT)
 *   FORCED(LOC)  = TRUE IF LOC MOVES WITHOUT ASKING FOR INPUT (COND=2)
 *   DARK(DUMMY)  = TRUE IF LOCATION "LOC" IS DARK
 *   PCT(N)       = TRUE N% OF THE TIME (N INTEGER FROM 0 TO 100)
 */
#include "advent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bit1 TOTING(fixed31 OBJ)
{
    return (bit1)(PLACE[OBJ] == -1);
}

bit1 HERE(fixed31 OBJ)
{
    return (bit1)(PLACE[OBJ] == LOC || TOTING(OBJ));
}

bit1 AT(fixed31 OBJ)
{
    return (bit1)(PLACE[OBJ] == LOC || FIXED[OBJ] == LOC);
}

fixed31 LIQ(fixed31 DUMMY)
{
    fixed31 a = PROP[BOTTLE], b = -1 - PROP[BOTTLE];
    (void)DUMMY;
    return LIQ2(a > b ? a : b);
}

fixed31 LIQ2(fixed31 PBOTL)
{
    fixed31 LIQ2TEMP;

    LIQ2TEMP = PBOTL / 2;
    return (1 - PBOTL) * WATER + (LIQ2TEMP) * (WATER + OIL);
}

fixed31 LIQLOC(fixed31 LOC)
{
    fixed31 LIQTEMP1, LIQTEMP2;

    LIQTEMP1 = COND[LOC] / 2;
    LIQTEMP1 = LIQTEMP1 * 2;
    LIQTEMP2 = COND[LOC] / 4;
    return LIQ2((LIQTEMP1 % 8 - 5) * (LIQTEMP2 % 2) + 1);
}

bit1 BITSET(fixed31 L, fixed31 N)
{
    static fixed31 BITTEMP;

    BITTEMP = COND[L] / (fixed31)(1L << N);
    return (bit1)(BITTEMP % 2 != 0);
}

bit1 FORCED(fixed31 LOC)
{
    return (bit1)(COND[LOC] == 2);
}

bit1 DARK(fixed31 DUMMY)
{
    (void)DUMMY;
    return (bit1)(COND[LOC] % 2 == 0 && (PROP[LAMP] == 0 || !HERE(LAMP)));
}

bit1 PCT(fixed31 N)
{
    return (bit1)(RAN(100) < N);
}

/* I/O ROUTINES (SPEAK, PSPEAK, RSPEAK, MSPEAK, A5TOA1, GETIN, YES) */

void SPEAK(fixed31 N)
{
    fixed31 I, K, L;

/*  PRINT THE MESSAGE WHICH STARTS AT LINES(N).  PRECEDE IT WITH A
    BLANK LINE UNLESS BLKLIN IS FALSE.  */

    if (N == 0) return;
    if (pl_eq(LINES[N + 1], 5, LIT(">$<"))) return;
    if (BLKLIN) LINESKP();
    K = N;
L1:
    pl_assign(YDCHR, 5, LINES[K], 5);
    L = YDPIC % 50000 - 1;
    K = K + 1;
    {   /* PUT STRING (OUTSTR) EDIT ((LINES(I) DO I=K TO L)) (14 A(5)) */
        char buf[133];
        int p = 0;
        for (I = K; I <= L && p + 5 <= 133; I++) {
            memcpy(buf + p, LINES[I], 5);
            p += 5;
        }
        pl_vassign(&OUTSTR, buf, p);
    }
    if (INHIB[0] == 'Y') return;
    LINEOUT();
    K = L + 1;
    if (LINES[K][0] < '5') goto L1;
}

void PSPEAK(fixed31 MSG, fixed31 SKIP)
{
    fixed31 I, M;

/*  FIND THE SKIP+1ST MESSAGE FROM MSG AND PRINT IT.  */

    M = PTEXT[MSG];
    if (SKIP < 0) goto L9;
    for (I = 0; I <= SKIP; I++) {
L1:
        pl_assign(YDCHR, 5, LINES[M], 5);
        M = YDPIC % 50000;
        if (LINES[M][0] < '5') goto L1;
    }
L9:
    SPEAK(M);
}

void RSPEAK(fixed31 I)
{
    if (I != 0) SPEAK(RTEXT[I]);
}

void MSPEAK(fixed31 I)
{
    if (I != 0) SPEAK(MTEXT[I]);
}

void GETIN(char *WORD1, char *WORD1X, char *WORD2, char *WORD2X)
{
/*  GET A COMMAND FROM THE ADVENTURER.  SNARF OUT THE FIRST WORD, PAD
    IT WITH BLANKS, AND RETURN IT IN WORD1.  CHARS 6 THRU 10 ARE
    RETURNED IN WORD1X.  IF A SECOND WORD APPEARS, IT IS RETURNED IN
    WORD2 (CHARS 6 THRU 10 IN WORD2X), ELSE WORD2 IS SET TO ZERO.  */

    pl_assign(WORD1,  5, LIT("     "));
    pl_assign(WORD1X, 5, LIT("     "));
    pl_assign(WORD2,  5, LIT("     "));
    pl_assign(WORD2X, 5, LIT("     "));
    if (LOGON) SYSPRINT_skip();
    WORDSTRT = 0;
    while (WORDSTRT == 0) {
        if (BLKLIN) LINESKP();
        TREAD(INSTR, 0, INSTR, &INLEN, &CCODE);
        if (PLIRETV() == 0) {
            if (CCODE > 0 && CCODE != 13) {   /* VDU AND NOT <ENTER> */
                pl_assign(WD2, 5, LIT(" "));
                CLRSCRN();
                switch (CCODE) {
                case  1: pl_assign(WD1, 5, LIT("H    ")); break;
                case  2: pl_assign(WD1, 5, LIT("I    ")); break;
                case  3: pl_assign(WD1, 5, LIT("Q    ")); break;
                case  4: pl_assign(WD1, 5, LIT("LOG  ")); break;
                case  9: if (WIZARD) pl_assign(WD1, 5, LIT("SWAP ")); break;
                case 12: pl_assign(WD1, 5, LIT("EXIT ")); break;
                case 14:                          /* RESHOW FOR PA2 */
                    pl_vassign(&OUTSTR, RETRYSTR.s, RETRYSTR.len);
                    LINEOUT();
                    goto RETRY;
                default: goto ENTER;
                }
                return;
            }
ENTER:
            pl_vassign(&OUTSTR, INSTR, (int)INLEN);
            WORDSTRT = pl_verify(OUTSTR.s, OUTSTR.len, LIT(" "));
        } else {
            if (BLKLIN) LINESKP();
            pl_vassign(&OUTSTR, LIT("Terminal error..reenter."));
            LINEOUT();
        }
RETRY: ;
    }
    if (LOGON) {
        SYSPRINT_skip();
        SYSPRINT_put(OUTSTR.s, OUTSTR.len);
    }
    pl_vassign(&OUTSTR, OUTSTR.s + WORDSTRT - 1, OUTSTR.len - WORDSTRT + 1);
    WORDEND = pl_index(OUTSTR.s, OUTSTR.len, LIT(" ")) - 1;
    if (WORDEND == -1) WORDEND = OUTSTR.len;
    WORDSIZE = (WORDEND < 5) ? WORDEND : 5;
    pl_assign(WORD1, 5, OUTSTR.s, (int)WORDSIZE);
    if (WORDEND > 5) {
        WORDSIZE = (WORDEND - 5 < 5) ? WORDEND - 5 : 5;
        pl_assign(WORD1X, 5, OUTSTR.s + 5, (int)WORDSIZE);
    }
    if (WORDEND == OUTSTR.len) return;
    pl_vassign(&OUTSTR, OUTSTR.s + WORDEND, OUTSTR.len - WORDEND);
    WORDSTRT = pl_verify(OUTSTR.s, OUTSTR.len, LIT(" "));
    if (WORDSTRT == 0) return;
    pl_vassign(&OUTSTR, OUTSTR.s + WORDSTRT - 1, OUTSTR.len - WORDSTRT + 1);
    WORDEND = pl_index(OUTSTR.s, OUTSTR.len, LIT(" ")) - 1;
    if (WORDEND == -1) WORDEND = OUTSTR.len;
    WORDSIZE = (WORDEND < 5) ? WORDEND : 5;
    pl_assign(WORD2, 5, OUTSTR.s, (int)WORDSIZE);
    if (WORDEND > 5) {
        WORDSIZE = (WORDEND - 5 < 5) ? WORDEND - 5 : 5;
        pl_assign(WORD2X, 5, OUTSTR.s + 5, (int)WORDSIZE);
    }
}

bit1 YES(fixed31 X, fixed31 Y, fixed31 Z)
{
/*  PRINT MESSAGE X, WAIT FOR YES/NO ANSWER.  IF YES, PRINT Y AND
    LEAVE YEA TRUE- IF NO, PRINT Z AND LEAVE YEA FALSE.  */

L1:
    if (X > 0) RSPEAK(X);
    if (X < 0) MSPEAK(-X);
    GETIN(REPLY, JUNK1, JUNK2, JUNK3);
    if (pl_eq(REPLY, 5, LIT("YES")) || pl_eq(REPLY, 5, LIT("Y"))) goto L10;
    if (pl_eq(REPLY, 5, LIT("NO"))  || pl_eq(REPLY, 5, LIT("N"))) goto L20;
    LINESKP();
    RSPEAK(206);
    goto L1;
L10:
    if (Y > 0) RSPEAK(Y);
    if (Y < 0) MSPEAK(-Y);
    return ON;
L20:
    if (Z > 0) RSPEAK(Z);
    if (Z < 0) MSPEAK(-Z);
    return OFF;
}

void A5TOA1(const char *A, const char *B, char CHARS[][1], fixed31 *LENG)
{
/*  A AND B CONTAIN A 1-10 CHARACTER WORD IN A5 FORMAT.  THEY ARE
    CONCATENATED AND MOVED INTO A CHAR(1) ARRAY UNTIL A BLANK IS
    ENCOUNTERED.  THE TOTAL LENGTH IS RETURNED IN LENG.

*/
    char WORDS[3][5];
    pl_assign(WORDS[1], 5, A, 5);
    pl_translate(WORDS[1], 5, XLATETO, 26, XLATEFR, 26);
    pl_assign(WORDS[2], 5, B, 5);
    pl_translate(WORDS[2], 5, XLATETO, 26, XLATEFR, 26);
    *LENG = 0;
    for (WORD = 1; WORD <= 2; WORD++) {
        for (CH = 1; CH <= 5; CH++) {
            CHARS[*LENG + 1][0] = WORDS[WORD][CH - 1];
            if (CHARS[*LENG + 1][0] == ' ') return;
            *LENG = *LENG + 1;
        }
    }
}

/* DATA STRUCTURE ROUTINES (VOCAB, DSTROY, JUGGLE, MOVE, PUT, CARRY,
   DROP) */

fixed31 VOCAB(const char *ID_in, fixed31 INIT)
{
    char ID[5];
    fixed31 I;
    fixed31 VOCRTN;
    int b;

/*  LOOK UP ID IN THE VOCABULARY (ATAB) AND RETURN ITS "DEFINITION"
    (KTAB), OR -1 IF NOT FOUND.  IF INIT IS NON-NEGATIVE, THIS IS AN
    INITIALISATION CALL SETTING UP A KEYWORD VARIABLE, AND NOT FINDING
    IT CONSTITUTES A BUG.

    The PL/I complements ID in place through a BIT(40) overlay -- the
    same minimal hash the vocabulary itself was stored under -- and
    complements it back before returning.  A local copy does the same
    job here without writing through to the caller's WD1.  */

    pl_assign(ID, 5, ID_in, 5);
    for (b = 0; b < 5; b++) ID[b] = (char)(~(unsigned char)ID[b]);

    for (I = 1; I <= TABSIZ; I++) {
        if (KTAB[I] == -1) goto L2;
        if (INIT >= 0 && KTAB[I] / 1000 != INIT) continue;
        if (pl_eq(ATAB[I], 5, ID, 5)) goto L3;
    }
    BUG(21);

L2:
    if (INIT < 0) return -1;
    BUG(5);

L3:
    VOCRTN = KTAB[I];
    if (INIT >= 0) VOCRTN = VOCRTN % 1000;
    return VOCRTN;
}

void DSTROY(fixed31 OBJECT)
{
/*  PERMANENTLY ELIMINATE "OBJECT" BY MOVING TO A NON-EXISTANT
    LOCATION.  */

    MOVE_(OBJECT, 0);
}

void JUGGLE(fixed31 OBJECT)
{
    fixed31 I, J;

/*  JUGGLE AN OBJECT BY PICKING IT UP AND PUTTING IT DOWN AGAIN, THE
    PURPOSE BEING TO GET THE OBJECT TO THE FRONT OF THE CHAIN OF
    THINGS AT ITS LOC.  */

    I = PLACE[OBJECT];
    J = FIXED[OBJECT];
    MOVE_(OBJECT, I);
    MOVE_(OBJECT + 100, J);
}

void MOVE_(fixed31 OBJECT, fixed31 WHERE)
{
/*  PLACE ANY OBJECT ANYWHERE BY PICKING IT UP AND DROPPING IT.  */

    if (OBJECT > 100) goto L1;
    FROM = PLACE[OBJECT];
    goto L2;
L1:
    FROM = FIXED[OBJECT - 100];
L2:
    if (FROM > 0 && FROM <= 300) CARRY(OBJECT, FROM);
    DROP(OBJECT, WHERE);
}

fixed31 PUT_(fixed31 OBJECT, fixed31 WHERE, fixed31 PVAL)
{
/*  PUT IS THE SAME AS MOVE, EXCEPT IT RETURNS A VALUE USED TO SET UP
    THE NEGATED PROP VALUES FOR THE REPOSITORY OBJECTS.  */

    MOVE_(OBJECT, WHERE);
    return (-1) - PVAL;
}

void CARRY(fixed31 OBJECT, fixed31 WHERE)
{
/*  START TOTING AN OBJECT, REMOVING IT FROM THE LIST OF THINGS AT ITS
    FORMER LOCATION.  */

    if (OBJECT > 100) goto L5;
    if (PLACE[OBJECT] == -1) return;
    PLACE[OBJECT] = -1;
    HOLDNG = HOLDNG + 1;
L5:
    if (ATLOC[WHERE] != OBJECT) goto L6;
    ATLOC[WHERE] = LINK[OBJECT];
    return;
L6:
    TEMP = ATLOC[WHERE];
L7:
    if (LINK[TEMP] == OBJECT) goto L8;
    TEMP = LINK[TEMP];
    goto L7;
L8:
    LINK[TEMP] = LINK[OBJECT];
}

void DROP(fixed31 OBJECT, fixed31 WHERE)
{
/*  PLACE AN OBJECT AT A GIVEN LOC, PREFIXING IT ONTO THE ATLOC LIST.
    DECR HOLDNG IF THE OBJECT WAS BEING TOTED.  */

    if (OBJECT > 100) goto L1;
    if (PLACE[OBJECT] == -1) HOLDNG = HOLDNG - 1;
    PLACE[OBJECT] = WHERE;
    goto L2;
L1:
    FIXED[OBJECT - 100] = WHERE;
L2:
    if (WHERE <= 0) return;
    LINK[OBJECT] = ATLOC[WHERE];
    ATLOC[WHERE] = OBJECT;
}

/*  UTILITY ROUTINES (RAN, CIAO, BUG, LINESKP, LINEOUT, HOURFMT) */

void CIAO(void)                                 /*** SUSPEND GAME ***/
{
    char rec[4800];
    int  n;

    STORAGE_open();
    STGREC = 0;
    for (J = 1; J <= 2; J++) {
        NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
        STGREC = STGREC + 1;               /* SKIP FIRST RECORD */
        if (NOSTG) goto BUST;
    }
    if (pl_eq(WD2, 5, LIT(" "))) {
        pl_low(NAME, 8);
    } else {
        pl_assign(PW, 4, WD2, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(NAME, 8, 1, 4, PW, 4);
        {
            char tmp[4];
            tmp[0] = WD2[4];
            memcpy(tmp + 1, WD2X, 3);
            pl_assign(PW, 4, tmp, 4);
        }
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(NAME, 8, 5, 4, PW, 4);
    }
GETRD:
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    STGREC = STGREC + 1;
    if (NOSTG) goto BUST;
    M = PIC_HH * 60 + PIC_MM;
    DATETIME(DATE_STG);
    TODAY = DATE_PIC;
    HHMM = PIC_HHMM;
    ELAPSED = PIC_HH * 60 + PIC_MM + ELAPSED - M;
    if (ELAPSED > 150 && !WIZARD) {            /* ALLOW 150 MINUTES */
        CLRSCRN();
        MSPEAK(1);
        longjmp(pli_goto, JMP_L20000);
    }
    {
        char key[16];
        memcpy(key, NAME, 8);
        memcpy(key + 8, USERID, 8);
        if (pl_eq(ADVREC, sizeof ADVREC, LIT(" "))
            || pl_eq(ADVREC, 16, key, 16)) {
            /* REMOVE TELL-TALE CHARACTER STRINGS */
            int i;
            pl_blank(INSTR, sizeof INSTR);
            pl_blank(JUNK1, 5);
            pl_blank(JUNK2, 5);
            pl_blank(JUNK3, 5);
            pl_blank(REPLY, 5);
            pl_blank(WD1, 5);
            pl_blank(WD1X, 5);
            pl_blank(WD2, 5);
            pl_blank(WD2X, 5);
            for (i = 1; i <= 10; i++) TKWORD[i][0] = ' ';
            pl_vassign(&OUTSTR, INSTR, sizeof INSTR);
            SUSPEND_N = SUSPEND_N + 1;
            n = advars_pack(rec);
            STORAGE_write(STGREC, rec, n);
            STORAGE_close();
            RSPEAK(205);
            LINESKP();
            longjmp(pli_goto, JMP_DEALLOC);
        }
    }
    goto GETRD;
BUST:
    STORAGE_close();
    NOSTG = OFF;
}

void BUG(fixed31 NUM)
{
/*  THE FOLLOWING CONDITIONS ARE CURRENTLY CONSIDERED FATAL BUGS.
    NUMBERS < 20 ARE DETECTED WHILE READING THE DATABASE; THE OTHERS
    OCCUR AT "RUN TIME".  */

    char buf[133];
    pl_assign(buf, 14, LIT("Fatal error # "));
    pl_edit_f(buf + 14, 2, NUM);
    pl_vassign(&OUTSTR, buf, 16);
    if (NUM == 93) {
        /* PUT SKIP EDIT (OUTSTR) (A) -- straight to SYSPRINT, because
           the terminal is what just failed. */
        SYSPRINT_skip();
        SYSPRINT_put(OUTSTR.s, OUTSTR.len);
    } else {
        LINEOUT();
    }
    longjmp(pli_goto, JMP_DEALLOC);
}

void LINESKP(void)                          /* OUTPUTS A BLANK LINE */
{
    pl_vassign(&OUTSTR, LIT(" "));
    LINEOUT();
}

void LINEOUT(void)                       /* OUTPUT A LINE TO MILTEN */
{
    pl_assign(INSTR, sizeof INSTR, OUTSTR.s, OUTSTR.len);
    if (!pl_eq(OUTSTR.s, OUTSTR.len, LIT(" "))) {
        if (LOGON) {
            SYSPRINT_skip();
            SYSPRINT_put(OUTSTR.s, OUTSTR.len);
        }
        pl_vassign(&RETRYSTR, OUTSTR.s, OUTSTR.len);
    }
    for (L = 1; L <= 50; L++) {
        TWRITE(INSTR, OUTSTR.len, &CCODE);
        if (CCODE == 0) return;
    }
    BUG(93);
}

fixed31 RAN(fixed31 N)
{
                         /* RETURNS RANDOM NUMBER BETWEEN 0 AND N-1 */
    fixed31 RANRTN;

    RANRTN = N;
    while (RANRTN == N) {
        RANDU(IX, &IY, &Y);
        IX = IY;
        RANRTN = (fixed31)(Y * (float)N);
    }
    return RANRTN;
}

void HOURFMT(void)                            /* DISPLAY OPEN HOURS */
{
    char buf[133];
    int p = 0;
#define CAT(s, n) do { memcpy(buf + p, (s), (size_t)(n)); p += (n); } while (0)
    CAT("before ", 7);
    CAT(TIMES[1], 2); CAT(":", 1); CAT(TIMES[1] + 2, 2); CAT(", ", 2);
    CAT(TIMES[2], 2); CAT(":", 1); CAT(TIMES[2] + 2, 2); CAT(" to ", 4);
    CAT(TIMES[3], 2); CAT(":", 1); CAT(TIMES[3] + 2, 2); CAT(", after ", 8);
    CAT(TIMES[4], 2); CAT(":", 1); CAT(TIMES[4] + 2, 2);
    CAT(", and at weekends.", 18);
#undef CAT
    pl_vassign(&OUTSTR, buf, p);
    LINEOUT();
}

/*  WIZARD ROUTINES (REVERT, WIZPROC, DEMOCHK, PUTBACK, SCANSTG) */

void REVERT(void)
{
    fixed31 I;

    if (pl_eq(CARD + 1, 2, LIT("EN"))) {
        COLS_set(4, (int16_t)-COLS_get(4));
        pl_blank(CARD + 32, 20);
    } else {
        for (I = 3; I <= 36; I++) {
            if (COLS_get(I) == 0)
                pl_blank(CARD + I * 2 - 2, 2);
            else
                COLS_set(I, (int16_t)-COLS_get(I));
        }
    }
    pl_blank(CARD, 4);
    if (pl_eq(CARD + 72, 2, LIT("PR"))) pl_blank(CARD + 72, 2);
    else COLS_set(37, (int16_t)-COLS_get(37));
    if (pl_eq(CARD + 74, 2, LIT("OG"))) pl_blank(CARD + 74, 2);
    else COLS_set(38, (int16_t)-COLS_get(38));
    if (pl_cmp(CARD + 76, 2, LIT("00")) >= 0) pl_blank(CARD + 76, 2);
    else COLS_set(39, (int16_t)-COLS_get(39));
    if (pl_cmp(CARD + 78, 2, LIT("00")) >= 0) pl_blank(CARD + 78, 2);
    else COLS_set(40, (int16_t)-COLS_get(40));
}

/* PL/I MOD always returns a result with the sign of the modulus. */
static fixed31 plimod(fixed31 a, fixed31 b)
{
    fixed31 r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    return r;
}

void THISDAY(vchar133 *RSTRING)
{
    fixed31 DAY;
    const char *MONTH;
    fixed15 IM, MM, DD, CC, YY;
    fixed15 WORKBIN1, WORKBIN2, WORKBIN3, WORKBIN4;
    char buf[133];
    int p = 0;

    YY  = (fixed15)PIC_YEAR;
    IM  = (fixed15)PIC_MONTH;
    DAY = PIC_DAY;
    MONTH = CMONTH[IM];                     /* EBCDIC MONTH */

    /*  ZELLERS CONGRUENCE USED TO DETERMINE DAY OF WEEK  */
    DD = (fixed15)DAY;
    CC = (fixed15)(YY / 100);                       /* CENTURY */
    YY = (fixed15)(YY - (CC * 100));                /* YEAR IN CENTURY */
    MM = (fixed15)(IM - 2);
    if (MM <= 0) {
        MM = (fixed15)(MM + 12);                    /* MONTHS + 12 */
        YY = (fixed15)(YY - 1);                     /* YEAR - 1    */
        if (YY < 0) {
            YY = 99;
            CC = (fixed15)(CC - 1);                 /* CENTURY - 1 */
        }
    }
    WORKBIN1 = (fixed15)(((MM * 26) - 2) / 10);
    WORKBIN2 = (fixed15)(YY / 4);
    WORKBIN3 = (fixed15)(CC / 4);
    WORKBIN4 = (fixed15)(CC * 2);
    DAY_N = plimod(WORKBIN1 + DD + YY + WORKBIN2 + WORKBIN3 - WORKBIN4, 7);

    /* PUT STRING(RSTRING) EDIT(WEEK_DAY(DAY#),DAY,MONTH,PIC_YEAR)
                               (A,P'Z9',X(1),A,X(1),A) */
    pl_assign(buf + p, 10, WEEK_DAY[DAY_N], 9);         p += 10;
    pl_edit_zn(buf + p, 2, DAY);                        p += 2;
    buf[p++] = ' ';
    memcpy(buf + p, MONTH, strlen(MONTH));              p += (int)strlen(MONTH);
    buf[p++] = ' ';
    pl_pic_put(buf + p, 4, PIC_YEAR);                   p += 4;
    pl_vassign(RSTRING, buf, p);
}

void WIZPROC(void)
{
    char MSGUID[8];
    fixed31 i_;

    if (WIZARD) {                               /* BEEN HERE BEFORE */
        RSPEAK(205);
        goto GETCMD;
    }
    MSPEAK(17);
REASK1:
    WHISPER();                          /* SUPPLY ECHO SUPPRESSION */
    TREAD(INSTR, 0, INSTR, &INLEN, &CCODE);
    TEST_WORD = PLIRETV();                    /* SAVE RETURN CODE */
    CLRSCRN();
    if (TEST_WORD != 0) goto REASK1;
    if (CCODE == 11 && pl_eq(INSTR, 5, LIT("RC=04"))) {
        /* SORRY TO HAVE BOTHERED YOU... */
    } else {
        if (pl_eq(INSTR, 5, LIT("RC=04"))) { /* LET HIM PLAY OFF HOURS */
            MSPEAK(20);
            TURNS = TURNS + 1;
            return;
        }
        MSPEAK(18);                        /* GIVE HIM ANOTHER TRY */
REASK2:
        WHISPER();
        TREAD(INSTR, 0, INSTR, &INLEN, &CCODE);
        TEST_WORD = PLIRETV();
        CLRSCRN();
        if (TEST_WORD != 0) goto REASK2;
        if (CCODE == 11 && pl_eq(INSTR, 5, LIT("RC=04"))) {
            /* SORRY TO HAVE BOTHERED YOU... */
        } else {
            MSPEAK(20);
            if (pl_eq(INSTR, 5, LIT("RC=04"))) {
                TURNS = TURNS + 1;
                return;
            }
            INHIB[0] = 'Y';
            MSPEAK(33);
            pl_vsubstr_assign(&OUTSTR, 24, 8, TSOID, 8); /* PLEASE CANCEL */
            /* CANMSG = OUTSTR: the operator-console WTOR text.  There is
               no operator console standalone, so BUG(35) directly --
               the same substitution the PL/I port makes. */
            BUG(35);
        }
    }
    MSPEAK(19);
    WIZARD = ON;

GETCMD:
    GETIN(WD1, WD1X, WD2, WD2X);            /* GET WIZARD'S WISH */

    J = VOCAB(WD1, -1);
    switch (J) {
    case   11: longjmp(pli_goto, JMP_DEALLOC);       /* LEAVE */
    case   57: longjmp(pli_goto, JMP_L31);           /* LOOK  */
    case 2002: goto DUMP;                            /* DROP  */
    case 2003: goto SEND;                            /* SAY   */
    case 2008: WIZARD = OFF; break;                  /* OFF   */
    case 2011: goto EXPLORE;                         /* GO TO */
    case 2019: goto DESCRIBE;                        /* WHERE */
    case 2027: GETMSGS(TSOID, 'W'); break;           /* READ  */
    case 2031: goto HOURS;                           /* HOURS */
    case 2035: goto ACTION;                          /* SWAP  */
    case 2032: goto SETLOG;                          /* HARDC */
    case 3051: MSPEAK(52); break;                    /* HELP  */
    case 3142: goto HISTORY;                         /* INFO  */
    default:   goto WDCMPR;
    }
    goto GETCMD;

WDCMPR:
    if      (pl_eq(WD1, 5, LIT("ACTIO"))) goto ACTION;
    else if (pl_eq(WD1, 5, LIT("ADD  "))) goto ADDUSER;
    else if (pl_eq(WD1, 5, LIT("DECOD"))) goto DECODE;
    else if (pl_eq(WD1, 5, LIT("DLOCS"))) goto DLOCS;
    else if (pl_eq(WD1, 5, LIT("DEL  "))) goto DELUSER;
    else if (pl_eq(WD1, 5, LIT("ENCOD"))) goto ENCODE;
    else if (pl_eq(WD1, 5, LIT("HIST "))) goto HISTORY;
    else if (pl_eq(WD1, 5, LIT("LISTB"))) GETMSGS(TSOID, 'A');
    else if (pl_eq(WD1, 5, LIT("LISTI"))) goto LISTIDS;
    else if (pl_eq(WD1, 5, LIT("LOGOF"))) longjmp(pli_goto, JMP_DEALLOC);
    else if (pl_eq(WD1, 5, LIT("SAUCE"))) goto SAUCE;
    else if (pl_eq(WD1, 5, LIT("SCAN "))) SCANSTG();
    else if (pl_eq(WD1, 5, LIT("SEND "))) goto SEND;
    else if (pl_eq(WD1, 5, LIT("STG  "))) goto PUTBLNK;
    else {
        SPK = 60;
        if (PCT(20)) SPK = 61;
        if (PCT(20)) SPK = 13;
        RSPEAK(SPK);
    }
    goto GETCMD;

LISTIDS:                                  /* LIST AUTHORIZED USERS */
    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    STORAGE_close();
    if (NOSTG) {
        MSPEAK(34);                                   /* EMPTY FILE */
        NOSTG = OFF;
        goto GETCMD;
    }
    if (pl_eq(WD2, 5, LIT(" "))) {
        for (M = 0; M <= 74; M++) {                      /* LISTIDS */
            pl_vassign(&OUTSTR, ADVREC + M * 64, 64);
            for (KQ = 0; KQ <= 7; KQ++) {
                if (OUTSTR.s[KQ * 8] != ' ') {
                    char tmp[4];
                    pl_assign(PW, 4, OUTSTR.s + KQ * 8, 4);
                    TEST_WORD = -TEST_WORD;
                    pl_vsubstr_assign(&OUTSTR, KQ * 8 + 1, 4, PW, 4);
                    memcpy(tmp, OUTSTR.s + KQ * 8 + 4, 3);
                    tmp[3] = ' ';
                    pl_assign(PW, 4, tmp, 4);
                    TEST_WORD = -TEST_WORD;
                    PW[3] = ' ';
                    pl_vsubstr_assign(&OUTSTR, KQ * 8 + 5, 4, PW, 4);
                }
            }
            if (M == 74) OUTSTR.len = 48;
            if (!pl_eq(OUTSTR.s, OUTSTR.len, LIT(" "))) LINEOUT();
        }
    } else {                                   /* OPERAND SUPPLIED */
        if (pl_eq(WD2, 5, LIT("ALL  "))) {
            for (M = 0; M <= 597; M++) {  /* LIST ALL USERS & STATS */
                char buf[133];
                if (ADVREC[M * 8] == ' ') continue;
                pl_assign(MSGUID, 8, ADVREC + M * 8, 7);
                pl_assign(PW, 4, MSGUID, 4);
                TEST_WORD = -TEST_WORD;
                pl_substr_assign(MSGUID, 8, 1, 4, PW, 4);
                pl_assign(PW, 4, MSGUID + 4, 4);
                TEST_WORD = -TEST_WORD;
                pl_substr_assign(MSGUID, 8, 5, 3, PW, 3);
                INTEGER = (fixed15)(unsigned char)ADVREC[M * 8 + 7];
                memcpy(buf, MSGUID, 8);
                pl_edit_f(buf + 8, 3, INTEGER);
                pl_vassign(&OUTSTR, buf, 11);
                LINEOUT();
            }
        } else {                        /* LIST A PARTICULAR USER */
            char tmp[4];
            pl_assign(PW, 4, WD2, 4);
            TEST_WORD = -TEST_WORD;
            pl_substr_assign(MSGUID, 8, 1, 4, PW, 4);
            tmp[0] = WD2[4];
            memcpy(tmp + 1, WD2X, 2);
            tmp[3] = ' ';
            pl_assign(PW, 4, tmp, 4);
            TEST_WORD = -TEST_WORD;
            pl_substr_assign(MSGUID, 8, 5, 4, PW, 4);
            M = pl_index(ADVREC, sizeof ADVREC, MSGUID, 7);
            if (M == 0) {                      /* USERID NOT FOUND */
                char buf[133];
                int p, q;
                A5TOA1(WD2, WD2X, TKWORD, &K);
                INHIB[0] = 'Y';
                RSPEAK(220);
                pl_edit_a(buf, 9, OUTSTR.s, OUTSTR.len);  p = 9;
                for (q = 1; q <= K && p < 126; q++) buf[p++] = TKWORD[q][0];
                memcpy(buf + p, " here!", 6); p += 6;
                pl_vassign(&OUTSTR, buf, p);
                INHIB[0] = 'N';
                LINEOUT();
            } else {
                char buf[133];
                INTEGER = (fixed15)(unsigned char)ADVREC[M + 7 - 1];
                memcpy(buf, WD2, 5);
                memcpy(buf + 5, WD2X, 5);
                pl_edit_f(buf + 10, 3, INTEGER);
                pl_vassign(&OUTSTR, buf, 13);
                LINEOUT();
            }
        }
    }
    pl_blank(ADVREC, sizeof ADVREC);
    goto GETCMD;

DLOCS:                                /* DISPLAY DWARFS' LOCATIONS */
    for (M = 1; M <= 6; M++)
        PLOC_SET(M, DLOC[M]);
    pl_vassign(&OUTSTR, LOC_LINE, sizeof LOC_LINE);
    LINEOUT();
    goto GETCMD;

HOURS:
    if (YES(-10, -6, 0)) {                            /* SEE HOURS */
        HOURFMT();
        goto GETCMD;
    }
    if (!YES(-11, -21, -7)) goto GETCMD;
    GETIN(WD1, WD1X, WD2, WD2X);                   /* CHANGE HOURS */
    pl_assign(TIMES[1], 4, WD1, 4);
    pl_assign(TIMES[2], 4, WD2, 4);
    GETIN(WD1, WD1X, WD2, WD2X);
    pl_assign(TIMES[3], 4, WD1, 4);
    pl_assign(TIMES[4], 4, WD2, 4);
    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    if (NOSTG) goto NO_STORAGE;
    for (M = 1; M <= 4; M++) {
        pl_assign(PW, 4, TIMES[M], 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(ADVREC, sizeof ADVREC, 4781 + M * 4, 4, PW, 4);
    }
    STORAGE_rewrite(ADVREC, sizeof ADVREC);
    STORAGE_close();
    LINESKP();
    MSPEAK(22);
    HOURFMT();
    goto GETCMD;

EXPLORE:                        /* ZAP THE WIZARD TO A NEW LOCATION */
    L = pl_get_f(WD2, 5, 1, 5);
    if (L < 0 || L > 300) {
        RSPEAK(25);                    /* INVALID LOCATION NUMBER */
    } else {
        LOC = L;
        OLDLOC = 0;
        OLDLC2 = 0;
        RSPEAK(205);
    }
    goto GETCMD;

DUMP:                                           /* DROP AN OBJECT */
    if (pl_eq(WD2, 5, LIT(" "))) {
        pl_vassign(&OUTSTR, LIT("OBJ?"));
        LINEOUT();
        GETIN(WD1, WD1X, WD2, WD2X);
        pl_assign(WD2, 5, WD1, 5);
    }
    OBJ = VOCAB(WD2, -1);
    if (OBJ < 1001 || OBJ > 1100) {
        if (OBJ == -1) RSPEAK(60);
        else           RSPEAK(12);
        goto GETCMD;
    }
    OBJ = OBJ - 1000;
    pl_vassign(&OUTSTR, LIT("LOC?"));
    LINEOUT();
    GETIN(WD1, WD1X, WD2, WD2X);
    L = pl_get_f(WD1, 5, 1, 5);
    if (L < 0 || L > 300) {
        RSPEAK(25);                    /* INVALID LOCATION NUMBER */
        goto GETCMD;
    }
    MOVE_(OBJ, L);                                 /* DUMP OBJECT */
    RSPEAK(205);
    goto GETCMD;

DESCRIBE:                                  /* DESCRIBE A LOCATION */
    L = pl_get_f(WD2, 5, 1, 5);
    if (L < 0 || L > 300) {
        RSPEAK(25);
        goto GETCMD;
    }
    if (L == 0) {
        char buf[8];
        pl_edit_f(buf, 3, LOC);
        pl_vassign(&OUTSTR, buf, 3);
        LINEOUT();
    } else {
        SPEAK(LTEXT[L]);
    }
    goto GETCMD;

SETLOG:
    LOGON = (bit1)!LOGON;        /* TOGGLE SYSPRINT LOGGING SWITCH */
    if (LOGON) MSPEAK(42);                             /* LOG ON  */
    else       MSPEAK(43);                             /* LOG OFF */
    goto GETCMD;

ENCODE:                                          /* ENCODE DATABASE */
    OBJECT_open_update();
    CARD_N_SET(0);
RD1:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) goto SHUT1;
    CARD_N_SET(CARD_N + 1);
/*  DATABASE IS DISGUISED AS AN OBJECT DECK: ESD'S ARE REAL, TXT'S ARE
    SECTIONS 1 TO 11, RLD'S ARE SECTION 12, END IS SECTION 0.  */
    if (CARD[0] == HEX02) goto RD1;         /* IGNORE ENCODED CARDS */
    if (pl_eq(CARD + 5, 11, LIT(" 12        "))) RLD = ON;
    CARD[0] = HEX02;
    if (RLD) {
        if (pl_eq(CARD + 6, 3, LIT(" 0 "))) {
            DECDATE(&YYDDD);           /* GET DATE IN YYDDD FORMAT */
            CARD[1] = XLATEFR[4];
            CARD[2] = XLATEFR[13];
            CARD[3] = XLATEFR[3];
            YDPIC_SET(YYDDD);
            pl_substr_assign(CARD, 80, 33, 6, LIT("15734-"));
            CARD[38] = XLATEFR[15];
            CARD[39] = XLATEFR[11];
            pl_substr_assign(CARD, 80, 41, 7, LIT("1  0400"));
            pl_substr_assign(CARD, 80, 48, 5, YDCHR, 5);
        } else {
            CARD[1] = XLATEFR[17];
            CARD[2] = XLATEFR[11];
            CARD[3] = XLATEFR[3];
        }
    } else {
        CARD[1] = XLATEFR[19];
        CARD[2] = XLATEFR[23];
        CARD[3] = XLATEFR[19];
    }
    if (pl_eq(CARD + 1, 2, LIT("EN"))) {
        COLS_set(4, (int16_t)-COLS_get(4));
    } else {
        for (i_ = 3; i_ <= 36; i_++) {
            if (pl_eq(CARD + i_ * 2 - 2, 2, LIT(" ")))
                COLS_set(i_, 0);
            else
                COLS_set(i_, (int16_t)-COLS_get(i_));
        }
    }
    if (pl_eq(CARD + 72, 2, LIT("  ")))
        pl_substr_assign(CARD, 80, 73, 2, LIT("PR"));
    else COLS_set(37, (int16_t)-COLS_get(37));
    if (pl_eq(CARD + 74, 2, LIT("  ")))
        pl_substr_assign(CARD, 80, 75, 2, LIT("OG"));
    else COLS_set(38, (int16_t)-COLS_get(38));
    if (pl_eq(CARD + 76, 2, LIT("  ")))
        pl_substr_assign(CARD, 80, 77, 2, SEQ_N, 2);
    else COLS_set(39, (int16_t)-COLS_get(39));
    if (pl_eq(CARD + 78, 2, LIT("  ")))
        pl_substr_assign(CARD, 80, 79, 2, SEQ_N + 2, 2);
    else COLS_set(40, (int16_t)-COLS_get(40));
    OBJECT_rewrite(CARD);
    goto RD1;
SHUT1:
    OBJECT_close();
    EOCARD = OFF;
    RSPEAK(205);
    goto GETCMD;

DECODE:                              /* DECODE DATABASE FOR EDITING */
    OBJECT_open_update();
RD2:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) goto SHUT2;
    if (pl_eq(CARD + 1, 2, LIT("ES"))) goto RD2;  /* IGNORE ESD CARDS */
    if (CARD[0] == HEX02) REVERT();
    OBJECT_rewrite(CARD);
    goto RD2;
SHUT2:
    OBJECT_close();
    EOCARD = OFF;
    RSPEAK(205);
    goto GETCMD;

SAUCE:
    SOURCE_open_update();
RD3:
    EOCARD = (bit1)SOURCE_read(CARD);
    if (EOCARD) goto SHUT3;
    for (i_ = 1; i_ <= 40; i_++) {
        if (pl_eq(CARD + i_ * 2 - 2, 2, LIT(" "))) {
            COLS_set(i_, 0);
        } else {
            if (COLS_get(i_) == 0)
                pl_blank(CARD + i_ * 2 - 2, 2);
            else
                COLS_set(i_, (int16_t)-COLS_get(i_));
        }
    }
    SOURCE_rewrite(CARD);
    goto RD3;
SHUT3:
    SOURCE_close();
    EOCARD = OFF;
    RSPEAK(205);
    goto GETCMD;

HISTORY:                           /* DISPLAY PROGRAM & D/B HISTORY */
    OBJECT_open();
RD4:
    EOCARD = (bit1)OBJECT_read(CARD);
    if (EOCARD) goto SHUT4;
    if (pl_eq(CARD + 1, 2, LIT("EN"))) {
        char buf[133];
        int p = 0;
#define CAT(s, n) do { memcpy(buf + p, (s), (size_t)(n)); p += (n); } while (0)
        CAT("DB: V", 5);        CAT(CARD + 43, 2);  CAT(".", 1);
        CAT(CARD + 45, 2);      CAT(" last updated on ", 17);
        CAT(CARD + 47, 2);      CAT(".", 1);        CAT(CARD + 49, 3);
        CAT(" and has ", 9);    CAT(CARD + 76, 4);  CAT(" cards", 6);
#undef CAT
        pl_vassign(&OUTSTR, buf, p);
        LINEOUT();
    } else {
        goto RD4;
    }
SHUT4:
    OBJECT_close();
    EOCARD = OFF;
    {
        char buf[133];
        memcpy(buf, "Program was compiled on ", 24);
        memcpy(buf + 24, MACROTIME, 24);
        pl_vassign(&OUTSTR, buf, 48);
    }
    LINEOUT();
    goto GETCMD;

SEND:                                               /* SEND MESSAGE */
    STORAGE_open();
    for (I = 1; I <= 2; I++) {
        NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
        if (NOSTG) goto NO_STORAGE;
    }
    MSPEAK(23);
    for (J = 0; J <= 79; J++) {
        pl_assign(CARD, 80, ADVREC + J * 80, 80);
        if (CARD[0] != ' ') {
            for (L = 1; L <= 40; L++)
                COLS_set(L, (int16_t)-COLS_get(L));
            if (pl_eq(WD2, 5, LIT(" "))) {
                if (CARD[79] != '*') continue;
            } else {
                char key[7];
                memcpy(key, WD2, 5);
                memcpy(key + 5, WD2X, 2);
                if (!pl_eq(CARD + 72, 7, key, 7)) continue;
            }
        }
        pl_vassign(&OUTSTR, CARD, 79);
SEND_RETRY:
        LINEOUT();
SENDRD:
        TREAD(INSTR, 0, INSTR, &INLEN, &CCODE);
        if (CCODE == 14) goto SEND_RETRY;
        if (CCODE == 3) goto DSPTCH;
        if (INLEN > 79) {
            MSPEAK(24);
            goto SENDRD;
        }
        if (INLEN > 0)
            pl_substr_assign(CARD, 80, 1, (int)INLEN, INSTR, (int)INLEN);
        if (pl_eq(WD2, 5, LIT(" "))) {
            CARD[79] = '*';
        } else {
            char key[8];
            memcpy(key, WD2, 5);
            memcpy(key + 5, WD2X, 3);
            pl_substr_assign(CARD, 80, 73, 8, key, 8);
        }
        for (L = 1; L <= 40; L++)
            COLS_set(L, (int16_t)-COLS_get(L));
        pl_substr_assign(ADVREC, sizeof ADVREC, J * 80 + 1, 80, CARD, 80);
    }
    MSPEAK(25);
DSPTCH:
    STORAGE_rewrite(ADVREC, sizeof ADVREC);
    STORAGE_close();
    RSPEAK(205);
    goto GETCMD;

PUTBLNK:                 /* INCREASE MAX # OF SIMULT. STORED GAMES */
    M = pl_get_f(WD2, 5, 1, 5);
    INHIB[0] = 'Y';
    MSPEAK(48);                       /* ADDING TO MOD FILE, OKAY? */
    pl_vsubstr_assign(&OUTSTR, 8, 5, WD2, 5);
    INHIB[0] = 'N';
    LINEOUT();
    GETIN(REPLY, JUNK1, JUNK2, JUNK3);
    if (REPLY[0] == 'N') goto GETCMD;
    pl_blank(ADVREC, sizeof ADVREC);
    STORAGE_open_output();
    for (I = 1; I <= M; I++)
        STORAGE_append(ADVREC, sizeof ADVREC);
    STORAGE_close();
    RSPEAK(205);
    goto GETCMD;

ADDUSER:                      /* AUTHORIZE A USER TO PLAY ADVENTURE */
    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    if (NOSTG) goto NO_STORAGE;
    {
        char tmp[4];
        pl_assign(PW, 4, WD2, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(MSGUID, 8, 1, 4, PW, 4);
        tmp[0] = WD2[4];
        memcpy(tmp + 1, WD2X, 3);
        pl_assign(PW, 4, tmp, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(MSGUID, 8, 5, 4, PW, 4);
    }
    for (I = 0; I <= 599; I++) {
        if (pl_eq(ADVREC + I * 8, 7, MSGUID, 7)) {
            M = 36;
            goto ADDEND;
        }
        if (pl_eq(ADVREC + I * 8, 8, LIT(" "))) {
            pl_substr_assign(ADVREC, sizeof ADVREC, I * 8 + 1, 7, MSGUID, 7);
            ADVREC[I * 8 + 7] = 0;                       /* LOW(1) */
            STORAGE_rewrite(ADVREC, sizeof ADVREC);
            M = 39;
            goto ADDEND;
        }
    }
    M = 37;
ADDEND:
    STORAGE_close();
    MSPEAK(M);
    goto GETCMD;

DELUSER:                         /* WITHDRAW A USER'S AUTHORIZATION */
    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    if (NOSTG) goto NO_STORAGE;
    {
        char tmp[4];
        pl_assign(PW, 4, WD2, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(MSGUID, 8, 1, 4, PW, 4);
        tmp[0] = WD2[4];
        memcpy(tmp + 1, WD2X, 3);
        pl_assign(PW, 4, tmp, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(MSGUID, 8, 5, 4, PW, 4);
    }
    for (I = 0; I <= 597; I++) {
        if (pl_eq(ADVREC + I * 8, 7, MSGUID, 7)) {
            pl_blank(ADVREC + I * 8, 8);
            MSPEAK(38);                                  /* DELETED */
        }
    }
    STORAGE_rewrite(ADVREC, sizeof ADVREC);
    STORAGE_close();
    {
        char m8[8];
        memcpy(m8, WD2, 5);
        memcpy(m8 + 5, WD2X, 3);
        GETMSGS(m8, 'M');
    }
    RSPEAK(205);
    goto GETCMD;

NO_STORAGE:
    STORAGE_close();
    MSPEAK(35);
    NOSTG = OFF;
    goto GETCMD;

ACTION:                                           /* LET'S GO PLAY! */
    CLRSCRN();
    MSPEAK(7);
}

void DEMOCHK(void)
{
    if (TTYPE[0] == 'T') {
        MSPEAK(3);
        HOURFMT();
        goto DEMO;
    }
    MSPEAK(4);
    if (YES(-16, 0, 0)) {
        WIZPROC();
        longjmp(pli_goto, JMP_L31);
    }
DEMO:
    DEMOGM = YES(-5, -7, -7);                     /* NOT A WIZARD */
    LINESKP();
    if (!DEMOGM) longjmp(pli_goto, JMP_DEALLOC);
    TURNS = TURNS + 1;
}

void PUTBACK(void)
{
/*  RESTORE SUSPENDED GAME.  CARD# COUNTS HOW MANY SAVED GAMES THIS
    USER HAS; ADVREC IS A TEMPORARY HOLDING THE SEARCH KEY:
      BYTES  1 -> 8   ENCODED USERID
      BYTES  9 -> 16  NAME OF GAME
      BYTES 17 -> 24  ENCODED NAME OF GAME  */

    char rec[4800];

    CARD_N_SET(0);
    pl_substr_assign(ADVREC, sizeof ADVREC, 1, 8, USERID, 8);
    {
        char n8[8];
        memcpy(n8, WD2, 5);
        memcpy(n8 + 5, WD2X, 3);
        pl_substr_assign(ADVREC, sizeof ADVREC, 9, 8, n8, 8);
    }
    if (pl_eq(WD2, 5, LIT(" "))) {
        pl_low(ADVREC + 16, 8);
    } else {
        pl_assign(PW, 4, ADVREC + 8, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(ADVREC, sizeof ADVREC, 17, 4, PW, 4);
        pl_assign(PW, 4, ADVREC + 12, 4);
        TEST_WORD = -TEST_WORD;
        pl_substr_assign(ADVREC, sizeof ADVREC, 21, 4, PW, 4);
    }
    STORAGE_open();
    STGREC = 0;
BRWSE:
    if (STORAGE_read(rec, sizeof rec)) {
        NOSTG = ON;
    } else {
        advars_unpack(rec);        /* READ FILE (STORAGE) INTO (ADVARS) */
        NOSTG = OFF;               /* set after the unpack, not before:
                                    * the record overwrites ADVARS, and
                                    * NOSTG is itself a member of it */
    }
    STGREC = STGREC + 1;
    if (NOSTG) {
        STORAGE_close();
        CLRSCRN();
        MSPEAK(47);                    /* SUSPENDED GAME NOT FOUND */
        INHIB[0] = 'Y';
        MSPEAK(49);                /* REPORT NUMBER OF GAMES FOUND */
        pl_vsubstr_assign(&OUTSTR, 1, 4, SEQ_N, 4);
        pl_vsubstr_assign(&OUTSTR, 22, 8, TSOID, 8);
        LINEOUT();
        if (ADVREC[8] != ' ') {                   /* NAME WAS GIVEN */
            INHIB[0] = 'Y';
            MSPEAK(50);                          /* GAME NOT SAVED */
            pl_vsubstr_assign(&OUTSTR, 1, 8, ADVREC + 8, 8);
            pl_vsubstr_assign(&OUTSTR, 29, 8, TSOID, 8);
            INHIB[0] = 'N';
            LINEOUT();
        }
        LINESKP();
        longjmp(pli_goto, JMP_DEALLOC);
    }
    if (!pl_eq(USERID, 8, ADVREC, 8)) goto BRWSE;
    CARD_N_SET(CARD_N + 1);
    if (!pl_eq(NAME, 8, ADVREC + 16, 8)) goto BRWSE;

    if (!pl_eq(VERSION, 4, LIT("V4.0"))) {      /*** FOUND GAME ***/
        STORAGE_close();                    /** INCOMPATIBLE LEVEL **/
        MSPEAK(46);
        LINESKP();
        longjmp(pli_goto, JMP_DEALLOC);
    }
    if (DATE_PIC == TODAY) {             /* GAME WAS SUSPENDED TODAY */
        if (PIC_HHMM - HHMM < 100) {        /* LESS THAN AN HOUR AGO */
            STORAGE_close();
            CLRSCRN();
            MSPEAK(2);
            MSPEAK(9);
            LINESKP();
            longjmp(pli_goto, JMP_DEALLOC);
        }
        if (PIC_HHMM - HHMM < 200 && !WIZARD) { /* LESS THAN 2 HOURS */
            STORAGE_close();
            CLRSCRN();
            MSPEAK(8);
            MSPEAK(9);
            LINESKP();
            longjmp(pli_goto, JMP_DEALLOC);
        }
    }
    pl_blank(ADVREC, sizeof ADVREC);
    STORAGE_write(STGREC, ADVREC, sizeof ADVREC);
    STORAGE_close();
    INHIB[0] = 'Y';
    MSPEAK(51);                         /* REPORT RESTORE STATS */
    pl_edit_f(INSTR,     3, SUSPEND_N);
    pl_edit_f(INSTR + 3, 4, TURNS);
    pl_edit_f(INSTR + 7, 4, ELAPSED);
    pl_vsubstr_assign(&OUTSTR, 12, 3, INSTR,     3);
    pl_vsubstr_assign(&OUTSTR, 21, 4, INSTR + 3, 4);
    pl_vsubstr_assign(&OUTSTR, 34, 4, INSTR + 7, 4);
    INHIB[0] = 'N';
    LINEOUT();
    LINESKP();
    CARD_N_SET(0);
}

void GETMSGS(const char *UID, char X)
{
/*  UPDTE SAYS WHETHER THE MESSAGE RECORD HAS TO BE REWRITTEN.
    X: A - ALL MESSAGES                    (MAIL, NOTICES)
       B - BROADCAST MESSAGES ONLY         (NOMAIL, NOTICES)
       M - MAIL ONLY                       (MAIL, NONOTICES)
       W - WIZARD LISTING MSG USE  (EVERYBODY'S MAIL, NOTICES)  */

    char UPDTE;

    UPDTE = 'N';
    STORAGE_open();
    for (J = 1; J <= 2; J++) {
        NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
        if (NOSTG) {
            NOSTG = OFF;
            STORAGE_close();
            if (J == 1) MSPEAK(34);                   /* EMPTY FILE */
            else        MSPEAK(35);                   /* NO STORAGE */
            return;
        }
    }
    for (J = 0; J <= 59; J++) {
        pl_assign(CARD, 80, ADVREC + J * 80, 80);
        if (CARD[0] == ' ') continue;                    /* NEXT J */
        for (L = 1; L <= 40; L++)
            COLS_set(L, (int16_t)-COLS_get(L));
        if (X == 'M') goto MAIL;
        if (CARD[79] == '*'                   /* BROADCAST MESSAGES */
            || (X == 'W' && CARD[72] != ' ')) {    /* ALL FOR WIZ */
            pl_vassign(&OUTSTR, CARD, 79);
            LINEOUT();
        }
        if (X == 'B') continue;                 /* NOTICES, NOMAIL */
MAIL:
        if (pl_eq(CARD + 72, 7, UID, 7)) {
            pl_vassign(&OUTSTR, CARD, 72);
            LINEOUT();
            pl_blank(ADVREC + J * 80, 80);
            UPDTE = 'Y';
        }
    }
    if (UPDTE == 'Y')
        STORAGE_rewrite(ADVREC, sizeof ADVREC);
    STORAGE_close();
}

void SCANSTG(void)
{
    char rec[4800];

    STORAGE_open();
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    if (NOSTG) {
        STORAGE_close();
        MSPEAK(34);                                   /* EMPTY FILE */
        return;
    }
    for (M = 0; M <= 74; M++) {
        pl_vassign(&OUTSTR, ADVREC + M * 64, 64);
        for (KQ = 0; KQ <= 7; KQ++) {
            if (OUTSTR.s[KQ * 8] != ' ') {
                pl_assign(PW, 4, OUTSTR.s + KQ * 8, 4);
                TEST_WORD = -TEST_WORD;
                pl_vsubstr_assign(&OUTSTR, KQ * 8 + 1, 4, PW, 4);
                if (M == 74 && (KQ == 6 || KQ == 7)) {          /* HOURS */
                    pl_assign(PW, 4, OUTSTR.s + KQ * 8 + 4, 4);
                    TEST_WORD = -TEST_WORD;
                    pl_vsubstr_assign(&OUTSTR, KQ * 8 + 5, 4, PW, 4);
                } else {                                        /* USERS */
                    char tmp[4];
                    memcpy(tmp, OUTSTR.s + KQ * 8 + 4, 3);
                    tmp[3] = ' ';
                    pl_assign(PW, 4, tmp, 4);
                    TEST_WORD = -TEST_WORD;
                    PW[3] = ' ';
                    pl_vsubstr_assign(&OUTSTR, KQ * 8 + 5, 4, PW, 4);
                }
            }
        }
        if (!pl_eq(OUTSTR.s, OUTSTR.len, LIT(" "))) LINEOUT();
    }
    NOSTG = (bit1)STORAGE_read(ADVREC, sizeof ADVREC);
    if (NOSTG) goto LOOPND;
    pl_assign(CARD, 80, ADVREC, 80);    /* FIRST BIT OF MSG RECORD */
    if (!pl_eq(CARD, 80, LIT(" ")))
        for (L = 1; L <= 40; L++)
            COLS_set(L, (int16_t)-COLS_get(L));
    pl_vassign(&OUTSTR, CARD, 79);
    LINEOUT();
    pl_blank(ADVREC, sizeof ADVREC);
    GETIN(REPLY, JUNK1, JUNK2, JUNK3);
    L = VOCAB(REPLY, -1);
    if (pl_eq(REPLY, 3, LIT("DEL")) || L == 1028 || L == 2012) {
        STORAGE_rewrite(ADVREC, sizeof ADVREC);
        MSPEAK(38);                                      /* DELETED */
        LINESKP();
    }
RDLOOP:
    if (STORAGE_read(rec, sizeof rec)) {
        NOSTG = ON;
    } else {
        advars_unpack(rec);
        NOSTG = OFF;               /* see the note in PUTBACK */
    }
    if (NOSTG) goto LOOPND;
    if (pl_eq(USERID, 8, LIT(" "))) {
        MSPEAK(44);                                 /* EMPTY RECORD */
        goto RDLOOP;
    }
    pl_assign(PW, 4, USERID, 4);
    TEST_WORD = -TEST_WORD;
    pl_substr_assign(TSOID, 8, 1, 4, PW, 4);
    pl_assign(PW, 4, USERID + 4, 4);
    TEST_WORD = -TEST_WORD;
    {
        char tmp[4];
        memcpy(tmp, PW, 3);
        tmp[3] = USERID[7];
        pl_substr_assign(TSOID, 8, 5, 4, tmp, 4);
    }
    pl_assign(PW, 4, NAME, 4);
    TEST_WORD = -TEST_WORD;
    pl_substr_assign(TEMPNAME, 8, 1, 4, PW, 4);
    pl_assign(PW, 4, NAME + 4, 4);
    TEST_WORD = -TEST_WORD;
    pl_substr_assign(TEMPNAME, 8, 5, 4, PW, 4);
    DATE_PIC_SET(TODAY);           /* both write THROUGH the overlay */
    PIC_HHMM_SET(HHMM);            /* into DATE_STG -- see advent.h  */
    pl_substr_assign(PUTDATE, 10, 1, 4, DATE_STG, 4);
    pl_substr_assign(PUTDATE, 10, 6, 2, DATE_STG + 4, 2);
    pl_substr_assign(PUTDATE, 10, 9, 2, DATE_STG + 6, 2);
    pl_substr_assign(PUTHHMM, 5, 1, 2, DATE_STG + 8, 2);
    pl_substr_assign(PUTHHMM, 5, 4, 2, DATE_STG + 10, 2);
    {
        char buf[133];
        int p = 0;
#define CAT(s, n) do { memcpy(buf + p, (s), (size_t)(n)); p += (n); } while (0)
        CAT(TSOID, 8);
        CAT(TEMPNAME, 8);
        CAT(" suspend no.", 12);
        pl_edit_f(buf + p, 2, SUSPEND_N);   p += 2;
        CAT(" on ", 4);
        CAT(PUTDATE, 10);
        CAT(" at ", 4);
        CAT(PUTHHMM, 5);
        CAT(" at location ", 13);
        pl_edit_f(buf + p, 3, LOC);         p += 3;
        pl_vassign(&OUTSTR, buf, p);
        LINEOUT();

        p = 0;
        CAT("   after", 8);
        pl_edit_f(buf + p, 4, ELAPSED);     p += 4;
        CAT(" minutes and", 12);
        pl_edit_f(buf + p, 4, TURNS);       p += 4;
        CAT(" turns?", 7);
#undef CAT
        pl_vassign(&OUTSTR, buf, p);
    }
    if (DEMOGM) pl_vcat_char(&OUTSTR, '?');
    if (WIZARD) pl_vcat_char(&OUTSTR, '!');
    LINEOUT();
    GETIN(REPLY, JUNK1, JUNK2, JUNK3);
    L = VOCAB(REPLY, -1);
    if (L == 2027) goto RDLOOP;         /* DO NOT UPDATE DISK FILE */
    if (L == 57 || L == 2001) {
        STORAGE_close();
        return;
    }
    if (pl_eq(REPLY, 3, LIT("DEL")) || L == 1028 || L == 2012) {
        STORAGE_rewrite(ADVREC, sizeof ADVREC);
        MSPEAK(38);                                     /* DELETED */
        LINESKP();
        goto RDLOOP;
    }
    if (pl_eq(REPLY, 5, LIT("DEMO ")))    /* CANNOT BE SUSPENDED AGAIN */
        DEMOGM = ON;
    if (pl_eq(REPLY, 5, LIT("^DEMO")))              /* UNDO A "DEMO" */
        DEMOGM = OFF;
    if (L == 2008)                              /* RESET WIZARD FLAG */
        WIZARD = OFF;
    {
        int i;
        pl_blank(INSTR, sizeof INSTR);
        pl_blank(JUNK1, 5);
        pl_blank(JUNK2, 5);
        pl_blank(JUNK3, 5);
        pl_blank(REPLY, 5);
        pl_blank(WD1, 5);
        pl_blank(WD1X, 5);
        pl_blank(WD2, 5);
        pl_blank(WD2X, 5);
        for (i = 1; i <= 10; i++) TKWORD[i][0] = ' ';
        pl_vassign(&OUTSTR, INSTR, sizeof INSTR);
        advars_pack(rec);
        STORAGE_rewrite(rec, (int)sizeof rec);
    }
    MSPEAK(7);
    LINESKP();
    goto RDLOOP;
LOOPND:
    MSPEAK(45);                             /* END OF FILE REACHED */
    STORAGE_close();
    LINESKP();
    longjmp(pli_goto, JMP_DEALLOC);
}
