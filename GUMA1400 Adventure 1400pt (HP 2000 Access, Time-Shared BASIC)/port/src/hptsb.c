/* hptsb.c -- an interpreter for HP 2000 Access Time-Shared BASIC.
 *
 * Written to run the 1979 HP 2000 Access adventure game "Adventure ]I["
 * (a.k.a. ANON1400) from its original source, unmodified.
 *
 * The dialect implemented here is HP 2000 Access TSB as it appears in the
 * Hammerstone printout, plus the small number of HP 3000 BASIC spellings
 * that Alex Guma's 2023 reconstruction uses, so that both variants of the
 * game run from their archived sources without editing them:
 *
 *      #            and  <>            (not equal)
 *      IF END #n    and  ON END #n     (arm deferred end-of-file branch)
 *      CHAIN s,"f"  and  CHAIN "f"     (status variable optional)
 *      **           and  ^             (exponentiation)
 *      PRINT "a"B$  and  PRINT "a";B$  (implicit semicolon)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <setjmp.h>
#include <stdarg.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <conio.h>
#  define ISATTY(f) _isatty(_fileno(f))
#else
#  include <unistd.h>
#  include <sys/select.h>
#  include <sys/stat.h>
#  define ISATTY(f) isatty(fileno(f))
#endif

/* ------------------------------------------------------------------ */
/* configuration                                                       */
/* ------------------------------------------------------------------ */

#define RECWORDS   512          /* words per file record (1024 bytes)  */
#define MAXFILES   16
#define MAXLINES   4000
#define MAXSTR     512
#define GOSUB_MAX  64
#define FOR_MAX    32

static char  g_progdir[1024] = ".";
static char  g_datadir[1024] = ".";
static char  g_userid[16]   = "B500";   /* what SYSTEM ...,"Time" reports */
static int   g_randomize    = 0;
static int   g_echo_input   = 0;        /* echo stdin when it is not a tty */
static int   g_quiet_done   = 0;
static int   g_silent       = 0;        /* swallow output while building data */
static int   g_progdir_set  = 0;        /* -p given: game mode must not override */
static int   g_datadir_set  = 0;        /* -d given: likewise                     */

/* ------------------------------------------------------------------ */
/* item encoding inside data files                                     */
/* ------------------------------------------------------------------ */

#define IT_VOID  0x0000u                /* word never written          */
#define IT_STR   0x8000u                /* | length                    */
#define IT_NUM   0x4000u                /* + 2 words of float32        */
#define IT_EOF   0x2000u                /* end-of-file mark            */
#define IT_PAD   0x1000u                /* skip to next record         */

/* ------------------------------------------------------------------ */
/* diagnostics                                                         */
/* ------------------------------------------------------------------ */

static int      g_curline = 0;          /* BASIC line number, for errors */
static jmp_buf  g_errjmp;
static int      g_errline = 0;          /* IF ERROR THEN nnnn target   */
static int      g_haderr  = 0;

static void out_flushline(void);

static void die(const char *fmt, ...)
{
    va_list ap;
    out_flushline();
    fflush(stdout);
    fprintf(stderr, "\n*** ");
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fprintf(stderr, " IN LINE %d\n", g_curline);
    exit(2);
}

/* A recoverable BASIC error: honours "IF ERROR THEN nnnn" when armed. */
static void bas_error(const char *fmt, ...)
{
    va_list ap;
    char buf[256];
    va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    if (g_errline) { g_haderr = 1; longjmp(g_errjmp, 1); }
    die("%s", buf);
}

/* ------------------------------------------------------------------ */
/* terminal output                                                     */
/*                                                                     */
/* HP terminals treat CR and LF independently: LF moves down one line  */
/* and keeps the column, CR returns to column 1 of the current line.   */
/* The game relies on this -- PRINT "x"'10 leaves a blank line, while  */
/* PRINT "x"'13'10"y" starts y hard against the left margin.  So we    */
/* model the carriage rather than translating character by character.  */
/* ------------------------------------------------------------------ */

static unsigned long g_io_serial = 0;  /* bumped by every terminal read/write */

static int out_col  = 0;   /* logical carriage column (what HP sees)   */
static int out_emit = 0;   /* columns actually emitted on this line    */

static void emit(int c) { g_io_serial++; if (!g_silent) fputc(c, stdout); }

static void out_lf(void)   { emit(10); out_emit = 0; }
static void out_cr(void)   { out_col = 0; }

static void out_flushline(void)
{
    if (out_emit > 0) { out_cr(); out_lf(); }
}

/* end of a PRINT statement: carriage return then line feed */
static void out_endline(void) { out_cr(); out_lf(); }

static void out_ch(int c)
{
    c &= 0xff;
    if (c == 13) { out_cr(); return; }
    if (c == 10) { out_lf(); return; }
    if (c < 32 || c == 127) {            /* bell and friends: no column */
        emit(c);
        return;
    }
    if (out_col < out_emit) {            /* overprint after a bare CR   */
        emit(13);
        out_emit = 0;
    }
    while (out_emit < out_col) { emit(32); out_emit++; }
    emit(c);
    out_col++; out_emit++;
}

static void out_mem(const char *s, int n)
{
    int i; for (i = 0; i < n; i++) out_ch((unsigned char)s[i]);
}

static void out_s(const char *s) { out_mem(s, (int)strlen(s)); }

/* after an input statement the terminal has already returned home */
static void out_input_done(void) { out_col = 0; out_emit = 0; }

/* ------------------------------------------------------------------ */
/* values                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int    isstr;
    double num;
    char   s[MAXSTR];
    int    slen;
} Val;

static void val_setnum(Val *v, double d) { v->isstr = 0; v->num = d; v->slen = 0; }

static void val_setstr(Val *v, const char *p, int n)
{
    if (n > MAXSTR) n = MAXSTR;
    v->isstr = 1; v->slen = n;
    if (n) memcpy(v->s, p, (size_t)n);
}

/* ------------------------------------------------------------------ */
/* variables                                                           */
/*                                                                     */
/* HP 2000 names are a letter optionally followed by a digit.  Slot 10 */
/* of the digit axis means "no digit", so A and A0 stay distinct.      */
/* Arrays are single letters and live in their own namespace, which is */
/* how the game can use both O[69,2] and O$ at the same time.          */
/* ------------------------------------------------------------------ */

typedef struct {
    char *buf;      /* physical buffer, blank filled                   */
    int   phys;     /* dimensioned length                              */
    int   len;      /* logical length                                  */
} SVar;

typedef struct {
    double *v;
    int     d1, d2;   /* d2 == 0 for a vector                          */
} NArr;

static double numv[26][11];
static SVar   strv[26][11];
static NArr   arrv[26];

static unsigned char com_num[26][11], com_str[26][11], com_arr[26];

static void svar_dim(SVar *s, int phys)
{
    if (phys < 1) phys = 1;
    if (s->buf && s->phys == phys) { memset(s->buf, ' ', (size_t)phys); s->len = 0; return; }
    free(s->buf);
    s->buf = (char *)malloc((size_t)phys + 1);
    memset(s->buf, ' ', (size_t)phys);
    s->buf[phys] = 0;
    s->phys = phys;
    s->len  = 0;
}

static void arr_dim(NArr *a, int d1, int d2)
{
    int n = (d1 + 1) * (d2 > 0 ? d2 + 1 : 1);
    free(a->v);
    a->v  = (double *)calloc((size_t)n, sizeof(double));
    a->d1 = d1; a->d2 = d2;
}

/* ------------------------------------------------------------------ */
/* files                                                               */
/*                                                                     */
/* An HP 2000 Access data file is a fixed number of 256-word records.  */
/* Items are written serially and never straddle a record boundary; if */
/* one will not fit, the rest of the record is skipped.  READ #n,r     */
/* positions to the head of record r.  Running off the last record, or */
/* reading the end-of-file mark, raises the end-of-file condition.     */
/* ------------------------------------------------------------------ */

typedef struct {
    int             used;
    char            name[80];        /* name as the program spells it  */
    char            path[1200];
    unsigned short *w;
    int             nrec;
    int             pos;             /* word index                     */
    int             eofline;         /* IF END / ON END target         */
    int             dirty;
    int             readonly;
} BFile;

static BFile files[MAXFILES + 1];

/* end-of-file condition: unwinds to the interpreter loop */
static jmp_buf g_eofjmp;
static int     g_eof_target = 0;

static void raise_eof(BFile *f)
{
    if (!f->eofline)
        bas_error("END OF FILE");
    g_eof_target = f->eofline;
    longjmp(g_eofjmp, 1);
}

/* ---- name resolution ---------------------------------------------- */
/* HP 2000 file names may carry the owning account, e.g. Matrix.L957.  */
/* A catalog file in the data directory maps those onto the plain      */
/* names the data-builder programs create.                             */

typedef struct { char from[96], to[96]; } Alias;
static Alias g_alias[64];
static int   g_naliases = 0;

static void load_aliases(void)
{
    char p[1200], line[256];
    FILE *fp;
    snprintf(p, sizeof p, "%s/catalog.txt", g_progdir);
    if (!(fp = fopen(p, "r"))) return;
    while (fgets(line, sizeof line, fp)) {
        char *eq, *s = line, *e;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '*' || *s == '#' || *s == '\n' || *s == '\r' || !*s) continue;
        if (!(eq = strchr(s, '='))) continue;
        *eq = 0;
        e = eq - 1; while (e >= s && (*e == ' ' || *e == '\t')) *e-- = 0;
        eq++;
        while (*eq == ' ' || *eq == '\t') eq++;
        e = eq + strlen(eq) - 1;
        while (e >= eq && (*e == '\n' || *e == '\r' || *e == ' ' || *e == '\t')) *e-- = 0;
        if (g_naliases < 64) {
            size_t la = strlen(s), lb = strlen(eq);
            if (la >= sizeof g_alias[0].from) la = sizeof g_alias[0].from - 1;
            if (lb >= sizeof g_alias[0].to)   lb = sizeof g_alias[0].to   - 1;
            memcpy(g_alias[g_naliases].from, s,  la); g_alias[g_naliases].from[la] = 0;
            memcpy(g_alias[g_naliases].to,   eq, lb); g_alias[g_naliases].to[lb]   = 0;
            g_naliases++;
        }
    }
    fclose(fp);
}

static int ci_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

/* map a BASIC file name onto a bare file name in the data directory */
static void resolve_name(const char *name, char *out, size_t outsz)
{
    int i;
    for (i = 0; i < g_naliases; i++)
        if (ci_eq(g_alias[i].from, name)) {
            snprintf(out, outsz, "%s", g_alias[i].to);
            return;
        }
    snprintf(out, outsz, "%s", name);
}

