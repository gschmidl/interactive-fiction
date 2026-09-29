/* cpu.c -- 16-bit Data General Eclipse CPU, enough of it to run THISSALA.
 *
 *   thissala [-t] [-e <entry>] [-n <maxinstr>] [-s <startpc>]
 *
 * Diagnostic-first: anything not implemented halts with the PC, the
 * instruction word and a short dump, so the machine tells us what it needs
 * instead of us guessing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <conio.h>                 /* _getch                               */
#include <io.h>                    /* _isatty, _fileno                     */
#include "eclipse.h"
#include "fpu.h"

/* ---------------------------------------------------------------- state */
static word M[MEMWORDS];
static word AC[4];
static int  C;                     /* carry, 0 or 1                      */
static word PC;
static int  halted, trace, catch_systm;
static int  verbose, permissive;
static int  ovl_force = -1;
static int  lef_mode = 1;   /* ?LEFE turns the I/O opcodes into LEF */
static word bp_addr[8]; static int bp_n; static long bp_hits;
static int  bp_stop;  static const char *wrimg;
static long icount, maxinstr = 0;

/* Eclipse stack lives in page zero: 0o40 SP, 0o41 FP, 0o42 SL, 0o43 SFA */
#define SP  M[040]
#define FP  M[041]
#define SL  M[042]
#define SFA M[043]

#include "debug.h"

static const char *ALU[8] = {"COM","NEG","MOV","INC","ADC","SUB","ADD","AND"};

static void die(const char *why, word ir)
{
    fprintf(stderr,
        "\n*** %s\n"
        "    PC=%04X  IR=%04X\n"
        "    AC0=%04X AC1=%04X AC2=%04X AC3=%04X  C=%d\n"
        "    SP=%04X FP=%04X SL=%04X\n"
        "    after %ld instructions\n",
        why, PC, ir, AC[0], AC[1], AC[2], AC[3], C, SP, FP, SL, icount);
    halted = 1;
}

/* ------------------------------------------------------------- loading */
static int load_image(const char *path, unsigned base)
{
    FILE *f = fopen(path, "rb");
    unsigned char b[2];
    unsigned a = base;
    if (!f) { perror(path); return -1; }
    while (fread(b, 1, 2, f) == 2 && a < MEMWORDS) {
        M[a++] = (word)((b[0] << 8) | b[1]);
    }
    fclose(f);
    if (verbose) fprintf(stderr, "loaded %-10s %04X..%04X\n", path, base, a - 1);
    return 0;
}

/* ---------------------------------------------------------------------
 * AOS/VS program load.
 *
 * PLOT.PR is a flat image of 0x7C00 words with no separate header block:
 *
 *   1. the whole file maps to memory 0x0400..0x7FFF, so the shared (read
 *      only) half lands at 0x3000..0x7FFF -- USTST=12 blocks in, USTSZ=20
 *      blocks long, 1024 words to the block;
 *   2. the impure image, file words 0x2000.. (== memory 0x2400 after step
 *      1), is then copied down over memory 0x0000..0x07FF.  That is
 *      USTBL = 2 blocks, and it carries page zero, the runtime's own
 *      variables and the startup code;
 *   3. the UST template, file words 0x0100..0x0125, goes to memory 0x0100.
 *
 * Every one of those was read back out of the live machine: page zero, the
 * overlay database at 0x126, M[0400]=0020 and M[0403]=0404 from the impure
 * image, and USTOD=0127 from the UST.
 */
#define UST        0x100u        /* PARU: UST = 400 octal                 */
#define USTBL      (UST + 0x0Cu) /* # impure 1024-word blocks             */
/* The overlay directory's UST slot: 0x0D in an AOS/VS 16-bit program,
 * 0x10 in an original AOS one -- see load_pr_aos. */
static unsigned ustod_off = 0x0Du;
static char progbase[256] = "";      /* "ADVENTURE" for ADVENTURE.PR       */
static const char *prog_args[16];     /* [0] the program, then its CLI arguments */
static int prog_nargs;
#define USTOD_W    (UST + ustod_off)  /* overlay directory address        */
#define USTST      (UST + 0x0Fu) /* shared area's starting block          */
#define PAGEW      1024u         /* words in an AOS/VS 16-bit block       */

static unsigned cur_pages, max_pages;
static unsigned pr_entry;              /* from the .PR header, see load_pr */

static unsigned pzimage = 0x2400;      /* only used by the -D dump path   */
static unsigned pzwords = 0x800;

static void build_low_memory(void)
{
    unsigned n;
    word ustsave[0x26];
    for (n = 0; n < 0x26; n++) ustsave[n] = M[0x500 + n];   /* file 0x100.. */
    for (n = 0; n < pzwords; n++) M[n] = M[pzimage + n];
    for (n = 0; n < 0x26; n++) M[UST + n] = ustsave[n];
    cur_pages = M[USTBL];
    max_pages = M[USTST];
    if (!cur_pages) cur_pages = 2;
    if (!max_pages) max_pages = 12;
}

/* ---------------------------------------------------------------------
 * AOS/VS program load, taken from the UST instead of from constants.
 * Checked against PLOT.PR, ADVENTURE.PR, ZORK.PR and FERRET.PR -- in all
 * four the block arithmetic closes exactly on the top of the address space:
 *
 *   file blocks 0..7    header; the UST template is at file word 0x100
 *   file word 0x2000    the impure image, USTBL blocks, loads at memory 0
 *   the LAST USTSZ blocks of the file are the shared, read-only half and
 *                       they load at memory block USTST
 *
 * so unshared memory runs 0..USTST*1024-1 with only its first USTBL blocks
 * initialised; the rest is the heap that ?MEMI hands out, and it must start
 * as zeros, not as whatever the file happens to hold there.  (USTSH, the
 * "physical starting page of shared area in .PR", reads 0 in all four, so it
 * is the last-blocks rule that actually places the shared half.)
 *
 * Proof for ADVENTURE.PR, whose 10 impure blocks rule out simply mapping the
 * whole file at 0x400 the way PLOT.PR allowed: the .SYSTM thunk that page
 * zero names at M[15]=717A is only the real thunk -- FE48 A6C9 3000 ... 3821
 * 9FC8, the same opening as PLOT.PR's at 7CBC -- when the shared half is
 * placed as the last 20 blocks.
 */
/* ---------------------------------------------------------------------
 * An original AOS program file -- the pre-AOS/VS format, file type ?FPRG
 * (65), not ?FPRV.  ADVENTURE.PR from the AOS games tapes is one.  It is not
 * the AOS/VS layout with a header and the halves placed separately: it is
 * the 32K-word address space itself, exactly 0x8000 words, so file word N is
 * memory word N.  Page zero is live in it (M[15] = 7C81 is the .SYSTM thunk),
 * the overlay area is present but zero, and the UST at 0x100 is the older,
 * contiguous one, without the 32-bit gaps AOS/VS put into the 16-bit table:
 *
 *   +0B USTTC tasks (1)          +10 USTOD overlay directory (0128)
 *   +0F USTBL impure blocks (3)  +11 USTST shared start block (16)
 *   +12 USTSZ shared blocks (16) +1D start address (02C6 = .F5INIT)
 *
 * Read off the data, and every field checks against something else in the
 * file: 3 impure blocks end where the unshared zeros begin at 0B6D; 16 + 16
 * blocks close exactly on 0x8000; the directory at 0128 names 13 overlays
 * of 1024 words based at 6800, which is the .OL's 26624 bytes and the
 * address the symbol table gives every overlaid routine; and the start
 * address is the symbol .F5INIT, the FORTRAN 5 runtime's initialiser. */
static int load_pr_aos(const word *fw, unsigned nw, const char *path)
{
    unsigned i;
    {   /* the program's own name: its overlays are <name>.OL */
        const char *b = strrchr(path, '/'), *b2 = strrchr(path, (char)92), *dot;
        size_t k;
        if (b2 > b) b = b2;
        b = b ? b + 1 : path;
        dot = strrchr(b, '.');
        k = dot ? (size_t)(dot - b) : strlen(b);
        if (k >= sizeof progbase) k = sizeof progbase - 1;
        memcpy(progbase, b, k); progbase[k] = 0;
    }
    memset(M, 0, sizeof M);
    for (i = 0; i < nw && i < MEMWORDS; i++) M[i] = fw[i];
    ustod_off = 0x10u;
    cur_pages = fw[0x100 + 0x0F];
    max_pages = fw[0x100 + 0x11];
    pr_entry  = fw[0x100 + 0x1D];
    if (!max_pages) max_pages = 16;
    if (verbose)
        fprintf(stderr, "%s: original AOS program.  impure %u blk, shared from "
                        "blk %u for %u, USTOD=%04X, start %04X, @15=%04X\n",
                path, cur_pages, max_pages, fw[0x100 + 0x12], M[USTOD_W],
                pr_entry, M[15]);
    return 0;
}

static int load_pr(const char *path)
{
    FILE *f = fopen(path, "rb");
    word *fw;
    long  nb;
    unsigned nw, nblk, bl, st, sz, sblk, i;

    if (!f) { perror(path); return -1; }
    fseek(f, 0, SEEK_END); nb = ftell(f); rewind(f);
    nw = (unsigned)(nb / 2);
    if (nw < 9 * PAGEW) { fprintf(stderr, "%s: too short for a .PR\n", path);
                          fclose(f); return -1; }
    fw = (word *)malloc((size_t)nw * sizeof *fw);
    if (!fw) { fclose(f); return -1; }
    for (i = 0; i < nw; i++) {
        int hi = fgetc(f), lo = fgetc(f);
        fw[i] = (word)((hi << 8) | (lo & 0xFF));
    }
    fclose(f);

    /* The original AOS format: the address space itself, and no AOS/VS
     * program-type flag in the UST. */
    if (nw == 0x8000u && !(fw[0x100 + 0x14] & 0x8000)) {
        int r = load_pr_aos(fw, nw, path);
        free(fw);
        return r;
    }

    nblk = nw / PAGEW;
    bl = fw[0x100 + 0x0C];              /* USTBL  impure blocks           */
    st = fw[0x100 + 0x0F];              /* USTST  shared starting block   */
    sz = fw[0x100 + 0x13];              /* USTSZ  shared size in blocks   */
    if (!(fw[0x100 + 0x14] & 0x8000)) {
        fprintf(stderr, "%s: USTPR=%04X -- this is a 32-bit program, and this\n"
                        "    emulator is the 16-bit Eclipse one.\n",
                path, fw[0x100 + 0x14]);
        free(fw); return -1;
    }
    if (!bl || !st || !sz || sz > nblk || st + sz > MEMWORDS / PAGEW) {
        fprintf(stderr, "%s: UST makes no sense (USTBL=%u USTST=%u USTSZ=%u)\n",
                path, bl, st, sz);
        free(fw); return -1;
    }
    sblk = nblk - sz;                   /* shared = the last USTSZ blocks */

    memset(M, 0, sizeof M);
    for (i = 0; i < sz * PAGEW; i++)
        M[st * PAGEW + i] = fw[sblk * PAGEW + i];
    for (i = 0; i < bl * PAGEW && 0x2000 + i < nw; i++)
        M[i] = fw[0x2000 + i];
    for (i = 0; i < 0x26; i++)          /* UST template, file 0x100..0x125 */
        M[UST + i] = fw[0x100 + i];

    /* The entry point is a ring-qualified 32-bit address at file word 0x17C,
     * in the .PR header past the UST template.  PLOT.PR reads 700002F6 --
     * 02F6 is exactly the entry that had to be found by hand for Thissala --
     * ADVENTURE.PR 70002347, ZORK.PR 70075530, FERRET.PR 700535E8.  Ring 7
     * is where AOS/VS runs user code; the offset is a word address. */
    pr_entry = (((unsigned)fw[0x17C] << 16) | fw[0x17D]) & 0x0FFFFFFFu;
    free(fw);

    cur_pages = bl;
    max_pages = st;
    if (verbose) {
        fprintf(stderr, "%s: %u blocks.  impure %u blk -> 0000..%04X, "
                        "shared %u blk (file blk %u) -> %04X..%04X\n",
                path, nblk, bl, bl * PAGEW - 1, sz, sblk,
                st * PAGEW, (st + sz) * PAGEW - 1);
        fprintf(stderr, "  page zero: SP=%04X FP=%04X SL=%04X SFA=%04X "
                        "@12=%04X @15=%04X  USTOD=%04X\n",
                SP, FP, SL, SFA, M[12], M[15], M[USTOD_W]);
        fprintf(stderr, "  entry %05X\n", pr_entry);
    }
    return 0;
}

/* Load a slice of the overlay file: <nwords> words starting at file word
 * <fword>, placed at word address <base>.  Overlay code is entirely
 * PC-relative, so any base works. */
static int load_overlay(const char *path, unsigned fword, unsigned base, unsigned nwords)
{
    FILE *f = fopen(path, "rb");
    unsigned char b[2];
    unsigned a = base, k = 0;
    if (!f) { perror(path); return -1; }
    if (fseek(f, (long)fword * 2, SEEK_SET)) { fclose(f); return -1; }
    while (k < nwords && fread(b, 1, 2, f) == 2 && a < MEMWORDS) {
        M[a++] = (word)((b[0] << 8) | b[1]); k++;
    }
    fclose(f);
    fprintf(stderr, "overlay %s[%04X..] -> %04X..%04X (%u words)\n",
            path, fword, base, a - 1, k);
    return 0;
}

/* ---------------------------------------------------- effective address */
static word wlo, whi;

/* -g holds the game own ASSIST gate on.  Arming it before each console read
 * is not enough on its own: an overlay routine clears the word again between
 * the read and the parse, so the verbs would be refused anyway.  Nothing else
 * writes this address, so pinning it costs the game nothing. */
static word dbghold(word a, word v)
{
    if (debug && a == DEBUG_FLAG) return 1;
    return v;
}

