/* monitor.c -- the slice of the TOPS-10 / TYMCOM-X monitor that ADVENTURE
 * needs.  Taken from the EXPLOR and CRYSTAL CAVE ports, which need the same
 * slice: all three are DEC FORTRAN programs off the same Tymshare tapes, and
 * they ask the monitor for the same things.  Only the default core file name
 * differs.
 *
 * FOROTS (the FORTRAN-10 object time system) talks to the operating
 * system through UUOs.  This file answers them: terminal I/O by TTCALL
 * and by buffered device I/O, the handful of CALLIs FOROTS asks for,
 * and enough of the file UUOs that a LOOKUP on DSK fails politely.
 */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define isatty  _isatty
#define fileno  _fileno
#else
#include <unistd.h>
#endif
#include "pdp10.h"

/* ------------------------------------------------------------------ */
/* options set from the command line                                    */
/* ------------------------------------------------------------------ */
int  opt_verbose = 0;
int  opt_delays = 1;      /* honour the game's SLEEP calls (combat pacing) */
/* DEVCHR as reported for TTY.  The left half is the standard TOPS-10
 * device characteristics word; F40's object time system reads it closely:
 *
 *   200000  DVIN   device can do input
 *   100000  DVOUT  device can do output
 *      200  DVTTY  device is a terminal
 *       40  DVAVL  device is available
 *
 * and no DVDIR (400000), because a terminal is not a directory device.
 * The right half is the bit mask of legal data modes -- ASCII and
 * ASCII-line.
 *
 * DVIN matters more than it looks.  At 120716 the OTS does TLNN 5,200020
 * on this word before every WRITE that follows a READ on the same unit,
 * and if neither DVIN nor DVMTA (20) is set it decides the unit is a
 * file it would have to reposition, prints "WARNING! FORMATTED READ
 * FOLLOWED BY WRITE MAY FAIL." and abandons the transfer -- which, for a
 * program whose whole life is read-a-command / write-a-reply on the
 * terminal, means it never gets a command at all.
 *
 * Which device this is reported for matters just as much; see DEVCHR. */
w36  opt_devchr_tty = 0300240000003ULL;
/* On the DECsystem-10 the terminal did not echo what you typed -- the
 * program did.  When a READ is followed by a WRITE on the same unit,
 * F40's runtime carries the record it just read into the output buffer,
 * and that carried copy is what put your command back on the screen.
 * A Windows console echoes as you type, so the line would appear twice.
 * When standard input is an interactive console we therefore drop the
 * runtime's copy, which is exactly the redundant one; when input is
 * piped nothing else echoes, so it is kept and transcripts read
 * properly.  -e / --echo forces it back on. */
int  opt_dedupe_echo = -1;       /* -1 = decide from isatty            */
int  opt_faketime = -1;          /* minutes past midnight, or -1       */
const char *opt_corefile = "advent.core";
int  opt_autosave = 1;

/* ------------------------------------------------------------------ */
/* terminal                                                             */
/* ------------------------------------------------------------------ */
static const char *pending;      /* lines the driver answers for us */

void queue_input(const char *s) { pending = s; }

static unsigned char tibuf[1024];
static int tilen, tipos;
static int tty_eof;
static int col;                  /* output column, for the log         */

/* SUSPEND is the game's save command, and it works the way saving
 * worked on TOPS-10: the program tells you to break out and save your
 * core image, and you do.  There is no file UUO to hook -- the program
 * simply stops -- so watch the output for the line it prints when it has
 * finished tidying its state, and let the driver write the image then. */
int suspend_requested = 0;
static char otail[32];
static int otailn;

static void tty_out(int c)
{
    c &= 0177;
    if (c == 0) return;
    if (c >= ' ') {
        if (otailn == (int)sizeof otail) {
            memmove(otail, otail + 1, sizeof otail - 1);
            otailn--;
        }
        otail[otailn++] = (char)c;
        if (otailn >= 10 && !memcmp(otail + otailn - 10, "CORE-IMAGE", 10))
            suspend_requested = 1;
    }
    if (c == 015) return;                    /* CR: let LF do the work */
    if (c == 012) { putchar('\n'); col = 0; return; }
    if (c == 011) { putchar('\t'); col = (col + 8) & ~7; return; }
    if (c < 040 && c != 014) return;
    putchar(c);
    col++;
}

