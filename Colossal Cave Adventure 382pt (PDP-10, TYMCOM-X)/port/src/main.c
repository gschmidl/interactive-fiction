/* main.c -- driver for the Tymshare 382-point ADVENTURE port.
 *
 * Loads the repaired ADVENT core image and runs it on the emulated
 * DECsystem-10.  The image is a single-segment TOPS-10 .SAV, so there
 * is nothing to do here but reset the processor and go.  Taken from the
 * CRYSTAL CAVE port, which needs the same driver: both are DEC FORTRAN
 * games off the same Tymshare tapes.
 *
 * The game's database is already parsed into the image -- on TYMCOM-X
 * you ran it once to read the data file, then SSAVEd the result, which
 * is what these tapes hold.  Nothing external is needed at runtime.
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
static const char COREMAGIC[] = "ADVENT-CORE-1\n";

/* ------------------------------------------------------------------ *
 * Cave hours, the turn limit, and the wait before you may resume.
 *
 * ADVENTURE ran on a commercial timesharing service, and it polices
 * the clock the way a program on such a service had to.  Three things
 * in the game's own core do it, and -u disarms all three:
 *
 *   050637/40/41  prime-time masks, one word each for weekdays,
 *                 weekends and holidays; bit h set means the cave is
 *                 shut during hour h.  The hours routine ANDs the
 *                 current hour's bit against the right mask, and a hit
 *                 means you are a wizard, or a visitor allowed a short
 *                 expedition of the 30 turns at 050650, or nothing
 *                 at all.  All three are already zero in the copy that
 *                 survived -- the cave is open all day -- so this costs
 *                 nothing and covers a game where a wizard has set
 *                 hours.
 *   050653        latency for restart, in minutes -- 90.  The same
 *                 routine works out how long ago you suspended, turns
 *                 you away below this, and below a third of it refuses
 *                 outright.  This is the one that bites a player today.
 *   004114        JUMPL 2,13444 -- the short expedition has used its
 *                 turns, and a wizard appears in green smoke to say so.
 *                 It becomes JFCL 0,0, so the turn counter still runs
 *                 and nothing acts on it.  (There is no limit on an
 *                 ordinary game here; only the short one is counted.)
 *
 * Patching the branch rather than the count is what covers a game
 * resumed from a core image, where the count is already set.  The
 * saved image stays honest either way: the originals go back before
 * SUSPEND writes advent.core, so a game saved under -u is an ordinary
 * saved game, and continuing it wants -u again.
 */
/* The five words -u touches are a property of the core image, and this
 * port builds more than one, from compilations that do not agree about
 * where anything lives.  So they are found rather than named, from code
 * the hours routine cannot do without:
 *
 *   IMULI 2,2640      1440 minutes in a day.  The routine works out how
 *                     long ago you suspended as (today - then) * 1440 +
 *                     (now - then), and nothing else in the program
 *                     multiplies by 1440.  That is the anchor.
 *   CAML 2,LATENCY    a few words along, the one comparison of that
 *                     elapsed figure against the restart latency.
 *   MOVE 2,mask       just above it, three loads of a prime-time mask,
 *   MOVEM 2,temp      each stashing it in the same one temporary -- one
 *                     per day class, weekday, weekend, holiday.
 *   AOS turns         and the turn counter: the one place that steps it
 *   MOVE 2,turns      and immediately tests it against the short-game
 *   CAMGE 2,SHORTLEN  count, through the TDZA/SETO pair F40 emits for a
 *   TDZA 2,2          logical, ANDs it with the short-game flag, and
 *   SETO 2,0          branches.  The JUMPL is what -u turns into a NOP.
 *   AND  2,flag
 *   JUMPL 2,over
 *
 * Every one has to match exactly once or -u does nothing and says so:
 * poking a guessed address in a core image is worse than not poking.
 */
#define PDP10_NOP      0255000000000ULL   /* JFCL 0,0 */
#define OPC(w)         ((int)(((w) >> 27) & 0777))
#define ACF(w)         ((int)(((w) >> 23) & 017))

static int opt_unlimited;
static int a_mask[3], a_latency, a_turns;

