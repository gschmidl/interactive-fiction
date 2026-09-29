/*
 * pcpvc.c - the C half of the CP-V Adventure port's run-time: the
 * terminal, the clock, CP-V keyed files, and the EBCDIC collating order.
 *
 * Text inside the port is ASCII plus five control bytes that stand for the
 * EBCDIC control codes the cave's text uses (see src/cpvtape.py):
 *   07 bell, 08 backspace, 0D carriage return (CP-V's COC printed CR LF),
 *   15 NAK, 0A "line feed only" (EBCDIC 20: down a line, no return).
 *
 * Keyed files stand in for CP-V's: records keyed by a byte string (a
 * 3-byte EDIT line number such as 4005.000 = 4005000, or a 12-character
 * save key), kept sorted by key.  On disk: "CPVKEYED", a record count, then
 * per record: key length, key, data length (big-endian halfword), data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

/* ------------------------------------------------------------------ */
/* terminal                                                            */

static HANDLE hout = INVALID_HANDLE_VALUE;
static int outcon = 0;          /* stdout is a console */
static int incon = 0;           /* stdin is a console */

static void kcloseall(void);

void pioini_(void)
{
    DWORD mode;
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    /* CP-V closed (and kept) a program's open files when it ended: the
       munger, listing off, STOPs without closing ADVT and ADVI */
    atexit(kcloseall);
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(hout, &mode)) {
        outcon = 1;
        /* LF moves down without returning, as "line feed only" did */
        SetConsoleMode(hout, mode | 0x0004 /* VT processing */
                             | 0x0008 /* DISABLE_NEWLINE_AUTO_RETURN */);
        SetConsoleOutputCP(1252);
    }
    if (GetConsoleMode(hin, &mode))
        incon = 1;
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stdin), _O_BINARY);
}

/* write n raw bytes */
void ptwrite_(const char *s, const int *n, int slen)
{
    (void)slen;
    if (*n > 0) fwrite(s, 1, (size_t)*n, stdout);
}

void ptflush_(void)
{
    fflush(stdout);
}

/*
 * Read a line from the terminal into buf (blank-filled to buflen).
 * Returns the number of characters, or -1 at end of file.  Bytes are
 * masked to 7 bits, a tab becomes a blank, other control bytes go.
 */
int ptread_(char *buf, int buflen)
{
    int c, n = 0;
    fflush(stdout);
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
        if (n < buflen) buf[n] = (char)c;
        n++;
    }
    if (!incon) {
        /* piped input: echo it, as the terminal would have */
        int k = n < buflen ? n : buflen;
        fwrite(buf, 1, (size_t)k, stdout);
        fputs("\r\n", stdout);
    }
    return n < buflen ? n : buflen;
}

int ptread0_(char *buf, int *len, int buflen)
{
    int n = ptread_(buf, buflen);
    *len = n;
    return n;
}

/* ------------------------------------------------------------------ */
/* clock                                                               */

static int frozen = 0;          /* 1 = time held, 2 = date held */
static struct tm heldtm;
static int heldms = 0;
static int dateset = 0, dy, dm, dd;

/* --date: y m d; --time: h m s ms (either may be given) */
void pclkset_(const int *which, const int *a, const int *b, const int *c,
              const int *d)
{
    if (*which == 1) {          /* date */
        dateset = 1; dy = *a; dm = *b; dd = *c;
    } else {                    /* time */
        frozen = 1;
        memset(&heldtm, 0, sizeof heldtm);
        heldtm.tm_hour = *a; heldtm.tm_min = *b; heldtm.tm_sec = *c;
        heldms = *d;
    }
}

/* year - 1900, day of year (1-based), hour, minute, second, millisecond */
void pclock_(int *yr, int *doy, int *hh, int *mi, int *ss, int *ms)
{
    SYSTEMTIME st;
    struct tm t;
    GetLocalTime(&st);
    memset(&t, 0, sizeof t);
    t.tm_year = st.wYear - 1900; t.tm_mon = st.wMonth - 1;
    t.tm_mday = st.wDay; t.tm_hour = st.wHour; t.tm_min = st.wMinute;
    t.tm_sec = st.wSecond;
    *ms = st.wMilliseconds;
    if (dateset) {
        t.tm_year = dy - 1900; t.tm_mon = dm - 1; t.tm_mday = dd;
    }
    if (frozen) {
        t.tm_hour = heldtm.tm_hour; t.tm_min = heldtm.tm_min;
        t.tm_sec = heldtm.tm_sec; *ms = heldms;
    }
    t.tm_isdst = -1;
    {
        struct tm u = t;
        u.tm_hour = 12;         /* normalise the date only */
        mktime(&u);
        *doy = u.tm_yday + 1;
        *yr = u.tm_year;
    }
    *hh = t.tm_hour; *mi = t.tm_min; *ss = t.tm_sec;
}

