/* main.c -- driver for the ARCHON port.
 *
 * Loads the BLOCKE core image, runs it on the emulated processor, and
 * provides the equivalent of the TOPS-10 "break out and SAVE your core
 * image" cycle that the game's SUSPEND command relies on.
 */
#include <stdlib.h>
#include <string.h>
#include "pdp10.h"

extern int  opt_verbose;
extern int  opt_faketime;
extern int  watch_addr;
extern int  opt_delays;
extern int  suspend_requested;
void monitor_init(void);
void queue_input(const char *s);

#define PAGEWORDS 512
static const char COREMAGIC[] = "ARCHON-CORE-1\n";

/* ------------------------------------------------------------------ *
 * Cave hours, the turn limit, and the wait before you may resume.
 *
 * ARCHON ran on a commercial timesharing service, and it polices the
 * clock the way a program on such a service had to.  Four things in
 * the game's own core do it, and -u disarms all four:
 *
 *   110451/52/53  prime-time masks, one word each for weekdays,
 *                 weekends and holidays; bit h set means ARCHON is
 *                 shut during hour h.  The routine at 417346 ANDs the
 *                 current hour's bit against the right mask, and a hit
 *                 means you are a DUNGEON MASTER, or a visitor allowed
 *                 a short exploration of the 110462 turns, or nothing.
 *                 All three are already zero in the copy that survived
 *                 -- ARCHON is open all day -- so this costs nothing
 *                 and covers a game where a master has set hours.
 *   110464        latency for restart, in minutes -- 90.  The same
 *                 routine works out from 110465/110466 how long ago
 *                 you suspended, turns you away below this, and below
 *                 a third of it refuses outright.
 *   421131        MOVEI 2,132 -- the world reset a fresh game runs sets
 *                 the whole wizard block from immediates of its own, and
 *                 132 is the 90 it puts back into 110464.  Poking the
 *                 latency alone would be undone a moment later, so the
 *                 immediate becomes MOVEI 2,0 as well.  (The masks it
 *                 writes are zero already, and the short-game count it
 *                 writes no longer has a branch to act on it.)
 *   403733        JRST 0,410707 -- a short exploration has used the
 *                 turns at 110462.
 *   403736        JRST 0,410707 -- an ordinary game has used the turns
 *                 at 114033, an allowance the game itself extends as
 *                 you get on.  Either way the dungeon master appears
 *                 in green smoke and declares that "THIS EXPLORATION
 *                 HAS LASTED TOO LONG".  Both become JFCL 0,0, so the
 *                 turn counter still runs and nothing acts on it.
 *
 * Patching those two branches rather than the two counts is what
 * covers a game resumed from a core image, where the counts are
 * already set.  The saved image stays honest either way: the
 * originals go back before SUSPEND writes archon.core, so a game
 * saved under -u is an ordinary saved game, and continuing it wants
 * -u again.
 */
#define A_PRIME_WEEK   0110451
#define A_PRIME_END    0110452
#define A_PRIME_HOL    0110453
#define A_LATENCY      0110464
#define A_LATENCY_INIT 0421131
#define A_TURNS_SHORT  0403733
#define A_TURNS_LONG   0403736
#define PDP10_NOP      0255000000000ULL   /* JFCL 0,0 */
#define MOVEI_2_0      0201100000000ULL   /* MOVEI 2,0 */

static int opt_unlimited;

static struct { int addr; w36 was; } patch[32];
static int npatch;

static void poke(int addr, w36 val)
{
    patch[npatch].addr = addr;
    patch[npatch].was  = M[addr];
    npatch++;
    M[addr] = val;
}

static void unlimited_on(void)
{
    npatch = 0;
    poke(A_PRIME_WEEK,   0);
    poke(A_PRIME_END,    0);
    poke(A_PRIME_HOL,    0);
    poke(A_LATENCY,      0);
    poke(A_LATENCY_INIT, MOVEI_2_0);
    poke(A_TURNS_SHORT,  PDP10_NOP);
    poke(A_TURNS_LONG,   PDP10_NOP);
}

static void patches_undo(void)
{
    while (npatch--) M[patch[npatch].addr] = patch[npatch].was;
    npatch = 0;
}