static void datapath(const char *name, char *out, size_t outsz)
{
    char bare[80];
    resolve_name(name, bare, sizeof bare);
    snprintf(out, outsz, "%s/%s.dat", g_datadir, bare);
}

/* ---- record storage ------------------------------------------------ */

static int file_load(const char *name, BFile *f)
{
    FILE *fp; long sz; int i;
    unsigned char *raw;

    datapath(name, f->path, sizeof f->path);
    if (!(fp = fopen(f->path, "rb"))) return 0;
    fseek(fp, 0, SEEK_END); sz = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (sz <= 0 || (sz % (RECWORDS * 2)) != 0) { fclose(fp); return 0; }
    f->nrec = (int)(sz / (RECWORDS * 2));
    raw = (unsigned char *)malloc((size_t)sz);
    if (fread(raw, 1, (size_t)sz, fp) != (size_t)sz) { free(raw); fclose(fp); return 0; }
    fclose(fp);
    f->w = (unsigned short *)malloc((size_t)f->nrec * RECWORDS * sizeof(unsigned short));
    for (i = 0; i < f->nrec * RECWORDS; i++)
        f->w[i] = (unsigned short)((raw[2 * i] << 8) | raw[2 * i + 1]);
    free(raw);
    f->pos = 0; f->dirty = 0;
    snprintf(f->name, sizeof f->name, "%s", name);
    return 1;
}

static void file_flush(BFile *f)
{
    FILE *fp; int i, n;
    unsigned char *raw;
    if (!f->used || !f->dirty || !f->w) return;
    n = f->nrec * RECWORDS;
    raw = (unsigned char *)malloc((size_t)n * 2);
    for (i = 0; i < n; i++) { raw[2 * i] = (unsigned char)(f->w[i] >> 8); raw[2 * i + 1] = (unsigned char)f->w[i]; }
    if ((fp = fopen(f->path, "wb"))) { fwrite(raw, 1, (size_t)n * 2, fp); fclose(fp); }
    free(raw);
    f->dirty = 0;
}

static void file_close(BFile *f)
{
    if (!f->used) return;
    file_flush(f);
    free(f->w); f->w = NULL;
    f->used = 0; f->nrec = 0; f->pos = 0; f->eofline = 0;
    f->name[0] = 0;
}

static int file_create(const char *name, int nrec)
{
    char path[1200]; FILE *fp; unsigned char *raw; long n;
    if (nrec < 1) nrec = 1;
    datapath(name, path, sizeof path);
    n = (long)nrec * RECWORDS * 2;
    raw = (unsigned char *)calloc((size_t)n, 1);
    if (!(fp = fopen(path, "wb"))) { free(raw); return 1; }
    fwrite(raw, 1, (size_t)n, fp);
    fclose(fp); free(raw);
    return 0;
}

static int file_purge(const char *name)
{
    char path[1200];
    datapath(name, path, sizeof path);
    return remove(path) == 0 ? 0 : 1;
}

static int file_exists(const char *name)
{
    char path[1200];
    FILE *fp;
    long sz;
    datapath(name, path, sizeof path);
    if (!(fp = fopen(path, "rb"))) return 0;
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fclose(fp);
    return sz > 0 && (sz % (RECWORDS * 2)) == 0;
}

/* ---- item level read / write --------------------------------------- */

static int words_for_str(int len) { return 1 + (len + 1) / 2; }
#define WORDS_FOR_NUM 3

/* step the position past record padding; return 0 if past the end */
static int skip_pad(BFile *f)
{
    for (;;) {
        int rec, off;
        if (f->pos >= f->nrec * RECWORDS) return 0;
        rec = f->pos / RECWORDS; off = f->pos % RECWORDS;
        if (f->w[f->pos] == IT_PAD) { f->pos = (rec + 1) * RECWORDS; continue; }
        (void)off;
        return 1;
    }
}

/* make room for n words in the current record; 0 if the file is full */
static int make_room(BFile *f, int n)
{
    int rec, off;
    for (;;) {
        if (f->pos >= f->nrec * RECWORDS) return 0;
        rec = f->pos / RECWORDS; off = f->pos % RECWORDS;
        if (RECWORDS - off >= n) return 1;
        f->w[f->pos] = IT_PAD;
        f->pos = (rec + 1) * RECWORDS;
        f->dirty = 1;
    }
}

static void float_to_words(double d, unsigned short *w)
{
    union { float f; unsigned int u; } u;
    u.f = (float)d;
    w[0] = (unsigned short)(u.u >> 16);
    w[1] = (unsigned short)(u.u & 0xffff);
}

static double words_to_float(const unsigned short *w)
{
    union { float f; unsigned int u; } u;
    u.u = ((unsigned int)w[0] << 16) | w[1];
    return (double)u.f;
}

/* Read one item.  Raises the end-of-file condition at the EOF mark, at
   a word that was never written, and past the last record. */
static void file_read_item(BFile *f, Val *v)
{
    unsigned short h;
    if (!f->used) bas_error("FILE NOT ASSIGNED");
    if (!skip_pad(f)) raise_eof(f);
    h = f->w[f->pos];
    if (h == IT_VOID || h == IT_EOF) raise_eof(f);
    if (h == IT_NUM) {
        val_setnum(v, words_to_float(&f->w[f->pos + 1]));
        f->pos += WORDS_FOR_NUM;
        return;
    }
    if (h & IT_STR) {
        int len = (int)(h & 0x0fff), i;
        char buf[MAXSTR];
        if (len > MAXSTR) len = MAXSTR;
        for (i = 0; i < len; i++) {
            unsigned short ww = f->w[f->pos + 1 + i / 2];
            buf[i] = (char)((i & 1) ? (ww & 0xff) : (ww >> 8));
        }
        val_setstr(v, buf, len);
        f->pos += words_for_str((int)(h & 0x0fff));
        return;
    }
    bas_error("FILE DATA CORRUPT");
}

/* Skip one item without decoding it.  Returns 0 at end of file. */
static int file_skip_item(BFile *f)
{
    unsigned short h;
    if (!skip_pad(f)) return 0;
    h = f->w[f->pos];
    if (h == IT_VOID || h == IT_EOF) return 0;
    if (h == IT_NUM)  { f->pos += WORDS_FOR_NUM; return 1; }
    if (h & IT_STR)   { f->pos += words_for_str((int)(h & 0x0fff)); return 1; }
    return 0;
}

static void file_write_item(BFile *f, const Val *v)
{
    if (!f->used) bas_error("FILE NOT ASSIGNED");
    if (f->readonly) bas_error("FILE IS PROTECTED");
    if (v->isstr) {
        int len = v->slen, i, nw;
        if (len > 0x0fff) len = 0x0fff;
        nw = words_for_str(len);
        if (!make_room(f, nw)) raise_eof(f);
        f->w[f->pos] = (unsigned short)(IT_STR | (unsigned)len);
        for (i = 0; i < nw - 1; i++) f->w[f->pos + 1 + i] = 0;
        for (i = 0; i < len; i++) {
            unsigned short *ww = &f->w[f->pos + 1 + i / 2];
            unsigned char  c   = (unsigned char)v->s[i];
            if (i & 1) *ww = (unsigned short)((*ww & 0xff00) | c);
            else       *ww = (unsigned short)((*ww & 0x00ff) | ((unsigned)c << 8));
        }
        f->pos += nw;
    } else {
        if (!make_room(f, WORDS_FOR_NUM)) raise_eof(f);
        f->w[f->pos] = IT_NUM;
        float_to_words(v->num, &f->w[f->pos + 1]);
        f->pos += WORDS_FOR_NUM;
    }
    f->dirty = 1;
}

static void file_write_eofmark(BFile *f)
{
    if (!f->used) bas_error("FILE NOT ASSIGNED");
    if (f->readonly) bas_error("FILE IS PROTECTED");
    if (!make_room(f, 1)) raise_eof(f);
    f->w[f->pos] = IT_EOF;
    f->dirty = 1;
}

static void file_seek_record(BFile *f, int rec)
{
    if (!f->used) bas_error("FILE NOT ASSIGNED");
    if (rec < 1) rec = 1;
    if (rec > f->nrec) raise_eof(f);
    f->pos = (rec - 1) * RECWORDS;
}

/* ------------------------------------------------------------------ */
/* program storage                                                     */
/* ------------------------------------------------------------------ */

typedef struct { int num; char *text; } Line;
static Line  prog[MAXLINES];
static int   nlines = 0;
static char  g_progname[80] = "";

