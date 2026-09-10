/* monitor.c -- the slice of the TOPS-10 / TYMCOM-X monitor that ZARAST needs.
 *
 * Started life in the EXPLOR port, where it answered FOROTS.  Here the
 * caller is the TBA (Tymshare BASIC) object time system, which wants the
 * same terminal I/O and the same handful of CALLIs but also, unlike
 * EXPLOR, real files: the game keeps its world in NEWADV.DAT, each player
 * in a .ADV file and each player's game in SCENAR.IO, and the five
 * programs hand off to each other with the RUN UUO.
 */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
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
int  opt_echo_input = -1;        /* -1 = decide from isatty            */

/* LDLLCT, "LOWER CASE TO UPPER CASE" -- the TYMCOM-X terminal line flag
 * that makes the scanner fold what you type before the program ever sees
 * it (SCNSER: "TLNE U,LDLLCT / SUBI T3,40 / HERE IN LOWER CASE, CONVERT
 * TO UPPER").  It was on for the terminals these programs were written
 * for, and the programs assume it: DUNGEN compares your answer against
 * "Y", so a typed y falls through to the no branch and the question
 * about rolling up a character can never be answered yes.  TBA reads the
 * line characteristics once with GETLCH and never sets them, so the
 * setting is the terminal's, and here it is ours. */
int  opt_lcfold = 1;
int  opt_faketime = -1;          /* minutes past midnight, or -1       */
const char *opt_dir = ".";       /* where the game's data files live   */

/* Set by the RUN UUO; main.c loads this image next.  On Tymshare the
 * programs really did chain -- CHARAC ends by running VENTUR, VENTUR
 * runs DUNGEN when you go down the stairs and DUNGEN runs VENTUR when
 * you come back up -- so the handoff has to survive here too. */
char run_next[16];

/* TBA opens a scratch file called COM.<port number> at startup and
 * leaves it behind.  On Tymshare that was the monitor's business; here
 * it is litter in the player's save directory, so remember it and take
 * it away again. */
static char scratch[512];

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

/* On TYMCOM-X the monitor echoed what you typed, so the line you entered
 * was part of the transcript and supplied the newline the program never
 * printed.  A Windows console echoes for us when someone is really
 * typing; when input is piped nothing else does, so echo it here and
 * transcripts read the way the terminal looked. */
static int tty_in(void)
{
    int c;
    if (tipos >= tilen && !tty_fill())
        return 032;                          /* ^Z at end of input     */
    c = tibuf[tipos++];
    if (opt_lcfold && c >= 'a' && c <= 'z') c -= 040;
    if (opt_echo_input) tty_out(c);
    return c;
}

static int tty_avail(void) { return tipos < tilen; }

/* the line most recently handed to the program, for echo suppression */
static unsigned char lastin[1024];
static int lastinlen;

static void warn(const char *fmt, ...);
static w36 str_sixbit(const char *s, int n);
static w36 date_word(void);

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
    /* disk files -- held as whole word arrays, see dsk_load()         */
    int  isdsk;
    char fname[32];              /* "NEWADV.DAT"                       */
    w36 *fw;
    int  flen, fcap;             /* words used / allocated             */
    int  fpos;                   /* next word to read or write         */
    int  fout;                   /* opened by ENTER rather than LOOKUP */
    int  fdirty;
    int  isufd;                  /* synthesised directory, never written */
} chan_t;

static chan_t chan[NCHAN];

/* ------------------------------------------------------------------ */
/* disk files                                                           */
/*                                                                      */
/* A TOPS-10 disk file is an array of 36-bit words, and the programs use
 * it as one: NEWADV.DAT is 13,529 single-precision floats laid end to
 * end, and USETI/USETO index it by 128-word block.  So a file is held
 * here as a word array, read in whole at LOOKUP and written back at
 * CLOSE, which makes random access and rewriting the same operation.
 *
 * On the host the words are stored the way the Tymshare tape stores
 * them, and for the same reason.  A binary file becomes five bytes per
 * word -- five 7-bit septets, the 36th bit in the high bit of the fifth
 * byte; a file opened in an ASCII mode becomes plain bytes with the
 * trailing nulls of the last word trimmed.  Both match the originals
 * exactly, so NEWADV.DAT written by this port is byte-for-byte the same
 * kind of file as the NEWADV.DAT that came off the tape, and the
 * characters other Tymshare users left behind in novafield/ can be
 * dropped straight into a save directory and played.                   */

