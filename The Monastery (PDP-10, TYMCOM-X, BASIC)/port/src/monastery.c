/* monastery.c -- "The Monastery" (working title)
 *
 * A line-by-line transliteration of SAVE.TBA, an unfinished text adventure
 * written in Tymshare TYMCOM-X BASIC for a PDP-10 and dated 8 January 1987.
 * The room and message text lives in TEXT.GME, dated the following day.
 * Both files were recovered from the "novafield" Tymshare tape.
 *
 * The structure below follows the original exactly: every DEFINE becomes a
 * function, every GOTO becomes a goto, and the original line numbers are
 * kept as labels and comments so the two can be read side by side.  BASIC
 * has no locals, so every variable here is a file-scope global -- the
 * program depends on that in several places (subroutines communicate by
 * clobbering I, N, E, Z and friends).
 *
 * Quirks of the original that are deliberately preserved are marked
 * "[original bug]".  See ../docs/NOTES.md.
 */

#include "basic.h"
#include "gamedata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* dimensions, from lines 100-280                                      */
/* ------------------------------------------------------------------ */

#define MAXWORD 35              /* DIM WORD$(35)   */
#define MAXTEXT 400             /* DIM TEXT$(400)  */

/* ------------------------------------------------------------------ */
/* scalars                                                             */
/* ------------------------------------------------------------------ */

static int A1, AA, ANUM, ADVERBN, BHAND, CNUM, COMMFLAG, COMMLOC;
static int DOORNUM, E, E1, ER, FACE, HAND, I, J, L, LINEN, M, MF;
static int MODE, N, N1, NNUM, P, PNUM, PP, PTH, ROND, S, TB, TURN;
static int VNUM, WHAND, WNUM, Z, AC;

/* Floating point, as every BASIC variable was.  A and TH pick up
 * fractions from the two unguarded divisions; DAM from KILL.SUB. */
static double A, TH, DAM;

static char A1S[256];           /* A1$ */
static char IS[8];              /* I$  */
static char FS[64];             /* F$  */

/* ------------------------------------------------------------------ */
/* arrays -- 1-based, slot 0 present because BASIC DIMs 0..n and the    */
/* program does reach subscript 0 (e.g. NOUN$(PRS(5)) with PRS(5)=0)    */
/* ------------------------------------------------------------------ */

static int MOV[51][11];
static int LOCS[51][11];
static int DOORS[19][7];
static int NOUN[21][25];
static int NHITSII[21];
static int DFLAG[101];
static int PRS[16], PRSBAK[16];
static int CHARACTER[83];
static int VCODE[67];

static char WORD[MAXWORD + 1][64];
static char CHARACS[7][64];                     /* CHARAC$(6) */

static const char *NOUNS[21], *NOUN2S[21];
static const char *VERBS[67], *DIRS[11], *ADJECTS[45], *PREPS[8], *ADVERBS[6];

static char *TEXTS[MAXTEXT + 2];
static int   ITEXT[MAXTEXT + 2];

/* ------------------------------------------------------------------ */
/* port options (see README)                                           */
/* ------------------------------------------------------------------ */

static int opt_strict = 0;      /* --strict: QUIT is inert, as in 1987 */
static const char *opt_char = NULL;
static const char *opt_text = NULL;

/* ------------------------------------------------------------------ */
/* forward declarations, in source order                               */
/* ------------------------------------------------------------------ */

static void PARSER_SUB(void);
static void READ_DESC(void);
static void MOV_SUB(void);
static void PRINTER_SUB(void);
static void STARTUP(void);
static void DOOR_SUB(void);
static void DCLOSE(void);
static void CHECK_EM(void);
static void CHECK_EMI(void);
static void OBJECT_PRINT(void);
static void GET_SUB(void);
static void DROP_SUB(void);
static void INVENTORY_SUB(void);
static void SEARCH_SUB(void);
static void SEARCH_DOOR1(void);
static void SEARCH_DOOR2(void);
static void CHECK_ADVERB(void);
static void LOOK_SUB(void);
static void MONSTER_SUB(void);
static void UNDEAD_MOV(void);
static void DFLAG_PRINT(void);
static void ADDPLAYER(void);
static void OPEN_SUB(void);
static void CLOSE_SUB(void);
static void PUT_SUB(void);
static void PUT_COMPLETE(void);
static void PRINT_NOUN(void);
static void WEAR_SUB(void);
static void CHECK_BP(void);
static void MOVE_SUB(void);
static void CPARSER_SUB(void);
static void KILL_SUB(void);
static void CPUMMEL_SUB(void);
static void CGRAPPL_SUB(void);
static void CROLL_SUB(void);
static void PUT_HAND(void);
static void ENGAGE_SUB(void);
static void DISENGAGE_SUB(void);
static void VALUE_MANAGE(void);
static void MKILL_SUB(void);
static void MPUMMEL_SUB(void);
static void MGRAPPL_SUB(void);
static void MROLL_SUB(void);
static void DISPLAY(void);
static void HOBBIT_SUB(void);
static void SEARCH_OBJ(void);
static void LIGHT_MODE(void);
static void LIGHT_IT(void);
static void ENTER_SUB(void);

static void roll_up_character(const char *name);

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

#define EQ(a, b) (strcmp((a), (b)) == 0)
#define NE(a, b) (strcmp((a), (b)) != 0)

static char *dupstr(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) { fprintf(stderr, "monastery: out of memory\n"); exit(1); }
    memcpy(p, s, n);
    return p;
}

/* [port] Room 46 leads NORTH and NORTHWEST to room 52, which is past the
 * end of a 50-room map -- the author was plainly planning more ground.
 * The 1987 interpreter would have stopped with a subscript error the
 * moment a player (or a wandering monster) took that exit; here those
 * two exits are simply walls. */
static int room_ok(int r)
{
    return r >= 1 && r <= 50;
}

/* INT(RND(x)*n) */
static int RNDI(int n)
{
    return (int)(bas_rnd() * (double)n);
}

/* ================================================================== */
/* 580  DEFINE PARSER.SUB                                              */
/* ================================================================== */

static void PARSER_SUB(void)
{
L640:
    AA = 1;
    for (I = 0; I <= MAXWORD; I++)              /* 650 MAT WORD$=" " */
        strcpy(WORD[I], " ");
L660:
    bas_cib();                                  /* 660 CIB */
    Ps("]");                                    /* 670 */
L680:
    IS[0] = (char)bas_getch();                  /* 680 INPUT IN FORM "X" */
    IS[1] = '\0';
    /* 700 */
    if (IS[0] == 13 && (int)strlen(A1S) < 1 && AA == 1)
        goto L660;
    if (EQ(IS, ","))    COMMLOC = AA;           /* 710 */
    if (EQ(IS, ","))    strcpy(IS, " ");        /* 720 */
    /* 730 */
    if (IS[0] != 13 && NE(IS, " ") && NE(IS, ","))
        strncat(A1S, IS, sizeof A1S - strlen(A1S) - 1);
    if (IS[0] == 13)    goto L770;              /* 740 */
    if (EQ(IS, " "))    goto L770;              /* 750 */
    goto L680;                                  /* 760 */
L770:
    if (AA >= 0 && AA <= MAXWORD)               /* 770 WORD$(AA)=A1$ */
        snprintf(WORD[AA], sizeof WORD[0], "%s", A1S);
    A1S[0] = '\0';                              /* 780 */
    /* 790 */
    if (AA >= 0 && AA <= MAXWORD &&
        (EQ(WORD[AA], "THE") || EQ(WORD[AA], "GO") || EQ(WORD[AA], "A")))
        AA = AA - 1;
    AA = AA + 1;                                /* 800 */
    if (AA > MAXWORD) AA = MAXWORD;             /* [port] keep AA in range */
    if (IS[0] == 13)    goto L830;              /* 810 */
    goto L680;                                  /* 820 */

L830:                                           /* 830 REM DONE */
    memcpy(PRSBAK, PRS, sizeof PRS);            /* 840 */
    memset(PRS, 0, sizeof PRS);                 /* 850 */
    WNUM = 1;                                   /* 860 */
    if (COMMFLAG == 1) {                        /* 870 */
        COMMFLAG = 0; WNUM = COMMLOC + 1; COMMLOC = 0;
        goto L1160;
    }
    if (EQ(WORD[1], "TAKE") && EQ(WORD[2], "INVENTORY")) {   /* 910 */
        INVENTORY_SUB();
        goto L2210;
    }
    if (EQ(WORD[1], "PICK") && EQ(WORD[2], "UP")) {          /* 950 */
        WNUM = 3; PRS[1] = 27;
        goto L1240;
    }
    if (EQ(WORD[1], "PUT") && EQ(WORD[2], "DOWN")) {         /* 1000 */
        WNUM = 3; PRS[1] = 28;
        goto L1240;
    }
    CHECK_ADVERB();                                          /* 1050 */
    if (PRS[10] > 0 && EQ(WORD[2], "PICK") && EQ(WORD[3], "UP")) {   /* 1060 */
        WNUM = 4; PRS[1] = 27;
        goto L1240;
    }
    if (PRS[10] > 0 && EQ(WORD[2], "PUT") && EQ(WORD[3], "DOWN")) {  /* 1110 */
        WNUM = 4; PRS[1] = 28;
        goto L1240;
    }
L1160:                                          /* SEARCH WORD LISTS */
    for (I = 1; I <= VNUM; I++) {
        if (EQ(WORD[WNUM], VERBS[I])) {
            PRS[1] = I; WNUM = WNUM + 1;
            goto L1240;
        }
    }
L1240:
    for (I = 1; I <= PNUM; I++) {
        if (EQ(WORD[WNUM], PREPS[I])) {
            PRS[2] = I; WNUM = WNUM + 1;
            goto L1310;
        }
    }
L1310:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[3] = I; WNUM = WNUM + 1;
            goto L1380;
        }
    }
L1380:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[4] = I; WNUM = WNUM + 1;
            goto L1450;
        }
    }
L1450:
    for (I = 1; I <= NNUM; I++) {
        if (EQ(WORD[WNUM], "IT")) {             /* 1460 */
            PRS[5] = PRSBAK[5]; PRS[3] = PRSBAK[3]; PRS[4] = PRSBAK[4];
        }
        if (EQ(WORD[WNUM], "IT")) WNUM = WNUM + 1;             /* 1480 */
        if (EQ(WORD[WNUM], NOUNS[I]) || EQ(WORD[WNUM], NOUN2S[I])) {
            PRS[5] = I; WNUM = WNUM + 1;
            goto L1550;
        }
    }
L1550:
    for (I = 1; I <= PNUM; I++) {
        if (EQ(WORD[WNUM], PREPS[I])) {
            WNUM = WNUM + 1; PRS[6] = I;
            goto L1620;
        }
    }
L1620:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[7] = I; WNUM = WNUM + 1;
            goto L1690;
        }
    }
L1690:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[8] = I; WNUM = WNUM + 1;
            goto L1760;
        }
    }
L1760:
    for (I = 1; I <= NNUM; I++) {
        if (EQ(WORD[WNUM], "IT")) {             /* 1770 */
            PRS[9] = PRSBAK[5]; PRS[7] = PRSBAK[3]; PRS[8] = PRSBAK[4];
        }
        if (EQ(WORD[WNUM], "IT")) WNUM = WNUM + 1;             /* 1790 */
        if (EQ(WORD[WNUM], NOUNS[I]) || EQ(WORD[WNUM], NOUN2S[I])) {
            PRS[9] = I; WNUM = WNUM + 1;
            goto L1860;
        }
    }
L1860:
    if (PRS[1] < 1) goto L2210;                 /* 1870 */

    /* ---- execute verbs, 1880 onwards ---- */
    PP = P;                                     /* 1910 */
    if (VCODE[PRS[1]] > 0 && VCODE[PRS[1]] < 11) MOV_SUB();      /* 1930 */
    if (VCODE[PRS[1]] > 0 && VCODE[PRS[1]] < 11) ROND = ROND + 6;/* 1932 */
    if (VCODE[PRS[1]] > 0 && VCODE[PRS[1]] < 11) goto L2210;     /* 1940 */
    if (VCODE[PRS[1]] == 27) CLOSE_SUB();       /* 1950 */
    if (VCODE[PRS[1]] == 26) OPEN_SUB();        /* 1960 */
    if (VCODE[PRS[1]] == 28) LOOK_SUB();        /* 1970 */
    if (VCODE[PRS[1]] == 17) INVENTORY_SUB();   /* 1980 */
    /* search stuff */
    if (VCODE[PRS[1]] == 29 && PRS[5] != 8 && PRS[5] != 12) SEARCH_SUB();  /* 2000 */
    if (VCODE[PRS[1]] == 29 && PRS[9] == 12 && PRS[5] == 8) SEARCH_DOOR1();/* 2010 */
    if (VCODE[PRS[1]] == 29 && PRS[5] == 8 && PRS[9] < 1) PL("WHICH DOOR?");
    if (VCODE[PRS[1]] == 29 && PRS[5] == 12 && PRS[9] < 1) PL("FOR WHAT?");
    if (VCODE[PRS[1]] == 29 && PRS[5] == 12 && PRS[9] == 8) SEARCH_DOOR2();
    /* [port] QUIT (VCODE 21) was never wired up in 1987; --strict restores that */
    if (!opt_strict && VCODE[PRS[1]] == 21) bas_end("SORRY CHUCK.");
    /* noun modifiers */
    if (PRS[1] < 1) goto L2160;                 /* 2060 */
    if (VCODE[PRS[1]] == 15) GET_SUB();         /* 2070 */
    if (VCODE[PRS[1]] == 39) ENTER_SUB();       /* 2075 */
    if (VCODE[PRS[1]] == 11) ENGAGE_SUB();      /* 2080 */
    if (VCODE[PRS[1]] == 16) DROP_SUB();        /* 2090 */
    if (VCODE[PRS[1]] == 33) ENGAGE_SUB();      /* 2100 */
    if (VCODE[PRS[1]] == 30) PUT_SUB();         /* 2110 */
    if (VCODE[PRS[1]] == 32) WEAR_SUB();        /* 2120 */
    if (VCODE[PRS[1]] == 31) MOVE_SUB();        /* 2130 */
    if (VCODE[PRS[1]] == 38) DISPLAY();         /* 2140 */
    if (VCODE[PRS[1]] == 27 || VCODE[PRS[1]] == 26 || VCODE[PRS[1]] == 28)
        ROND = ROND + 4;                        /* 2142 */
    A = VCODE[PRS[1]];                          /* 2144 */
    if (A == 17 || A == 29 || A == 11 || A == 33) ROND = ROND + 4;  /* 2146 */
    if (A == 15 || A == 16 || A == 30) ROND = ROND + 4;             /* 2148 */
    if (A == 31) ROND = ROND + 10;              /* 2150 */
