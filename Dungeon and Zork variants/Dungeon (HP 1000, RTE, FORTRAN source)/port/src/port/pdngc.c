/*
 * pdngc.c - the C half of the Dungeon (HP 1000 RTE) port's run-time.
 *
 *  - the terminal: the program's standard output (gfortran unit 6, and
 *    REIO) goes through a pipe to a thread that does what RTE's terminal
 *    driver did - a record ending in '_' is not followed by CR LF - and
 *    input is read a line at a time, 7 bits, in capitals;
 *  - the clock (EXEC 11), held still by --time;
 *  - the segment linker DLINK/DLIN2/RETRN (#DLINK), the character routines
 *    A2A1/A1A2 (#A2A1, #A1A2), UMOVE (#UMOVE), and CNUMD, KCVT, SSEED and
 *    URAN from HP's libraries;
 *  - FMP files as files of word records: the data base beside the
 *    program, saved games in saves\.
 *
 * Words hold two characters, the first one first in memory, as the
 * converted program's Hollerith constants have them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

typedef short w16;

/* ------------------------------------------------------------------ */
/* output filter                                                       */

static HANDLE hreal = INVALID_HANDLE_VALUE;
static int realcon = 0, incon = 0;
static int pipefd[2] = {-1, -1};
static HANDLE thr = NULL;
static unsigned char obuf[8192];
static int on = 0;

static void oflush(void)
{
    DWORD k;
    if (on) WriteFile(hreal, obuf, (DWORD)on, &k, NULL);
    on = 0;
}

static void oput(const char *s, int n)
{
    while (n-- > 0) {
        if (on == (int)sizeof obuf) oflush();
        obuf[on++] = (unsigned char)*s++;
    }
}

/* The HP 264x escape sequences the game uses, as ANSI; '&' sequences:
   &d display enhancement, &f softkey definitions (swallowed). */
enum { S_TEXT, S_ESC, S_AMP, S_AMPPAR, S_SKIP, S_US, S_USCR };
static int st = S_TEXT;
static char grp;                  /* letter after & */
static int par, parsign, skipn;
static int flabel, fstring;       /* &f label and string lengths */

static void enhance(int c)
{
    /* &d@ normal, A blink, B inverse, D underline, H half bright;
       the letters from @ to O are these bits */
    char s[32];
    int v = c - '@';
    strcpy(s, "\x1b[0");
    if (v & 1) strcat(s, ";5");
    if (v & 2) strcat(s, ";7");
    if (v & 4) strcat(s, ";4");
    if (v & 8) strcat(s, ";2");
    strcat(s, "m");
    if (realcon) oput(s, (int)strlen(s));
}

static void esc(int c)
{
    const char *m = NULL;
    switch (c) {
    case 'A': m = "\x1b[A"; break;
    case 'B': m = "\x1b[B"; break;
    case 'C': m = "\x1b[C"; break;
    case 'D': m = "\x1b[D"; break;
    case 'H': case 'h': m = "\x1b[H"; break;
    case 'J': m = "\x1b[J"; break;
    case 'K': m = "\x1b[K"; break;
    case '@':                     /* delay one second */
        oflush();
        if (realcon) Sleep(1000);
        return;
    default: return;              /* memory lock (l, m) and the rest */
    }
    if (realcon) oput(m, (int)strlen(m));
}