static int mode_is_ascii(int m) { return m == 0 || m == 1; }

static void dsk_path(char *out, size_t n, const char *fname)
{
    size_t i;
    snprintf(out, n, "%s/%s", opt_dir, fname);
    for (i = strlen(opt_dir) + 1; i < strlen(out); i++)
        out[i] = (char)tolower((unsigned char)out[i]);
}

static void dsk_room(chan_t *ch, int need)
{
    if (need <= ch->fcap) return;
    ch->fcap = need < 1024 ? 1024 : need * 2;
    ch->fw = (w36 *)realloc(ch->fw, (size_t)ch->fcap * sizeof(w36));
    if (!ch->fw) fatal("out of memory for file %s", ch->fname);
}

/* SIXBIT for a filename component, left justified, blank padded. */
static w36 str_sixbit(const char *s, int n)
{
    w36 w = 0;
    int i;
    for (i = 0; i < n; i++) {
        int c = s[i] ? toupper((unsigned char)s[i]) : ' ';
        if (!s[i]) c = ' ';
        w |= (w36)((c - 040) & 077) << (30 - 6 * i);
        if (!s[i]) continue;
    }
    return w;
}

/* Build the user file directory.
 *
 * DUNGEN will not start until it has satisfied itself that NEWADV.DAT
 * and SCENAR.IO are both there -- "SORRY YOU MUST HAVE TWO FILES ON
 * YOUR SYSTEM TO PLAY / RUN FILER.SHR" -- and the way it looks is the
 * way a TOPS-10 program looked: it opens the directory itself, the file
 * called <ppn>.UFD, and reads it.  There is no such file here, so make
 * one out of what is in the save directory.  A UFD is a plain list of
 * two-word entries, the SIXBIT filename and then the SIXBIT extension
 * in the left half of the next word, and a zero word ends it. */
static int dsk_load_ufd(chan_t *ch)
{
    DIR *d = opendir(opt_dir);
    struct dirent *de;
    if (!d) return 0;
    ch->flen = 0;
    dsk_room(ch, 256);
    while ((de = readdir(d)) != NULL) {
        char base[16], ext[8];
        const char *dot = strrchr(de->d_name, '.');
        size_t nb;
        if (de->d_name[0] == '.') continue;
        nb = dot ? (size_t)(dot - de->d_name) : strlen(de->d_name);
        if (nb > 6) nb = 6;
        memset(base, 0, sizeof base);
        memcpy(base, de->d_name, nb);
        memset(ext, 0, sizeof ext);
        if (dot) { strncpy(ext, dot + 1, 3); }
        dsk_room(ch, ch->flen + 4);
        ch->fw[ch->flen++] = str_sixbit(base, 6);
        ch->fw[ch->flen++] = str_sixbit(ext, 3) & ~(w36)HMASK;
    }
    closedir(d);
    dsk_room(ch, ch->flen + 2);
    ch->fw[ch->flen++] = 0;
    ch->fw[ch->flen++] = 0;
    ch->fpos = 0;
    ch->fdirty = 0;
    ch->isufd = 1;
    return ch->flen > 2;
}

/* Read the host file into ch->fw.  Returns 0 if it is not there. */
static int dsk_load(chan_t *ch)
{
    char path[512];
    FILE *f;
    long sz;
    unsigned char *raw;
    int i;

    dsk_path(path, sizeof path, ch->fname);
    f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    raw = (unsigned char *)malloc((size_t)sz + 8);
    if (!raw) fatal("out of memory reading %s", path);
    sz = (long)fread(raw, 1, (size_t)sz, f);
    fclose(f);
    memset(raw + sz, 0, 8);

    if (mode_is_ascii(ch->mode)) {
        ch->flen = (int)((sz + 4) / 5);
        dsk_room(ch, ch->flen + 1);
        for (i = 0; i < ch->flen; i++) {
            const unsigned char *b = raw + (long)i * 5;
            ch->fw[i] = ((w36)(b[0] & 0177) << 29) | ((w36)(b[1] & 0177) << 22)
                      | ((w36)(b[2] & 0177) << 15) | ((w36)(b[3] & 0177) <<  8)
                      | ((w36)(b[4] & 0177) <<  1);
        }
    } else {
        /* Trailing nulls of the last word are trimmed on the way out --
         * that is why novafield/scenar.io is 4602 bytes and not 4600 --
         * so round up rather than losing the tail. */
        ch->flen = (int)((sz + 4) / 5);
        dsk_room(ch, ch->flen + 1);
        for (i = 0; i < ch->flen; i++) {
            const unsigned char *b = raw + (long)i * 5;
            ch->fw[i] = ((w36)(b[0] & 0177) << 29) | ((w36)(b[1] & 0177) << 22)
                      | ((w36)(b[2] & 0177) << 15) | ((w36)(b[3] & 0177) <<  8)
                      | ((w36)(b[4] & 0177) <<  1) | ((w36)(b[4] >> 7) & 1);
        }
    }
    free(raw);
    ch->fpos = 0;
    return 1;
}

