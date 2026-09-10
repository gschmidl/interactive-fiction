/* mv32.c -- ECLIPSE MV (32-bit) emulator with an AOS/VS shim, enough of it
 * to run the 32-bit AOS/VS games ZORK.PR and FERRET.PR.
 *
 * Diagnostic-first, the same way the 16-bit one is: anything not implemented
 * stops with the PC, the instruction and a window of memory around it, so the
 * machine tells us what it needs instead of us guessing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "mv.h"

/* ---------------------------------------------------------------- state */
static word  M[MEMWORDS];
static dword AC[4];
static dword PC;
static int   C;
static int   halted, trace, verbose;
static int   fixtrap = 1;   /* the fixed-point overflow trap FXTD/FXTE switch */
static long  icount, maxinstr;
static dword pr_entry;
static dword dumplo, dumphi;
static dword pokepc[8], pokead[8], pokeval[8];
static int   npoke;
static unsigned ustbl, ustst, ustsz;

/* The wide stack lives in the ring's page zero as doublewords. */
#define DW(a)      (((dword)M[MADDR(a)] << 16) | M[MADDR((a) + 1)])
/* A function, not a macro: SETDW(WFP_A, wpopdw()) has to pop once, and a
 * macro that names its argument twice pops twice, which silently shifts
 * the whole frame restore along by one doubleword. */
static void setdw(dword a, dword v)
{ M[MADDR(a)] = (word)(v >> 16); M[MADDR(a + 1)] = (word)v; }
#define SETDW(a,v) setdw((a), (v))
#define WFP_A 0x10u
#define WSP_A 0x12u
#define WSL_A 0x14u
#define WSB_A 0x16u
#define WSP   DW(WSP_A)
#define WFP   DW(WFP_A)

static void dump_around(dword a, const char *why)
{
    unsigned k;
    fprintf(stderr, "    %s\n", why);
    for (k = 0; k < 4; k++) {
        dword r = (a & ~7u) + k * 8;
        unsigned j;
        fprintf(stderr, "    %05X:", r);
        for (j = 0; j < 8; j++) fprintf(stderr, " %04X", M[MADDR(r + j)]);
        fprintf(stderr, "\n");
    }
}

static void die(const char *why, word ir)
{
    fprintf(stderr,
        "\n*** %s\n"
        "    PC=%05X  IR=%04X\n"
        "    AC0=%08X AC1=%08X AC2=%08X AC3=%08X  C=%d\n"
        "    WSP=%08X WFP=%08X WSL=%08X WSB=%08X\n"
        "    after %ld instructions\n",
        why, PC, ir, AC[0], AC[1], AC[2], AC[3], C,
        DW(WSP_A), DW(WFP_A), DW(WSL_A), DW(WSB_A), icount);
    dump_around(PC, "memory around the PC:");
    halted = 1;
}

/* ------------------------------------------------------------- loading */
static int load_pr(const char *path)
{
    FILE *f = fopen(path, "rb");
    word *fw;
    long nb;
    unsigned nw, nblk, sblk, i;

    if (!f) { perror(path); return -1; }
    fseek(f, 0, SEEK_END); nb = ftell(f); rewind(f);
    nw = (unsigned)(nb / 2);
    fw = (word *)malloc((size_t)nw * sizeof *fw);
    if (!fw) { fclose(f); return -1; }
    for (i = 0; i < nw; i++) {
        int hi = fgetc(f), lo = fgetc(f);
        fw[i] = (word)((hi << 8) | (lo & 0xFF));
    }
    fclose(f);

    nblk  = nw / 1024;
    ustbl = fw[0x100 + 0x0C];
    ustst = fw[0x100 + 0x0F];
    ustsz = fw[0x100 + 0x13];
    if (fw[0x100 + 0x14] & 0x8000) {
        fprintf(stderr, "%s: this is a 16-bit program; use aosvs16.\n", path);
        free(fw); return -1;
    }
    sblk = nblk - ustsz;
    memset(M, 0, sizeof M);
    for (i = 0; i < ustsz * 1024u; i++)
        M[MADDR(ustst * 1024u + i)] = fw[sblk * 1024u + i];
    for (i = 0; i < ustbl * 1024u && 0x2000 + i < nw; i++)
        M[i] = fw[0x2000 + i];
    for (i = 0; i < 0x26; i++) M[0x100 + i] = fw[0x100 + i];
    pr_entry = (((dword)fw[0x17C] << 16) | fw[0x17D]) & OFFMASK;
    free(fw);

    if (verbose) {
        fprintf(stderr, "%s: %u blocks, 32-bit.  impure %u blk -> 0..%05X, "
                        "shared %u blk -> %05X..%05X\n", path, nblk,
                ustbl, ustbl * 1024u - 1, ustsz,
                ustst * 1024u, (ustst + ustsz) * 1024u - 1);
        fprintf(stderr, "  entry %05X  @6=%08X  WSP=%08X WSL=%08X WSB=%08X\n",
                pr_entry, DW(6), DW(WSP_A), DW(WSL_A), DW(WSB_A));
    }
    return 0;
}