static void watchwr(word a, word at, word v, const char *how)
{
    if (whi && a >= wlo && a <= whi)
        fprintf(stderr, "[%s %04X (+%d) <- %04X  PC=%04X]\n",
                how, a, (int)(a - wlo), v, at);
}

static void watchrd(word a, word at, const char *how)
{
    if (whi && a >= wlo && a <= whi)
        fprintf(stderr, "[%s %04X (+%d) = %04X  PC=%04X]\n",
                how, a, (int)(a - wlo), M[a], at);
}

static word indirect(word a)
{
    int hops = 0;
    for (;;) {
        word v;
        if      (a >= 020 && a <= 027) M[a] = (word)(M[a] + 1);
        else if (a >= 030 && a <= 037) M[a] = (word)(M[a] - 1);
        v = M[a];
        if (!(v & 0x8000)) return (word)(v & AMASK);
        a = (word)(v & AMASK);
        if (++hops > 16) { die("indirection loop", 0); return a; }
    }
}

static word ea(word ir, word at)
{
    word a;
    switch (MR_IDX(ir)) {
    case 0: a = MR_DISP(ir); break;
    case 1: a = (word)((at + disp8(ir)) & AMASK); break;
    case 2: a = (word)((AC[2] + disp8(ir)) & AMASK); break;
    default:a = (word)((AC[3] + disp8(ir)) & AMASK); break;
    }
    if (MR_IND(ir)) a = indirect(a);
    return a & AMASK;
}

/* ------------------------------------------------------- Eclipse stack */
static void push(word v) { SP = (word)((SP + 1) & AMASK); M[SP] = v; }
static word pop(void)    { word v = M[SP]; SP = (word)((SP - 1) & AMASK); return v; }

static void do_save(word frame)
{
    push(AC[0]); push(AC[1]); push(AC[2]); push(FP);
    push((word)((C ? 0x8000 : 0) | (AC[3] & AMASK)));
    FP = SP;
    AC[3] = FP;                 /* saved AC0 then sits at FP-4 */
    SP = (word)((SP + frame) & AMASK);
}

/* POPB pops the five-word return block -- PC (bit 15 is carry), FP, AC2,
 * AC1, AC0 -- and then re-establishes the DG frame convention by setting
 * AC3 to the restored FP.  Callers rely on that: the game's routines use
 * AC3 as the frame pointer immediately after a call returns, without
 * reloading it from page-zero 41.  RTN is POPB with SP set from FP first.
 */
static void do_popb(void)
{
    word t;
    t = pop(); C = (t & 0x8000) ? 1 : 0; PC = t & AMASK;
    FP    = pop();
    AC[2] = pop();
    AC[1] = pop();
    AC[0] = pop();
    AC[3] = FP;
}

static void do_rtn(void)
{
    SP = FP;
    do_popb();
}

/* ---- Stack faults --------------------------------------------------------
 * Page-zero 042 is the stack limit and 043 the fault handler.  DG.PR relies
 * on them: its image starts with SP = FP = 01A0 and the limit at 01A1, so the
 * very first SAVE -- .MAIN's own, 1383 words -- overflows, and the "handler"
 * at 7993 is the FORTRAN runtime's memory initialiser, which sizes memory
 * with ?MEM/?MEMI, builds the real stack at 2058 and goes back to the SAVE.
 * Without the fault the program ran with its stack at 01A0, straight over
 * the overlays' constants at 08F0.
 *
 * Programmer's Reference, ECLIPSE Line (015-000024) 3-18..3-22 and ECLIPSE
 * S/140 (014-000642) 2-14/2-15: SAVE and MSP check before they execute and
 * are not executed, the saved PC being their own address so the handler's
 * POPB re-runs them; PSH, PSHR, PSHJ, XOP and SYC complete first and save
 * the next PC.  Overflow is SP > limit, unsigned.  The fault clears SP's
 * bit 0 and sets the limit's (so its own push cannot fault again -- the
 * handler puts both right), pushes AC0 AC1 AC2 AC3 and carry+PC, and does
 * JMP @43.  Underflow -- after a pop, SP below 0400 with the limit's bit 0
 * clear -- first sets SP to the limit. */

static void stack_fault(word pc)
{
    if (verbose)
        fprintf(stderr, "   [stack fault: SP=%04X limit=%04X, return to %04X, handler @43=%04X]\n",
                SP, SL, pc, M[043]);
    SP = (word)(SP & 0x7FFF);
    SL = (word)(SL | 0x8000);
    push(AC[0]); push(AC[1]); push(AC[2]); push(AC[3]);
    push((word)((C ? 0x8000 : 0) | (pc & AMASK)));
    PC = indirect(043);
}

/* After a push-type instruction; pc is where execution goes next. */
static void stack_check(word pc)
{
    if (SP > SL) stack_fault(pc);
}

/* After a pop-type instruction. */
static void stack_check_under(word pc)
{
    if (SP < 0400 && !(SL & 0x8000)) { SP = SL; stack_fault(pc); }
}

/* ---- The character instructions: CMV CMP CTR CMT (EBID.SR 153650,
 * 157650, 163650, 167650; "character option on S series, standard on M & C
 * series").  From the ECLIPSE S/140 Programmer's Reference 4-43..4-47.
 * Strings are byte pointers with signed lengths: a negative length runs
 * the string downwards from the pointer. */
#define E1_CMV 0xD7A8u
#define E1_CMP 0xDFA8u
#define E1_CTR 0xE7A8u
#define E1_CMT 0xEFA8u

static int bget(word bp)
{
    word w = M[(bp >> 1) & AMASK];
    return (bp & 1) ? (w & 0xFF) : (w >> 8);
}

static void bput(word bp, int b)
{
    word a = (word)((bp >> 1) & AMASK);
    if (bp & 1) M[a] = (word)((M[a] & 0xFF00) | (b & 0xFF));
    else        M[a] = (word)((M[a] & 0x00FF) | ((b & 0xFF) << 8));
}

static void char_instruction(word ir)
{
    int16_t n0 = (int16_t)AC[0], n1 = (int16_t)AC[1];
    int d0 = n0 < 0 ? -1 : 1, d1 = n1 < 0 ? -1 : 1;
    int c0 = n0 < 0 ? -n0 : n0, c1 = n1 < 0 ? -n1 : n1;
    word p2 = AC[2], p3 = AC[3];

    switch (ir) {
    case E1_CMV:
        /* AC0/AC2 the destination, AC1/AC3 the source; a short source is
         * padded with spaces, a long one sets carry. */
        C = c1 > c0;
        while (c0) {
            int b = ' ';
            if (c1) { b = bget(p3); p3 = (word)(p3 + d1); c1--; }
            bput(p2, b); p2 = (word)(p2 + d0); c0--;
        }
        AC[0] = 0;
        AC[1] = (word)(d1 * c1);
        break;

    case E1_CMP: {
        /* AC0/AC2 string 2, AC1/AC3 string 1; AC1 = -1, 0 or 1 as string 1
         * is less, equal or greater, the shorter padded with spaces. */
        int r = 0;
        while (c0 || c1) {
            int b1 = c1 ? bget(p3) : ' ', b2 = c0 ? bget(p2) : ' ';
            if (b1 != b2) { r = b1 < b2 ? -1 : 1; break; }
            if (c1) { p3 = (word)(p3 + d1); c1--; }
            if (c0) { p2 = (word)(p2 + d0); c0--; }
        }
        AC[0] = (word)(d0 * c0);
        AC[1] = (word)r;
        break;
    }

    case E1_CMT: {
        /* AC0 the 16-word delimiter bit table (perhaps indirect), AC1/AC3
         * the source, AC2 the destination, both in the same direction.
         * Stops before copying a delimiter. */
        word t = AC[0];
        while (t & 0x8000) t = M[t & AMASK];
        while (c1) {
            int b = bget(p3);
            if (M[(t + (b >> 4)) & AMASK] & (0x8000u >> (b & 15))) break;
            bput(p2, b);
            p2 = (word)(p2 + d1); p3 = (word)(p3 + d1); c1--;
        }
        AC[0] = t;
        AC[1] = (word)(d1 * c1);
        break;
    }

    case E1_CTR: {
        /* AC0 addresses (perhaps indirectly) a word holding a byte pointer
         * to a 256-byte table.  AC1 negative: translate string 1 (AC3) into
         * string 2 (AC2); positive: translate both and compare. */
        word a = AC[0], tb;
        int r = 0;
        while (a & 0x8000) a = M[a & AMASK];
        tb = M[a & AMASK];
        if (n1 < 0) {
            while (c1) {
                bput(p2, bget((word)(tb + bget(p3))));
                p2++; p3++; c1--;
            }
        } else {
            while (c1) {
                int t1 = bget((word)(tb + bget(p3))), t2 = bget((word)(tb + bget(p2)));
                if (t1 != t2) { r = t1 < t2 ? -1 : 1; break; }
                p2++; p3++; c1--;
            }
        }
        AC[0] = a;
        AC[1] = (word)r;
        break;
    }
    }
    AC[2] = p2;
    AC[3] = p3;
}

/* ------------------------------------------------------------ .SYSTM  */
/* JSR @15 / call-code / error-return / normal-return.  Unimplemented for
 * now: report the call and stop, so the inventory builds itself. */

/* ---------------------------------------------------------- system calls
 * Codes from :UTIL:SYSID.16.SR, packet layout from PARU.16.SR.  The macro
 * ?SCL2 loads the packet address into AC2 (ELEF 2,packet) before the call.
 *
 *   general user I/O packet (?OPEN / ?READ / ?WRITE / ?CLOSE)
 *     0 ?ICH  channel      3 ?IBAD byte ptr to buffer   7 ?IRNH rec no hi
 *     1 ?ISTI status in    5 ?IRCL record length        8 ?IRNL rec no lo
 *     2 ?ISTO file type    6 ?IRLR length returned      9 ?IFNP byte ptr filename
 *                                                      10 ?IMRS physical rec size-1
 */
#define SC_CREATE  0
#define SC_DELETE  1
#define SC_MEM     3
#define SC_GHRZ   60         /* 0074 get clock frequency code           */
#define SC_LEFE  181         /* 0265 enable LEF mode                    */
#define SC_LEFD  182         /* 0266 disable LEF mode                   */
#define SC_LEFS  183         /* 0267 LEF status                         */
#define SC_IFPU  354         /* 0542 initialise floating point unit     */
#define SC_ERMSG 201         /* 0311 error message text                 */
#define SC_CTYPE  27
#define SC_STOM  256         /* 0400, part of the console setup      */         /* 0033 console type                       */
#define SC_GTMES 199         /* 0307 get CLI message                    */

#define SC_MEMI   12
#define SC_DELAY  13
#define SC_GTOD   30
#define SC_GDAY   33
#define SC_AGENT0 184
#define SC_AGENT1 185
#define SC_AGENT2 186        /* 0272, agent-reserved: resolve routine addr */
#define SC_AGENT3 187         /* 0273, resource return: no PC adjustment */
#define SC_AGENT4 188
#define SC_GPOS  198        /* 0306, agent get position               */
#define SC_SPOS  210        /* 0322, agent set position               */
#define SC_OPEN  192
#define SC_CLOSE 193
#define SC_READ  194
#define SC_WRITE 195
#define SC_RETURN 200
#define SC_GCHR  202
#define SC_SCHR  203
#define SC_KILL  324
#define SC_CISND  50         /* 0062 enable console interrupt sending */

#define P_ICH  0
#define P_ISTI 1
#define P_IFLG 4
#define P_IBAD 3
#define P_IRCL 5
#define P_IRLR 6
#define P_IRNH 7
#define P_IRNL 8
#define P_IFNP 9
#define P_IMRS 10

static FILE *chan[64];
static int   chan_is_ovl[64];     /* the channel the AOS agent opened on <prog>.OL */
static int   chan_console[64];
static long  chan_pos[64];
static int   chan_spos[64];     /* a ?SPOS is pending on this channel */      /* ?SPOS / ?GPOS, in records       */
/* Files opened with ?SOPEN (AOS shared files).  A ?SPAGE of one of them puts
 * blocks of the file into memory; the mapping is kept so the words can be
 * written back when the program flushes, closes or exits. */
static int   chan_shared[64];
static char  shared_name[64][288];
static struct { int used, ch; word addr; long blk; unsigned cnt; } smap[16];
static void smap_flush(int ch, int release);
/* Where the original AOS/VS files live.  PLOT.PR, PLOT.OL, THISSALA.DB1..DB8
 * and THISSALA.HELP are all read from here and never written. */
static char datadir_buf[512] = "";
static const char *datadir = datadir_buf;
static char olpath_buf[512] = "";
static const char *olpath  = olpath_buf;

/* Load node/overlay per the directory at USTOD (see PARU "USER OVERLAY DATA
 * BASE").  Returns the area base address, or 0 on failure.
 *   node desc:  +1 ?NDOVS  +2/+3 ?NDFHI/?NDFLO (file block)  +4 ?NDASZ
 *               (256-word blocks, bit15 = shared)   +5.. area descriptors
 *   area desc:  +0 ?ARNOD  +1 ?ARBAS  +2 ?AROUC                            */
#define MAXNODE 8
static word     area_base[MAXNODE];
static unsigned area_words[MAXNODE];
static int      resident[MAXNODE];      /* overlay number, -1 = area empty  */
static int      geom_done;

/* Read ?ARBAS and ?NDASZ out of the overlay database so an address can be
 * mapped back to the node whose area it lies in. */
static void overlay_geometry(void)
{
    word ustod = M[USTOD_W];
    unsigned k;
    if (geom_done) return;
    if (!ustod) ustod = 0x127;
    for (k = 0; k < MAXNODE; k++) {
        word nd = M[(ustod + k) & AMASK];
        resident[k] = -1;
        if (nd < 0x100 || nd >= 0x400) { area_words[k] = 0; continue; }
        area_words[k] = (M[(nd + 4) & AMASK] & 0x7FFF) * 256u;
        area_base[k]  = M[(nd + 6) & AMASK];
    }
    geom_done = 1;
}

