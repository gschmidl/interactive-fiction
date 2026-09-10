/* jsys.c -- the TOPS-20 calls ADVENTURE makes for itself.
 *
 * Almost all of this game's dealings with the operating system go
 * through FOROTS and come out as TOPS-10 UUOs, which monitor.c answers.
 * But the game was written for TOPS-20 and reaches past the FORTRAN
 * runtime in a few places -- to find out who is playing, to time the
 * session, to check whether the player may run in wizard mode, and,
 * most importantly, to map its text file straight into memory instead of
 * reading it.  Those are JSYS calls, opcode 0104, and this file is the
 * whole of that side of the interface.
 *
 * A JSYS returns to the word after itself on failure and skips one word
 * on success, so every one of these returns 0 or 1.  The numbers are
 * from MONSYM.UNV taken off the same pack as the game.
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pdp10.h"

extern int opt_delays;
extern void add_logical_name(const char *name);

/* JSYS numbers, from <TOOLS>MONSYM.UNV */
#define J_ERSTR  0011
#define J_GJINF  0013
#define J_TIME   0014
#define J_RUNTM  0015
#define J_GTJFN  0020
#define J_OPENF  0021
#define J_CLOSF  0022
#define J_CLZFF  0034
#define J_SIZEF  0036
#define J_DIRST  0041
#define J_PMAP   0056
#define J_PSOUT  0076
#define J_RPCAP  0150
#define J_EPCAP  0151
#define J_DISMS  0167
#define J_HALTF  0170
#define J_CRLNM  0502
#define J_STPPN  0556
#define J_GETOK  0574

/* The player.  On the pack this was whoever had logged in; the game asks
 * for the number with GJINF, turns it into a string with DIRST, and puts
 * it in the scoreboard.  Here it comes from the environment, or from
 * -player on the command line, and gets a directory number of its own. */
const char *opt_player = NULL;

#define MY_DIRNO 0400000042ULL    /* a plausible <structure,,directory> */

static time_t t_start;
static clock_t c_start;

void jsys_init(void)
{
    t_start = time(NULL);
    c_start = clock();
}

const char *player_name(void)
{
    const char *p = opt_player;
    if (!p) p = getenv("ADVENTURER");
    if (!p) p = getenv("USERNAME");
    if (!p) p = getenv("USER");
    if (!p || !*p) p = "PLAYER";
    return p;
}

/* ---- string helpers ------------------------------------------------ */
/* TOPS-20 strings are seven-bit bytes packed five to a word, addressed
 * by an ordinary byte pointer.  There is one shorthand, and the game
 * uses it everywhere: HRROI 1,BUFFER leaves -1,,BUFFER in AC1, and the
 * monitor reads that as "a seven bit byte pointer to the start of
 * BUFFER".  Turn it into the real thing before using it. */
static w36 mkbp(int addr) { return XWD(0440700, addr); }

static w36 fixbp(w36 bp)
{
    if (LH(bp) == 0777777 || LH(bp) == 0)   /* -1,,addr  or  0,,addr */
        return mkbp((int)RH(bp));
    return bp;
}

static void bp_put(w36 *bp, int c)
{
    int p = (int)((*bp >> 30) & 077);
    int s = (int)((*bp >> 24) & 077);
    int a;
    if (s == 0) { s = 7; p = 36; }             /* bare address          */
    p -= s;
    if (p < 0) { p = 36 - s; *bp = (*bp & ~(w36)HMASK) | ((*bp + 1) & HMASK); }
    *bp = (*bp & ~((w36)077 << 30)) | ((w36)p << 30);
    *bp = (*bp & ~((w36)077 << 24)) | ((w36)s << 24);
    a = (int)RH(*bp);
    M[a] = (M[a] & ~((w36)((1u << s) - 1) << p)) | ((w36)(c & ((1 << s) - 1)) << p);
}

static void put_string(int ac, const char *s)
{
    w36 bp = fixbp(AC(ac));
    while (*s) bp_put(&bp, (unsigned char)*s++);
    bp_put(&bp, 0);
    AC(ac) = bp;                 /* left pointing at the null, as TOPS-20 does */
}

