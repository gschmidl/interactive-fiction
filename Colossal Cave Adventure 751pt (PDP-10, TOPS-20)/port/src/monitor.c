/* monitor.c -- the slice of the TOPS-10 monitor that ADVENTURE needs.
 *
 * FOROTS (the FORTRAN-10 object time system) talks to the operating
 * system through UUOs, and on the real machine TOPS-20 answered them
 * through the PA1050 compatibility package.  This file answers them
 * directly instead: terminal I/O by TTCALL and by buffered device I/O,
 * the CALLIs FOROTS and the game ask for -- including the GETSEG that
 * brings FOROTS itself in at 0400000 -- and disk files, which the game
 * needs for its databases and its saved games.
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
int  opt_delays = 1;      /* honour the game's SLEEP calls               */
/* DEVCHR and DEVTYP, as the machine this game came off answers them.
 *
 * These are not guesses.  A short MACRO-10 program assembled and run on
 * the same TOPS-20 pack --
 *
 *      MOVE  1,[SIXBIT /TTY/]
 *      CALLI 1,4                ;DEVCHR
 *      ...print AC1 in octal...
 *      MOVE  1,[SIXBIT /TTY/]
 *      CALLI 1,53               ;DEVTYP
 *
 * -- printed exactly the words below, which is what PA1050 handed
 * FOROTS on the real system.
 *
 * The left half of a DEVCHR word is the device characteristics and the
 * right half the mask of legal data modes.  The three bottom bits of
 * the left half are the ones FOROTS checks an ACCESS mode against
 * (output, input, random); DVAVL is 040 and it insists on that too.
 * DEVTYP returns the device type in the bottom six bits of its right
 * half -- 0 for the disk, 3 for a terminal -- and two flags in the left
 * half that FOROTS reads at 0400401: 010 marks a sequential device and
 * 020 a randomly addressable one.
 */
w36  opt_devchr_tty = 0030053400403ULL;
w36  opt_devchr_dsk = 0201047177777ULL;
w36  opt_devtyp_tty = 0000013000003ULL;
w36  opt_devtyp_dsk = 0400063000000ULL;
/* Echoing typed lines.  On the real system the terminal echoed what you
 * typed, and the transcripts show your commands in among the game's
 * replies.  A Windows console does the same for itself, so echo only
 * when input is not a console -- which is also when a transcript is
 * being made and the commands are worth having in it. */
int  opt_echo = -1;              /* -1 = decide from isatty            */
int  opt_faketime = -1;          /* minutes past midnight, or -1       */

/* ------------------------------------------------------------------ */
/* terminal                                                             */
/* ------------------------------------------------------------------ */
static unsigned char tibuf[1024];
static int tilen, tipos;
int tty_eof;
static int col;                  /* output column, for the log         */

void tty_flush(void) { fflush(stdout); }

void tty_out(int c)
{
    c &= 0177;
    if (c == 0) return;
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
    if (opt_echo) {
        int k;
        for (k = 0; k < n; k++) tty_out(tibuf[k]);
        fflush(stdout);
    }
    return n;
}

int tty_in(void)
{
    if (tipos >= tilen && !tty_fill())
        return 032;                          /* ^Z at end of input     */
    return tibuf[tipos++];
}

int tty_avail(void) { return tipos < tilen; }

/* ------------------------------------------------------------------ */
/* SIXBIT helpers                                                       */
/* ------------------------------------------------------------------ */
static void warn(const char *fmt, ...);

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
/* disk files                                                           */
/* ------------------------------------------------------------------ */
/* The game keeps its world in two binary files -- ADVTXT.BIN, the text,
 * and ADVVAR.BIN, everything else -- and writes saved games, the
 * scoreboard and the bulletin board beside them.  Those come off the
 * TOPS-20 pack and are built into this program (data.c); anything the
 * game writes is kept in memory and flushed to a real file of the same
 * name in the working directory when the channel is closed, so a file
 * once written is picked up from disk next time in preference to the
 * built-in copy.
 *
 * A TOPS-10 disk file is a vector of 36-bit words addressed in blocks of
 * 128, which is exactly how FOROTS gets at a FORTRAN random access unit,
 * so that is what a file is here. */
#define BLKWORDS 128