/* Which node's overlay area holds this address?  -1 for the root. */
static int area_of(word addr)
{
    unsigned k;
    overlay_geometry();
    for (k = 0; k < MAXNODE; k++)
        if (area_words[k] && addr >= area_base[k] &&
            addr < area_base[k] + area_words[k])
            return (int)k;
    return -1;
}

static word load_overlay_res(unsigned node, unsigned ovl)
{
    word ustod = M[(0x500 + 13) & AMASK];      /* UST+13, if the UST is still mapped */
    if (!ustod) ustod = 0x127;                 /* at runtime only low memory survives */
    word nd    = M[(ustod + node) & AMASK];
    word blks  = (word)(M[(nd + 4) & AMASK] & 0x7FFF);
    unsigned fileblk = ((unsigned)M[(nd + 2) & AMASK] << 16) | M[(nd + 3) & AMASK];
    word base  = M[(nd + 6) & AMASK];          /* area descriptor +1 = ?ARBAS */
    unsigned words = blks * 256u;
    unsigned fword = fileblk * 256u + ovl * words;
    FILE *f = fopen(olpath, "rb");
    unsigned a = base, k = 0;
    unsigned char t[2];
    if (!f) { perror(olpath); return 0; }
    if (fseek(f, (long)fword * 2, SEEK_SET)) { fclose(f); return 0; }
    while (k < words && fread(t, 1, 2, f) == 2 && a < MEMWORDS) {
        M[a++] = (word)((t[0] << 8) | t[1]); k++;
    }
    fclose(f);
    /* Mark the area occupied, or the runtime sees ?AREPY and loads it again
     * itself.  ?AROUC: bit15 loading, bits14-6 overlay #, bits5-0 use count. */
    M[(nd + 7) & AMASK] = (word)(((ovl & 0x1FF) << 6) | 1);
    resident[node & (MAXNODE - 1)] = (int)ovl;
    if (verbose) fprintf(stderr, "[overlay node %u ovl %u: file word %05X, %u words -> %04X]\n",
            node, ovl, fword, k, base);
    return base;
}

/* The shipped game files are read-only: reads look in the save directory
 * first (so a restored game is found) and fall back to the data directory,
 * while creates, writes and deletes only ever touch the save directory.
 * Nothing the game does can reach THISSALA.DB* or PLOT.* that way. */
static const char *savedir = ".";

static int file_exists(const char *p)
{
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fclose(f); return 1;
}

static void save_path(char *out, size_t n, const char *name)
{
    snprintf(out, n, "%s/%s", savedir, name);
}

static void read_path(char *out, size_t n, const char *name)
{
    save_path(out, n, name);
    if (file_exists(out)) return;
    snprintf(out, n, "%s/%s", datadir, name);
}

static char *getbstr(word bp, char *buf, int max)
{
    int i = 0;
    while (i < max - 1) {
        word w = M[(bp >> 1) & AMASK];
        int c = (bp & 1) ? (w & 0xFF) : (w >> 8);
        if (!c) break;
        buf[i++] = (char)c; bp++;
    }
    buf[i] = 0;
    return buf;
}

/* Copy exactly len bytes from a byte pointer; ?READ/?WRITE are counted,
 * not NUL-terminated. */
static void getbytes(word bp, char *dst, int len)
{
    int i;
    for (i = 0; i < len; i++, bp++) {
        word w = M[(bp >> 1) & AMASK];
        dst[i] = (char)((bp & 1) ? (w & 0xFF) : (w >> 8));
    }
}

static void putbstr(word bp, const char *src, int len)
{
    int i;
    for (i = 0; i < len; i++, bp++) {
        word a = (word)((bp >> 1) & AMASK);
        if (bp & 1) M[a] = (word)((M[a] & 0xFF00) | (unsigned char)src[i]);
        else        M[a] = (word)((M[a] & 0x00FF) | ((unsigned char)src[i] << 8));
    }
}

/* PARU: ?ISTI carries ?ICRF (change record format) plus the three format
 * bits; without ?ICRF the file's own format applies, and everything this
 * game opens that way is read with an explicit format anyway. */
#define IFLG_IPST 0x2000u        /* ?IPST: record number is absolute        */
#define ICRF      0x4000u
#define OFOT      0x0008u        /* ?OFOT: open for output                  */        /* ?ICRF: use the format bits below        */
#define RF_DY 1                  /* ?ORDY dynamic                           */
#define RF_DS 2                  /* ?ORDS data sensitive                    */
#define RF_FX 3                  /* ?ORFX fixed length                      */

/* Without ?ICRF the file's own record format applies.  Everything this game
 * opens on disk it then reads back with an explicit format, so the only
 * channel that ever arrives here without one is the console -- and that is
 * data sensitive, a line at a time.
 *
 * The delimiters are NULL, NEW LINE and FORM FEED.  Carriage return is NOT
 * one of them here: the game's own line records end "\r\n", and treating the
 * CR as the end of the record splits every line in two. */
static int rec_format(word pkt, int ch)
{
    word sti = M[(pkt + P_ISTI) & AMASK];
    if (sti & ICRF) return (int)(sti & 7);
    return chan_console[ch] ? RF_DS : RF_FX;
}

static int is_delim(int c) { return c == 0 || c == '\n' || c == '\f'; }

/* The console as the terminal an AOS program expected.
 *
 * Input: a DG console whose characteristics lack /ULC ("upper and lower
 * case") hands the program capitals whatever was typed.  The 500-point
 * Adventure only understands capitals -- typed "no" gets "Please answer the
 * question." -- so fold the case unless -L asks for lower case.
 *
 * Output: the FORTRAN 5 runtime writes a formatted record as NEW LINE, the
 * text, CARRIAGE RETURN.  On a terminal the RETURN parks the cursor in
 * column 0 until the next record's NEW LINE moves it down.  Keep that, but
 * hold a RETURN back until the next character shows whether it was only
 * the end of a line: before a NEW LINE it is dropped, so a transcript is
 * plain text and not a file full of stray CRs. */
static int con_upper = 1;
static int con_cr_pending;

/* -Z: ?GTOD and ?GDAY always answer 1981-06-15 12:00:00, so the game seeds
 * its random numbers the same way every run and a scripted session can be
 * compared with a recorded one. */
static int frozen_clock;
#define FROZEN_TIME ((time_t)361454400L)

/* FORM FEED clears a DG display terminal's screen -- Dungeons sends one
 * before the Dungeon Keeper's welcome.  At a console that becomes the VT
 * clear-screen sequence; in a transcript it becomes nothing. */
__declspec(dllimport) void *__stdcall GetStdHandle(unsigned long);
__declspec(dllimport) int   __stdcall GetConsoleMode(void *, unsigned long *);
__declspec(dllimport) int   __stdcall SetConsoleMode(void *, unsigned long);

static void con_formfeed(void)
{
    static int vt = -1;
    if (vt < 0) {
        void *h = GetStdHandle((unsigned long)-11);     /* STD_OUTPUT_HANDLE */
        unsigned long mode;
        vt = GetConsoleMode(h, &mode) && SetConsoleMode(h, mode | 0x0004);
    }
    if (vt) fputs("\033[H\033[2J", stdout);
}

static void con_write(const char *buf, int n)
{
    int k;
    for (k = 0; k < n; k++) {
        char c = buf[k];
        if (c == 0) continue;           /* a terminal shows nothing for NUL */
        if (c == '\r') { con_cr_pending = 1; continue; }
        if (con_cr_pending) {
            con_cr_pending = 0;
            if (c != '\n') fputc('\r', stdout);
        }
        if (c == '\f') { con_formfeed(); continue; }
        fputc(c, stdout);
    }
}

/* One keystroke, for a binary (?IBIN) console read.  Dungeons reads its
 * commands a key at a time and types the rest of the word itself ("A" ->
 * "Attack"), so at a real console the key must arrive without Enter and
 * without an echo -- _getch.  Piped input is read a byte at a time, carriage
 * returns dropped.  Enter is NEW LINE (012); Ctrl-C ends the session. */

static int con_getkey(void)
{
    static int tty = -1;
    int c;
    if (tty < 0) tty = _isatty(_fileno(stdin));
    fflush(stdout);
    if (tty) {
        for (;;) {
            c = _getch();
            if (c == 0 || c == 0xE0) { _getch(); continue; }   /* arrows, F-keys */
            if (c == 3) return EOF;
            if (c == '\r') c = '\n';
            break;
        }
    } else {
        do c = fgetc(stdin); while (c == '\r');
        if (c == EOF) return EOF;
    }
    c &= 0x7F;
    if (con_upper && c >= 'a' && c <= 'z') c -= 32;
    return c;
}

/* Put a fixed or dynamic record transfer where it belongs.  PARU: ?IPST in
 * ?ISTI (1B2) says ?IRNH/?IRNL is an absolute record number; clear, it
 * counts records on from where the channel stands, so 0 is simply the next
 * record -- which is how the FORTRAN 5 runtime's RDSEQ reads a file's
 * records one after another.  A ?SPOS since the last transfer has already
 * put the channel where it should be.  (The copy of this emulator in the
 * Thissala port read the flag from ?IRES, found it clear, and treated
 * every record number as absolute; with ?ISTI there is no conflict.) */
static int rec_seek(word pkt, int ch, long recsize)
{
    long rn = ((long)M[(pkt + P_IRNH) & AMASK] << 16) | M[(pkt + P_IRNL) & AMASK];
    int  r;
    if (chan_spos[ch])
        r = fseek(chan[ch], 0L, SEEK_CUR);
    else if (M[(pkt + P_ISTI) & AMASK] & IFLG_IPST)
        r = fseek(chan[ch], rn * recsize, SEEK_SET);
    else
        r = fseek(chan[ch], rn * recsize, SEEK_CUR);
    chan_spos[ch] = 0;
    return r == 0;
}

#define SC_RDB       7          /* 0007 read physical blocks               */
#define SC_SPAGE16  48          /* 0060 include a shared page              */
#define SC_SOPEN16  51          /* 0063 open a shared file                 */
#define SC_FLUSH16  80          /* 0120 flush shared pages                 */
#define SC_SSHPT16  36          /* 0044 set the shared partition           */
#define SC_GSHPT16  59          /* 0073 get the shared partition           */
#define SC_SCLOSE16 113         /* 0161 close a shared file                */

static int copy_file(const char *from, const char *to)
{
    FILE *a = fopen(from, "rb"), *b = a ? fopen(to, "wb") : NULL;
    int c;
    if (a && b) while ((c = fgetc(a)) != EOF) fputc(c, b);
    if (a) fclose(a);
    if (b) fclose(b);
    return a && b;
}

/* Write shared mapping m back to its file if the program changed it, and
 * release the mapping if asked.  A shared file still open on the original
 * in the data directory (chan_shared 2) moves to a copy in the save
 * directory the first time that happens. */
static void smap_write(int m, int release)
{
    int ch = smap[m].ch, differs = 0;
    FILE *f = chan[ch];
    unsigned k, nw = smap[m].cnt * 256u;
    if (f && !fseek(f, smap[m].blk * 512L, SEEK_SET))
        for (k = 0; k < nw && !differs; k++) {
            int hi = fgetc(f), lo = fgetc(f);
            differs = hi == EOF || lo == EOF ||
                      M[(smap[m].addr + k) & AMASK] != (word)((hi << 8) | lo);
        }
    if (f && differs && chan_shared[ch] == 2) {
        char from[512], to[512];
        snprintf(from, sizeof from, "%s/%s", datadir, shared_name[ch]);
        save_path(to, sizeof to, shared_name[ch]);
        fclose(f);
        f = chan[ch] = copy_file(from, to) ? fopen(to, "r+b") : NULL;
        chan_shared[ch] = f ? 1 : 0;
    }
    if (f && differs && !fseek(f, smap[m].blk * 512L, SEEK_SET)) {
        for (k = 0; k < nw; k++) {
            word w = M[(smap[m].addr + k) & AMASK];
            fputc(w >> 8, f);
            fputc(w & 0xFF, f);
        }
        fflush(f);
    }
    if (release) smap[m].used = 0;
}

/* The same for every page mapped from channel ch (every channel if ch is
 * negative). */
static void smap_flush(int ch, int release)
{
    int m;
    for (m = 0; m < 16; m++)
        if (smap[m].used && (ch < 0 || smap[m].ch == ch))
            smap_write(m, release);
}

/* The AOS agent services an original AOS program reaches through call 50
 * with a top-bit call word at block+19 (see the SVC path).  Only what the
 * programs are seen to use is here; anything else stops the machine so it
 * says what it wanted. */
static int do_syscall(word code);