static void dsk_store(chan_t *ch)
{
    char path[512];
    FILE *f;
    int i, k;
    if (!ch->fdirty || ch->isufd) return;
    dsk_path(path, sizeof path, ch->fname);
    f = fopen(path, "wb");
    if (!f) { warn("cannot write %s", path); return; }
    if (mode_is_ascii(ch->mode)) {
        long n = (long)ch->flen * 5;
        unsigned char *raw = (unsigned char *)malloc((size_t)n + 1);
        for (i = 0; i < ch->flen; i++)
            for (k = 0; k < 5; k++)
                raw[(long)i * 5 + k] =
                    (unsigned char)((ch->fw[i] >> (29 - 7 * k)) & 0177);
        while (n > 0 && raw[n - 1] == 0) n--;      /* pad of the last word */
        fwrite(raw, 1, (size_t)n, f);
        free(raw);
    } else {
        for (i = 0; i < ch->flen; i++) {
            w36 w = ch->fw[i];
            for (k = 0; k < 5; k++)
                fputc((int)((w >> (29 - 7 * k)) & 0177) |
                      (k == 4 ? (int)((w & 1) << 7) : 0), f);
        }
    }
    fclose(f);
    ch->fdirty = 0;
}

static void dsk_close(chan_t *ch)
{
    if (ch->isdsk) {
        dsk_store(ch);
        free(ch->fw);
        ch->fw = NULL; ch->flen = ch->fcap = ch->fpos = 0;
    }
}

static w36 dsk_get(chan_t *ch)
{
    if (ch->fpos >= ch->flen) return 0;
    return ch->fw[ch->fpos++];
}

static void dsk_put(chan_t *ch, w36 w)
{
    dsk_room(ch, ch->fpos + 1);
    while (ch->flen < ch->fpos) ch->fw[ch->flen++] = 0;
    ch->fw[ch->fpos++] = w;
    if (ch->fpos > ch->flen) ch->flen = ch->fpos;
    ch->fdirty = 1;
}

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

/* Dump mode (data modes 14-17) does not use buffers at all: the UUO's
 * effective address points at a list of IOWDs, <-count,,addr-1>, and the
 * transfer runs straight between core and the file.  TBA reads and writes
 * NEWADV.DAT this way. */
static int do_dump(int c, int e, int out)
{
    chan_t *ch = &chan[c];
    w36 cmd;
    int a;
    if (!ch->isdsk) return 1;
    for (a = e; a; ) {
        cmd = M[a & HMASK];
        if (cmd == 0) break;
        if (LH(cmd) == 0) { a = (int)RH(cmd); continue; }   /* GOTO word */
        {
            int n = 01000000 - (int)LH(cmd);                /* -count    */
            int p = (int)RH(cmd);
            int i;
            if (opt_verbose > 1)
                warn("dump %s ch%o %d words at core %06o, file word %d of %d",
                     out ? "OUT" : "IN", c, n, p + 1, ch->fpos, ch->flen);
            for (i = 0; i < n; i++) {
                if (out) dsk_put(ch, M[(p + 1 + i) & HMASK]);
                else {
                    if (ch->fpos >= ch->flen) { ch->status |= IO_EOF; return 0; }
                    M[(p + 1 + i) & HMASK] = dsk_get(ch);
                }
            }
        }
        a++;
    }
    return 1;
}

