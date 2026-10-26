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

/* Every patch is recorded so it can come out again before SUSPEND writes
 * the core image.  A word goes back only if it still holds what the patch
 * put there: if the game has written it since, the game's value stands.
 * A flag patch sets one bit and takes back only that bit, so whatever
 * the game has done to the object's other flags is kept. */
static struct { int addr; w36 was, now, bit; } patch[64];
static int npatch;

static void poke(int addr, w36 val)
{
    patch[npatch].addr = addr;
    patch[npatch].was  = M[addr];
    patch[npatch].now  = val;
    patch[npatch].bit  = 0;
    npatch++;
    M[addr] = val;
}

static void poke_bit(int addr, w36 bit)
{
    if (M[addr] & bit) return;
    patch[npatch].addr = addr;
    patch[npatch].was  = M[addr];
    patch[npatch].now  = M[addr] | bit;
    patch[npatch].bit  = bit;
    npatch++;
    M[addr] |= bit;
}

static void unlimited_on(void)
{
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
    while (npatch--) {
        int a = patch[npatch].addr;
        if (patch[npatch].bit)
            M[a] &= ~patch[npatch].bit & WMASK;
        else if (M[a] == patch[npatch].now)
            M[a] = patch[npatch].was;
    }
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


/* ------------------------------------------------------------------ *
 * The barrow wight's tomb, LOC 373, lets nobody out.
 *
 * The player's decoder at 404570 splits a travel entry into a kind
 * (entry / 10^8, plus one), a condition ((entry / 10^4) mod 1000) and a
 * destination (entry mod 10^4; above 2000 it is message dest-2000).
 * Kind 1 is a percentage: take the entry if PCT(condition).  Kind 6
 * (404647) takes it if PROP(condition) equals the ten-millions digit.
 * Every monster that guards a doorway is written as kind 6, digit = its
 * live state, destination = "The way is blocked!  You'll have to fight
 * it to get by!":
 *
 *   LOC 319  510762081   SKELETON  in state 1
 *   LOC 320  510732084   HARPIES   in state 1
 *   LOC 321  510722084   VIPERS    in state 1
 *   LOC 322  530712081   MANTICOR  in state 3
 *   LOC 323  510742081   CHIMERA   in state 1
 *   LOC 372  530362081   GIANT CLAM in state 3
 *
 * Kill the monster and its state changes, the entry stops applying, and
 * the decoder falls through to the next exit.  The four ways out of 373
 * are written 51292081 -- the BARROW WIGHT (129) in state 1 with its
 * state digit dropped.  That shifts the 5 down into the critter slot and
 * leaves kind 1, PCT(129), which a percentage can never fail: the way is
 * blocked whether the wight is alive or dead.  It caught a player in
 * 1980 -- BLOCKE.LOW on the tape is a game suspended in this very room,
 * the wight's state 0, the decoder's scratch word at 110473 still
 * holding 51292081.
 *
 * -b puts the digit back: 511292081, the wight in its live state, so the
 * way out opens once it is dead, the same as everywhere else.  The scan
 * states the rule rather than the four addresses: a kind-1 entry whose
 * percentage is over 100 and whose ten-millions digit is 5 is a kind-6
 * entry that lost its state digit, and a guard's live state is 1.
 */
#define GUARD_KIND6    500000000LL
#define GUARD_ALIVE    10000000LL     /* state digit 1 */

static int opt_fixwight;

static int fixwight_on(void)
{
    int k, fixed = 0;

    for (k = 1; k <= 3456; k++) {
        w36 w = M[A_TRAVEL2 + k];
        long long raw = sx36(w), mag = raw < 0 ? -raw : raw;
        if (mag < 100000000LL && mag / 10000000 == 5
            && (mag / 10000) % 1000 > 100) {
            long long m = GUARD_KIND6 + GUARD_ALIVE + mag % 10000000;
            poke(A_TRAVEL2 + k, (w36)(raw < 0 ? -m : m) & WMASK);
            fixed++;
        }
    }
    return fixed;
}


/* ------------------------------------------------------------------ *
 * -b also mends six more slips in the shipped data and code.  Each one
 * checks that the word it changes still holds the slip before touching
 * it, so a later copy of the game that has them fixed is left alone.
 *
 * The maze cove, LOC 429.  The stream issues from the cliffs; east and
 * IN lead into the maze, and that is all -- no way back north to the
 * narrow canyon (428) that brought you, and no era travel either, so
 * once you have stepped down to the cove the rest of the Vale is gone
 * for good.  The travel table is full where the cove's two entries sit,
 * but it has room at the end (3457 of 3599), so -b copies them there
 * and adds N, UP, OUT and LEAVE back to 428.  Like the cove's own exits
 * they carry the critter flag, so the bull herd in the cove stays put.
 *
 * Two maze rooms that lead into themselves.  458's south exit is 458:
 * the only way into 458 is north from 456, and the only way out of the
 * dead end beyond it (459, where the dwarf keeps the mithril broadsword)
 * is back into 458, so whoever goes to fight the dwarf stays there.
 * 476's south exit is 476, cutting off the branch it lies on.  Each was
 * surely meant to lead back the way you came, to 456 and to 475.
 *
 * The sheep herd (99) is critter class 4, which it shares only with the
 * archeologist.  Class 4 picks one of messages 2-9 of the creature's
 * text to mutter as it passes -- the archeologist's eight remarks.  The
 * herd has four lines, so it reads on into the mithril dagger's text
 * and the rat's.  Every other herd -- goats, donkeys, horses, bulls --
 * is class 2, with the same flags as the sheep; -b makes it class 2.
 *
 * The Saracen (121).  When a creature is searched (OLOOT) or killed
 * (OBOOTY) the game brings out the object numbered after it: the dwarf
 * yields the mithril axe, the troll its mace.  Object 122, the steel
 * simitar, follows the Saracen and nothing else can ever produce it,
 * but he carries neither flag.  -b gives him OLOOT, like the dwarf and
 * the archeologist, so SEARCH SARACEN finds the simitar -- unless the
 * simitar is somewhere already.
 *
 * The fish (106) that the goblin drops has no word in the vocabulary,
 * and unlike every other object without one it is not found by name
 * either.  The vocabulary has room (509 of 549), so -b adds FISH.
 *
 * FILL (410252) asks only that you carry the thing and that its state
 * is 0, then makes it 1.  For the canteen and the flasks 1 is "full of
 * water"; for anything else it is whatever the next line of text holds
 * -- FILL CHALICE leaves the chalice described as "hoard of gold".  -b
 * routes the TOTING call at 410256 through a few words placed past the
 * end of the high segment, which also ask ODRINK, and answer anything
 * that cannot hold water with "That doesn't apply to this thing."
 */
#define A_TRVCOUNT     0114732   /* next free travel slot              */
#define A_CLASS        0116734   /* critter class, per object          */
#define A_FLAGS        0065574   /* object flags                       */
#define A_FMASK        0114310   /* the mask for each flag bit         */
#define A_PLACE        0065323   /* PLACE(obj)                         */
#define A_KTAB         0063210   /* vocabulary: type*1000 + number     */
#define A_ATAB         0064255   /* vocabulary: the word, enciphered   */
#define A_ACODE        0122661   /*   ... with this key                */
#define A_FILLCALL     0410256   /* FILL: PUSHJ 17,TOTING              */
#define A_FILLCHK      0441400   /* free words past the high segment   */
#define OBJ_SHEEP      99
#define OBJ_SARACEN    121
#define OBJ_SIMITAR    122
#define OBJ_FISH       106
#define F_LOOT         16

static int cove_on(void)
{
    static const struct { int word; long long x; } cove[] = {
        {  4, 10000431 },   /* E     -> maze, as before */
        { 17, 10000431 },   /* IN    -> maze, as before */
        {  2, 10000428 },   /* N     -> the narrow canyon */
        { 10, 10000428 },   /* UP */
        { 15, 10000428 },   /* OUT */
        { 16, 10000428 },   /* LEAVE, last in the list */
    };
    int k, n = sizeof cove / sizeof cove[0];
    int top = (int)M[A_TRVCOUNT];

    if ((M[A_TRVSTA + 429] & HMASK) != 3155 || top != 3457) return 0;
    for (k = 0; k < n; k++)
        if (M[A_TRAVEL1 + 1 + top + k] || M[A_TRAVEL2 + top + k]) return 0;
    for (k = 0; k < n; k++) {
        long long w = k == n - 1 ? -cove[k].word : cove[k].word;
        poke(A_TRAVEL1 + 1 + top + k, (w36)w & WMASK);
        poke(A_TRAVEL2 + top + k, (w36)cove[k].x);
    }
    poke(A_TRVSTA + 429, (w36)top);
    poke(A_TRVCOUNT, (w36)(top + n));
    return n - 2;
}

static int maze_on(void)
{
    static const struct { int k, from, to; } loop[] = {
        { 3274, 458, 456 },   /* 458 S */
        { 3346, 476, 475 },   /* 476 S */
    };
    int i, fixed = 0;
    for (i = 0; i < 2; i++)
        if (M[A_TRAVEL2 + loop[i].k] == (w36)loop[i].from) {
            poke(A_TRAVEL2 + loop[i].k, (w36)loop[i].to);
            fixed++;
        }
    return fixed;
}

static w36 a5(const char *s)
{
    w36 w = 0;
    int i;
    for (i = 0; i < 5; i++) w = (w << 7) | (w36)(*s ? *s++ : ' ');
    return w << 1;
}

static int fish_on(void)
{
    int i, n = (int)M[A_KTAB];
    for (i = 1; i < n; i++)
        if (M[A_KTAB + i] == WMASK) break;          /* the -1 that ends it */
    if (i >= n || M[A_KTAB + i + 1]) return 0;
    poke(A_ATAB + i, a5("FISH") ^ M[A_ACODE]);
    poke(A_KTAB + i, 1000 + OBJ_FISH);
    poke(A_KTAB + i + 1, WMASK);
    return 1;
}

static int fill_on(void)
{
    static const w36 chk[] = {
        0261740000016ULL,   /* PUSH  17,16                         */
        0260740400015ULL,   /* PUSHJ 17,TOTING                     */
        0262740000016ULL,   /* POP   17,16                         */
        0325000441413ULL,   /* JUMPGE 0,441413  not carried: as before */
        0261740000016ULL,   /* PUSH  17,16                         */
        0260740400536ULL,   /* PUSHJ 17,ODRINK                     */
        0262740000016ULL,   /* POP   17,16                         */
        0321000441413ULL,   /* JUMPL  0,441413  holds water: carry on */
        0201000000114ULL,   /* MOVEI 0,76  "That doesn't apply..." */
        0202000120632ULL,   /* MOVEM 0,120632  (the reply pending) */
        0400000000000ULL,   /* SETZ  0,0                           */
        0263740000000ULL,   /* POPJ  17,                           */
    };
    int i, n = sizeof chk / sizeof chk[0];
    if (M[A_FILLCALL] != 0260740400015ULL) return 0;
    for (i = 0; i < n; i++) if (M[A_FILLCHK + i]) return 0;
    for (i = 0; i < n; i++) poke(A_FILLCHK + i, chk[i]);
    poke(A_FILLCALL, 0260740000000ULL | A_FILLCHK);
    return 1;
}

static int fixbugs_on(int *detail)
{
    detail[0] = fixwight_on();
    detail[1] = cove_on();
    detail[2] = maze_on();
    detail[3] = 0;
    if (M[A_CLASS + OBJ_SHEEP] == 4) { poke(A_CLASS + OBJ_SHEEP, 2); detail[3] = 1; }
    detail[4] = 0;
    if (M[A_PLACE + OBJ_SIMITAR] == 0 && !(M[A_FLAGS + OBJ_SARACEN] & M[A_FMASK + F_LOOT])) {
        poke_bit(A_FLAGS + OBJ_SARACEN, M[A_FMASK + F_LOOT]);
        detail[4] = 1;
    }
    detail[5] = fish_on();
    detail[6] = fill_on();
    return detail[0] + detail[1] + detail[2] + detail[3] + detail[4] + detail[5] + detail[6];
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
    "  -b, --fix-bugs      mend slips in the shipped data: the barrow wight's\n"
    "                      tomb, the maze cove and two maze rooms you cannot\n"
    "                      leave, the sheep herd's stray lines, the Saracen's\n"
    "                      simitar, the fish's name, and FILL on things that\n"
    "                      cannot hold water\n"
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
        else if (!strcmp(a, "-b") || !strcmp(a, "--fix-bugs")
                 || !strcmp(a, "--fix-wight")) opt_fixwight = 1;
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
    if (opt_fixwight) {
        int d[7];
        fixbugs_on(d);
        if (opt_verbose)
            fprintf(stderr, "[bugs] wight exits %d, cove exits %d, maze loops %d, "
                    "sheep %d, saracen %d, fish %d, fill %d\n",
                    d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
    }

    cpu_run();

    fflush(stdout);
    if (suspend_requested) {
        patches_undo();    /* what lands on disk is an ordinary saved game */
        char flags[16];
        snprintf(flags, sizeof flags, "%s%s%s",
                 opt_unlimited ? " -u" : "", opt_fixmap ? " -m" : "",
                 opt_fixwight ? " -b" : "");
        if (save_core(corefile))
            printf("\n[core image saved in %s -- \"archon -c%s\" to continue]\n",
                   corefile, flags);
        else
            printf("\n[could not write %s]\n", corefile);
        fflush(stdout);
    }
    return 0;
}
