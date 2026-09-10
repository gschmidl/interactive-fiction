/* The slice of the WAITS monitor that FisK actually asks for.
 *
 * Skip conventions, read off the SAIL runtime in the image itself:
 *   OPEN / LOOKUP / ENTER / RENAME   skip on SUCCESS
 *   IN   / OUT                       skip on ERROR or END OF FILE
 *   INPUT / OUTPUT / CLOSE / RELEAS  never skip
 *
 * The runtime owns its buffer ring - the three-word header IS part of its own
 * per-channel table (26(2)/27(2)/30(2)), so the monitor only has to move data
 * in and out of the buffer the header points at and leave a fresh byte
 * pointer and byte count behind.  The runtime resets that pointer with P=0
 * and an address of BUF+1, so the first ILDB/IDPB lands on BUF+2; we build
 * ours the same way.
 *
 * Disk files live in the current directory as NAME.EXT, so LOG, PHOTO and
 * RUNFILE work the way they did on WAITS.  DSK:FISK.TXT falls back to the
 * copy carried inside the image if there is no file of that name.
 */
#include "fisk.h"
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
static void msleep1(void) { Sleep(1); }
#else
#include <unistd.h>
static void msleep1(void) { usleep(1000); }
#endif

extern int pc;
#define UPC ((pc - 1) & 0777777)

int exit_code = 0;
int exiting   = 0;
int montrace  = 0;

/* --------------------------------------------------------------- terminal */

/* A WAITS terminal is a real terminal, and FisK uses it as one: the envelope
   on the letter draws its left border by returning the carriage and
   overprinting the line it has just typed.  That is the only bare CR in the
   whole message database, but dropping it turns the envelope into a mess, so
   output is assembled a line at a time instead of being streamed straight
   out. */
static char oline[1024];
static int  ocol, olen, oemit;

static void oflush_partial(void)
{
    if (olen > oemit) {
        fwrite(oline + oemit, 1, (size_t)(olen - oemit), stdout);
        oemit = olen;
    }
}

static void oseek(int col)              /* about to write at this column */
{
    if (col < oemit) {                  /* already on screen - retype it */
        fputc('\r', stdout);
        oemit = 0;
    }
}

void tty_out(int c)
{
    c &= 0177;
    if (c == 0 || c == 0177) return;
    if (c == '\r') { ocol = 0; return; }
    if (c == '\n' || c == 014) {
        oflush_partial();
        fputc('\n', stdout);
        ocol = olen = oemit = 0;
        return;
    }
    if (c == '\t') {
        int stop = (ocol + 8) & ~7;
        oseek(ocol);
        while (ocol < stop && ocol < (int)sizeof oline - 1) {
            if (ocol >= olen) { oline[ocol] = ' '; olen = ocol + 1; }
            ocol++;
        }
        return;
    }
    oseek(ocol);
    if (ocol < (int)sizeof oline - 1) {
        oline[ocol++] = (char)c;
        if (ocol > olen) olen = ocol;
    }
}

void tty_flush(void) { oflush_partial(); fflush(stdout); }

static char inbuf[512];
static int  inlen, inpos;

static int refill(void)
{
    int c, n = 0;
    tty_peek();                             /* show the prompt first         */
    for (;;) {
        c = fgetc(stdin);
        if (c < 0) {
            if (n == 0) { exiting = 1; exit_code = 0; return 0; }
            break;
        }
        if (c == '\n') break;
        if (c == '\r') continue;
        if (n < (int)sizeof inbuf - 3) inbuf[n++] = (char)c;
    }
    inbuf[n++] = '\r';
    inbuf[n++] = '\n';
    inlen = n; inpos = 0;
    return 1;
}

int tty_in(int wait)
{
    if (inpos >= inlen) {
        if (!wait) return -1;
        if (!refill()) return 0;
    }
    return (unsigned char)inbuf[inpos++];
}

/* ------------------------------------------------------------ i/o channels */

#define NCHAN     16
#define BUFWORDS  0200            /* data words in one buffer                */
#define BUFSPAN   0202            /* ... plus link word and count word       */