/* CHANIO function 33, read the user file directory.
 *
 * This is how DUNGEN finds out whether NEWADV.DAT and SCENAR.IO are
 * there, so it has to work or the game will not start.  The calling
 * sequence is the monitor's, from UFDUUO in the TYMCOM-X sources:
 *
 *   E+0  AOBJN pointer <-words,,address> into the caller's buffer
 *   E+1  filename to match, SIXBIT
 *   E+2  extension to match, SIXBIT in the left half
 *   E+3  flags: negative enables the * and # wildcards, and bits 1-4
 *        ask for the block count, licence, date and protection to be
 *        returned after each entry
 *   E+4  where the search got to; zero to start
 *   E+5  the last name returned, and E+6 its extension
 *   E+7  how many entries were returned
 *
 * Each entry stores two words -- the SIXBIT name, then the extension in
 * the left half of the next word -- and the skip return means at least
 * one was found.
 */
static w36 aobj(w36 p)
{
    return ((p + XWD(1, 1)) & WMASK);
}

/* One field of a wildcard match: * matches anything, # matches any one
 * character, so both come back as a mask of bits to ignore. */
static w36 ufd_mask(w36 pat)
{
    w36 m = 0, x;
    int i;
    if (pat == str_sixbit("*", 6)) return WMASK;
    x = pat ^ str_sixbit("######", 6);
    for (i = 0; i < 6; i++) {
        w36 f = (w36)077 << (30 - 6 * i);
        if (!(x & f)) m |= f;
    }
    return m;
}

