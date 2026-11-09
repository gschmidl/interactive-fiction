/* Runtime for the translated HPL programs of NERO0366 (see rt.h). */
#include "rt.h"

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

void game_run(int prog, int line);        /* generated */
extern const char *const game_progs[];   /* generated */
extern int rt_prog, rt_line;              /* generated code keeps these current */

int opt_fixes = 1;
int rt_interactive;
int opt_width = 79;
double flg13;
static int opt_echo = -1;                 /* -1: echo when stdin is not a console */
static int opt_pause = 1;
static int opt_reflow;
static int have_seed;
static unsigned long long opt_seed;
static char save_path[1024];

/* ======================================================================== */
/* arena                                                                     */
/* ======================================================================== */
#define ARENA (8u << 20)
static unsigned char arena[ARENA];
static size_t atop;

size_t rt_mark(void) { return atop; }
void rt_release(size_t m) { atop = m; }

static unsigned char *alloc(int n)
{
    if (n < 0) n = 0;
    if (atop + (size_t)n + 1 > ARENA) rt_fail("string arena exhausted", rt_prog, rt_line);
    unsigned char *p = arena + atop;
    atop += (size_t)n + 1;
    return p;
}

static SV mk(const unsigned char *d, int len)
{
    SV s;
    unsigned char *p = alloc(len);
    if (len > 0) memcpy(p, d, (size_t)len);
    s.len = len;
    s.d = p;
    return s;
}

/* ======================================================================== */
/* numbers                                                                   */
/* ======================================================================== */
long rt_round(double x)
{
    return x >= 0 ? (long)floor(x + 0.5) : -(long)floor(-x + 0.5);
}

double rt_int(double x) { return x >= 0 ? floor(x) : -floor(-x); }

double rt_mod(double a, double b)
{
    if (b == 0) rt_fail("mod by zero", rt_prog, rt_line);
    return a - rt_int(a / b) * b;
}

double rt_prnd(double x, double n)
{
    double s = 1;
    for (long k = rt_round(n); k > 0; k--) s *= 10;
    for (long k = rt_round(n); k < 0; k++) s /= 10;
    return (double)rt_round(x * s) / s;
}

