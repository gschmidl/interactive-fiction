/* advars.c -- definitions, PL/I INIT values, and the SUSPEND/RESTORE
 * serialisation of the ADVARS state.  Driven by advars.def.
 */
#include "advent.h"

#include <string.h>

/* ---- definitions ----------------------------------------------------- */

#define F_CHR(n, l)        char    n[l];
#define F_VCH(n, m)        vchar133 n;
#define F_I32(n, i)        fixed31 n;
#define F_I16(n, i)        fixed15 n;
#define F_FLT(n, i)        float   n;
#define F_BIT(n, i)        bit1    n;
#define F_I32A(n, c)       fixed31 n[(c) + 1];
#define F_BITA(n, c)       bit1    n[(c) + 1];
#define F_CHRA(n, c, l)    char    n[(c) + 1][l];
#define F_I32M(n, r, c)    fixed31 n[(r) + 1][(c) + 1];
#include "advars.def"
#undef F_CHR
#undef F_VCH
#undef F_I32
#undef F_I16
#undef F_FLT
#undef F_BIT
#undef F_I32A
#undef F_BITA
#undef F_CHRA
#undef F_I32M

/* ---- INIT ------------------------------------------------------------ */

void advars_init(void)
{
    int i_, j_;

#define F_CHR(n, l)        pl_blank(n, l);
#define F_VCH(n, m)        do { (n).len = 1; (n).s[0] = ' '; } while (0);
#define F_I32(n, i)        n = (i);
#define F_I16(n, i)        n = (i);
#define F_FLT(n, i)        n = (float)(i);
#define F_BIT(n, i)        n = (i);
#define F_I32A(n, c)       for (i_ = 0; i_ <= (c); i_++) n[i_] = 0;
#define F_BITA(n, c)       for (i_ = 0; i_ <= (c); i_++) n[i_] = 0;
#define F_CHRA(n, c, l)    for (i_ = 0; i_ <= (c); i_++) pl_blank(n[i_], l);
#define F_I32M(n, r, c)    for (i_ = 0; i_ <= (r); i_++) \
                               for (j_ = 0; j_ <= (c); j_++) n[i_][j_] = 0;
#include "advars.def"
#undef F_CHR
#undef F_VCH
#undef F_I32
#undef F_I16
#undef F_FLT
#undef F_BIT
#undef F_I32A
#undef F_BITA
#undef F_CHRA
#undef F_I32M

    /* the one array with a non-uniform INIT */
    DLOC[1] = 19; DLOC[2] = 27; DLOC[3] = 33;
    DLOC[4] = 44; DLOC[5] = 64; DLOC[6] = 114;

    pl_assign(VERSION, sizeof VERSION, LIT("V4.0"));
}

/* ---- serialisation ---------------------------------------------------
 * Packed into the front of a 4800-byte STORAGE record, in declaration
 * order, arrays as their PL/I 1..n elements only.  The layout is this
 * port's own -- the PL/I build's records are not interchangeable, since
 * PL/I packs BIT(1) into bits and prefixes VARYING with a halfword --
 * but the record size and the leading NAME || USERID are the same,
 * which is all the engine itself depends on.
 *
 * (ADVARS's 29-word ZZZZZZ "SPARE FILLER" is the one field left out; it
 * is never referenced anywhere in adventure.pli.)
 */
#define PUTB(p, n)  do { if (off + (n) <= 4800) memcpy(rec + off, (p), (size_t)(n)); \
                         off += (n); } while (0)
#define GETB(p, n)  do { if (off + (n) <= 4800) memcpy((p), rec + off, (size_t)(n)); \
                         off += (n); } while (0)

int advars_pack(char *rec)
{
    int off = 0, i_;

#define F_CHR(n, l)        PUTB(n, l);
#define F_VCH(n, m)        PUTB(&(n).len, 4); PUTB((n).s, m);
#define F_I32(n, i)        PUTB(&n, 4);
#define F_I16(n, i)        PUTB(&n, 2);
#define F_FLT(n, i)        PUTB(&n, 4);
#define F_BIT(n, i)        PUTB(&n, 1);
#define F_I32A(n, c)       PUTB(&n[1], 4 * (c));
#define F_BITA(n, c)       PUTB(&n[1], (c));
#define F_CHRA(n, c, l)    PUTB(n[1], (c) * (l));
#define F_I32M(n, r, c)    for (i_ = 1; i_ <= (r); i_++) PUTB(&n[i_][1], 4 * (c));
#include "advars.def"
#undef F_CHR
#undef F_VCH
#undef F_I32
#undef F_I16
#undef F_FLT
#undef F_BIT
#undef F_I32A
#undef F_BITA
#undef F_CHRA
#undef F_I32M

    return off;
}

