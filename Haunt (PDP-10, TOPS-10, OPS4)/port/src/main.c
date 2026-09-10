/* main.c -- driver for the HAUNT port.
 *
 * Loads the HAUNT.EXE core image and runs it on the emulated
 * DECsystem-10.  The image is a two-segment TOPS-10 .EXE, already
 * flattened by tools/mkimage.py, so there
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
static const char COREMAGIC[] = "HAUNT-CORE-2\n";

int save_core(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned p;
    if (!f) return 0;
    fwrite(COREMAGIC, 1, sizeof COREMAGIC - 1, f);
    /* the program counter matters: Haunt has no resume logic of its
     * own, so a snapshot that only carried memory would restart the
     * game from the top with stale state. */
    { int k; for (k = 0; k < 4; k++) fputc((PC >> (8 * k)) & 0377, f);
      for (k = 0; k < 4; k++) fputc((FLAGS >> (8 * k)) & 0377, f); }
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

int load_core(const char *path)
{
    FILE *f = fopen(path, "rb");
    char magic[sizeof COREMAGIC];
    int a, b;
    if (!f) return 0;
    if (fread(magic, 1, sizeof COREMAGIC - 1, f) != sizeof COREMAGIC - 1 ||
        memcmp(magic, COREMAGIC, sizeof COREMAGIC - 1)) {
        fclose(f); return 0;
    }
    { int k, v = 0; for (k = 0; k < 4; k++) v |= (fgetc(f) & 0377) << (8 * k);
      PC = v; v = 0;
      for (k = 0; k < 4; k++) v |= (fgetc(f) & 0377) << (8 * k);
      FLAGS = v; }
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
    halted = 0;
    return 1;
}

static void usage(void)
{
    printf(
    "HAUNT -- John E. Laird's DECsystem-10 adventure, version 4.6\n"
    "(6-21-82), run as the original OPS4 image on a PDP-10 emulator.\n"
    "\n"
    "usage: haunt [options]\n"
    "  -c, --continue      resume from a snapshot\n"
    "  -f FILE             use FILE instead of haunt.core\n"
    "  -t HH:MM            tell the game it is HH:MM\n"
    "  -q, --no-delay      skip pauses the game asks for\n"
    "  -e, --echo          keep the program's own echo of what you type\n"
    "                      (kept anyway when input is piped, not a console)\n"
    "  -h, --help          this text\n"
    "\n"
    "Haunt has no save command of its own -- the game never had one.\n"
    "Type #save at the prompt to write a snapshot of the machine, and\n"
    "haunt -c to pick it up again.  #help lists the meta-commands.\n"
    "\n"
    "diagnostics:\n"
    "  -v, -vv, -vvv       report monitor calls in increasing detail\n"
    "  -T                  trace every instruction (very slow, very loud)\n"
    "  -w ADDR             report every change to that octal core address\n"
    );
}

int main(int argc, char **argv)
{
    const char *corefile = "haunt.core";
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
            fprintf(stderr, "haunt: unknown option %s (try -h)\n", a);
            return 1;
        }
    }

    opt_corefile = corefile;

    monitor_init();
    cpu_reset();

    if (resume && !load_core(corefile)) {
        fprintf(stderr, "haunt: no saved game in %s\n", corefile);
        return 1;
    }


    cpu_run();
    fflush(stdout);

    /* monitor.c sets this when the program prints "CORE-IMAGE".  That
     * is EXPLOR wording and Haunt never says it, so the hook is inert
     * here and no saved game is written yet.  It is kept because the
     * save/restore underneath it works at the emulator level and is
     * independent of which game is loaded: -c will resume whatever
     * -f names.  Giving Haunt a save of its own means finding how the
     * OPS4 image itself checkpoints, which is not yet established. */
    if (suspend_requested) {
        if (save_core(corefile))
            printf("\n[core image saved in %s -- \"haunt -c\" to continue]\n",
                   corefile);
        else
            printf("\n[could not write %s]\n", corefile);
        fflush(stdout);
    }
    return 0;
}