#define IO_ERR  0740000
#define IO_BKT  0100000
#define IO_EOF  0020000
#define IO_ACT  0010000

typedef struct {
    int    inuse, open;
    word   dev;
    char   devname[8];
    char   file[32];
    int    mode;
    int    istty, isdsk;
    int    hin, hout;
    word   status;
    unsigned char *data;         /* file being read                          */
    int    len, pos, owned;
    FILE  *wf;                   /* file being written                       */
    int    shown;                /* bytes of this buffer already echoed      */
} Chan;

static Chan chan[NCHAN];

static const char *sixbit_str(word w, char *buf)
{
    int i;
    for (i = 0; i < 6; i++) {
        int c = (int)((w >> (30 - 6 * i)) & 077);
        buf[i] = (char)(c + 040);
    }
    buf[6] = 0;
    while (i > 0 && buf[i - 1] == ' ') buf[--i] = 0;
    return buf;
}

static void mtrace(const char *fmt, ...)
{
    va_list ap;
    if (!montrace) return;
    tty_flush();
    va_start(ap, fmt);
    fprintf(stderr, "[%06o ", UPC);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "]\n");
    va_end(ap);
}

void mon_init(void) { memset(chan, 0, sizeof chan); }
void mon_exit(int code) { exiting = 1; exit_code = code; }

/* ------------------------------------------------------------ byte helpers */

static int bp_size(word bp) { int s = (int)((bp >> 24) & 077); return s ? s : 7; }
static int bp_pos (word bp) { return (int)((bp >> 30) & 077); }
static int bp_adr (word bp) { return (int)(bp & RMASK); }

static word make_bp(int siz, int adr)
{
    return (((word)siz & 077) << 24) | ((word)adr & RMASK);
}

/* how many bytes have been deposited in the buffer that BP walks over */
static int bytes_used(word bp, int buf)
{
    int siz = bp_size(bp), pos = bp_pos(bp), adr = bp_adr(bp);
    int per = 36 / siz, idx;
    if (adr <= buf + 1) return 0;
    idx = (36 - pos) / siz;                     /* bytes taken in last word  */
    if (idx > per) idx = per;
    return (adr - (buf + 2)) * per + idx;
}

/* Show whatever the program has deposited in a terminal buffer but not yet
   handed to the monitor with OUT.  On a real system the prompt is on the
   screen before the program blocks for a line; SAIL leaves it sitting in the
   ring, so the monitor has to peek. */
void tty_peek(void)
{
    int i;
    for (i = 0; i < NCHAN; i++) {
        Chan *c = &chan[i];
        int hdr = c->hout, buf, siz, per, nb, j;
        if (!c->inuse || !c->istty || !hdr) continue;
        buf = (int)(mem[hdr] & RMASK);
        if (!buf) continue;
        siz = bp_size(mem[hdr + 1]);
        per = 36 / siz;
        nb  = bytes_used(mem[hdr + 1], buf);
        for (j = c->shown; j < nb; j++) {
            word w = mem[buf + 2 + j / per];
            int  k = j % per;
            tty_out((int)((w >> (36 - siz * (k + 1))) & 0177));
        }
        if (nb > c->shown) c->shown = nb;
    }
    tty_flush();
}

/* ------------------------------------------------------------ buffer rings */

/* The game re-opens its scratch channel once a move, and INBUF would carve a
   fresh ring out of .JBFF every time.  Hand back the ring we already built
   for that header instead, or a long game eats all of core. */
static struct { int hdr, first, nbuf; } rings[32];
static int nrings;