L2160:
    NL();                                       /* 2170 PRINT */
    if (PRS[1] < 1)                             /* 2180 */
        PL("YOU NEED TO SPECIFY A VERB.");
L2210:
    if (COMMLOC != 0) {                         /* 2210 */
        COMMFLAG = 1;
        goto L830;
    }
    if (PRS[1] < 1) {                           /* 2250 */
        PL("I DON'T UNDERSTAND THAT.");
        goto L640;
    }
}                                               /* 2290 ENDF PARSER.SUB */

/* ================================================================== */
/* 2310 DEFINE READ.DESC                                               */
/* ================================================================== */

/* Reads TEXT.GME.  A line shorter than four characters is an entry
 * number; anything else is a body line belonging to the last number. */
static void READ_DESC(void)
{
    FILE *fp = NULL;
    char buf[512];
    int idx = 0;

    LINEN = 1; P = 1;                           /* 2330 LINE=1,P=1 */

    if (opt_text)
        fp = fopen(opt_text, "rb");
    if (!fp)
        fp = fopen("TEXT.GME", "rb");
    if (!fp)
        fp = fopen("text.gme", "rb");

    for (;;) {
        const char *A_ = NULL;
        if (fp) {
            if (!fgets(buf, (int)sizeof buf, fp))
                break;                          /* ON ENDFILE(1) GOTO 2430 */
            {
                size_t n = strlen(buf);
                while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
                    buf[--n] = '\0';
            }
            A_ = buf;
        } else {
            if (idx >= DATA_TEXT_GME_LINES)
                break;
            A_ = DATA_TEXT_GME[idx++];
        }
        if ((int)strlen(A_) < 4) {              /* 2360 */
            P = atoi(A_);                       /* 2370 */
            continue;
        }
        if (LINEN <= MAXTEXT) {                 /* 2400 */
            TEXTS[LINEN] = dupstr(A_);
            ITEXT[LINEN] = P;
            LINEN = LINEN + 1;                  /* 2410 */
        }
    }
    if (fp)
        fclose(fp);
}                                               /* 2430 ENDF READ.DESC */

/* ================================================================== */
/* 2450 DEFINE MOV.SUB                                                 */
/* ================================================================== */

static void MOV_SUB(void)
{
    PP = P;                                     /* 2460 */
    P = MOV[P][VCODE[PRS[1]]];                  /* 2470 */
    if (P > 50) P = 0;                          /* [port] see room_ok */
    if (P < 1) {                                /* 2480 */
        if (P == 0) Z = 30;                     /* 2490 */
        if (P == 0) {                           /* 2500 */
            for (I = 1; I <= DOORNUM; I++) {
                if ((DOORS[I][1] == PP && DOORS[I][2] == VCODE[PRS[1]]) ||
                    (DOORS[I][3] == PP && DOORS[I][4] == VCODE[PRS[1]])) {
                    if (DOORS[I][5] == 2) goto L2550;   /* 2530 */
                    Z = 31;                             /* 2540 */
                }
            L2550: ;
            }
        }
        P = PP;                                 /* 2580 */
        PRINTER_SUB();                          /* 2590 */
    }
    /* 2610 special considerations, on wall, inviso, etc. */
    Z = LOCS[P][7];                             /* 2620 */
    if (LOCS[P][10] == 1) Z = LOCS[P][9];       /* 2630 */
    N = P;                                      /* 2632 */
    LIGHT_MODE();                               /* 2634 */
    if (L == 1) {                               /* 2636 */
        PL("IT'S TOO DARK TO SEE.");
        goto L2670;
    }
    if (P != PP) PRINTER_SUB();                 /* 2649 */
    LOCS[P][10] = 1;                            /* 2650 */
    if (P != PP) OBJECT_PRINT();                /* 2660 */
L2670: ;
}                                               /* 2670 ENDF MOV.SUB */

/* ================================================================== */
/* 2710 DEFINE PRINTER.SUB                                             */
/* ================================================================== */

static void PRINTER_SUB(void)
{
    for (J = 1; J <= LINEN; J++)                /* 2720 */
        if (Z == ITEXT[J])                      /* 2730 */
            PL(TEXTS[J] ? TEXTS[J] : "");
    Z = 0;                                      /* 2750 */
}

/* ================================================================== */
/* 2800 DEFINE STARTUP                                                 */
/* ================================================================== */

static void STARTUP(void)
{
    CNUM = 1;                                   /* 2810 */
    ADDPLAYER();                                /* 2820 */
    P = 1; PP = 1;                              /* 2830 */
    VALUE_MANAGE();                             /* 2840 */
    Z = 1;                                      /* 2850 */
    PRINTER_SUB();                              /* 2860 */
L2870:
    if (CHARACTER[67] != 0 && CHARACTER[67] != 4) {          /* 2870 */
        if (CHARACTER[67] == 1) PL("YOU ARE STUNNED.");
        if (CHARACTER[67] == 2) PL("YOU ARE UNCONSCIOUS.");
        if (CHARACTER[67] == 3) PL("YOU ARE PARALYZED.");
        if (CHARACTER[67] == 5) PL("YOU ARE HELPLESS.");
        CHARACTER[66] = CHARACTER[66] - 1;                   /* 2920 */
        if (CHARACTER[66] < 1) CHARACTER[66] = 0;            /* 2930 */
        if (CHARACTER[66] == 0) CHARACTER[67] = 0;           /* 2940 */
        goto L2990;
    }
    if (!(CHARACTER[69] != 0)) DFLAG_PRINT();   /* 2965 ... UNLESS ... */
    if (!(CHARACTER[69] != 0)) PARSER_SUB();    /* 2970 ... UNLESS ... */
    if (CHARACTER[69] != 0) CPARSER_SUB();      /* 2980 */
L2990:
    MONSTER_SUB();                              /* 2990 */
    VALUE_MANAGE();                             /* 3000 */
    goto L2870;                                 /* 3010 */
}

/* ================================================================== */
/* 3030 DEFINE DOOR.SUB                                                */
/* ================================================================== */

static void DOOR_SUB(void)
{
    if (PRS[3] == 34) S = 1;                    /* 3070 */
    if (PRS[4] == 34) S = 1;
    if (PRS[7] == 34) S = 1;
    if (PRS[8] == 34) S = 1;
    /* 3110 in case he says OPEN DOOR ON THE LEFT ... we must reverse */
    if (PRS[3] < 26 || PRS[3] > 33) {
        if (PRS[4] < 26 || PRS[4] > 33) {
            if (PRS[7] > 25 || PRS[7] < 34) {
                if (PRS[6] == 1) PRS[3] = PRS[7];
            }
            if (PRS[8] > 25 || PRS[8] < 34) {
                if (PRS[6] == 1) PRS[4] = PRS[8];
            }
        }
    }
    if (PRS[4] > 26 && PRS[4] < 34) goto L3270;              /* 3220 */
    if (PRS[3] < 27 || PRS[3] > 34) {                        /* 3230 */
        PL("YOU MUST SPECIFY WHICH DOOR YOU WISH TO OPEN");
        goto L3620;
    }
L3270:
    if (PRS[3] > 26 && PRS[3] < 34) {
        if (PRS[4] < 34 && PRS[4] > 26) {
            Ps("MAKE YOUR MIND? "); Ps(ADJECTS[PRS[4]]);
            Ps(" OR "); Ps(ADJECTS[PRS[3]]); PL("?");
            goto L3620;
        }
    }
    if (PRS[4] < 34 && PRS[4] > 26) I = PRS[4] - 26;          /* 3330 */
    if (PRS[3] < 34 && PRS[3] > 26) I = PRS[3] - 26;          /* 3340 */
    if (I == 2) FACE = 4;
    if (I == 5) FACE = 7;
    if (I == 6) FACE = 8;
    if (I == 7) FACE = 1;
    if (I == 3) FACE = 7;
    if (I == 4) FACE = 8;
    if (I == 1) FACE = 1;
    for (I = 1; I <= DOORNUM; I++) {                         /* 3420 */
        if ((DOORS[I][1] == P && DOORS[I][2] == FACE) ||
            (DOORS[I][3] == P && DOORS[I][4] == FACE)) {
            if (DOORS[I][5] == 2 && S != 1) {
                PL("THERE IS NO DOOR THERE TO OPEN.");
                goto L3620;
            }
            if (DOORS[I][5] == 1) {
                PL("THAT DOOR IS LOCKED.");
                goto L3590;
            }
            if (DOORS[I][6] == 1) PL("THE DOOR IS ALREADY OPEN.");
            if (DOORS[I][6] == 1) goto L3620;
            /* [original bug] door 1 is DATA 2,1,2,4 -- both sides name room
               2, so opening the north door of room 2 writes self-loops into
               MOV and cuts the room off.  Reproduced as written. */
            MOV[DOORS[I][1]][DOORS[I][2]] = DOORS[I][3];     /* 3540 */
            MOV[DOORS[I][3]][DOORS[I][4]] = DOORS[I][1];     /* 3550 */
            PL("OK. THE DOOR IS NOW OPEN.");
            DOORS[I][6] = 1;
            goto L3620;
        }
    L3590: ;
    }
    PL("I CAN'T SEE A DOOR THERE.");                         /* 3610 */
L3620:
    S = 0;
}

/* ================================================================== */
/* 3670 DEFINE DCLOSE                                                  */
/* ================================================================== */

static void DCLOSE(void)
{
    /* 3710 in case he says CLOSE DOOR ON LEFT ... reverse */
    if (PRS[3] < 26 || PRS[3] > 33) {
        if (PRS[4] < 26 || PRS[4] > 33) {
            if (PRS[7] > 25 || PRS[7] < 34) {
                if (PRS[6] == 1) PRS[3] = PRS[7];
            }
            if (PRS[8] > 25 || PRS[8] < 34) {
                if (PRS[6] == 1) PRS[4] = PRS[8];
            }
        }
    }
    if (PRS[4] > 26 && PRS[4] < 34) goto L3870;
    if (PRS[3] < 27 || PRS[3] > 34) {
        PL("YOU MUST SPECIFY WHICH DOOR YOU WISH TO CLOSE.");
        goto L4180;
    }
L3870:
    if (PRS[3] > 26 && PRS[3] < 34) {
        if (PRS[4] < 34 && PRS[4] > 26) {
            Ps("MAKE YOUR MIND? "); Ps(ADJECTS[PRS[4]]);
            Ps(" OR "); Ps(ADJECTS[PRS[3]]); PL("?");
            goto L4180;
        }
    }
    if (PRS[4] < 34 && PRS[4] > 26) I = PRS[4] - 26;
    if (PRS[3] < 34 && PRS[3] > 26) I = PRS[3] - 26;
    if (I == 2) FACE = 4;
    if (I == 5) FACE = 7;
    if (I == 6) FACE = 8;
    if (I == 7) FACE = 1;
    if (I == 3) FACE = 7;
    if (I == 4) FACE = 8;
    if (I == 1) FACE = 1;
    for (I = 1; I <= DOORNUM; I++) {
        if ((DOORS[I][1] == P && DOORS[I][2] == FACE) ||
            (DOORS[I][3] == P && DOORS[I][4] == FACE)) {
            if (DOORS[I][5] == 1) {
                PL("THAT DOOR IS LOCKED.");
                goto L4180;
            }
            if (DOORS[I][6] == 0) PL("THE DOOR IS ALREADY CLOSED.");
            if (DOORS[I][6] == 0) goto L4180;
            MOV[DOORS[I][1]][DOORS[I][2]] = 0;
            MOV[DOORS[I][3]][DOORS[I][4]] = 0;
            PL("OK. THE DOOR IS NOW CLOSED.");
            DOORS[I][6] = 0;
            goto L4180;
        }
    }
    PL("I CAN'T SEE A DOOR THERE.");
L4180: ;
}

/* ================================================================== */
/* 4190 DEFINE CHECK.EM   -- resolve PRS(5) against adjectives + place  */
/* ================================================================== */

static void CHECK_EM(void)
{
    ER = 0;                                     /* 4200 */
    for (I = 1; I <= NNUM; I++) {               /* 4220 */
        if (NE(NOUNS[I], NOUNS[PRS[5]])) goto L4380;
        if ((PRS[3] > 0 && NOUN[I][6] != PRS[3] && NOUN[I][7] != PRS[3]) ||
            (PRS[4] > 0 && NOUN[I][6] != PRS[4] && NOUN[I][7] != PRS[4])) {
            ER = 1;
            goto L4380;
        }
        if (PRS[3] > 0 && PRS[4] > 0 && PRS[3] == PRS[4]) {
            ER = 2;
            goto L4380;
        }
        /* 4320 check conditions */
        if (E == NOUN[I][1]) goto L4360;
        if (E1 != 0 && E1 == NOUN[I][1]) goto L4360;
        goto L4380;
    L4360:
        PRS[5] = I;
        goto L4410;
    L4380: ;
    }
    if (ER == 1) PL("YOU USED THE WRONG ADJECTIVE(S) FOR A PRESENT SUITABLE OBJECT.");
    if (ER == 1) PRS[5] = 0;
L4410:
    E = 0; E1 = 0;
}

/* ================================================================== */
/* 4430 DEFINE CHECK.EMI  -- same, for the indirect noun PRS(9)         */
/* ================================================================== */