/* ---------------------------------------------------------------- exec */
#include "syscall32.h"
#include "mvops.h"
#include "wide.h"
#include "wideexec.h"

/* -W lo hi: report every change to a range of memory, with the instruction
 * that made it.  A frame quietly overwritten from somewhere else is the
 * hardest kind of bug to find by reading a trace; this finds it in one run. */
static dword watch_lo, watch_hi;
static word  watch_old[64];

static void watch_snap(void)
{
    dword a;
    if (!watch_hi) return;
    for (a = watch_lo; a <= watch_hi && a - watch_lo < 64; a++)
        watch_old[a - watch_lo] = M[MADDR(a)];
}

static void watch_check(dword at, word ir)
{
    dword a;
    if (!watch_hi) return;
    for (a = watch_lo; a <= watch_hi && a - watch_lo < 64; a++)
        if (M[MADDR(a)] != watch_old[a - watch_lo]) {
            fprintf(stderr, "  [%05X] %04X -> %04X  by %04X at %05X\n",
                    a, watch_old[a - watch_lo], M[MADDR(a)], ir, at);
            watch_old[a - watch_lo] = M[MADDR(a)];
        }
}

static void step(void)
{
    dword at = PC;
    word  ir = M[MADDR(PC)];

    if (trace)
        fprintf(stderr, "%05X %04X  AC %08X %08X %08X %08X C%d SP%05X FP%05X\n",
                PC, ir, AC[0], AC[1], AC[2], AC[3], C, DW(WSP_A) & OFFMASK,
                DW(WFP_A) & OFFMASK);
    PC = (PC + 1) & OFFMASK;

    if (IS_WIDE(ir)) wide_exec(ir, at);
    else             narrow_exec(ir, at);
    watch_check(at, ir);
}

static void usage(void)
{
    fprintf(stderr,
      "aosvs32 -- runs an AOS/VS 32-bit program on an ECLIPSE MV emulator.\n"
      "\n"
      "  aosvs32 [options] [<program>.PR]\n"
      "\n"
      "  -d <dir>   directory holding the .PR and its data files\n"
      "  -s <dir>   where the game saves and restores (default: .)\n"
      "  -e <hex>   entry point (default: from the .PR header)\n"
      "  -v         trace system calls and file I/O\n"
      "  -t         instruction trace\n"
      "  -D <lo> <hi> dump that word range at the end (with -v)\n"
      "  -W <lo> <hi> report writes to that word range (max 64 words)\n"
      "  -F         trace floating point\n"
      "  -P <pc> <addr> <val>  set that word every time the PC reaches <pc>,\n"
      "             for asking what-if questions of a running program\n"
      "  -n <count> stop after this many instructions\n");
    exit(0);
}