static void setup_ring(int hdr, int nbuf, int siz)
{
    int first, i, b;
    if (!hdr) return;
    if (nbuf < 2) nbuf = 2;
    for (i = 0; i < nrings; i++)
        if (rings[i].hdr == hdr && rings[i].nbuf >= nbuf) {
            first = rings[i].first;
            goto reuse;
        }
    first = b = (int)(mem[0121] & RMASK);
    for (i = 0; i < nbuf; i++) {
        int next = b + BUFSPAN;
        mem[b]     = (word)((i == nbuf - 1) ? first : next) & RMASK;
        mem[b + 1] = 0;
        b = next;
    }
    mem[0121] = (mem[0121] & LMASK) | ((word)b & RMASK);
    if (nrings < (int)(sizeof rings / sizeof rings[0])) {
        rings[nrings].hdr = hdr; rings[nrings].first = first;
        rings[nrings].nbuf = nbuf; nrings++;
    }
    mtrace("ring hdr=%06o %d bufs at %06o", hdr, nbuf, first);
reuse:
    for (i = 0, b = first; i < nbuf; i++) {
        mem[b] &= RMASK;                        /* clear the use bit        */
        mem[b + 1] = 0;
        b = (int)(mem[b] & RMASK);
        if (!b) break;
    }
    mem[hdr]     = (word)first & RMASK;         /* use bit clear: not filled */
    mem[hdr + 1] = make_bp(siz, first + 1);
    mem[hdr + 2] = 0;
}

/* the buffer the next IN should work on */
static int next_buffer(int hdr)
{
    int buf = (int)(mem[hdr] & RMASK);
    if (!buf) return 0;
    if (mem[hdr] & SIGN) {                      /* current one already used  */
        int nx = (int)(mem[buf] & RMASK);
        if (nx) buf = nx;
    }
    return buf;
}

/* ------------------------------------------------------------- host files */

static void hostname(char *out, size_t n, const char *nm, const char *ex)
{
    size_t i;
    snprintf(out, n, "%s%s%s", nm, ex[0] ? "." : "", ex);
    for (i = 0; out[i]; i++) out[i] = (char)tolower((unsigned char)out[i]);
}

/* Slurp a host file, giving every bare newline the CR that WAITS text has. */
static unsigned char *slurp(const char *path, int *lenp)
{
    long sz;
    size_t got, i, j;
    unsigned char *raw, *out;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    raw = (unsigned char *)malloc((size_t)sz + 1);
    if (!raw) { fclose(f); return NULL; }
    got = fread(raw, 1, (size_t)sz, f);
    fclose(f);
    out = (unsigned char *)malloc(got * 2 + 4);
    if (!out) { free(raw); return NULL; }
    for (i = j = 0; i < got; i++) {
        if (raw[i] == '\n' && (i == 0 || raw[i - 1] != '\r')) out[j++] = '\r';
        out[j++] = raw[i];
    }
    free(raw);
    *lenp = (int)j;
    return out;
}

static void chan_close(Chan *c)
{
    if (c->wf) { fclose(c->wf); c->wf = NULL; }
    if (c->owned && c->data) free(c->data);
    c->data = NULL; c->owned = 0; c->len = c->pos = 0;
}

/* ---------------------------------------------------------------- the UUOs */

#define AC   mem[ac]

static word date_word(void)
{
    time_t t = time(NULL);
    struct tm *g = localtime(&t);
    int y = g->tm_year + 1900 - 1964, m = g->tm_mon, d = g->tm_mday - 1;
    return (word)(((y * 12 + m) * 31 + d) & 07777777777ULL);
}

/* FisK pauses by spinning on MSTIME, so this has to tick in real
   milliseconds - one-second granularity would turn every dramatic pause into
   a full second of burnt CPU.  Yield the processor now and then while the
   program is doing nothing but asking the time. */
static word mstime(void)
{
    static int inited, spins;
    static word base;
    static clock_t t0;
    if (!inited) {
        time_t t = time(NULL);
        struct tm *g = localtime(&t);
        base = (word)((g->tm_hour * 3600 + g->tm_min * 60 + g->tm_sec) * 1000);
        t0 = clock();
        inited = 1;
    }
    if (++spins >= 400) { spins = 0; msleep1(); }
    return (base + (word)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC))
           & 07777777777ULL;
}

/* ---- input ------------------------------------------------------------- */