static int do_agent(word agent)
{
    char path[512];
    int ch;
    switch (agent) {
    /* The top-bit numbers are the AOS agent's own.  AOS/VS kept the agent
     * calls (SYSID.16.SR lists 0300 "AGENT OPEN", 0301 "AGENT CLOSE", ...)
     * but not their AOS order, so each number here is pinned by evidence:
     *
     *   0-3  ?OPEN ?CLOSE ?READ ?WRITE -- the packets (.F5INIT opens the
     *        console with 0x8000, RDSEQ reads with 0x8002, WRSEQ writes
     *        with 0x8003).
     *   9    ?GTMES, 10 ?RETURN -- RUNAWAY, an AOS assembly utility that
     *        survives as both source and program (BJ_BB/AOS/PERFORM),
     *        issues ?GTMES ?OPEN ?WRITE ... ?RETURN in that order, and its
     *        .PR has 8009 8000 8003 ... 800A.  AOS/VS has them at 0307 and
     *        0310: two calls AOS/VS dropped came before them.
     *   11   ?ERMSG, after ?RETURN as in AOS/VS; F.CLOSE's error path.
     *   22   ?SPOS -- the FORTRAN direct-access positioning routine at
     *        752C files record number - 1 in ?IRNH/?IRNL and issues 8016
     *        with the packet in AC2.  AOS/VS 0322.
     *   23   the overlay open, AOS/VS ?OVOPN 0323 -- see below.
     *
     * Anything else stops the machine and says what it was. */
    case 0: case 1: case 2: case 3:
        return do_syscall((word)(0300 + agent));
    case 9:  return do_syscall(0307);           /* ?GTMES  */
    case 10: return do_syscall(0310);           /* ?RETURN */
    case 11: return do_syscall(0311);           /* ?ERMSG  */
    case 22: return do_syscall(0322);           /* ?SPOS   */
    case 0x144: {
        /* Error message text.  DG.PR's runtime issues it (76E0) with the
         * error code in AC0, the buffer length in AC1's left byte and the
         * message file's channel in its right byte (377, the system's), and
         * a byte pointer to the buffer in AC2 -- AOS/VS ?ERMSG's registers
         * -- and keeps the length that comes back in AC0.  The system's
         * message file is not here, so the text is only the code, in octal
         * as the CLI would show it. */
        char msg[32];
        int len = snprintf(msg, sizeof msg, "ERROR %o", AC[0]);
        int room = AC[1] >> 8;
        if (len > room) len = room;
        putbstr(AC[2], msg, len);
        AC[0] = (word)len;
        return 1;
    }
    default:
        return -1;
    case 0x17:
        /* ?LODO, the overlay loader, asks for the channel its overlays are
         * read from and keeps it (+1) at the top of its stack; the ?RDB or
         * ?SPAGE that follows reads from it.  On AOS the system opened
         * <program>.OL when it loaded the program. */
        for (ch = 1; ch < 64; ch++)
            if (chan[ch] && !chan_console[ch] && chan_is_ovl[ch]) { AC[1] = (word)ch; return 1; }
        for (ch = 1; ch < 64 && chan[ch]; ch++) ;
        if (ch >= 64) return 0;
        snprintf(path, sizeof path, "%s/%s.OL", datadir, progbase);
        chan[ch] = fopen(path, "rb");
        if (!chan[ch]) { AC[0] = 025; return 0; }         /* ERFDE */
        chan_console[ch] = 0; chan_is_ovl[ch] = 1;
        AC[1] = (word)ch;
        return 1;
    }
}