/* ------------------------------------------------------------------ *
 * The Vale is five parallel planes, and critters are not supposed to
 * cross between them.
 *
 * Every location carries a plane bit in the array at 110503 -- 2, 4, 8,
 * 16 or 32, for 149, 90, 90, 43 and 115 rooms.  A travel entry is
 * kind*10^8 + condition*10^4 + destination, and the ten-millions digit
 * on top of that is a critter flag: the mover at 402520 divides the
 * entry by 10000000 (121643) and rejects the exit if the quotient is 1,
 * while the player's decoder at 404570 divides by 10000 and then by
 * 1000 and keeps only the remainder, so the digit is invisible to the
 * player.  That is how the author kept the plane doorways open to you
 * and shut to the wildlife.
 *
 * He also left a check behind it: having accepted an exit, the mover
 * compares the plane bit of the critter's room against the plane bit of
 * the destination (402601) and, if they differ, prints
 *
 *     LEAKAGE...CRITTER #nnn got from LOCnnnn to nnnnn!!
 *
 * with a BEL either side of it.  Of the 3,456 travel entries, 645 cross
 * planes and 640 carry the flag.  Five do not, in two places, and both
 * read as slips rather than intent:
 *
 *   LOC  66 -> 264   the plane 1 / plane 3 doorway.  All four entries
 *                    coming back the other way, 264 -> 66, are written
 *                    10000066.  The one going out is a bare 264.
 *   LOC 383 ->   1   six exits, all to the same room.  Two of them are
 *                    written 10000001.  The other four are a bare 1.
 *
 * -m gives those five the digit their siblings already have.  It is the
 * author's own mechanism, applied where he missed it, and it cannot
 * change your own travel, because your travel never looks at that digit.
 * The scan states the rule rather than the five addresses: any exit a
 * critter could take into a room with a different plane bit.
 */
#define A_TRVSTA       0070203   /* TRVSTA(loc): first travel entry     */
#define A_TRAVEL1      0071305   /* TRAVEL(n,1): verb; <0 starts a room */
#define A_TRAVEL2      0100325   /* TRAVEL(n,2): the encoded exit       */
#define A_ZONE         0110503   /* ZONE(loc): the plane bit            */
#define NLOC           504
#define CRITTER_FLAG   10000000  /* the digit the mover reads           */

static int opt_fixmap;

static int fixmap_on(void)
{
    int loc, fixed = 0;

    for (loc = 1; loc <= NLOC; loc++) {
        int ptr = (int)(M[A_TRVSTA + loc] & HMASK);
        if (!ptr) continue;
        for (;;) {
            w36 w = M[A_TRAVEL2 + ptr];
            long long raw = sx36(w), mag = raw < 0 ? -raw : raw;
            int dest = (int)(mag % 10000);
            if (dest >= 1 && dest <= NLOC
                && M[A_ZONE + loc] != M[A_ZONE + dest]
                && mag / CRITTER_FLAG != 1) {
                long long m = mag + CRITTER_FLAG;
                poke(A_TRAVEL2 + ptr, (w36)(raw < 0 ? -m : m) & WMASK);
                fixed++;
            }
            ptr++;
            if (sx36(M[A_TRAVEL1 + ptr]) < 0) break;
        }
    }
    return fixed;
}



static int save_core(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned p;
    if (!f) return 0;
    fwrite(COREMAGIC, 1, sizeof COREMAGIC - 1, f);
    for (p = 0; p < MEMTOP / PAGEWORDS; p++) {
        unsigned i;
        int any = 0;
        for (i = 0; i < PAGEWORDS; i++)
            if (M[p * PAGEWORDS + i]) { any = 1; break; }
        if (!any) continue;
        fputc(p & 0377, f); fputc((p >> 8) & 0377, f);
        for (i = 0; i < PAGEWORDS; i++) {
            w36 w = M[p * PAGEWORDS + i];
            int k;
            for (k = 0; k < 5; k++) fputc((int)((w >> (8 * k)) & 0377), f);
        }
    }
    fputc(0377, f); fputc(0377, f);
    fclose(f);
    return 1;
}

static int load_core(const char *path)
{
    FILE *f = fopen(path, "rb");
    char magic[sizeof COREMAGIC];
    int a, b;
    if (!f) return 0;
    if (fread(magic, 1, sizeof COREMAGIC - 1, f) != sizeof COREMAGIC - 1 ||
        memcmp(magic, COREMAGIC, sizeof COREMAGIC - 1)) {
        fclose(f); return 0;
    }
    memset(M, 0, sizeof M);
    for (;;) {
        unsigned p, i;
        a = fgetc(f); b = fgetc(f);
        if (a < 0 || b < 0) break;
        if (a == 0377 && b == 0377) break;
        p = (unsigned)a | ((unsigned)b << 8);
        if (p >= MEMTOP / PAGEWORDS) break;
        for (i = 0; i < PAGEWORDS; i++) {
            w36 w = 0;
            int k;
            for (k = 0; k < 5; k++) w |= (w36)(fgetc(f) & 0377) << (8 * k);
            M[p * PAGEWORDS + i] = w & WMASK;
        }
    }
    fclose(f);
    /* Resuming works the way it did on TOPS-10: the core image is
     * re-RUN from the program's start address, and the game notices its
     * own "exploration in progress" state word and picks up where it
     * left off. */
    PC = image_start;
    FLAGS = F_USER;
    halted = 0;
    return 1;
}

