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
#include "qdefs.h"

/* ---------------------------------------------------------------- state */
static word *M;
static dword AC[4];
static dword PC;
static int   C;
static int   halted, trace, verbose;
static int   fixtrap = 1;   /* the fixed-point overflow trap FXTD/FXTE switch */
/* 64 bits: QUEST_SERVER alone takes about ninety million instructions to
 * build its world, and a 32-bit long -- which is what long is on Windows --
 * wraps within minutes of play and turns every delay into a deadlock. */
static long long icount, maxinstr;
static dword pr_entry;
static dword dumplo, dumphi;
static dword pokepc[8], pokead[8], pokeval[8];
static int   npoke;
static unsigned ustbl, ustst, ustsz;
/* -B <hex>: log every arrival at that PC, in whichever process */
static dword bkpt[16];
static long  bkhits[16];
static int   bkstr[16];     /* -BS: also show the byte strings at AC2/AC3 */
static int   nbkpt;

static void show_bytes(const char *tag, dword bp, int n)
{
    int k;
    fprintf(stderr, "      %s %08X:", tag, bp);
    for (k = 0; k < n; k++) {
        dword b = bp + (dword)k;
        word w = M[MADDR(b >> 1)];
        int c = (b & 1) ? (w & 0xFF) : (w >> 8);
        if (c >= 32 && c < 127) fputc(c, stderr); else fprintf(stderr, "<%02X>", c);
    }
    fputc(10, stderr);
}

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
        "    after %lld instructions\n",
        why, PC, ir, AC[0], AC[1], AC[2], AC[3], C,
        DW(WSP_A), DW(WFP_A), DW(WSL_A), DW(WSB_A), icount);
    dump_around(PC, "memory around the PC:");
    halted = 1;
}

/* ------------------------------------------------------------- loading */
static int load_pr(const char *path, int pi)
{
    FILE *f = fopen(path, "rb");
    word *fw;
    long nb;
    unsigned nw, nblk, sblk, i;

    if (!f) { perror(path); return -1; }
    if (!proc[pi].mem)
        proc[pi].mem = (word *)calloc(MEMWORDS, sizeof(word));
    if (!proc[pi].mem) { fclose(f); return -1; }
    M = proc[pi].mem;
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
    memset(M, 0, MEMWORDS * sizeof *M);
    for (i = 0; i < ustsz * 1024u; i++)
        M[MADDR(ustst * 1024u + i)] = fw[sblk * 1024u + i];
    for (i = 0; i < ustbl * 1024u && 0x2000 + i < nw; i++)
        M[i] = fw[0x2000 + i];
    for (i = 0; i < 0x26; i++) M[0x100 + i] = fw[0x100 + i];
    pr_entry = (((dword)fw[0x17C] << 16) | fw[0x17D]) & OFFMASK;
    proc[pi].ustbl = ustbl; proc[pi].ustst = ustst; proc[pi].ustsz = ustsz;
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
#include "sched.h"
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

/* Set a freshly loaded program up as process `pi` with one task running.
 *
 * The .PR header's entry is the user's own main, and running it directly
 * skips the whole runtime set-up: the heap high-water mark and the memory
 * cache stay at -1, so the first allocation hands back 1 and the file
 * package writes a buffer through it into ring page zero.  The runtime's
 * start-up routine is I.INIT, entered with the user main both pushed and in
 * AC1.
 *
 * Where to find it: the runtime's start-up vector is a block of long jumps
 * in the UST extension that begins LJMP I.INIT (A6D9 + a 32-bit address),
 * followed by the fault handlers.  In ZORK.PR and FERRET.PR the block starts
 * at ?USTA, word 0x126, so the address is the doubleword at 0x127 -- which
 * is where this used to look.  QUEST.PR and QUEST_SERVER.PR were linked for
 * the SWAT debugger: their UST carries two ring-3 pointers to 0x1AC where
 * Zork's has -1, the extension holds ".SWAT.IPC" and "?000SWAT.TMP.BKPT"
 * first, and the same block of jumps has moved down to 0x1B8.  Word 0x127
 * is zero there.  Looking only at 0x127 started QUEST at its user main with
 * the heap never set up, and it ran surprisingly far -- through the shared
 * data, IPC and logon -- before its first allocation, the stack for its
 * first task, came back as address 1.
 *
 * So scan the extension for the first LJMP whose target begins WPSH 1,1,
 * which is how I.INIT opens in every program checked.  For Zork and Ferret
 * that is the same answer as before. */
static dword find_iinit(void)
{
    dword a;
    for (a = 0x126; a < 0x400; a++)
        if (M[a] == 0xA6D9) {
            dword t = DW(a + 1) & OFFMASK;
            if (t >= 0x1000 && t < MEMWORDS && M[MADDR(t)] == 0xAD79) return t;
        }
    return 0;
}

static void proc_start(int pi, const char *nm, int pid, dword entry)
{
    Proc *p = &proc[pi];

    /* task_save() and the shim globals both work on the CURRENT process,
     * so this one has to become current before anything is filed away. */
    curproc = pi;
    M = p->mem;
    snprintf(p->name, sizeof p->name, "%s", nm);
    p->pid = pid; p->alive = 1; p->ntask = 1; p->cur = 0; p->rsched = 0;
    p->task[0].used = 1; p->task[0].id = 1; p->task[0].pri = 1;
    p->task[0].wait = W_NONE;

    shim_reset(p->ustbl, p->ustst);
    AC[0] = AC[1] = AC[2] = AC[3] = 0; C = 0;

    /* The .PR leaves the wide stack pointer, limit and base all equal, which
     * would make the very first push a stack fault.  AOS/VS sizes the stack
     * when it loads the program; give it the rest of the unshared area. */
    if ((DW(WSL_A) & OFFMASK) == (DW(WSB_A) & OFFMASK))
        SETDW(WSL_A, RING | ((p->ustst * 1024u - 1) & OFFMASK));

    if (!entry) {
        dword init = find_iinit();
        if (init) {
            wpushdw(RING | (pr_entry & OFFMASK));
            AC[1] = RING | (pr_entry & OFFMASK);
            entry = init;
            if (verbose)
                fprintf(stderr, "%s: I.INIT at %06X, user main in AC1 = %08X\n",
                        nm, init, AC[1]);
        } else {
            entry = pr_entry;
        }
    }
    PC = entry & OFFMASK;
    task_save();
    shim_save(pi);
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
      "  -n <count> stop after this many instructions\n"
      "  -T <mode> the player's D200 console: ansi, snap (plain-text screens at\n"
      "             each key wait), raw, off -- default ansi on a console\n"
      "  -S <file> the server program (default: QUEST_SERVER.PR beside QUEST.PR)\n"
      "  -q        no server\n"
      "  -c <file> type this file on the D200 first (QUEST.CLI's TYPE CASTLE)\n");
    exit(0);
}