static void CHECK_EMI(void)
{
    for (I = 1; I <= NNUM; I++) {               /* 4450 */
        if ((PRS[7] > 0 && NOUN[I][6] != PRS[7] && NOUN[I][7] != PRS[7]) ||
            (PRS[8] > 0 && NOUN[I][6] != PRS[8] && NOUN[I][7] != PRS[8])) {
            ER = 1;
            goto L4610;
        }
        if (PRS[7] > 0 && PRS[8] > 0 && PRS[7] == PRS[8]) {
            ER = 2;
            goto L4610;
        }
        if (NE(NOUNS[I], NOUNS[PRS[9]]) && NE(NOUN2S[I], NOUN2S[PRS[9]]))
            goto L4610;
        ER = 0;
        if (E != NOUN[I][1]) goto L4610;
        if (E1 != 0 && E1 != NOUN[I][1]) goto L4610;
        PRS[9] = I;
        goto L4620;
    L4610: ;
    }
L4620: ;
}

/* ================================================================== */
/* 4630 DEFINE OBJECT.PRINT                                            */
/* ================================================================== */

static void OBJECT_PRINT(void)
{
    for (I = 1; I <= NNUM; I++) {               /* 4670 */
        if (NOUN[I][9] != 0) goto L4860;        /* 4690 */
        if (NOUN[I][3] != 0) goto L4860;        /* 4700 */
        if (NOUN[I][18] == 1) goto L4770;       /* 4705 */
        if (NOUN[I][1] == P) {                  /* 4710 */
            Z = NOUN[I][17];
            if (P == 4 && I == 7 && DFLAG[1] != 1) goto L4860;   /* 4730 */
            if (P == 4 && I == 7 && DFLAG[1] == 1) Z = 9;        /* 4740 book */
            PRINTER_SUB();
        }
    L4770:
        if (NOUN[I][3] == 0 && NOUN[I][18] == 1) {               /* 4770 */
            if (P != NOUN[I][1]) goto L4860;                     /* 4785 */
            N = I;
            Ps("THERE IS A ");
            PRINT_NOUN();
            PL(" HERE.");
        }
    L4860: ;
    }
}

/* ================================================================== */
/* 4880 DEFINE GET.SUB                                                 */
/* ================================================================== */

static void GET_SUB(void)
{
L4920:
    if (PRS[5] < 1) {                           /* 4920 */
        Z = 55;
        PRINTER_SUB();
        goto L5620;
    }
    E = P;                                      /* 4970 */
    E1 = -1;                                    /* 4980 */
    CHECK_EM();                                 /* 4990 */
    if (PRS[5] < 1) goto L5670;                 /* 5000 */
    /* 5010 if he's got it but it is not in his hands, try to put it there */
    if ((NOUN[PRS[5]][1] == P && NOUN[PRS[5]][9] != 0) ||
        (NOUN[PRS[5]][1] == -1 && NOUN[PRS[5]][9] != 0)) {       /* 5030 */
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L5620;
    }
    if (NOUN[PRS[5]][1] == -1 && CHARACTER[64] != PRS[5] &&
        CHARACTER[65] != PRS[5]) goto L5330;                     /* 5090 */
    if (NOUN[PRS[5]][1] == -1) {                                 /* 5100 */
        PL("YOU ALREADY HAVE THAT!");
        goto L5620;
    }
    if (NOUN[PRS[5]][1] != P && NOUN[PRS[5]][1] != -1) {         /* 5140 */
        PL("I DON'T SEE IT.");
        goto L5620;
    }
    if (NOUN[PRS[5]][2] != 1) {                                  /* 5180 */
        PL("I'M SORRY BUT YOU ARE UNABLE TO PICK THAT UP.");
        goto L5620;
    }
    if (CHARACTER[64] != 0 && CHARACTER[65] != 0) {              /* 5230 */
        PL("YOUR HANDS ARE FULL.");
        goto L5620;
    }
    if (P == 4 && PRS[5] == 7 && DFLAG[1] != 1) {                /* 5280 */
        Z = 9;
        PRINTER_SUB();
        DFLAG[1] = 0;
    }
L5330:
    NOUN[PRS[5]][1] = -1;                                        /* 5330 */
    if (CHARACTER[64] == 0) {                                    /* 5360 */
        CHARACTER[64] = PRS[5];
        goto L5410;
    }
    if (CHARACTER[65] == 0) CHARACTER[65] = PRS[5];              /* 5400 */
L5410:
    if (CHARACTER[64] == PRS[5]) NOUN[PRS[5]][9] = 1;            /* 5420 */
    if (CHARACTER[65] == PRS[5]) NOUN[PRS[5]][9] = 2;            /* 5430 */
    if (CHARACTER[64] == PRS[5]) {                               /* 5440 */
        N = PRS[5];
        Ps("OK, THE ");
        PRINT_NOUN();
        PL(" IS IN YOUR LEFT HAND.");
    }
    /* [original bug] line 5500 reads CHAR(65), a variable that is never
       assigned, so the right-hand message can never fire. */
    if (0 == PRS[5]) {                                           /* 5500 */
        N = PRS[5];
        Ps("THE ");
        PRINT_NOUN();
        PL(" IS IN YOUR RIGHT HAND.");
    }
    if (NOUN[PRS[5]][5] != 0) {                                  /* 5580 */
        E = -1; N = PRS[5];
        PUT_COMPLETE();
    }
L5620:
    if (PRS[6] == 7) {                                           /* 5620 GET x AND y */
        PRS[5] = PRS[9]; PRS[3] = PRS[7]; PRS[4] = PRS[8];
        PRS[6] = -1;
        goto L4920;
    }
L5670: ;
}

/* ================================================================== */
/* 5750 DEFINE DROP.SUB                                                */
/* ------------------------------------------------------------------ */
/* [original bug] lines 5700-5740 -- the PRS(5)<1 guard -- sit *above*  */
/* the DEFINE, so they are dead code and DROP never checks for a noun.  */
/* ================================================================== */

static void DROP_SUB(void)
{
    E = -1;                                     /* 5760 */
    CHECK_EM();                                 /* 5770 */
    if (NOUN[PRS[5]][1] == -1 && NOUN[PRS[5]][9] != 0) {         /* 5790 */
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L5980;
    }
    if (NOUN[PRS[5]][1] != -1) {                                 /* 5840 */
        PL("I'M SORRY BUT YOU ARE NOT CARRYING THAT.");
        goto L5980;
    }
    PL("OK.");                                                   /* 5880 */
    if (CHARACTER[64] == PRS[5]) CHARACTER[64] = 0;
    if (CHARACTER[65] == PRS[5]) CHARACTER[65] = 0;
    NOUN[PRS[5]][9] = 0;
    NOUN[PRS[5]][1] = P;
    if (NOUN[PRS[5]][5] != 0) {                                  /* 5940 */
        E = P; N = PRS[5];
        PUT_COMPLETE();
    }
L5980: ;
}

/* ================================================================== */
/* 5990 DEFINE INVENTORY.SUB                                           */
/* ================================================================== */

static void INVENTORY_SUB(void)
{
    NL();
    PL("YOU ARE CARRYING THE FOLLOWING:");
    NL();
    for (I = 1; I <= NNUM; I++) {               /* 6060 */
        if (NOUN[I][1] == -1) {
            if (NOUN[I][9] < 1) goto L6100;
            if (NOUN[NOUN[I][9]][5] == 2) goto L6200;
        L6100:
            N = I;
            PRINT_NOUN();
            if (NOUN[I][9] != 0) {
                if (NOUN[I][9] == 1 || NOUN[I][9] == 2 || NOUN[I][9] == 3)
                    goto L6170;
                Ps(" INSIDE THE ");
                N = NOUN[I][9];
                PRINT_NOUN();
            L6170:
                if (NOUN[I][5] == 2) PL(". IT IS CLOSED."); else PL(".");
            }
        }
    L6200: ;
    }
    NL();
    /* 6220 explain where everything is */
    if (CHARACTER[64] != 0) {
        N = CHARACTER[64];
        Ps("THE ");
        PRINT_NOUN();
        PL(" IS IN YOUR LEFT HAND.");
    }
    if (CHARACTER[65] != 0) {
        N = CHARACTER[65];
        Ps("THE ");
        PRINT_NOUN();
        PL(" IS IN YOUR RIGHT HAND.");
    }
    for (I = 1; I <= NNUM; I++) {
        if (NOUN[I][9] == 3) {
            N = I;
            Ps("YOU ARE WEARING THE ");
            PRINT_NOUN();
            PL(".");
        }
    }
}

/* ================================================================== */
/* 6440 DEFINE SEARCH.SUB                                              */
/* ================================================================== */

static void SEARCH_SUB(void)
{
    if (PRS[5] < 1) LOOK_SUB();                 /* 6480 */
    if (PRS[5] < 1) goto L6720;                 /* 6485 */
    if (PRS[5] == 11 && P != 4) {               /* 6500 search ashes */
        PL("ASHES? WHAT ASHES?");
        goto L6720;
    }
    if (DFLAG[1] == 1 && P == 4 && PRS[5] == 11) {
        Z = 49;
        PRINTER_SUB();
        goto L6720;
    }
    if (DFLAG[1] == 0 && P == 4 && PRS[5] == 11 && NOUN[11][1] == 4) {
        Z = 9;
        DFLAG[1] = 1;
        PRINTER_SUB();
        goto L6720;
    }
    E = P; E1 = -1;                             /* 6670 */
    CHECK_EM();
    if (NOUN[PRS[5]][1] == P || NOUN[PRS[5]][1] == -1) SEARCH_OBJ();
    if (NOUN[PRS[5]][1] == P || NOUN[PRS[5]][1] == -1) goto L6720;
    PL("YOU FIND NOTHING SPECIAL.");
L6720: ;
}

/* ================================================================== */
/* 6730 DEFINE SEARCH.DOOR1  -- "SEARCH FOR SECRET DOOR ON NORTH WALL" */
/* ================================================================== */

static void SEARCH_DOOR1(void)
{
    if (PRS[2] != 6 || PRS[6] != 1) {           /* 6770 */
        Z = 50;
        PRINTER_SUB();
        goto L7140;
    }
    if (PRS[3] != 34 && PRS[3] != 40 && PRS[3] != 41 && PRS[3] != 42 &&
        PRS[4] != 34 && PRS[4] != 40 && PRS[4] != 41 && PRS[4] != 42)
        S = 0;
    else
        S = 1;                                  /* 6820 */
    if (PRS[7] < 27 || PRS[7] > 33) {           /* 6830 */
        if (PRS[8] < 27 || PRS[8] > 33) {
            PL("YOU MUST SPECIFY WHICH WALL YOU ARE SEARCHING.");
            goto L7140;
        }
        if (PRS[3] > 26 && PRS[3] < 34 && PRS[4] > 26 && PRS[4] < 34) {
            Ps("MAKE UP YOUR MIND? "); Ps(ADJECTS[PRS[7]]);
            Ps(" OR "); Ps(ADJECTS[PRS[8]]); PL("?");
            goto L7140;
        }
    }
    if (PRS[7] > 26) I = PRS[7] - 26;
    if (PRS[8] > 26) I = PRS[8] - 26;
    if (I == 1) FACE = 1;
    if (I == 2) FACE = 4;
    if (I == 3) FACE = 7;
    if (I == 4) FACE = 8;
    if (I == 5) FACE = 7;
    if (I == 6) FACE = 8;
    if (I == 7) FACE = 1;
    for (I = 1; I <= DOORNUM; I++) {
        if ((DOORS[I][1] == P && DOORS[I][2] == FACE) ||
            (DOORS[I][3] == P && DOORS[I][4] == FACE)) {
            if (DOORS[I][5] == 2 && S == 0) goto L7100;
            if (DOORS[I][5] == 2) Z = 51;
            if (DOORS[I][5] != 2) Z = 52;
            PRINTER_SUB();
            goto L7140;
        }
    L7100: ;
    }
    Z = 54;
    if (S == 1) Z = 53;
    PRINTER_SUB();
L7140: ;
}

/* ================================================================== */
/* 7150 DEFINE SEARCH.DOOR2  -- "SEARCH NORTH WALL FOR SECRET DOOR"    */
/* ================================================================== */

static void SEARCH_DOOR2(void)
{
    if (PRS[6] != 6) {                          /* 7190 */
        Z = 50;
        PRINTER_SUB();
        goto L7560;
    }
    if (PRS[7] != 34 && PRS[7] != 40 && PRS[7] != 41 && PRS[7] != 42 &&
        PRS[8] != 34 && PRS[8] != 40 && PRS[8] != 41 && PRS[8] != 42)
        S = 0;
    else
        S = 1;
    if (PRS[3] < 27 || PRS[3] > 33) {
        if (PRS[4] < 27 || PRS[4] > 33) {
            PL("YOU MUST SPECIFY WHICH WALL YOU ARE SEARCHING.");
            goto L7560;
        }
        if (PRS[7] > 26 && PRS[7] < 34 && PRS[8] > 26 && PRS[8] < 34) {
            Ps("MAKE UP YOUR MIND? "); Ps(ADJECTS[PRS[3]]);
            Ps(" OR "); Ps(ADJECTS[PRS[8]]); PL("?");
            goto L7560;
        }
    }
    if (PRS[3] > 26) I = PRS[3] - 26;
    if (PRS[4] > 26) I = PRS[4] - 26;
    if (I == 1) FACE = 1;
    if (I == 2) FACE = 4;
    if (I == 3) FACE = 7;
    if (I == 4) FACE = 8;
    if (I == 5) FACE = 7;
    if (I == 6) FACE = 8;
    if (I == 7) FACE = 1;
    for (I = 1; I <= DOORNUM; I++) {
        if ((DOORS[I][1] == P && DOORS[I][2] == FACE) ||
            (DOORS[I][3] == P && DOORS[I][4] == FACE)) {
            if (DOORS[I][5] == 2 && S == 0) goto L7520;
            if (DOORS[I][5] == 2) Z = 51;
            if (DOORS[I][5] != 2) Z = 52;
            PRINTER_SUB();
            goto L7560;
        }
    L7520: ;
    }
    Z = 54;
    if (S == 1) Z = 53;
    PRINTER_SUB();
L7560: ;
}