static void usage(void)
{
    printf(
    "ARCHON -- the Vale of ARCHON, a DECsystem-10 adventure by way of a\n"
    "PDP-10 emulator running the original 1970s FORTRAN binary.\n"
    "\n"
    "usage: archon [options]\n"
    "  -c, --continue      resume a suspended exploration (see below)\n"
    "  -f FILE             use FILE instead of archon.core for that\n"
    "  -t HH:MM            tell the game it is HH:MM.  It refuses to resume a\n"
    "                      suspended game less than 90 minutes old, so this is\n"
    "                      how you skip the wait.\n"
    "  -q, --no-delay      skip the pauses the game asks for during combat\n"
    "  -u, --unlimited     ignore ARCHON's hours, the turn limit and the\n"
    "                      wait before a suspended game may resume\n"
    "  -m, --fix-map       stop critters straying between the Vale's five\n"
    "                      planes, which is what LEAKAGE complains about\n"
    "      --tables        enter at the world-reset instead, so the program\n"
    "                      prints its own table-space report first\n"
    "  -h, --help          this text\n"
    "\n"
    "diagnostics:\n"
    "  -v, -vv, -vvv       report monitor calls in increasing detail\n"
    "  -T                  trace every instruction (very slow, very loud)\n"
    "  -w ADDR             report every change to that octal core address\n"
    "\n"
    "SUSPEND asks you to save your core image, the way TOPS-10 wanted you to.\n"
    "Say yes: the image is written out, and \"archon -c\" resumes exactly where\n"
    "you stopped.\n"
    );
}

int main(int argc, char **argv)
{
    const char *corefile = "archon.core";
    int resume = 0, tables = 0, i;

    setvbuf(stdout, NULL, _IOFBF, 8192);

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-c") || !strcmp(a, "--continue")) resume = 1;
        else if (!strcmp(a, "-v")) opt_verbose = 1;
        else if (!strcmp(a, "-vv")) opt_verbose = 2;
        else if (!strcmp(a, "-vvv")) opt_verbose = 3;
        else if (!strcmp(a, "-T")) trace = 1;
        else if (!strcmp(a, "-q") || !strcmp(a, "--no-delay")) opt_delays = 0;
        else if (!strcmp(a, "-u") || !strcmp(a, "--unlimited")) opt_unlimited = 1;
        else if (!strcmp(a, "-m") || !strcmp(a, "--fix-map")) opt_fixmap = 1;
        else if (!strcmp(a, "--tables")) tables = 1;
        else if (!strcmp(a, "-w") && i + 1 < argc)
            watch_addr = (int)strtoul(argv[++i], NULL, 8);
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else if (!strcmp(a, "-f") && i + 1 < argc) corefile = argv[++i];
        else if (!strcmp(a, "-t") && i + 1 < argc) {
            int hh = 0, mm = 0;
            if (sscanf(argv[++i], "%d:%d", &hh, &mm) >= 1)
                opt_faketime = hh * 60 + mm;
        } else {
            fprintf(stderr, "archon: unknown option %s (try -h)\n", a);
            return 1;
        }
    }

    monitor_init();
    cpu_reset();

    /* Location 110467 is the program's own state word:
     *    0  the world tables are not built yet.  The program builds them
     *       from ARCHON.DAT, which is how this image was made; that file
     *       was not on the tape, and is not needed, because the tables
     *       are already here.
     *    1  tables built.  This entry prints the program's table-space
     *       report, runs its own world reset, and falls through into 2.
     *    2  greet the player and start playing.
     *    3  an exploration is in progress.
     *   -1  an exploration was SUSPENDed.
     * archon.low holds 2, which is what the author's maintenance mode
     * leaves behind -- "OKAY.  YOU CAN SAVE THIS VERSION NOW." -- so a
     * distribution copy is ready to play as it stands and the port has
     * nothing to set.  --tables winds it back to 1 for the report; the
     * reset it runs on the way past has nothing to undo, and the "Type G
     * to Continue" checkpoint that follows is answered for the player. */
    if (tables) {
        M[0110467] = 1;
        queue_input("G");
    }

    if (resume) {
        if (!load_core(corefile)) {
            fprintf(stderr, "archon: no suspended game in %s\n", corefile);
            return 1;
        }
    }

    if (opt_unlimited) unlimited_on();
    if (opt_fixmap) {
        int n = fixmap_on();
        if (opt_verbose) fprintf(stderr, "[map] %d exits given the critter flag\n", n);
    }

    cpu_run();

    fflush(stdout);
    if (suspend_requested) {
        patches_undo();    /* what lands on disk is an ordinary saved game */
        char flags[16];
        snprintf(flags, sizeof flags, "%s%s",
                 opt_unlimited ? " -u" : "", opt_fixmap ? " -m" : "");
        if (save_core(corefile))
            printf("\n[core image saved in %s -- \"archon -c%s\" to continue]\n",
                   corefile, flags);
        else
            printf("\n[could not write %s]\n", corefile);
        fflush(stdout);
    }
    return 0;
}