/* Read an ASCIZ string out of the machine. */
static void get_string(w36 bp, char *out, int max)
{
    int n = 0;
    bp = fixbp(bp);
    for (;;) {
        int p = (int)((bp >> 30) & 077), sz = (int)((bp >> 24) & 077), a, c;
        p -= sz;
        if (p < 0) { p = 36 - sz; bp = (bp & ~(w36)HMASK) | ((bp + 1) & HMASK); }
        bp = (bp & ~((w36)077 << 30)) | ((w36)p << 30);
        a = (int)RH(bp);
        c = (int)((M[a] >> p) & ((1u << sz) - 1));
        if (!c || n >= max - 1) break;
        out[n++] = (char)c;
    }
    out[n] = 0;
}

/* ---- the file window ----------------------------------------------- */
/* PMAP is how the game reads ADVTXT.BIN: it opens the file, then slides
 * a four page window over it, one PMAP per move of the window, and picks
 * the text out of memory.  There is no JFN table to speak of -- the game
 * opens exactly one file this way -- so keep the words and copy them. */
static const builtin_file_t *mapped_file;
static w36 *mapped_words;
static int  mapped_nwords;

static void load_mapped(const char *name)
{
    const builtin_file_t *b;
    for (b = builtin_files; b->name; b++)
        if (!strcmp(b->name, name)) { mapped_file = b; break; }
    if (mapped_file) {
        mapped_words = (w36 *)mapped_file->words;
        mapped_nwords = mapped_file->nwords;
    }
}