/* ================================================================== */
/* 7570 DEFINE CHECK.ADVERB                                            */
/* ================================================================== */

static void CHECK_ADVERB(void)
{
    for (I = 1; I <= ADVERBN; I++) {
        if (EQ(WORD[WNUM], ADVERBS[I])) {
            PRS[10] = I;
            WNUM = WNUM + 1;
            goto L7680;
        }
    }
L7680: ;
}

/* ================================================================== */
/* 7690 DEFINE LOOK.SUB                                                */
/* ================================================================== */

static void LOOK_SUB(void)
{
    N = P;                                      /* 7722 */
    LIGHT_MODE();
    if (L == 1) {
        PL("IT'S TOO DARK TO SEE.");
        goto L7800;
    }
    Z = LOCS[P][7];                             /* 7750 */
    PRINTER_SUB();
    OBJECT_PRINT();
L7800: ;
}

/* ================================================================== */
/* 7810 DEFINE MONSTER.SUB                                             */
/* ================================================================== */

static void MONSTER_SUB(void)
{
    for (M = 1; M <= NNUM; M++) {               /* 7820 */
        if (NOUN[M][3] == 0) goto L8100;        /* 7830 */
        if (NOUN[M][19] > 0 && NOUN[M][19] < 6 && NOUN[M][1] == P) {
            Ps("THE ");
            N = M;
            PRINT_NOUN();
            if (NOUN[M][19] == 1) PL(" IS STUNNED.");
            if (NOUN[M][19] == 2) PL(" IS UNCONSCIOUS.");
        }
        if (NOUN[M][18] > 0) {                  /* 7910 */
            NOUN[M][18] = NOUN[M][18] - 1;
            if (NOUN[M][18] == 0 && NOUN[M][19] != 0 && NOUN[M][1] == P) {
                Ps("THE ");
                N = M;
                PRINT_NOUN();
                PL(" IS COMING AROUND.");
            }
            goto L8100;
        }
        NOUN[M][19] = 0;                        /* 8020 */
        if (NOUN[M][1] < 1) goto L8100;
        if (NOUN[M][3] == 0) goto L8100;
        if (NOUN[M][14] == 6 || NOUN[M][14] == 7) UNDEAD_MOV();
        if (NOUN[M][14] == 9) HOBBIT_SUB();
    L8100: ;
    }
}

/* ================================================================== */
/* 8120 DEFINE UNDEAD.MOV                                              */
/* ================================================================== */

static void UNDEAD_MOV(void)
{
    if (NOUN[M][20] != 0) goto L8180;           /* 8140 */
    if (NOUN[M][1] == P) {                      /* 8150 */
        Z = NOUN[M][17];
        PRINTER_SUB();
    L8180:
        Ps("THE ");
        N = M;
        PRINT_NOUN();
        PL(" ATTACKS YOU!");
        CHARACTER[69] = M;
        if (CHARACTER[67] != 0) MODE = 1;       /* attack to kill */
        if (MODE == 1) MKILL_SUB();
        if (MODE == 2) MGRAPPL_SUB();
        if (MODE == 3) MPUMMEL_SUB();
        if (MODE < 1 || MODE > 3) MPUMMEL_SUB();
        goto L8640;
    }
    /* 8320 move the monster; first decide direction */
    A = RNDI(9) + 1;                            /* 8340 */
    for (I = 1; I <= 10; I++) {
        if (MOV[NOUN[M][1]][I] == P) {
            A = I;
            goto L8410;
        }
    }
L8410:                                          /* 8410 check for a nearby door */
    for (I = 1; I <= DOORNUM; I++) {
        if ((DOORS[I][1] == NOUN[M][1] && DOORS[I][3] == P && DOORS[I][6] == 0) ||
            (DOORS[I][3] == NOUN[M][1] && DOORS[I][1] == P && DOORS[I][6] == 0)) {
            if (DOORS[I][1] == NOUN[M][1] && DOORS[I][3] == P && DOORS[I][6] == 0)
                Z = DOORS[I][2];
            if (DOORS[I][3] == NOUN[M][1] && DOORS[I][1] == P && DOORS[I][6] == 0)
                Z = DOORS[I][4];
            if (Z == 1) J = 4;
            if (Z == 4) J = 1;
            if (Z == 8) J = 7;
            if (Z == 7) J = 8;
            Ps("YOU HEAR A LOUD BANGING ON THE DOOR ON THE ");
            Ps(DIRS[J >= 0 && J <= 10 ? J : 0]);
            PL(" WALL!");
            goto L8640;
        }
    }
    A1 = MOV[NOUN[M][1]][(int)A];               /* 8540 */
    if (A1 < 1 || !room_ok(A1)) goto L8640;     /* [port] see room_ok */
    NOUN[M][1] = A1;
    if (P == A1) {
        Ps("A ");
        N = M;
        PRINT_NOUN();
        if (P == PP) PL(" IS APPROACHING!");
        if (P != PP) PL(" FOLLOWS YOU!");
    }
L8640: ;
}

/* ================================================================== */
/* 8650 DEFINE DFLAG.PRINT                                             */
/* ================================================================== */

static void DFLAG_PRINT(void)
{
    if (P == 22 && DFLAG[2] == 1) {
        PL("THE STATUE ON THE SOUTHERN WALL HAS BEEN");
        PL("MOVED ASIDE, EXPOSING A SOUTHERN EXIT.");
    }
}

/* ================================================================== */
/* 8870 DEFINE ADDPLAYER                                               */
/* ================================================================== */

/* The original opened F$+".GME-A"; the "-A" is a TYMCOM-X file mode, not
 * part of the name (READ.DESC opens "TEXT.GME-A" for the file that is
 * plain TEXT.GME on the tape).  No character file and no generator for
 * this format survived, so the port offers to roll one up -- see README. */
static void ADDPLAYER(void)
{
    char path[128];
    FILE *fp;
    int i;

    if (opt_char) {
        snprintf(FS, sizeof FS, "%s", opt_char);
    } else {
        NL();
        Ps("ENTER THE NAME OF THE CHARACTER YOU WISH TO ENTER:");
        bas_input_line(FS, sizeof FS);
    }
    if ((int)strlen(FS) > 5) FS[6] = '\0';      /* 8950 LEFT(F$,6) */
    if (FS[0] == '\0') snprintf(FS, sizeof FS, "PLAYER");

    snprintf(path, sizeof path, "%s.GME", FS);
    fp = fopen(path, "rb");
    if (!fp) {
        if (opt_char) {
            fprintf(stderr, "monastery: cannot open %s\n", path);
            exit(1);
        }
        roll_up_character(FS);
        fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "monastery: cannot open %s\n", path);
            exit(1);
        }
    }

    for (i = 1; i <= 6; i++) {                  /* 8980-9000 */
        char buf[128];
        size_t n;
        if (!fgets(buf, (int)sizeof buf, fp)) { CHARACS[i][0] = '\0'; continue; }
        n = strlen(buf);
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
            buf[--n] = '\0';
        snprintf(CHARACS[i], sizeof CHARACS[0], "%s", buf);
    }
    for (i = 1; i <= 82; i++) {                 /* 9010 IN FORM "DDDDD" */
        char buf[64];
        if (!fgets(buf, (int)sizeof buf, fp)) { CHARACTER[i] = 0; continue; }
        CHARACTER[i] = atoi(buf);
    }
    fclose(fp);

    /* [port] CHARACTER(68) indexes CHARACTER() as the "good hand"; a
       corrupt file would take the original off the end of the array. */
    if (CHARACTER[68] != 64 && CHARACTER[68] != 65)
        CHARACTER[68] = 65;
}

/* ================================================================== */
/* 9040 DEFINE OPEN.SUB                                                */
/* ================================================================== */

static void OPEN_SUB(void)
{
    if (PRS[5] < 1) {                           /* 9080 */
        Z = 55;
        PRINTER_SUB();
        goto L9390;
    }
    if (PRS[5] == 8) DOOR_SUB();                /* 9130 */
    if (PRS[5] == 8) goto L9390;
    E = P;
    E1 = -1;
    CHECK_EM();
    if ((NOUN[PRS[5]][1] == P && NOUN[PRS[5]][9] != 0) ||
        (NOUN[PRS[5]][1] == -1 && NOUN[PRS[5]][9] != 0)) {
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L9390;
    }
    if (NOUN[PRS[5]][1] != P && NOUN[PRS[5]][1] != -1) {
        PL("I DON'T SEE IT HERE.");
        goto L9390;
    }
    if (NOUN[PRS[5]][3] != 0) goto L9390;
    if (NOUN[PRS[5]][5] == 0) {
        PL("THAT CANNOT BE OPENED.");
        goto L9390;
    }
    if (NOUN[PRS[5]][5] == 1) {
        PL("IT IS ALREADY OPEN.");
        goto L9390;
    }
    PL("OK. IT IS NOW OPEN.");
    NOUN[PRS[5]][5] = 1;
L9390: ;
}

/* ================================================================== */
/* 9410 DEFINE CLOSE.SUB                                               */
/* ================================================================== */

static void CLOSE_SUB(void)
{
    if (PRS[5] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L9730;
    }
    if (NOUN[PRS[5]][3] != 0) goto L9730;
    if (PRS[5] == 8) DCLOSE();
    if (PRS[5] == 8) goto L9730;
    E = P;
    E1 = -1;
    CHECK_EM();
    if ((NOUN[PRS[5]][1] == P && NOUN[PRS[5]][9] != 0) ||
        (NOUN[PRS[5]][1] == -1 && NOUN[PRS[5]][9] != 0)) {
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L9730;
    }
    if (NOUN[PRS[5]][1] != P && NOUN[PRS[5]][1] != -1) {
        PL("I DON'T SEE IT HERE.");
        goto L9730;
    }
    if (NOUN[PRS[5]][5] == 2) {
        PL("IT IS CLOSED.");
        goto L9730;
    }
    if (NOUN[PRS[5]][5] != 1) {
        PL("THAT IS NOT OPEN.");
        goto L9730;
    }
    PL("OK, IT IS CLOSED.");
    NOUN[PRS[5]][5] = 2;
L9730: ;
}

/* ================================================================== */
/* 9740 DEFINE PUT.SUB                                                 */
/* ================================================================== */

static void PUT_SUB(void)
{
    if (EQ(WORD[3], "DOWN") || EQ(WORD[4], "DOWN") || EQ(WORD[5], "DOWN")) {
        DROP_SUB();
        goto L10920;
    }
    if (PRS[5] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L10920;
    }
    if (PRS[6] == 1) goto L10830;               /* 9900 */
    E = -1;                                     /* 9940 */
    CHECK_EM();
    if (NOUN[PRS[5]][1] == -1 && NOUN[PRS[5]][9] != 0) {
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L10920;
    }
    if (PRS[6] != 1 && PRS[6] != 2 && PRS[6] != 3) {
        Z = 50;
        PRINTER_SUB();
        goto L10920;
    }
    if (NOUN[PRS[5]][1] != -1) {
        Ps("YOU DON'T HAVE THE ");
        N = PRS[5];
        PRINT_NOUN();
        PL(".");
        goto L10920;
    }
    if (PRS[9] < 1) {
        PL("YOU NEED TO SPECIFY AND INDIRECT NOUN.");
        goto L10920;
    }
    if (PRS[9] == 18) {                         /* 10180 put something in right hand */
        PUT_HAND();
        goto L10920;
    }
    E = P;
    CHECK_EMI();
    E = -1;
    CHECK_EMI();
    if ((NOUN[PRS[9]][1] == -1 && NOUN[PRS[9]][9] != 0) ||
        (NOUN[PRS[9]][1] == P && NOUN[PRS[9]][9] != 0)) {
        N = PRS[9];
        CHECK_BP();
        if (ER == 1) goto L10920;
    }
    if (PRS[5] == PRS[9]) {
        PL("THAT WILL NOT WORK.");
        goto L10920;
    }
    if (NOUN[PRS[9]][1] != P && NOUN[PRS[9]][1] != -1) {
        Ps("I DON'T SEE THE ");
        N = PRS[9];
        PRINT_NOUN();
        PL(".");
        goto L10920;
    }
    if (NOUN[PRS[9]][5] == 0) {
        PL("THAT IS NOT A CONTAINER!");
        goto L10920;
    }
    if (NOUN[PRS[9]][5] != 1) {
        Ps("THE ");
        N = PRS[9];
        PRINT_NOUN();
        PL(" IS NOT OPEN.");
        goto L10920;
    }
    A = 0;                                      /* 10550 how much weight already */
    for (I = 1; I <= NNUM; I++)
        if (NOUN[I][9] == PRS[9]) A = A + NOUN[I][13];
    if ((A + NOUN[PRS[5]][13]) > NOUN[PRS[9]][8]) {
        Ps("THE ");
        N = PRS[9];
        PRINT_NOUN();
        PL(" CAN HOLD NOTHING MORE.");
        goto L10920;
    }
    if (NOUN[PRS[5]][15] > NOUN[PRS[9]][15]) {  /* 10670 too bulky */
        Ps("THE "); Ps(NOUNS[PRS[5]]); PL(" WON'T FIT. IT'S TOO BIG.");
        goto L10920;
    }
    Ps("OK, THE "); Ps(NOUNS[PRS[5]]);
    Ps(" IS NOW INSIDE THE "); Ps(NOUNS[PRS[9]]); PL(".");
    NOUN[PRS[5]][9] = PRS[9];
    if (CHARACTER[64] == PRS[5]) CHARACTER[64] = 0;
    if (CHARACTER[65] == PRS[5]) CHARACTER[65] = 0;
    NOUN[PRS[5]][1] = NOUN[PRS[9]][1];          /* 10770 */
    if (NOUN[PRS[5]][5] != 0) {
        E = NOUN[PRS[9]][1]; N = PRS[5];
        PUT_COMPLETE();
    }
L10830:
    if (PRS[6] != 1) goto L10920;               /* 10830 */
    DROP_SUB();                                 /* 10850 PUT ON -> drop */
L10920: ;
}

