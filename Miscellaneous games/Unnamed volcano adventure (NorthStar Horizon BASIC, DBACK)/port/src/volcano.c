/* volcano.c - the unnamed volcano adventure (the BASIC file DBACK on
 * 101DISK.NSI), run by the North Star BASIC it was written for - HYBASIC,
 * from the same disk - on a Z80.
 *
 * Only North Star DOS is not run: HYBASIC reaches it through the jump table
 * at 2000H, and a call anywhere into 2000H-2CFFH is done here instead:
 *
 *   200DH COUT   character in B to the terminal (device in A); returns it in A
 *   2010H CIN    a character from the terminal into A (device in A)
 *   2013H TINIT  terminal initialisation
 *   2016H CONTC  control-C check: Z if one was typed (never, here)
 *
 * The game is typed into HYBASIC at the start, a line at a time as listed
 * from DBACK (output hidden), then RUN; HYBASIC tokenises it again, and
 * --check compares that with DBACK's bytes on the disk.  A fix is a line
 * typed after the listing, replacing the author's (--no-fixes: none).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include "z80.h"
#include "data.h"

static uint8_t mem[0x10000];
static Z80 cpu;
static int trace = 0;

/* ------------------------------------------------------------------ */
/* the disk controller                                                 */
/*
 * The North Star controller answers at E800H-EBFFH.  HYBASIC reads it for
 * one thing: RND(-1) reseeds the generator by counting how long the low
 * four bits of EB30H - the number of the sector passing under the head -
 * take to change.  The disk turns at 300 rpm, ten sectors: a new one every
 * 20 ms, whatever the program does, and while the player types too.  The
 * Horizon's Z80 ran at 4 MHz; that loop takes 34 T-states a turn (6.8 an
 * instruction), so the port times the disk at 6.8 T-states an instruction
 * - about 2350 turns of the loop to a sector, as on the machine - plus the
 * real time spent waiting for the player (--seed: a fixed time instead).
 * The clock starts at RUN, so typing the program in (and the fixes) does
 * not move it.  A positive argument to RND is a seed, and repeats.
 *
 * The generator itself is a 16-bit shift register, 23 shifts a number
 * (feedback: the parity of the low byte AND 2DH), and RND is (HL+1)/65536.
 * Over the ~2350 counts the maze's INT(3.499*RND(-1))+1 lets you out into
 * the pit, the dirt passage or the narrow passage 28.4-28.8% of the time
 * each, into the crevice 14.1%: what the 3.499 asks for.
 */
static double waited = 0.0;         /* seconds spent waiting for the player */
static int fixedseed = 0;           /* --seed: the waiting time is fixed */

static uint8_t disk_status(void)
{
    double t = (double)(cpu.cycles / 4) * 6.8 / 4.0e6 + waited;
    unsigned sector = (unsigned)(t / 0.020) % 10u;
    return (uint8_t)sector;
}

uint8_t mem_rd(uint16_t a)
{
    if (a >= 0xE800 && a < 0xEC00) {
        if (a == 0xEB30) return disk_status();
        fprintf(stderr, "volcano: read of the disk controller at %04X\n", a);
        exit(3);
    }
    return mem[a];
}

void mem_wr(uint16_t a, uint8_t v) { mem[a] = v; }

uint8_t io_in(uint16_t p)
{
    fprintf(stderr, "volcano: IN %02X at %04X\n", p & 0xFF, cpu.pc);
    exit(3);
}

void io_out(uint16_t p, uint8_t v)
{
    fprintf(stderr, "volcano: OUT %02X,%02X at %04X\n", p & 0xFF, v, cpu.pc);
    exit(3);
}

/* ------------------------------------------------------------------ */
/* the terminal                                                        */

static int priming = 1;             /* typing the program in: output hidden */
static int echo = 0;                /* RUN's own new line still to drop */
static int line = 0;                /* next listing line to type */
static int fix = 0;                 /* next fix to type */
static const char *pending = NULL;  /* rest of the line being typed */
static int typedrun = 0;
static int check = 0;               /* --check: compare the program, stop */
static int basic = 0;               /* --basic: stay in BASIC after the game */
static int nofixes = 0;             /* --no-fixes: the program as on the disk */
static int console = 0;             /* the player is at a console */

/* The fixes, typed after the listing.
 *
 * 1  When the trap door opens and you are not killed, the orcs drag you up
 *    the staircase and chain you in their prison (17000-17060), and "AS
 *    YOUR EYES SLOWLY BECOME ACCOSTOMED TO THE DIM LIGHT" (17070) you see
 *    the other prisoners.  16005 goes to 17070, past the capture, so you
 *    are suddenly among prisoners in the trap-door room; nothing else
 *    reaches 17000. */