/* Returns 1 for the normal (success) return, 0 for the error return. */
static int do_syscall(word code)
{
    word pkt = AC[2];
    char name[288], path[512];
    int ch, i;

    switch (code) {
    case SC_AGENT2:
        /* Agent resource-call resolution (?RCALL).  AC2 carries the resource
         * id on entry and comes back as ?DESC; the trampoline at 7D15 takes
         * the entry address to jump to from AC1 (STA 1,-5,3 at 7D33).  A
         * target in the root resolves to itself. */
        {   /* The caller's return address says which overlay it is running
             * in.  Loading the callee can evict that overlay, so hand the
             * matching ?RCALL return (0273) a token for it: the trampoline
             * files whatever comes back in AC2 at callerFP+1 and gives it
             * straight back on the way out.  Without this the caller resumes
             * on top of whatever overlay happens to be resident, reading its
             * own locals out of another routine's code. */
            word ra  = (word)(M[FP & AMASK] & AMASK);
            int  a   = area_of(ra);
            word ctx = (a >= 0 && resident[a] >= 0)
                     ? (word)(((unsigned)a << 9) | ((unsigned)resident[a] & 0x1FF))
                     : 0;
            if (AC[2] < 0x400) {        /* an overlay resource descriptor */
                word ds = M[AC[2] & AMASK], of = M[(AC[2] + 1) & AMASK];
                unsigned node = (ds >> 9) & 0x7F, ovl = ds & 0x1FF;
                word base;
                if (ovl_force >= 0) { ovl = (unsigned)ovl_force; node = 0; }
                base = load_overlay_res(node, ovl);
                if (!base) return 0;
                AC[1] = (word)((base + of) & AMASK);
            } else {
                AC[1] = AC[2];          /* already in the root */
            }
            AC[2] = ctx;
            /* AC0 comes back as the caller's skip count, and it must be zero.
             * The trampoline puts 0x8000 in AC0 before the call to mark a
             * procedure-variable call (71D8 MOVR 0,0 after 71D1 SUBZ 0,0),
             * files it at FP-5, and then at 71E3 does SUB 0,2 on the saved
             * return address.  Leave 0x8000 there and the stored resume
             * address comes back with bit 15 set -- which is the indirect
             * bit, so the JMP @2,3 that returns to the caller chases it one
             * level too far and lands on the contents of the resume address
             * instead of on the resume address.  In ADVENTURE.PR that reads
             * the word at 668B, STA 2,32 = 0x5020, and 5020 is itself live
             * code, so the game runs on in the wrong routine with a stale
             * frame rather than failing outright. */
            AC[0] = 0;
        }
        return 1;

    case SC_AGENT3:
        /* Resource return: bring the caller's overlay back if the call
         * displaced it.  AC0 must come back zero -- 7D4D adds it to the
         * saved return address. */
        if (AC[2]) {
            unsigned a = (AC[2] >> 9) & 0x7F, o = AC[2] & 0x1FF;
            if (a < MAXNODE && area_words[a] && resident[a] != (int)o)
                load_overlay_res(a, o);
        }
        AC[0] = 0;
        return 1;

    case SC_AGENT0: case SC_AGENT1: case SC_AGENT4:
        AC[0] = 0;
        return 1;

    case SC_RETURN:
    case SC_KILL:
        if (verbose) fprintf(stderr, "\n[?%s, AC0=%04X AC1=%04X AC2=%04X]\n",
                             code == SC_KILL ? "KILL" : "RETURN", AC[0], AC[1], AC[2]);
        /* AC1 is a byte pointer to a message and AC2 the PARU flags --
         * ?RFCF, the severity ?RFWA/?RFER/?RFAB in bits 1-2, ?RFEC -- with
         * the message length in its right byte.  (DG.PR, started without a
         * level, returns AC1=5036 -> " (From ERROR 1712+12)", AC2=E015:
         * CLI format, abort, 21 bytes.)  The CLI shows the message; so does
         * this, on stderr. */
        if (code == SC_RETURN && (AC[2] & 0xFF)) {
            char msg[256];
            int len = AC[2] & 0xFF, sev = (AC[2] >> 13) & 3;
            getbytes(AC[1], msg, len);
            fflush(stdout);
            fprintf(stderr, "%s%.*s\n", sev == 1 ? "WARNING: " : sev >= 2 ? "ERROR: " : "",
                    len, msg);
        }
        smap_flush(-1, 1);
        halted = 1;
        return 1;

    case SC_OPEN:
        getbstr(M[(pkt + P_IFNP) & AMASK], name, sizeof name);
        if (verbose)
            fprintf(stderr, "   [?OPEN pkt %04X: isti=%04X ibad=%04X ifnp=%04X -> %c%s%c]\n",
                    pkt, M[(pkt+P_ISTI)&AMASK], M[(pkt+P_IBAD)&AMASK],
                    M[(pkt+P_IFNP)&AMASK], 34, name, 34);
        for (ch = 1; ch < 64 && chan[ch]; ch++) ;
        if (ch >= 64) return 0;
        if (name[0] == '@') {                       /* @CONSOLE, @OUTPUT   */
            chan[ch] = stdout; chan_console[ch] = 1;
        } else {
            /* PARU ?ISTI: ?OFOT (or ?APND) asks for output, ?OFCR to create
             * the file first, ?OFCE to put right the error that gets in the
             * way -- with ?OFCR, delete a file that is already there; alone,
             * create one that is not.  The 500-point Adventure's SAVE opens
             * its file for input and output, and on "does not exist" opens
             * it again with ?OFCE added.  Output happens only in the save
             * directory: a data file the program wants to write is copied
             * there first, so the originals are never changed. */
            word sti = M[(pkt + P_ISTI) & AMASK];
            int wants_out = (sti & (OFOT | 0x0080u)) != 0;
            int ofcr = (sti & 0x0040u) != 0, ofce = (sti & 0x0020u) != 0;
            save_path(path, sizeof path, name);
            if (wants_out || ofcr) {
                char src[512];
                if (ofcr && ofce) remove(path);
                if (!file_exists(path)) {
                    snprintf(src, sizeof src, "%s/%s", datadir, name);
                    if (!ofcr && file_exists(src)) {
                        copy_file(src, path);
                    } else if (ofcr || ofce) {
                        FILE *b = fopen(path, "wb");
                        if (b) fclose(b);
                    }
                }
                chan[ch] = file_exists(path) ? fopen(path, "r+b") : NULL;
                if (chan[ch] && (sti & 0x0080u)) fseek(chan[ch], 0L, SEEK_END);
            } else {
                read_path(path, sizeof path, name);
                chan[ch] = fopen(path, "rb");
            }
            if (!chan[ch]) {
                if (verbose) fprintf(stderr, "[?OPEN failed: %s]\n", name);
                AC[0] = 025;                                    /* ERFDE */
                return 0;
            }
            chan_console[ch] = 0;
        }
        M[(pkt + P_ICH) & AMASK] = (word)ch;
        if (verbose) fprintf(stderr, "[?OPEN %-14s -> channel %d]\n", name, ch);
        return 1;

    case SC_CLOSE:
        ch = M[(pkt + P_ICH) & AMASK] & 63;
        if (chan[ch] && !chan_console[ch]) fclose(chan[ch]);
        chan[ch] = NULL;
        return 1;

    case SC_READ: {
        char buf[8192];
        int n = 0, fmt, rcl;
        long recsize;
        ch  = M[(pkt + P_ICH) & AMASK] & 63;
        fmt = rec_format(pkt, ch);
        rcl = M[(pkt + P_IRCL) & AMASK];
        if (verbose)
            fprintf(stderr, "   [?READ ch=%d fmt=%d rcl=%d sti=%04X mrs=%04X rec=%u]\n",
                    ch, fmt, rcl, M[(pkt + P_ISTI) & AMASK], M[(pkt + P_IMRS) & AMASK],
                    (unsigned)((M[(pkt+P_IRNH)&AMASK] << 16) | M[(pkt+P_IRNL)&AMASK]));
        if (!chan[ch]) return 0;
        if (rcl <= 0 || rcl > (int)sizeof buf) rcl = (int)sizeof buf;

        if (chan_console[ch] && (M[(pkt + P_ISTI) & AMASK] & 0x1000u)) {
            /* ?IBIN: raw bytes, as many as ?IRCL asks for, no line editing
             * and no echo. */
            for (n = 0; n < rcl; n++) {
                int c = con_getkey();
                if (c == EOF) { if (!n) { halted = 1; return 0; } break; }
                buf[n] = (char)c;
            }
        } else if (fmt == RF_DS) {
            /* Data sensitive: one record, ending at NEW LINE, FORM FEED,
             * CARRIAGE RETURN or NULL.  The delimiter is consumed but not
             * handed back, and ?IRLR reports what was. */
            /* The delimiter is part of the record: it goes into the
             * buffer and it counts towards ?IRLR.  Leave it out and the
             * game trims a character of its own, so "look" arrives as
             * "loo". */
            int c = 0;
            if (chan_console[ch]) {
                /* The prompt the game just wrote is still sitting in stdio's
                 * buffer.  Flush before blocking on the keyboard, or it does
                 * not appear until something else forces the buffer out --
                 * which interactively puts the prompt *after* the line you
                 * typed.  Piped runs never show it, because there is no echo
                 * to interleave with. */
                fflush(stdout);
                debug_arm();
                for (;;) {
                    n = 0;
                    while (n < rcl - 1) {
                        c = fgetc(stdin);
                        /* End of input is the end of the session.  Handing
                         * the game an I/O error instead just makes it ask
                         * again, for ever. */
                        if (c == EOF) { if (!n) { halted = 1; return 0; } break; }
                        if (c == '\r') continue;
                        if (c == '\n') break;
                        if (con_upper && c >= 'a' && c <= 'z') c -= 32;
                        buf[n++] = (char)c;
                    }
                    /* -g: a '#' line is answered by the shim and never
                     * reaches the game's parser.  Keep reading until a real
                     * command turns up. */
                    if (debug && n && n < rcl - 1) {
                        buf[n] = 0;
                        if (buf[0] == '#')      debug_command(buf);
                        else if (!debug_verb(buf)) goto have_line;
                        if (c == EOF) return 0;
                        continue;
                    }
                    have_line: ;
                    break;
                }
                buf[n++] = '\n';
            } else {
                while ((c = fgetc(chan[ch])) == 0) ;      /* word padding */
                if (c == EOF) return 0;
                while (n < rcl - 1 && c != EOF && !is_delim(c)) {
                    buf[n++] = (char)c;
                    c = fgetc(chan[ch]);
                }
                buf[n++] = (c == EOF) ? '\n' : (char)c;
            }
        } else {
            /* Fixed or dynamic.  ?IMRS is only set up by ?OPEN and holds
             * junk in these packets, so the record size is ?IRCL itself. */
            size_t got;
            if (chan_console[ch]) return 0;
            recsize = rcl;
            if (!rec_seek(pkt, ch, recsize)) return 0;
            got = fread(buf, 1, (size_t)rcl, chan[ch]);
            chan_pos[ch] = ftell(chan[ch]) / (recsize ? recsize : 1);
            if (!got) return 0;
            n = (int)got;
        }
        putbstr(M[(pkt + P_IBAD) & AMASK], buf, n);
        M[(pkt + P_IRLR) & AMASK] = (word)n;
        if (verbose) {
            int q; fprintf(stderr, "   [?READ -> n=%d <", n);
            for (q = 0; q < n && q < 60; q++)
                fprintf(stderr, (buf[q] >= 32 && buf[q] < 127) ? "%c" : "<%02X>",
                        (unsigned char)buf[q]);
            fprintf(stderr, ">]\n");
        }
        return 1;
    }

    case SC_WRITE: {
        char buf[8192];
        int n, fmt, rcl, k, delim = -1;
        ch  = M[(pkt + P_ICH) & AMASK] & 63;
        fmt = rec_format(pkt, ch);
        rcl = M[(pkt + P_IRCL) & AMASK];
        if (rcl <= 0 || rcl > (int)sizeof buf) rcl = (int)sizeof buf;
        getbytes(M[(pkt + P_IBAD) & AMASK], buf, rcl);
        n = rcl;
        if (fmt != RF_DS && !chan_console[ch] && chan[ch]) {
            /* Fixed and dynamic records are positioned the same way on the
             * way out as on the way in. */
            if (!rec_seek(pkt, ch, rcl)) return 0;
        }
        chan_spos[ch] = 0;
        if (fmt == RF_DS) {
            for (k = 0; k < rcl; k++)
                if (is_delim((unsigned char)buf[k]))
                    { delim = (unsigned char)buf[k]; n = k; break; }
            if (delim > 0) n++;    /* NEW LINE / FORM FEED goes out too */
        }
        if (verbose) {
            int q; fprintf(stderr, "   [?WRITE ch=%d fmt=%d rcl=%d delim=%d n=%d <", ch, fmt, rcl, delim, n);
            for (q = 0; q < n && q < 60; q++)
                fprintf(stderr, (buf[q] >= 32 && buf[q] < 127) ? "%c" : "<%02X>",
                        (unsigned char)buf[q]);
            fprintf(stderr, ">]\n");
        }
        if (chan[ch] && chan_console[ch]) con_write(buf, n);
        else if (chan[ch]) fwrite(buf, 1, (size_t)n, chan[ch]);
        /* A NUL just ends the transfer -- "Initializing ", ".", ".", "."
         * and " Initialized\n" are five records that make one line.  Only a
         * record that runs to ?IRCL without any delimiter at all gets a
         * NEW LINE supplied for it. */
        if (fmt == RF_DS && delim < 0 && chan[ch]) {
            if (chan_console[ch]) con_write("\n", 1);
            else fputc('\n', chan[ch]);
        }
        M[(pkt + P_IRLR) & AMASK] = (word)n;
        return 1;
    }

    /* ?SPOS / ?GPOS.  ADVENTURE.PR reads ADVENTURE.TXT as fixed 72-byte
     * records (?ISTI=6003: ?ICRF plus format 3) and seeks with these before
     * each ?READ; the position is a record number, and it arrives as the
     * doubleword AC0:AC1 with the channel in the packet. */
    case SC_SPOS: {
        long pos = (((long)AC[0] << 16) | AC[1]);
        int rc;
        ch = M[(pkt + P_ICH) & AMASK] & 63;
        rc = M[(pkt + P_IRCL) & AMASK];
        if (!chan[ch] || chan_console[ch]) return 0;
        if (rc <= 0) rc = 1;
        if (fseek(chan[ch], pos * rc, SEEK_SET)) return 0;
        chan_pos[ch] = pos;
        chan_spos[ch] = 1;
        if (verbose) fprintf(stderr, "   [?SPOS ch=%d rec %ld (rcl %d)]\n", ch, pos, rc);
        return 1;
    }

    case SC_GPOS:
        ch = M[(pkt + P_ICH) & AMASK] & 63;
        if (!chan[ch] || chan_console[ch]) return 0;
        AC[0] = (word)(chan_pos[ch] >> 16);
        AC[1] = (word)chan_pos[ch];
        return 1;

    case SC_MEM:
        /* AC0 unshared pages still available, AC1 pages allocated, AC2 the
         * highest allocated address.  The runtime takes min(AC0+AC1, M[0400])
         * and asks ?MEMI for the difference; with 2 of 12 pages in hand it
         * asks for 10 and ends up owning 0000..2FFF, which is what the live
         * machine shows (its UST reads USTBL=12 and the runtime's own top of
         * memory word at 0337 reads 2FA7). */
        AC[0] = (word)(max_pages - cur_pages);
        AC[1] = (word)cur_pages;
        AC[2] = (word)(cur_pages * PAGEW - 1);
        return 1;

    case SC_MEMI: {                 /* ?MEMI - change memory allocation    */
        int want = (int16_t)AC[0];
        unsigned np = cur_pages + (unsigned)want;
        if ((int)np < 1 || np > max_pages) return 0;
        cur_pages = np;
        /* The impure block count lives at UST+0C in an AOS/VS program but at
         * UST+0F in an original AOS one -- where UST+0C is the address of
         * the .SYSTM context block.  Writing the AOS/VS slot there pointed
         * every later system call's saved registers at word 4. */
        if (ustod_off == 0x10u) M[UST + 0x0Fu] = (word)cur_pages;
        else                    M[USTBL] = (word)cur_pages;
        AC[1] = (word)(cur_pages * PAGEW - 1);
        return 1;
    }

    /* ?GSHPT (59) / ?SSHPT (36): the shared partition, as first page and
     * page count.  DG.PR's runtime initialiser asks, moves the partition down
     * by the number of pages at 01C3 (none), then takes every unshared page
     * that is left with ?MEMI and builds its stack in them. */
    case SC_GSHPT16:
        AC[0] = (word)max_pages;
        AC[1] = (word)(32u - max_pages);
        return 1;
    case SC_SSHPT16:
        if (AC[0] < cur_pages || AC[0] + AC[1] != 32u) { AC[0] = 053; return 0; }
        max_pages = AC[0];
        if (ustod_off == 0x10u) M[UST + 0x11u] = AC[0], M[UST + 0x12u] = AC[1];
        return 1;

    case SC_GHRZ:
        /* The runtime indexes a five-group table at 0362 with AC0 to get its
         * ticks-per-second and ticks-per-minute constants.  The live machine
         * ends up with (100, 10, 600) at 2FAC..2FAE, which is group 1. */
        AC[0] = 1;
        return 1;

    case SC_LEFE: lef_mode = 1; return 1;
    case SC_LEFD: lef_mode = 0; return 1;
    case SC_LEFS: AC[0] = (word)lef_mode; return 1;

    /* ?CISND, when it arrives as itself: nothing to send.  (Call 50 is also
     * the carrier for the agent calls -- see the SVC path.) */
    case SC_CISND:
        return 1;

    /* ?RDB (7) and ?SPAGE (48): read physical blocks.  AC1 is the channel
     * and AC2 the 16-bit block I/O packet of PARU.16.SR: ?PSTI with the
     * block count in its right byte, ?PCAD the word address, ?PRNH/?PRNL
     * the block number.  Blocks are 512 bytes.  The AOS overlay loader
     * ?LODO reads an overlay into its area with one of these -- ?SPAGE for
     * a shared area, ?RDB otherwise -- and for this emulator the two are
     * the same thing: the words go into memory at ?PCAD. */
    case SC_RDB:
    case SC_SPAGE16: {
        unsigned cnt = M[(pkt + 0) & AMASK] & 0xFFu;
        word addr = M[(pkt + 2) & AMASK];
        long blk = ((long)M[(pkt + 4) & AMASK] << 16) | M[(pkt + 5) & AMASK];
        unsigned k;
        ch = AC[1] & 63;
        if (!chan[ch] || chan_console[ch] || !cnt) return 0;
        if (code == SC_SPAGE16) {           /* a page already there goes home first */
            int m;
            for (m = 0; m < 16; m++)
                if (smap[m].used && smap[m].addr < addr + cnt * 256u
                                 && addr < smap[m].addr + smap[m].cnt * 256u)
                    smap_write(m, 1);
        }
        if (fseek(chan[ch], blk * 512L, SEEK_SET)) return 0;
        for (k = 0; k < cnt * 256u; k++) {
            int hi = fgetc(chan[ch]), lo = fgetc(chan[ch]);
            if (hi == EOF || lo == EOF) break;
            M[(addr + k) & AMASK] = (word)((hi << 8) | lo);
        }
        if (verbose)
            fprintf(stderr, "   [%s ch=%d blk=%ld cnt=%u -> %04X]\n",
                    code == SC_RDB ? "rdb" : "spage", ch, blk, cnt, addr);
        /* A page of a file opened with ?SOPEN is the file: remember where it
         * went, so what the program writes there goes back.  The overlay
         * channel is never one of these. */
        if (code == SC_SPAGE16 && chan_shared[ch]) {
            int m;
            for (m = 0; m < 16 && smap[m].used; m++) ;
            if (m < 16) {
                smap[m].used = 1; smap[m].ch = ch; smap[m].addr = addr;
                smap[m].blk = blk; smap[m].cnt = cnt;
            }
        }
        return 1;
    }

    /* ?SOPEN: AC0 a byte pointer to the pathname, AC1 the channel the caller
     * would like (the FORTRAN 5 wrapper at 6C1F passes its unit number, 33),
     * AC2 the access; the channel comes back in AC1.  A copy in the save
     * directory is used if there is one; otherwise the original is opened
     * read-only and smap_write copies it out only if the program ever
     * changes a page of it (a normal game never does). */
    case SC_SOPEN16: {
        int own;
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        own = file_exists(path);
        if (!own) snprintf(path, sizeof path, "%s/%s", datadir, name);
        ch = (int)(AC[1] & 0xFFFF);
        if (ch <= 0 || ch >= 64 || chan[ch])
            for (ch = 1; ch < 64 && chan[ch]; ch++) ;
        if (ch >= 64) return 0;
        chan[ch] = fopen(path, own ? "r+b" : "rb");
        if (!chan[ch]) { AC[0] = 025; return 0; }            /* ERFDE */
        snprintf(shared_name[ch], sizeof shared_name[ch], "%s", name);
        chan_console[ch] = 0; chan_shared[ch] = own ? 1 : 2; chan_is_ovl[ch] = 0;
        AC[1] = (word)ch;
        if (verbose) fprintf(stderr, "   [sopen %s -> channel %d]\n", name, ch);
        return 1;
    }

    /* ?FLUSH: write shared pages back now. */
    case SC_FLUSH16:
        smap_flush(-1, 0);
        return 1;

    /* ?SCLOSE: write the channel's shared pages back, then close it. */
    case SC_SCLOSE16:
        ch = (int)(AC[0] & 63);
        if (!chan[ch] || !chan_shared[ch]) ch = (int)(AC[1] & 63);
        if (!chan[ch] || !chan_shared[ch]) return 0;
        smap_flush(ch, 1);
        fclose(chan[ch]); chan[ch] = NULL; chan_shared[ch] = 0;
        return 1;

    case SC_IFPU:                   /* no FPU is needed by the game        */
        return 1;

    /* ?GTMES: the CLI message.  The arguments are whatever followed the .PR
     * on the emulator's command line, in capitals as the CLI passed them.
     * PARU.16: packet ?GREQ ?GNUM ?GSW ?GRES; ?GCNT answers the count in
     * AC0, ?GARG copies argument ?GNUM (0 is the program) to the byte
     * pointer ?GRES and answers its LENGTH IN AC0 and its value in AC1
     * (-1 when it is not a number).  That way round is DG.PR's evidence:
     * its argument routine (overlay 1, 3413..343C) hands AC0 to the string
     * copy as the length, so with the registers swapped "DG 3" copied three
     * bytes -- "3", the NUL and junk -- failed to parse and always built a
     * level 1 dungeon.  (The 500-point Adventure only uses the NUL-ended
     * string and works either way.) */
    case SC_GTMES: {
        word req = M[pkt & AMASK], num = M[(pkt + 1) & AMASK];
        word res = M[(pkt + 3) & AMASK];
        char msg[512];
        int a, len;
        if (verbose)
            fprintf(stderr, "   [?GTMES pkt %04X: req=%04X num=%04X sw=%04X res=%04X]\n",
                    pkt, req, num, M[(pkt+2) & AMASK], res);
        prog_args[0] = progbase;
        switch (req & 0x7FFF) {
        case 0: case 1:                             /* ?GMES, ?GCMD      */
            msg[0] = 0;
            for (a = 0; a <= prog_nargs; a++) {
                if (a) strncat(msg, " ", sizeof msg - strlen(msg) - 1);
                strncat(msg, prog_args[a], sizeof msg - strlen(msg) - 1);
            }
            break;
        case 2:                                     /* ?GCNT             */
            AC[0] = (word)prog_nargs;
            return 1;
        case 3:                                     /* ?GARG             */
            if (num > (word)prog_nargs) { AC[0] = 0; return 0; }
            snprintf(msg, sizeof msg, "%s", prog_args[num]);
            break;
        default:                                    /* no switches given */
            AC[0] = 0; AC[1] = 0;
            return req == 4 ? 0 : 1;
        }
        for (len = 0; msg[len]; len++)
            if (msg[len] >= 'a' && msg[len] <= 'z') msg[len] -= 32;
        putbstr(res, msg, len + 1);                 /* with its NUL      */
        AC[0] = (word)len;
        {   char *e;
            long v = strtol(msg, &e, 10);
            AC[1] = (len && !*e) ? (word)v : 0xFFFF;
        }
        return 1;
    }

    case SC_GCHR:
        /* Device characteristics, PARU "PERIPHERAL DEVICE CHARACTERISTICS".
         * The caller keeps ?CH1 & 037 -- upper-case-only, ring monitor,
         * form feed on open, and the two echo-mode bits.  Report a CRT that
         * echoes straight and accepts upper and lower case. */
        M[(pkt + 0) & AMASK] = 0x0001;      /* ?CEOS: straight echo        */
        M[(pkt + 1) & AMASK] = 0x8000;      /* ?CULC, device type ?TTY */
        M[(pkt + 2) & AMASK] = 80;          /* page width                  */
        M[(pkt + 3) & AMASK] = 24;          /* page length                 */
        /* PARU defines ?CH1..?CH15, but only these four are ever read back
         * and the callers do not always leave 15 words for them.  PLOT.PR
         * passes a buffer at FP+13 with the stack top eight words later, so
         * filling all fifteen writes over the return address the .SYSTM
         * thunk has just pushed -- and the POPJ at 7CC6 then returns to
         * whatever the packet's tail happened to be.  Write only what is
         * answered and leave the rest of the caller's buffer alone. */
        /* PARU defines ?CH1..?CH15, but PLOT.PR hands this call a buffer at
         * FP+13 with only eight words before the top of its stack, and the
         * .SYSTM thunk has already pushed its return address just past that.
         * Filling all fifteen overwrites the return address, and the POPJ at
         * 7CC6 then jumps into whatever the packet's tail held -- the ninth
         * word is exactly where it starts to break.  Answer the four fields
         * that are ever read, clear through ?CH8, and go no further. */
        for (i = 4; i < 8; i++) M[(pkt + i) & AMASK] = 0;
        return 1;

    case SC_CREATE:
        /* AC0 is a byte pointer to the pathname, AC2 the ?CREATE packet.
         * The game deletes and recreates its save file, so just make an
         * empty one -- in the save directory, never in the data one. */
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        {   FILE *f = fopen(path, "wb");
            if (!f) return 0;
            fclose(f);
        }
        if (verbose) fprintf(stderr, "[?CREATE %s]\n", path);
        return 1;

    case SC_DELETE:
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        if (verbose) fprintf(stderr, "[?DELETE %s]\n", path);
        return remove(path) == 0;

    case SC_STOM:
        /* SYSID calls 0400 ?STOM "SET TIME".  The game issues it once while
         * setting up @CONSOLE, between ?GCHR and ?SCHR, and never looks at
         * what comes back -- the wrapper at 6E8F only tests the carry the
         * return path leaves.  Succeeding is enough. */
        return 1;

    case SC_GTOD: {                 /* seconds, minutes, hours           */
        time_t t = frozen_clock ? FROZEN_TIME : time(NULL);
        struct tm *lt = frozen_clock ? gmtime(&t) : localtime(&t);
        AC[0] = (word)lt->tm_sec; AC[1] = (word)lt->tm_min; AC[2] = (word)lt->tm_hour;
        return 1;
    }

    case SC_GDAY: {                 /* day, month, year - 1900           */
        time_t t = frozen_clock ? FROZEN_TIME : time(NULL);
        struct tm *lt = frozen_clock ? gmtime(&t) : localtime(&t);
        AC[0] = (word)lt->tm_mday; AC[1] = (word)(lt->tm_mon + 1);
        AC[2] = (word)lt->tm_year;
        return 1;
    }

    case SC_SCHR: case SC_DELAY: case SC_CTYPE:
        return 1;                                   /* benign no-ops       */
    }
    return -1;                                      /* unknown             */
}