static void filter(const unsigned char *b, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int c = b[i];
        switch (st) {
        case S_TEXT:
            if (c == 0x1b) st = S_ESC;
            else if (c == '_') st = S_US;
            else if (c == 0 || c == 0x7f) ;      /* the 264x ignores these */
            else oput((const char *)&b[i], 1);
            break;
        case S_US:                /* '_' then CR LF: no new line */
            if (c == '\r') st = S_USCR;
            else if (c == '_') oput("_", 1);
            else { oput("_", 1); st = S_TEXT; i--; }
            break;
        case S_USCR:
            if (c == '\n') st = S_TEXT;
            else { oput("_\r", 2); st = S_TEXT; i--; }
            break;
        case S_ESC:
            if (c == '&') { st = S_AMP; break; }
            esc(c);
            st = S_TEXT;
            break;
        case S_AMP:
            grp = (char)c;
            par = 0; parsign = 1; flabel = fstring = 0;
            st = S_AMPPAR;
            break;
        case S_AMPPAR:
            if (c >= '0' && c <= '9') { par = par * 10 + c - '0'; break; }
            if (c == '-') { parsign = -1; break; }
            if (c == '+' || c == ' ') break;
            if (grp == 'd') {                 /* &d<letter> */
                enhance(c);
                st = S_TEXT;
                break;
            }
            if (grp == 'f') {
                if (c == 'd' || c == 'D') flabel = par * parsign;
                if (c == 'l' || c == 'L') fstring = par * parsign;
            }
            par = 0; parsign = 1;
            if (c >= 'A' && c <= 'Z') {       /* the last parameter */
                skipn = (grp == 'f') ? (flabel > 0 ? flabel : 0) +
                                       (fstring > 0 ? fstring : 0) : 0;
                st = skipn ? S_SKIP : S_TEXT;
            }
            break;
        case S_SKIP:
            if (--skipn == 0) st = S_TEXT;
            break;
        }
    }
}

static DWORD WINAPI filter_thread(LPVOID p)
{
    unsigned char b[4096];
    int n;
    (void)p;
    while ((n = _read(pipefd[0], b, sizeof b)) > 0) {
        filter(b, n);
        oflush();
    }
    oflush();
    return 0;
}

void pioini_(void)
{
    DWORD mode;
    int realfd;
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    realfd = _dup(1);
    hreal = (HANDLE)_get_osfhandle(realfd);
    if (GetConsoleMode(hreal, &mode)) {
        realcon = 1;
        SetConsoleMode(hreal, mode | 0x0004);      /* VT processing */
        SetConsoleOutputCP(1252);
    }
    if (GetConsoleMode(hin, &mode)) incon = 1;
    _setmode(_fileno(stdin), _O_BINARY);
    if (_pipe(pipefd, 65536, _O_BINARY) != 0) return;
    _dup2(pipefd[1], 1);
    _close(pipefd[1]);
    thr = CreateThread(NULL, 0, filter_thread, NULL, 0, NULL);
}

/* after FLUSH(6): close the pipe and let the filter finish */
void pfini_(void)
{
    if (thr) {
        _close(1);
        WaitForSingleObject(thr, INFINITE);
        thr = NULL;
    }
}

/* the directory the program lives in */
void pgmdir_(char *buf, int *n, int buflen)
{
    char p[MAX_PATH];
    char *s;
    GetModuleFileNameA(NULL, p, MAX_PATH);
    s = strrchr(p, '\\');
    if (s) *s = 0; else strcpy(p, ".");
    memset(buf, ' ', (size_t)buflen);
    *n = (int)strlen(p);
    if (*n > buflen) *n = buflen;
    memcpy(buf, p, (size_t)*n);
}

/* raw bytes into the output stream (REIO, the echo of piped input) */
void pwrite_(const char *s, const int *n, int slen)
{
    (void)slen;
    if (*n > 0) _write(1, s, (unsigned)*n);
}

/* ------------------------------------------------------------------ */
/* terminal input                                                      */

/* A line into buf (blank-filled to buflen): 7 bits, a tab a blank, other
   control characters dropped, folded to upper case - the game's words are
   all capitals, as the terminals' caps lock had them.  -1 at end of file.
   Piped input is echoed, as the terminal would have. */