/* ================================================================== */
/* 10930 DEFINE PUT.COMPLETE                                           */
/* ================================================================== */

static void PUT_COMPLETE(void)
{
    /* E = location we are putting to, N = container noun */
    for (I = 1; I <= NNUM; I++) {
        if (NOUN[I][9] == N) NOUN[I][1] = E;
        if (NOUN[I][5] != 0) {
            for (J = 1; J <= NNUM; J++)
                if (NOUN[J][9] == I) NOUN[J][1] = NOUN[I][1];
        }
    }
}

/* ================================================================== */
/* 11150 DEFINE PRINT.NOUN  -- "<adj> <adj> <noun>", no newline        */
/* ================================================================== */

static void PRINT_NOUN(void)
{
    if (NOUN[N][6] < 1) goto L11230;
    Ps(ADJECTS[NOUN[N][6]]); Ps(" ");
L11230:
    if (NOUN[N][7] < 1) goto L11250;
    Ps(ADJECTS[NOUN[N][7]]); Ps(" ");
L11250:
    Ps(NOUNS[N]);
}

/* ================================================================== */
/* 11270 DEFINE WEAR.SUB                                               */
/* ================================================================== */

static void WEAR_SUB(void)
{
    if (PRS[1] == 65) {                         /* 11320 STRAP */
        if (PRS[2] != 1 && PRS[6] != 1) {
            PL("THAT DOESN'T MAKE SENSE TO ME.");
            goto L11910;
        }
    }
    if (PRS[5] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L11910;
    }
    E = -1;
    CHECK_EM();
    if (NOUN[PRS[5]][1] != -1 && CHARACTER[64] != PRS[5] &&
        CHARACTER[65] != PRS[5]) {
        Ps("YOU DON'T HAVE THE ");
        N = PRS[5];
        PRINT_NOUN();
        PL(" IN HAND.");
        goto L11910;
    }
    if (NOUN[PRS[5]][14] != 1 && NOUN[PRS[5]][14] != 2 &&
        NOUN[PRS[5]][14] != 3 && NOUN[PRS[5]][14] != 9) {
        Ps("YOU CAN'T WEAR A ");
        N = PRS[5];
        PRINT_NOUN();
        PL("!");
        goto L11910;
    }
    E = 0;                                      /* 11600 armor must fit */
    if (NOUN[PRS[5]][14] == 1) {
        ROND = ROND + 10;
        if (EQ(CHARACS[4], "ELF")    && NOUN[PRS[5]][11] != 2) E = 1;
        if (EQ(CHARACS[4], "HUMAN")  && NOUN[PRS[5]][11] != 1) E = 1;
        if (EQ(CHARACS[4], "HOBBIT") && NOUN[PRS[5]][11] != 4 &&
                                        NOUN[PRS[5]][11] != 3) E = 1;
        if (EQ(CHARACS[4], "DWARF")  && NOUN[PRS[5]][11] != 4 &&
                                        NOUN[PRS[5]][11] != 3) E = 1;
        if (EQ(CHARACS[4], "ORC")    && NOUN[PRS[5]][11] != 5) E = 1;
        if (E == 1) {
            PL("THE ARMOR DOESN'T FIT YOU.");
            goto L11910;
        }
        if (E != 1) {
            Z = 63;
            PRINTER_SUB();
        }
    }
    for (I = 1; I <= NNUM; I++) {               /* 11770 no duplicates */
        if (NOUN[I][14] == NOUN[PRS[5]][14] && NOUN[I][9] == 3) {
            Ps("YOU ARE ALREADY WEARING A "); Ps(NOUNS[PRS[5]]); PL(".");
            goto L11910;
        }
    }
    if (CHARACTER[64] == PRS[5]) CHARACTER[64] = 0;
    if (CHARACTER[65] == PRS[5]) CHARACTER[65] = 0;
    NOUN[PRS[5]][9] = 3;
    if (NOUN[PRS[5]][14] == 2) Z = 64;
    if (NOUN[PRS[5]][14] == 3) Z = 65;
    if (NOUN[PRS[5]][14] == 9) Z = 66;
    PRINTER_SUB();
L11910: ;
}

/* ================================================================== */
/* 11920 DEFINE CHECK.BP  -- is the item buried in a worn pack/closed   */
/* container?  MF=1 suppresses the messages (used for monsters).        */
/* ================================================================== */

static void CHECK_BP(void)
{
    ER = 0;
    if (NOUN[NOUN[N][9]][14] == 9 && NOUN[NOUN[N][9]][9] == 3) {  /* 12010 */
        Z = 67;
        if (MF == 1) goto L12040;
        PRINTER_SUB();
    L12040:
        ER = 1;
        goto L12240;
    }
    if (NOUN[NOUN[N][9]][5] == 2) {             /* 12080 item is closed */
        if (MF == 1) goto L12100;
        Ps("THE "); Ps(NOUNS[NOUN[N][9]]); PL(" IS CLOSED.");
    L12100:
        ER = 1;
        goto L12240;
    }
    if (NOUN[NOUN[N][9]][9] == 0) goto L12240;  /* 12140 */
    if (NOUN[NOUN[NOUN[N][9]][9]][5] == 2) {
        if (MF == 1) goto L12210;
        Ps("THE ");
        N = NOUN[NOUN[N][9]][9];
        PRINT_NOUN();
        PL(" IS CLOSED,SO YOU CANNOT REACH IT.");
    L12210:
        ER = 1;
        goto L12240;
    }
L12240:
    MF = 0;
}

/* ================================================================== */
/* 12260 DEFINE MOVE.SUB  -- the cathedral statue                      */
/* ================================================================== */

static void MOVE_SUB(void)
{
    if (PRS[5] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L12710;
    }
    E = P;
    CHECK_EM();
    if (NOUN[PRS[5]][1] == -2) goto L12420;
    if (NOUN[PRS[5]][1] == P && NOUN[PRS[5]][1] != -1) {
        PL("I DON'T SEE ONE HERE.");
        goto L12710;
    }
L12420:
    if (NOUN[PRS[5]][1] == -2) {
        if (PRS[5] == 17 && P != 22) {
            PL("I DON'T SEE IT HERE.");
            goto L12710;
        }
        if (PRS[5] == 17 && P == 22 && DFLAG[2] == 0) {
            if (CHARACTER[1] < 8) {
                Z = 58;
                PRINTER_SUB();
                goto L12710;
            }
            MOV[22][4] = 10; MOV[10][1] = 22;   /* 12520 expose the exit */
            Z = 59;
            DFLAG[2] = 1;
            PRINTER_SUB();
            goto L12710;
        }
        if (PRS[5] == 17 && P == 22 && DFLAG[2] == 1) {         /* 12580 */
            if (CHARACTER[1] < 8) {
                Z = 58;
                PRINTER_SUB();
                goto L12710;
            }
            MOV[22][4] = 0; MOV[10][1] = 0;
            Z = 84;
            DFLAG[2] = 0;
            PRINTER_SUB();
            goto L12710;
        }
    }
L12710: ;
}

/* ================================================================== */
/* 12720 DEFINE CPARSER.SUB  -- the combat parser, prompt "*"          */
/* ================================================================== */

static void CPARSER_SUB(void)
{
    AA = 1;
    for (I = 0; I <= MAXWORD; I++)
        strcpy(WORD[I], " ");
L12800:
    bas_cib();
    Ps("*");
L12820:
    IS[0] = (char)bas_getch();
    IS[1] = '\0';
    if (IS[0] == 13 && (int)strlen(A1S) < 1 && AA == 1)
        goto L12800;
    if (EQ(IS, ",")) COMMLOC = AA;
    if (EQ(IS, ",")) strcpy(IS, " ");
    if (IS[0] != 13 && NE(IS, " ") && NE(IS, ","))
        strncat(A1S, IS, sizeof A1S - strlen(A1S) - 1);
    if (IS[0] == 13) goto L12910;
    if (EQ(IS, " ")) goto L12910;
    goto L12820;
L12910:
    if (AA >= 0 && AA <= MAXWORD)
        snprintf(WORD[AA], sizeof WORD[0], "%s", A1S);
    A1S[0] = '\0';
    if (AA >= 0 && AA <= MAXWORD &&
        (EQ(WORD[AA], "THE") || EQ(WORD[AA], "GO") || EQ(WORD[AA], "A")))
        AA = AA - 1;
    AA = AA + 1;
    if (AA > MAXWORD) AA = MAXWORD;
    if (IS[0] == 13) goto L12970;
    goto L12820;

L12970:
    memcpy(PRSBAK, PRS, sizeof PRS);
    memset(PRS, 0, sizeof PRS);
    WNUM = 1;
    if (COMMFLAG == 1) {
        COMMFLAG = 0; WNUM = COMMLOC + 1; COMMLOC = 0;
        goto L13260;
    }
    if (EQ(WORD[1], "PICK") && EQ(WORD[2], "UP")) {
        WNUM = 3; PRS[1] = 27;
        goto L13340;
    }
    if (EQ(WORD[1], "PUT") && EQ(WORD[2], "DOWN")) {
        WNUM = 3; PRS[1] = 28;
        goto L13340;
    }
    CHECK_ADVERB();
    if (PRS[10] > 0 && EQ(WORD[2], "PICK") && EQ(WORD[3], "UP")) {
        WNUM = 4; PRS[1] = 27;
        goto L13340;
    }
    if (PRS[10] > 0 && EQ(WORD[2], "PUT") && EQ(WORD[3], "DOWN")) {
        WNUM = 4; PRS[1] = 28;
        goto L13340;
    }
L13260:
    for (I = 1; I <= VNUM; I++) {
        if (EQ(WORD[WNUM], VERBS[I])) {
            PRS[1] = I; WNUM = WNUM + 1;
            goto L13340;
        }
    }
L13340:
    for (I = 1; I <= PNUM; I++) {
        if (EQ(WORD[WNUM], PREPS[I])) {
            PRS[2] = I; WNUM = WNUM + 1;
            goto L13410;
        }
    }
L13410:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[3] = I; WNUM = WNUM + 1;
            goto L13480;
        }
    }
L13480:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[4] = I; WNUM = WNUM + 1;
            goto L13550;
        }
    }
L13550:
    for (I = 1; I <= NNUM; I++) {
        if (EQ(WORD[WNUM], "IT")) {
            PRS[5] = PRSBAK[5]; PRS[3] = PRSBAK[3]; PRS[4] = PRSBAK[4];
        }
        if (EQ(WORD[WNUM], "IT")) WNUM = WNUM + 1;
        if (EQ(WORD[WNUM], NOUNS[I]) || EQ(WORD[WNUM], NOUN2S[I])) {
            PRS[5] = I; WNUM = WNUM + 1;
            goto L13650;
        }
    }
L13650:
    for (I = 1; I <= PNUM; I++) {
        if (EQ(WORD[WNUM], PREPS[I])) {
            WNUM = WNUM + 1; PRS[6] = I;
            goto L13720;
        }
    }
L13720:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[7] = I; WNUM = WNUM + 1;
            goto L13790;
        }
    }
L13790:
    for (I = 1; I <= ANUM; I++) {
        if (EQ(WORD[WNUM], ADJECTS[I])) {
            PRS[8] = I; WNUM = WNUM + 1;
            goto L13860;
        }
    }
L13860:
    for (I = 1; I <= NNUM; I++) {
        if (EQ(WORD[WNUM], "IT")) {
            PRS[9] = PRSBAK[5]; PRS[7] = PRSBAK[3]; PRS[8] = PRSBAK[4];
        }
        if (EQ(WORD[WNUM], "IT")) WNUM = WNUM + 1;
        if (EQ(WORD[WNUM], NOUNS[I]) || EQ(WORD[WNUM], NOUN2S[I])) {
            PRS[9] = I; WNUM = WNUM + 1;
            goto L13960;
        }
    }
L13960:
    if (PRS[1] < 1) goto L14170;                /* 13970 */
    /* 14020 noun modifiers.  Note VCODE 28 is LOOK, but in combat the
       original dispatches it to CLOSE.SUB [original bug]. */
    if (PRS[1] < 1) goto L14170;
    if (VCODE[PRS[1]] == 30) PUT_SUB();
    if (VCODE[PRS[1]] == 26) OPEN_SUB();
    if (VCODE[PRS[1]] == 28) CLOSE_SUB();
    if (VCODE[PRS[1]] == 15) GET_SUB();
    if (VCODE[PRS[1]] == 35) CPUMMEL_SUB();
    if (VCODE[PRS[1]] == 36) CGRAPPL_SUB();
    if (VCODE[PRS[1]] == 11) KILL_SUB();
    if (VCODE[PRS[1]] == 34) DISENGAGE_SUB();
    if (VCODE[PRS[1]] == 16) DROP_SUB();
    if (VCODE[PRS[1]] == 35) MODE = 2;
    if (VCODE[PRS[1]] == 36) MODE = 3;
    if (VCODE[PRS[1]] == 11) MODE = 1;
    /* [port] QUIT, as above */
    if (!opt_strict && VCODE[PRS[1]] == 21) bas_end("SORRY CHUCK.");
    A = PRS[1];
L14170:
    if (PRS[1] < 1)
        PL("YOU CAN'T DO THAT!");
}

/* ================================================================== */
/* 14290 DEFINE KILL.SUB  -- armed attack                              */
/* ================================================================== */