static int find_line(int n)
{
    int lo = 0, hi = nlines - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (prog[mid].num == n) return mid;
        if (prog[mid].num <  n) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* DATA pool                                                           */
/* ------------------------------------------------------------------ */

typedef struct { int isstr; double num; char *s; int slen; int line; } DItem;
static DItem *dpool = NULL;
static int    ndata = 0, dcap = 0, dptr = 0;

static void data_add(DItem d)
{
    if (ndata == dcap) { dcap = dcap ? dcap * 2 : 64; dpool = (DItem *)realloc(dpool, (size_t)dcap * sizeof(DItem)); }
    dpool[ndata++] = d;
}

/* ------------------------------------------------------------------ */
/* user defined functions                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int  letter, digit, isstr;
    unsigned char params[8][3];
    int  nparams;
    char *body;
} Fndef;
static Fndef fns[32];
static int   nfns = 0;

/* ------------------------------------------------------------------ */
/* scanner                                                             */
/* ------------------------------------------------------------------ */

static const char *P;

static void sk(void) { while (*P == ' ' || *P == '\t') P++; }

static int mt(const char *kw)          /* match keyword, spaces ignored */
{
    const char *save = P, *k = kw;
    sk();
    while (*k) {
        if (toupper((unsigned char)*P) != *k) { P = save; return 0; }
        P++; k++;
    }
    return 1;
}

static int peek(char c) { sk(); return *P == c; }
static int eat(char c)  { sk(); if (*P == c) { P++; return 1; } return 0; }
static void need(char c){ if (!eat(c)) die("EXPECTED '%c'", c); }

/* not-equal may be spelled # (HP 2000) or <> (HP 3000) */
static int mt_ne(void)
{
    const char *save = P;
    sk();
    if (*P == '#') { P++; return 1; }
    if (P[0] == '<' && P[1] == '>') { P += 2; return 1; }
    P = save; return 0;
}


/* ------------------------------------------------------------------ */
/* variable access                                                     */
/* ------------------------------------------------------------------ */

static SVar *svar(int L, int D)
{
    SVar *s = &strv[L][D];
    if (!s->buf) svar_dim(s, MAXSTR - 1);
    return s;
}

static NArr *narr(int L)
{
    NArr *a = &arrv[L];
    if (!a->v) arr_dim(a, 10, 0);
    return a;
}

static double *arr_elem(int L, int i, int j, int has2)
{
    NArr *a = narr(L);
    if (has2) {
        if (a->d2 == 0) die("ARRAY %c IS ONE DIMENSIONAL", 'A' + L);
        if (i < 0 || i > a->d1 || j < 0 || j > a->d2) bas_error("SUBSCRIPT OUT OF RANGE");
        return &a->v[i * (a->d2 + 1) + j];
    }
    if (a->d2 != 0) {
        if (i < 0 || i > a->d1) bas_error("SUBSCRIPT OUT OF RANGE");
        return &a->v[i * (a->d2 + 1)];
    }
    if (i < 0 || i > a->d1) bas_error("SUBSCRIPT OUT OF RANGE");
    return &a->v[i];
}

/* read a substring; hi < 0 means "to the logical end" */
static void svar_get(SVar *s, int lo, int hi, Val *out)
{
    if (lo < 1) lo = 1;
    if (hi < 0) hi = s->len;
    if (hi > s->phys) hi = s->phys;
    if (hi < lo) { val_setstr(out, "", 0); return; }
    val_setstr(out, s->buf + lo - 1, hi - lo + 1);
}

/* whole-variable assignment blank fills the rest of the buffer, which is
   what lets the game compare O1$[1,5] against a table of padded names */
static void svar_set(SVar *s, const char *p, int n)
{
    if (n > s->phys) n = s->phys;
    if (n) memcpy(s->buf, p, (size_t)n);
    memset(s->buf + n, ' ', (size_t)(s->phys - n));
    s->len = n;
}

/* A$[lo]=v truncates at the new end; A$[lo,hi]=v leaves the rest alone */
static void svar_setsub(SVar *s, int lo, int hi, const char *p, int n)
{
    int i, end;
    if (lo < 1) lo = 1;
    if (lo > s->phys) return;
    if (hi < 0) {
        end = lo - 1 + n;
        if (end > s->phys) { end = s->phys; n = end - lo + 1; }
        for (i = 0; i < n; i++) s->buf[lo - 1 + i] = p[i];
        s->len = end;
    } else {
        if (hi > s->phys) hi = s->phys;
        if (n > hi - lo + 1) n = hi - lo + 1;
        for (i = 0; i < n; i++) s->buf[lo - 1 + i] = p[i];
        for (; lo - 1 + i < hi; i++) s->buf[lo - 1 + i] = ' ';
        if (hi > s->len) s->len = hi;
    }
    if (s->len > s->phys) s->len = s->phys;
}

/* ------------------------------------------------------------------ */
/* number formatting                                                   */
/*                                                                     */
/* HP 2000 is a single precision machine and prints six significant    */
/* digits.  CONVERT yields the bare characters with no field padding,  */
/* which is what lets the game poke a room number into ": room 00.".   */
/* ------------------------------------------------------------------ */

static void fmt_number(double d, char *out, size_t n)
{
    double a;
    if (d == 0) { snprintf(out, n, "0"); return; }
    a = fabs(d);
    if (a < 1e10 && d == floor(d)) { snprintf(out, n, "%.0f", d); return; }
    if (a >= 1e-5 && a < 1e6) {
        char buf[64];
        int i, prec = (int)(5 - floor(log10(a)));
        if (prec < 0) prec = 0;
        snprintf(buf, sizeof buf, "%.*f", prec, d);
        if (strchr(buf, '.')) {
            i = (int)strlen(buf) - 1;
            while (i > 0 && buf[i] == '0') buf[i--] = 0;
            if (buf[i] == '.') buf[i] = 0;
        }
        /* HP drops the leading zero of a pure fraction: .5 not 0.5 */
        if (buf[0] == '0' && buf[1] == '.') {
            snprintf(out, n, "%s", buf + 1);
        } else if (buf[0] == '-' && buf[1] == '0' && buf[2] == '.') {
            out[0] = '-';
            snprintf(out + 1, n - 1, "%s", buf + 2);
        } else {
            snprintf(out, n, "%s", buf);
        }
        return;
    }
    {
        char buf[64], *e;
        snprintf(buf, sizeof buf, "%.5E", d);
        if ((e = strchr(buf, 'E'))) {
            char mant[32];
            int ex = atoi(e + 1), i;
            snprintf(mant, sizeof mant, "%.*s", (int)(e - buf), buf);
            if (strchr(mant, '.')) {
                i = (int)strlen(mant) - 1;
                while (i > 0 && mant[i] == '0') mant[i--] = 0;
                if (mant[i] == '.') mant[i] = 0;
            }
            snprintf(out, n, "%sE%c%02d", mant, ex < 0 ? '-' : '+', abs(ex));
        } else {
            snprintf(out, n, "%s", buf);
        }
    }
}

/* ------------------------------------------------------------------ */
/* expressions                                                         */
/* ------------------------------------------------------------------ */

static void expr(Val *v);

static int cmp_vals(const Val *a, const Val *b)
{
    if (a->isstr || b->isstr) {
        int n = a->slen < b->slen ? a->slen : b->slen;
        int c = n ? memcmp(a->s, b->s, (size_t)n) : 0;
        if (c) return c < 0 ? -1 : 1;
        if (a->slen == b->slen) return 0;
        return a->slen < b->slen ? -1 : 1;
    }
    if (a->num < b->num) return -1;
    if (a->num > b->num) return  1;
    return 0;
}

static double as_num(const Val *v)
{
    if (v->isstr) bas_error("NUMBER EXPECTED");
    return v->num;
}

static int as_int(const Val *v)
{
    double d = as_num(v);
    return (int)(d < 0 ? -floor(-d + 0.5) : floor(d + 0.5));
}

static int num_arg(void) { Val v; expr(&v); return as_int(&v); }

/* --- names ---------------------------------------------------------- */

static int try_varname(int *L, int *D, int *isstr)
{
    const char *save = P;
    sk();
    if (!isalpha((unsigned char)*P)) return 0;
    *L = toupper((unsigned char)*P) - 'A'; P++;
    *D = 10;
    if (isdigit((unsigned char)*P)) { *D = *P - '0'; P++; }
    *isstr = 0;
    if (*P == '$') { *isstr = 1; P++; }
    if (isalnum((unsigned char)*P) || *P == '$') { P = save; return 0; }
    return 1;
}

/* parse [lo] / [lo,hi] / (lo) / (lo,hi); returns how many were given */
static int subscripts(int *a, int *b)
{
    char close;
    sk();
    if (*P == '[')      close = ']';
    else if (*P == '(') close = ')';
    else return 0;
    P++;
    *a = num_arg();
    if (eat(',')) { *b = num_arg(); need(close); return 2; }
    need(close);
    return 1;
}

static const char *FUNCS[] = {
    "LEN","POS","UPS$","CHR$","NUM","RND","INT","ABS","SGN","SQR",
    "LOG","EXP","SIN","COS","TAN","ATN","TIM", NULL
};

static int at_func(void)
{
    const char *save = P;
    int i;
    sk();
    for (i = 0; FUNCS[i]; i++) if (mt(FUNCS[i])) { P = save; return 1; }
    if (mt("FN")) { P = save; return 1; }
    P = save;
    return 0;
}

static void call_fn(int idx, Val *out);

/* RND repeats identically from run to run, as it does on HP unless the
   program reseeds it with a negative argument. */
static unsigned long g_rndstate = 37;

static double hp_rnd(void)
{
    g_rndstate = g_rndstate * 1103515245UL + 12345UL;
    return (double)((g_rndstate >> 16) & 0x7fff) / 32768.0;
}

static void do_func(Val *out)
{
    Val a, b;
    if (mt("LEN")) {
        need('('); expr(&a); need(')');
        if (!a.isstr) bas_error("STRING EXPECTED");
        val_setnum(out, a.slen);
        return;
    }
    if (mt("POS")) {
        int i, n = 0;
        need('('); expr(&a); need(','); expr(&b); need(')');
        if (!a.isstr || !b.isstr) bas_error("STRING EXPECTED");
        if (b.slen == 0) { val_setnum(out, 1); return; }
        for (i = 0; i + b.slen <= a.slen; i++)
            if (memcmp(a.s + i, b.s, (size_t)b.slen) == 0) { n = i + 1; break; }
        val_setnum(out, n);
        return;
    }
    if (mt("UPS$")) {
        int i;
        need('('); expr(&a); need(')');
        if (!a.isstr) bas_error("STRING EXPECTED");
        for (i = 0; i < a.slen; i++) a.s[i] = (char)toupper((unsigned char)a.s[i]);
        *out = a;
        return;
    }
    if (mt("CHR$")) {
        char c;
        need('('); expr(&a); need(')');
        c = (char)(as_int(&a) & 0xff);
        val_setstr(out, &c, 1);
        return;
    }
    if (mt("NUM")) {
        need('('); expr(&a); need(')');
        if (!a.isstr) bas_error("STRING EXPECTED");
        val_setnum(out, a.slen ? (unsigned char)a.s[0] : 0);
        return;
    }
    if (mt("RND")) {
        if (peek('(')) {
            need('('); expr(&a); need(')');
            if (!a.isstr && a.num < 0) g_rndstate = (unsigned long)(-a.num);
        }
        val_setnum(out, hp_rnd());
        return;
    }
    if (mt("INT")) { need('('); expr(&a); need(')'); val_setnum(out, floor(as_num(&a))); return; }
    if (mt("ABS")) { need('('); expr(&a); need(')'); val_setnum(out, fabs(as_num(&a))); return; }
    if (mt("SGN")) {
        double d;
        need('('); expr(&a); need(')');
        d = as_num(&a);
        val_setnum(out, d > 0 ? 1 : d < 0 ? -1 : 0);
        return;
    }
    if (mt("SQR")) { need('('); expr(&a); need(')'); val_setnum(out, sqrt(as_num(&a))); return; }
    if (mt("LOG")) {
        double d;
        need('('); expr(&a); need(')');
        d = as_num(&a);
        if (d <= 0) bas_error("LOG OF NON POSITIVE NUMBER");
        val_setnum(out, log(d));
        return;
    }
    if (mt("EXP")) { need('('); expr(&a); need(')'); val_setnum(out, exp(as_num(&a))); return; }
    if (mt("SIN")) { need('('); expr(&a); need(')'); val_setnum(out, sin(as_num(&a))); return; }
    if (mt("COS")) { need('('); expr(&a); need(')'); val_setnum(out, cos(as_num(&a))); return; }
    if (mt("TAN")) { need('('); expr(&a); need(')'); val_setnum(out, tan(as_num(&a))); return; }
    if (mt("ATN")) { need('('); expr(&a); need(')'); val_setnum(out, atan(as_num(&a))); return; }
    if (mt("TIM")) {
        time_t t = time(NULL);
        struct tm *tmv = localtime(&t);
        int k;
        need('('); expr(&a); need(')');
        k = as_int(&a);
        val_setnum(out, k == 0 ? tmv->tm_min : k == 1 ? tmv->tm_hour :
                        k == 2 ? tmv->tm_yday + 1 : tmv->tm_year % 100);
        return;
    }
    if (mt("FN")) {
        int i, L, D = 10, isstr = 0;
        sk();
        if (!isalpha((unsigned char)*P)) die("BAD FN NAME");
        L = toupper((unsigned char)*P) - 'A'; P++;
        if (isdigit((unsigned char)*P)) { D = *P - '0'; P++; }
        if (*P == '$') { isstr = 1; P++; }
        for (i = 0; i < nfns; i++)
            if (fns[i].letter == L && fns[i].digit == D && fns[i].isstr == isstr) {
                call_fn(i, out);
                return;
            }
        die("UNDEFINED FUNCTION");
    }
    die("UNKNOWN FUNCTION");
}

/* --- operand -------------------------------------------------------- */

/* words that end an expression rather than continue it */
static int at_terminator(void)
{
    const char *save = P;
    int r = 0;
    sk();
    if      (mt("THEN")) r = 1;
    else if (mt("STEP")) r = 1;
    else if (mt("TO"))   r = 1;
    else if (mt("OF"))   r = 1;
    P = save;
    return r;
}

static void atom(Val *out)
{
    sk();
    if (*P == '(') { P++; expr(out); need(')'); return; }
    if (*P == '"') {
        char buf[MAXSTR];
        int n = 0;
        P++;
        while (*P && *P != '"') { if (n < MAXSTR) buf[n++] = *P; P++; }
        if (*P == '"') P++;
        val_setstr(out, buf, n);
        return;
    }
    if (isdigit((unsigned char)*P) || (*P == '.' && isdigit((unsigned char)P[1]))) {
        char *end;
        double d = strtod(P, &end);
        P = end;
        if (*P == 'E' || *P == 'e') {           /* strtod already ate it */
            ;
        }
        val_setnum(out, d);
        return;
    }
    if (at_func()) { do_func(out); return; }
    {
        int L, D, isstr, a = 0, b = 0, ns;
        if (try_varname(&L, &D, &isstr)) {
            if (isstr) {
                SVar *s = svar(L, D);
                ns = subscripts(&a, &b);
                if (ns == 0)      svar_get(s, 1, s->len, out);
                else if (ns == 1) svar_get(s, a, -1, out);
                else              svar_get(s, a, b, out);
                return;
            }
            ns = subscripts(&a, &b);
            if (ns == 0) { val_setnum(out, numv[L][D]); return; }
            if (D != 10) die("ARRAY NAME MUST BE A SINGLE LETTER");
            val_setnum(out, *arr_elem(L, a, b, ns == 2));
            return;
        }
    }
    die("SYNTAX ERROR");
}

static void power(Val *out)
{
    Val rhs;
    atom(out);
    sk();
    if ((P[0] == '*' && P[1] == '*') || P[0] == '^') {
        P += (P[0] == '^') ? 1 : 2;
        {
            /* right associative, and unary minus binds tighter on the right */
            Val t;
            sk();
            if (*P == '-') { P++; power(&t); val_setnum(&rhs, -as_num(&t)); }
            else power(&rhs);
        }
        val_setnum(out, pow(as_num(out), as_num(&rhs)));
    }
}

static void unary(Val *out)
{
    sk();
    if (*P == '-') { Val t; P++; unary(&t); val_setnum(out, -as_num(&t)); return; }
    if (*P == '+') { P++; unary(out); return; }
    power(out);
}

static void term(Val *out)
{
    Val rhs;
    unary(out);
    for (;;) {
        sk();
        if (*P == '*' && P[1] != '*') {
            P++; unary(&rhs);
            val_setnum(out, as_num(out) * as_num(&rhs));
        } else if (*P == '/') {
            double d;
            P++; unary(&rhs);
            d = as_num(&rhs);
            if (d == 0) bas_error("DIVISION BY ZERO");
            val_setnum(out, as_num(out) / d);
        } else return;
    }
}

static void addsub(Val *out)
{
    Val rhs;
    term(out);
    for (;;) {
        sk();
        if (*P == '+') {
            P++; term(&rhs);
            if (out->isstr || rhs.isstr) {          /* string concatenation */
                int n = out->slen + rhs.slen;
                if (n > MAXSTR) n = MAXSTR;
                if (n > out->slen) memcpy(out->s + out->slen, rhs.s, (size_t)(n - out->slen));
                out->isstr = 1;
                out->slen  = n;
            } else {
                val_setnum(out, out->num + rhs.num);
            }
        } else if (*P == '-' ) {
            P++; term(&rhs);
            val_setnum(out, as_num(out) - as_num(&rhs));
        } else return;
    }
}

static void minmax(Val *out)
{
    Val rhs;
    addsub(out);
    for (;;) {
        if (at_terminator()) return;
        if (mt("MIN")) {
            addsub(&rhs);
            val_setnum(out, as_num(out) < as_num(&rhs) ? out->num : rhs.num);
        } else if (mt("MAX")) {
            addsub(&rhs);
            val_setnum(out, as_num(out) > as_num(&rhs) ? out->num : rhs.num);
        } else return;
    }
}

static void relation(Val *out)
{
    minmax(out);
    for (;;) {
        Val rhs;
        int c, r;
        if (at_terminator()) return;
        sk();
        if (mt_ne())                             { minmax(&rhs); c = cmp_vals(out, &rhs); r = c != 0; }
        else if (P[0] == '<' && P[1] == '=')     { P += 2; minmax(&rhs); c = cmp_vals(out, &rhs); r = c <= 0; }
        else if (P[0] == '>' && P[1] == '=')     { P += 2; minmax(&rhs); c = cmp_vals(out, &rhs); r = c >= 0; }
        else if (P[0] == '=' && P[1] == '<')     { P += 2; minmax(&rhs); c = cmp_vals(out, &rhs); r = c <= 0; }
        else if (P[0] == '=' && P[1] == '>')     { P += 2; minmax(&rhs); c = cmp_vals(out, &rhs); r = c >= 0; }
        else if (P[0] == '<')                    { P++;    minmax(&rhs); c = cmp_vals(out, &rhs); r = c <  0; }
        else if (P[0] == '>')                    { P++;    minmax(&rhs); c = cmp_vals(out, &rhs); r = c >  0; }
        else if (P[0] == '=')                    { P++;    minmax(&rhs); c = cmp_vals(out, &rhs); r = c == 0; }
        else return;
        val_setnum(out, r ? 1 : 0);
    }
}

static void notexpr(Val *out)
{
    if (mt("NOT")) { Val t; notexpr(&t); val_setnum(out, as_num(&t) == 0 ? 1 : 0); return; }
    relation(out);
}

static void andexpr(Val *out)
{
    Val rhs;
    notexpr(out);
    for (;;) {
        if (at_terminator()) return;
        if (!mt("AND")) return;
        notexpr(&rhs);
        val_setnum(out, (as_num(out) != 0 && as_num(&rhs) != 0) ? 1 : 0);
    }
}

static void expr(Val *out)
{
    Val rhs;
    andexpr(out);
    for (;;) {
        if (at_terminator()) return;
        if (!mt("OR")) return;
        andexpr(&rhs);
        val_setnum(out, (as_num(out) != 0 || as_num(&rhs) != 0) ? 1 : 0);
    }
}

/* --- user defined functions ----------------------------------------- */

static void call_fn(int idx, Val *out)
{
    Fndef *f = &fns[idx];
    Val args[8];
    double saved[8];
    const char *save_p;
    int i;

    if (f->nparams) {
        need('(');
        for (i = 0; i < f->nparams; i++) {
            if (i) need(',');
            expr(&args[i]);
        }
        need(')');
    } else if (peek('(')) {
        need('('); need(')');
    }
    for (i = 0; i < f->nparams; i++) {
        int L = f->params[i][0], D = f->params[i][1];
        saved[i] = numv[L][D];
        numv[L][D] = as_num(&args[i]);
    }
    save_p = P;
    P = f->body;
    expr(out);
    P = save_p;
    for (i = 0; i < f->nparams; i++) numv[f->params[i][0]][f->params[i][1]] = saved[i];
}

/* ------------------------------------------------------------------ */
/* control flow between statements                                     */
/* ------------------------------------------------------------------ */

enum { NEXT_SEQ, NEXT_GOTO, NEXT_STOP, NEXT_CHAIN };
static int  g_next = NEXT_SEQ;
static int  g_target = 0;
static char g_chain_name[80];
static int  g_chain_line = 0;

static int  g_pc = 0;                   /* index into prog[]           */

typedef struct { int line; } Gframe;
static Gframe gsub[GOSUB_MAX];
static int    ngsub = 0;

typedef struct {
    int    L, D;
    double limit, step;
    int    body;                        /* prog index of the FOR line  */
} Fframe;
static Fframe fstk[FOR_MAX];
static int    nfor = 0;

static void goto_line(int n)
{
    if (find_line(n) < 0) die("UNDEFINED LINE %d", n);
    g_next = NEXT_GOTO;
    g_target = n;
}

/* ------------------------------------------------------------------ */
/* terminal input                                                      */
/* ------------------------------------------------------------------ */

static int g_stdin_tty = 0;

static int read_line(char *buf, int max)
{
    int c, n = 0;
    for (;;) {
        c = fgetc(stdin);
        if (c == EOF) { if (n == 0) return 0; break; }
        if (c == '\n') break;
        if (c == '\r') continue;
        if (n < max - 1) buf[n++] = (char)c;
    }
    buf[n] = 0;
    g_io_serial++;
    if (!g_stdin_tty && g_echo_input) { out_s(buf); out_endline(); }
    out_input_done();
    return 1;
}

/* ENTER waits a bounded number of seconds for a reply */
static int read_line_timed(char *buf, int max, int seconds, int *elapsed)
{
    time_t t0 = time(NULL);
    if (!g_stdin_tty) {
        int r = read_line(buf, max);
        *elapsed = (int)(time(NULL) - t0);
        return r;
    }
#ifdef _WIN32
    {
        /* poll for a real keystroke: a console handle also signals mouse
           and focus events, which would defeat the time limit */
        int waited = 0;
        while (!_kbhit()) {
            if (waited >= seconds * 1000) { *elapsed = seconds; return -1; }
            Sleep(50);
            waited += 50;
        }
    }
#else
    {
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds); FD_SET(0, &fds);
        tv.tv_sec = seconds; tv.tv_usec = 0;
        if (select(1, &fds, NULL, NULL, &tv) <= 0) { *elapsed = seconds; return -1; }
    }
#endif
    {
        int r = read_line(buf, max);
        *elapsed = (int)(time(NULL) - t0);
        return r;
    }
}