/* -c: the 1984 QUEST.CLI shows CASTLE -- a D200 drawing, all cursor
 * addressing -- between ROLL DISABLE and ROLL ENABLE before it starts the
 * game, which then takes a while to come up.  The file is an animation:
 * diagonal strokes sweep the screen, then a castle, a knight and "Welcome to
 * QUEST" are drawn.  None of that is visible unless it arrives at a line's
 * pace, so at a console it is typed at 19200 baud -- about ten seconds; a
 * 9600-baud D200 took twice that.
 *
 * The roll codes are not decoration.  CASTLE's very first stroke addresses
 * column 79 of row 23 and writes a space there, and the sweeps that follow
 * keep writing the bottom-right corner: with roll enabled that wraps off the
 * bottom and scrolls the whole picture up a line each time, so the drawing
 * came apart.  With roll disabled the D200 wraps to the top instead, which is
 * what the file was made for.  Each CLI WRITE ends with a NEW LINE. */
static void type_on_d200(const char *path)
{
    FILE *f = fopen(path, "rb");
    char buf[64];
    size_t n;
    if (!f) { perror(path); return; }
    d2_write("\223\n", 2);                       /* WRITE [!ascii 223]: roll off */
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
        d2_write(buf, (int)n);
        if (term_mode == TM_ANSI) Sleep((unsigned long)(n * 1000 / 1920));
    }
    fclose(f);
    d2_write("\222\n", 2);                       /* WRITE [!ascii 222]: roll on  */
    v_show(1);
}