static void KILL_SUB(void)
{
    if (PRS[9] > 0 && CHARACTER[HAND] != PRS[9] && CHARACTER[BHAND] != PRS[9]) {
        PL("YOU DON'T HAVE THAT IN HAND TO FIGHT WITH.");
        goto L15070;
    }
    if (CHARACTER[HAND] < 1 && CHARACTER[BHAND] < 1) {
        PL("YOUR HANDS ARE EMPTY.");
        PL("YOU MUST HAVE A WEAPON TO FIGHT WITH.");
        goto L15070;
    }
    if (CHARACTER[HAND] < 1) WHAND = BHAND; else WHAND = HAND;
    if (CHARACTER[70] != 0)
        PL("ALTHOUGH YOU ARE IN A HELD POSITION, YOU ATTACK ANYWAY.");
    A = RNDI(99) + 1;                           /* 14480 */
    TH = RNDI(19) + 1;
    PTH = TH;
    if (CHARACTER[1] > 8) TH = TH + CHARACTER[1] - 8;
    if (CHARACTER[2] > 8) TH = TH + CHARACTER[2] - 8;
    if (CHARACTER[7] > 6) TH = TH + CHARACTER[7] - 6;
    if (NOUN[CHARACTER[69]][19] == 2 || NOUN[CHARACTER[69]][19] == 3) TH = TH + 30;
    if (NOUN[CHARACTER[69]][19] == 1) TH = TH + 4;
    TH = TH + CHARACTER[58] / 10.0;
    if (PTH < 3) {                              /* 14580 fumble */
        PL("YOU SERIOUSLY FUMBLED THAT ATTACK!");
        if (PTH == 1) {
            Ps("YOU DROPPED THE ");
            N = CHARACTER[WHAND];
            PRINT_NOUN();
            PL("ON THE GROUND!");
            if (PTH == 1) { NOUN[CHARACTER[WHAND]][1] = P; CHARACTER[WHAND] = 0; }
            CHARACTER[66] = 1;
            goto L15070;
        }
    }
    if (TH < (12 + (10 - NOUN[CHARACTER[69]][5]) - (CHARACTER[58] / 10.0))) {
        PL("YOUR ATTACK MISSES.");
        goto L15070;
    }
    if (TH >= (12 + (10 - NOUN[CHARACTER[69]][5]) - (CHARACTER[58] / 10.0))) {
        if (TH - (12 + (10 - NOUN[CHARACTER[69]][5])) < 3) {
            PL("YOUR BLOW GLANCES OFF OF IT'S TARGET.");
            goto L15070;
        }
        if (TH == (12 + (10 - NOUN[CHARACTER[69]][5]))) {
            PL("THE MONSTER BARELY DODGES YOUR ATTACK!");
            goto L15070;
        }
        PL("YOUR ATTACK DAMAGED YOUR OPPONENT.");
        DAM = RNDI(NOUN[CHARACTER[WHAND]][4]) + 1;
        if (NOUN[CHARACTER[69]][19] == 2 || NOUN[CHARACTER[69]][19] == 3) DAM = DAM + 100;
        if (NOUN[CHARACTER[69]][19] == 1) DAM = DAM + 4;
        if (CHARACTER[1] > 8) DAM = DAM + CHARACTER[2] - 8;
        if (WHAND == BHAND) TH = TH - 4;
        if (CHARACTER[7] > 8) DAM = DAM + CHARACTER[7] - 8;
        DAM = DAM + CHARACTER[58] / 10.0;
        /* [original bug] DAM is computed but never applied; the monster
           loses exactly one hit point per landed blow. */
        NOUN[CHARACTER[69]][8] = NOUN[CHARACTER[69]][8] - 1;      /* 14930 */
        if (NOUN[CHARACTER[69]][8] < 1) {
            Ps("THE ");
            N = CHARACTER[69];
            PRINT_NOUN();
            PL(" SLUMPS TO THE GROUND, DEAD.");
            CHARACTER[69] = 0;
            NOUN[N][7] = NOUN[N][6]; NOUN[N][6] = 17; NOUN[N][3] = 0;
            NOUN[N][19] = 0; NOUN[N][20] = 0; NOUN[N][21] = 0; NOUN[N][18] = 1;
            NOUN[N][18] = 1; NOUN[N][9] = 0;
            goto L15070;
        }
    }
L15070: ;
}

/* ================================================================== */
/* 15080 DEFINE CPUMMEL.SUB  -- the player punches                     */
/* ================================================================== */

static void CPUMMEL_SUB(void)
{
    DAM = 0;
    if (CHARACTER[70] != 0) {                   /* 15160 is he held? */
        CROLL_SUB();
        if (A < CHARACTER[70]) {
            PL("YOU FAIL TO BREAK THE HOLD YOUR OPPONENT HAS YOU IN.");
            goto L15860;
        }
        PL("YOU BREAK YOUR OPPONENTS HOLD AND ATTACK.");
        CHARACTER[70] = 0;
    }
    if (CHARACTER[HAND] != 0) {
        Ps("YOU DROP THE ");
        N = CHARACTER[HAND];
        PRINT_NOUN();
        PL(" THAT WAS IN YOUR HAND.");
        NOUN[CHARACTER[HAND]][1] = P;
    }
    CROLL_SUB();
    TH = A;
    if (TH < 1) {
        PL("YOUR BLOW MISSES.");
        goto L15720;
    }
    if (TH < 21) {
        PL("INEFFECTIVE BLOW, YOU MAY STRIKE AGAIN.");
        NOUN[CHARACTER[69]][18] = 1;
        goto L15720;
    }
    if (TH < 41) {
        PL("YOU MANAGE A GLANCING BLOW, THROWING YOURSELF OFF BALANCE.");
        DAM = 2;
        goto L15720;
    }
    if (TH < 61) {
        PL("YOU SCORE A GLANCING BLOW.");
        NOUN[CHARACTER[69]][18] = 1;
        DAM = 4;
        goto L15720;
    }
    if (TH < 81) {
        PL("YOU SCORE A SOLID PUNCH, BUT THROW YOURSELF OFF BALANCE.");
        DAM = 6;
        goto L15720;
    }
    if (TH < 101) {
        PL("YOU LAND A VERY SOLID PUNCH.");
        DAM = 8;
        NOUN[CHARACTER[69]][18] = 1;
        goto L15720;
    }
    if (TH > 100) {
        PL("YOU STRIKE YOUR TARGET WITH A CRUSHING BLOW, STUNNING IT.");
        DAM = 10;
        NOUN[CHARACTER[69]][18] = 3;
        NOUN[CHARACTER[69]][19] = 1;
    }
L15720:
    if (DAM < 1) goto L15860;
    if (CHARACTER[1] > 6) DAM = DAM + CHARACTER[1] - 6;
    if (CHARACTER[7] > 6) DAM = DAM + CHARACTER[7] - 6;
    NOUN[CHARACTER[69]][8] = NOUN[CHARACTER[69]][8] - DAM;
    if (NOUN[CHARACTER[69]][8] < 1) {
        CHARACTER[70] = 0; NOUN[CHARACTER[69]][20] = 0;
        Ps("THE ");
        N = CHARACTER[69];
        PRINT_NOUN();
        PL(" FALLS TO THE GROUND, UNCONSCIOUS.");
        NOUN[N][8] = 1;
        NOUN[N][19] = 2; NOUN[N][18] = 3 + RNDI(4) + 2;
    }
L15860: ;
}

/* ================================================================== */
/* 15870 DEFINE CGRAPPL.SUB  -- the player wrestles                    */
/* ================================================================== */

static void CGRAPPL_SUB(void)
{
    DAM = 0;
    if (CHARACTER[70] != 0) {
        CROLL_SUB();
        if (A < CHARACTER[70]) {
            PL("YOUR OPPONENT HAS YOU IN A HOLD, AND YOU'VE FAILED TO BREAK IT.");
            goto L16530;
        }
        PL("YOU BREAK THE HOLD YOU OPPONENT HAS YOU IN, ALLOWING AN ATTACK.");
        CHARACTER[70] = 0;
    }
    if (CHARACTER[HAND] != 0) {
        N = CHARACTER[HAND];
        Ps("YOU DROP THE ");
        PRINT_NOUN();
        PL(" THAT YOU HAD IN YOUR HAND.");
        NOUN[CHARACTER[HAND]][1] = P; CHARACTER[HAND] = 0;
    }
    CROLL_SUB();
    TH = floor(A);                              /* 16120 TH=INT(A) */
    if (TH < 1)                 PL("YOU GET YOUR OPPONENT IN A LIGHT WAIST CLINCH.");
    if (TH > 20 && TH < 41)   { PL("YOU GET YOUR OPPONENT IN AN ARMLOCK."); DAM = 1; }
    if (TH > 40 && TH < 56)   { PL("YOU GET YOUR TARGET IN A FINGER LOCK."); DAM = 2; }
    if (TH > 55 && TH < 71)   { PL("YOU HAVE WRANGLED YOUR ENEMY INTO A BEAR HUG."); DAM = 3; }
    if (TH > 70 && TH < 86)   { PL("YOU HAVE YOUR OPPONENT IN A HEAD LOCK."); DAM = 5; }
    if (TH > 85 && TH < 96)   { PL("YOU HAVE YOUR OPPONENT IN A STRANGLE-HOLD."); DAM = 6; }
    if (TH > 95) {
        PL("YOU KICK YOUR OPPONENT DOWN, STUNNING IT.");
        DAM = 8;
        NOUN[CHARACTER[69]][18] = 2; NOUN[CHARACTER[69]][19] = 1;
    }
    NOUN[CHARACTER[69]][20] = TH;
    N = CHARACTER[69];
    if (CHARACTER[1] > 6) DAM = DAM + CHARACTER[1] - 6;
    if (CHARACTER[7] > 6) DAM = DAM + CHARACTER[7] - 6;
    NOUN[N][8] = NOUN[N][8] - DAM;
    if (NOUN[N][8] < 1) {
        CHARACTER[70] = 0; NOUN[CHARACTER[69]][20] = 0;
        Ps("THE ");
        PRINT_NOUN();
        PL(" SLUMPS TO THE GROUND, UNCONSCIOUS.");
        NOUN[N][8] = NHITSII[N] / 2;
        NOUN[N][19] = 2; NOUN[N][18] = 3 + RNDI(4);
    }
L16530: ;
}

/* ================================================================== */
/* 16540 DEFINE CROLL.SUB                                              */
/* ================================================================== */

static void CROLL_SUB(void)
{
    A = RNDI(99) + 1;
    A = RNDI(99) + 1;                           /* 16590, as written */
    A = A + CHARACTER[63] / 2;
    A = A + CHARACTER[1];
    A = A + CHARACTER[2];
    A = A + CHARACTER[1] / 2.0;
    A = A + CHARACTER[7] / 2.0;
    if (NOUN[CHARACTER[69]][19] == 4) A = A + 10;
    if (NOUN[CHARACTER[69]][19] == 1) A = A + 20;
    if (NOUN[CHARACTER[69]][20] != 0) A = A + 40;
    if (NOUN[CHARACTER[69]][19] != 1 && NOUN[CHARACTER[69]][19] != 4) {
        A = A - (NOUN[CHARACTER[69]][23] - 6);
        A = A - (NOUN[CHARACTER[69]][22] - 6);
        if (NOUN[CHARACTER[69]][5] < 6) A = A - 10;
        if (NOUN[CHARACTER[69]][5] < 4) A = A - 20;
        if (NOUN[CHARACTER[69]][5] < 2) A = A - 30;
    }
}

/* ================================================================== */
/* 16760 DEFINE PUT.HAND                                               */
/* ================================================================== */

static void PUT_HAND(void)
{
    A = 0;
    if (PRS[7] != 31 && PRS[7] != 32 && PRS[8] != 31 && PRS[8] != 32) {
        PL("WHICH HAND?");
        goto L17070;
    }
    if (PRS[7] > 30 && PRS[7] < 33 && PRS[8] > 30 && PRS[8] < 33) {
        PL("WHICH HAND?");
        goto L17070;
    }
    if (PRS[7] == PRS[8]) {
        PL("WHICH HAND?");
        goto L17070;
    }
    if (PRS[7] > 30 && PRS[7] < 33) A = PRS[7];
    if (PRS[8] > 30 && PRS[8] < 33) A = PRS[8];
    if (A == 31) A = 64;
    if (A == 32) A = 65;
    if (CHARACTER[(int)A] != 0) {
        PL("THERE IS SOMETHING ALREADY IN THAT HAND!");
        goto L17070;
    }
    if (CHARACTER[64] == PRS[5]) CHARACTER[64] = 0;
    if (CHARACTER[65] == PRS[5]) CHARACTER[65] = 0;
    PL("OK.");
    CHARACTER[(int)A] = PRS[5];
    /* [original bug] A has already been rewritten to 64/65 above, so these
       two lines can never fire and NOUN(...,9) is left stale. */
    if (A == 31) NOUN[PRS[5]][9] = 1;
    if (A == 32) NOUN[PRS[5]][9] = 2;
L17070: ;
}

/* ================================================================== */
/* 17080 DEFINE ENGAGE.SUB                                             */
/* ================================================================== */

static void ENGAGE_SUB(void)
{
    if (PRS[5] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L17310;
    }
    E = P;
    CHECK_EM();
    if (NOUN[PRS[5]][1] != P) {
        PL("I DON'T SEE IT HERE.");
        goto L17310;
    }
    if (NOUN[PRS[5]][3] == 0) {
        PL("YOU CAN'T ATTACK THAT!");
        goto L17310;
    }
    PL("OK.");
    NOUN[PRS[5]][15] = 1;
    PL("YOU ARE NOW IN COMBAT MODE.");
    CHARACTER[69] = PRS[5];
    NOUN[PRS[5]][21] = 1;
    if (RNDI(99) + 1 < 51) {
        Ps("THE ");
        N = PRS[5];
        PRINT_NOUN();
        PL(" GETS THE FIRST ADVANTAGE!");
        CHARACTER[66] = 1;
    }
L17310: ;
}

/* ================================================================== */
/* 17320 DEFINE DISENGAGE.SUB                                          */
/* ================================================================== */

static void DISENGAGE_SUB(void)
{
    A = RNDI(99) + 1;
    A = A - NOUN[CHARACTER[69]][22];
    A = A - NOUN[CHARACTER[69]][23];
    if (NOUN[CHARACTER[69]][19] == 2) A = A + 100;
    if (A < 60) {
        PL("YOU FAIL TO BREAK COMBAT!");
        goto L17490;
    }
    PL("YOU MANAGE TO BREAK FROM COMBAT!");
    NOUN[CHARACTER[69]][18] = 2;
    N = CHARACTER[69];
    CHARACTER[69] = 0;
    NOUN[N][21] = 0;
L17490: ;
}

/* ================================================================== */
/* 17500 DEFINE VALUE.MANAGE                                           */
/* ================================================================== */