/* ------------------------------------------------------------------ */
/* the directory the program lives in                                  */

void pgmdir_(char *buf, int *n, int buflen)
{
    char p[MAX_PATH];
    DWORD k = GetModuleFileNameA(NULL, p, MAX_PATH);
    char *s = strrchr(p, '\\');
    if (s) *s = 0; else strcpy(p, ".");
    (void)k;
    memset(buf, ' ', (size_t)buflen);
    *n = (int)strlen(p);
    if (*n > buflen) *n = buflen;
    memcpy(buf, p, (size_t)*n);
}

int pmkdir_(const char *path, int plen)
{
    char p[MAX_PATH];
    int n = plen;
    while (n > 0 && path[n - 1] == ' ') n--;
    if (n >= MAX_PATH) n = MAX_PATH - 1;
    memcpy(p, path, (size_t)n); p[n] = 0;
    return CreateDirectoryA(p, NULL) ? 0 : 1;
}

/* ------------------------------------------------------------------ */
/* EBCDIC collating order                                              */

/* EBCDIC code of each internal byte (ASCII + the controls above) */
static const unsigned char EBC[128] = {
      0,  0,  0,  0,  0,  0,  0,  7,  8,  0, 32,  0,  0, 13,  0,  0,
      0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     64, 90,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97,
    240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111,
    124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
    215,216,217,226,227,228,229,230,231,232,233,180,177,181,176,109,
    121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
    151,152,153,162,163,164,165,166,167,168,169,178, 79,179,161,  0
};

int pebcdc_(const char *c, int clen)
{
    (void)clen;
    return EBC[(unsigned char)*c & 0x7F];
}