static long syscount[512];
static long stubcount[8];
static long systotal;
static int  syslog_left = 0;   /* -v raises it */

static void systm(void)
{
    word code = M[(PC + 1) & AMASK];
    if (code < 512) syscount[code]++;
    systotal++;
    if (syslog_left > 0) {
        fprintf(stderr, "  .SYSTM %3u (0%03o) at %04X   AC0=%04X AC1=%04X AC2=%04X\n",
                code, code, PC, AC[0], AC[1], AC[2]);
        syslog_left--;
    }
    /* Report success and take the normal return: JSR@15 / code / err / ok. */
    C = 0;
    PC = (word)((PC + 3) & AMASK);
}

#include "fpuexec.h"
#include "cis.h"

/* ----------------------------------------------------------- execute  */
static void step(void)
{
    word at = PC;
    {   int b; for (b = 0; b < bp_n; b++) if (PC == bp_addr[b]) {
            bp_hits++;
            fprintf(stderr, "[bp %04X hit #%ld  AC=%04X %04X %04X %04X C%d SP=%04X]\n",
                    PC, bp_hits, AC[0],AC[1],AC[2],AC[3],C,SP);
            if (bp_stop) { halted = 1; return; }
        } }
    word ir = M[PC];
    word w2;

    if (trace) {
        fprintf(stderr, "%04X %04X  AC %04X %04X %04X %04X C%d SP%04X\n",
                PC, ir, AC[0], AC[1], AC[2], AC[3], C, SP);
    }
    PC = (word)((PC + 1) & AMASK);

    /* ---- PSHJ, push-jump: EBID.SR 102270 ------------------------------
     * Two words, the index mode in bits 9-8 like the other extended memory
     * references, but its low byte is B8, not 38, so it never reached that
     * decoder -- and neither Thissala nor the AOS/VS Adventure uses it.  The
     * AOS FORTRAN 5 runtime calls its own internal routines with it:
     * R?CAL at 7B07 is 85B8 0079, PSHJ 0079,PC, which is ?RSRE at 7B81, and
     * 7AF5's 85B8 0064 is ?RSLO at 7B5A.  It pushes the address of the next
     * instruction and jumps; POPJ comes back. */
    if ((ir & 0xFCFFu) == 0x84B8u) {
        unsigned ix = (ir >> 8) & 3;
        word a;
        w2 = M[PC];
        PC = (word)((PC + 1) & AMASK);
        switch (ix) {
        case 0: a = w2 & AMASK; break;
        case 1: a = (word)((at + 1 + (int16_t)w2) & AMASK); break;
        case 2: a = (word)((AC[2] + (int16_t)w2) & AMASK); break;
        default:a = (word)((AC[3] + (int16_t)w2) & AMASK); break;
        }
        if (w2 & 0x8000) a = indirect(a);
        push(PC);
        PC = a;
        stack_check(PC);
        return;
    }

    /* ---- ELDB, ESTB, DSPA: EBID.SR 102170, 122170, 142170 --------------
     * Same layout as the 38 family -- operation in bits 1-2, AC in 3-4,
     * index in 6-7 -- with 78 in the low byte.  The FORTRAN 5 runtime in
     * DG.PR is the first program here to use them.
     *
     * ELDB/ESTB (ECLIPSE S/140 Programmer's Reference): the displacement is
     * a byte pointer -- shifted right one bit it is a 15-bit word address,
     * the bit shifted out picks the byte (0 left, 1 right) -- and the index
     * value is ADDED TO THAT WORD ADDRESS: 0, the address of the
     * displacement word itself, AC2 or AC3.  ELDB clears the AC's left byte.
     *
     * DSPA: E, computed as for ELDA, is a table with signed limits L and H
     * in the two words before it.  With L <= AC <= H the word at E-L+AC,
     * unless it is 177777, is the start of an indirection chain whose end
     * goes into the PC; otherwise execution simply continues. */
    if ((ir & 0x84FFu) == 0x8478u) {
        unsigned op = (ir >> 13) & 3, ac = (ir >> 11) & 3, ix = (ir >> 8) & 3;
        word a;
        w2 = M[PC];
        PC = (word)((PC + 1) & AMASK);
        if (op == 0 || op == 1) {
            word off = ix == 0 ? 0 : ix == 1 ? (word)(at + 1) : ix == 2 ? AC[2] : AC[3];
            a = (word)(((w2 >> 1) + off) & AMASK);
            if (op == 0)
                AC[ac] = (word)((w2 & 1) ? (M[a] & 0xFF) : (M[a] >> 8));
            else if (w2 & 1)
                M[a] = (word)((M[a] & 0xFF00) | (AC[ac] & 0xFF));
            else
                M[a] = (word)((M[a] & 0x00FF) | ((AC[ac] & 0xFF) << 8));
            return;
        }
        if (op == 2) {
            int16_t v = (int16_t)AC[ac], lo, hi;
            switch (ix) {
            case 0: a = w2 & AMASK; break;
            case 1: a = (word)((at + 1 + (int16_t)w2) & AMASK); break;
            case 2: a = (word)((AC[2] + (int16_t)w2) & AMASK); break;
            default:a = (word)((AC[3] + (int16_t)w2) & AMASK); break;
            }
            if (w2 & 0x8000) a = indirect(a);
            lo = (int16_t)M[(a - 2) & AMASK];
            hi = (int16_t)M[(a - 1) & AMASK];
            if (v >= lo && v <= hi) {
                word t = M[(a - lo + v) & AMASK];
                if (t != 0xFFFF)
                    PC = (t & 0x8000) ? indirect((word)(t & AMASK)) : (word)(t & AMASK);
            }
            return;
        }
        die("unimplemented 170-family Eclipse instruction", ir);
        return;
    }

    /* ---- Eclipse two-word memory reference ------------------------- */
    if ((ir & 0x00FF) == 0x38 && (ir & 0x8000)) {
        unsigned ac = (ir >> 11) & 3, ix = (ir >> 8) & 3;
        word a;
        w2 = M[PC];
        PC = (word)((PC + 1) & AMASK);
        switch (ix) {
        case 0: a = w2 & AMASK; break;
        case 1: a = (word)((at + 1 + (int16_t)w2) & AMASK); break;
        case 2: a = (word)((AC[2] + (int16_t)w2) & AMASK); break;
        default:a = (word)((AC[3] + (int16_t)w2) & AMASK); break;
        }
        /* Bit 15 of the displacement word is the indirect bit, and it applies in
         * every index mode, not just absolute.  Restricting it to ix==0 left the
         * indexed forms -- ELDA ac,@disp,2 and ,3 -- loading the pointer itself
         * instead of what it points at.  That is what stopped ADVENTURE.PR ever
         * resolving an object name: at 4005 it does ELDA 1,@0C9,3 to fetch
         * PLACE(obj) through the address it just stored, got the address back,
         * and so compared 104C against the room number.  The one-word ea() has
         * always applied MR_IND in all four modes; this is the two-word path
         * catching up with it.
         *
         * The displacement needs no extra sign care: AMASK is 15 bits, so the
         * indexed arithmetic is modulo 32768 and bit 15 contributes nothing. */
        if (w2 & 0x8000) a = indirect(a);

        switch (ir & E2_MASK_AC) {
        case E2_ELDA: watchrd(a, at, "ELDA"); AC[ac] = M[a];  return;
        case E2_ESTA: watchwr(a, at, AC[ac], "ESTA"); M[a] = dbghold(a, AC[ac]);  return;
        case E2_ELEF: AC[ac] = a;     return;
        }
        switch (ir & E2_MASK_NOAC) {
        case E2_EJMP: PC = a; return;
        case E2_EJSR: AC[3] = PC; PC = a; return;
        case E2_EISZ: M[a] = (word)((M[a] + 1) & 0xFFFF);
                      if (!M[a]) { PC = (word)((PC + 1) & AMASK); }
                      return;
        case E2_EDSZ: M[a] = (word)((M[a] - 1) & 0xFFFF);
                      if (!M[a]) { PC = (word)((PC + 1) & AMASK); }
                      return;
        }
        die("unimplemented two-word Eclipse instruction", ir);
        return;
    }

    /* ---- two-word immediate family (IORI/XORI/ANDI/ADDI) --------------- */
    if ((ir & EI_MASK) == EI_IORI || (ir & EI_MASK) == EI_XORI ||
        (ir & EI_MASK) == EI_ANDI || (ir & EI_MASK) == EI_ADDI) {
        unsigned acd = (ir >> 11) & 3;
        word imm = M[PC];
        PC = (word)((PC + 1) & AMASK);
        switch (ir & EI_MASK) {
        case EI_IORI: AC[acd] |= imm; break;
        case EI_XORI: AC[acd] ^= imm; break;
        case EI_ANDI: AC[acd] &= imm; break;
        default:      AC[acd] = (word)(AC[acd] + imm); break;
        }
        return;
    }

    /* ---- Eclipse one-word extended --------------------------------- */
    if (IS_ECLIPSE_EXT(ir)) {
        unsigned acs = ALC_ACS(ir), acd = ALC_ACD(ir);
        switch (ir) {
        /* The floating point instructions all go to fp_exec below.  PLOT.PR
         * only ever cleared the FPU and disabled its traps, but ADVENTURE.PR
         * does real arithmetic in it, and DG.PR keeps state in it across
         * FPSH/FPOP. */
        case E1_RTN:  do_rtn(); stack_check_under(PC); return;
        case E1_POPJ: PC = (word)(pop() & AMASK); stack_check_under(PC); return;
        case E1_POPB: do_popb(); stack_check_under(PC); return;
        /* BLM: AC1 words from AC2 to AC3; AC1 ends at 0 and AC2/AC3
         * point past their blocks.  AC0 is not involved.  The runtime
         * fills its 88-word control block by storing -1 at 2FA8 and
         * moving 87 words from there to 2FA9, and it relies on AC1
         * coming back as zero for the four stores that follow.
         * BAM is the same move with AC0 added to every word. */
        case E1_BLM:  { while (AC[1]) { M[AC[3] & AMASK] = M[AC[2] & AMASK];
                            AC[2]++; AC[3]++; AC[1]--; } return; }
        case E1_BAM:  { while (AC[1]) { M[AC[3] & AMASK] =
                            (word)(M[AC[2] & AMASK] + AC[0]);
                            AC[2]++; AC[3]++; AC[1]--; } return; }
        case E1_PSHR: push((word)((C ? 0x8000 : 0) | (AC[3] & AMASK)));
                      stack_check(PC); return;
        case E1_CMV: case E1_CMP: case E1_CTR: case E1_CMT:
            char_instruction(ir); return;
        case E1_MUL:  { uint32_t pr = (uint32_t)AC[1] * AC[2] + AC[0];
                        AC[0] = (word)(pr >> 16); AC[1] = (word)pr; return; }
        case E1_MULS: { int32_t pr = (int32_t)(int16_t)AC[1] * (int16_t)AC[2] + (int16_t)AC[0];
                        AC[0] = (word)(pr >> 16); AC[1] = (word)pr; return; }
        case E1_DIV:  { uint32_t nu = ((uint32_t)AC[0] << 16) | AC[1];
                        if (!AC[2] || AC[0] >= AC[2]) { C = 1; return; }
                        AC[1] = (word)(nu / AC[2]); AC[0] = (word)(nu % AC[2]); C = 0; return; }
        case E1_DIVX: /* sign-extend AC1 into AC0, then divide as DIVS */
                      AC[0] = (AC[1] & 0x8000) ? 0xFFFFu : 0;
                      /* fall through */
        case E1_DIVS: { int32_t nu = (int32_t)(((uint32_t)AC[0] << 16) | AC[1]);
                        if (!AC[2]) { C = 1; return; }
                        AC[1] = (word)(nu / (int16_t)AC[2]);
                        AC[0] = (word)(nu % (int16_t)AC[2]); C = 0; return; }
        }
        if (ir == E1_SAVE) {
            w2 = M[PC];
            if ((word)(SP + 5 + w2) > SL) { stack_fault(at); return; }
            PC = (word)((PC + 1) & AMASK);
            do_save(w2); return;
        }
        switch (ir & E1_MASK) {
        case E1_IOR: AC[acd] |= AC[acs]; return;
        case E1_XOR: AC[acd] ^= AC[acs]; return;
        case E1_ANC: AC[acd] &= (word)~AC[acs]; return;
        case E1_XCH: { word t = AC[acs]; AC[acs] = AC[acd]; AC[acd] = t; return; }
        case E1_SGT: if ((int16_t)AC[acs] >  (int16_t)AC[acd])
                         PC = (word)((PC + 1) & AMASK); return;
        case E1_SGE: if ((int16_t)AC[acs] >= (int16_t)AC[acd])
                         PC = (word)((PC + 1) & AMASK); return;
        case E1_PSH: { unsigned a2 = acs;
                       for (;;) { push(AC[a2]); if (a2 == acd) break; a2 = (a2 + 1) & 3; }
                       stack_check(PC); return; }
        case E1_ADI: AC[acd] = (word)(AC[acd] + (acs + 1)); return;
        case E1_SBI: AC[acd] = (word)(AC[acd] - (acs + 1)); return;
        case E1_LSH: { int n = (int8_t)(AC[acs] & 0xFF);
                       if (n > 0)  AC[acd] = (word)(n >= 16 ? 0 : (AC[acd] << n));
                       else if (n) AC[acd] = (word)(-n >= 16 ? 0 : (AC[acd] >> -n));
                       return; }
        case E1_HXL: { unsigned n = 4 * (acs + 1);
                       AC[acd] = (word)(n >= 16 ? 0 : (AC[acd] << n)); return; }
        case E1_HXR: { unsigned n = 4 * (acs + 1);
                       AC[acd] = (word)(n >= 16 ? 0 : (AC[acd] >> n)); return; }
        /* The double shifts work on ACD:ACD+1 as one 32-bit number, AC3+1
         * being AC0; a DLSH count beyond 31 either way clears it. */
        case E1_DLSH: case E1_DHXL: case E1_DHXR: {
                       unsigned lo = (acd + 1) & 3;
                       uint32_t v = ((uint32_t)AC[acd] << 16) | AC[lo];
                       int n = (ir & E1_MASK) == E1_DLSH ? (int8_t)(AC[acs] & 0xFF)
                             : (ir & E1_MASK) == E1_DHXL ? (int)(4 * (acs + 1))
                             : -(int)(4 * (acs + 1));
                       if (n >= 32 || n <= -32) v = 0;
                       else if (n > 0) v <<= n;
                       else if (n < 0) v >>= -n;
                       AC[acd] = (word)(v >> 16); AC[lo] = (word)v;
                       return; }
        case E1_LDB: { word bp = AC[acs]; word w = M[(bp >> 1) & AMASK];
                       AC[acd] = (word)((bp & 1) ? (w & 0xFF) : (w >> 8)); return; }
        case E1_STB: { word bp = AC[acs]; word a = (word)((bp >> 1) & AMASK);
                       if (bp & 1) M[a] = (word)((M[a] & 0xFF00) | (AC[acd] & 0xFF));
                       else        M[a] = (word)((M[a] & 0x00FF) | ((AC[acd] & 0xFF) << 8));
                       return; }
        case E1_BTO: { word a = (word)((AC[acs] + (AC[acd] >> 4)) & AMASK);
                       M[a] |= (word)(0x8000u >> (AC[acd] & 15)); return; }
        case E1_BTZ: { word a = (word)((AC[acs] + (AC[acd] >> 4)) & AMASK);
                       M[a] &= (word)~(0x8000u >> (AC[acd] & 15)); return; }
        case E1_SZB: { word a = (word)((AC[acs] + (AC[acd] >> 4)) & AMASK);
                       if (!(M[a] & (0x8000u >> (AC[acd] & 15))))
                           PC = (word)((PC + 1) & AMASK); return; }
        /* SZBO (0102310): the atomic test-and-set.  Skip if the bit was
         * zero, and set it to one either way.  ADVENTURE.PR takes its
         * console lock with it before the first ?WRITE. */
        case E1_SZBO: { word a = (word)((AC[acs] + (AC[acd] >> 4)) & AMASK);
                       word m = (word)(0x8000u >> (AC[acd] & 15));
                       int was = (M[a] & m) != 0;
                       M[a] |= m;
                       if (!was) PC = (word)((PC + 1) & AMASK);
                       return; }
        case E1_SNB: { word a = (word)((AC[acs] + (AC[acd] >> 4)) & AMASK);
                       if (M[a] & (0x8000u >> (AC[acd] & 15)))
                           PC = (word)((PC + 1) & AMASK); return; }
        case E1_COB: { unsigned k, n = 0; for (k = 0; k < 16; k++)
                           if (AC[acd] & (1u << k)) n++;
                       AC[acs] = (word)(AC[acs] + n); return; }
        /* MSP, XCT and HLV share their low 11 bits (0103370, 0123370,
         * 0143370), so they cannot be told apart under E1_MASK: they are a
         * .DIAC group, discriminated by bits 14-13 with the accumulator in
         * bits 12-11.  Decoding HLV as MSP turns "AC1 = AC1/2" into
         * "SP += AC2", which walks the stack straight out of the unshared
         * area and into read-only shared memory a few thousand instructions
         * later -- far from where it went wrong. */
        case E1_MSP:
            switch (ir & E1_MSP_MASK) {
            case E1_MSP: { word t = (word)(SP + AC[acd]);
                           if (t > SL) { stack_fault(at); return; }
                           SP = (word)(t & AMASK); return; }
            case E1_HLV: AC[acd] = (word)(((int16_t)AC[acd]) >> 1); return;
            case E1_XCT: die("XCT (execute) is not implemented", ir); return;
            }
            die("unimplemented MSP/XCT/HLV group", ir);
            return;
        case E1_CLM: { /* acs==acd: limits are inline after the instruction,
                        * otherwise they sit at AC[acd] and AC[acd]+1.
                        * Skip when the value lies within the pair. */
                       int16_t v = (int16_t)AC[acs], lo, hi;
                       if (acs == acd) {
                           lo = (int16_t)M[PC]; hi = (int16_t)M[(PC + 1) & AMASK];
                           PC = (word)((PC + 2) & AMASK);
                       } else {
                           lo = (int16_t)M[AC[acd] & AMASK];
                           hi = (int16_t)M[(AC[acd] + 1) & AMASK];
                       }
                       if (v >= lo && v <= hi) PC = (word)((PC + 1) & AMASK);
                       return; }
        case E1_POP: { unsigned a2 = acs;
                       for (;;) { AC[a2] = pop(); if (a2 == acd) break; a2 = (a2 - 1) & 3; }
                       stack_check_under(PC); return; }
        }
        if (cis_exec(ir)) return;
        if (fp_exec(ir, at)) return;
        /* SVC -- how an original AOS program enters the kernel.  The .SYSTM
         * thunk (7C81 in ADVENTURE.PR) does not trap with the call number
         * in hand the way the AOS/VS one does.  It files the caller's state
         * in the context block whose address is in UST+0C, a block that
         * lies inside the UST itself at UST+13:
         *
         *   +2 SP  +3 FP  +4 SL  +6 AC0  +7 AC1  +8 AC2  +9 FP
         *   +10 the SUCCESS return -- the thunk adds two to the address of
         *       the call number, stepping over it and over the one-word
         *       error JMP that follows it; the error return is one less
         *   +15 the call number
         *
         * and executes SVC with AC0 pointing at the block.  The kernel runs
         * the call and resumes the program at +10 or +11.  That +10 is also
         * the start address before the program has run at all -- the slot
         * the loader reads as UST+1D -- which is what says it is the saved
         * PC. */
        if (ir == 0x8748 && ustod_off == 0x10u) {
            word blk = M[UST + 0x0C];
            if (AC[0] == blk) {
                word code = M[(blk + 15) & AMASK];
                int r;
                AC[0] = M[(blk + 6) & AMASK];
                AC[1] = M[(blk + 7) & AMASK];
                AC[2] = M[(blk + 8) & AMASK];
                if (code < 512) syscount[code]++;
                systotal++;
                if (syslog_left > 0) {
                    fprintf(stderr, "  .SYSTM %3u (0%03o) from %04X  AC0=%04X AC1=%04X AC2=%04X\n",
                            code, code, M[(blk + 10) & AMASK], AC[0], AC[1], AC[2]);
                    syslog_left--;
                }
                /* A call word with its top bit set does not go to the kernel
                 * as itself.  The thunk (7CA7-7CAA) files it at block+19,
                 * sends call 50 instead, and marks UST+7 bit 1 (0x4000) busy
                 * so a second one cannot start; nothing in the program
                 * clears that bit, so the system does when it is done.
                 * These are the AOS agent's services -- the overlay loader
                 * ?LODO uses 0x8017 to get the channel its overlays are
                 * read from. */
                if (code == 50 && (M[(blk + 19) & AMASK] & 0x8000)) {
                    word agent = (word)(M[(blk + 19) & AMASK] & 0x7FFF);
                    r = do_agent(agent);
                    M[UST + 7] &= (word)~0x4000u;
                    if (syslog_left > 0) {
                        fprintf(stderr, "  agent %u (0%o) -> %d  AC0=%04X AC1=%04X AC2=%04X\n",
                                agent, agent, r, AC[0], AC[1], AC[2]);
                        syslog_left--;
                    }
                } else
                    r = do_syscall(code);
                if (r < 0) {
                    fprintf(stderr, "\n*** unimplemented .SYSTM %u (0%03o) from %04X\n",
                            code, code, M[(blk + 10) & AMASK]);
                    if (!permissive) { halted = 1; return; }
                    r = 1;
                }
                /* The thunk sets bit 15 of block+1 on the way in (7CAB-7CAD,
                 * ADDOR 0,0 on the word) to say the task is inside a system
                 * call.  The system clears it on the way out.  Left set, the
                 * runtime's scheduler SCHED (7C48) takes it for a pending
                 * reschedule, files the whole task in the block and traps
                 * with SVC 3. */
                M[(blk + 1) & AMASK] &= 0x7FFF;
                AC[3] = M[(blk + 9) & AMASK];
                PC = (word)((M[(blk + 10) & AMASK] - (r ? 0 : 1)) & AMASK);
                return;
            }
        }
        die("unimplemented one-word Eclipse instruction", ir);
        return;
    }

    /* ---- AOS entry stub: A6C9 / 3000 / selector / 0000 ------------------
     * Five of these exist, all in PLOT.PR's runtime glue, selectors 0..4.
     * Page-zero 15 reaches selector 0, which is .SYSTM.  The caller did
     * JSR @15, so AC3 holds the address of the call-code word, and the thunk
     * at 7CBC pushed it before trapping.
     *
     * The thunk -- not the system -- does the bookkeeping on the way out:
     *
     *      7CC1  JMP 7CC7        error:  bump the pushed return once
     *      7CC2  LDA 3,32        normal: bump it twice
     *      7CC3  ISZ 0,3
     *      7CC4  ISZ 0,3
     *      7CC5  LDA 3,33        AC3 <- FP
     *      7CC6  POPJ
     *
     * so all this has to do is resume at the word after the four-word block
     * for an error and one further for success, leaving the pushed AC3 on
     * the stack.  Returning straight to AC3+1/AC3+2 instead skips the
     * LDA 3,33 and leaves AC3 pointing into the runtime, which is what made
     * ?OPEN file its channel number through a garbage frame pointer. */
    if (ir == 0xA6C9) {
        word sel = M[(at + 2) & AMASK];
        if (sel == 0) {
            word code = M[AC[3] & AMASK];
            if (code < 512) syscount[code]++;
            systotal++;
            if (syslog_left > 0) {
                fprintf(stderr, "  .SYSTM %3u (0%03o) from %04X  AC0=%04X AC1=%04X AC2=%04X\n",
                        code, code, AC[3], AC[0], AC[1], AC[2]);
                syslog_left--;
            }
            {   int r = do_syscall(code);
                if (r < 0) {
                    fprintf(stderr, "\n*** unimplemented .SYSTM %u (0%03o) from %04X\n",
                            code, code, AC[3]);
                    if (!permissive) { halted = 1; return; }
                    r = 1;
                }
                /* Error is signalled by which return the thunk takes, not by
                 * carry -- carry belongs to the caller. */
                PC = (word)((at + (r ? 5 : 4)) & AMASK);
            }
            return;
        }
        /* selectors 1..4 sit inline and resume at the word after the
         * four-word block. */
        stubcount[sel]++;
        if (syslog_left > 0) {
            fprintf(stderr, "  AOS stub %u at %04X  AC0=%04X AC1=%04X AC2=%04X\n",
                    sel, at, AC[0], AC[1], AC[2]);
            syslog_left--;
        }
        PC = (word)((at + 4) & AMASK);
        return;
    }

    /* ---- plain ALC -------------------------------------------------- */
    if (ir & 0x8000) {
        unsigned acs = ALC_ACS(ir), acd = ALC_ACD(ir), op = ALC_OP(ir);
        unsigned sh = ALC_SH(ir), cy = ALC_CY(ir), skip = ALC_SKIP(ir);
        uint32_t src = AC[acs], dst = AC[acd], r, c;
        switch (cy) {
        case 0: c = C ? 0x10000u : 0; break;
        case 1: c = 0; break;
        case 2: c = 0x10000u; break;
        default:c = C ? 0 : 0x10000u; break;
        }
        switch (op) {
        case 0: r = c | ((~src) & 0xFFFFu); break;                 /* COM */
        case 1: r = (c | ((~src) & 0xFFFFu)) + 1; break;           /* NEG */
        case 2: r = c | src; break;                                /* MOV */
        case 3: r = (c | src) + 1; break;                          /* INC */
        case 4: r = (c | ((~src) & 0xFFFFu)) + dst; break;         /* ADC */
        case 5: r = (c | ((~src) & 0xFFFFu)) + dst + 1; break;     /* SUB */
        case 6: r = (c | src) + dst; break;                        /* ADD */
        default:r = c | (src & dst); break;                        /* AND */
        }
        r &= 0x1FFFFu;
        switch (sh) {
        case 1: r = ((r << 1) | (r >> 16)) & 0x1FFFFu; break;      /* L   */
        case 2: r = ((r >> 1) | (r << 16)) & 0x1FFFFu; break;      /* R   */
        case 3: r = (r & 0x10000u) |
                    (((r << 8) | ((r >> 8) & 0xFF)) & 0xFFFFu); break; /* S */
        }
        {   int cr = (r & 0x10000u) ? 1 : 0, zr = (r & 0xFFFFu) == 0;
            int sk = 0;
            switch (skip) {
            case 0: break;
            case 1: sk = 1; break;
            case 2: sk = !cr; break;
            case 3: sk =  cr; break;
            case 4: sk =  zr; break;
            case 5: sk = !zr; break;
            case 6: sk = !cr || zr; break;
            default:sk =  cr && !zr; break;
            }
            if (sk) PC = (word)((PC + 1) & AMASK);
        }
        if (!ALC_NL(ir)) { AC[acd] = (word)(r & 0xFFFFu); C = (r & 0x10000u) ? 1 : 0; }
        (void)ALU;
        return;
    }

    /* ---- memory reference ------------------------------------------- */
    {
        unsigned op = MR_OP(ir);
        word a;
        /* In LEF mode (?LEFE) the I/O opcode space is Load Effective
         * Address: the memory-reference addressing is evaluated and the
         * address itself is loaded into AC(op-12) instead of the word. */
        if (op >= 12) { AC[op - 12] = ea(ir, at); return; }
        /* .SYSTM is JSR @15; intercept before computing the address. */
        if (catch_systm && op == 1 && MR_IND(ir) && MR_IDX(ir) == 0 && MR_DISP(ir) == 15) {
            PC = at; systm(); return;
        }
        a = ea(ir, at);
        switch (op) {
        case 0: PC = a; return;                                   /* JMP  */
        case 1: AC[3] = PC; PC = a; return;                       /* JSR  */
        case 2: M[a] = (word)((M[a] + 1) & 0xFFFF);           /* ISZ  */
                if (!M[a]) { PC = (word)((PC + 1) & AMASK); }
                return;
        case 3: M[a] = (word)((M[a] - 1) & 0xFFFF);           /* DSZ  */
                if (!M[a]) { PC = (word)((PC + 1) & AMASK); }
                return;
        default:
            if (op < 8) { watchrd(a, at, "LDA "); AC[op - 4] = M[a]; }  /* LDA  */
            else        { watchwr(a, at, AC[op - 8], "STA "); M[a] = dbghold(a, AC[op - 8]); }  /* STA  */
            return;
        }
    }
}

