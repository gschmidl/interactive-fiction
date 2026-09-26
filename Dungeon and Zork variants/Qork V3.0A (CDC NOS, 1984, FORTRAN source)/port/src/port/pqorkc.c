/* pqorkc.c - what Qork's COMPASS module RIO did, and the port's files.
 *
 * RIO kept the text database on TAPE2, a random file of coded lines, by
 * the NOS macros WRITER, WRITES, READ and READS:
 *
 *   OPEN(2)              start the file
 *   WRR(2,X)             end the record being written (even an empty one)
 *                        and have the next one's random address - its
 *                        first PRU - returned into X
 *   WRI(2,LINE,170)      a line of the record, one display code character
 *                        a word (WRITES drops trailing blanks)
 *   CLOSE(2)             end the last record
 *   RDR(2,B,N,ADDR,EOF)  go to the record at ADDR and read its first line
 *   RNL(2,B,N,EOF)       the next line of it
 *
 * Reading fills B with the line, one character a word, and blanks (55
 * octal) up to N; at the end of the record B is all blanks and EOF is
 * non-zero (0703 octal came back on NOS 2.8.7).  A record takes
 * words/64 + 1 PRUs of 64 words, a line of n characters (n+11)/10 words
 * (ten to a word and an end of line of at least two zero characters),
 * and the file's first record starts at PRU 1 - PRS's first WRR ends an
 * empty one there.  tests\cyber\mkprobe.py measured this on the machine.
 *
 * --build writes the records to qork.dat when PRS closes the file; a
 * game reads them back at its first RDR.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <direct.h>
#include <windows.h>

typedef struct { int n; unsigned char *c; } Line;
typedef struct { long pru; int nlines, cap; Line *lines; } Record;

static Record *recs;
static int nrec, caprec;
static Record cur;
static long nextpru = 1;
static int loaded, writing;
static int rrec = -1, rline;

#define BLANK 45
#define EOR_STATUS 0703

static void fail(const char *msg)
{
    fprintf(stderr, "qork: %s\n", msg);
    exit(1);
}

/* PINIT: into the program's directory, and a saves\ there */
void pinit_(void)
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, sizeof path);
    if (n > 0 && n < sizeof path) {
        char *p = strrchr(path, '\\');
        if (p) {
            *p = 0;
            if (_chdir(path) != 0) fail("cannot go to the program's directory");
        }
    }
    _mkdir("saves");
}

void pcopyf_(const char *from, const char *to, size_t lf, size_t lt)
{
    char a[260], b[260], buf[8192];
    size_t k;
    FILE *f, *g;
    snprintf(a, sizeof a, "%.*s", (int)lf, from);
    snprintf(b, sizeof b, "%.*s", (int)lt, to);
    for (k = strlen(a); k > 0 && a[k - 1] == ' '; k--) a[k - 1] = 0;
    for (k = strlen(b); k > 0 && b[k - 1] == ' '; k--) b[k - 1] = 0;
    f = fopen(a, "rb");
    if (!f) fail("cannot read qork.ini");
    g = fopen(b, "wb");
    if (!g) fail("cannot write saves\\QORK.SAV");
    while ((k = fread(buf, 1, sizeof buf, f)) > 0) fwrite(buf, 1, k, g);
    fclose(f);
    fclose(g);
}

/* ------------------------------------------------------------ writing */
static long words_of(const Record *r)
{
    long w = 0;
    int i;
    for (i = 0; i < r->nlines; i++) w += (r->lines[i].n + 11) / 10;
    return w;
}

static void end_record(void)
{
    if (nrec == caprec) {
        caprec = caprec ? 2 * caprec : 256;
        recs = realloc(recs, caprec * sizeof *recs);
        if (!recs) fail("out of memory");
    }
    cur.pru = nextpru;
    recs[nrec++] = cur;
    nextpru += words_of(&cur) / 64 + 1;
    memset(&cur, 0, sizeof cur);
}

void prioop_(int64_t *unit)
{
    (void)unit;
    nrec = 0;
    nextpru = 1;
    memset(&cur, 0, sizeof cur);
    writing = 1;
    loaded = 1;
}

void priowr_(int64_t *unit, int64_t *addr)
{
    (void)unit;
    end_record();
    *addr = nextpru;
}

