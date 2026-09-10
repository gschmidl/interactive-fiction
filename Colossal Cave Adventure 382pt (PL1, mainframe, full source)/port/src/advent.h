/* advent.h -- state of the SHARE/CBT "Adventure - PL/1 Version 4.0"
 * engine, transliterated from the DECLARE section of adventure.pli
 * (lines 49-470).
 *
 * The PL/I put every one of these in the scope of the outer PROGRAM
 * procedure, where an internal procedure that declared a local of the
 * same name shadowed it -- VOCAB's I, SPEAK's L, LIQLOC's LOC and so on
 * all do.  They are therefore ordinary globals here, not struct
 * members, so C's own shadowing reproduces that.
 *
 * The two PL/I identifiers containing '#' become DAY_N and SUSPEND_N;
 * SEQ# / CARD# become SEQ_N / CARD_N.  That is the only renaming.
 */
#ifndef ADVENT_H
#define ADVENT_H

#include "plisup.h"
#include <string.h>

/* ---- ADVARS: the live game state, saved and restored by SUSPEND ----
 * NAME and USERID stay first: PUTBACK and SCANSTG identify a saved game
 * by comparing SUBSTR(ADVREC,1,16) against NAME || USERID on the raw
 * 4800-byte record.
 */
#define F_CHR(n, l)        extern char    n[l];
#define F_VCH(n, m)        extern vchar133 n;
#define F_I32(n, i)        extern fixed31 n;
#define F_I16(n, i)        extern fixed15 n;
#define F_FLT(n, i)        extern float   n;
#define F_BIT(n, i)        extern bit1    n;
#define F_I32A(n, c)       extern fixed31 n[(c) + 1];
#define F_BITA(n, c)       extern bit1    n[(c) + 1];
#define F_CHRA(n, c, l)    extern char    n[(c) + 1][l];
#define F_I32M(n, r, c)    extern fixed31 n[(r) + 1][(c) + 1];
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

void advars_init(void);                 /* apply the PL/I INIT values  */
int  advars_pack(char *rec4800);        /* returns bytes used          */
void advars_unpack(const char *rec4800);

/* ---- globals outside ADVARS (not part of a saved game) -------------- */

extern char     CARD[80];
extern char     MSGPRMS[16];
extern char     TSOID[8];
extern char     TEMPNAME[8];
extern char     SEQ_N[4];               /* SEQ#; CARD# is a PICTURE over it */
extern char     DATE_STG[17];
extern char     PUTDATE[10];
extern char     PUTHHMM[5];
extern fixed31  YYDDD;
extern char     YDCHR[5];
extern int32_t  TEST_WORD;              /* PW is a CHARACTER(4) overlay */
extern char     LOC_LINE[30];
extern char     TIMES[5][4];            /* TIMES(4), 1-based */
extern char     INHIB[1];
extern char     TTYPE[1];
extern vchar133 RETRYSTR;
extern char     ADVREC[4800];
extern fixed31  STGREC;
extern fixed15  INTEGER;                /* BYTE_NUM is its low-order byte */
extern char     QUOTE[1];
extern char     MACROTIME[24];
extern char     XLATETO[26];
extern char     XLATEFR[26];
extern vchar100 INPARM;

/* TXTCOM */
extern fixed31  CTEXT[13];
extern fixed31  LTEXT[151];
extern fixed31  MTEXT[56];
extern fixed31  PTEXT[101];
extern fixed31  RTEXT[231];
extern fixed31  STEXT[151];
extern fixed31  TRAVEL[801];
extern char     LINES[11201][5];

/* VOCCOM */
extern fixed31  KTAB[331];
extern char     ATAB[331][5];
extern fixed31  TABSIZ;

extern fixed31  ACTSPK[51];

/* PLACOM */
extern fixed31  COND[151];
extern fixed31  FIXD[101];
extern fixed31  KEY[151];
extern fixed31  PLAC[101];

extern const char *const WEEK_DAY[7];
extern const char *const CMONTH[13];

/* ---- PICTURE and DEFINED overlays -----------------------------------
 * adventure.pli declares these as "DEFINED DATE_STG POS(n)".  As
 * accessors they keep the two places where the engine deliberately
 * writes THROUGH an overlay to update DATE_STG itself visible at the
 * call site: TIME_CHR = TIME() in the scoring code, and
 * PIC_HHMM = HHMM in SCANSTG.
 */
#define DATE_PIC        pl_pic_get(DATE_STG, 8)
#define DATE_PIC_SET(v) pl_pic_put(DATE_STG, 8, (v))
#define PIC_YEAR        pl_pic_get(DATE_STG, 4)
#define PIC_MONTH       pl_pic_get(DATE_STG + 4, 2)
#define PIC_DAY         pl_pic_get(DATE_STG + 6, 2)
#define PIC_HHMM        pl_pic_get(DATE_STG + 8, 4)
#define PIC_HHMM_SET(v) pl_pic_put(DATE_STG + 8, 4, (v))
#define PIC_HH          pl_pic_get(DATE_STG + 8, 2)
#define PIC_MM          pl_pic_get(DATE_STG + 10, 2)
#define TIME_CHR_SET(p) memcpy(DATE_STG + 8, (p), 9)
#define CHR_HHMM        (DATE_STG + 8)          /* CHARACTER(4) */