static int do_ufdread(int c, int e)
{
    chan_t *ch = &chan[c];
    w36 p = M[e], wname = M[e + 1], wext = M[e + 2] & ~(w36)HMASK;
    w36 flags = M[e + 3], nmask = 0, xmask = 0;
    int start = (int)RH(M[e + 4]), i, n = 0;

    if (!ch->isufd) return 0;
    if (flags & SIGNBIT) {
        nmask = ufd_mask(wname);
        xmask = ufd_mask(wext >> 18) << 18;
    }
    M[e + 7] = 0;
    for (i = start * 2; i + 1 < ch->flen; i += 2) {
        w36 nam = ch->fw[i], ext = ch->fw[i + 1] & ~(w36)HMASK;
        if (!nam && !ext) break;
        if ((nam & ~nmask) != (wname & ~nmask)) continue;
        if ((ext & ~xmask) != (wext & ~xmask)) continue;
        p = aobj(p);
        if (!(p & SIGNBIT)) break;                   /* buffer full     */
        M[(RH(p) - 1) & HMASK] = nam;
        M[RH(p)] = ext;
        /* The optional trailers.  Nothing here keeps a block count, a
         * licence, a creation date or a protection code, but the caller
         * decides the stride, so the slots have to be filled. */
        if (flags & 0200000000000ULL) { p = aobj(p); if (!(p & SIGNBIT)) break;
                                        M[RH(p)] = 0; }
        if (flags & 0100000000000ULL) { p = aobj(p); if (!(p & SIGNBIT)) break;
                                        M[RH(p)] = 0; }
        if (flags & 0040000000000ULL) { p = aobj(p); if (!(p & SIGNBIT)) break;
                                        M[RH(p)] = date_word(); }
        if (flags & 0020000000000ULL) { p = aobj(p); if (!(p & SIGNBIT)) break;
                                        M[RH(p)] = 0755; }
        M[e + 5] = nam;
        M[e + 6] = ext;
        M[e + 7] = (w36)(++n);
        p = aobj(p);
        if (!(p & SIGNBIT)) break;
    }
    M[e + 4] = XWD(0, (i / 2));
    if (opt_verbose) warn("READ UFD: returned %d entries", n);
    return n ? 1 : 0;
}

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

    if (ch->isdsk) {
        /* A buffered write hands over whole words; the byte count says how
         * many of the last word's bytes are real, and trailing nulls in the
         * final word are trimmed when the file reaches the host. */
        int nw = mode_is_ascii(ch->mode) ? (nb + 4) / 5 : nb;
        for (i = 0; i < nw; i++) dsk_put(ch, M[buf + 2 + i]);
        buf = (int)RH(M[buf]);
        set_hdr_out(hdr, buf, ch->obufwords);
        return;
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
    } else if (ch->isdsk) {
        int want = ch->ibufwords, k;
        if (ch->fpos >= ch->flen) { ch->status |= IO_EOF; return 0; }
        if (want > ch->flen - ch->fpos) want = ch->flen - ch->fpos;
        for (k = 0; k < want; k++) M[buf + 2 + k] = dsk_get(ch);
        n = mode_is_ascii(ch->mode) ? want * 5 : want;
        M[buf + 1] = XWD(0, n);
        M[hdr]     = XWD(0, buf);
        M[hdr + 1] = make_bp(buf + 2);
        M[hdr + 2] = (w36)n;
        return 1;
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

/* Milliseconds the program has been told to wait but we did not really
 * wait for.  TBA's RND reads MSTIME, and the game rolls its dice with a
 * HIBER between each roll, so a clock that only advances in whole
 * seconds hands back the same "random" number six times -- a character
 * with 9/9/9/9/9/9 for its abilities.  Sleeping for real fixes it and
 * so does this, which is what -q does instead. */
static long vclock_ms;

static long mstime(void)
{
    struct timespec ts;
    long ms;
    if (opt_faketime >= 0) {
        ms = opt_faketime * 60L * 1000L;
    } else if (!timespec_get(&ts, TIME_UTC)) {
        struct tm *tm = nowtm();
        ms = ((tm->tm_hour * 60L + tm->tm_min) * 60L + tm->tm_sec) * 1000L;
    } else {
        struct tm *tm = nowtm();
        ms = ((tm->tm_hour * 60L + tm->tm_min) * 60L + tm->tm_sec) * 1000L
           + ts.tv_nsec / 1000000L;
    }
    return ms + vclock_ms;
}

/* Wait n milliseconds, or -- with -q -- just move the clock on. */
static void do_wait(long ms)
{
    if (ms <= 0) return;
    if (ms > 5000) ms = 5000;
    if (opt_delays) {
        fflush(stdout);
#ifdef _WIN32
        Sleep((DWORD)ms);
#else
        usleep((useconds_t)ms * 1000);
#endif
    } else {
        vclock_ms += ms;
    }
}

/* ------------------------------------------------------------------ */
void monitor_init(void)
{
    /* The echo-suppression below belongs to FOROTS, which carried the
     * record it had just read into the output buffer; TBA does not, and
     * writes to the terminal through TTCALL rather than through buffered
     * device I/O, so the path is never taken.  Leave it off. */
    if (opt_dedupe_echo < 0) opt_dedupe_echo = 0;
    if (opt_echo_input < 0)
        opt_echo_input = isatty(fileno(stdin)) ? 0 : 1;
    t0 = time(NULL);
    memset(chan, 0, sizeof chan);
}

void monitor_cleanup_scratch(void)
{
    if (scratch[0]) remove(scratch);
    scratch[0] = 0;
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

/* The negative CALLIs are Tymshare's own, and their names and calling
 * sequences come from the TYMCOM-X monitor sources that survive on the
 * same tape as the game (calstate/p034n.fdm, the P034 monitor edit).
 * The convention throughout is that the AC is both argument and result --
 * the monitor's W is the AC *number*, so its MOVEM T2,(W) writes back
 * into the AC -- and that success takes the skip return.
 *
 * Nothing here has anything to do with the game: the whole set is the
 * TBA runtime arranging for ^C and ^O to interrupt it, asking what kind
 * of terminal it has, and identifying itself to Tymshare's accounting.
 * A single-player port answers "fine, nothing was set before". */
static const char *negcalli_name(int n)
{
    static const char *t[] = {
        /* 0 */ 0, "LIGHTS", 0, 0, 0, 0, "ATTACH", "SETE",
        /*10 */ "SETLIC", "SETPRV", "POKE", "WAITCH", "REDNXT", "SETTMC",
                "GETTMC", "SETMAL",
        /*20 */ "ONEJOB", "SETJAL", "DSKCLR", "DISMIS", "SYSDVF", "RUNSEG",
                "SETMOD", "MOVBUF",
        /*30 */ "LEVDEF", "CHKLIC", "HANG", "INTADR", "INTENB", "INTACT",
                "INTASS", "SETTIM",
        /*40 */ "SETTR1", "SETTR2", "TINASS", "REDPIP", "CREAUX", "ZAPCIR",
                "AUXRED", "CRERMT",
        /*50 */ "ZAPRMT", "IDLRMT", "INTRMT", "VALRMT", "DDT620", "DATUUO",
                "TYMCHG", "SETRFC",
        /*60 */ "XCHARG", "CHPJC", "PUTSAR", "LSAUUO", "VREPLC", "VREMOV",
                "VCLEAR", "VCREAT",
        /*70 */ "VPROT", "VPGSTS", "PERSET", "REFBIT", "WSCTL", "PREREF",
                "VALPAG", "VFSTAT",
    };
    return (n >= 0 && n < (int)(sizeof t / sizeof *t) && t[n]) ? t[n] : "?";
}

/* Returns the number of words to skip past the UUO. */
static int do_negcalli(int ac, int n)
{
    switch (n) {
    /* Software interrupts.  Each of these hands back the previous
     * setting in the AC and skips; there was no previous setting. */
    case 033: case 034: case 035: case 036: case 040: case 041:
    case 042:                       /* INTADR INTENB INTACT INTASS  */
        AC(ac) = 0;                 /* SETTR1 SETTR2 TINASS         */
        return 1;
    case 023: return 0;             /* DISMIS -- no interrupt to leave */

    case 016: AC(ac) = 0; return 1; /* GETTMC -- terminal mode word  */
    case 015: return 1;             /* SETTMC                        */
    case 026: AC(ac) = 0; return 1; /* SETMOD                        */
    case 063: return 0;             /* LSAUUO -- accounting; the manual
                                     * in the monitor source says it
                                     * always returns here, AC intact. */
    case 011: return 0;             /* SETPRV                        */
    case 031: return 1;             /* CHKLIC                        */
    default:
        warn("unimplemented CALLI -%o (%s) ac%o=%012llo", n, negcalli_name(n),
             ac, (unsigned long long)AC(ac));
        return 1;                   /* the calls we do not know are all
                                     * "arrange something optional", so
                                     * take the success return */
    }
}

static int do_calli(int ac, int fn)
{
    switch (fn) {
    case 000:                                            /* RESET      */
        { int i; for (i = 0; i < NCHAN; i++) {
              if (chan[i].fp) fclose(chan[i].fp);
              dsk_close(&chan[i]);
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
    case 031:                                            /* SLEEP      */
        do_wait((long)(AC(ac) & 0777777) * 1000L);
        return 0;
    case 072:                                            /* HIBER      */
        /* AC holds flags in the left half and a millisecond timeout in
         * the right; nothing here will ever wake it early. */
        do_wait((long)RH(AC(ac)));
        return 1;
    case 073: return 1;                                  /* WAKE       */
    case 032: return 0;                                  /* SETPOV     */
    case 034: AC(ac) = 0; return 0;                      /* GETLIN     */
    case 036: return 1;                                  /* SETUWP     */
    case 041:                                            /* GETTAB     */
        if (opt_verbose > 1) warn("GETTAB %012llo", (unsigned long long)AC(ac));
        AC(ac) = 0;
        return 1;
    case 043: return 1;                                  /* SETUUO     */
    case 035: {                                          /* RUN        */
        /* AC holds XWD start-offset, address of a six-word argument
         * block: device, filename, XWD ext,0, zero, PPN, core.  The
         * programs chain among themselves with this, so hand the name
         * back to main.c and stop the processor. */
        int b = (int)RH(AC(ac));
        char name[8];
        sixbit_str(M[(b + 1) & HMASK], name);
        { int i; for (i = 0; name[i]; i++) name[i] = (char)tolower((unsigned char)name[i]); }
        snprintf(run_next, sizeof run_next, "%s", name);
        { int i; for (i = 0; i < NCHAN; i++) {
              if (chan[i].fp) fclose(chan[i].fp);
              dsk_close(&chan[i]);
              memset(&chan[i], 0, sizeof chan[i]); } }
        if (opt_verbose) warn("RUN %s", run_next);
        halted = 1;
        return 0; }
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
    case 006:                                            /* GETLCH     */
        /* Report the line as the port actually behaves.  The bits the
         * monitor hands back here are in the same positions it keeps
         * them in, so LDLLCT is 020 in the left half. */
        M[e] = opt_lcfold ? XWD(020, 0) : 0;
        return 0;
    case 007:                                            /* SETLCH     */
        opt_lcfold = (LH(M[e]) & 020) ? 1 : 0;
        return 0;
    case 010: return 0;                                  /* RESCAN     */
    case 011: tipos = tilen = 0; return 0;               /* CLRBFI     */
    case 012: fflush(stdout); return 0;                  /* CLRBFO     */
    case 013: return tty_avail() ? 1 : 0;                /* SKPINC     */
    case 014: return tty_avail() ? 1 : 0;                /* SKPINL     */
    case 015: tty_out(e & 0177); fflush(stdout); return 0;   /* IONEOU */
    case 016: tty_out(e & 0177); return 0;               /* OUTCHI     */
    case 017: {                                          /* OUTPTR     */
        /* Output an ASCIZ string through the byte pointer in C(E).
         * This is TBA's main way of writing to the terminal. */
        w36 bp = M[e];
        for (;;) {
            int c = bp_peek(&bp);
            if (!c) break;
            tty_out(c);
        }
        fflush(stdout);
        return 0; }
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
    (void)inst;                          /* the opcode and E are enough */
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
        if (sx18(e) < 0) return do_negcalli(ac, -sx18(e));
        return do_calli(ac, e);

    case 051:                                            /* TTCALL     */
        return do_ttcall(ac, e);

    /* Opcodes 42-46 are the five UUOs DEC left for each installation to
     * define; Tymshare used 42 for AUXCAL, 43 for CHANIO and 44 for
     * FRMOP.  CHANIO is the one that matters: it is every file UUO in
     * one instruction, with the function in the left half of the AC and
     * the channel in the right, so a program can pick its channel at run
     * time.  TBA uses it for all of its file work.  The function numbers
     * and their mapping onto the ordinary UUOs are from the monitor's
     * own CHNOTB dispatch table. */
    case 043: {                                          /* CHANIO     */
        static const short toop[] = {
            0071, 0070, 0067, 0066, 0076, 0077, 0074, 0075,   /*  0- 7 */
            0073, 0055, 0072, 0050, 0056, 0057, 0060, 0062,   /* 10-17 */
            0063, 0061, 0064, 0065                            /* 20-23 */
        };
        int fn   = (int)LH(AC(ac));
        int c    = (int)RH(AC(ac));
        if (c == (int)HMASK) {           /* -1: "pick a channel for me" */
            for (c = 1; c < NCHAN; c++) if (!chan[c].open) break;
            if (c >= NCHAN) return 0;
            AC(ac) = XWD(fn, c);
        }
        if (fn < (int)(sizeof toop / sizeof *toop))
            return monitor_uuo(0, toop[fn], c, e);
        switch (fn) {
        case 033: return do_ufdread(c, e);               /* READ UFD   */
        case 024: return 1;                              /* SEEK       */
        case 025: return 0;                              /* WAIT       */
        case 027:                                        /* full-word USETI */
        case 030: {                                      /* full-word USETO */
            chan_t *ch = &chan[c];
            int blk = (int)(M[e] & HMASK);
            if (ch->isdsk) ch->fpos = (blk ? blk - 1 : 0) * 0200;
            if (opt_verbose > 1)
                warn("CHANIO full-word USET%c ch%o block %d -> word %d of %d",
                     fn == 027 ? 'I' : 'O', c, blk, ch->fpos, ch->flen);
            return 0; }
        case 046:                                        /* NXTCHN     */
            return 1;
        default:
            warn("unimplemented CHANIO function %o on channel %o", fn, c);
            return 1;
        } }

    /* AUXCAL drives an auxiliary circuit -- a second terminal, or
     * another job's pseudo-terminal.  TBA probes for one around every
     * line of input and output; there is never going to be one here. */
    case 042:
        if (opt_verbose > 1) warn("AUXCAL ac=%o e=%06o -- no circuit", ac, e);
        return 1;
    case 044:                                            /* FRMOP      */
        if (opt_verbose > 1) warn("FRMOP ac=%o e=%06o -- ignored", ac, e);
        return 1;

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

    case 056: {                                          /* IN         */
        chan_t *ch = &chan[ac & 017];
        if (ch->isdsk && ch->mode >= 014) return do_dump(ac & 017, e, 0);
        return do_in(ac & 017) ? 1 : 0; }
    case 066: {                                          /* INPUT      */
        chan_t *ch = &chan[ac & 017];
        if (ch->isdsk && ch->mode >= 014) { do_dump(ac & 017, e, 0); return 0; }
        do_in(ac & 017);
        return 0; }
    case 057: {                                          /* OUT        */
        chan_t *ch = &chan[ac & 017];
        if (ch->isdsk && ch->mode >= 014) return do_dump(ac & 017, e, 1);
        do_out(ac & 017);
        return 1; }
    case 067: {                                          /* OUTPUT     */
        chan_t *ch = &chan[ac & 017];
        if (ch->isdsk && ch->mode >= 014) { do_dump(ac & 017, e, 1); return 0; }
        do_out(ac & 017);
        return 0; }

    case 070: {                                          /* CLOSE      */
        chan_t *ch = &chan[ac & 017];
        if (ch->obufhdr && !(ch->isdsk && ch->mode >= 014)) do_out(ac & 017);
        if (ch->fp) { fclose(ch->fp); ch->fp = NULL; }
        dsk_close(ch);
        ch->status &= ~IO_EOF;
        return 0; }

    case 071: {                                          /* RELEAS     */
        chan_t *ch = &chan[ac & 017];
        if (ch->fp) { fclose(ch->fp); ch->fp = NULL; }
        dsk_close(ch);
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
        int nam = e, ex = e + 1, err = e + 1, siz = -1, xtnd = 0, keep = 0;

        if (ch->istty || ch->isnull) return 1;

        /* Two shapes of argument block.  The short one is four words
         * beginning with the SIXBIT filename; the extended one begins
         * with a word count, which is how it is told apart -- a real
         * filename always has something in its left half.  TBA uses the
         * extended form, with a count of 12:
         *
         *   .RBCNT 0  length of the block
         *   .RBPPN 1  directory
         *   .RBNAM 2  SIXBIT filename
         *   .RBEXT 3  XWD extension, creation date
         *   .RBPRV 4  protection and mode
         *   .RBSIZ 5  size in words                                  */
        if (LH(M[e]) == 0 && RH(M[e]) >= 3 && RH(M[e]) <= 040) {
            xtnd = (int)RH(M[e]);
            nam = e + 2; ex = e + 3; err = e + 3;
            if (xtnd > 5) siz = e + 5;
        }
        sixbit_str(M[nam], name);
        sixbit_str((M[ex] >> 18) << 18, ext);
        if (opt_verbose > 1)
            warn("%s %s block at %06o: %012llo %012llo %012llo %012llo",
                 op == 076 ? "LOOKUP" : "ENTER", xtnd ? "extended" : "short", e,
                 (unsigned long long)M[e], (unsigned long long)M[e+1],
                 (unsigned long long)M[e+2], (unsigned long long)M[e+3]);

        {
            char want[32];
            snprintf(want, sizeof want, "%s%s%s",
                     name, ext[0] ? "." : "", ext);
            keep = (op == 077 && ch->isdsk && ch->fw && !ch->fout
                    && !strcmp(want, ch->fname));
            if (!keep) dsk_close(ch);
            snprintf(ch->fname, sizeof ch->fname, "%s", want);
        }
        ch->isdsk = 1;
        ch->fout = (op == 077);
        ch->fdirty = 0;
        if (op == 076) {                       /* LOOKUP: must exist   */
            ch->isufd = 0;
            if (!strcmp(ext, "UFD") ? !dsk_load_ufd(ch) : (!name[0] || !dsk_load(ch))) {
                if (opt_verbose) warn("LOOKUP %s -- not found", ch->fname);
                M[err] = (M[err] & ~(w36)HMASK) | 1;   /* file not found */
                return 0;
            }
            if (siz >= 0) M[siz] = (w36)ch->flen;
        } else {                               /* ENTER: create        */
            if (!name[0]) { M[err] = (M[err] & ~(w36)HMASK) | 1; return 0; }
            if (!strcmp(name, "COM")) dsk_path(scratch, sizeof scratch, ch->fname);
            /* LOOKUP then ENTER of the same file on the same channel is
             * TOPS-10's update mode -- the program means to rewrite
             * parts of the file in place, not to start it again.  TBA
             * opens NEWADV.DAT and SCENAR.IO exactly like this, so an
             * ENTER that truncated would throw the world away. */
            if (!keep) {
                ch->flen = ch->fpos = 0;
                dsk_room(ch, 1024);
            } else {
                ch->fpos = 0;
            }
            ch->fdirty = 1;
        }
        if (opt_verbose) warn("%s %s (%d words)",
                              op == 076 ? "LOOKUP" : "ENTER", ch->fname, ch->flen);
        return 1; }

    case 055:                                            /* RENAME     */
        return 1;
    case 072:                                            /* MTAPE      */
        return 0;
    case 073: AC(ac) = 1; return 0;                      /* UGETF      */
    case 074:                                            /* USETI      */
    case 075: {                                          /* USETO      */
        chan_t *ch = &chan[ac & 017];
        int blk = e ? e : 1;
        if (ch->isdsk) ch->fpos = (blk - 1) * 0200;
        if (opt_verbose > 1)
            warn("USET%c ch%o block %d -> word %d of %d",
                 op == 074 ? 'I' : 'O', ac & 017, blk, ch->fpos, ch->flen);
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