static int fill_from_file(Chan *c, int buf, int siz)
{
    int per = 36 / siz, i, n = 0;
    if (c->pos >= c->len) return 0;
    for (i = 0; i < BUFWORDS && c->pos < c->len; i++) {
        word w = 0; int k;
        for (k = 0; k < per; k++) {
            int ch = (c->pos < c->len) ? c->data[c->pos++] : 0;
            w |= ((word)(ch & ((1 << siz) - 1))) << (36 - siz * (k + 1));
        }
        mem[buf + 2 + i] = w;
        n++;
    }
    return n;
}

static int fill_from_tty(int buf, int siz)
{
    int per = 36 / siz, i, n = 0, done = 0;
    for (i = 0; i < BUFWORDS && !done; i++) {
        word w = 0; int k;
        for (k = 0; k < per; k++) {
            int ch = tty_in(1);
            if (ch <= 0) { done = 1; break; }
            w |= ((word)(ch & 0177)) << (36 - siz * (k + 1));
            if (ch == '\n') { done = 1; break; }
        }
        mem[buf + 2 + i] = w;
        n++;
    }
    return n;
}

/* returns 1 when the caller should skip (error / eof) */
static int do_in(Chan *c)
{
    int hdr = c->hin, buf, siz, n;
    if (!hdr) { c->status |= IO_EOF; return 1; }
    if (!(mem[hdr] & RMASK)) setup_ring(hdr, 2, 7);
    buf = next_buffer(hdr);
    if (!buf) { c->status |= IO_EOF; return 1; }
    siz = bp_size(mem[hdr + 1]);
    n = c->istty ? fill_from_tty(buf, siz) : fill_from_file(c, buf, siz);
    if (n <= 0) {
        c->status |= IO_EOF;
        mem[hdr] = SIGN | ((word)buf & RMASK);
        mem[hdr + 1] = make_bp(siz, buf + 1);
        mem[hdr + 2] = 0;
        mem[buf + 1] = 0;
        mtrace("in  ch eof");
        return 1;
    }
    mem[buf + 1] = (word)n;
    mem[buf] |= SIGN;
    mem[hdr] = SIGN | ((word)buf & RMASK);
    mem[hdr + 1] = make_bp(siz, buf + 1);
    mem[hdr + 2] = (word)(n * (36 / siz));
    mtrace("in  buf=%06o words=%d pos=%d/%d", buf, n, c->pos, c->len);
    return 0;
}

/* ---- output ------------------------------------------------------------ */

static int do_out(Chan *c)
{
    int hdr = c->hout, buf, siz, per, nb, i, nwords;
    if (!hdr) return 0;
    if (!(mem[hdr] & RMASK)) setup_ring(hdr, 2, 7);
    buf = (int)(mem[hdr] & RMASK);
    if (!buf) return 0;
    siz = bp_size(mem[hdr + 1]);
    per = 36 / siz;
    nb  = bytes_used(mem[hdr + 1], buf);
    nwords = (nb + per - 1) / per;
    mem[buf + 1] = (word)nwords;
    for (i = c->istty ? c->shown : 0; i < nb; i++) {
        word w = mem[buf + 2 + i / per];
        int  k = i % per;
        int ch = (int)((w >> (36 - siz * (k + 1))) & 0177);
        if (c->istty) tty_out(ch);
        else if (c->wf && ch && ch != '\r') fputc(ch, c->wf);
    }
    c->shown = 0;
    mtrace("out buf=%06o bytes=%d tty=%d", buf, nb, c->istty);
    {   /* hand the program a fresh buffer */
        int nx = (int)(mem[buf] & RMASK);
        if (!nx) nx = buf;
        mem[buf] &= ~SIGN;
        mem[hdr] = SIGN | ((word)nx & RMASK);
        mem[hdr + 1] = make_bp(siz, nx + 1);
        mem[hdr + 2] = (word)(BUFWORDS * per);
        mem[nx + 1] = 0;
    }
    return 0;
}

void mon_flush(void)
{
    int i;
    for (i = 0; i < NCHAN; i++)
        if (chan[i].inuse && chan[i].hout && (mem[chan[i].hout] & RMASK))
            do_out(&chan[i]);
    for (i = 0; i < NCHAN; i++) chan_close(&chan[i]);
    tty_flush();
}

