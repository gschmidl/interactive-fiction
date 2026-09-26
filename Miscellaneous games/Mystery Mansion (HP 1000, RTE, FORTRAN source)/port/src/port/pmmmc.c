/*
 * pmmmc.c - the C half of the Mystery Mansion (HP 1000 RTE) port's run-time.
 *
 *  - the segment dispatcher: EXEC 8 loads a segment and runs it, never to
 *    return; here the segment subroutines are called from psegrun_(), and
 *    PSEG (the converted EXEC 8) jumps back to it with the next name and
 *    the five parameters RMPAR will hand the segment;
 *  - the terminal: the program's standard output (gfortran unit 6) goes
 *    through a pipe to a thread that does what RTE's terminal driver and
 *    an HP 264x terminal did: a record ending in '_' is not followed by
 *    CR LF, and the 264x escape sequences become ANSI ones;
 *  - the clock (EXEC 11), held still by --time;
 *  - FMP files as files of word records under saves\CRn\ (cartridge n);
 *  - --site's cartridge MM, and the files of the LUs other than the
 *    terminal (saves\LUn.txt).
 *
 * Words hold two characters, the first one first in memory, as the
 * converted program's Hollerith constants and A2 editing have them.
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
/* segments                                                            */

extern void mmm_(void), mmsa_(void), mmsb_(void), mmsc_(void), mmsd_(void),
    mmse_(void), mmsf_(void), mmsg_(void), mmsh_(void), mmsi_(void),
    mmsj_(void), mmsk_(void), mmsl_(void);
extern void pstop_(void);

static const struct { const char *n; void (*f)(void); } SEG[] = {
    {"MMSA", mmsa_}, {"MMSB", mmsb_}, {"MMSC", mmsc_}, {"MMSD", mmsd_},
    {"MMSE", mmse_}, {"MMSF", mmsf_}, {"MMSG", mmsg_}, {"MMSH", mmsh_},
    {"MMSI", mmsi_}, {"MMSJ", mmsj_}, {"MMSK", mmsk_}, {"MMSL", mmsl_}};

static void *jb[5];
static volatile char seg[8];
static w16 parm[5];               /* what RMPAR returns */
static int nseg = 0;

void psetpar_(const int *p1, const int *p2, const int *p3, const int *p4,
              const int *p5)
{
    parm[0] = (w16)*p1; parm[1] = (w16)*p2; parm[2] = (w16)*p3;
    parm[3] = (w16)*p4; parm[4] = (w16)*p5;
}

void rmpar_(w16 *ip)
{
    memcpy(ip, parm, sizeof parm);
}

static void name6(char *out, const w16 *w)
{
    int i;
    for (i = 0; i < 3; i++) {
        out[2 * i] = (char)(w[i] & 0xFF);
        out[2 * i + 1] = (char)((w[i] >> 8) & 0xFF);
    }
    out[6] = 0;
    for (i = 5; i >= 0 && out[i] == ' '; i--) out[i] = 0;
}

/* the converted EXEC 8.  The parameters may be INTEGER*2 variables or
   INTEGER literals: read as 16 bits either way (little-endian). */
void pseg_(const w16 *name, const w16 *p1, const w16 *p2, const w16 *p3,
           const w16 *p4, const w16 *p5)
{
    char n[8];
    name6(n, name);
    memcpy((char *)seg, n, 8);
    parm[0] = *p1; parm[1] = *p2; parm[2] = *p3; parm[3] = *p4; parm[4] = *p5;
    nseg++;
    __builtin_longjmp(jb, 1);
}

void psegrun_(void)
{
    if (__builtin_setjmp(jb) == 0) {
        mmm_();
        pstop_();
    }
    for (;;) {
        int i;
        void (*f)(void) = NULL;
        for (i = 0; i < (int)(sizeof SEG / sizeof SEG[0]); i++)
            if (!strcmp(SEG[i].n, (const char *)seg)) f = SEG[i].f;
        if (!f) {
            fprintf(stderr, "mmm: no segment '%s'\n", (const char *)seg);
            pstop_();
        }
        if (__builtin_setjmp(jb) == 0) {
            f();
            pstop_();             /* a segment's END ends the program */
        }
    }
}

/* ------------------------------------------------------------------ */
/* FMP files                                                           */

typedef struct {
    int used, type, dirty, pos;
    char path[MAX_PATH];
    int n, cap;
    w16 **rec;
    int *len;
} fmpf;

#define NF 8
static fmpf F[NF];
static char savedir[MAX_PATH] = "saves";

void psavedir_(const char *s, int slen)
{
    int n = slen;
    while (n > 0 && s[n - 1] == ' ') n--;
    if (n >= MAX_PATH) n = MAX_PATH - 1;
    memcpy(savedir, s, (size_t)n);
    savedir[n] = 0;
}

