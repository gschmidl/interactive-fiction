/* main.c -- driver for the EXPLOR port.
 *
 * Loads the repaired EXPLOR core image and runs it on the emulated
 * DECsystem-10.  The image is a single-segment TOPS-10 .SAV, so there
 * is nothing to do here but reset the processor and go.
 */
#include <stdlib.h>
#include <string.h>
#include "pdp10.h"

extern int  opt_verbose;
extern int  opt_faketime;
extern int  opt_delays;
extern int  watch_addr;
extern int  suspend_requested;
extern int  opt_dedupe_echo;
void monitor_init(void);
void queue_input(const char *s);

#define PAGEWORDS 512
static const char COREMAGIC[] = "EXPLOR-CORE-1\n";

/* ------------------------------------------------------------------ *
 * Cave hours, turn limits, and the wait before you may resume.
 *
 * EXPLOR ran on a commercial timesharing service, and it polices the
 * clock the way a program on such a service had to.  Four things in
 * the game's own core do it, and -u disarms all four:
 *
 *   102614/15/16  prime-time masks, one word each for weekdays,
 *                 weekends and holidays; bit h set means the cave is
 *                 shut during hour h.  The distribution copy shuts it
 *                 9-12 and 13-17 Monday to Friday.  The routine at
 *                 104731 ANDs the current hour's bit against the
 *                 right mask, and a hit is "I'M TERRIBLY SORRY, BUT
 *                 MYSTIC CAVE IS CLOSED": from there you are a wizard,
 *                 or a visitor on a 50-turn short exploration, or out.
 *                 Zeroed, the cave is open all day, every day, and
 *                 none of that is ever reached.
 *   102630        latency for restart, in minutes -- 45.  The same
 *                 routine works out from 102631/102632 how long ago
 *                 you suspended, turns you away below this, and below
 *                 a third of it refuses outright.  Zeroed, any
 *                 elapsed time will do.
 *   004605        JUMPL 2,15705 -- the short exploration has used the
 *                 50 turns at 102625.
 *   004610        JRST 0,15705 -- an ordinary game has used the 900
 *                 turns at 017620.  Either way a wizard appears in
 *                 green smoke and declares that "THE EXPLORATION HAS
 *                 LASTED TOO LONG".  Both become JFCL 0,0, so the turn
 *                 counter still runs and nothing acts on it.
 *
 * Patching those two branches rather than the two counts is what
 * covers a game resumed from a core image, where the counts are
 * already set.  The saved image stays honest either way: the
 * originals go back before SUSPEND writes explor.core, so a game
 * saved under -u is an ordinary saved game, and continuing it wants
 * -u again.
 */
#define A_PRIME_WEEK   0102614
#define A_PRIME_END    0102615
#define A_PRIME_HOL    0102616
#define A_LATENCY      0102630
#define A_TURNS_SHORT  0004605
#define A_TURNS_LONG   0004610
#define PDP10_NOP      0255000000000ULL   /* JFCL 0,0 */

static int opt_unlimited;

static struct { int addr; w36 was; } patch[8];
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
    poke(A_PRIME_WEEK,  0);
    poke(A_PRIME_END,   0);
    poke(A_PRIME_HOL,   0);
    poke(A_LATENCY,     0);
    poke(A_TURNS_SHORT, PDP10_NOP);
    poke(A_TURNS_LONG,  PDP10_NOP);
}

static void unlimited_off(void)
{
    while (npatch--) M[patch[npatch].addr] = patch[npatch].was;
    npatch = 0;
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
    PC = image_start;
    FLAGS = F_USER;
    halted = 0;
    return 1;
}

static void usage(void)
{
    printf(
    "EXPLOR -- a Tymshare DECsystem-10 adventure, run as the original\n"
    "FORTRAN binary on a PDP-10 emulator.\n"
    "\n"
    "usage: explor [options]\n"
    "  -c, --continue      resume from a saved core image\n"
    "  -f FILE             use FILE instead of explor.core\n"
    "  -t HH:MM            tell the game it is HH:MM\n"
    "  -q, --no-delay      skip pauses the game asks for\n"
    "  -u, --unlimited     ignore cave hours, the turn limit and the\n"
    "                      wait before a suspended game may resume\n"
    "  -e, --echo          keep the program's own echo of what you type\n"
    "                      (kept anyway when input is piped, not a console)\n"
    "  -h, --help          this text\n"
    "\n"
    "diagnostics:\n"
    "  -v, -vv, -vvv       report monitor calls in increasing detail\n"
    "  -T                  trace every instruction (very slow, very loud)\n"
    "  -w ADDR             report every change to that octal core address\n"
    );
}

int main(int argc, char **argv)
{
    const char *corefile = "explor.core";
    int resume = 0, i;

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
        else if (!strcmp(a, "-e") || !strcmp(a, "--echo")) opt_dedupe_echo = 0;
        else if (!strcmp(a, "-w") && i + 1 < argc)
            watch_addr = (int)strtoul(argv[++i], NULL, 8);
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else if (!strcmp(a, "-f") && i + 1 < argc) corefile = argv[++i];
        else if (!strcmp(a, "-t") && i + 1 < argc) {
            int hh = 0, mm = 0;
            if (sscanf(argv[++i], "%d:%d", &hh, &mm) >= 1)
                opt_faketime = hh * 60 + mm;
        } else {
            fprintf(stderr, "explor: unknown option %s (try -h)\n", a);
            return 1;
        }
    }

    monitor_init();
    cpu_reset();

    if (resume && !load_core(corefile)) {
        fprintf(stderr, "explor: no saved game in %s\n", corefile);
        return 1;
    }

    if (opt_unlimited) unlimited_on();

    cpu_run();
    fflush(stdout);

    /* The game's SUSPEND asks you to save your core image; do it for the
     * player, and "explor -c" picks the exploration back up.  It refuses
     * to resume one less than 45 minutes old, which is what -t is for.
     * Under -u the patches come out first, so what lands on disk is an
     * ordinary saved game -- hence the -u in the line that says how to
     * continue it. */
    if (suspend_requested) {
        unlimited_off();
        if (save_core(corefile))
            printf("\n[core image saved in %s -- \"explor -c%s\" to continue]\n",
                   corefile, opt_unlimited ? " -u" : "");
        else
            printf("\n[could not write %s]\n", corefile);
        fflush(stdout);
    }
    return 0;
}