/* ------------------------------------------------------------------ */
/* assignment targets                                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    int isstr, L, D;
    int lo, hi;          /* substring / subscripts; hi<0 = open end    */
    int nsub;
} Target;

static int parse_target(Target *t)
{
    int a = 0, b = 0;
    if (!try_varname(&t->L, &t->D, &t->isstr)) return 0;
    t->nsub = subscripts(&a, &b);
    t->lo = a; t->hi = b;
    return 1;
}

static void store_val(Target *t, const Val *v)
{
    if (t->isstr) {
        SVar *s = svar(t->L, t->D);
        if (!v->isstr) bas_error("STRING EXPECTED");
        if (t->nsub == 0)      svar_set(s, v->s, v->slen);
        else if (t->nsub == 1) svar_setsub(s, t->lo, -1, v->s, v->slen);
        else                   svar_setsub(s, t->lo, t->hi, v->s, v->slen);
        return;
    }
    if (v->isstr) bas_error("NUMBER EXPECTED");
    if (t->nsub == 0) { numv[t->L][t->D] = v->num; return; }
    if (t->D != 10) die("ARRAY NAME MUST BE A SINGLE LETTER");
    *arr_elem(t->L, t->lo, t->hi, t->nsub == 2) = v->num;
}

/* ------------------------------------------------------------------ */
/* PRINT                                                               */
/* ------------------------------------------------------------------ */