static const char *const fixes[] = {
    "16005GOTO 17000",
    0
};

static const char *next_line(void)
{
    if (listing[line]) return listing[line++];
    if (!nofixes && !check && fixes[fix]) return fixes[fix++];
    typedrun = 1;
    return "RUN";
}

/* The game's output, a line at a time: when the program ends HYBASIC says
   READY and waits for a command - there the port ends too, with exit code
   1 if the program was stopped by an error ("LENGTH ERROR IN LINE 32001":
   a move of 74 characters or more fills HYBASIC's 80-column line).  What
   is held is written before every read, so typing shows at once. */
static char held[256];
static int nheld = 0;
static char cur[128], last[128];    /* the line being written; the last one */
static int ncur = 0;

static void flush_held(void)
{
    if (nheld) fwrite(held, 1, (size_t)nheld, stdout);
    nheld = 0;
    fflush(stdout);
}

static void out(int c)
{
    c &= 0x7F;
    if (priming) return;
    if (echo && (c == '\r' || c == '\n')) {
        if (c == '\n') echo = 0;
        return;
    }
    echo = 0;
    if (c == '\r') {
        cur[ncur] = 0;
        if (!basic && !strcmp(cur, "READY")) {
            fflush(stdout);
            exit(strstr(last, " IN LINE ") ? 1 : 0);
        }
        if (ncur) memcpy(last, cur, (size_t)ncur + 1);
        ncur = 0;
    } else if (c == 0x08) {
        if (ncur) ncur--;
    } else if (c != '\n' && ncur < (int)sizeof cur - 1)
        cur[ncur++] = (char)c;
    if (nheld > (int)sizeof held - 3) flush_held();
    if (c == 0x08 && console) {     /* rubout: blank the character too */
        held[nheld++] = 0x08; held[nheld++] = ' '; held[nheld++] = 0x08;
        return;
    }
    held[nheld++] = (char)c;
    if (c == '\n') flush_held();
}

/* --check: HYBASIC has tokenised the typed program; it must be DBACK as it
   is on the disk, byte for byte */
static void check_program(void)
{
    size_t a, k;
    for (a = 0x2D00; a + sizeof dback <= sizeof mem; a++)
        if (!memcmp(mem + a, dback, sizeof dback)) {
            printf("volcano: the program HYBASIC made of the listing is DBACK "
                   "as on the disk (%u bytes at %04X)\n",
                   (unsigned)sizeof dback, (unsigned)a);
            exit(0);
        }
    /* where does it start (the first line), and where does it differ? */
    for (a = 0x2D00; a + 8 <= sizeof mem; a++)
        if (!memcmp(mem + a, dback, dback[0])) break;
    printf("volcano: the program HYBASIC made of the listing is NOT DBACK");
    if (a + 8 <= sizeof mem) {
        for (k = 0; k < sizeof dback && mem[a + k] == dback[k]; k++) ;
        printf(": it starts at %04X and differs at byte %u:\n  disk:",
               (unsigned)a, (unsigned)k);
        for (size_t j = k < 8 ? 0 : k - 8; j < k + 16 && j < sizeof dback; j++)
            printf(" %02x", dback[j]);
        printf("\n  made:");
        for (size_t j = k < 8 ? 0 : k - 8; j < k + 16; j++)
            printf(" %02x", mem[a + j]);
    }
    printf("\n");
    exit(1);
}

/* the next character typed: first the program and RUN, then the player */
static int in(void)
{
    static int lastcr = 0;
    int c;
    if (priming) {
        if (!pending) pending = next_line();
        if (typedrun && check) check_program();
        if (*pending) return (unsigned char)*pending++;
        pending = NULL;
        if (typedrun) {             /* from here on the output is the game's */
            priming = 0;
            echo = 1;
            cpu.cycles = 0;         /* and the disk's clock starts */
        }
        return '\r';
    }
    flush_held();
    for (;;) {
        clock_t t0 = clock();
        c = getchar();
        if (!fixedseed) waited += (double)(clock() - t0) / CLOCKS_PER_SEC;
        if (c == EOF) exit(0);
        c &= 0x7F;                  /* a 7-bit terminal line: */
        if (c == 0) continue;       /* NULs were dropped */
        if (c != '\n' || !lastcr) break;
        lastcr = 0;                 /* CR LF is one end of line */
    }
    lastcr = (c == '\r');
    if (c == '\n') c = '\r';
    return toupper(c);
}

/* ------------------------------------------------------------------ */
/* North Star DOS                                                      */

static uint16_t pop(void)
{
    uint16_t v = mem[cpu.sp] | (mem[(uint16_t)(cpu.sp + 1)] << 8);
    cpu.sp += 2;
    return v;
}