/* ---- the JSYSes ---------------------------------------------------- */
int do_jsys(int number)
{
    switch (number) {

    case J_GJINF:                                  /* GJINF%           */
        /* 1: logged in directory, 2: connected directory,
         * 3: job number, 4: controlling terminal. */
        AC(1) = MY_DIRNO;
        AC(2) = MY_DIRNO;
        AC(3) = 1;
        AC(4) = 1;
        return 0;

    case J_DIRST: {                                /* DIRST%           */
        /* AC1 is a string pointer to write to, AC2 the directory
         * number.  The game does this to find out who is playing. */
        char buf[64];
        int i;
        const char *p = player_name();
        for (i = 0; i < 39 && p[i]; i++)
            buf[i] = (p[i] >= 'a' && p[i] <= 'z') ? p[i] - 32 : p[i];
        buf[i] = 0;
        put_string(1, buf);
        return 1; }

    case J_STPPN:                                  /* STPPN%           */
        /* A directory name to a TOPS-10 project-programmer number, which
         * the game uses as the player identity in its own files.  The
         * answer goes in AC2 and there is no skip; the game reads AC2
         * apart into AC0 and AC1 in the two words that follow.
         *
         * The last five lines of ADVWIZ.DAT are the numbers of the CMU
         * accounts allowed to be wizards -- 4,45 and four others.  This
         * is not one of them, and neither was the account on the pack. */
        AC(2) = XWD(1, 042);
        return 0;

    case J_TIME:                                   /* TIME%            */
        /* Milliseconds since system startup, plus uptime in AC2. */
        AC(1) = (w36)((time(NULL) - t_start) * 1000 + 1);
        AC(2) = (w36)((time(NULL) - t_start) * 1000 + 1);
        return 0;

    case J_RUNTM: {                                /* RUNTM%           */
        /* Runtime of the job in AC1, console time in AC2.  The game
         * prints these when you quit. */
        double cpu = (double)(clock() - c_start) / CLOCKS_PER_SEC;
        AC(1) = (w36)(long)(cpu * 1000.0);
        AC(2) = (w36)((time(NULL) - t_start) * 1000);
        return 0; }

    case J_RPCAP:                                  /* RPCAP%           */
        /* Capabilities: what the job could enable, and what is enabled.
         * The game tests SC%WHL/SC%OPR to decide whether the player is
         * allowed into wizard mode without a password. */
        AC(2) = 0;
        AC(3) = 0;
        return 0;

    case J_EPCAP:                                  /* EPCAP%           */
        return 0;

    case J_GETOK:                                  /* GETOK%           */
        /* "May I?", asked of the access control job.  GETOK% has three
         * returns -- no access control job, denied, granted -- and the
         * pack this game came off ran none, so the first is what the
         * game actually got, and its first return falls straight through
         * into its second.  Answer the same way. */
        AC(1) = 0;
        return 0;

    case J_CRLNM: {                                /* CRLNM%           */
        /* The game defines ADVEN: as its own directory and then opens
         * everything through it, so FOROTS asks DEVCHR about a device
         * called ADVEN.  Record the name; monitor.c answers for any name
         * defined here as though it were the disk, which -- since every
         * file lives in the working directory -- it is. */
        char name[16];
        get_string(AC(2), name, sizeof name);
        if (opt_verbose > 1) {
            fflush(stdout);
            fprintf(stderr, "[jsys] CRLNM %s\n", name);
        }
        if (name[0]) add_logical_name(name);
        return 1; }

    case J_DISMS: {                                /* DISMS%           */
        /* Sleep for AC1 milliseconds. */
        if (opt_delays) {
            long ms = (long)(AC(1) & 0777777777);
            if (ms > 0) {
                if (ms > 5000) ms = 5000;
                tty_flush();
#ifdef _WIN32
                { extern void Sleep(unsigned long); Sleep((unsigned long)ms); }
#else
                { struct timespec ts;
                  ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000L;
                  nanosleep(&ts, NULL); }
#endif
            }
        }
        return 0; }

    case J_PSOUT: {                                /* PSOUT%           */
        /* Write an ASCIZ string from the pointer in AC1. */
        w36 bp = fixbp(AC(1));
        int guard = 0;
        for (;;) {
            int p = (int)((bp >> 30) & 077), s = (int)((bp >> 24) & 077), a, c;
            p -= s;
            if (p < 0) { p = 36 - s; bp = (bp & ~(w36)HMASK) | ((bp + 1) & HMASK); }
            bp = (bp & ~((w36)077 << 30)) | ((w36)p << 30);
            a = (int)RH(bp);
            c = (int)((M[a] >> p) & ((1u << s) - 1));
            if (!c || ++guard > 100000) break;
            tty_out(c);
        }
        tty_flush();
        AC(1) = bp;
        return 0; }

    case J_ERSTR:                                  /* ERSTR%           */
        put_string(1, "?");
        return 1;

    case J_HALTF:                                  /* HALTF%           */
        halted = 1;
        return 0;

    case J_CLZFF:                                  /* CLZFF%           */
        return 0;

    case J_PMAP: {                                 /* PMAP%            */
        /* AC1 is source (JFN,,page), AC2 destination (fork,,page),
         * AC3 the count and access bits.  The game maps pages of its
         * text file into its own address space; anything else is an
         * unmap, which costs nothing here. */
        int spage = (int)(AC(1) & 0777777);
        int dpage = (int)(AC(2) & 0777777);
        int n = (int)(AC(3) & 0777);
        int i, k;
        if (!mapped_words) load_mapped("ADVTXT.BIN");
        if ((AC(1) & SIGNBIT) || !mapped_words)
            return 0;                              /* unmap: nothing to do */
        if (n == 0) n = 1;
        for (i = 0; i < n; i++) {
            int src = (spage + i) * 512;
            int dst = (dpage + i) * 512;
            for (k = 0; k < 512; k++)
                M[(dst + k) & (MEMTOP - 1)] =
                    (src + k < mapped_nwords) ? mapped_words[src + k] : 0;
        }
        return 0; }

    default:
        fflush(stdout);
        fprintf(stderr, "\n[adv751: unimplemented JSYS %o at %06o]\n",
                number, (PC - 1) & HMASK);
        halted = 1;
        exit_code = 3;
        return 0;
    }
}