void priowi_(int64_t *unit, int64_t *line, int64_t *len)
{
    int n = *len > 0 && *len < 1000 ? (int)*len : 0, i;
    Line l;
    (void)unit;
    while (n > 0 && (line[n - 1] & 63) == BLANK) n--;
    l.n = n;
    l.c = malloc(n ? n : 1);
    if (!l.c) fail("out of memory");
    for (i = 0; i < n; i++) l.c[i] = (unsigned char)(line[i] & 63);
    if (cur.nlines == cur.cap) {
        cur.cap = cur.cap ? 2 * cur.cap : 8;
        cur.lines = realloc(cur.lines, cur.cap * sizeof *cur.lines);
        if (!cur.lines) fail("out of memory");
    }
    cur.lines[cur.nlines++] = l;
}

static void put32(FILE *f, long v)
{
    unsigned char b[4];
    b[0] = v & 255; b[1] = (v >> 8) & 255; b[2] = (v >> 16) & 255; b[3] = (v >> 24) & 255;
    fwrite(b, 1, 4, f);
}

void priocl_(int64_t *unit)
{
    FILE *f;
    int i, j;
    (void)unit;
    end_record();
    writing = 0;
    f = fopen("qork.dat", "wb");
    if (!f) fail("cannot write qork.dat");
    fwrite("QORKDAT1", 1, 8, f);
    put32(f, nrec);
    for (i = 0; i < nrec; i++) {
        put32(f, recs[i].pru);
        put32(f, recs[i].nlines);
        for (j = 0; j < recs[i].nlines; j++) {
            put32(f, recs[i].lines[j].n);
            fwrite(recs[i].lines[j].c, 1, recs[i].lines[j].n, f);
        }
    }
    if (fclose(f) != 0) fail("cannot write qork.dat");
}

/* ------------------------------------------------------------ reading */
static long get32(FILE *f)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) fail("qork.dat is cut short");
    return b[0] | (b[1] << 8) | ((long)b[2] << 16) | ((long)b[3] << 24);
}

static void load(void)
{
    FILE *f = fopen("qork.dat", "rb");
    char magic[8];
    int i, j;
    if (!f) fail("qork.dat is missing (build.sh makes it, with qork --build)");
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "QORKDAT1", 8) != 0)
        fail("qork.dat is not Qork's text file");
    nrec = caprec = (int)get32(f);
    if (nrec < 0 || nrec > 100000) fail("qork.dat is not Qork's text file");
    recs = calloc(nrec ? nrec : 1, sizeof *recs);
    if (!recs) fail("out of memory");
    for (i = 0; i < nrec; i++) {
        recs[i].pru = get32(f);
        recs[i].nlines = recs[i].cap = (int)get32(f);
        if (recs[i].nlines < 0 || recs[i].nlines > 100000)
            fail("qork.dat is not Qork's text file");
        recs[i].lines = calloc(recs[i].nlines ? recs[i].nlines : 1, sizeof(Line));
        if (!recs[i].lines) fail("out of memory");
        for (j = 0; j < recs[i].nlines; j++) {
            Line *l = &recs[i].lines[j];
            l->n = (int)get32(f);
            if (l->n < 0 || l->n > 1000) fail("qork.dat is not Qork's text file");
            l->c = malloc(l->n ? l->n : 1);
            if (!l->c || fread(l->c, 1, l->n, f) != (size_t)l->n)
                fail("qork.dat is cut short");
        }
    }
    fclose(f);
    loaded = 1;
}

static void reads(int64_t *buf, int64_t *len, int64_t *eof)
{
    int n = (int)*len, i, k = 0;
    const Line *l = NULL;
    if (rrec >= 0 && rline < recs[rrec].nlines) l = &recs[rrec].lines[rline++];
    if (l) {
        k = l->n < n ? l->n : n;
        for (i = 0; i < k; i++) buf[i] = l->c[i];
    }
    for (i = k; i < n; i++) buf[i] = BLANK;
    *eof = l ? 0 : EOR_STATUS;
}

void priord_(int64_t *unit, int64_t *buf, int64_t *len, int64_t *addr, int64_t *eof)
{
    int lo = 0, hi, i;
    (void)unit;
    if (!loaded) load();
    rrec = -1;
    hi = nrec - 1;
    while (lo <= hi) {                  /* the records are in PRU order */
        i = (lo + hi) / 2;
        if (recs[i].pru == *addr) { rrec = i; break; }
        if (recs[i].pru < *addr) lo = i + 1; else hi = i - 1;
    }
    rline = 0;
    reads(buf, len, eof);
}

void priorn_(int64_t *unit, int64_t *buf, int64_t *len, int64_t *eof)
{
    (void)unit;
    reads(buf, len, eof);
}