static unsigned long long rng;
static unsigned long long next64(void)
{
    unsigned long long z = (rng += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* rnd(x): x < 0 seeds (the game does rnd(-pi) once at the start; the port
   seeds from the clock instead, or from --seed), else the next number in
   [0,1). */
double rt_rnd(double x)
{
    if (x < 0) rng = have_seed ? opt_seed : ((unsigned long long)time(0) * 2654435761ull) ^ (unsigned long long)clock();
    return (double)(next64() >> 11) / 9007199254740992.0;
}

double rt_bit(double n, double x) { return (double)((rt_round(x) >> rt_round(n)) & 1); }
double rt_max(double a, double b) { return a > b ? a : b; }
double rt_min(double a, double b) { return a < b ? a : b; }

double *rt_aptr(NArr *a, double i)
{
    long k = rt_round(i);
    if (!a->d) rt_fail("array not dimensioned", rt_prog, rt_line);
    if (k < 1 || k > a->n) rt_fail("subscript out of range", rt_prog, rt_line);
    return &a->d[k];
}

void rt_ndim(NArr *a, double n)
{
    long k = rt_round(n);
    a->n = (int)k;
    a->d = calloc((size_t)k + 1, sizeof(double));
}

/* ======================================================================== */
/* strings                                                                   */
/* ======================================================================== */
void rt_sdim(HSVar *v, double n, double max)
{
    int k = n < 0 ? 0 : (int)rt_round(n);
    v->n = k;
    v->max = (int)rt_round(max);
    v->len = calloc((size_t)k + 1, sizeof(int));
    v->d = calloc((size_t)(k + 1) * (size_t)v->max + 1, 1);
}

SV rt_lit(const char *s, int len)
{
    SV v;
    v.len = len;
    v.d = (const unsigned char *)s;
    return v;
}

static int element(HSVar *v, int nsub, double *a, double *b, double *c, double *s1, double *s2)
{
    /* returns element index; s1/s2 the substring subscripts left (count in return via nsub) */
    if (!v->d) rt_fail("string not dimensioned", rt_prog, rt_line);
    if (v->n == 0) {
        *s1 = *a;
        *s2 = *b;
        (void)c;
        return 0;
    }
    if (nsub < 1) rt_fail("string array used without subscript", rt_prog, rt_line);
    long e = rt_round(*a);
    if (e < 1 || e > v->n) rt_fail("string element out of range", rt_prog, rt_line);
    *s1 = *b;
    *s2 = *c;
    return (int)e;
}

SV rt_sget(HSVar *v, int nsub, double a, double b, double c)
{
    double s1, s2;
    int e = element(v, nsub, &a, &b, &c, &s1, &s2);
    int ns = v->n ? nsub - 1 : nsub;
    const unsigned char *d = v->d + (size_t)e * (size_t)v->max;
    int len = v->len[e];
    long i = 1, j = len;
    if (ns >= 1) i = rt_round(s1);
    if (ns >= 2) j = rt_round(s2);
    if (i < 1) rt_fail("substring start below 1", rt_prog, rt_line);
    if (j > len) j = len;
    if (i > j) return mk(d, 0);
    return mk(d + i - 1, (int)(j - i + 1));
}

SV rt_sset(HSVar *v, int nsub, double a, double b, double c, SV val)
{
    double s1, s2;
    SV src = mk(val.d, val.len);           /* the value may overlap the target */
    int e = element(v, nsub, &a, &b, &c, &s1, &s2);
    int ns = v->n ? nsub - 1 : nsub;
    unsigned char *d = v->d + (size_t)e * (size_t)v->max;
    int len = v->len[e], max = v->max;
    if (ns == 0) {
        int n = src.len > max ? max : src.len;
        memcpy(d, src.d, (size_t)n);
        v->len[e] = n;
        return val;
    }
    long i = rt_round(s1);
    if (i < 1 || i > max) rt_fail("substring start out of range", rt_prog, rt_line);
    for (long k = len; k < i - 1; k++) d[k] = ' ';
    if (ns == 1) {
        long n = src.len;
        if (i - 1 + n > max) n = max - (i - 1);
        memcpy(d + i - 1, src.d, (size_t)n);
        v->len[e] = (int)(i - 1 + n);
        return val;
    }
    long j = rt_round(s2);
    if (j > max) j = max;
    long w = j - i + 1;
    for (long k = 0; k < w; k++) d[i - 1 + k] = k < src.len ? src.d[k] : ' ';
    if (j > len) v->len[e] = (int)j;
    else if (i - 1 > len) v->len[e] = (int)j;
    return val;
}

SV rt_cat(SV a, SV b)
{
    unsigned char *p = alloc(a.len + b.len);
    memcpy(p, a.d, (size_t)a.len);
    memcpy(p + a.len, b.d, (size_t)b.len);
    SV s;
    s.len = a.len + b.len;
    s.d = p;
    return s;
}

SV rt_cap(SV a)
{
    unsigned char *p = alloc(a.len);
    for (int i = 0; i < a.len; i++) p[i] = (unsigned char)toupper(a.d[i]);
    SV s;
    s.len = a.len;
    s.d = p;
    return s;
}

SV rt_fti(double x)
{
    long k = rt_round(x);
    if (k < -32768 || k > 32767) rt_fail("fti out of range", rt_prog, rt_line);
    unsigned char b[2];
    b[0] = (unsigned char)((k >> 8) & 0xff);
    b[1] = (unsigned char)(k & 0xff);
    return mk(b, 2);
}

double rt_itf(SV a)
{
    if (a.len < 2) rt_fail("itf of a string shorter than 2", rt_prog, rt_line);
    int k = (a.d[0] << 8) | a.d[1];
    if (k >= 32768) k -= 65536;
    return k;
}

double rt_len(SV a) { return a.len; }

double rt_pos(SV a, SV b)
{
    if (b.len == 0) return 0;
    for (int i = 0; i + b.len <= a.len; i++)
        if (memcmp(a.d + i, b.d, (size_t)b.len) == 0) return i + 1;
    return 0;
}

int rt_scmp(SV a, SV b)
{
    int n = a.len < b.len ? a.len : b.len;
    int r = memcmp(a.d, b.d, (size_t)n);
    if (r) return r < 0 ? -1 : 1;
    return a.len < b.len ? -1 : a.len > b.len;
}

/* ======================================================================== */
/* output and input                                                          */
/* ======================================================================== */
void rt_out(SV s)
{
    fwrite(s.d, 1, (size_t)s.len, stdout);
    fputc('\n', stdout);
}

void rt_out_blank(void) { fputc('\n', stdout); }

void rt_dsp(SV s) { rt_out(s); }

/* one line from stdin, without its end; returns -1 at end of input */
static int getline_in(char *buf, int size)
{
    fflush(stdout);
    if (!fgets(buf, size, stdin)) return -1;
    int n = (int)strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    return n;
}

static int prompt_is(SV p, const char *s) { return p.len == (int)strlen(s) && memcmp(p.d, s, (size_t)p.len) == 0; }

static void show_prompt(SV p)
{
    if (!rt_interactive) return;
    fwrite(p.d, 1, (size_t)p.len, stdout);
    fputc(' ', stdout);
}

static void end_of_input(void)
{
    opt_pause = 0;
    rt_stop();
}

void rt_ent_str(SV prompt, HSVar *v, int nsub, double a, double b, double c)
{
    char buf[512];
    show_prompt(prompt);
    int n = getline_in(buf, sizeof buf);
    if (n < 0) end_of_input();
    if (n == 0) {
        flg13 = 1;
        return;
    }
    flg13 = 0;
    rt_sset(v, nsub, a, b, c, mk((const unsigned char *)buf, n));
}

void rt_ent_num(SV prompt, double *x)
{
    char buf[512];
    /* ADVENT's set-up questions: the port's console is the output device */
    if (prompt_is(prompt, "Enter output selectcode")) {
        *x = 11;
        flg13 = 0;
        return;
    }
    if (prompt_is(prompt, "What is your printer's width?")) {
        *x = opt_width;
        flg13 = 0;
        return;
    }
    for (;;) {
        show_prompt(prompt);
        int n = getline_in(buf, sizeof buf);
        if (n < 0) end_of_input();
        if (n == 0) {
            flg13 = 1;
            return;
        }
        char *end;
        double d = strtod(buf, &end);
        while (*end == ' ') end++;
        if (end != buf && !*end) {
            flg13 = 0;
            *x = d;
            return;
        }
    }
}

/* echo of the player's command (the game prints it on its output device;
   a console has already shown it) */
int rt_echo(void) { return opt_echo < 0 ? !rt_interactive : opt_echo; }

/* The longest line in the data files (SEC5H) is 71 characters. */
#define DATA_LINE_MAX 71
int rt_reflow(void) { return opt_reflow || !opt_fixes || opt_width < DATA_LINE_MAX; }

/* ---- buffer "M" and its format statements ---------------------------------- */
static const FmtItem *fmts[10];
static const FmtItem *wf;
static int wi;
static unsigned char *bq;
static size_t blen, bhead, bcap;

static void bput(const void *p, size_t n)
{
    if (blen + n > bcap) {
        bcap = (blen + n) * 2 + 256;
        bq = realloc(bq, bcap);
    }
    memcpy(bq + blen, p, n);
    blen += n;
}

void rt_fmt(int n, const FmtItem *items) { fmts[n] = items; }

static void fmt_literals(void)
{
    while (wf[wi].kind == 's' || wf[wi].kind == '/') {
        if (wf[wi].kind == 's') bput(wf[wi].s, strlen(wf[wi].s));
        else bput("\r\n", 2);
        wi++;
    }
}

void rt_wrt_begin(int fmt)
{
    wf = fmts[fmt];
    if (!wf) rt_fail("format not defined", rt_prog, rt_line);
    wi = 0;
    fmt_literals();
}

void rt_wrt_num(double x)
{
    char b[64];
    if (wf[wi].kind != 'f') rt_fail("number for a non-numeric format item", rt_prog, rt_line);
    snprintf(b, sizeof b, "%.0f", x);
    bput(b, strlen(b));
    wi++;
    fmt_literals();
}

void rt_wrt_str(SV s)
{
    if (wf[wi].kind != 'c') rt_fail("string for a non-string format item", rt_prog, rt_line);
    bput(s.d, (size_t)s.len);
    wi++;
    fmt_literals();
}

void rt_wrt_end(void) { bput("\r\n", 2); }

void rt_red(HSVar *v)
{
    size_t s = bhead, e = bhead;
    while (e < blen && bq[e] != '\n') e++;
    size_t next = e < blen ? e + 1 : e;
    size_t n = e - s;
    unsigned char tmp[512];
    size_t k = 0;
    for (size_t i = 0; i < n && k < sizeof tmp; i++)
        if (bq[s + i] != '\r') tmp[k++] = bq[s + i];
    bhead = next;
    if (bhead >= blen) bhead = blen = 0;
    rt_sset(v, 0, 0, 0, 0, mk(tmp, (int)k));
}

double rt_rds(void) { return (double)(blen - bhead); }

/* ======================================================================== */
/* files                                                                     */
/* ======================================================================== */
typedef struct { int t; double n; unsigned char *s; int len; } SItem;   /* t: 1 number, 2 string, 4 end of file */
static SItem *sv_items;
static int sv_count, sv_cap;

typedef struct { const HFile *df; int save; int rec, item; } FileNo;
static FileNo fno[16];

static FileNo *fileno_of(double f)
{
    long k = rt_round(f);
    if (k < 0 || k > 15 || (!fno[k].df && !fno[k].save)) rt_fail("file number not assigned", rt_prog, rt_line);
    return &fno[k];
}

static void save_load(FILE *fp)
{
    char line[2048];
    sv_count = 0;
    if (!fgets(line, sizeof line, fp) || strncmp(line, "NERO0366 ADVSAV", 15) != 0) return;
    while (fgets(line, sizeof line, fp)) {
        if (sv_count == sv_cap) {
            sv_cap = sv_cap * 2 + 64;
            sv_items = realloc(sv_items, (size_t)sv_cap * sizeof(SItem));
        }
        SItem *it = &sv_items[sv_count];
        memset(it, 0, sizeof *it);
        if (line[0] == 'N') {
            it->t = 1;
            it->n = strtod(line + 2, 0);
        } else if (line[0] == 'S') {
            char *p;
            it->t = 2;
            it->len = (int)strtol(line + 2, &p, 10);
            it->s = malloc((size_t)it->len + 1);
            while (*p == ' ') p++;
            for (int i = 0; i < it->len; i++) {
                unsigned v;
                if (sscanf(p + 2 * i, "%2x", &v) != 1) v = 0;
                it->s[i] = (unsigned char)v;
            }
        } else if (line[0] == 'E') {
            it->t = 4;
        } else {
            continue;
        }
        sv_count++;
    }
}

static void make_save_dir(void)
{
    char dir[1024];
    strcpy(dir, save_path);
    char *s = strrchr(dir, '\\'), *t = strrchr(dir, '/');
    if (t > s) s = t;
    if (!s) return;
    *s = 0;
#ifdef _WIN32
    _mkdir(dir);
#else
    mkdir(dir, 0777);
#endif
}

static void save_store(void)
{
    make_save_dir();
    FILE *fp = fopen(save_path, "w");
    if (!fp) rt_fail("cannot write the save file", rt_prog, rt_line);
    fputs("NERO0366 ADVSAV\n", fp);
    for (int i = 0; i < sv_count; i++) {
        SItem *it = &sv_items[i];
        if (it->t == 1) fprintf(fp, "N %.17g\n", it->n);
        else if (it->t == 4) fputs("E\n", fp);
        else {
            fprintf(fp, "S %d ", it->len);
            for (int k = 0; k < it->len; k++) fprintf(fp, "%02x", it->s[k]);
            fputc('\n', fp);
        }
    }
    fclose(fp);
}

double rt_asgn(const char *name, double f)
{
    long k = rt_round(f);
    if (k < 0 || k > 15) rt_fail("bad file number", rt_prog, rt_line);
    memset(&fno[k], 0, sizeof fno[k]);
    if (strcmp(name, "ADVSAV") == 0) {
        FILE *fp = fopen(save_path, "r");
        if (!fp) return 1;
        save_load(fp);
        fclose(fp);
        fno[k].save = 1;
        return 0;
    }
    for (const HFile *h = hfiles; h->name; h++)
        if (strcmp(h->name, name) == 0) {
            fno[k].df = h;
            return 0;
        }
    return 1;
}

void rt_open(const char *name, double nrec)
{
    (void)nrec;
    if (strcmp(name, "ADVSAV") != 0) rt_fail("open of a file other than ADVSAV", rt_prog, rt_line);
    sv_count = 0;
    save_store();
}

void rt_rread(double f, double rec)
{
    FileNo *fp = fileno_of(f);
    long r = rt_round(rec);
    if (fp->save) {
        if (r != 1) rt_fail("record other than 1 of the save file", rt_prog, rt_line);
        fp->item = 0;
        return;
    }
    if (r < 1 || r > fp->df->nrec) rt_fail("record number out of range", rt_prog, rt_line);
    fp->rec = (int)r - 1;
    fp->item = 0;
}

/* next item to read: 1 number, 2 string, 4 end of file */
static int peek(FileNo *fp, double *n, const unsigned char **s, int *len)
{
    if (fp->save) {
        if (fp->item >= sv_count || sv_items[fp->item].t == 4) return 4;
        SItem *it = &sv_items[fp->item];
        *n = it->n;
        *s = it->s;
        *len = it->len;
        return it->t;
    }
    while (fp->rec < fp->df->nrec && fp->item >= fp->df->rec[fp->rec].nitems) {
        fp->rec++;
        fp->item = 0;
    }
    if (fp->rec >= fp->df->nrec) return 4;
    const HItem *it = &fp->df->rec[fp->rec].it[fp->item];
    *n = it->n;
    *s = (const unsigned char *)it->s;
    *len = it->len;
    return it->t;
}

double rt_type(double f)
{
    double n;
    const unsigned char *s;
    int len;
    return peek(fileno_of(f), &n, &s, &len);
}

static int take(double f, double *n, SV *sv)
{
    FileNo *fp = fileno_of(f);
    const unsigned char *s = 0;
    int len = 0;
    int t = peek(fp, n, &s, &len);
    if (t == 4) rt_fail("end of file", rt_prog, rt_line);
    fp->item++;
    if (t == 2) *sv = mk(s, len);
    return t;
}

void rt_rd_num(double f, double *x)
{
    double n;
    SV s;
    if (take(f, &n, &s) != 1) rt_fail("string read into a number", rt_prog, rt_line);
    *x = n;
}

void rt_rd_narr(double f, NArr *a)
{
    for (int i = 1; i <= a->n; i++) rt_rd_num(f, &a->d[i]);
}

void rt_rd_str(double f, HSVar *v, int nsub, double a, double b, double c)
{
    double n;
    SV s;
    if (take(f, &n, &s) != 2) rt_fail("number read into a string", rt_prog, rt_line);
    rt_sset(v, nsub, a, b, c, s);
}

void rt_rd_sarr(double f, HSVar *v)
{
    for (int i = 1; i <= v->n; i++) rt_rd_str(f, v, 1, i, 0, 0);
}

static SItem *wslot(double f)
{
    FileNo *fp = fileno_of(f);
    if (!fp->save) rt_fail("write to a data file", rt_prog, rt_line);
    if (fp->item >= sv_count) {
        if (sv_count == sv_cap) {
            sv_cap = sv_cap * 2 + 64;
            sv_items = realloc(sv_items, (size_t)sv_cap * sizeof(SItem));
        }
        memset(&sv_items[sv_count], 0, sizeof(SItem));
        sv_count++;
    }
    SItem *it = &sv_items[fp->item++];
    free(it->s);
    memset(it, 0, sizeof *it);
    return it;
}

void rt_wr_num(double f, double x)
{
    SItem *it = wslot(f);
    it->t = 1;
    it->n = x;
}

void rt_wr_str(double f, SV s)
{
    SItem *it = wslot(f);
    it->t = 2;
    it->len = s.len;
    it->s = malloc((size_t)s.len + 1);
    memcpy(it->s, s.d, (size_t)s.len);
}

void rt_wr_narr(double f, NArr *a)
{
    for (int i = 1; i <= a->n; i++) rt_wr_num(f, a->d[i]);
}

void rt_wr_sarr(double f, HSVar *v)
{
    for (int i = 1; i <= v->n; i++) rt_wr_str(f, rt_sget(v, 1, i, 0, 0));
}

void rt_wr_eof(double f) { wslot(f)->t = 4; }

void rt_wr_done(double f)
{
    (void)f;
    save_store();
}

/* ======================================================================== */
/* control and main                                                          */
/* ======================================================================== */
static jmp_buf chain_jb;
static int next_prog, next_line;

void rt_chain(int prog, int line)
{
    next_prog = prog;
    next_line = line;
    longjmp(chain_jb, 1);
}

void rt_stop(void)
{
    fflush(stdout);
    if (opt_pause && rt_interactive) {
        char buf[16];
        fputs("\n(Press Enter to close.)", stdout);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) { }
    }
    exit(0);
}

void rt_fail(const char *msg, int prog, int line)
{
    fflush(stdout);
    fprintf(stderr, "\nHPL error: %s (program %s, line %d)\n", msg, game_progs[prog], line);
    exit(2);
}

static void usage(void)
{
    puts("NERO0366 - Adventure for the HP 9825 (Eric Nelson and Craig Robinson, 1980)\n"
         "\n"
         "  advent [--no-fixes] [-w WIDTH] [--reflow] [--seed N] [--saves FILE] [--echo|--no-echo] [--no-pause]\n"
         "\n"
         "  --no-fixes  play exactly as the 9825 program does, its bugs included\n"
         "  -w WIDTH    line width (the game's \"printer's width\"; default 79)\n"
         "  --reflow    join the lines of the game's text and refill them to the width,\n"
         "              as the 9825 does (always below width 71, and with --no-fixes)\n"
         "  --seed N    seed the random numbers (default: the clock)\n"
         "  --saves F   the suspended game (default: saves\\ADVSAV next to advent.exe)\n"
         "  --echo      print each command after it is typed, as the 9825 does\n"
         "              (the default when input is not a console)\n"
         "  --no-pause  do not wait for Enter when the game ends");
    exit(0);
}

static void default_save_path(void)
{
    char exe[1024] = "";
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(0, exe, sizeof exe);
    if (n == 0 || n >= sizeof exe) exe[0] = 0;
#endif
    char *s = strrchr(exe, '\\'), *t = strrchr(exe, '/');
    if (t > s) s = t;
    if (s) s[1] = 0;
    else exe[0] = 0;
    snprintf(save_path, sizeof save_path, "%ssaves%cADVSAV", exe,
#ifdef _WIN32
             '\\'
#else
             '/'
#endif
    );
}

int main(int argc, char **argv)
{
    default_save_path();
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--no-fixes")) opt_fixes = 0;
        else if ((!strcmp(a, "-w") || !strcmp(a, "--width")) && i + 1 < argc) opt_width = atoi(argv[++i]);
        else if (!strcmp(a, "--seed") && i + 1 < argc) {
            have_seed = 1;
            opt_seed = strtoull(argv[++i], 0, 10);
        } else if (!strcmp(a, "--saves") && i + 1 < argc) snprintf(save_path, sizeof save_path, "%s", argv[++i]);
        else if (!strcmp(a, "--echo")) opt_echo = 1;
        else if (!strcmp(a, "--no-echo")) opt_echo = 0;
        else if (!strcmp(a, "--no-pause")) opt_pause = 0;
        else if (!strcmp(a, "--reflow")) opt_reflow = 1;
        else usage();
    }
    if (opt_width < 20) opt_width = 20;
    rt_interactive = isatty(fileno(stdin));
    rt_rnd(-1);                          /* a restored game never calls rnd(-pi) */
    int prog = 0, line = 0;
    for (;;) {
        if (setjmp(chain_jb) == 0) {
            rt_release(0);
            game_run(prog, line);
            rt_fail("program ran off its end", prog, line);
        }
        prog = next_prog;
        line = next_line;
    }
}