static void VALUE_MANAGE(void)
{
    HAND = CHARACTER[68];
    if (HAND == 64) BHAND = 65;
    if (HAND == 65) BHAND = 64;
    if (CHARACTER[64] > 0) if (NOUN[CHARACTER[64]][1] != -1) CHARACTER[64] = 0;
    if (CHARACTER[65] > 0) if (NOUN[CHARACTER[65]][1] != -1) CHARACTER[65] = 0;
    A = RNDI(99) + 1;
    TURN = ROND / 10;
    AC = 10;                                    /* 17608 armor class */
    /* [original bug] 17610 reads AC-CHARACTER(2)-8, not AC-(CHARACTER(2)-8) */
    if (CHARACTER[2] > 8) AC = AC - CHARACTER[2] - 8;
    for (I = 1; I <= NNUM; I++) {
        if (NOUN[I][14] == 1 && NOUN[I][9] == 3) {
            AC = AC - NOUN[I][20];
            goto L17624;
        }
    }
L17624:
    CHARACTER[71] = AC;
}

/* ================================================================== */
/* 17640 DEFINE MKILL.SUB  -- the monster attacks to kill              */
/* ================================================================== */

static void MKILL_SUB(void)
{
    if (NOUN[M][20] != 0)
        PL("EVEN THOUGH YOU HAVE YOUR OPPONENT IN A HOLD, HE ATTACKS YOU.");
    TH = RNDI(19) + 1;
    if (TH < 3) {
        PL("THE MONSTER FUMBLES IT'S ATTACK!");
        NOUN[M][18] = 1;
        goto L18110;
    }
    if (NOUN[M][22] > 8) TH = TH + NOUN[M][22] - 8;
    if (NOUN[M][21] > 8) TH = TH + NOUN[M][21] - 8;
    TH = TH + NOUN[M][24] / 10;
    TB = (12 + (10 - CHARACTER[71]) - NOUN[M][24] / 10);
    if (TH < TB) {
        PL("THE MONSTERS ATTACK MISSES.");
        goto L18110;
    }
    if (TH > TB) {
        if (TH - TB < 3) {
            PL("YOU EVADE THE MONSTERS ATTACK.");
            goto L18110;
        }
        if (TH == TB) {
            PL("THE MONSTERS BLOW MERELY GLANCES AWAY, WITHOUT HARMING YOU.");
            goto L18110;
        }
        if (NOUN[M][13] != 0) DAM = RNDI(NOUN[NOUN[M][13]][4]) + 1;
        if (NOUN[M][13] == 0) {
            DAM = 0;
            for (I = 1; I <= NOUN[M][4]; I++)
                DAM = DAM + RNDI(NOUN[M][10]) + 1;
        }
    }
    CHARACTER[8] = CHARACTER[8] - DAM;
    if (CHARACTER[8] < 1) {
        PL("YOU ARE DEAD.");
        for (I = 1; I <= NNUM; I++)
            if (NOUN[I][1] == -1) NOUN[I][1] = P;
        for (I = 1; I <= 80; I++)
            CHARACTER[I] = 0;
        bas_end("SORRY CHUCK.");
    }
L18110: ;
}

/* ================================================================== */
/* 18120 DEFINE MPUMMEL.SUB                                            */
/* ================================================================== */

static void MPUMMEL_SUB(void)
{
    if (NOUN[M][20] != 0) {
        MROLL_SUB();
        if (A < NOUN[M][18]) {
            PL("THE MONSTER FAILS TO BREAK YOUR HOLD.");
            goto L18690;
        }
        PL("THE MONSTER BREAKS FREE OF YOUR HOLD.");
        NOUN[M][20] = 0;
    }
    MROLL_SUB();
    TH = A;
    if (TH < 1) {
        PL("YOUR OPPONENT'S ATTACK MISERABLY MISSES.");
        goto L18690;
    }
    if (TH > 0 && TH < 21) {
        PL("THE ATTACK WAS INEFFECTIVE.");
        CHARACTER[66] = 1;
        goto L18690;
    }
    if (TH > 20 && TH < 41) { PL("YOU RECEIVE A GLANCING BLOW."); DAM = 2; }
    if (TH > 40 && TH < 61) { PL("YOU RECEIVE A GLANCING BLOW."); DAM = 4; CHARACTER[66] = 1; }
    if (TH > 60 && TH < 81) { PL("A SOLID PUNCH HITS YOU AND STARTS YOU TO WORRY!"); DAM = 6; }
    if (TH > 80 && TH < 101) { PL("YOU RECEIVE A VERY SOLID PUNCH!"); DAM = 8; CHARACTER[66] = 1; }
    if (TH > 100) {
        PL("YOU ARE THE VICTM OF A SERIOUSLY CRUSHING BLOW!");
        DAM = 10;
        CHARACTER[67] = 1; CHARACTER[66] = 2;
    }
    if (NOUN[M][22] > 6) DAM = DAM + NOUN[M][22] - 6;
    if (NOUN[M][23] > 6) DAM = DAM + NOUN[M][23] - 6;
    CHARACTER[8] = CHARACTER[8] - DAM;
    if (DAM >= 8 && CHARACTER[HAND] != 0) {
        NOUN[CHARACTER[HAND]][1] = P;
        Ps("YOU DROP THE ");
        N = CHARACTER[HAND];
        PRINT_NOUN();
        if (HAND == 64) PL(" THAT WAS IN YOUR LEFT HAND.");
        if (HAND == 65) PL(" THAT WAS IN YOUR RIGHT HAND.");
        CHARACTER[HAND] = 0;
    }
    if (CHARACTER[8] < 1) {
        CHARACTER[8] = CHARACTER[9] / 2;
        CHARACTER[66] = (10 - CHARACTER[4] - 6);
        if (CHARACTER[66] < 1) CHARACTER[66] = 2;
        CHARACTER[67] = 2;
        CHARACTER[70] = 0; NOUN[M][18] = 0;
    }
L18690: ;
}

/* ================================================================== */
/* 18700 DEFINE MGRAPPL.SUB                                            */
/* ================================================================== */

static void MGRAPPL_SUB(void)
{
    DAM = 0;
    if (NOUN[M][20] != 0) {
        MROLL_SUB();
        if (A < NOUN[M][18]) {
            PL("YOUR OPPONENT FAILS TO BREAK FREE OF YOUR HOLD.");
            goto L19260;
        }
        PL("YOUR OPPONENT BREAKS FREE OF YOUR HOLD, ALLOWING HIM TO ATTACK.");
        NOUN[M][20] = 0;
    }
    MROLL_SUB();
    TH = A;
    if (TH < 21)              PL("YOUR OPPONENT BARELY GET'S YOU INTO A WAIST-CLINCH");
    if (TH > 20 && TH < 41) { PL("YOUR OPPONENT GET'S YOU INTO AN ARMLOCK."); DAM = 1; }
    if (TH > 40 && TH < 56) { PL("YOUR OPPONENT HOLDS YOU IN A HAND LOCK."); DAM = 2; }
    if (TH > 55 && TH < 71) { PL("YOUR ENEMY HAS MANAGED TO GET YOU INTO A BEAR HUG."); DAM = 3; }
    if (TH > 70 && TH < 86) { PL("YOUR OPPONENT WRANGLES YOU INTO A HEADLOCK."); DAM = 5; }
    if (TH > 85 && TH < 96) { PL("YOUR ENEMY HAS YOU IN A STRANGLE-HOLD."); DAM = 6; }
    if (TH > 95) {
        PL("YOUR ENEMY KICKS YOU, KNOCKING YOU DOWN, AND STUNNING YOU.");
        DAM = 8;
        CHARACTER[66] = 2; CHARACTER[67] = 1;
    }
    CHARACTER[70] = TH;
    CHARACTER[8] = CHARACTER[8] - DAM;
    if (CHARACTER[8] < 1) {
        PL("YOU SLIP INTO UNCONSCIOUSNESS.");
        CHARACTER[67] = 2; CHARACTER[66] = RNDI(4) + 2;
        CHARACTER[70] = 0;
        NOUN[M][18] = 0;
    }
L19260: ;
}

/* ================================================================== */
/* 19270 DEFINE MROLL.SUB                                              */
/* ================================================================== */

static void MROLL_SUB(void)
{
    A = RNDI(99) + 1;
    A = A + NOUN[M][23];
    A = A + NOUN[M][22];
    A = A + NOUN[M][22] / 5;
    A = A + CHARACTER[71];
    if (CHARACTER[67] != 1 && CHARACTER[67] != 2) {
        A = A - CHARACTER[1] - 6;
        A = A - CHARACTER[2] - 6;
        A = A - CHARACTER[7] - 6;
    }
}

/* ================================================================== */
/* 19450 DEFINE DISPLAY                                                */
/* ================================================================== */

static void DISPLAY(void)
{
    NL();
    NL();
    PL("*********************************************************************");
    Ps(CHARACS[1]); Ps("  "); Ps(CHARACS[6]); Ps(" ");
    Ps(CHARACS[4]); Ps("-"); PL(CHARACS[2]);
    Ps("STRENGTH:");    Pn(CHARACTER[1]); Ptab(25); Ps("HITS:");         Pn(CHARACTER[8]); NL();
    Ps("DEXTERITY:");   Pn(CHARACTER[2]); Ptab(25); Ps("HIT MAX:");      Pn(CHARACTER[9]); NL();
    Ps("PERSONALITY:"); Pn(CHARACTER[3]); Ptab(25); PL("SKILLS:");
    Ps("ENDURANCE:");   Pn(CHARACTER[4]); Ptab(25); Ps("FIGHTING:");     Pn(CHARACTER[58]); NL();
    Ps("INTELIGENCE:"); Pn(CHARACTER[5]); Ptab(25); Ps("THIEVING:");     Pn(CHARACTER[59]); NL();
    Ps("MANA:");        Pn(CHARACTER[6]); Ptab(25); Ps("TRADING:");      Pn(CHARACTER[60]); NL();
    Ps("LUCK:");        Pn(CHARACTER[7]); Ptab(25); Ps("MAGIC:");        Pn(CHARACTER[61]); NL();
    Ps("GOLD:");        Pn(CHARACTER[10]); Ptab(25); Ps("MECHANICS:");   Pn(CHARACTER[62]); NL();
    Ps("ARMOR CLASS:"); Pn(CHARACTER[71]); Ptab(25); Ps("HAND TO HAND:"); Pn(CHARACTER[68]); NL();
    NL();
    if (CHARACTER[68] == 64) PL("YOU ARE LEFT HANDED.");
    if (CHARACTER[68] == 65) PL("YOU ARE RIGHT HANDED.");
    Ps("THIS IS TURN "); Pn(TURN); NL();
    PL("*********************************************************************");
    NL();
}

/* ================================================================== */
/* 19650 DEFINE HOBBIT.SUB  -- demi-human movement and larceny         */
/* ================================================================== */

static void HOBBIT_SUB(void)
{
    FACE = 0;
    if (NOUN[M][1] == P && CHARACTER[67] == 0 && NOUN[M][21] == 0) {
        Z = NOUN[M][17];
        PRINTER_SUB();
    }
    if (NOUN[M][1] == P && NOUN[M][15] == 1 && NOUN[M][21] == 0 &&
        CHARACTER[69] == 0 && CHARACTER[67] == 0) {
        Ps("THE ");
        N = M;
        PRINT_NOUN();
        PL(" ATTACKS YOU!");
        CHARACTER[69] = M; NOUN[M][21] = 1;
    }
    if (CHARACTER[67] != 0 && CHARACTER[69] == M) CHARACTER[69] = 0;
    if (CHARACTER[69] == 0) NOUN[M][21] = 0;
    if (NOUN[M][21] == 1 && CHARACTER[69] == M) {
        A = RNDI(99) + 1;
        if (A < 45) goto L19880;
        if (NOUN[M][15] == 1) MKILL_SUB();
        if (NOUN[M][15] == 1) goto L19930;
    L19880:
        if (NOUN[M][13] != 0 && MODE == 1) MKILL_SUB();
        A = RNDI(99) + 1;
        if (NOUN[M][13] == 0 && A < 60) MPUMMEL_SUB();
        if (NOUN[M][13] == 0 && A > 59) MGRAPPL_SUB();
        if (NOUN[M][13] != 0 && MODE != 1) MPUMMEL_SUB();
    L19930:
        goto L20770;
    }
    /* 19950 search dead or unconscious bodies */
    if (P == NOUN[M][1]) {
        if (CHARACTER[67] == 2 || CHARACTER[67] == 3) {
            if (CHARACTER[67] == 3) {
                Ps("THE ");
                N = M;
                PRINT_NOUN();
                PL(" RAVAGES YOUR PARALYZED BODY FOR ANYTHING VALUABLE.");
            }
            for (I = 1; I <= NNUM; I++) {
                if (NOUN[I][1] == -1) {
                    NOUN[I][1] = -M;
                    N = I; E = -M;
                    CHARACTER[64] = 0; CHARACTER[65] = 0;
                    if (NOUN[I][9] == 3) NOUN[I][9] = 0;
                }
            }
            goto L20590;
            /* 20135 NOUN(M,15)=3 is unreachable in the original too */
        }
    }
    if (CHARACTER[67] != 0) goto L20590;        /* 20155 */
    /* 20160 can we pick something up off the ground? */
    for (I = 1; I <= NNUM; I++) {
        N1 = I;
        if (NOUN[M][1] == NOUN[I][1] && NOUN[I][2] == 1) {
            if (NOUN[I][9] != 0) {
                N = I; MF = 1;                  /* tell CHECK.BP we are not the player */
                CHECK_BP();
                if (ER == 1) goto L20390;
            }
            NOUN[I][1] = -M; NOUN[I][9] = 0;
            N = N1; E = -M;
            PUT_COMPLETE();
            if (P == NOUN[M][1]) {
                Ps("THE ");
                N = M;
                PRINT_NOUN();
                Ps(" PICKS UP THE ");
                N = N1;
                PRINT_NOUN();
                PL(".");
            }
        }
    L20390: ;
    }
    if (RNDI(99) + 1 < 51) {                    /* 20400 let's open a door */
        for (I = 1; I <= DOORNUM; I++) {
            if ((DOORS[I][1] == NOUN[M][1] && DOORS[I][6] != 1 &&
                 DOORS[I][5] != 1 && DOORS[I][5] != 2) ||
                (DOORS[I][3] == NOUN[M][1] && DOORS[I][6] != 1 &&
                 DOORS[I][5] != 1 && DOORS[I][5] != 2)) {
                MOV[DOORS[I][1]][DOORS[I][2]] = DOORS[I][3];
                MOV[DOORS[I][3]][DOORS[I][4]] = DOORS[I][1];
                DOORS[I][6] = 1;
                if (DOORS[I][1] == NOUN[M][1]) FACE = DOORS[I][2];
                if (DOORS[I][3] == NOUN[M][1]) FACE = DOORS[I][4];
                if (NOUN[M][1] == P) {
                    Ps("THE ");
                    N = M;
                    PRINT_NOUN();
                    Ps(" OPENS THE DOOR ON THE ");
                    Ps(DIRS[FACE >= 0 && FACE <= 10 ? FACE : 0]);
                    PL(" WALL.");
                    goto L20590;
                }
            }
        }
    }
L20590:
    NOUN[M][12] = NOUN[M][1];                   /* 20600 */
    for (A = 1; A <= 10; A += 1) {
        if (FACE != 0) A = FACE;                /* 20612 */
        if (FACE == 0 && RNDI(99) + 1 < 25) { A = RNDI(9) + 1; FACE = 1; }
        A1 = MOV[NOUN[M][1]][A >= 0 && A <= 10 ? (int)A : 0];
        if (A1 < 1 || !room_ok(A1)) goto L20750;  /* [port] see room_ok */
        NOUN[M][1] = A1;
        if (P == NOUN[M][12]) {
            N = M;
            Ps("THE ");
            PRINT_NOUN();
            Ps(" JUST WENT ");
            Ps(DIRS[A >= 0 && A <= 10 ? (int)A : 0]);
            PL(".");
        }
        goto L20755;
    L20750:
        if (FACE != 0) A = 10;
    }
L20755:
L20770:
    A = NOUN[M][10]; A1 = 0;                    /* 20770 pick the best weapon */
    for (I = 1; I <= NNUM; I++)
        if (NOUN[I][1] == -M && NOUN[I][4] > A) A1 = I;
    NOUN[M][13] = A1;
    FACE = 0;
}