typedef struct {
    char name[32];
    w36 *w;
    int  nwords;
    int  cap;
    int  dirty;
} f10file_t;

/* Work out the file name a LOOKUP or ENTER is asking for.
 *
 * There are two shapes of argument block.  The short one is four words
 * beginning with the SIXBIT file name; the extended one begins with a
 * word whose left half is zero and whose right half is the block length,
 * and puts the name two words further on:
 *
 *      E+0  .RBCNT  0,,length of block
 *      E+1  .RBPPN  directory
 *      E+2  .RBNAM  SIXBIT file name
 *      E+3  .RBEXT  SIXBIT extension,,date and flags
 *      E+4  .RBPRV  protection,,mode
 *      E+5  .RBSIZ  size of the file in words
 *
 * A SIXBIT file name never has a zero left half -- a name cannot begin
 * with a space -- so the zero is what tells the two apart, and FOROTS
 * uses the long form.  Returns the address of the name word. */
static int fnameblock(int e)
{
    return (LH(M[e]) == 0 && RH(M[e]) >= 4) ? e + 2 : e;
}

static void fname(char *out, int nameaddr)
{
    char n[8], x[8];
    int i;
    sixbit_str(M[nameaddr], n);
    sixbit_str(M[nameaddr + 1] & ~(w36)HMASK, x);
    for (i = 0; x[i]; i++) if (x[i] == ' ') { x[i] = 0; break; }
    if (x[0]) sprintf(out, "%s.%s", n, x);
    else      sprintf(out, "%s", n);
}

/* Pack/unpack the on-disk form: five 8-bit frames to a 36-bit word, the
 * same core-dump layout the words came off the pack in. */
static int file_read_disk(const char *name, w36 **wp, int *np)
{
    FILE *fp = fopen(name, "rb");
    long len;
    unsigned char *raw;
    int i, n;
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END); len = ftell(fp); fseek(fp, 0, SEEK_SET);
    raw = malloc((size_t)len + 5);
    if (!raw) { fclose(fp); return 0; }
    len = (long)fread(raw, 1, (size_t)len, fp);
    fclose(fp);
    while (len % 5) raw[len++] = 0;
    n = (int)(len / 5);
    *wp = malloc((size_t)(n ? n : 1) * sizeof(w36));
    for (i = 0; i < n; i++) {
        unsigned char *f = raw + i * 5;
        (*wp)[i] = ((w36)f[0] << 28) | ((w36)f[1] << 20) | ((w36)f[2] << 12)
                 | ((w36)f[3] << 4)  | (w36)(f[4] & 017);
    }
    free(raw);
    *np = n;
    return 1;
}

static void file_write_disk(f10file_t *f)
{
    FILE *fp;
    int i;
    if (!f->dirty) return;
    fp = fopen(f->name, "wb");
    if (!fp) { warn("cannot write %s", f->name); return; }
    for (i = 0; i < f->nwords; i++) {
        w36 w = f->w[i];
        fputc((int)((w >> 28) & 0377), fp);
        fputc((int)((w >> 20) & 0377), fp);
        fputc((int)((w >> 12) & 0377), fp);
        fputc((int)((w >>  4) & 0377), fp);
        fputc((int)( w        & 017),  fp);
    }
    fclose(fp);
    f->dirty = 0;
}

/* Logical names.  The game defines its own with CRLNM% and then uses it
 * as a device; every file lives in the working directory here, so a
 * logical name means nothing more than "this is the disk". */
static char lognames[8][16];
static int nlognames;

void add_logical_name(const char *name)
{
    int i;
    for (i = 0; i < nlognames; i++)
        if (!strcmp(lognames[i], name)) return;
    if (nlognames < 8)
        snprintf(lognames[nlognames++], 16, "%s", name);
}

static int is_disk_device(const char *n)
{
    int i;
    if (!strncmp(n, "DSK", 3) || !strncmp(n, "SYS", 3) || !strncmp(n, "SRC", 3))
        return 1;
    for (i = 0; i < nlognames; i++)
        if (!strcmp(lognames[i], n)) return 1;
    return 0;
}

static const builtin_file_t *builtin(const char *name)
{
    const builtin_file_t *b;
    for (b = builtin_files; b->name; b++)
        if (!strcmp(b->name, name)) return b;
    return NULL;
}