/* Fortran character comparison in EBCDIC order: -1, 0, 1 */
int pecmp_(const char *a, const char *b, int la, int lb)
{
    int n = la > lb ? la : lb, i;
    for (i = 0; i < n; i++) {
        int x = EBC[(unsigned char)(i < la ? a[i] : ' ') & 0x7F];
        int y = EBC[(unsigned char)(i < lb ? b[i] : ' ') & 0x7F];
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* keyed files                                                         */

typedef struct {
    unsigned char klen;
    unsigned char key[16];
    int len;
    unsigned char *data;
} krec;

typedef struct {
    int unit, open, mode, dirty, cursor, err;
    char path[MAX_PATH];
    krec *r;
    int n, cap;
} kfile;

#define NKF 16
static kfile kf[NKF];

static kfile *kfind(int unit)
{
    int i;
    for (i = 0; i < NKF; i++)
        if (kf[i].open && kf[i].unit == unit) return &kf[i];
    return NULL;
}

static int kcmp(const unsigned char *a, int la, const unsigned char *b, int lb)
{
    int n = la < lb ? la : lb;
    int c = memcmp(a, b, (size_t)n);
    if (c) return c;
    return la - lb;
}

/* index of key, or -(insertion point)-1 */
static int ksearch(kfile *f, const unsigned char *key, int kl)
{
    int lo = 0, hi = f->n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = kcmp(f->r[mid].key, f->r[mid].klen, key, kl);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1; else hi = mid - 1;
    }
    return -lo - 1;
}

static void kput(kfile *f, const unsigned char *key, int kl,
                 const unsigned char *data, int len)
{
    int i = ksearch(f, key, kl);
    krec *r;
    if (i >= 0) {
        r = &f->r[i];
        free(r->data);
    } else {
        i = -i - 1;
        if (f->n == f->cap) {
            f->cap = f->cap ? f->cap * 2 : 256;
            f->r = (krec *)realloc(f->r, sizeof(krec) * (size_t)f->cap);
        }
        memmove(&f->r[i + 1], &f->r[i], sizeof(krec) * (size_t)(f->n - i));
        f->n++;
        r = &f->r[i];
        r->klen = (unsigned char)kl;
        memcpy(r->key, key, (size_t)kl);
    }
    r->len = len;
    r->data = (unsigned char *)malloc((size_t)(len ? len : 1));
    memcpy(r->data, data, (size_t)len);
    f->dirty = 1;
}

static void cpath(char *out, const char *s, int slen)
{
    int n = slen;
    while (n > 0 && s[n - 1] == ' ') n--;
    if (n >= MAX_PATH) n = MAX_PATH - 1;
    memcpy(out, s, (size_t)n);
    out[n] = 0;
}

/* mode 1 input, 2 output (new), 3 update.  ierr = X'0300' if missing. */
void kopen_(const int *unit, const char *path, const int *mode, int *ierr,
            int plen)
{
    kfile *f = kfind(*unit);
    FILE *fp;
    int i;
    *ierr = 0;
    if (f) {                    /* reopen: close first */
        extern void kclose_(const int *);
        kclose_(unit);
    }
    for (i = 0; i < NKF; i++) if (!kf[i].open) break;
    if (i == NKF) { *ierr = 0x0700; return; }
    f = &kf[i];
    memset(f, 0, sizeof *f);
    f->unit = *unit;
    f->mode = *mode;
    cpath(f->path, path, plen);
    if (*mode == 2) {
        f->open = 1;
        f->dirty = 1;
        return;
    }
    fp = fopen(f->path, "rb");
    if (!fp) { *ierr = 0x0300; f->err = 0x0300; return; }
    {
        char magic[8];
        unsigned char hdr[4];
        int cnt, k;
        if (fread(magic, 1, 8, fp) != 8 || memcmp(magic, "CPVKEYED", 8) ||
            fread(hdr, 1, 4, fp) != 4) {
            fclose(fp); *ierr = 0x0400; return;
        }
        cnt = (hdr[0] << 24) | (hdr[1] << 16) | (hdr[2] << 8) | hdr[3];
        f->cap = cnt > 0 ? cnt : 16;
        f->r = (krec *)calloc((size_t)f->cap, sizeof(krec));
        for (k = 0; k < cnt; k++) {
            krec *r = &f->r[k];
            unsigned char l2[2];
            if (fread(&r->klen, 1, 1, fp) != 1 || r->klen > 16 ||
                fread(r->key, 1, r->klen, fp) != r->klen ||
                fread(l2, 1, 2, fp) != 2) break;
            r->len = (l2[0] << 8) | l2[1];
            r->data = (unsigned char *)malloc((size_t)(r->len ? r->len : 1));
            if (fread(r->data, 1, (size_t)r->len, fp) != (size_t)r->len) break;
            f->n = k + 1;
        }
        fclose(fp);
        if (f->n != cnt) { *ierr = 0x0400; free(f->r); return; }
    }
    f->open = 1;
}

void kclose_(const int *unit)
{
    kfile *f = kfind(*unit);
    int i;
    if (!f) return;
    if (f->dirty && f->mode != 1) {
        FILE *fp = fopen(f->path, "wb");
        if (fp) {
            unsigned char hdr[4];
            hdr[0] = (unsigned char)(f->n >> 24); hdr[1] = (unsigned char)(f->n >> 16);
            hdr[2] = (unsigned char)(f->n >> 8); hdr[3] = (unsigned char)f->n;
            fwrite("CPVKEYED", 1, 8, fp);
            fwrite(hdr, 1, 4, fp);
            for (i = 0; i < f->n; i++) {
                krec *r = &f->r[i];
                unsigned char l2[2];
                l2[0] = (unsigned char)(r->len >> 8); l2[1] = (unsigned char)r->len;
                fwrite(&r->klen, 1, 1, fp);
                fwrite(r->key, 1, r->klen, fp);
                fwrite(l2, 1, 2, fp);
                fwrite(r->data, 1, (size_t)r->len, fp);
            }
            fclose(fp);
        }
    }
    for (i = 0; i < f->n; i++) free(f->r[i].data);
    free(f->r);
    memset(f, 0, sizeof *f);
}

static void kcloseall(void)
{
    int i;
    for (i = 0; i < NKF; i++)
        if (kf[i].open) kclose_(&kf[i].unit);
    fflush(stdout);
}

int kerr_(const int *unit)
{
    int i;
    for (i = 0; i < NKF; i++)
        if (kf[i].unit == *unit) return kf[i].err;
    return 0x0300;
}

/* rewind / next record in key order (for reading source files) */
void krew_(const int *unit)
{
    kfile *f = kfind(*unit);
    if (f) f->cursor = 0;
}

/* next record: data into buf (len bytes, blank-filled), key; -1 at end */
int knext_(const int *unit, char *buf, int *len, int *key, int buflen)
{
    kfile *f = kfind(*unit);
    krec *r;
    int k, n;
    memset(buf, ' ', (size_t)buflen);
    if (!f || f->cursor >= f->n) return -1;
    r = &f->r[f->cursor++];
    n = r->len < buflen ? r->len : buflen;
    memcpy(buf, r->data, (size_t)n);
    *len = n;
    for (k = 0, *key = 0; k < r->klen; k++) *key = (*key << 8) | r->key[k];
    return 0;
}

/* the record buffer: packed before a write, unpacked after a read */
static unsigned char pb[65536];
static int pbn = 0, pbpos = 0;

void kpbeg_(void) { pbn = 0; pbpos = 0; }

void kpput_(const int *v)       /* R2 */
{
    pb[pbn++] = (unsigned char)(*v >> 8);
    pb[pbn++] = (unsigned char)*v;
}

void kpput4_(const int *v)      /* R4 */
{
    pb[pbn++] = (unsigned char)(*v >> 24);
    pb[pbn++] = (unsigned char)(*v >> 16);
    pb[pbn++] = (unsigned char)(*v >> 8);
    pb[pbn++] = (unsigned char)*v;
}

void kpputc_(const char *s, const int *n, int slen)   /* An */
{
    int i;
    for (i = 0; i < *n; i++) pb[pbn++] = (unsigned char)(i < slen ? s[i] : ' ');
}

static void ikey(unsigned char *k, int v)
{
    k[0] = (unsigned char)(v >> 16);
    k[1] = (unsigned char)(v >> 8);
    k[2] = (unsigned char)v;
}

void kpwri_(const int *unit, const int *key)
{
    kfile *f = kfind(*unit);
    unsigned char k[3];
    if (!f) return;
    ikey(k, *key);
    kput(f, k, 3, pb, pbn);
}

void kpwrc_(const int *unit, const char *key, int klen)
{
    kfile *f = kfind(*unit);
    if (!f) return;
    kput(f, (const unsigned char *)key, klen > 16 ? 16 : klen, pb, pbn);
}

static int kload(kfile *f, const unsigned char *k, int kl)
{
    int i;
    pbn = 0; pbpos = 0;
    if (!f) return 0;
    i = ksearch(f, k, kl);
    if (i < 0) return 0;
    memcpy(pb, f->r[i].data, (size_t)f->r[i].len);
    pbn = f->r[i].len;
    return 1;
}

void kprdi_(const int *unit, const int *key, int *found)
{
    unsigned char k[3];
    ikey(k, *key);
    *found = kload(kfind(*unit), k, 3);
}

void kprdc_(const int *unit, const char *key, int *found, int klen)
{
    *found = kload(kfind(*unit), (const unsigned char *)key, klen > 16 ? 16 : klen);
}

int kplen_(void) { return pbn; }

/* past the end of a record FORTRAN supplied blanks: X'4040' as R2 */
void kpget_(int *v)
{
    int hi = pbpos < pbn ? pb[pbpos] : 0x40;
    int lo = pbpos + 1 < pbn ? pb[pbpos + 1] : 0x40;
    pbpos += 2;
    *v = (hi << 8) | lo;
}

void kpget4_(int *v)
{
    int i, x = 0;
    for (i = 0; i < 4; i++, pbpos++)
        x = (x << 8) | (pbpos < pbn ? pb[pbpos] : 0x40);
    *v = x;
}

void kpgetc_(char *s, const int *n, int slen)
{
    int i;
    for (i = 0; i < *n; i++, pbpos++)
        if (i < slen) s[i] = (char)(pbpos < pbn ? pb[pbpos] : ' ');
    for (; i < slen; i++) s[i] = ' ';
}

void kdelc_(const int *unit, const char *key, int klen)
{
    kfile *f = kfind(*unit);
    int i;
    if (!f) return;
    i = ksearch(f, (const unsigned char *)key, klen > 16 ? 16 : klen);
    if (i < 0) return;
    free(f->r[i].data);
    memmove(&f->r[i], &f->r[i + 1], sizeof(krec) * (size_t)(f->n - i - 1));
    f->n--;
    f->dirty = 1;
}