/* Look for the game files beside the executable, then in ./ and ./data,
 * then in the source tree's src_original.  -d overrides all of it. */
static char prpath_buf[600] = "";

/* Is there exactly one .PR in this directory?  A per-game build sits next to
 * its own files, so one is the normal case; a directory holding a whole
 * :UTIL is not something to guess in, and picking wrong there wastes a lot
 * of time looking like a CPU bug. */
static int npr_seen;
static char pr_seen[4][300];

static int have_data(const char *dir)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    npr_seen = 0;
    if (!d) return 0;
    while ((e = readdir(d))) {
        size_t n = strlen(e->d_name);
        if (n > 3 && (!strcmp(e->d_name + n - 3, ".PR") ||
                      !strcmp(e->d_name + n - 3, ".pr"))) {
            if (npr_seen < 4) {
                strncpy(pr_seen[npr_seen], e->d_name, sizeof pr_seen[0] - 1);
                pr_seen[npr_seen][sizeof pr_seen[0] - 1] = 0;
            }
            npr_seen++;
        }
    }
    closedir(d);
    if (npr_seen != 1) return 0;
    snprintf(prpath_buf, sizeof prpath_buf, "%s/%s", dir, pr_seen[0]);
    return 1;
}

static void find_data(const char *argv0)
{
    static char here[512];
    static char heredata[560], heresrc[560];
    const char *cands[8];
    size_t n = 0, i;
    const char *slash, *bslash;

    if (datadir_buf[0]) cands[n++] = datadir_buf;
    slash  = strrchr(argv0, '/');
    bslash = strrchr(argv0, '\\');
    if (bslash > slash) slash = bslash;
    if (slash && (size_t)(slash - argv0) < sizeof here - 16) {
        memcpy(here, argv0, (size_t)(slash - argv0));
        here[slash - argv0] = 0;
    } else {
        strcpy(here, ".");
    }
    snprintf(heredata, sizeof heredata, "%s/data", here);
    snprintf(heresrc,  sizeof heresrc,  "%s/../src_original", here);
    cands[n++] = ".";
    cands[n++] = "data";
    cands[n++] = here;
    cands[n++] = heredata;
    cands[n++] = heresrc;

    for (i = 0; i < n; i++) {
        if (!have_data(cands[i])) continue;
        if (cands[i] != datadir_buf) {
            strncpy(datadir_buf, cands[i], sizeof datadir_buf - 1);
            datadir_buf[sizeof datadir_buf - 1] = 0;
        }
        return;
    }
    fprintf(stderr,
        "aosvs16: no .PR found.  Put the AOS/VS program and its data files\n"
        "         beside the executable (or in ./data), or point -d at them,\n"
        "         or name the .PR on the command line.\n");
    exit(1);
}

