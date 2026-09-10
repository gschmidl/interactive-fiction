/* platform.c -- native replacements for the MVS/TSO externals the PL/I
 * engine calls, plus the record I/O that PL/I's FILE statements did.
 *
 * These mirror the PL/I port's own replacement routines one for one
 * (tapecave/port/tread.pli, twrite.pli, clrscrn.pli, randu.pli,
 * itime.pli, decdate.pli, r062a10.pli, warnmsg.pli, whisper.pli,
 * stgwr.pli), so the two builds behave the same way at the edges.
 */
#include "advent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- terminal input ------------------------------------------------
 * TREAD in the WELLPUT assembler (tapecave/DECDATE.txt) did a TPUT of
 * the prompt, a TGET of up to 133 bytes, and then
 *      GOTRTN  OC  0(133,R2),BLANKS   UPPER CASE THIS SUCKER
 * -- an EBCDIC fold to upper case.  Without it the engine only
 * understands commands typed in capitals.  CCODE is the 3270 PF-key
 * number, of which there is none here, so it is always 0.  End of file
 * on stdin has no mainframe counterpart (GETIN would spin forever), so
 * it ends the process.
 */
void TREAD(const char *prompt, fixed31 promptlen,
           char *msgarea, fixed31 *outlen, fixed31 *ccode)
{
    int c, n = 0;
    int sawany = 0;

    if (promptlen > 0)
        fwrite(prompt, 1, (size_t)promptlen, stdout);
    fflush(stdout);

    pl_blank(msgarea, 133);
    for (;;) {
        c = getchar();
        if (c == EOF)
            break;
        sawany = 1;
        if (c == '\n')
            break;
        if (c == '\r')
            continue;
        if (n < 133) {
            if (c >= 'a' && c <= 'z')
                c -= 'a' - 'A';
            msgarea[n++] = (char)c;
        }
    }
    if (!sawany && n == 0) {                /* terminal went away */
        putchar('\n');
        fflush(stdout);
        plat_exit(0);
    }
    *outlen = n;
    *ccode = 0;
}

/* ---- terminal output -----------------------------------------------
 * TWRITE was a single TPUT of MSGLEN bytes.  The one addition is the
 * code page: the game database is single-byte, as it was on the
 * mainframe, and four bytes in it fall outside ASCII -- X'4A' (cent
 * sign) in the "FEE FIE FOE FOO" and "WITT CONSTRUCTION COMPANY" signs
 * and X'5F' (logical not) in the wizard's SCAN help.  A 3270 turned
 * those into glyphs at the terminal, not in the dataset; this does the
 * same for a UTF-8 terminal.
 */
static int xlate8 = 1;

void TWRITE(const char *msg, fixed31 msglen, fixed31 *ccode)
{
    fixed31 i;
    for (i = 0; i < msglen; i++) {
        unsigned char c = (unsigned char)msg[i];
        if (c < 0x80 || !xlate8) {
            putchar(c);
        } else {
            putchar(0xC0 | (c >> 6));
            putchar(0x80 | (c & 0x3F));
        }
    }
    putchar('\n');
    if (ccode)
        *ccode = 0;
}

void CLRSCRN(void)
{
    fputs("\033[2J\033[H", stdout);
}

/* ---- RANDU ---------------------------------------------------------
 * Line for line the Fortran of tapecave/RANDU.txt (and the PL/I port's
 * randu.pli): the classic, famously bad, IBM RANDU generator.  The
 * multiply is allowed to wrap, which is what the PL/I port spells
 * (NOFIXEDOVERFLOW).
 */
void RANDU(fixed31 ix, fixed31 *iy, float *yr)
{
    int32_t y = (int32_t)((uint32_t)ix * 65539u);
    if (y < 0)
        y = (int32_t)((uint32_t)y + 2147483647u + 1u);
    *iy = y;
    *yr = (float)y;
    *yr = *yr * 0.4656613e-9f;
}

/* ITIME returned the S/370 timer in units masked to 0..511.  The engine
 * throws the value away -- the CALL RAN(1) inside the loop it drives is
 * commented out in the original source -- so the game is deterministic
 * either way. */