static int tty_fill(void)
{
    int c, n = 0;
    if (pending) {
        while (*pending && *pending != 10)
            tibuf[n++] = (unsigned char)*pending++;
        if (*pending == 10) pending++;
        if (!*pending) pending = NULL;
        tibuf[n++] = 015;
        tibuf[n++] = 012;
        tilen = n; tipos = 0; col = 0;
        return n;
    }
    if (tty_eof) { halted = 1; return 0; }
    fflush(stdout);
    for (;;) {
        c = getchar();
        if (c == EOF) {
            tty_eof = 1;
            if (n == 0) { halted = 1; return 0; }
            break;
        }
        if (c == '\r') continue;
        if (c == '\n') break;
        if (n < (int)sizeof tibuf - 4) tibuf[n++] = (unsigned char)c;
    }
    tibuf[n++] = 015;
    tibuf[n++] = 012;
    tilen = n; tipos = 0;
    col = 0;
    return n;
}

static int tty_in(void)
{
    if (tipos >= tilen && !tty_fill())
        return 032;                          /* ^Z at end of input     */
    return tibuf[tipos++];
}

static int tty_avail(void) { return tipos < tilen; }

/* the line most recently handed to the program, for echo suppression */
static unsigned char lastin[1024];
static int lastinlen;

/* ------------------------------------------------------------------ */
/* SIXBIT helpers                                                       */
/* ------------------------------------------------------------------ */
static void sixbit_str(w36 w, char *out)
{
    int i, n = 0;
    for (i = 0; i < 6; i++) {
        int c = (int)((w >> (30 - 6 * i)) & 077) + 040;
        out[n++] = (char)c;
    }
    while (n > 0 && out[n - 1] == ' ') n--;
    out[n] = 0;
}

/* ------------------------------------------------------------------ */
/* channels                                                             */
/* ------------------------------------------------------------------ */
#define NCHAN 16

/* Buffer sizes, in words, as reported by DEVSIZ and used to build the
 * rings.  Two words of each buffer are the link and the byte count. */
#define TTYBUF 0023
#define DSKBUF 0203

#define IO_IMP 0400000000000ULL
#define IO_DER 0200000000000ULL
#define IO_DTE 0100000000000ULL
#define IO_BKT 0040000000000ULL
#define IO_EOF 0020000000000ULL
#define IO_ACT 0010000000000ULL

typedef struct {
    int  open;
    int  istty;
    int  isnull;
    char dev[8];
    int  mode;
    w36  status;
    int  ibufhdr, obufhdr;       /* buffer header addresses            */
    int  ibuf, obuf;             /* first buffer of each ring          */
    int  ibufwords, obufwords;   /* data words per buffer              */
    FILE *fp;
} chan_t;

static chan_t chan[NCHAN];

/* Allocate n words from the top of the low segment (.JBFF). */
static int alloc_core(int n)
{
    int a = (int)RH(M[0121]);
    M[0121] = XWD(0, a + n);
    if (a + n > (int)RH(M[0120]))
        M[0120] = XWD(a + n, RH(M[0120]));
    return a;
}

/* Build a ring of nbuf buffers of nwords data words each.
 *
 * TOPS-10 buffer layout, which the object time system reads directly:
 *
 *   word 0   XWD(length, next buffer)   length counts every word of the
 *                                       buffer except this one, so it is
 *                                       nwords + 1 (the count word plus
 *                                       the data).  Bit 0 of the left
 *                                       half is the "use" bit.
 *   word 1   XWD(0, byte count)
 *   word 2.. data
 *
 * The length in that left half is not decoration.  At 120731 the OTS
 * picks up LH of the *next* buffer's link word and uses it as the word
 * count for the BLT that copies a line between buffers; left at zero the
 * BLT is handed a backwards range and walks the whole address space. */
static int make_ring(int nbuf, int nwords, int *firstp)
{
    int i, first = 0, prev = 0, b;
    for (i = 0; i < nbuf; i++) {
        b = alloc_core(nwords + 2);
        if (!first) first = b;
        else M[prev] = XWD(nwords + 1, b);
        prev = b;
        M[b + 1] = 0;
    }
    M[prev] = XWD(nwords + 1, first);
    *firstp = first;
    return first;
}

static w36 make_bp(int addr) { return XWD(0440700, addr); }

/* Advance a byte pointer and return the byte it then addresses -- the
 * ILDB the hardware would do. */