/* LOOKUP: find an existing file.  Returns NULL if there is none. */
static f10file_t *file_open(const char *name)
{
    f10file_t *f;
    const builtin_file_t *b;
    w36 *w = NULL;
    int n = 0;
    if (!file_read_disk(name, &w, &n)) {
        b = builtin(name);
        if (!b) return NULL;
        n = b->nwords;
        w = malloc((size_t)(n ? n : 1) * sizeof(w36));
        memcpy(w, b->words, (size_t)n * sizeof(w36));
    }
    f = calloc(1, sizeof *f);
    snprintf(f->name, sizeof f->name, "%s", name);
    f->w = w; f->nwords = n; f->cap = n;
    return f;
}

/* ENTER: create (or truncate) a file. */
static f10file_t *file_create(const char *name)
{
    f10file_t *f = calloc(1, sizeof *f);
    snprintf(f->name, sizeof f->name, "%s", name);
    f->cap = BLKWORDS;
    f->w = calloc((size_t)f->cap, sizeof(w36));
    f->dirty = 1;
    return f;
}

static void file_close(f10file_t *f)
{
    if (!f) return;
    file_write_disk(f);
    free(f->w);
    free(f);
}

static void file_grow(f10file_t *f, int need)
{
    if (need <= f->cap) return;
    while (f->cap < need) f->cap = f->cap ? f->cap * 2 : BLKWORDS;
    f->w = realloc(f->w, (size_t)f->cap * sizeof(w36));
    memset(f->w + f->nwords, 0, (size_t)(f->cap - f->nwords) * sizeof(w36));
}

static w36 file_get(f10file_t *f, int i)
{
    return (i >= 0 && i < f->nwords) ? f->w[i] : 0;
}

static void file_put(f10file_t *f, int i, w36 v)
{
    if (i < 0) return;
    file_grow(f, i + 1);
    if (i >= f->nwords) f->nwords = i + 1;
    f->w[i] = v;
    f->dirty = 1;
}

/* ------------------------------------------------------------------ */
/* channels                                                             */
/* ------------------------------------------------------------------ */
#define NCHAN 16

/* Buffer sizes, in words, as reported by DEVSIZ and used to build the
 * rings.  Two words of each buffer are the link and the byte count. */
#define TTYBUF 0023
#define DSKBUF 0203

/* File status, as GETSTS reports it and SETSTS sets it.  These live in
 * the right half of the word, together with the data mode in the bottom
 * four bits -- FOROTS clears IO_ACT with TRZ at 0414741, which only
 * reaches the right half, and STATO and STATZ take an 18 bit mask.  Put
 * them in the left half instead and end of file never gets through: the
 * program reads to the end of the last buffer and then loops asking for
 * a record that never comes. */
#define IO_IMP 0400000                /* improper mode                */
#define IO_DER 0200000                /* device error                 */
#define IO_DTE 0100000                /* data error                   */
#define IO_BKT 0040000                /* block too large              */
#define IO_EOF 0020000                /* end of file                  */
#define IO_ACT 0010000                /* device active                */

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
    int  isdsk;
    int  entered;                /* opened for output by ENTER         */
    f10file_t *file;             /* the disk file, if this is one      */
    int  pos;                    /* next word of it to transfer        */
    int  eof;
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

/* The pointer the monitor leaves in the second word of a buffer header.
 *
 * In ASCII it is an ordinary seven bit byte pointer positioned before
 * the first data word, so the program's first ILDB picks up the first
 * character: P=36, S=7, address buf+2.
 *
 * In the word oriented modes the byte is the whole word, and the same
 * "one before the first" rule puts the address at buf+1 with P=0 and
 * S=36 -- an ILDB there takes P below zero, resets it and steps the
 * address to buf+2.  FOROTS does not even ILDB: at 0402130 it does
 *
 *      AOS  0,6(14)          step the pointer
 *      MOVE 0,@6(14)         and take the word it now addresses
 *
 * which needs that same address one short of the data.  Getting this
 * wrong reads every binary record starting one word in, so the FORTRAN
 * record control word is missed and the record looks like nonsense. */
static int word_mode(int mode) { return mode != 0 && mode != 1; }

static w36 make_bufptr(int mode, int buf)
{
    return word_mode(mode) ? XWD(0004400, buf + 1) : XWD(0440700, buf + 2);
}