/* ================================================================== */
/* 20830 DEFINE SEARCH.OBJ                                             */
/* ================================================================== */

static void SEARCH_OBJ(void)
{
    if (NOUN[PRS[5]][3] != 0 && NOUN[PRS[5]][19] == 0) {
        Ps("THE ");
        N = PRS[5];
        PRINT_NOUN();
        PL(" WON'T LET YOU DO THAT!");
        goto L21060;
    }
    if (NOUN[PRS[5]][1] == P)  E = P;
    if (NOUN[PRS[5]][1] == -1) E = -1;
    A = 0;
    PL("YOU FIND THE FOLLOWING:");
    for (I = 1; I <= NNUM; I++) {
        if (NOUN[I][1] == -PRS[5]) {
            NOUN[I][1] = P;
            A = A + 1;
            N = I;
            if (NOUN[I][9] != 0) goto L20990;
            PRINT_NOUN();
            PL(".");
        L20990:
            if (E == P) NOUN[I][1] = P;
            E = P; N = I;
            if (NOUN[I][5] != 0) PUT_COMPLETE();
        }
    }
    NL();
    if (A == 0) PL("NOTHING.");
L21060: ;
}

/* ================================================================== */
/* 22000 DEFINE LIGHT.MODE                                             */
/* ------------------------------------------------------------------ */
/* [original bug] lines 22080/22085 test L, which was just set to 0,    */
/* where the author clearly meant N (the location).  L is therefore     */
/* always 0 and the game is never dark -- which is just as well, since  */
/* LIGHT.IT below is never reachable.                                   */
/* ================================================================== */

static void LIGHT_MODE(void)
{
    L = 0;                                      /* 0 = lit, 1 = dark */
    if (L < 36) L = 0;
    if (L > 35) L = 1;
    for (I = 1; I <= NNUM; I++) {
        if (NOUN[I][9] != 0) {
            N = I;
            MF = 1;
            CHECK_BP();
            if (ER == 1) goto L22500;
        }
        if (NOUN[I][1] == -1 && NOUN[I][14] == 5 && NOUN[I][11] == 1) L = 0;
        if (NOUN[I][1] == N  && NOUN[I][14] == 5 && NOUN[I][11] == 1) L = 0;
        for (J = 1; J <= NNUM; J++)
            if (NOUN[J][1] == N && NOUN[I][1] == -J &&
                NOUN[I][11] == 1 && NOUN[I][14] == 5) L = 0;
    }
L22500: ;
}

/* ================================================================== */
/* 23000 DEFINE LIGHT.IT                                               */
/* [original] no VCODE dispatches here, so this is dead code in 1987   */
/* too.  Kept for completeness.                                        */
/* ================================================================== */

static void LIGHT_IT(void)
{
    if (PRS[1] < 1) {
        Z = 55;
        PRINTER_SUB();
        goto L23800;
    }
    E = -1;
    CHECK_EM();
    if (PRS[5] < 1) goto L23800;
    if (NOUN[PRS[5]][1] != -1) {
        Ps("YOU DON'T HAVE THE ");
        N = PRS[5];
        PRINT_NOUN();
        PL(".");
        goto L23800;
    }
    if (NOUN[PRS[5]][9] != 0) {
        N = PRS[5];
        CHECK_BP();
        if (ER == 1) goto L23800;
    }
    if (NOUN[PRS[5]][14] != 5) {
        PL("YOU CAN'T LIGHT THAT!");
        goto L23800;
    }
    PL("IT IS NOW ALIGHT.");
    NOUN[PRS[5]][9] = 1;
L23800: ;
}

/* ================================================================== */
/* 24000 DEFINE ENTER.SUB                                              */
/* ================================================================== */

static void ENTER_SUB(void)
{
    for (I = 1; I <= 10; I++) {
        if (MOV[P][I] > 0 && room_ok(MOV[P][I])) {   /* [port] see room_ok */
            P = MOV[P][I];
            Z = LOCS[P][7];
            PRINTER_SUB();
            OBJECT_PRINT();
            goto L24500;
        }
    }
    PL("ENTER WHAT?");
L24500: ;
}

/* ================================================================== */
/* character roll-up  [port addition -- see README]                    */
/* ------------------------------------------------------------------ */
/* No .GME character file and no generator for this format survived on  */
/* the tape (CHARC.TBA writes a different layout, for a different       */
/* game).  This writes a file in exactly the shape ADDPLAYER reads:     */
/* six strings, then 82 numbers, one per line.  The slot meanings are   */
/* taken from how SAVE.TBA itself uses them -- see DISPLAY at 19450.    */
/* ================================================================== */

static int roll_stat(void)
{
    return RNDI(4) + RNDI(4) + RNDI(4) + 3;     /* 3..12, mean 7.5 */
}

static void roll_up_character(const char *name)
{
    static const char *races[]  = { "HUMAN", "ELF", "DWARF", "HOBBIT" };
    static const char *classes[] = { "WARRIOR", "THIEF", "MONK", "MAGIC-USER" };
    static const char *ranks[]  = { "VETERAN", "ROGUE", "NOVICE", "PRESTIDIGITATOR" };
    char path[128];
    FILE *fp;
    char line[128];
    int cls = 0, race = 0, i, hp;

    NL();
    PL("NO CHARACTER FILE OF THAT NAME EXISTS.  ROLLING ONE UP.");
    NL();
    PL("WHAT RACE DO YOU WISH TO BE :");
    PL("HUMAN / ELF / DWARF / HOBBIT ");
    for (;;) {
        bas_input_line(line, sizeof line);
        for (i = 0; i < 4; i++)
            if (EQ(line, races[i])) { race = i; goto got_race; }
        PL("HUMAN / ELF / DWARF / HOBBIT ");
    }
got_race:
    NL();
    PL("WHICH CLASS WOULD YOU LIKE :");
    PL("WARRIOR / THIEF / MONK / MAGIC-USER ");
    for (;;) {
        bas_input_line(line, sizeof line);
        for (i = 0; i < 4; i++)
            if (EQ(line, classes[i])) { cls = i; goto got_class; }
        PL("WARRIOR / THIEF / MONK / MAGIC-USER ");
    }
got_class:
    memset(CHARACTER, 0, sizeof CHARACTER);
    CHARACTER[1] = roll_stat();                 /* STRENGTH    */
    CHARACTER[2] = roll_stat();                 /* DEXTERITY   */
    CHARACTER[3] = roll_stat();                 /* PERSONALITY */
    CHARACTER[4] = roll_stat();                 /* ENDURANCE   */
    CHARACTER[5] = roll_stat();                 /* INTELIGENCE */
    CHARACTER[6] = roll_stat();                 /* MANA        */
    CHARACTER[7] = roll_stat();                 /* LUCK        */
    if (cls == 0 && CHARACTER[1] < 8) CHARACTER[1] = 8;   /* warriors are strong */
    hp = 8 + CHARACTER[4];
    CHARACTER[8] = hp;                          /* HITS        */
    CHARACTER[9] = hp;                          /* HIT MAX     */
    CHARACTER[10] = 10 * (RNDI(12) + 1);        /* GOLD        */
    CHARACTER[58] = 10 + RNDI(31);              /* FIGHTING    */
    CHARACTER[59] = 5 + RNDI(36);               /* THIEVING    */
    CHARACTER[60] = 5 + RNDI(36);               /* TRADING     */
    CHARACTER[61] = 5 + RNDI(36);               /* MAGIC       */
    CHARACTER[62] = 5 + RNDI(36);               /* MECHANICS   */
    CHARACTER[63] = 10 + RNDI(31);              /* combat skill used by CROLL */
    CHARACTER[68] = (RNDI(100) < 12) ? 64 : 65; /* handedness  */
    CHARACTER[71] = 10;                         /* armor class, recomputed */

    snprintf(path, sizeof path, "%s.GME", name);
    fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "monastery: cannot create %s\n", path);
        exit(1);
    }
    fprintf(fp, "%s\n", name);                  /* CHARAC$(1) name      */
    fprintf(fp, "%s\n", classes[cls]);          /* CHARAC$(2) class     */
    fprintf(fp, "%s\n", "NEUTRAL");             /* CHARAC$(3) alignment */
    fprintf(fp, "%s\n", races[race]);           /* CHARAC$(4) race      */
    fprintf(fp, "%s\n", "");                    /* CHARAC$(5) password  */
    fprintf(fp, "%s\n", ranks[cls]);            /* CHARAC$(6) rank      */
    for (i = 1; i <= 82; i++)
        fprintf(fp, "%5d\n", CHARACTER[i]);
    fclose(fp);

    NL();
    Ps("SAVED AS "); Ps(path); NL();
    NL();
}

/* ================================================================== */
/* 100-570  main program                                               */
/* ================================================================== */

static void init_tables(void)
{
    int i, j;

    A = 20;                                     /* 100 */
    NNUM = A;                                   /* 110 */
    VNUM = 66; ANUM = 44; PNUM = 7; ADVERBN = 5;/* 120 */
    DOORNUM = 18;                               /* 130 */

    memcpy(MOV, DATA_MOV, sizeof MOV);          /* 290 MAT READ MOV */
    for (i = 0; i <= 66; i++) { VERBS[i] = DATA_VERB[i]; VCODE[i] = DATA_VCODE[i]; }
    for (i = 0; i <= 10; i++) DIRS[i] = DATA_DIR[i];
    for (i = 0; i <= 44; i++) ADJECTS[i] = DATA_ADJECT[i];
    for (i = 0; i <= 7;  i++) PREPS[i] = DATA_PREP[i];
    for (i = 0; i <= 5;  i++) ADVERBS[i] = DATA_ADVERB[i];
    for (i = 0; i <= 20; i++) {                 /* 370-420 */
        NOUNS[i] = DATA_NOUN[i];
        NOUN2S[i] = DATA_NOUN2[i];
        for (j = 0; j <= 24; j++)
            NOUN[i][j] = DATA_NOUN_V[i][j];
    }
    for (i = 1; i <= 20; i++)                   /* 430-450 */
        NHITSII[i] = NOUN[i][9];

    (void)LIGHT_IT;   /* dead in the original too; kept for completeness */
    (void)CNUM;

    READ_DESC();                                /* 460 */

    memcpy(LOCS, DATA_LOCS, sizeof LOCS);       /* 470-510 */
    memcpy(DOORS, DATA_DOORS, sizeof DOORS);    /* 520-560 */
}

int main(int argc, char **argv)
{
    int i;
    unsigned seed = 0;

    for (i = 1; i < argc; i++) {
        if (EQ(argv[i], "--strict")) {
            opt_strict = 1;
        } else if (EQ(argv[i], "--char") && i + 1 < argc) {
            opt_char = argv[++i];
        } else if (EQ(argv[i], "--text") && i + 1 < argc) {
            opt_text = argv[++i];
        } else if (EQ(argv[i], "--seed") && i + 1 < argc) {
            seed = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (EQ(argv[i], "--help") || EQ(argv[i], "-h")) {
            printf("The Monastery -- port of SAVE.TBA (Tymshare TYMCOM-X BASIC, 1987)\n"
                   "\n"
                   "  --char NAME   load NAME.GME instead of asking\n"
                   "  --text FILE   read room text from FILE instead of the built-in copy\n"
                   "  --seed N      fix the random seed (for reproducible transcripts)\n"
                   "  --strict      leave QUIT inert, exactly as the 1987 source had it\n"
                   "  --help        this message\n");
            return 0;
        } else {
            fprintf(stderr, "monastery: unknown option %s (try --help)\n", argv[i]);
            return 1;
        }
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    bas_seed(seed);
    init_tables();
    STARTUP();                                  /* 570 -- never returns */
    return 0;
}