int main(int argc, char **argv)
{
    int i;
    dword entry = 0;
    const char *pr = NULL, *server_pr = NULL, *typefile = NULL;
    char srvbuf[512];
    int noserver = 0;

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
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) maxinstr = strtoll(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) set_datadir(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) set_savedir(argv[++i]);
        else if (!strcmp(argv[i], "-D") && i + 2 < argc) {
            dumplo = (dword)strtoul(argv[++i], NULL, 16);
            dumphi = (dword)strtoul(argv[++i], NULL, 16);
        }
        else if (!strcmp(argv[i], "-S") && i + 1 < argc) server_pr = argv[++i];
        else if (!strcmp(argv[i], "-q")) noserver = 1;
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) typefile = argv[++i];
        else if (!strcmp(argv[i], "-Z") && i + 1 < argc) frozen_clock = strtol(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-B") && i + 1 < argc && nbkpt < 16)
            bkpt[nbkpt++] = (dword)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "-BS") && i + 1 < argc && nbkpt < 16) {
            bkstr[nbkpt] = 1;
            bkpt[nbkpt++] = (dword)strtoul(argv[++i], NULL, 16);
        }
        else if (!strcmp(argv[i], "-T") && i + 1 < argc) {
            const char *m = argv[++i];
            if      (!strcmp(m, "ansi")) { term_d200 = 1; term_mode = TM_ANSI; }
            else if (!strcmp(m, "snap")) { term_d200 = 1; term_mode = TM_SNAP; }
            else if (!strcmp(m, "raw"))  { term_d200 = 1; term_mode = TM_RAW; }
            else if (!strcmp(m, "off"))  term_d200 = -1;
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
    /* QUEST is two programs.  QUP.CLI started QUEST_SERVER as a process of
     * its own before any player ran QUEST, so if the server is sitting
     * beside the client, start it too and let the scheduler run both. */
    if (!server_pr && !noserver) {
        const char *base = strrchr(pr, '/');
        const char *bs2 = strrchr(pr, (char)92);
        if (bs2 > base) base = bs2;
        base = base ? base + 1 : pr;
        if (!strcmp(base, "QUEST.PR")) {
            snprintf(srvbuf, sizeof srvbuf, "%s/QUEST_SERVER.PR", datadir_buf);
            if (file_exists(srvbuf)) server_pr = srvbuf;
        }
    }

    /* QUEST needs its D200.  Anything else keeps the plain console the
     * other games were ported against, unless -T asks. */
    if (server_pr && !term_d200) term_d200 = 1;
    if (term_d200 < 0) term_d200 = 0;
    if (term_d200) term_init();
    if (term_d200 && typefile) type_on_d200(typefile);

    nproc = 0;
    if (server_pr) {
        if (load_pr(server_pr, nproc)) return 1;
        proc_start(nproc, "QUEST.SERVER", 2, entry);
        nproc++;
    }
    if (load_pr(pr, nproc)) return 1;
    proc_start(nproc, server_pr ? "QUEST" : "MAIN", 3, entry);
    nproc++;
    /* The server goes first: it has to register its port and leave its PID
     * in the shared data before the player can look either of them up. */
    curproc = 0;
    M = proc[0].mem;
    shim_load(0);
    task_load();
    watch_snap();
    if (verbose) fprintf(stderr, "starting at %06X\n\n", PC);

    while (!halted) {
        if (PC == 0) { q_proc_exit(); if (!halted) sched_yield(); continue; }
        { int q; for (q = 0; q < npoke; q++)
              if (PC == pokepc[q]) M[MADDR(pokead[q])] = (word)pokeval[q]; }
        if (nbkpt) { int q; for (q = 0; q < nbkpt; q++) if (PC == bkpt[q]) {
            bkhits[q]++;
            fprintf(stderr, "   [bkpt %06X #%ld %s t%d  AC %08X %08X %08X %08X SP %08X FP %08X]\n",
                    PC, bkhits[q], proc[curproc].name, proc[curproc].cur,
                    AC[0], AC[1], AC[2], AC[3], DW(WSP_A), DW(WFP_A));
            if (bkstr[q]) {
                int n0 = (int)AC[0], n1 = (int)AC[1];
                if (n0 < 0 || n0 > 800) n0 = 64;
                if (n1 < 0 || n1 > 800) n1 = 64;
                show_bytes("AC2", AC[2], n0);
                show_bytes("AC3", AC[3], n1);
            } } }
        step();
        if (++icount == maxinstr && maxinstr) {
            fprintf(stderr, "\n*** instruction limit at PC=%06X\n", PC);
            break;
        }
        if (nproc > 1 && (icount % QUANTUM) == 0) sched_yield();
    }

finish:
    term_done();
    /* Whatever ran last has not been switched away from, so its view of the
     * shared files is newer than the files: write it back.  That is the
     * server tidying up after the player's obituary -- without this the
     * next session found the player still in the world and refused the
     * initials as already in use. */
    if (nproc && proc[curproc].alive) maps_sync(curproc, M, 1);
    if (nproc) sf_flush_all();
    if (verbose) fprintf(stderr, "\nstopped after %lld instructions, PC=%05X\n", icount, PC);
    if (dumphi > dumplo) {
        dword r;
        int dp;
        /* Every process's view, one after the other: comparing what two
         * processes see of the same shared page is the whole point. */
        for (dp = 0; dp < (nproc ? nproc : 1); dp++) {
            if (nproc) {
                M = proc[dp].mem;
                fprintf(stderr, "--- %s ---\n", proc[dp].name);
            }
            for (r = dumplo & ~7u; r < dumphi; r += 8) {
                unsigned j;
                fprintf(stderr, "%06X:", r);
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
    }
    return 0;
}