void ITIME(fixed31 *t)
{
    *t = (fixed31)(time(NULL) % 512);
}

/* DECDATE returned the current date as packed YYDDD.  Used only by the
 * "magic parm of the day" wizard puzzle. */
void DECDATE(fixed31 *yyddd)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    *yyddd = (fixed31)((lt->tm_year % 100) * 1000 + lt->tm_yday + 1);
}

void WHISPER(void)  { }                     /* 3270 dark-field masking */
void WARNMSG(const char *msgprms) { (void)msgprms; }
void R062A10(void)  { }                     /* MVS DYNALLOC */

/* PL/I DATETIME() -- 'YYYYMMDDHHMISS999'.  Local time, unlike Iron
 * Spring's builtin, which returns UTC regardless of TZ. */
void DATETIME(char *out17)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char buf[64];
    snprintf(buf, sizeof buf, "%04d%02d%02d%02d%02d%02d000",
             lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
             lt->tm_hour, lt->tm_min, lt->tm_sec);
    memcpy(out17, buf, 17);
}

/* PL/I TIME() -- 'HHMMSSTTT', local time. */
void PLI_TIME(char *out9)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char buf[16];
    snprintf(buf, sizeof buf, "%02d%02d%02d000",
             lt->tm_hour, lt->tm_min, lt->tm_sec);
    memcpy(out9, buf, 9);
}

fixed31 PLIRETV(void) { return 0; }


/* ---- SYSPRINT ------------------------------------------------------
 * PL/I's standard print file, which the game writes to only when the
 * wizard LOG command is on.  PUT SKIP ends the current line, so the
 * newline goes in front of the data and the last line is terminated
 * when the file is closed; buffering it and flushing at exit is what
 * the Iron Spring build does, and keeps the two transcripts comparable.
 */
static char  *sysbuf;
static size_t syslen, syscap;
static int    sysstarted;

static void sysapp(const char *s, size_t n)
{
    if (syslen + n > syscap) {
        size_t want = (syscap ? syscap * 2 : 4096);
        while (want < syslen + n) want *= 2;
        sysbuf = (char *)realloc(sysbuf, want);
        if (!sysbuf) return;
        syscap = want;
    }
    memcpy(sysbuf + syslen, s, n);
    syslen += n;
}

static const char NEWLINE[1] = { '\n' };

void SYSPRINT_skip(void)
{
    sysapp(NEWLINE, 1);        /* SKIP ends the current line, and the
                                * first one ends the (empty) line the
                                * file was positioned on */
    sysstarted = 1;
}

void SYSPRINT_put(const char *s, int n)
{
    if (n > 0) sysapp(s, (size_t)n);
}

void SYSPRINT_flush(void)
{
    if (sysstarted) sysapp(NEWLINE, 1);
    if (syslen) fwrite(sysbuf, 1, syslen, stdout);
    syslen = 0;
    sysstarted = 0;
}

/* ---- record files --------------------------------------------------
 * PL/I's RECORD files with a fixed RECSIZE.  READ advances one record;
 * REWRITE writes back the record just read, which is what the wizard's
 * ENCODE/DECODE/SAUCE/SEND/ADDUSER/DELUSER commands rely on.  (Those
 * commands silently no-op in the Iron Spring PL/I build, whose runtime
 * leaves REWRITE unimplemented; here they work.)
 */
typedef struct {
    FILE *f;
    int   reclen;
    long  lastrec;          /* 1-based number of the record just read */
} recfile;

static recfile objf = { NULL, 80, 0 };
static recfile srcf = { NULL, 80, 0 };
static recfile stgf = { NULL, 4800, 0 };

static int rf_open(recfile *r, const char *name, int update)
{
    r->lastrec = 0;
    r->f = fopen(name, update ? "r+b" : "rb");
    if (!r->f && update)
        r->f = fopen(name, "w+b");
    return r->f ? 0 : 1;
}