static void doscall(uint16_t pc)
{
    switch (pc) {
    case 0x200D:                    /* COUT */
        out(cpu.b);
        cpu.a = cpu.b;
        break;
    case 0x2010:                    /* CIN */
        cpu.a = (uint8_t)in();
        break;
    case 0x2013:                    /* TINIT */
        break;
    case 0x2016:                    /* CONTC: NZ, no control-C typed */
        cpu.a = 0xFF;
        cpu.f &= (uint8_t)~FLAG_Z;
        break;
    default:
        fprintf(stderr, "volcano: HYBASIC called DOS at %04X (from %04X)\n",
                pc, mem[cpu.sp] | (mem[cpu.sp + 1] << 8));
        exit(3);
    }
    if (trace) fprintf(stderr, "DOS %04X A=%02X B=%02X\n", pc, cpu.a, cpu.b);
    cpu.pc = pop();
}

/* --sample: where the Z80 spends its time, every 20 million instructions
   once the game runs (for finding a loop) */
static unsigned hist[0x10000];

static void sample(void)
{
    static unsigned long long n = 0;
    int j, k;
    if (!priming) hist[cpu.pc]++;
    if (++n % 20000000ULL || priming) return;
    fprintf(stderr, "--- %llu instructions, SP=%04X\n", n, cpu.sp);
    for (j = 0; j < 12; j++) {
        unsigned m = 0;
        int bi = 0;
        for (k = 0; k < 0x10000; k++)
            if (hist[k] > m) { m = hist[k]; bi = k; }
        fprintf(stderr, "  %04X %u\n", bi, m);
        hist[bi] = 0;
    }
    memset(hist, 0, sizeof hist);
}

/* at a console, keys go to HYBASIC one at a time: it echoes them and does
   its own rubout; the console's own mode comes back at the end */
static HANDLE hin;
static DWORD oldmode;

static void restore_console(void)
{
    if (console) SetConsoleMode(hin, oldmode);
}

static BOOL WINAPI on_break(DWORD type)
{
    (void)type;
    restore_console();
    return FALSE;                   /* and the program ends as usual */
}

static void usage(FILE *f)
{
    fprintf(f,
        "usage: volcano [--seed MS] [--basic] [--no-fixes]\n"
        "\n"
        "The unnamed volcano adventure (DBACK, 101DISK.NSI), run by North\n"
        "Star BASIC (HYBASIC, from the same disk) on an emulated Z80.\n"
        "\n"
        "  --seed MS      the time you take to type counts as MS milliseconds\n"
        "                 (fractions too; a sector passes every 20): the\n"
        "                 maze's RND(-1) then repeats\n"
        "  --basic        stay in BASIC (READY) when the game ends\n"
        "  --no-fixes     the program exactly as on the disk\n"
        "  -u, --unlimited  accepted; the game has no limits to lift\n"
        "  --check        compare the program HYBASIC tokenised with DBACK\n"
        "  --direct       BASIC alone, without the game\n"
        "  --trace, --sample  debugging\n"
        "  -h, --help     this\n");
}

int main(int argc, char **argv)
{
    int i, sampling = 0;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--trace")) trace = 1;
        else if (!strcmp(argv[i], "--direct")) priming = 0, basic = 1;
        else if (!strcmp(argv[i], "--sample")) sampling = 1;
        else if (!strcmp(argv[i], "--check")) check = 1;
        else if (!strcmp(argv[i], "--basic")) basic = 1;
        else if (!strcmp(argv[i], "--no-fixes")) nofixes = 1;
        else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--unlimited")) ;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(stdout);
            return 0;
        } else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            fixedseed = 1;
            waited = atof(argv[++i]) / 1000.0;
        } else {
            fprintf(stderr, "volcano: unknown option %s (volcano --help "
                    "lists them)\n", argv[i]);
            return 2;
        }
    }
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    hin = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode(hin, &oldmode)) {
        console = 1;
        atexit(restore_console);
        SetConsoleCtrlHandler(on_break, TRUE);
        SetConsoleMode(hin, oldmode & ~(DWORD)(ENABLE_ECHO_INPUT |
                                               ENABLE_LINE_INPUT));
    }
    memcpy(mem + 0x2000, dos, sizeof dos);
    memcpy(mem + 0x2D00, hybasic, sizeof hybasic);
    z80_reset(&cpu);
    cpu.pc = 0x2D00;                /* HYBASIC's cold start */
    cpu.sp = 0x2D00;
    for (;;) {
        if (cpu.pc >= 0x2000 && cpu.pc < 0x2D00) {
            doscall(cpu.pc);
            continue;
        }
        z80_step(&cpu);
        if (sampling) sample();
        if (cpu.halted) {
            fprintf(stderr, "volcano: HALT at %04X\n", cpu.pc);
            return 3;
        }
    }
}