static int unlimited_find(void)
{
    int a, k, i, anchor = -1, n = 0;
    int msk[16], tmp[16], np = 0;

    for (a = 0; a < MEMTOP; a++)                 /* IMULI 2,2640 */
        if (M[a] == 0221100002640ULL) { anchor = a; n++; }
    if (n != 1) return 0;

    for (k = anchor, n = 0; k < anchor + 13; k++)          /* CAML 2,LATENCY */
        if (OPC(M[k]) == 0311 && ACF(M[k]) == 2) { a_latency = (int)(M[k] & HMASK); n++; }
    if (n != 1) return 0;

    for (k = anchor - 64; k < anchor; k++) {     /* MOVE 2,mask / MOVEM 2,temp */
        if (k < 1 || np == 16) continue;
        if (OPC(M[k]) == 0200 && ACF(M[k]) == 2 &&
            OPC(M[k + 1]) == 0202 && ACF(M[k + 1]) == 2) {
            msk[np] = (int)(M[k] & HMASK);
            tmp[np] = (int)(M[k + 1] & HMASK);
            np++;
        }
    }
    for (i = 0, n = 0; i < np; i++) {
        int c = 0, j, f = 0;
        for (j = 0; j < np; j++) if (tmp[j] == tmp[i]) c++;
        if (c != 3 || (i && tmp[i] == tmp[i - 1])) continue;
        for (j = 0; j < np; j++) if (tmp[j] == tmp[i]) a_mask[f++] = msk[j];
        n++;
    }
    if (n != 1) return 0;

    for (a = 1, n = 0; a < MEMTOP - 7; a++) {    /* the turn-limit branch */
        if (!(OPC(M[a]) == 0350 &&
              OPC(M[a+1]) == 0200 && ACF(M[a+1]) == 2 &&
              (M[a+1] & HMASK) == (M[a] & HMASK) &&
              OPC(M[a+2]) == 0315 && ACF(M[a+2]) == 2 &&
              M[a+3] == 0634100000002ULL && M[a+4] == 0474100000000ULL &&
              OPC(M[a+5]) == 0404 && ACF(M[a+5]) == 2 &&
              OPC(M[a+6]) == 0321 && ACF(M[a+6]) == 2)) continue;
        a_turns = a + 6;
        n++;
    }
    return n == 1;
}

static struct { int addr; w36 was; } patch[8];
static int npatch;

static void poke(int addr, w36 val)
{
    patch[npatch].addr = addr;
    patch[npatch].was  = M[addr];
    npatch++;
    M[addr] = val;
}

static int unlimited_on(void)
{
    if (!unlimited_find()) return 0;
    npatch = 0;
    poke(a_mask[0], 0);
    poke(a_mask[1], 0);
    poke(a_mask[2], 0);
    poke(a_latency, 0);
    poke(a_turns, PDP10_NOP);
    if (opt_verbose)
        fprintf(stderr, "[hours] masks %06o/%06o/%06o  latency %06o  branch %06o\n",
                a_mask[0], a_mask[1], a_mask[2], a_latency, a_turns);
    return 1;
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
    "ADVENT -- Tymshare's 382-point COLOSSAL CAVE ADVENTURE, run as the\n"
    "original DECsystem-10 FORTRAN binary on a PDP-10 emulator.\n"
    "\n"
    "usage: advent [options]\n"
    "  -c, --continue      resume from a saved core image\n"
    "  -f FILE             use FILE instead of advent.core\n"
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
    const char *corefile = "advent.core";
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
            fprintf(stderr, "advent: unknown option %s (try -h)\n", a);
            return 1;
        }
    }

    monitor_init();
    cpu_reset();

    if (resume && !load_core(corefile)) {
        fprintf(stderr, "advent: no saved game in %s\n", corefile);
        return 1;
    }

    if (opt_unlimited && !unlimited_on()) {
        fprintf(stderr, "advent: -u found nothing to patch in this image\n");
        return 1;
    }

    cpu_run();
    fflush(stdout);

    /* SUSPEND tells you to break out and save your core image, exactly as
     * it did on TYMCOM-X.  Do it for the player; "advent -c" picks the
     * expedition back up.  The game refuses to resume one that is too
     * recent, which is what -t is for. */
    if (suspend_requested) {
        unlimited_off();   /* what lands on disk is an ordinary saved game */
        if (save_core(corefile))
            printf("\n[core image saved in %s -- \"advent -c%s\" to continue]\n",
                   corefile, opt_unlimited ? " -u" : "");
        else
            printf("\n[could not write %s]\n", corefile);
        fflush(stdout);
    }
    return 0;
}