static char g_last_assign_name[80] = "";

static BFile *file_by_num(int n)
{
    if (n < 1 || n > MAXFILES) bas_error("BAD FILE NUMBER");
    return &files[n];
}

/* an apostrophe introduces a literal control character: PRINT "x"'10 */
static int at_tick(void) { sk(); return *P == 39; }

static void do_print(void)
{
    int tofile = 0, fnum = 0, suppress = 0;
    BFile *f = NULL;

    sk();
    if (*P == 35) {                            /* PRINT #n; ... */
        P++;
        fnum = num_arg();
        f = file_by_num(fnum);
        tofile = 1;
        if (!eat(59)) eat(44);
    }

    for (;;) {
        sk();
        if (*P == 0) break;

        if (at_tick()) {                       /* PRINT "x"'10 */
            int code = 0;
            P++;
            while (isdigit((unsigned char)*P)) code = code * 10 + (*P++ - 48);
            if (tofile) bas_error("CONTROL CHARACTER IN FILE PRINT");
            out_ch(code);
            suppress = 0;
        } else if (mt("END")) {                /* PRINT #n; ... ;END */
            if (!tofile) bas_error("END IN TERMINAL PRINT");
            file_write_eofmark(f);
            suppress = 1;
            break;
        } else if (!tofile && mt("LIN")) {
            int k;
            need(40); k = num_arg(); need(41);
            if (k >= 0) { out_cr(); while (k-- > 0) out_lf(); }
            else        { while (k++ < 0) out_lf(); }
            suppress = 1;
        } else if (!tofile && mt("SPA")) {
            int k;
            need(40); k = num_arg(); need(41);
            while (k-- > 0) out_ch(32);
            suppress = 1;
        } else if (!tofile && mt("TAB")) {
            int k;
            need(40); k = num_arg(); need(41);
            if (k < 1) k = 1;
            if (k - 1 > out_col) out_col = k - 1;
            suppress = 1;
        } else {
            Val v;
            expr(&v);
            suppress = 0;
            if (tofile) {
                file_write_item(f, &v);
            } else if (v.isstr) {
                out_mem(v.s, v.slen);
            } else {
                char buf[64];
                fmt_number(v.num, buf, sizeof buf);
                if (v.num >= 0) out_ch(32);
                out_s(buf);
                out_ch(32);
            }
        }

        sk();
        if (*P == 59) { P++; suppress = 1; continue; }      /* ; */
        if (*P == 44) {                                      /* , */
            P++;
            if (!tofile) out_col = ((out_col / 15) + 1) * 15;
            suppress = 1;
            continue;
        }
        if (*P == 0) break;
        suppress = 1;                        /* implicit semicolon */
    }
    if (!tofile && !suppress) out_endline();
}

/* ------------------------------------------------------------------ */
/* MAT                                                                 */
/* ------------------------------------------------------------------ */

static NArr *parse_arrname(void)
{
    int L;
    sk();
    if (!isalpha((unsigned char)*P)) die("ARRAY NAME EXPECTED");
    L = toupper((unsigned char)*P) - 'A';
    P++;
    if (isalnum((unsigned char)*P) || *P == '$') die("ARRAY NAME EXPECTED");
    return narr(L);
}

static int arr_count(NArr *a) { return a->d1 * (a->d2 > 0 ? a->d2 : 1); }

static double *arr_at(NArr *a, int k)
{
    if (a->d2 > 0) return &a->v[(k / a->d2 + 1) * (a->d2 + 1) + (k % a->d2) + 1];
    return &a->v[k + 1];
}