static int bp_peek(w36 *bp)
{
    int p = (int)((*bp >> 30) & 077), s = (int)((*bp >> 24) & 077), a;
    p -= s;
    if (p < 0) { p = 36 - s; *bp = (*bp & ~(w36)HMASK) | ((*bp + 1) & HMASK); }
    *bp = (*bp & ~((w36)077 << 30)) | ((w36)p << 30);
    a = (int)RH(*bp);
    return (int)((M[a] >> p) & ((1u << s) - 1));
}

static void set_hdr_out(int hdr, int buf, int nwords)
{
    M[hdr]     = XWD(0, buf);
    M[hdr + 1] = make_bp(buf + 2);
    M[hdr + 2] = (w36)(nwords * 5);
    M[buf + 1] = 0;
}

static void ensure_in_ring(int c);
static void ensure_out_ring(int c);

/* ------------------------------------------------------------------ */
static void do_out(int c)
{
    chan_t *ch = &chan[c];
    int hdr = ch->obufhdr, buf, nb, i, dedupe;
    w36 bp;

    if (!hdr) return;
    ensure_out_ring(c);
    buf = (int)RH(M[hdr]);
    if (!buf) return;
    nb = ch->obufwords * 5 - (int)(M[hdr + 2] & HMASK);
    if (nb < 0) nb = 0;

    /* If this record opens with the line the program was just handed,
     * that is the runtime's carried-forward copy of what you typed --
     * the echo.  Drop it when the console has already shown the line.
     * All or nothing: a record that merely starts the same way is left
     * alone.  See opt_dedupe_echo. */
    dedupe = 0;
    if (ch->istty && lastinlen && nb >= lastinlen) {
        w36 q = make_bp(buf + 2);
        for (i = 0; i < lastinlen; i++) {
            if (bp_peek(&q) != lastin[i]) break;
        }
        if (i == lastinlen) dedupe = lastinlen;
        lastinlen = 0;                     /* one line, one suppression */
    }

    bp = make_bp(buf + 2);
    for (i = 0; i < nb; i++) {
        int byte = bp_peek(&bp);
        if (i < dedupe) continue;
        if (ch->istty) tty_out(byte);
        else if (ch->fp) fputc(byte & 0177, ch->fp);
    }
    if (ch->istty) fflush(stdout);

    /* advance the ring */
    buf = (int)RH(M[buf]);
    set_hdr_out(hdr, buf, ch->obufwords);
}

static int do_in(int c)
{
    chan_t *ch = &chan[c];
    int hdr = ch->ibufhdr, buf, i, n = 0;
    w36 bp;

    if (!hdr) return 0;
    ensure_in_ring(c);
    buf = (int)RH(M[hdr]);
    if (!buf) return 0;
    buf = (int)RH(M[buf]);                   /* next buffer in the ring */

    bp = make_bp(buf + 2);
    /* Clear the buffer before filling it.  When a READ is followed by a
     * WRITE on the same unit the object time system copies the whole
     * input buffer into the output buffer rather than just the bytes it
     * read, which is what puts the typed line back on the screen here.
     * Leaving the tail of a previous, longer line in place makes that
     * copy trail stale characters after the new one. */
    for (i = 0; i < ch->ibufwords; i++)
        M[buf + 2 + i] = 0;
    if (ch->istty) {
        int limit = ch->ibufwords * 5;
        for (i = 0; i < limit; i++) {
            int ci = tty_in();
            int p, s, a;
            p = (int)((bp >> 30) & 077); s = (int)((bp >> 24) & 077);
            p -= s;
            if (p < 0) { p = 36 - s; bp = (bp & ~(w36)HMASK) | ((bp + 1) & HMASK); }
            bp = (bp & ~((w36)077 << 30)) | ((w36)p << 30);
            a = (int)RH(bp);
            M[a] = (M[a] & ~((w36)0177 << p)) | ((w36)(ci & 0177) << p);
            n++;
            if (ci == 012) break;
        }
    } else if (ch->fp) {
        int limit = ch->ibufwords * 5;
        for (i = 0; i < limit; i++) {
            int ci = fgetc(ch->fp);
            int p, s, a;
            if (ci == EOF) break;
            p = (int)((bp >> 30) & 077); s = (int)((bp >> 24) & 077);
            p -= s;
            if (p < 0) { p = 36 - s; bp = (bp & ~(w36)HMASK) | ((bp + 1) & HMASK); }
            bp = (bp & ~((w36)077 << 30)) | ((w36)p << 30);
            a = (int)RH(bp);
            M[a] = (M[a] & ~((w36)0177 << p)) | ((w36)(ci & 0177) << p);
            n++;
            if (ci == '\n') break;
        }
    }

    if (ch->istty) {
        int k;
        lastinlen = (n > (int)sizeof lastin) ? (int)sizeof lastin : n;
        for (k = 0; k < lastinlen; k++) {
            int a2 = buf + 2 + k / 5;
            lastin[k] = (unsigned char)((M[a2] >> (29 - 7 * (k % 5))) & 0177);
        }
        if (!opt_dedupe_echo) lastinlen = 0;
    }
    M[buf + 1] = XWD(0, n);
    M[hdr]     = XWD(0, buf);
    M[hdr + 1] = make_bp(buf + 2);
    M[hdr + 2] = (w36)n;
    if (opt_verbose > 2) {
        int k;
        fflush(stdout);
        fprintf(stderr, "[in] ch%o hdr=%06o buf=%06o n=%d |", c, hdr, buf, n);
        for (k = 0; k < n && k < 80; k++) {
            int a2 = buf + 2 + k / 5;
            int ch2 = (int)((M[a2] >> (29 - 7 * (k % 5))) & 0177);
            fputc(ch2 >= 32 && ch2 < 127 ? ch2 : '.', stderr);
        }
        fprintf(stderr, "|");
        fputc(10, stderr);
    }
    if (n == 0) { ch->status |= IO_EOF; return 0; }
    return 1;
}