void advars_unpack(const char *rec)
{
    int off = 0, i_;

#define F_CHR(n, l)        GETB(n, l);
#define F_VCH(n, m)        GETB(&(n).len, 4); GETB((n).s, m);
#define F_I32(n, i)        GETB(&n, 4);
#define F_I16(n, i)        GETB(&n, 2);
#define F_FLT(n, i)        GETB(&n, 4);
#define F_BIT(n, i)        GETB(&n, 1); n = (bit1)(n == 1);
#define F_I32A(n, c)       GETB(&n[1], 4 * (c));
#define F_BITA(n, c)       GETB(&n[1], (c));                            for (i_ = 1; i_ <= (c); i_++) n[i_] = (bit1)(n[i_] == 1);
#define F_CHRA(n, c, l)    GETB(n[1], (c) * (l));
#define F_I32M(n, r, c)    for (i_ = 1; i_ <= (r); i_++) GETB(&n[i_][1], 4 * (c));
#include "advars.def"
#undef F_CHR
#undef F_VCH
#undef F_I32
#undef F_I16
#undef F_FLT
#undef F_BIT
#undef F_I32A
#undef F_BITA
#undef F_CHRA
#undef F_I32M
}

/* ---- globals outside ADVARS ------------------------------------------ */

char     CARD[80];
char     MSGPRMS[16];
char     TSOID[8];
char     TEMPNAME[8];
char     SEQ_N[4];
char     DATE_STG[17];
char     PUTDATE[10];
char     PUTHHMM[5];
fixed31  YYDDD;
char     YDCHR[5];
int32_t  TEST_WORD;
char     LOC_LINE[30];
char     TIMES[5][4];
char     INHIB[1];
char     TTYPE[1];
vchar133 RETRYSTR;
char     ADVREC[4800];
fixed31  STGREC;
fixed15  INTEGER;
char     QUOTE[1];
char     MACROTIME[24];
char     XLATETO[26];
char     XLATEFR[26];
vchar100 INPARM;

fixed31  CTEXT[13];
fixed31  LTEXT[151];
fixed31  MTEXT[56];
fixed31  PTEXT[101];
fixed31  RTEXT[231];
fixed31  STEXT[151];
fixed31  TRAVEL[801];
char     LINES[11201][5];

fixed31  KTAB[331];
char     ATAB[331][5];
fixed31  TABSIZ;

fixed31  ACTSPK[51];

fixed31  COND[151];
fixed31  FIXD[101];
fixed31  KEY[151];
fixed31  PLAC[101];

const char *const WEEK_DAY[7] = {
    "SUNDAY   ", "MONDAY   ", "TUESDAY  ", "WEDNESDAY",
    "THURSDAY ", "FRIDAY   ", "SATURDAY "
};

const char *const CMONTH[13] = {
    "", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
    "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

jmp_buf pli_goto;

/* The statics the PL/I initialises in its DECLARE section. */
void statics_init(void)
{
    int i;

    pl_blank(CARD, sizeof CARD);
    pl_blank(MSGPRMS, sizeof MSGPRMS);
    pl_blank(TSOID, sizeof TSOID);
    pl_blank(TEMPNAME, sizeof TEMPNAME);
    memcpy(SEQ_N, "0000", 4);
    memset(DATE_STG, '0', sizeof DATE_STG);
    memcpy(PUTDATE, "    /  /  ", 10);
    memcpy(PUTHHMM, "  :  ", 5);
    YYDDD = 0;
    memcpy(YDCHR, "00000", 5);
    TEST_WORD = 0;
    pl_blank(LOC_LINE, sizeof LOC_LINE);
    for (i = 0; i < 5; i++)
        memcpy(TIMES[i], "0000", 4);
    INHIB[0] = 'N';
    TTYPE[0] = 'T';
    RETRYSTR.len = 1;
    RETRYSTR.s[0] = ' ';
    pl_blank(ADVREC, sizeof ADVREC);
    STGREC = 0;
    INTEGER = 0;
    QUOTE[0] = '\'';
    pl_assign(MACROTIME, sizeof MACROTIME, LIT("unknown"));
    memcpy(XLATETO, "abcdefghijklmnopqrstuvwxyz", 26);
    memcpy(XLATEFR, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);

    for (i = 0; i <= 12; i++)  CTEXT[i] = 0;
    for (i = 0; i <= 150; i++) LTEXT[i] = 0;
    for (i = 0; i <= 55; i++)  MTEXT[i] = 0;
    for (i = 0; i <= 100; i++) PTEXT[i] = 0;
    for (i = 0; i <= 230; i++) RTEXT[i] = 0;
    for (i = 0; i <= 150; i++) STEXT[i] = 0;
    for (i = 0; i <= 800; i++) TRAVEL[i] = 0;
    for (i = 0; i <= 11200; i++) pl_blank(LINES[i], 5);

    for (i = 0; i <= 330; i++) { KTAB[i] = 0; pl_blank(ATAB[i], 5); }
    TABSIZ = 330;

    for (i = 0; i <= 50; i++)  ACTSPK[i] = 0;

    for (i = 0; i <= 150; i++) { COND[i] = 0; KEY[i] = 0; }
    for (i = 0; i <= 100; i++) { FIXD[i] = 0; PLAC[i] = 0; }
}