int pread_(char *buf, int buflen)
{
    int c, n = 0;
    memset(buf, ' ', (size_t)buflen);
    for (;;) {
        c = getchar();
        if (c == EOF) {
            if (n == 0) return -1;
            break;
        }
        c &= 0x7F;
        if (c == '\n') break;
        if (c == '\r') continue;
        if (c == '\t') c = ' ';
        if (c < ' ' || c == 0x7F) continue;
        c = toupper(c);
        if (n < buflen) buf[n] = (char)c;
        n++;
    }
    if (n > buflen) n = buflen;
    if (!incon) {
        _write(1, buf, (unsigned)n);
        _write(1, "\r\n", 2);
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* the clock                                                           */

static int held = 0, hh, hm, hs, hms, dated = 0, dy, dm, dd;

void pclkset_(const int *which, const int *a, const int *b, const int *c,
              const int *d)
{
    if (*which == 1) { dated = 1; dy = *a; dm = *b; dd = *c; }
    else { held = 1; hh = *a; hm = *b; hs = *c; hms = *d; }
}

/* EXEC 11: tens of milliseconds, seconds, minutes, hours, day of year */
void ptime_(w16 *t)
{
    SYSTEMTIME s;
    struct tm u;
    int ms;
    GetLocalTime(&s);
    memset(&u, 0, sizeof u);
    u.tm_year = s.wYear - 1900; u.tm_mon = s.wMonth - 1; u.tm_mday = s.wDay;
    u.tm_hour = s.wHour; u.tm_min = s.wMinute; u.tm_sec = s.wSecond;
    ms = s.wMilliseconds;
    if (dated) { u.tm_year = dy - 1900; u.tm_mon = dm - 1; u.tm_mday = dd; }
    if (held) { u.tm_hour = hh; u.tm_min = hm; u.tm_sec = hs; ms = hms; }
    {
        struct tm v = u;
        v.tm_hour = 12; v.tm_isdst = -1;
        mktime(&v);
        t[4] = (w16)(v.tm_yday + 1);
    }
    t[0] = (w16)(ms / 10);
    t[1] = (w16)u.tm_sec;
    t[2] = (w16)u.tm_min;
    t[3] = (w16)u.tm_hour;
}


/* ------------------------------------------------------------------ */
/* the terminal: REIO                                                  */

extern void pstop_(void);

/* REIO write: n characters of buf, then CR LF (the filter drops both
   after a closing '_', as RTE's terminal driver did) */
void preiow_(const char *buf, const int *n)
{
    if (*n > 0) _write(1, buf, (unsigned)*n);
    _write(1, "\r\n", 2);
}

/* REIO read: a line into at most n characters of buf; what the line does
   not reach is left as it was, except that an odd count fills the other
   half of its last word with a blank.  The end of piped input ends the
   game, which would otherwise ask for ever. */
void preior_(char *buf, const int *n)
{
    char line[256];
    int k = pread_(line, (int)sizeof line), m;
    if (k < 0) pstop_();
    m = k < *n ? k : *n;
    memcpy(buf, line, (size_t)m);
    if (m & 1) buf[m] = ' ';
}

/* ------------------------------------------------------------------ */
/* the segment linker (#DLINK)                                         */

/* COMMON /SEG/ IZP1,IZP2,IZP3,IZRESZ */
extern w16 seg_[4];
extern void dungb_(void), dungc_(void), dungd_(void), dunge_(void),
    dungf_(void);

static void *jb[5];

static void segment(const w16 *name)
{
    char n[7];
    int i;
    for (i = 0; i < 3; i++) {
        n[2 * i] = (char)(name[i] & 0xFF);
        n[2 * i + 1] = (char)((name[i] >> 8) & 0xFF);
    }
    n[6] = 0;
    if (strncmp(n, "DUNG", 4) != 0) {
        fprintf(stderr, "dungeon: no segment '%s'\n", n);
        pstop_();
    }
    switch (n[4]) {
    case 'B': dungb_(); break;
    case 'C': dungc_(); break;
    case 'D': dungd_(); break;
    case 'E': dunge_(); break;
    case 'F': dungf_(); break;
    default:
        fprintf(stderr, "dungeon: no segment '%s'\n", n);
        pstop_();
    }
}

/* CALL DLINK(SEGMENT, P1, P2, P3): the parameters go to COMMON /SEG/ by
   value and the segment runs; its CALL RETRN comes back here.  The
   parameters may be 16-bit variables or literals: read as 16 bits. */
void dlink_(const w16 *name, const w16 *p1, const w16 *p2, const w16 *p3)
{
    seg_[0] = *p1; seg_[1] = *p2; seg_[2] = *p3;
    if (__builtin_setjmp(jb) == 0)
        segment(name);
}

/* CALL DLIN2(...): the same from inside a segment, without a new return
   point - the next RETRN goes back to the main program's DLINK, past the
   segment that called DLIN2 (which the original overlaid). */
void dlin2_(const w16 *name, const w16 *p1, const w16 *p2, const w16 *p3)
{
    seg_[0] = *p1; seg_[1] = *p2; seg_[2] = *p3;
    segment(name);
    __builtin_longjmp(jb, 1);
}

void retrn_(void)
{
    __builtin_longjmp(jb, 1);
}

/* RMPAR: RU,DUNGN from a session - parameter 1 is the terminal, LU 1 */
void rmpar_(w16 *ip)
{
    ip[0] = 1; ip[1] = 0; ip[2] = 0; ip[3] = 0; ip[4] = 0;
}

/* ------------------------------------------------------------------ */
/* characters.  A word holds two, the first first in memory here (on the
   HP the first was the high byte), so byte offsets carry over as they
   are.  A1 form is one character to a word, then a blank.             */

/* A2A1 (#A2A1): n packed characters of buf to A1 form, in place */
void a2a1_(char *buf, const w16 *n)
{
    int k;
    for (k = *n - 1; k >= 0; k--) {
        char c = buf[k];
        buf[2 * k] = c;
        buf[2 * k + 1] = ' ';
    }
}

/* A1A2 (#A1A2): n characters in A1 form to packed, in place */
void a1a2_(char *buf, const w16 *n)
{
    int k;
    for (k = 0; k < *n; k++) buf[k] = buf[2 * k];
}

/* UMOVE (#UMOVE, contribution F017): n bytes from byte soff of src to
   byte doff of dst, one at a time from the first, as MBT moved them */
void umove_(const w16 *n, const char *src, const w16 *soff, char *dst,
            const w16 *doff)
{
    int k;
    for (k = 0; k < *n; k++) dst[*doff + k] = src[*soff + k];
}

/* DECODE (80,f,IBUFF) with 80A1 / 78A1: one character to a word */
void punpk1_(const char *src, w16 *out, const w16 *n)
{
    int k;
    for (k = 0; k < *n; k++)
        out[k] = (w16)((unsigned char)src[k] | (' ' << 8));
}

/* DECODE (80,99,IBUFF) DIR,(IN(I),I=1,78) with (A2,78A1) */
void punpk2_(const char *src, w16 *dir, w16 *in)
{
    int k;
    *dir = (w16)((unsigned char)src[0] | ((unsigned char)src[1] << 8));
    for (k = 0; k < 78; k++)
        in[k] = (w16)((unsigned char)src[2 + k] | (' ' << 8));
}

/* CNUMD(N, BUF): N as six decimal characters, leading blanks */
void cnumd_(const w16 *v, char *buf)
{
    char t[16];
    snprintf(t, sizeof t, "%6d", (int)*v);
    memcpy(buf, t + strlen(t) - 6, 6);
}

/* KCVT(N): the last two of those six characters, as one word - the
   system library's KCVT loads N, sets E for decimal and calls $CVT3, as
   CNUMD does, and returns the last word (read from DL.RUN on the RTE-6/VM
   disc, where the loader had put it at 51515) */
w16 kcvt_(const w16 *v)
{
    char t[16];
    size_t l;
    snprintf(t, sizeof t, "%6d", (int)*v);
    l = strlen(t);
    return (w16)((unsigned char)t[l - 2] | ((unsigned char)t[l - 1] << 8));
}

/* SSEED/URAN - the math library's uniform random numbers (24998-1X456).
   Not yet the HP's own algorithm: a stand-in that repeats from the same
   seed, which is what the game needs for --time.  See the README. */
static unsigned long useed = 1;

void sseed_(const w16 *s)
{
    useed = (unsigned long)(unsigned short)*s;
}

float uran_(const w16 *dummy)
{
    (void)dummy;
    useed = (useed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (float)((double)(useed >> 7) / (double)(1UL << 24));
}

/* ------------------------------------------------------------------ */
/* FMP files: a file is its records, kept as "RTEFMP type count", then
   each record's length in words and its words.  Files whose names begin
   with @ (the game's data base) live beside the program; the others
   (saved games) in saves\.                                            */

typedef struct {
    int used, type, dirty, pos;
    char path[MAX_PATH + 32];
    int n, cap;
    w16 **rec;
    int *len;
} fmpf;

#define NF 8
static fmpf F[NF];
static char savedir[MAX_PATH] = "saves", datadir[MAX_PATH] = ".";

static void setdir(char *dst, const char *s, int slen)
{
    int n = slen;
    while (n > 0 && s[n - 1] == ' ') n--;
    if (n >= MAX_PATH) n = MAX_PATH - 1;
    memcpy(dst, s, (size_t)n);
    dst[n] = 0;
}

void psavedir_(const char *s, int slen) { setdir(savedir, s, slen); }
void pdatadir_(const char *s, int slen) { setdir(datadir, s, slen); }

static void fpath(char *out, const w16 *name)
{
    char n[8];
    int i;
    for (i = 0; i < 3; i++) {
        n[2 * i] = (char)(name[i] & 0xFF);
        n[2 * i + 1] = (char)((name[i] >> 8) & 0xFF);
    }
    n[6] = 0;
    for (i = 5; i >= 0 && n[i] == ' '; i--) n[i] = 0;
    if (n[0] == '@') {
        snprintf(out, MAX_PATH + 32, "%s\\%s", datadir, n);
    } else {
        CreateDirectoryA(savedir, NULL);
        snprintf(out, MAX_PATH + 32, "%s\\%s", savedir, n);
    }
}

static int fslot(const w16 *dcb)
{
    int i = dcb[0] - 1000;
    if (i >= 0 && i < NF && dcb[1] == 0x4D4D && F[i].used) return i;
    return -1;
}

static void fgrow(fmpf *f)
{
    if (f->n == f->cap) {
        f->cap = f->cap ? 2 * f->cap : 64;
        f->rec = realloc(f->rec, sizeof(w16 *) * (size_t)f->cap);
        f->len = realloc(f->len, sizeof(int) * (size_t)f->cap);
    }
}

static void fload(fmpf *f, FILE *fp)
{
    int t, n, k;
    /* exactly one newline after the header: a length byte may be a blank */
    if (fscanf(fp, "RTEFMP %d %d", &t, &n) != 2 || fgetc(fp) != '\n') return;
    f->type = t;
    for (k = 0; k < n; k++) {
        unsigned char h[2];
        int len, j;
        if (fread(h, 1, 2, fp) != 2) break;
        len = h[0] | (h[1] << 8);
        fgrow(f);
        f->rec[f->n] = malloc(sizeof(w16) * (size_t)(len ? len : 1));
        for (j = 0; j < len; j++) {
            if (fread(h, 1, 2, fp) != 2) break;
            f->rec[f->n][j] = (w16)(h[0] | (h[1] << 8));
        }
        f->len[f->n++] = len;
    }
}

static int fnew(w16 *dcb)
{
    int i;
    for (i = 0; i < NF; i++) if (!F[i].used) break;
    if (i == NF) return -1;
    memset(&F[i], 0, sizeof F[i]);
    F[i].used = 1;
    dcb[0] = (w16)(1000 + i);
    dcb[1] = 0x4D4D;
    return i;
}

static void fsave(fmpf *f)
{
    FILE *fp;
    int k, j;
    if (!f->dirty) return;
    fp = fopen(f->path, "wb");
    if (!fp) return;
    fprintf(fp, "RTEFMP %d %d\n", f->type, f->n);
    for (k = 0; k < f->n; k++) {
        fputc(f->len[k] & 0xFF, fp); fputc((f->len[k] >> 8) & 0xFF, fp);
        for (j = 0; j < f->len[k]; j++) {
            fputc(f->rec[k][j] & 0xFF, fp);
            fputc((f->rec[k][j] >> 8) & 0xFF, fp);
        }
    }
    fclose(fp);
    f->dirty = 0;
}

/* OPEN: IERR -6 if there is no such file, else the file type */
void fopen_(w16 *dcb, w16 *ierr, const w16 *name, const w16 *iopt,
            const w16 *isecu, const w16 *icr)
{
    char p[MAX_PATH + 32];
    FILE *fp;
    int i;
    (void)iopt; (void)isecu; (void)icr;
    fpath(p, name);
    fp = fopen(p, "rb");
    if (!fp) { *ierr = -6; return; }
    i = fnew(dcb);
    if (i < 0) { fclose(fp); *ierr = -13; return; }
    strcpy(F[i].path, p);
    fload(&F[i], fp);
    fclose(fp);
    *ierr = (w16)F[i].type;
}

/* CREAT: IERR -2 if the name exists, else the size in blocks (ISIZE(1);
   for type 2 ISIZE(2) is the record length) */
void fcreat_(w16 *dcb, w16 *ierr, const w16 *name, const w16 *isize,
             const w16 *itype, const w16 *isecu, const w16 *icr)
{
    char p[MAX_PATH + 32];
    FILE *fp;
    int i;
    (void)isecu; (void)icr;
    fpath(p, name);
    fp = fopen(p, "rb");
    if (fp) { fclose(fp); *ierr = -2; return; }
    i = fnew(dcb);
    if (i < 0) { *ierr = -13; return; }
    strcpy(F[i].path, p);
    F[i].type = *itype;
    F[i].dirty = 1;
    *ierr = isize[0];
}

void fclose_(w16 *dcb)
{
    int i = fslot(dcb), k;
    fmpf *f;
    if (i < 0) return;
    f = &F[i];
    fsave(f);
    for (k = 0; k < f->n; k++) free(f->rec[k]);
    free(f->rec); free(f->len);
    memset(f, 0, sizeof *f);
    dcb[0] = 0;
}

/* PURGE: IERR 0, or -6 if there is no such file */
void fpurge_(w16 *dcb, w16 *ierr, const w16 *name, const w16 *isecu,
             const w16 *icr)
{
    char p[MAX_PATH + 32];
    (void)dcb; (void)isecu; (void)icr;
    fpath(p, name);
    *ierr = (w16)(DeleteFileA(p) ? 0 : -6);
}

/* READF: record NUM (from 1), or the next one if NUM is 0; at most IL
   words of it, LEN the number read.  Past the end: IERR -12 - except in a
   type 1 or 2 file, whose space was allocated and reads as zeros. */
void freadf_(w16 *dcb, w16 *ierr, w16 *buf, const w16 *il, w16 *len,
             const w16 *num)
{
    int i = fslot(dcb), k, m;
    fmpf *f;
    if (i < 0) { *ierr = -11; return; }
    f = &F[i];
    if (*num > 0) f->pos = *num - 1;
    if (f->pos >= f->n) {
        if (f->type == 1 || f->type == 2) {
            for (k = 0; k < *il; k++) buf[k] = 0;
            f->pos++;
            *len = *il;
            *ierr = 0;
            return;
        }
        *ierr = -12;
        *len = -1;
        return;
    }
    m = f->len[f->pos] < *il ? f->len[f->pos] : *il;
    for (k = 0; k < m; k++) buf[k] = f->rec[f->pos][k];
    f->pos++;
    *len = (w16)m;
    *ierr = 0;
}

/* WRITF: IL words as record NUM (from 1), or as the next one */
void fwritf_(w16 *dcb, w16 *ierr, const w16 *buf, const w16 *il,
             const w16 *num)
{
    int i = fslot(dcb), n = *il;
    fmpf *f;
    if (i < 0) { *ierr = -11; return; }
    f = &F[i];
    if (*num > 0) f->pos = *num - 1;
    while (f->n <= f->pos) {              /* empty records up to here */
        fgrow(f);
        f->rec[f->n] = calloc(1, sizeof(w16));
        f->len[f->n++] = 0;
    }
    free(f->rec[f->pos]);
    f->rec[f->pos] = malloc(sizeof(w16) * (size_t)(n > 0 ? n : 1));
    if (n > 0) memcpy(f->rec[f->pos], buf, sizeof(w16) * (size_t)n);
    f->len[f->pos] = n > 0 ? n : 0;
    f->pos++;
    f->dirty = 1;
    *ierr = 0;
}

/* at the end: files still open are written, as RTE's would be on disc */
void pfmpend_(void)
{
    int i;
    for (i = 0; i < NF; i++) if (F[i].used) fsave(&F[i]);
}