/* the cartridge: a number (a player's own) or two letters (MM) */
static void fpath(char *out, const w16 *name, const w16 *icr)
{
    char n[8], d[MAX_PATH], c[8];
    int v = *icr;
    name6(n, name);
    if (v > 0 && v < 1000) sprintf(c, "%d", v);
    else if (v != 0 && (v & 0xFF) >= 'A' && (v & 0xFF) <= 'Z') {
        c[0] = (char)(v & 0xFF); c[1] = (char)((v >> 8) & 0xFF); c[2] = 0;
        if (c[1] == ' ') c[1] = 0;
    } else strcpy(c, "0");
    sprintf(d, "%s\\CR%s", savedir, c);
    CreateDirectoryA(savedir, NULL);
    CreateDirectoryA(d, NULL);
    sprintf(out, "%s\\%s", d, n);
}

static int fslot(w16 *dcb)
{
    int i = dcb[0] - 1000;
    if (i >= 0 && i < NF && dcb[1] == 0x4D4D && F[i].used) return i;
    return -1;
}

static void fload(fmpf *f, FILE *fp)
{
    int t, n, k;
    /* exactly one newline after the header: a record length of 32 is a
       blank, which a whitespace directive would swallow */
    if (fscanf(fp, "RTEFMP %d %d", &t, &n) != 2 || fgetc(fp) != '\n') return;
    f->type = t;
    for (k = 0; k < n; k++) {
        unsigned char h[2];
        int len, j;
        if (fread(h, 1, 2, fp) != 2) break;
        len = h[0] | (h[1] << 8);
        if (f->n == f->cap) {
            f->cap = f->cap ? 2 * f->cap : 16;
            f->rec = realloc(f->rec, sizeof(w16 *) * (size_t)f->cap);
            f->len = realloc(f->len, sizeof(int) * (size_t)f->cap);
        }
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

/* OPEN: IERR -6 if there is no such file, else the file type */
void fopen_(w16 *dcb, w16 *ierr, const w16 *name, const w16 *iopt,
            const w16 *isc, const w16 *icr)
{
    char p[MAX_PATH];
    FILE *fp;
    int i;
    (void)iopt; (void)isc;
    fpath(p, name, icr);
    fp = fopen(p, "rb");
    if (!fp) { *ierr = -6; return; }
    i = fnew(dcb);
    if (i < 0) { fclose(fp); *ierr = -13; return; }
    strcpy(F[i].path, p);
    fload(&F[i], fp);
    fclose(fp);
    *ierr = (w16)F[i].type;
}

/* CREAT: IERR -2 if the name exists, else the size in blocks */
void fcreat_(w16 *dcb, w16 *ierr, const w16 *name, const w16 *isize,
             const w16 *itype, const w16 *isc, const w16 *icr)
{
    char p[MAX_PATH];
    FILE *fp;
    int i;
    (void)isc;
    fpath(p, name, icr);
    fp = fopen(p, "rb");
    if (fp) { fclose(fp); *ierr = -2; return; }
    i = fnew(dcb);
    if (i < 0) { *ierr = -13; return; }
    strcpy(F[i].path, p);
    F[i].type = *itype;
    F[i].dirty = 1;
    *ierr = *isize;
}

void fclose_(w16 *dcb)
{
    int i = fslot(dcb), k, j;
    fmpf *f;
    if (i < 0) return;
    f = &F[i];
    if (f->dirty) {
        FILE *fp = fopen(f->path, "wb");
        if (fp) {
            fprintf(fp, "RTEFMP %d %d\n", f->type, f->n);
            for (k = 0; k < f->n; k++) {
                fputc(f->len[k] & 0xFF, fp); fputc((f->len[k] >> 8) & 0xFF, fp);
                for (j = 0; j < f->len[k]; j++) {
                    fputc(f->rec[k][j] & 0xFF, fp);
                    fputc((f->rec[k][j] >> 8) & 0xFF, fp);
                }
            }
            fclose(fp);
        }
    }
    for (k = 0; k < f->n; k++) free(f->rec[k]);
    free(f->rec); free(f->len);
    memset(f, 0, sizeof *f);
    dcb[0] = 0;
}

/* READF: the next record, up to L words.  Past the end: IERR -12 - except
   in a new type 1 file, whose space was allocated and reads as zeros. */
void freadf_(w16 *dcb, w16 *ierr, w16 *buf, const w16 *l)
{
    int i = fslot(dcb), k;
    fmpf *f;
    if (i < 0) { *ierr = -11; return; }
    f = &F[i];
    if (f->pos >= f->n) {
        if (f->type == 1) {
            for (k = 0; k < *l; k++) buf[k] = 0;
            f->pos++;
            *ierr = 0;
            return;
        }
        *ierr = -12;
        return;
    }
    for (k = 0; k < *l; k++)
        buf[k] = (k < f->len[f->pos]) ? f->rec[f->pos][k] : 0;
    f->pos++;
    *ierr = 0;
}

void fwritf_(w16 *dcb, w16 *ierr, const w16 *buf, const w16 *l)
{
    int i = fslot(dcb), n = *l;
    fmpf *f;
    if (i < 0) { *ierr = -11; return; }
    f = &F[i];
    while (f->n < f->pos) {               /* zero records up to here */
        if (f->n == f->cap) {
            f->cap = f->cap ? 2 * f->cap : 16;
            f->rec = realloc(f->rec, sizeof(w16 *) * (size_t)f->cap);
            f->len = realloc(f->len, sizeof(int) * (size_t)f->cap);
        }
        f->rec[f->n] = calloc(1, sizeof(w16));
        f->len[f->n++] = 0;
    }
    if (f->pos == f->n) {
        if (f->n == f->cap) {
            f->cap = f->cap ? 2 * f->cap : 16;
            f->rec = realloc(f->rec, sizeof(w16 *) * (size_t)f->cap);
            f->len = realloc(f->len, sizeof(int) * (size_t)f->cap);
        }
        f->rec[f->n] = NULL;
        f->len[f->n++] = 0;
    }
    free(f->rec[f->pos]);
    f->rec[f->pos] = malloc(sizeof(w16) * (size_t)(n ? n : 1));
    memcpy(f->rec[f->pos], buf, sizeof(w16) * (size_t)n);
    f->len[f->pos] = n;
    f->pos++;
    f->dirty = 1;
    *ierr = 0;
}

/* POSNT: move NUR records (relative) */
void fposnt_(w16 *dcb, w16 *ierr, const w16 *nur)
{
    int i = fslot(dcb);
    if (i < 0) { *ierr = -11; return; }
    F[i].pos += *nur;
    if (F[i].pos < 0) { F[i].pos = 0; *ierr = -12; return; }
    *ierr = 0;
}

/* --site: cartridge MM is mounted, as on Wolpert's machine.  Two of its
   files were never distributed: MMMC, which the game only opens to see
   that the cartridge is there, and MMTLAN, the messages kept for players by
   name (its presence is what makes the game ask for a name).  The port
   starts them empty; the game makes the others (MMTLDA, MMCF) itself. */
void pmountmm_(void)
{
    static const char names[2][7] = {"MMMC  ", "MMTLAN"};
    w16 nm[3], cr = (w16)('M' | ('M' << 8));
    char p[MAX_PATH];
    FILE *fp;
    int k, j;
    for (k = 0; k < 2; k++) {
        for (j = 0; j < 3; j++)
            nm[j] = (w16)(names[k][2 * j] | (names[k][2 * j + 1] << 8));
        fpath(p, nm, &cr);
        fp = fopen(p, "rb");
        if (fp) { fclose(fp); continue; }
        fp = fopen(p, "wb");
        if (fp) { fputs("RTEFMP 3 0\n", fp); fclose(fp); }
    }
}

/* ------------------------------------------------------------------ */
/* LUs other than the terminal: files in saves\ (see PU in pmmm.f)     */

/* the file of LU lu, blank-filled into buf */
void plupath_(const int *lu, char *buf, int *n, int buflen)
{
    char p[MAX_PATH];
    CreateDirectoryA(savedir, NULL);
    snprintf(p, sizeof p, "%s\\LU%d.txt", savedir, *lu);
    memset(buf, ' ', (size_t)buflen);
    *n = (int)strlen(p);
    if (*n > buflen) *n = buflen;
    memcpy(buf, p, (size_t)*n);
}

/* A line of LU lu's file, read as the terminal's are (7 bits, capitals).
   -1 at its end - or if there is none; the next read starts it again. */
static FILE *lufp[256];

int plread_(const int *lu, char *buf, int buflen)
{
    int c, n = 0, k = *lu;
    memset(buf, ' ', (size_t)buflen);
    if (k < 1 || k > 255) return -1;
    if (!lufp[k]) {
        char p[MAX_PATH];
        snprintf(p, sizeof p, "%s\\LU%d.txt", savedir, k);
        lufp[k] = fopen(p, "rb");
        if (!lufp[k]) return -1;
    }
    for (;;) {
        c = getc(lufp[k]);
        if (c == EOF) {
            if (n == 0) {
                fclose(lufp[k]);
                lufp[k] = NULL;
                return -1;
            }
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
    return n;
}