/* ------------------------------------------------------------------ */
/* time and date                                                        */
/* ------------------------------------------------------------------ */
static time_t t0;

static struct tm *nowtm(void)
{
    static struct tm tmv;
    time_t t = time(NULL);
    struct tm *p = localtime(&t);
    tmv = *p;
    if (opt_faketime >= 0) {
        tmv.tm_hour = opt_faketime / 60;
        tmv.tm_min  = opt_faketime % 60;
        tmv.tm_sec  = 0;
    }
    return &tmv;
}

static w36 date_word(void)
{
    struct tm *tm = nowtm();
    int y = tm->tm_year + 1900, mo = tm->tm_mon, d = tm->tm_mday - 1;
    return (w36)((((y - 1964) * 12 + mo) * 31 + d));
}

static long mstime(void)
{
    struct tm *tm = nowtm();
    return ((tm->tm_hour * 60L + tm->tm_min) * 60L + tm->tm_sec) * 1000L;
}

/* ------------------------------------------------------------------ */
void monitor_init(void)
{
    if (opt_dedupe_echo < 0)
        opt_dedupe_echo = isatty(fileno(stdin)) ? 1 : 0;
    t0 = time(NULL);
    memset(chan, 0, sizeof chan);
}

void monitor_shutdown(void)
{
    int i;
    for (i = 0; i < NCHAN; i++)
        if (chan[i].open && chan[i].istty && chan[i].obufhdr)
            do_out(i);
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
static void warn(const char *fmt, ...)
{
    va_list ap;
    if (!opt_verbose) return;
    va_start(ap, fmt);
    fflush(stdout);
    fprintf(stderr, "[uuo] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

/* Returns the number of words to skip past the UUO. */
static int do_calli(int ac, int fn)
{
    switch (fn) {
    case 000:                                            /* RESET      */
        { int i; for (i = 0; i < NCHAN; i++) {
              if (chan[i].fp) fclose(chan[i].fp);
              memset(&chan[i], 0, sizeof chan[i]); } }
        return 0;
    case 004:                                            /* DEVCHR     */
        { char n[8]; sixbit_str(AC(ac), n);
          AC(ac) = (strncmp(n, "TTY", 3) == 0) ? opt_devchr_tty : 0;
          return 0; }
    case 006: AC(ac) = 0; return 0;                      /* GETCHR     */
    case 010: return 0;                                  /* WAIT       */
    case 011: {                                          /* CORE       */
        int req = (int)RH(AC(ac));
        if (req) {
            int top = ((req + 1 + 01777) & ~01777) - 1;
            if (top > (int)RH(M[044])) M[044] = XWD(0, top);
        }
        return 1; }
    case 012:                                            /* EXIT       */
        halted = 1;
        return 0;
    case 013: return 0;                                  /* UTPCLR     */
    case 014: AC(ac) = date_word(); return 0;            /* DATE       */
    case 015: return 1;                                  /* LOGIN      */
    case 016: return 0;                                  /* APRENB     */
    case 020: AC(ac) = 0; return 0;                      /* SWITCH     */
    case 022: AC(ac) = (w36)(mstime() / 1000 * 60); return 0;  /* TIMER */
    case 023: AC(ac) = (w36)mstime(); return 0;          /* MSTIME     */
    case 024: AC(ac) = XWD(1, 1); return 0;              /* GETPPN     */
    case 025: return 1;                                  /* TRPSET     */
    case 027: AC(ac) = (w36)((time(NULL) - t0) * 1000);  /* RUNTIM     */
              return 0;
    case 030: AC(ac) = 1; return 0;                      /* PJOB       */
    case 031: {                                          /* SLEEP      */
        int secs = (int)(AC(ac) & 0777777);
        if (opt_delays && secs > 0) {
            if (secs > 5) secs = 5;
            fflush(stdout);
#ifdef _WIN32
            Sleep((DWORD)secs * 1000);
#else
            sleep((unsigned)secs);
#endif
        }
        return 0; }
    case 032: return 0;                                  /* SETPOV     */
    case 034: AC(ac) = 0; return 0;                      /* GETLIN     */
    case 036: return 1;                                  /* SETUWP     */
    case 041:                                            /* GETTAB     */
        warn("GETTAB %012llo", (unsigned long long)AC(ac));
        AC(ac) = 0;
        return 1;
    case 043: return 1;                                  /* SETUUO     */
    case 053: {                                          /* DEVTYP     */
        char n[8]; sixbit_str(AC(ac), n);
        if (strncmp(n, "TTY", 3) == 0)
            AC(ac) = 012;              /* device type 12 = terminal    */
        else if (strncmp(n, "DSK", 3) == 0)
            AC(ac) = 0;
        else { AC(ac) = 0; return 0; }
        return 1; }
    case 0101: {                                         /* DEVSIZ     */
        int blk = (int)RH(AC(ac));
        char n[8];
        sixbit_str(M[(blk + 1) & HMASK], n);
        AC(ac) = XWD(0, (strncmp(n, "TTY", 3) == 0) ? TTYBUF : DSKBUF);
        return 1; }
    case 052: AC(ac) = 0; return 0;                      /* JOBSTS     */
    case 066: AC(ac) = 0; return 1;                      /* TRMNO.     */
    case 067: AC(ac) = 0; return 1;                      /* TRMOP.     */
    default:
        warn("unimplemented CALLI %o (ac%o=%012llo)", fn, ac,
             (unsigned long long)AC(ac));
        AC(ac) = 0;
        return 1;
    }
}

static int do_ttcall(int ac, int e)
{
    switch (ac) {
    case 000: M[e] = (w36)tty_in(); return 0;            /* INCHRW     */
    case 001: tty_out((int)(M[e] & 0177)); return 0;     /* OUTCHR     */
    case 002:                                            /* INCHRS     */
        if (!tty_avail()) return 0;
        M[e] = (w36)tty_in(); return 1;
    case 003: {                                          /* OUTSTR     */
        w36 bp = make_bp(e);
        for (;;) {
            int p = (int)((bp >> 30) & 077), s = 7, a, c;
            p -= s;
            if (p < 0) { p = 36 - s; bp = (bp & ~(w36)HMASK) | ((bp + 1) & HMASK); }
            bp = (bp & ~((w36)077 << 30)) | ((w36)p << 30);
            a = (int)RH(bp);
            c = (int)((M[a] >> p) & 0177);
            if (c == 0) break;
            tty_out(c);
        }
        fflush(stdout);
        return 0; }
    case 004: M[e] = (w36)tty_in(); return 0;            /* INCHWL     */
    case 005:                                            /* INCHSL     */
        if (!tty_avail() && tipos >= tilen) { if (!tty_fill()) return 0; }
        M[e] = (w36)tty_in(); return 1;
    case 006: M[e] = 0; return 0;                        /* GETLCH     */
    case 007: return 0;                                  /* SETLCH     */
    case 010: return 0;                                  /* RESCAN     */
    case 011: tipos = tilen = 0; return 0;               /* CLRBFI     */
    case 012: fflush(stdout); return 0;                  /* CLRBFO     */
    case 013: return tty_avail() ? 1 : 0;                /* SKPINC     */
    case 014: return tty_avail() ? 1 : 0;                /* SKPINL     */
    case 015: tty_out(e & 0177); fflush(stdout); return 0;   /* IONEOU */
    default:
        warn("unimplemented TTCALL %o", ac);
        return 0;
    }
}

static void setup_channel(int c, int mode, w36 devword, w36 bufword)
{
    chan_t *ch = &chan[c];
    char name[8];
    sixbit_str(devword, name);
    memset(ch, 0, sizeof *ch);
    ch->open = 1;
    ch->mode = mode & 017;
    memcpy(ch->dev, name, strlen(name) + 1);
    ch->istty = (strncmp(name, "TTY", 3) == 0) || (name[0] == 0);
    ch->isnull = (strcmp(name, "NUL") == 0);
    ch->obufhdr = (int)LH(bufword);
    ch->ibufhdr = (int)RH(bufword);
    ch->status = (w36)(mode & 017);
    ch->ibufwords = (ch->istty ? TTYBUF : DSKBUF) - 2;
    ch->obufwords = (ch->istty ? TTYBUF : DSKBUF) - 2;
}

/* OPEN does not build buffer rings -- INBUF/OUTBUF do, and a program is
 * equally free to build its own and simply leave the header pointing at
 * it, which is what F40's object time system does: it asks DEVSIZ for a
 * sensible buffer size and lays the ring out itself, never issuing
 * INBUF or OUTBUF at all.  Allocating at OPEN time took core from .JBFF
 * that the OTS then handed out again for its own ring, so the two
 * overlapped and the first buffer advance walked off into a runaway BLT.
 * Allocate lazily instead, and only if the header is still empty. */
static void ensure_in_ring(int c)
{
    chan_t *ch = &chan[c];
    if (!ch->ibufhdr || RH(M[ch->ibufhdr])) return;
    make_ring(2, ch->ibufwords, &ch->ibuf);
    M[ch->ibufhdr]     = XWD(0, ch->ibuf);
    M[ch->ibufhdr + 1] = make_bp(ch->ibuf + 2);
    M[ch->ibufhdr + 2] = 0;
}

static void ensure_out_ring(int c)
{
    chan_t *ch = &chan[c];
    if (!ch->obufhdr || RH(M[ch->obufhdr])) return;
    make_ring(2, ch->obufwords, &ch->obuf);
    set_hdr_out(ch->obufhdr, ch->obuf, ch->obufwords);
}

/* ------------------------------------------------------------------ */
static const char *uuoname(int op)
{
    switch (op) {
    case 040: return "CALL";   case 041: return "INIT";   case 047: return "CALLI";
    case 050: return "OPEN";   case 051: return "TTCALL"; case 055: return "RENAME";
    case 056: return "IN";     case 057: return "OUT";    case 060: return "SETSTS";
    case 061: return "STATO";  case 062: return "GETSTS"; case 063: return "STATZ";
    case 064: return "INBUF";  case 065: return "OUTBUF"; case 066: return "INPUT";
    case 067: return "OUTPUT"; case 070: return "CLOSE";  case 071: return "RELEAS";
    case 072: return "MTAPE";  case 073: return "UGETF";  case 074: return "USETI";
    case 075: return "USETO";  case 076: return "LOOKUP"; case 077: return "ENTER";
    default:  return "UUO";
    }
}

int monitor_uuo(w36 inst, int op, int ac, int e)
{
    if (opt_verbose > 1) {
        fflush(stdout);
        fprintf(stderr, "[uuo] %06o %-6s %o,%06o  ac=%012llo\n",
                (PC - 1) & HMASK, uuoname(op), ac, e,
                (unsigned long long)AC(ac));
    }
    switch (op) {

    case 040: {                                          /* CALL       */
        char n[8];
        sixbit_str(M[e], n);
        if (!strcmp(n, "EXIT"))   { halted = 1; return 0; }
        if (!strcmp(n, "RESET"))  return do_calli(ac, 000);
        if (!strcmp(n, "DATE"))   return do_calli(ac, 014);
        if (!strcmp(n, "MSTIME")) return do_calli(ac, 023);
        if (!strcmp(n, "RUNTIM")) return do_calli(ac, 027);
        if (!strcmp(n, "TIMER"))  return do_calli(ac, 022);
        if (!strcmp(n, "SLEEP"))  return do_calli(ac, 031);
        if (!strcmp(n, "GETPPN")) return do_calli(ac, 024);
        if (!strcmp(n, "PJOB"))   return do_calli(ac, 030);
        if (!strcmp(n, "APRENB")) return do_calli(ac, 016);
        if (!strcmp(n, "SETUWP")) return do_calli(ac, 036);
        if (!strcmp(n, "CORE"))   return do_calli(ac, 011);
        if (!strcmp(n, "DEVCHR")) return do_calli(ac, 004);
        if (!strcmp(n, "GETTAB")) return do_calli(ac, 041);
        warn("unimplemented CALL [SIXBIT /%s/]", n);
        AC(ac) = 0;
        return 1; }

    case 047:                                            /* CALLI      */
        return do_calli(ac, sx18(e) < 0 ? (int)(e & 077777) : e);

    case 051:                                            /* TTCALL     */
        return do_ttcall(ac, e);

    case 041: {                                          /* INIT       */
        int c = ac & 017;
        setup_channel(c, e, M[PC], M[PC + 1]);
        return 3; }

    case 050: {                                          /* OPEN       */
        int c = ac & 017;
        setup_channel(c, (int)(M[e] & 017), M[e + 1], M[e + 2]);
        return 1; }

    case 064: {                                          /* INBUF      */
        int c = ac & 017;
        chan_t *ch = &chan[c];
        if (ch->ibufhdr && !ch->ibuf) {
            make_ring(e ? e : 2, ch->ibufwords, &ch->ibuf);
            M[ch->ibufhdr]     = XWD(0, ch->ibuf);
            M[ch->ibufhdr + 1] = make_bp(ch->ibuf + 2);
            M[ch->ibufhdr + 2] = 0;
        }
        return 0; }

    case 065: {                                          /* OUTBUF     */
        int c = ac & 017;
        chan_t *ch = &chan[c];
        if (ch->obufhdr && !ch->obuf) {
            make_ring(e ? e : 2, ch->obufwords, &ch->obuf);
            set_hdr_out(ch->obufhdr, ch->obuf, ch->obufwords);
        }
        return 0; }

    case 056:                                            /* IN         */
        return do_in(ac & 017) ? 1 : 0;
    case 066:                                            /* INPUT      */
        do_in(ac & 017);
        return 0;
    case 057:                                            /* OUT        */
        do_out(ac & 017);
        return 1;
    case 067:                                            /* OUTPUT     */
        do_out(ac & 017);
        return 0;

    case 070: {                                          /* CLOSE      */
        chan_t *ch = &chan[ac & 017];
        if (ch->obufhdr) do_out(ac & 017);
        if (ch->fp) { fclose(ch->fp); ch->fp = NULL; }
        ch->status &= ~IO_EOF;
        return 0; }

    case 071: {                                          /* RELEAS     */
        chan_t *ch = &chan[ac & 017];
        if (ch->fp) { fclose(ch->fp); ch->fp = NULL; }
        memset(ch, 0, sizeof *ch);
        return 0; }

    case 060:                                            /* SETSTS     */
        chan[ac & 017].status = (w36)e;
        return 0;
    case 062:                                            /* GETSTS     */
        M[e] = chan[ac & 017].status;
        return 0;
    case 061:                                            /* STATO      */
        return (chan[ac & 017].status & (w36)e) ? 1 : 0;
    case 063:                                            /* STATZ      */
        return (chan[ac & 017].status & (w36)e) ? 0 : 1;

    case 076:                                            /* LOOKUP     */
    case 077: {                                          /* ENTER      */
        chan_t *ch = &chan[ac & 017];
        char name[8], ext[8];
        sixbit_str(M[e], name);
        sixbit_str(M[e + 1], ext);
        if (ch->istty || ch->isnull) return 1;
        warn("%s %s.%s on channel %o -- failing", op == 076 ? "LOOKUP" : "ENTER",
             name, ext, ac & 017);
        M[e + 1] = XWD(0, 1);                  /* file not found       */
        return 0; }

    case 055:                                            /* RENAME     */
        return 0;
    case 072:                                            /* MTAPE      */
        return 0;
    case 073: AC(ac) = 0; return 0;                      /* UGETF      */
    case 074: case 075: return 0;                        /* USETI/USETO*/

    default:
        if (op >= 0700) {
            warn("user mode I/O instruction %03o at %06o", op, (PC - 1) & HMASK);
            return 0;
        }
        warn("unimplemented UUO %03o ac=%o e=%06o", op, ac, e);
        return 1;
    }
}