static void do_mat(void)
{
    if (mt("PRINT")) {
        BFile *f;
        int n;
        sk();
        if (*P != '#') bas_error("MAT PRINT TO TERMINAL NOT SUPPORTED");
        P++;
        n = num_arg();
        f = file_by_num(n);
        if (!eat(';')) eat(',');
        for (;;) {
            NArr *a = parse_arrname();
            int i, cnt = arr_count(a);
            Val v;
            for (i = 0; i < cnt; i++) { val_setnum(&v, *arr_at(a, i)); file_write_item(f, &v); }
            sk();
            if (mt("END")) { file_write_eofmark(f); break; }
            if (*P == ',' || *P == ';') { P++; sk(); if (mt("END")) { file_write_eofmark(f); break; } continue; }
            break;
        }
        return;
    }
    if (mt("READ")) {
        BFile *f;
        int n;
        sk();
        if (*P != '#') bas_error("MAT READ FROM DATA NOT SUPPORTED");
        P++;
        n = num_arg();
        f = file_by_num(n);
        if (!eat(';')) eat(',');
        for (;;) {
            NArr *a = parse_arrname();
            int i, cnt = arr_count(a);
            Val v;
            for (i = 0; i < cnt; i++) { file_read_item(f, &v); *arr_at(a, i) = as_num(&v); }
            sk();
            if (*P == ',' || *P == ';') { P++; continue; }
            break;
        }
        return;
    }
    {
        NArr *a = parse_arrname();
        need('=');
        if (mt("ZER")) {
            int i, cnt = arr_count(a);
            for (i = 0; i < cnt; i++) *arr_at(a, i) = 0;
            return;
        }
        if (mt("CON")) {
            int i, cnt = arr_count(a);
            for (i = 0; i < cnt; i++) *arr_at(a, i) = 1;
            return;
        }
        {
            NArr *b = parse_arrname();
            int i, cnt = arr_count(a), cb = arr_count(b);
            for (i = 0; i < cnt && i < cb; i++) *arr_at(a, i) = *arr_at(b, i);
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* statements                                                          */
/* ------------------------------------------------------------------ */

static void do_assignment(void);
static void exec_statement(void);

/* find the NEXT that closes the FOR on prog line index `from` */
static int find_matching_next(int from, int L, int D)
{
    int i, depth = 0;
    for (i = from + 1; i < nlines; i++) {
        const char *save = P;
        int hit = 0;
        P = prog[i].text;
        if (mt("FOR")) depth++;
        else if (mt("NEXT")) {
            int l2, d2, s2;
            if (try_varname(&l2, &d2, &s2) && !s2 && l2 == L && d2 == D) {
                if (depth == 0) hit = 1; else depth--;
            } else if (depth > 0) depth--;
        }
        P = save;
        if (hit) return i;
    }
    die("FOR WITHOUT NEXT");
    return -1;
}

static void do_for(void)
{
    Target t;
    Val v;
    double init, limit, step = 1;
    int i;

    if (!parse_target(&t) || t.isstr || t.nsub) die("BAD FOR VARIABLE");
    need(61);
    expr(&v); init = as_num(&v);
    if (!mt("TO")) die("EXPECTED TO");
    expr(&v); limit = as_num(&v);
    if (mt("STEP")) { expr(&v); step = as_num(&v); }

    numv[t.L][t.D] = init;

    /* re-entering a loop whose variable is already active reuses its frame */
    for (i = nfor - 1; i >= 0; i--)
        if (fstk[i].L == t.L && fstk[i].D == t.D) { nfor = i; break; }
    if (nfor >= FOR_MAX) die("FOR NESTING TOO DEEP");
    fstk[nfor].L = t.L; fstk[nfor].D = t.D;
    fstk[nfor].limit = limit; fstk[nfor].step = step;
    fstk[nfor].body = g_pc;
    nfor++;

    /* HP tests before the body: FOR A=1 TO 0 runs zero times */
    if ((step >= 0 && init > limit) || (step < 0 && init < limit)) {
        int n = find_matching_next(g_pc, t.L, t.D);
        nfor--;
        g_pc = n;               /* NEXT_SEQ then resumes past the NEXT */
    }
}

static void do_next(void)
{
    int L, D, isstr, i;
    if (!try_varname(&L, &D, &isstr) || isstr) die("BAD NEXT VARIABLE");
    for (i = nfor - 1; i >= 0; i--) if (fstk[i].L == L && fstk[i].D == D) break;
    if (i < 0) die("NEXT WITHOUT FOR");
    nfor = i + 1;
    numv[L][D] += fstk[i].step;
    if ((fstk[i].step >= 0 && numv[L][D] <= fstk[i].limit) ||
        (fstk[i].step <  0 && numv[L][D] >= fstk[i].limit)) {
        g_pc = fstk[i].body;    /* NEXT_SEQ then resumes past the FOR */
    } else {
        nfor = i;
    }
}

static void do_read(void)
{
    sk();
    if (*P == 35) {                                   /* READ #n... */
        BFile *f;
        int n;
        P++;
        n = num_arg();
        f = file_by_num(n);
        sk();
        if (*P == 44) {                               /* READ #n,rec */
            int rec;
            P++;
            rec = num_arg();
            file_seek_record(f, rec);
            return;
        }
        if (!eat(59)) die("EXPECTED ; OR , AFTER FILE NUMBER");
        for (;;) {
            Target t;
            Val v;
            if (!parse_target(&t)) die("VARIABLE EXPECTED");
            file_read_item(f, &v);
            store_val(&t, &v);
            if (!eat(44)) break;
        }
        return;
    }
    for (;;) {                                        /* READ from DATA */
        Target t;
        Val v;
        if (!parse_target(&t)) die("VARIABLE EXPECTED");
        if (dptr >= ndata) bas_error("OUT OF DATA");
        if (dpool[dptr].isstr) val_setstr(&v, dpool[dptr].s, dpool[dptr].slen);
        else                   val_setnum(&v, dpool[dptr].num);
        dptr++;
        store_val(&t, &v);
        if (!eat(44)) break;
    }
}

static void do_convert(void)
{
    Val v;
    Target t;
    expr(&v);
    if (!mt("TO")) die("EXPECTED TO");
    if (!parse_target(&t)) die("VARIABLE EXPECTED");
    if (v.isstr) {
        char buf[MAXSTR + 1];
        int n = v.slen > MAXSTR ? MAXSTR : v.slen;
        char *end;
        double d;
        memcpy(buf, v.s, (size_t)n); buf[n] = 0;
        d = strtod(buf, &end);
        while (*end == ' ') end++;
        if (end == buf || *end) bas_error("CANNOT CONVERT STRING TO NUMBER");
        val_setnum(&v, d);
        store_val(&t, &v);
    } else {
        char buf[64];
        Val s;
        fmt_number(v.num, buf, sizeof buf);
        val_setstr(&s, buf, (int)strlen(buf));
        store_val(&t, &s);
    }
}

static void do_assign_stmt(void)
{
    Val name;
    int fnum;
    Target status;
    BFile *f;
    int rc = 0;

    expr(&name);
    if (!name.isstr) bas_error("FILE NAME EXPECTED");
    need(44);
    fnum = num_arg();
    need(44);
    if (!parse_target(&status)) die("STATUS VARIABLE EXPECTED");
    if (eat(44)) { Val pw; expr(&pw); }    /* password: files here have none */

    f = file_by_num(fnum);
    file_close(f);
    {
        char nm[80];
        int n = name.slen > 79 ? 79 : name.slen;
        memcpy(nm, name.s, (size_t)n); nm[n] = 0;
        while (n > 0 && nm[n - 1] == ' ') nm[--n] = 0;
        snprintf(g_last_assign_name, sizeof g_last_assign_name, "%s", nm);
        if (!file_exists(nm)) {
            rc = 2;                        /* no such file            */
        } else if (!file_load(nm, f)) {
            rc = 2;
        } else {
            f->used = 1; f->readonly = 0; f->eofline = 0;
            rc = 0;
        }
    }
    /* "ASSIGN ... : IF status THEN retry" is an HP idiom for waiting on a
       file another user holds open.  There are no other users here, so a
       file that is missing will never appear and the retry is an unbreakable
       spin -- which is exactly what the game does at 2370/2380 for Score.
       Two failures for the same name, from the same line, with no terminal
       I/O in between, can only be that loop. */
    if (rc == 2) {
        static int           last_line = -1;
        static char          last_name[80];
        static unsigned long last_io;
        if (g_curline == last_line && g_io_serial == last_io &&
            strcmp(last_name, g_last_assign_name) == 0) {
            out_flushline();
            fflush(stdout);
            fprintf(stderr,
                "\n*** the file %s does not exist in %s, and line %d retries\n"
                "*** forever waiting for it.  Run with --rebuild to recreate it.\n",
                g_last_assign_name, g_datadir, g_curline);
            exit(2);
        }
        last_line = g_curline;
        last_io   = g_io_serial;
        snprintf(last_name, sizeof last_name, "%s", g_last_assign_name);
    }
    { Val v; val_setnum(&v, rc); store_val(&status, &v); }
}

static void do_create(void)
{
    Target status;
    Val name;
    int nrec, rc;
    char nm[80];
    int n;

    if (!parse_target(&status)) die("STATUS VARIABLE EXPECTED");
    need(44);
    expr(&name);
    if (!name.isstr) bas_error("FILE NAME EXPECTED");
    need(44);
    nrec = num_arg();
    n = name.slen > 79 ? 79 : name.slen;
    memcpy(nm, name.s, (size_t)n); nm[n] = 0;
    while (n > 0 && nm[n - 1] == ' ') nm[--n] = 0;
    rc = file_exists(nm) ? 1 : file_create(nm, nrec);
    { Val v; val_setnum(&v, rc); store_val(&status, &v); }
}

static void do_purge(void)
{
    Target status;
    Val name;
    int rc;
    char nm[80];
    int n;

    if (!parse_target(&status)) die("STATUS VARIABLE EXPECTED");
    need(44);
    expr(&name);
    if (!name.isstr) bas_error("FILE NAME EXPECTED");
    n = name.slen > 79 ? 79 : name.slen;
    memcpy(nm, name.s, (size_t)n); nm[n] = 0;
    while (n > 0 && nm[n - 1] == ' ') nm[--n] = 0;
    rc = file_purge(nm);
    { Val v; val_setnum(&v, rc); store_val(&status, &v); }
}

static void do_advance(void)
{
    BFile *f;
    int n, count, i, rc = 0;
    Target status;
    sk();
    if (*P != 35) die("EXPECTED # AFTER ADVANCE");
    P++;
    n = num_arg();
    f = file_by_num(n);
    if (!eat(59)) need(44);
    count = num_arg();
    need(44);
    if (!parse_target(&status)) die("STATUS VARIABLE EXPECTED");
    for (i = 0; i < count; i++) if (!file_skip_item(f)) { rc = 1; break; }
    { Val v; val_setnum(&v, rc); store_val(&status, &v); }
}

static void do_update(void)
{
    BFile *f;
    int n;
    sk();
    if (*P != 35) die("EXPECTED # AFTER UPDATE");
    P++;
    n = num_arg();
    f = file_by_num(n);
    if (!eat(59)) need(44);
    for (;;) {
        Val v;
        sk();
        if (mt("END")) { file_write_eofmark(f); break; }
        expr(&v);
        file_write_item(f, &v);
        sk();
        if (*P == 44 || *P == 59) { P++; continue; }
        break;
    }
}

static void do_linput(void)
{
    Target t;
    char buf[MAXSTR];
    Val v;
    sk();
    if (*P == 35) { int n; P++; n = num_arg(); (void)n; if (!eat(59)) need(44); }
    if (!parse_target(&t)) die("STRING VARIABLE EXPECTED");
    if (!read_line(buf, sizeof buf)) { g_next = NEXT_STOP; return; }
    val_setstr(&v, buf, (int)strlen(buf));
    store_val(&t, &v);
}

static void do_input(void)
{
    char buf[MAXSTR];
    char *p;
    out_ch(63); out_ch(32);
    if (!read_line(buf, sizeof buf)) { g_next = NEXT_STOP; return; }
    p = buf;
    for (;;) {
        Target t;
        Val v;
        if (!parse_target(&t)) die("VARIABLE EXPECTED");
        {
            char *comma = strchr(p, 44);
            if (comma) *comma = 0;
            if (t.isstr) {
                val_setstr(&v, p, (int)strlen(p));
            } else {
                val_setnum(&v, atof(p));
            }
            store_val(&t, &v);
            p = comma ? comma + 1 : p + strlen(p);
        }
        if (!eat(44)) break;
    }
}

static void do_enter(void)
{
    int seconds, elapsed = 0, r;
    Target status, tgt;
    char buf[MAXSTR];
    Val v;

    seconds = num_arg();
    need(44);
    if (!parse_target(&status)) die("STATUS VARIABLE EXPECTED");
    need(44);
    if (!parse_target(&tgt)) die("VARIABLE EXPECTED");

    r = read_line_timed(buf, sizeof buf, seconds, &elapsed);
    if (r <= 0) {                             /* timed out or no input */
        val_setnum(&v, -256);
        store_val(&status, &v);
        return;
    }
    if (tgt.isstr) {
        val_setstr(&v, buf, (int)strlen(buf));
        store_val(&tgt, &v);
        val_setnum(&v, elapsed);
        store_val(&status, &v);
        return;
    }
    {
        char *end;
        double d = strtod(buf, &end);
        while (*end == 32) end++;
        if (end == buf || *end) { val_setnum(&v, -1); store_val(&status, &v); return; }
        val_setnum(&v, d);
        store_val(&tgt, &v);
        val_setnum(&v, elapsed);
        store_val(&status, &v);
    }
}

static void do_system(void)
{
    Target t;
    Val v;
    if (!parse_target(&t)) die("VARIABLE EXPECTED");
    if (eat(44)) { Val k; expr(&k); }
    val_setstr(&v, g_userid, (int)strlen(g_userid));
    store_val(&t, &v);
}

static void do_chain(void)
{
    Val name;
    const char *save = P;
    int n;

    sk();
    /* CHAIN status,"name"[,line] as well as CHAIN "name"[,line] */
    if (*P != 34) {
        Target st;
        if (parse_target(&st) && eat(44)) {
            Val z;
            val_setnum(&z, 0);
            store_val(&st, &z);
        } else {
            P = save;
        }
    }
    expr(&name);
    if (!name.isstr) bas_error("PROGRAM NAME EXPECTED");
    n = name.slen > 79 ? 79 : name.slen;
    memcpy(g_chain_name, name.s, (size_t)n);
    g_chain_name[n] = 0;
    while (n > 0 && g_chain_name[n - 1] == 32) g_chain_name[--n] = 0;
    g_chain_line = 0;
    if (eat(44)) g_chain_line = num_arg();
    g_next = NEXT_CHAIN;
}

static void do_if(void)
{
    const char *save = P;
    Val v;

    if (mt("END")) {                              /* IF END #n THEN nnn */
        sk();
        if (*P == 35) {
            BFile *f;
            int n;
            P++;
            n = num_arg();
            f = file_by_num(n);
            if (!mt("THEN")) die("EXPECTED THEN");
            f->eofline = num_arg();
            return;
        }
        P = save;
    }
    if (mt("ERROR")) {                            /* IF ERROR THEN nnn  */
        sk();
        if (mt("THEN")) { g_errline = num_arg(); return; }
        P = save;
    }
    expr(&v);
    if (!mt("THEN")) die("EXPECTED THEN");
    if (as_num(&v) != 0) {
        sk();
        if (isdigit((unsigned char)*P)) goto_line(num_arg());
        else exec_statement();
    }
}

/* ON END #n THEN nnn -- the HP 3000 spelling of IF END */
static void do_on(void)
{
    if (mt("END")) {
        BFile *f;
        int n;
        sk();
        if (*P != 35) die("EXPECTED # AFTER ON END");
        P++;
        n = num_arg();
        f = file_by_num(n);
        if (!mt("THEN")) die("EXPECTED THEN");
        f->eofline = num_arg();
        return;
    }
    if (mt("ERROR")) {
        if (!mt("THEN")) die("EXPECTED THEN");
        g_errline = num_arg();
        return;
    }
    die("BAD ON STATEMENT");
}

static void do_goto_or_gosub(int isgosub)
{
    Val v;
    int target;
    expr(&v);
    if (mt("OF")) {                               /* computed branch    */
        int k = as_int(&v), i = 1, line = 0;
        for (;;) {
            int ln = num_arg();
            if (i == k) line = ln;
            i++;
            if (!eat(44)) break;
        }
        if (k < 1 || line == 0) return;           /* out of range: fall through */
        if (isgosub) { if (ngsub >= GOSUB_MAX) die("GOSUB NESTING TOO DEEP"); gsub[ngsub++].line = g_pc; }
        goto_line(line);
        return;
    }
    target = as_int(&v);
    if (isgosub) { if (ngsub >= GOSUB_MAX) die("GOSUB NESTING TOO DEEP"); gsub[ngsub++].line = g_pc; }
    goto_line(target);
}

static void do_assignment(void)
{
    const char *start = P, *q;
    int depth = 0, instr = 0, neq = 0, i;
    int eqpos[20];
    Target tg[20];
    Val v;

    /* v1=v2=...=expr assigns one value to every target.  Only equals
       signs at bracket depth zero separate targets, which is exactly why
       the game writes P0=P0-(L=K1) with the comparison parenthesised. */
    for (q = start; *q; q++) {
        if (instr) { if (*q == 34) instr = 0; continue; }
        if (*q == 34) { instr = 1; continue; }
        if (*q == 40 || *q == 91) depth++;
        else if (*q == 41 || *q == 93) depth--;
        else if (*q == 61 && depth == 0) {
            if (q > start && (q[-1] == 60 || q[-1] == 62)) continue;   /* <= >= */
            if (q[1] == 60 || q[1] == 62) { q++; continue; }           /* =< => */
            if (neq < 20) eqpos[neq++] = (int)(q - start);
        }
    }
    if (neq == 0) die("SYNTAX ERROR");

    for (i = 0; i < neq; i++) {
        P = start + (i ? eqpos[i - 1] + 1 : 0);
        if (!parse_target(&tg[i])) die("SYNTAX ERROR");
        sk();
        if ((int)(P - start) != eqpos[i]) die("SYNTAX ERROR");
    }
    P = start + eqpos[neq - 1] + 1;
    expr(&v);
    for (i = neq - 1; i >= 0; i--) store_val(&tg[i], &v);
}

static void exec_statement(void)
{
    sk();
    if (*P == 0) return;

    if (mt("REM"))     return;
    if (mt("DATA"))    return;
    if (mt("COM"))     return;
    if (mt("DIM"))     return;
    if (mt("FILES"))   return;
    if (mt("DEF"))     return;
    if (mt("IMAGE"))   return;

    if (mt("PRINT"))   { do_print();   return; }
    if (mt("LET"))     { do_assignment(); return; }
    if (mt("IF"))      { do_if();      return; }
    if (mt("ON"))      { do_on();      return; }
    if (mt("GOTO"))    { do_goto_or_gosub(0); return; }
    if (mt("GOSUB"))   { do_goto_or_gosub(1); return; }
    if (mt("RETURN"))  {
        if (ngsub == 0) die("RETURN WITHOUT GOSUB");
        g_pc = gsub[--ngsub].line;
        g_next = NEXT_SEQ;
        return;
    }
    if (mt("FOR"))     { do_for();     return; }
    if (mt("NEXT"))    { do_next();    return; }
    if (mt("READ"))    { do_read();    return; }
    if (mt("RESTORE")) {
        sk();
        if (isdigit((unsigned char)*P)) {
            int n = num_arg(), i;
            dptr = ndata;
            for (i = 0; i < ndata; i++) if (dpool[i].line >= n) { dptr = i; break; }
        } else dptr = 0;
        return;
    }
    if (mt("MAT"))     { do_mat();     return; }
    if (mt("LINPUT"))  { do_linput();  return; }
    if (mt("INPUT"))   { do_input();   return; }
    if (mt("ENTER"))   { do_enter();   return; }
    if (mt("CONVERT")) { do_convert(); return; }
    if (mt("ASSIGN"))  { do_assign_stmt(); return; }
    if (mt("CREATE"))  { do_create();  return; }
    if (mt("PURGE"))   { do_purge();   return; }
    if (mt("ADVANCE")) { do_advance(); return; }
    if (mt("UPDATE"))  { do_update();  return; }
    if (mt("CHAIN"))   { do_chain();   return; }
    if (mt("SYSTEM"))  { do_system();  return; }
    if (mt("STOP"))    { g_next = NEXT_STOP; return; }
    if (mt("END"))     { g_next = NEXT_STOP; return; }

    do_assignment();
}

/* ------------------------------------------------------------------ */
/* loading a program                                                   */
/* ------------------------------------------------------------------ */

static void clear_uncommon(void)
{
    int L, D;
    for (L = 0; L < 26; L++) {
        for (D = 0; D < 11; D++) {
            if (!com_num[L][D]) numv[L][D] = 0;
            if (!com_str[L][D] && strv[L][D].buf) {
                memset(strv[L][D].buf, 32, (size_t)strv[L][D].phys);
                strv[L][D].len = 0;
            }
        }
        if (!com_arr[L] && arrv[L].v) {
            int n = (arrv[L].d1 + 1) * (arrv[L].d2 > 0 ? arrv[L].d2 + 1 : 1);
            memset(arrv[L].v, 0, (size_t)n * sizeof(double));
        }
    }
}

/* one entry of a COM or DIM list */
static void declare_one(int common)
{
    int L, D, isstr, a = 0, b = 0, ns;
    if (!try_varname(&L, &D, &isstr)) die("BAD DECLARATION");
    ns = subscripts(&a, &b);
    if (isstr) {
        int phys = (ns >= 1) ? a : 1;
        SVar *s = &strv[L][D];
        if (!(common && com_str[L][D] && s->buf && s->phys == phys))
            svar_dim(s, phys);
        if (common) com_str[L][D] = 1;
        return;
    }
    if (ns == 0) {
        if (common) com_num[L][D] = 1;
        return;
    }
    if (D != 10) die("ARRAY NAME MUST BE A SINGLE LETTER");
    if (!(common && com_arr[L] && arrv[L].v && arrv[L].d1 == a && arrv[L].d2 == (ns == 2 ? b : 0)))
        arr_dim(&arrv[L], a, ns == 2 ? b : 0);
    if (common) com_arr[L] = 1;
}

static void open_files_decl(void)
{
    int n = 1;
    for (;;) {
        char name[80];
        int k = 0;
        sk();
        if (*P == 42) {                        /* '*' leaves it unassigned */
            P++;
            files[n].used = 0;
        } else {
            while (*P && *P != 44) {
                if (k < 79) name[k++] = *P;
                P++;
            }
            while (k > 0 && name[k - 1] == 32) k--;
            name[k] = 0;
            file_close(&files[n]);
            if (!file_load(name, &files[n]))
                die("FILE %s NOT FOUND IN %s", name, g_datadir);
            files[n].used = 1;
            files[n].eofline = 0;
        }
        n++;
        if (n > MAXFILES) break;
        sk();
        if (*P != 44) break;
        P++;
    }
}

static void define_fn(void)
{
    Fndef *f;
    int L, D = 10, isstr = 0;
    if (nfns >= 32) die("TOO MANY FUNCTIONS");
    f = &fns[nfns];
    if (!mt("FN")) die("EXPECTED FN");
    sk();
    if (!isalpha((unsigned char)*P)) die("BAD FN NAME");
    L = toupper((unsigned char)*P) - 'A'; P++;
    if (isdigit((unsigned char)*P)) { D = *P - '0'; P++; }
    if (*P == 36) { isstr = 1; P++; }
    f->letter = L; f->digit = D; f->isstr = isstr; f->nparams = 0;
    if (eat(40)) {
        for (;;) {
            int pl, pd, ps;
            if (!try_varname(&pl, &pd, &ps) || ps) die("BAD FN PARAMETER");
            if (f->nparams < 8) {
                f->params[f->nparams][0] = (unsigned char)pl;
                f->params[f->nparams][1] = (unsigned char)pd;
                f->nparams++;
            }
            if (!eat(44)) break;
        }
        need(41);
    }
    need(61);
    sk();
    f->body = strdup(P);
    nfns++;
}

static void collect_data(int lineno)
{
    for (;;) {
        DItem d;
        memset(&d, 0, sizeof d);
        d.line = lineno;
        sk();
        if (*P == 34) {
            char buf[MAXSTR];
            int n = 0;
            P++;
            while (*P && *P != 34) { if (n < MAXSTR) buf[n++] = *P; P++; }
            if (*P == 34) P++;
            d.isstr = 1; d.slen = n;
            d.s = (char *)malloc((size_t)n + 1);
            memcpy(d.s, buf, (size_t)n); d.s[n] = 0;
        } else {
            char buf[128];
            int n = 0;
            while (*P && *P != 44) { if (n < 127) buf[n++] = *P; P++; }
            while (n > 0 && buf[n - 1] == 32) n--;
            buf[n] = 0;
            if (n == 0) break;
            d.isstr = 0;
            d.num = atof(buf);
        }
        data_add(d);
        sk();
        if (*P != 44) break;
        P++;
    }
}

static int cmp_line(const void *a, const void *b)
{
    return ((const Line *)a)->num - ((const Line *)b)->num;
}

static int try_open_prog(const char *name, char *path, size_t psz)
{
    FILE *fp;
    char bare[80], lo[80], up[80];
    size_t i;
    resolve_name(name, bare, sizeof bare);
    for (i = 0; i < sizeof lo && bare[i]; i++) { lo[i] = (char)tolower((unsigned char)bare[i]); up[i] = (char)toupper((unsigned char)bare[i]); }
    lo[i] = 0; up[i] = 0;
    snprintf(path, psz, "%s/%s.txt", g_progdir, bare);
    if ((fp = fopen(path, "rb"))) { fclose(fp); return 1; }
    snprintf(path, psz, "%s/%s.txt", g_progdir, lo);
    if ((fp = fopen(path, "rb"))) { fclose(fp); return 1; }
    snprintf(path, psz, "%s/%s.txt", g_progdir, up);
    if ((fp = fopen(path, "rb"))) { fclose(fp); return 1; }
    return 0;
}

static void load_program(const char *name, int startline)
{
    char path[1200], buf[1024];
    FILE *fp;
    int i;

    for (i = 0; i < nlines; i++) free(prog[i].text);
    nlines = 0;
    for (i = 0; i < ndata; i++) free(dpool[i].s);
    ndata = 0; dptr = 0;
    for (i = 0; i < nfns; i++) free(fns[i].body);
    nfns = 0;

    if (!try_open_prog(name, path, sizeof path))
        die("PROGRAM %s NOT FOUND IN %s", name, g_progdir);
    if (!(fp = fopen(path, "rb"))) die("CANNOT OPEN %s", path);
    snprintf(g_progname, sizeof g_progname, "%s", name);

    while (fgets(buf, sizeof buf, fp)) {
        char *s = buf, *e;
        int num;
        while (*s == 32 || *s == 9) s++;
        if (!isdigit((unsigned char)*s)) continue;   /* listing headers */
        num = atoi(s);
        while (isdigit((unsigned char)*s)) s++;
        while (*s == 32 || *s == 9) s++;
        e = s + strlen(s);
        while (e > s && (e[-1] == 10 || e[-1] == 13 || e[-1] == 32 || e[-1] == 9)) *--e = 0;
        if (nlines >= MAXLINES) die("PROGRAM TOO LARGE");
        prog[nlines].num  = num;
        prog[nlines].text = strdup(s);
        nlines++;
    }
    fclose(fp);
    qsort(prog, (size_t)nlines, sizeof(Line), cmp_line);

    /* COM, DIM, FILES, DEF and DATA are declarations: they take effect
       when the program is loaded, not when control reaches them, which
       is what makes CHAIN "ADVENT",1830 work */
    for (i = 0; i < nlines; i++) {
        g_curline = prog[i].num;
        P = prog[i].text;
        if (mt("COM"))        { for (;;) { declare_one(1); if (!eat(44)) break; } }
        else if (mt("DIM"))   { for (;;) { declare_one(0); if (!eat(44)) break; } }
        else if (mt("FILES")) { open_files_decl(); }
        else if (mt("DEF"))   { define_fn(); }
        else if (mt("DATA"))  { collect_data(prog[i].num); }
    }

    g_pc = 0;
    if (startline) {
        int k = find_line(startline);
        if (k < 0) die("CHAIN TO UNDEFINED LINE %d", startline);
        g_pc = k;
    }
}

/* ------------------------------------------------------------------ */
/* the interpreter loop                                                */
/* ------------------------------------------------------------------ */

static void run(void)
{
    for (;;) {
        if (g_pc < 0 || g_pc >= nlines) return;
        g_curline = prog[g_pc].num;
        P = prog[g_pc].text;
        g_next = NEXT_SEQ;

        if (setjmp(g_eofjmp) == 0) {
            if (setjmp(g_errjmp) == 0) {
                exec_statement();
            } else {
                g_next = NEXT_GOTO;
                g_target = g_errline;
            }
        } else {
            g_next = NEXT_GOTO;
            g_target = g_eof_target;
        }

        switch (g_next) {
        case NEXT_SEQ:
            g_pc++;
            break;
        case NEXT_GOTO:
            g_pc = find_line(g_target);
            if (g_pc < 0) die("UNDEFINED LINE %d", g_target);
            break;
        case NEXT_STOP:
            return;
        case NEXT_CHAIN: {
            int i;
            for (i = 1; i <= MAXFILES; i++) file_close(&files[i]);
            clear_uncommon();
            ngsub = 0; nfor = 0; g_errline = 0;
            load_program(g_chain_name, g_chain_line);
            break;
        }
        }
    }
}

/* ------------------------------------------------------------------ */
/* entry point                                                         */
/* ------------------------------------------------------------------ */

static void reset_all(void)
{
    int L, D, i;
    for (i = 1; i <= MAXFILES; i++) file_close(&files[i]);
    for (L = 0; L < 26; L++) {
        for (D = 0; D < 11; D++) {
            numv[L][D] = 0;
            com_num[L][D] = com_str[L][D] = 0;
            if (strv[L][D].buf) { free(strv[L][D].buf); strv[L][D].buf = NULL; strv[L][D].phys = 0; strv[L][D].len = 0; }
        }
        com_arr[L] = 0;
        if (arrv[L].v) { free(arrv[L].v); arrv[L].v = NULL; arrv[L].d1 = arrv[L].d2 = 0; }
    }
    ngsub = 0; nfor = 0; dptr = 0; g_errline = 0;
    g_rndstate = g_randomize ? (unsigned long)time(NULL) : 37;
}


/* ------------------------------------------------------------------ */
/* game mode                                                           */
/*                                                                     */
/* Everything below this line is the wrapper that stands in for the HP */
/* 2000 account the game lived in: it builds the data files by running */
/* the builder programs, provides the empty Score and Bug files the    */
/* game expects to find already catalogued, and then RUNs ADVENT.      */
/* ------------------------------------------------------------------ */

static const char *MK_PROGRAMS[] = {
    "mkmatrix", "mkshort", "mkdescrp", "mkobject", "mkthings",
    "mkcmnds",  "mknote",  "mkhelp",   "mkread",   NULL
};

static void exe_dir(const char *argv0, char *out, size_t n)
{
    char buf[500];
    char *p;
#ifdef _WIN32
    DWORD r = GetModuleFileNameA(NULL, buf, (DWORD)sizeof buf);
    if (r == 0 || r >= sizeof buf) snprintf(buf, sizeof buf, "%s", argv0);
#else
    snprintf(buf, sizeof buf, "%s", argv0);
#endif
    for (p = buf; *p; p++) if (*p == 92) *p = 47;
    p = strrchr(buf, 47);
    if (p) *p = 0; else snprintf(buf, sizeof buf, ".");
    snprintf(out, n, "%s", buf);
}

static void make_dir(const char *path)
{
#ifdef _WIN32
    CreateDirectoryA(path, NULL);
#else
    mkdir(path, 0755);
#endif
}

static void run_one(const char *name, int startline)
{
    reset_all();
    load_program(name, startline);
    run();
    {
        int k;
        for (k = 1; k <= MAXFILES; k++) file_close(&files[k]);
    }
}

/* an empty HP data file of n records, as CREATE would leave it */
static int ensure_empty_file(const char *name, int nrec)
{
    if (file_exists(name)) return 0;
    if (file_create(name, nrec) != 0) {
        fprintf(stderr, "cannot create %s in %s\n", name, g_datadir);
        return 1;
    }
    return 0;
}

/* Score and Bug are part of the account, not part of the game data, and the
   game assumes both are already catalogued: ASSIGN "Score" failing puts it
   in an unbreakable retry loop.  Check them on every start, not just when
   the data files are being rebuilt. */
static int ensure_account_files(void)
{
    int bad = 0;
    bad |= ensure_empty_file("Score", 5);
    bad |= ensure_empty_file("Bug",  15);
    return bad;
}

static int data_is_built(void)
{
    static const char *need[] = { "MATRIX","SHORT","DESCRP","OBJECT","THINGS",
                                  "COMMANDS","NOTE","HELP","READABLE", NULL };
    int i;
    for (i = 0; need[i]; i++) if (!file_exists(need[i])) return 0;
    return 1;
}

static void build_data(void)
{
    int i;
    g_silent = 1;
    for (i = 0; MK_PROGRAMS[i]; i++) run_one(MK_PROGRAMS[i], 0);
    g_silent = 0;
}

static int game_mode(const char *argv0, const char *variant, int rebuild)
{
    char dir[512];
    exe_dir(argv0, dir, sizeof dir);
    if (!g_progdir_set)
        snprintf(g_progdir, sizeof g_progdir, "%s/basic/%s", dir, variant);
    if (g_datadir_set) {
        make_dir(g_datadir);
    } else {
        snprintf(g_datadir, sizeof g_datadir, "%s/data", dir);
        make_dir(g_datadir);
        snprintf(g_datadir, sizeof g_datadir, "%s/data/%s", dir, variant);
        make_dir(g_datadir);
    }

    {
        FILE *fp;
        char probe[1200];
        snprintf(probe, sizeof probe, "%s/advent.txt", g_progdir);
        if (!(fp = fopen(probe, "rb"))) {
            fprintf(stderr, "cannot find %s\n", probe);
            return 2;
        }
        fclose(fp);
    }

    load_aliases();
    if (rebuild || !data_is_built()) build_data();
    if (ensure_account_files()) return 2;

    reset_all();
    load_program("ADVENT", 0);
    run();
    {
        int k;
        for (k = 1; k <= MAXFILES; k++) file_close(&files[k]);
    }
    out_flushline();
    if (!g_quiet_done) { out_s("DONE"); out_endline(); }
    fflush(stdout);
    return 0;
}

static void usage(void)
{
    fprintf(stderr,
        "Adventure ]I[ -- HP 2000 Access Time-Shared BASIC\n"
        "\n"
        "  advent                 play the Guma reconstruction\n"
        "  advent -v orig         play the 1981 Hammerstone printout\n"
        "  advent --rebuild       rebuild the data files first\n"
        "\n"
        "interpreter options (for running any HP 2000 BASIC program):\n"
        "  -p DIR   directory holding the BASIC source (also honoured in game mode)\n"
        "  -d DIR   directory holding the data files   (also honoured in game mode)\n"
        "  -u ID    account id reported by SYSTEM        (default B500)\n"
        "  -r       reseed RND from the clock\n"
        "  -e       echo input lines when stdin is not a terminal\n"
        "  -q       do not print DONE when a program stops\n"
        "  PROG...  run these programs instead of the game\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int i, nprog = 0, rebuild = 0;
    char *progs[16];
    const char *variant = "recon";

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rebuild")) { rebuild = 1; continue; }
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) usage();
        if (argv[i][0] == 45 && argv[i][1] && !argv[i][2]) {
            switch (argv[i][1]) {
            case 'v': if (++i >= argc) usage(); variant = argv[i]; break;
            case 'p': if (++i >= argc) usage(); snprintf(g_progdir, sizeof g_progdir, "%s", argv[i]); g_progdir_set = 1; break;
            case 'd': if (++i >= argc) usage(); snprintf(g_datadir, sizeof g_datadir, "%s", argv[i]); g_datadir_set = 1; break;
            case 'u': if (++i >= argc) usage(); snprintf(g_userid,  sizeof g_userid,  "%s", argv[i]); break;
            case 'r': g_randomize = 1; break;
            case 'e': g_echo_input = 1; break;
            case 'q': g_quiet_done = 1; break;
            default: usage();
            }
        } else if (nprog < 16) {
            progs[nprog++] = argv[i];
        }
    }

    g_stdin_tty = ISATTY(stdin) ? 1 : 0;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (!nprog) {
        if (strcmp(variant, "recon") && strcmp(variant, "orig")) {
            fprintf(stderr, "unknown variant %s (use recon or orig)\n", variant);
            return 1;
        }
        return game_mode(argv[0], variant, rebuild);
    }

    load_aliases();
    for (i = 0; i < nprog; i++) {
        run_one(progs[i], 0);
        out_flushline();
        if (!g_quiet_done) { out_s("DONE"); out_endline(); }
    }
    fflush(stdout);
    return 0;
}