static int rf_read(recfile *r, void *buf, int len)
{
    unsigned char rec[4800];
    size_t n;
    if (!r->f)
        return 1;
    n = fread(rec, 1, (size_t)r->reclen, r->f);
    if (n == 0)
        return 1;
    if ((int)n < r->reclen)
        memset(rec + n, ' ', (size_t)(r->reclen - (int)n));
    r->lastrec++;
    memcpy(buf, rec, (size_t)(len < r->reclen ? len : r->reclen));
    return 0;
}

static void rf_write_at(recfile *r, long recnum, const void *buf, int len)
{
    unsigned char rec[4800];
    if (!r->f || recnum < 1)
        return;
    memset(rec, ' ', (size_t)r->reclen);
    memcpy(rec, buf, (size_t)(len < r->reclen ? len : r->reclen));
    if (fseek(r->f, (recnum - 1) * r->reclen, SEEK_SET) != 0)
        return;
    fwrite(rec, 1, (size_t)r->reclen, r->f);
    fflush(r->f);
    fseek(r->f, recnum * (long)r->reclen, SEEK_SET);
}

static void rf_close(recfile *r)
{
    if (r->f) {
        fclose(r->f);
        r->f = NULL;
    }
    r->lastrec = 0;
}

int OBJECT_open(void)
{
    if (rf_open(&objf, "OBJECT", 0)) {
        fprintf(stderr, "adventure: cannot open OBJECT (the game database)%c",
                '\n');
        plat_exit(2);
    }
    return 0;
}

int OBJECT_open_update(void)   { return rf_open(&objf, "OBJECT", 1); }
int OBJECT_read(char *card80)  { return rf_read(&objf, card80, 80); }
void OBJECT_rewrite(const char *card80)
{
    rf_write_at(&objf, objf.lastrec, card80, 80);
}
void OBJECT_close(void)        { rf_close(&objf); }

int SOURCE_open_update(void)   { return rf_open(&srcf, "SOURCE", 1); }
int SOURCE_read(char *card80)  { return rf_read(&srcf, card80, 80); }
void SOURCE_rewrite(const char *card80)
{
    rf_write_at(&srcf, srcf.lastrec, card80, 80);
}
void SOURCE_close(void)        { rf_close(&srcf); }

int STORAGE_open(void)         { return rf_open(&stgf, "STORAGE", 1); }
int STORAGE_read(void *buf, int len) { return rf_read(&stgf, buf, len); }
void STORAGE_write(fixed31 recnum, const void *buf, int len)
{
    rf_write_at(&stgf, recnum, buf, len);
}
void STORAGE_rewrite(const void *buf, int len)
{
    rf_write_at(&stgf, stgf.lastrec, buf, len);
}
/* OPEN FILE (STORAGE) OUTPUT: truncate and write from record 1. */
int STORAGE_open_output(void)
{
    rf_close(&stgf);
    stgf.f = fopen("STORAGE", "w+b");
    stgf.lastrec = 0;
    return stgf.f ? 0 : 1;
}
void STORAGE_append(const void *buf, int len)
{
    STORAGE_write((fixed31)(stgf.lastrec + 1), buf, len);
    stgf.lastrec++;
}
void STORAGE_close(void)       { rf_close(&stgf); }

void plat_exit(int code)
{
    SYSPRINT_flush();          /* PL/I closes SYSPRINT on termination */
    fflush(stdout);
    OBJECT_close();
    SOURCE_close();
    STORAGE_close();
    exit(code);
}

/* ---- COLS: FIXED BIN(15) BASED(ADDR(CARD)) -------------------------
 * The card image is a 370 object deck, so the halfwords REVERT negates
 * are big-endian.  (The PL/I port gets little-endian halfwords here
 * because Iron Spring runs on x86; it does not matter, because REVERT
 * is only ever applied to a card that some earlier pass negated the
 * same way, and the shipped OBJECT is already decoded.)
 */
int16_t COLS_get(int n)
{
    const unsigned char *p = (const unsigned char *)CARD + (n - 1) * 2;
    return (int16_t)((p[0] << 8) | p[1]);
}

void COLS_set(int n, int16_t v)
{
    unsigned char *p = (unsigned char *)CARD + (n - 1) * 2;
    p[0] = (unsigned char)((v >> 8) & 0xFF);
    p[1] = (unsigned char)(v & 0xFF);
}