/* How many data words this buffer holds.  The link word of a buffer in
 * a ring is XWD(length, next buffer), where length counts every word of
 * the buffer except the link itself -- so one for the count word plus
 * the data.  FOROTS lays its own rings out after asking DEVSIZ, so read
 * the size back from the ring rather than assuming the one we would
 * have built. */
static int bufwords(int buf, int dflt)
{
    int n = (int)(LH(M[buf]) & 0377777);
    return (n > 1) ? n - 1 : dflt;
}

/* How much the program put in the buffer, in the mode's own bytes.
 *
 * Read it off the byte pointer in the second word of the header, which
 * is where the program has got to.  The count in the third word says how
 * much room is left, and taking the buffer size less that gives the same
 * answer -- until the very first OUT on a channel, where FOROTS issues
 * the transfer before it has set the header up at all.  Both words are
 * then zero, "no room left" reads as "full", and a whole buffer of
 * nothing gets written.  On the terminal that is invisible, because it
 * is all nulls; in a file it is a block of zeros in front of the data,
 * which is enough to make the saved game unreadable. */
static int deposited(int mode, int hdr, int buf)
{
    w36 bp = M[hdr + 1];
    int addr = (int)RH(bp);
    int p = (int)((bp >> 30) & 077);
    int s = (int)((bp >> 24) & 077);

    if (!bp || !addr) return 0;
    if (word_mode(mode)) return addr - (buf + 1);
    if (s == 0) return 0;
    return (addr - (buf + 2)) * (36 / s) + (36 - p) / s;
}

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

static void set_hdr_out(int mode, int hdr, int buf, int nwords)
{
    M[hdr]     = XWD(0, buf);
    M[hdr + 1] = make_bufptr(mode, buf);
    M[hdr + 2] = (w36)(word_mode(mode) ? nwords : nwords * 5);
    M[buf + 1] = 0;
}

static void ensure_in_ring(int c);
static void ensure_out_ring(int c);

/* ------------------------------------------------------------------ */
static void do_out(int c)
{
    chan_t *ch = &chan[c];
    int hdr = ch->obufhdr, buf, nb, i;
    w36 bp;

    int nw;

    if (!hdr) return;
    ensure_out_ring(c);
    buf = (int)RH(M[hdr]);
    if (!buf) return;
    nw = bufwords(buf, ch->obufwords);
    nb = deposited(ch->mode, hdr, buf);
    if (opt_verbose > 2) {
        int k;
        fflush(stdout);
        fprintf(stderr, "[uuo] OUT ch%o %d bytes <", c, nb);
        for (k = 0; k < nb && k < 90; k++) {
            int ch2 = (int)((M[buf + 2 + k / 5] >> (29 - 7 * (k % 5))) & 0177);
            if (ch2 >= 32 && ch2 < 127) fputc(ch2, stderr);
            else fprintf(stderr, "<%o>", ch2);
        }
        fprintf(stderr, ">\n");
    }
    if (nb < 0) nb = 0;

    if (ch->file) {
        /* The buffer out to the file.  nb is the count in the mode's own
         * bytes; a word holds five of them in ASCII and is one of them
         * in every other mode.
         *
         * Only a file opened by ENTER may be written.  FOROTS gives a
         * channel one ring and points both headers at it, so the buffer
         * a LOOKUP-only channel is "holding" is the one the last read
         * filled -- flush that and a read-only database gets a copy of
         * its own contents written back over itself. */
        if (!ch->entered) {
            M[buf] &= ~((w36)0400000 << 18);
            buf = (int)RH(M[buf]);
            set_hdr_out(ch->mode, hdr, buf, bufwords(buf, ch->obufwords));
            return;
        }
        int nwrite = word_mode(ch->mode) ? nb : (nb + 4) / 5;
        if (nwrite > nw) nwrite = nw;
        for (i = 0; i < nwrite; i++)
            file_put(ch->file, ch->pos++, M[buf + 2 + i]);
        warn("OUT ch%o <- %s, %d words", c, ch->file->name, nwrite);
        M[buf] &= ~((w36)0400000 << 18);
        buf = (int)RH(M[buf]);
        set_hdr_out(ch->mode, hdr, buf, bufwords(buf, ch->obufwords));
        return;
    }

    bp = make_bp(buf + 2);
    for (i = 0; i < nb; i++) {
        int byte = bp_peek(&bp);
        if (ch->istty) tty_out(byte);
        else if (ch->fp) fputc(byte & 0177, ch->fp);
    }
    if (ch->istty) fflush(stdout);

    /* advance the ring */
    buf = (int)RH(M[buf]);
    set_hdr_out(ch->mode, hdr, buf, bufwords(buf, ch->obufwords));
}