static void usage(void)
{
    fprintf(stderr,
      "aosvs16 -- runs an original AOS or AOS/VS 16-bit program on a Data General\n"
      "           Eclipse emulator.  The program and its data files are the originals.\n"
      "\n"
      "  aosvs16 [options] [<program>.PR] [arguments...]\n"
      "\n"
      "  adventure MYGAME    resumes the game suspended in MYGAME\n"
      "\n"
      "  -d <dir>   directory holding the .PR and its data files\n"
      "  -s <dir>   where the game saves and restores (default: .)\n"
      "  -e <hex>   entry point (default: taken from the startup scan)\n"
      "  -g         enable the # debug verbs (#help lists them)\n"
      "  -L         pass lower case input through (default: fold to capitals)\n"
      "  -v         trace system calls, overlay loads and file I/O\n"
      "  -Z         freeze the clock (the random seed), for repeatable tests\n"
      "  -h         this message\n"
      "\n"
      "Diagnostics: -t instruction trace, -b <hex> breakpoint, -B stop on it,\n"
      "  -W <file> dump the 32K memory image, -n <count> limit,\n"
      "  -k keep going past an unimplemented system call.\n");
    exit(0);
}

int main(int argc, char **argv)
{
    int i;
    unsigned entry = 0;       /* 0 = take it from the .PR header */
    const char *pr = NULL;
    const char *ovl = NULL, *dumpimg = NULL;
    unsigned ovl_fword = 0, ovl_base = 0, ovl_words = 0;

    for (i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-S")) catch_systm = 1;
        else if (!strcmp(argv[i], "-e") && i + 1 < argc) entry = (unsigned)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) maxinstr = strtol(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) pr = argv[++i];
        else if (!strcmp(argv[i], "-D") && i + 1 < argc) dumpimg = argv[++i];
        else if (!strcmp(argv[i], "-c")) C = 1;
        else if (!strcmp(argv[i], "-B")) bp_stop = 1;
        else if (!strcmp(argv[i], "-v")) { verbose = 1; syslog_left = 1000000; }
        else if (!strcmp(argv[i], "-k")) permissive = 1;
        else if (!strcmp(argv[i], "-F")) fptrace = 1;
        else if (!strcmp(argv[i], "-w") && i + 2 < argc) {
            wlo = (word)strtoul(argv[++i], NULL, 16);
            whi = (word)strtoul(argv[++i], NULL, 16);
        }
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            strncpy(datadir_buf, argv[++i], sizeof datadir_buf - 1);
            datadir_buf[sizeof datadir_buf - 1] = 0;
        }
        else if (!strcmp(argv[i], "-h")) usage();
        else if (!strcmp(argv[i], "-g")) debug = 1;
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) savedir = argv[++i];
        else if (!strcmp(argv[i], "-W") && i + 1 < argc) wrimg = argv[++i];
        else if (!strcmp(argv[i], "-b") && i + 1 < argc && bp_n < 8)
            bp_addr[bp_n++] = (word)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "-O") && i + 1 < argc) ovl_force = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-a") && i + 4 < argc) {
            AC[0]=(word)strtoul(argv[++i],NULL,16); AC[1]=(word)strtoul(argv[++i],NULL,16);
            AC[2]=(word)strtoul(argv[++i],NULL,16); AC[3]=(word)strtoul(argv[++i],NULL,16);
        }
        else if (!strcmp(argv[i], "-z") && i + 1 < argc) pzimage = (unsigned)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "-V") && i + 4 < argc) {
            ovl = argv[++i];
            ovl_fword = (unsigned)strtoul(argv[++i], NULL, 16);
            ovl_base  = (unsigned)strtoul(argv[++i], NULL, 16);
            ovl_words = (unsigned)strtoul(argv[++i], NULL, 16);
        }
        else if (!strcmp(argv[i], "-L")) con_upper = 0;
        else if (!strcmp(argv[i], "-Z")) frozen_clock = 1;
        /* A name ending in .PR is the program; any other word is an argument
         * for it, as after "X ADVENTURE" -- "adventure MYGAME" resumes the
         * game suspended in MYGAME. */
        else if (argv[i][0] != '-' && !pr && strlen(argv[i]) > 3 &&
                 (!strcmp(argv[i] + strlen(argv[i]) - 3, ".PR") ||
                  !strcmp(argv[i] + strlen(argv[i]) - 3, ".pr")))
            pr = argv[i];
        else if (argv[i][0] != '-' && prog_nargs < 15) prog_args[++prog_nargs] = argv[i];
    }

    /* An explicitly named .PR also settles where its data files live. */
    if (pr && !datadir_buf[0]) {
        const char *sl = strrchr(pr, '/'), *bs = strrchr(pr, '\\');
        if (bs > sl) sl = bs;
        if (sl) {
            size_t k = (size_t)(sl - pr);
            if (k >= sizeof datadir_buf) k = sizeof datadir_buf - 1;
            memcpy(datadir_buf, pr, k); datadir_buf[k] = 0;
        } else strcpy(datadir_buf, ".");
    }

    if (!pr) { find_data(argv[0]); pr = prpath_buf; }

    /* The overlay file sits beside the .PR under the same name.  USTOD==0
     * means the program has no overlays at all and it is never opened. */
    {   size_t k = strlen(pr);
        snprintf(olpath_buf, sizeof olpath_buf, "%s", pr);
        if (k > 3 && !strcmp(olpath_buf + k - 3, ".PR")) strcpy(olpath_buf + k - 3, ".OL");
        else if (k > 3 && !strcmp(olpath_buf + k - 3, ".pr")) strcpy(olpath_buf + k - 3, ".ol");
    }

    if (dumpimg) {                  /* boot from a live memory dump */
        FILE *f = fopen(dumpimg, "rb");
        unsigned a = 0; unsigned char t[2];
        if (!f) { perror(dumpimg); return 1; }
        while (a < MEMWORDS && fread(t,1,2,f)==2) M[a++] = (word)((t[0]<<8)|t[1]);
        fclose(f);
        fprintf(stderr, "booted from %s: %u words\n", dumpimg, a);
    } else {
        if (load_pr(pr)) return 1;
    }
    if (dumpimg) build_low_memory();
    if (ovl) load_overlay(ovl, ovl_fword, ovl_base, ovl_words);

    if (debug) M[DEBUG_FLAG] = 1;   /* armed before the game even starts */
    if (!entry) entry = pr_entry;
    if (!entry) { fprintf(stderr, "no entry point in the .PR header; use -e\n"); return 1; }
    PC = (word)entry;
    if (verbose) fprintf(stderr, "starting at %04X\n\n", PC);

    while (!halted) {
        step();
        if (++icount == maxinstr && maxinstr) {
            fprintf(stderr, "\n*** instruction limit reached at PC=%04X\n", PC);
            break;
        }
    }
    smap_flush(-1, 1);              /* ended by end of input, too */
    if (con_cr_pending) fputc('\n', stdout);
    fflush(stdout);
    if (verbose) fprintf(stderr, "\nstopped after %ld instructions, PC=%04X\n", icount, PC);
    if (wrimg) {
        FILE *o = fopen(wrimg, "wb"); unsigned a;
        if (o) {
            for (a = 0; a < MEMWORDS; a++)
                { fputc(M[a] >> 8, o); fputc(M[a] & 0xFF, o); }
            fclose(o);
            fprintf(stderr, "wrote %s\n", wrimg);
        }
    }
    return 0;
}