#define CARD_N          pl_pic_get(SEQ_N, 4)
#define CARD_N_SET(v)   pl_pic_put(SEQ_N, 4, (v))

#define YDPIC           pl_pic_get(YDCHR, 5)
#define YDPIC_SET(v)    pl_pic_put(YDCHR, 5, (v))
#define DIGIT           pl_pic_get(YDCHR, 1)

/* PIC_VECTOR(n).PLOC is PICTURE'999' at LOC_LINE[(n-1)*5]. */
#define PLOC_SET(n, v)  pl_pic_put(LOC_LINE + ((n) - 1) * 5, 3, (v))

/* COLS(n): FIXED BIN(15) BASED(ADDR(CARD)) -- big-endian halfwords, the
 * 370 object-deck layout that REVERT undoes. */
int16_t COLS_get(int n);
void    COLS_set(int n, int16_t v);

/* PW: CHARACTER(4) BASED(ADDR(TEST_WORD)), the obfuscation overlay. */
#define PW ((char *)&TEST_WORD)

/* HEX02: CHARACTER(1) BASED(ADDR(BITPAT)) where BITPAT = '00000010'B. */
#define HEX02 '\002'

/* ---- non-local GO TO ------------------------------------------------
 * Three labels in the PROGRAM body are the target of a GO TO from
 * inside an internal procedure (BUG, CIAO, WIZPROC, DEMOCHK, PUTBACK,
 * SCANSTG).  C cannot goto across a function, so these unwind.
 */
#include <setjmp.h>
extern jmp_buf pli_goto;
#define JMP_DEALLOC 1
#define JMP_L31     2
#define JMP_L20000  3

/* ---- the engine ------------------------------------------------------ */

void PROGRAM(void);

bit1    TOTING(fixed31 OBJ);
bit1    HERE(fixed31 OBJ);
bit1    AT(fixed31 OBJ);
fixed31 LIQ(fixed31 DUMMY);
fixed31 LIQ2(fixed31 PBOTL);
fixed31 LIQLOC(fixed31 LOC);
bit1    BITSET(fixed31 L, fixed31 N);
bit1    FORCED(fixed31 LOC);
bit1    DARK(fixed31 DUMMY);
bit1    PCT(fixed31 N);
void    SPEAK(fixed31 N);
void    PSPEAK(fixed31 MSG, fixed31 SKIP);
void    RSPEAK(fixed31 I);
void    MSPEAK(fixed31 I);
void    GETIN(char *WORD1, char *WORD1X, char *WORD2, char *WORD2X);
bit1    YES(fixed31 X, fixed31 Y, fixed31 Z);
void    A5TOA1(const char *A, const char *B, char CHARS[][1], fixed31 *LENG);
fixed31 VOCAB(const char *ID, fixed31 INIT);
void    DSTROY(fixed31 OBJECT);
void    JUGGLE(fixed31 OBJECT);
void    MOVE_(fixed31 OBJECT, fixed31 WHERE);   /* MOVE: clashes with nothing,
                                                 * but keep the trailing _ so
                                                 * the verb label MOVE stays
                                                 * free in PROGRAM */
fixed31 PUT_(fixed31 OBJECT, fixed31 WHERE, fixed31 PVAL);
void    CARRY(fixed31 OBJECT, fixed31 WHERE);
void    DROP(fixed31 OBJECT, fixed31 WHERE);
void    CIAO(void);
void    BUG(fixed31 NUM);
void    LINESKP(void);
void    LINEOUT(void);
fixed31 RAN(fixed31 N);
void    HOURFMT(void);
void    REVERT(void);
void    WIZPROC(void);
void    DEMOCHK(void);
void    PUTBACK(void);
void    GETMSGS(const char *UID, char X);
void    THISDAY(vchar133 *RSTRING);
void    SCANSTG(void);

/* ---- platform layer (replaces the MVS/TSO externals) ---------------- */

void TREAD(const char *prompt, fixed31 promptlen,
           char *msgarea, fixed31 *outlen, fixed31 *ccode);
void TWRITE(const char *msg, fixed31 msglen, fixed31 *ccode);
void CLRSCRN(void);
void RANDU(fixed31 ix, fixed31 *iy, float *yr);
void ITIME(fixed31 *t);
void DECDATE(fixed31 *yyddd);
void WHISPER(void);
void WARNMSG(const char *msgprms);
void R062A10(void);
void DATETIME(char *out17);
void PLI_TIME(char *out9);      /* PL/I TIME() builtin: HHMMSSTTT */
fixed31 PLIRETV(void);

int  OBJECT_open(void);
int  OBJECT_open_update(void);
int  OBJECT_read(char *card80);
void OBJECT_rewrite(const char *card80);
void OBJECT_close(void);
int  SOURCE_open_update(void);
int  SOURCE_read(char *card80);
void SOURCE_rewrite(const char *card80);
void SOURCE_close(void);
int  STORAGE_open(void);
int  STORAGE_open_output(void);
int  STORAGE_read(void *buf, int len);
void STORAGE_write(fixed31 recnum, const void *buf, int len);
void STORAGE_rewrite(const void *buf, int len);
void STORAGE_append(const void *buf, int len);
void STORAGE_close(void);
void SYSPRINT_skip(void);
void SYSPRINT_put(const char *s, int n);
void SYSPRINT_flush(void);
void plat_exit(int code);

#endif /* ADVENT_H */