static int do_in(int c)
{
    chan_t *ch = &chan[c];
    int hdr = ch->ibufhdr, buf, i, n = 0, nwords;
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
    nwords = bufwords(buf, ch->ibufwords);
    for (i = 0; i < nwords; i++)
        M[buf + 2 + i] = 0;
    if (ch->file) {
        /* A block of the file into the next buffer of the ring.  The
         * count in the buffer and in the header is in bytes of the
         * channel's own data mode -- five to a word for ASCII, one for
         * everything else -- and the use bit in the buffer's link word
         * says the buffer now holds data. */
        int nw = nwords, nbytes;
        int ascii = !word_mode(ch->mode);
        if (ch->pos >= ch->file->nwords) {
            M[buf + 1] = 0;
            M[hdr]     = XWD(LH(M[hdr]), buf);
            M[hdr + 1] = make_bufptr(ch->mode, buf);
            M[hdr + 2] = 0;
            ch->status |= IO_EOF;
            return 0;
        }
        if (ch->pos + nw > ch->file->nwords) nw = ch->file->nwords - ch->pos;
        for (i = 0; i < nw; i++)
            M[buf + 2 + i] = file_get(ch->file, ch->pos++);
        nbytes = ascii ? nw * 5 : nw;
        M[buf]     |= (w36)0400000 << 18;              /* use bit */
        M[buf + 1]  = XWD(0, nbytes);
        M[hdr]      = XWD(LH(M[hdr]) | 0400000, buf);
        M[hdr + 1]  = make_bufptr(ch->mode, buf);
        M[hdr + 2]  = (w36)nbytes;
        warn("IN ch%o -> buf %06o, %d words, %d bytes", c, buf, nw, nbytes);
        return 1;
    }
    if (ch->istty) {
        int limit = nwords * 5;
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
        int limit = nwords * 5;
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

    M[buf + 1] = XWD(0, n);
    M[hdr]     = XWD(0, buf);
    M[hdr + 1] = make_bufptr(ch->mode, buf);
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
/* GETSEG: bring the FORTRAN object time system in as the high segment.
 * The pages are FOROTS.EXE own, at the addresses TOPS-20 gave them. */
void monitor_getseg(void)
{
    const w36 *p = forots_data;
    while (p[0] || p[1]) {
        int addr = (int)p[0], n = (int)p[1], k;
        p += 2;
        for (k = 0; k < n; k++)
            M[(addr + k) & (MEMTOP - 1)] = p[k];
        p += n;
    }
    /* .JBHRL: the high segment now runs 0400000 through the last word
     * mapped.  FOROTS looks at the right half to find its own top. */
    M[0115] = XWD(0400000, 0425777);
}

/* ------------------------------------------------------------------ */
void monitor_init(void)
{
    if (opt_echo < 0)
        opt_echo = isatty(fileno(stdin)) ? 0 : 1;
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
              if (chan[i].file) file_close(chan[i].file);
              if (chan[i].fp) fclose(chan[i].fp);
              memset(&chan[i], 0, sizeof chan[i]); } }
        return 0;
    case 004:                                            /* DEVCHR     */
        { char n[8]; sixbit_str(AC(ac), n);
          if (strncmp(n, "TTY", 3) == 0) AC(ac) = opt_devchr_tty;
          else if (is_disk_device(n)) AC(ac) = opt_devchr_dsk;
          else AC(ac) = 0;
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
    case 040:                                            /* GETSEG     */
        /* MOVEI 1,arg / CALLI 1,40 with SIXBIT "SYS","FOROTS" in the
         * argument block: the game asking for the FORTRAN object time
         * system as its high segment.  On the real machine PA1050 turned
         * this into a PMAP of <SUBSYS>FOROTS.EXE pages 2-27 at 0400000;
         * that is what forots_data holds, so map it and skip. */
        { char n[8]; sixbit_str(M[(int)RH(AC(ac)) + 1], n);
          if (strcmp(n, "FOROTS") != 0)
              warn("GETSEG of %s -- only FOROTS is built in", n);
          monitor_getseg();
          return 1; }
    case 041: {                                          /* GETTAB     */
        /* XWD item, table.  FOROTS asks for one thing before it will
         * run at all: item 17 of table 11, the configuration word whose
         * low left-half bit says the monitor supports FOROTS.  It is the
         * bit PA1050 set on the real system; say yes to it, and say
         * "no such table" to everything else rather than invent data. */
        int item = (int)LH(AC(ac)), tab = (int)RH(AC(ac));
        warn("GETTAB item %o table %o", item, tab);
        if (tab == 011 && item == 017) { AC(ac) = 0751317000000ULL; return 1; }
        AC(ac) = 0;
        return 0; }
    case 043: return 1;                                  /* SETUUO     */
    case 053: {                                          /* DEVTYP     */
        char n[8]; sixbit_str(AC(ac), n);
        if (strncmp(n, "TTY", 3) == 0) AC(ac) = opt_devtyp_tty;
        else if (is_disk_device(n))    AC(ac) = opt_devtyp_dsk;
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
    ch->isdsk = !ch->istty && !ch->isnull;
    ch->obufhdr = (int)LH(bufword);
    ch->ibufhdr = (int)RH(bufword);
    ch->status = (w36)(mode & 017);
    /* Data words per buffer.  A terminal buffer is whatever is left of
     * the standard size after the link and count words; a disk buffer
     * holds exactly one block, which is what USETI and USETO count in. */
    ch->ibufwords = ch->istty ? TTYBUF - 2 : BLKWORDS;
    ch->obufwords = ch->istty ? TTYBUF - 2 : BLKWORDS;
    /* Nothing has been put in the output buffer yet, so say so.  The
     * game reuses one channel for all of its databases in turn, and a
     * byte pointer left in the header by whichever file had it last
     * would otherwise be read as a part-filled buffer and written out
     * when this file is closed -- which is how a read-only database
     * ends up copied into the player's directory. */
    if (ch->obufhdr) M[ch->obufhdr + 1] = 0;
    warn("OPEN channel %o device %s mode %o in=%06o out=%06o",
         c, name, mode & 017, ch->ibufhdr, ch->obufhdr);
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
    M[ch->ibufhdr + 1] = make_bufptr(ch->mode, ch->ibuf);
    M[ch->ibufhdr + 2] = 0;
}

static void ensure_out_ring(int c)
{
    chan_t *ch = &chan[c];
    if (!ch->obufhdr || RH(M[ch->obufhdr])) return;
    make_ring(2, ch->obufwords, &ch->obuf);
    set_hdr_out(ch->mode, ch->obufhdr, ch->obuf, ch->obufwords);
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
    if (opt_verbose > 3 && (op == 076 || op == 077)) {
        int i;
        fflush(stdout);
        fprintf(stderr, "[blk] %06o:", e);
        for (i = 0; i < 8; i++)
            fprintf(stderr, " %012llo", (unsigned long long)M[e + i]);
        fputc(10, stderr);
    }
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
            M[ch->ibufhdr + 1] = make_bufptr(ch->mode, ch->ibuf);
            M[ch->ibufhdr + 2] = 0;
        }
        return 0; }

    case 065: {                                          /* OUTBUF     */
        int c = ac & 017;
        chan_t *ch = &chan[c];
        if (ch->obufhdr && !ch->obuf) {
            make_ring(e ? e : 2, ch->obufwords, &ch->obuf);
            set_hdr_out(ch->mode, ch->obufhdr, ch->obuf, ch->obufwords);
        }
        return 0; }

    /* IN and OUT take their error return by skipping, which is the
     * opposite way round from LOOKUP and ENTER.  FOROTS says so plainly:
     * it executes the transfer with an XCT and lays the two returns out
     *
     *      402373  XCT   0,0            the OUT
     *      402374  JRST  402411         carry on
     *      402375  XCT   4,413623       report an error
     *
     * and the same at 402160 for IN, so a skip past the first of those
     * is what sends it to the error reporter.  INPUT and OUTPUT are the
     * versions without an error return and never skip. */
    case 056:                                            /* IN         */
        return do_in(ac & 017) ? 0 : 1;
    case 066:                                            /* INPUT      */
        do_in(ac & 017);
        return 0;
    case 057:                                            /* OUT        */
        do_out(ac & 017);
        return 0;
    case 067:                                            /* OUTPUT     */
        do_out(ac & 017);
        return 0;

    case 070: {                                          /* CLOSE      */
        chan_t *ch = &chan[ac & 017];
        /* Flush whatever is still in the output buffer.  With the
         * count taken from the header byte pointer an empty buffer
         * writes nothing, so this is safe on a channel that has only
         * been read. */
        if (ch->obufhdr) do_out(ac & 017);
        if (ch->file) { file_close(ch->file); ch->file = NULL; }
        if (ch->fp) { fclose(ch->fp); ch->fp = NULL; }
        ch->status &= ~IO_EOF;
        return 0; }

    case 071: {                                          /* RELEAS     */
        chan_t *ch = &chan[ac & 017];
        if (ch->file) { file_close(ch->file); ch->file = NULL; }
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

    case 076: {                                          /* LOOKUP     */
        chan_t *ch = &chan[ac & 017];
        int nb = fnameblock(e);
        char name[32];
        if (ch->istty || ch->isnull) return 1;
        fname(name, nb);
        if (ch->file) { file_close(ch->file); ch->file = NULL; }
        ch->file = file_open(name);
        ch->pos = 0;
        ch->eof = 0;
        ch->entered = 0;
        if (!ch->file) {
            warn("LOOKUP %s -- not found", name);
            /* Error 0 is ERFNF, file not found -- which is what a
             * program opening a file for output wants to hear.  Error 1
             * is a missing directory, and FOROTS gives up on that one
             * and starts asking the player for new file specifications. */
            M[nb + 1] = M[nb + 1] & ~(w36)HMASK;
            return 0;
        }
        warn("LOOKUP %s -- %d words", name, ch->file->nwords);
        if (nb != e) {                        /* extended block: fill it in */
            if (RH(M[e]) > 5) M[nb + 3] = (w36)ch->file->nwords;
            if (RH(M[e]) > 4) M[nb + 2] = XWD(0777752, 0);
        }
        ch->status &= ~IO_EOF;
        return 1; }

    case 077: {                                          /* ENTER      */
        /* Create a file -- or, if the same file is already open on this
         * channel from a LOOKUP, take it for updating rather than
         * starting it again.  That is the TOPS-10 idiom for rewriting a
         * file in place, and it is how the game keeps its logs. */
        chan_t *ch = &chan[ac & 017];
        int nb = fnameblock(e);
        char name[32];
        if (ch->istty || ch->isnull) return 1;
        fname(name, nb);
        if (ch->file && strcmp(ch->file->name, name) != 0) {
            file_close(ch->file);
            ch->file = NULL;
        }
        if (!ch->file) ch->file = file_create(name);
        ch->pos = 0;
        ch->eof = 0;
        ch->entered = 1;
        ch->file->dirty = 1;
        warn("ENTER %s", name);
        ch->status &= ~IO_EOF;
        return 1; }

    case 055:                                            /* RENAME     */
        return 0;
    case 072:                                            /* MTAPE      */
        return 0;
    case 073: {                                          /* UGETF      */
        chan_t *ch = &chan[ac & 017];
        AC(ac) = ch->file ? (w36)(ch->file->nwords / BLKWORDS + 1) : 1;
        return 0; }

    case 074:                                            /* USETI      */
    case 075: {                                          /* USETO      */
        /* Position to a block.  Blocks are numbered from 1 and hold 128
         * words; this is how FOROTS reaches a FORTRAN random access
         * record. */
        chan_t *ch = &chan[ac & 017];
        int blk = e ? e : 1;
        ch->pos = (blk - 1) * BLKWORDS;
        ch->status &= ~IO_EOF;
        if (ch->ibufhdr && RH(M[ch->ibufhdr]))
            M[ch->ibufhdr + 2] = 0;
        return 0; }

    default:
        if (op >= 0700) {
            warn("user mode I/O instruction %03o at %06o", op, (PC - 1) & HMASK);
            return 0;
        }
        warn("unimplemented UUO %03o ac=%o e=%06o", op, ac, e);
        return 1;
    }
}