/* ------------------------------------------------------------------- entry */

int uuo(int op, int ac, int e)
{
    Chan *c = &chan[ac & 017];

    switch (op) {

    /* ------------------------------------------------------------- CALLI */
    case 047:
        switch (e) {
        case 0:    return 0;                                /* RESET  */
        case 012:  mon_exit(0); return 2;                   /* EXIT   */
        case 014:  AC = date_word(); return 0;              /* DATE   */
        case 023:  AC = mstime();    return 0;              /* MSTIME */
        case 027:  AC = (word)(icount / 1000); return 0;    /* RUNTIM */
        case 030:  AC = 0524660625242ULL; return 0;         /* GETPPN [RJB,JFP] */
        case 024:  AC = 0; return 0;                        /* SWITCH */
        case 011:  return 1;                                /* CORE   */
        case 010:  return 0;                                /* WAIT   */
        case 031:  return 0;                                /* SLEEP  */
        case 004:  AC = 0; return 1;                        /* DEVCHR */
        case 016:  return 0;                                /* APRENB */
        case 022:  return 1;                                /* TIMER  */
        case 034:  AC = 0; return 1;                        /* GETLIN */
        default:
            mtrace("calli %o ac=%o (ignored)", e, ac);
            return 0;
        }

    /* ------------------------------------------------------------ TTCALL */
    case 051:
        switch (ac) {
        case 0: case 4: {                                   /* INCHRW/INCHWL */
            int ch = tty_in(1);
            mem[e & (MEMSIZ - 1)] = (word)(ch & 0177);
            return 0;
        }
        case 2: case 5: {                                   /* INCHRS/INCHSL */
            int ch = tty_in(0);
            if (ch < 0) return 0;
            mem[e & (MEMSIZ - 1)] = (word)(ch & 0177);
            return 1;
        }
        case 1: case 015:                                   /* OUTCHR/IONEOU */
            tty_out((int)(mem[e & (MEMSIZ - 1)] & 0177));
            return 0;
        case 3: {                                           /* OUTSTR */
            int a = e, k;
            tty_peek();
            for (;;) {
                word w = mem[a & (MEMSIZ - 1)];
                for (k = 0; k < 5; k++) {
                    int ch = (int)((w >> (29 - 7 * k)) & 0177);
                    if (ch == 0) { tty_flush(); return 0; }
                    tty_out(ch);
                }
                a++;
            }
        }
        case 6:  mem[e & (MEMSIZ - 1)] = 0; return 0;       /* GETLCH */
        case 7:  return 0;                                  /* SETLCH */
        case 010: return 0;                                 /* RESCAN */
        case 011: inlen = inpos = 0; return 0;              /* CLRBFI */
        case 012: tty_flush(); return 0;                    /* CLRBFO */
        case 013: case 014: return 0;                       /* SKPINC/SKPINL */
        default:
            mtrace("ttcall %o e=%06o", ac, e);
            return 0;
        }

    /* -------------------------------------------------------------- OPEN */
    case 050: {
        int hout = (int)((mem[e + 2] >> 18) & RMASK);
        int hin  = (int)(mem[e + 2] & RMASK);
        chan_close(c);
        memset(c, 0, sizeof *c);
        c->inuse = 1;
        c->mode  = (int)(mem[e] & 017);
        c->dev   = mem[e + 1];
        c->hout  = hout;
        c->hin   = hin;
        sixbit_str(c->dev, c->devname);
        c->istty = (strncmp(c->devname, "TTY", 3) == 0);
        c->isdsk = (strncmp(c->devname, "DSK", 3) == 0);
        c->status = (word)c->mode;
        mtrace("open ch%o %s mode=%o out=%06o in=%06o",
               ac, c->devname, c->mode, c->hout, c->hin);
        return c->istty || c->isdsk;             /* skip = device available */
    }

    /* ------------------------------------------------------------ LOOKUP */
    case 076: {
        char nm[8], ex[8];
        sixbit_str(mem[e], nm);
        sixbit_str((mem[e + 1] >> 18) << 18, ex);
        mtrace("lookup ch%o %s: %s.%s", ac, c->devname, nm, ex);
        if (!c->isdsk) { c->open = 1; return 1; }
        chan_close(c);
        hostname(c->file, sizeof c->file, nm, ex);
        c->data = slurp(c->file, &c->len);
        if (c->data) c->owned = 1;
        if (!c->data && strcmp(nm, "FISK") == 0 && strcmp(ex, "TXT") == 0
            && image->txt_len > 1) {
            c->data = (unsigned char *)image->txt;
            c->len  = image->txt_len;
        }
        if (!c->data) {
            mem[e + 1] = (mem[e + 1] & LMASK) | 0;          /* no such file */
            return 0;
        }
        c->pos = 0; c->open = 1; c->status &= ~IO_EOF;
        mem[e + 3] = (word)((c->len + 4) / 5);
        return 1;
    }

    /* ------------------------------------------------------------- ENTER */
    case 077: {
        char nm[8], ex[8];
        sixbit_str(mem[e], nm);
        sixbit_str((mem[e + 1] >> 18) << 18, ex);
        mtrace("enter ch%o %s: %s.%s", ac, c->devname, nm, ex);
        if (!c->isdsk) { c->open = 1; return 1; }
        chan_close(c);
        hostname(c->file, sizeof c->file, nm, ex);
        c->wf = fopen(c->file, "wb");
        if (!c->wf) { mem[e + 1] = (mem[e + 1] & LMASK) | 2; return 0; }
        c->open = 1;
        return 1;
    }

    case 055: mtrace("rename ch%o", ac); return 1;          /* RENAME  */

    case 070:                                               /* CLOSE   */
        if (c->hout && (mem[c->hout] & RMASK)) do_out(c);
        chan_close(c);
        tty_flush();
        mtrace("close ch%o", ac);
        return 0;

    case 071:                                               /* RELEAS  */
        if (c->hout && (mem[c->hout] & RMASK)) do_out(c);
        chan_close(c);
        tty_flush();
        mtrace("releas ch%o", ac);
        memset(c, 0, sizeof *c);
        return 0;

    case 064: setup_ring(c->hin  ? c->hin  : e, e ? e : 2, 7); return 0; /* INBUF  */
    case 065: setup_ring(c->hout ? c->hout : e, e ? e : 2, 7); return 0; /* OUTBUF */

    case 056: return do_in(c);                              /* IN      */
    case 066: do_in(c); return 0;                           /* INPUT   */
    case 057: return do_out(c);                             /* OUT     */
    case 067: do_out(c); return 0;                          /* OUTPUT  */

    case 060: c->status = (c->status & ~RMASK) | (mem[e & (MEMSIZ-1)] & RMASK);
              return 0;                                     /* SETSTS  */
    case 062: mem[e & (MEMSIZ - 1)] = c->status; return 0;  /* GETSTS  */
    case 061: return (c->status & (word)e) == (word)e;      /* STATO   */
    case 063: return (c->status & (word)e) == 0;            /* STATZ   */
    case 072: mtrace("mtape ch%o e=%o", ac, e); return 1;   /* MTAPE   */
    case 073: AC = 1; return 0;                             /* UGETF   */

    case 074:                                               /* USETI   */
        if (c->data) {
            int blk = e ? e : 1;
            c->pos = (blk - 1) * BUFWORDS * 5;
            if (c->pos > c->len) c->pos = c->len;
            if (c->pos < 0) c->pos = 0;
            c->status &= ~IO_EOF;
            mtrace("useti ch%o blk=%d pos=%d", ac, blk, c->pos);
        }
        return 0;
    case 075: return 0;                                     /* USETO   */

    case 041: mtrace("init ch%o e=%06o", ac, e); return 1;  /* INIT    */

    default:
        mtrace("uuo %03o ac=%o e=%06o", op, ac, e);
        return 0;
    }
}