int main(int argc, char **argv)
{
    int i;
    dword entry = 0;
    const char *pr = NULL;

    for (i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-P") && i + 3 < argc && npoke < 8) {
            pokepc[npoke]  = (dword)strtoul(argv[++i], NULL, 16);
            pokead[npoke]  = (dword)strtoul(argv[++i], NULL, 16);
            pokeval[npoke] = (dword)strtoul(argv[++i], NULL, 16);
            npoke++; }
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (!strcmp(argv[i], "-F")) fptrace = 1;
        else if (!strcmp(argv[i], "-W") && i + 2 < argc) {
            watch_lo = (dword)strtoul(argv[++i], NULL, 16);
            watch_hi = (dword)strtoul(argv[++i], NULL, 16);
        }
        else if (!strcmp(argv[i], "-F")) fptrace = 1;
        else if (!strcmp(argv[i], "-h")) usage();
        else if (!strcmp(argv[i], "-e") && i + 1 < argc) entry = (dword)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) maxinstr = strtol(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) set_datadir(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) set_savedir(argv[++i]);
        else if (!strcmp(argv[i], "-D") && i + 2 < argc) {
            dumplo = (dword)strtoul(argv[++i], NULL, 16);
            dumphi = (dword)strtoul(argv[++i], NULL, 16);
        }
        else if (argv[i][0] != '-') pr = argv[i];
    }
    if (!pr) { fprintf(stderr, "aosvs32: name the .PR to run\n"); return 1; }
    set_progname(pr);
    if (!datadir_set()) {
        const char *sl = strrchr(pr, '/'), *bs = strrchr(pr, '\\');
        if (bs > sl) sl = bs;
        if (sl) { char d[512]; size_t k = (size_t)(sl - pr);
                  if (k >= sizeof d) k = sizeof d - 1;
                  memcpy(d, pr, k); d[k] = 0; set_datadir(d); }
        else set_datadir(".");
    }
    if (load_pr(pr)) return 1;

    cur_pages = ustbl; max_pages = ustst;
    /* The .PR leaves the wide stack pointer, limit and base all equal, which
     * would make the very first push a stack fault.  AOS/VS sizes the stack
     * when it loads the program; give it the rest of the unshared area. */
    if ((DW(WSL_A) & OFFMASK) == (DW(WSB_A) & OFFMASK))
        SETDW(WSL_A, RING | ((ustst * 1024u - 1) & OFFMASK));
    /* Where a 32-bit program really starts.
     *
     * The .PR header's entry is the user's own main, and running it directly
     * skips the whole runtime set-up: the heap high-water mark and the memory
     * cache stay at -1, so the first allocation hands back 1 and the file
     * package writes a buffer through it into ring page zero.
     *
     * The runtime's start-up routine is `I.INIT`, and the loader finds it in
     * the doubleword at word 0x127, just past the UST template.  It begins
     * WPSH 1,1 and ends XJMP 0,3, so it is entered with the user's main in
     * AC1: it saves that, sets up the stack registers, calls I.GINIT and
     * ?OUTER_MAIN_INIT, and jumps to what it saved.
     *
     * Every 32-bit program checked follows the rule.  ZORK.PR, FERRET.PR,
     * SCOM.PR, DISCO.PR, LFCOPY.PR and SED.PR all carry a pointer there, and
     * in the utilities the .ST symbol tables name it I.INIT.  Where the word
     * is zero -- BROWSE.PR, CPIO.PR -- the .PR header's entry is I.START
     * itself, so there is nothing to arrange. */
    if (!entry) {
        dword init = DW(0x127) & OFFMASK;
        if (init >= 0x1000 && init < MEMWORDS) {
            /* I.INIT reads the address it finally jumps to off the top of
             * the stack, not out of a register: after its own WPSH 1,1 /
             * LCALL I.GINIT / WPOP 3,3 -- which is only there to carry AC1
             * across the call -- it does LDATS to fetch it, and later XCALLs
             * through it.  AC1 itself is only tested against zero, choosing
             * between the stacked address and one out of the stack base. */
            wpushdw(RING | (pr_entry & OFFMASK));
            AC[1] = RING | (pr_entry & OFFMASK);
            entry = init;
            if (verbose)
                fprintf(stderr, "  I.INIT at %05X, user main in AC1 = %08X\n",
                        init, AC[1]);
        } else {
            entry = pr_entry;
        }
    }
    PC = entry & OFFMASK;
    watch_snap();
    if (verbose) fprintf(stderr, "starting at %05X\n\n", PC);

    while (!halted) {
        /* A transfer to address 0 is the end of the process.  The PL/I
         * runtime's exit path finishes with an LCALL whose 32-bit target is
         * literally 00000000 -- at 7DF4B in FERRET.PR, reached straight after
         * the farewell score is written -- i.e. it calls through location 0,
         * which on a real AOS/VS is the OS's own process-exit trampoline.
         * Nothing sets that word here, and word 0 reads as 0000, which the
         * narrow decoder reads as JMP 0: Ferret printed its score and then
         * span on that one instruction for ever.  (Piped runs hid it, because
         * EOF on the next read halted the emulator anyway; only an
         * interactive QUIT hangs.)  Zork never gets here -- it leaves through
         * ?RETURN -- so this costs it nothing. */
        if (PC == 0) {
            if (verbose) { fputc(10, stderr);
                fprintf(stderr, "[call through location 0 -- process exit]");
                fputc(10, stderr); }
            halted = 1;
            break;
        }
        { int q; for (q = 0; q < npoke; q++)
              if (PC == pokepc[q]) M[MADDR(pokead[q])] = (word)pokeval[q]; }
        step();
        if (++icount == maxinstr && maxinstr) {
            fprintf(stderr, "\n*** instruction limit at PC=%05X\n", PC);
            break;
        }
    }
    if (verbose) fprintf(stderr, "\nstopped after %ld instructions, PC=%05X\n", icount, PC);
    if (dumphi > dumplo) {
        dword r;
        for (r = dumplo & ~7u; r < dumphi; r += 8) {
            unsigned j;
            fprintf(stderr, "%05X:", r);
            for (j = 0; j < 8; j++) fprintf(stderr, " %04X", M[MADDR(r + j)]);
            fprintf(stderr, "  |");
            for (j = 0; j < 8; j++) {
                int c1 = M[MADDR(r + j)] >> 8, c2 = M[MADDR(r + j)] & 0xFF;
                fputc(c1 >= 32 && c1 < 127 ? c1 : '.', stderr);
                fputc(c2 >= 32 && c2 < 127 ? c2 : '.', stderr);
            }
            fprintf(stderr, "|\n");
        }
    }
    return 0;
}
