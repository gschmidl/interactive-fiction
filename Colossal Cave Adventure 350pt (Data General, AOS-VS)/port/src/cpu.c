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
#define USTOD_W    (UST + 0x0Du) /* overlay directory address             */
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
static int   chan_console[64];
static long  chan_pos[64];
static int   chan_spos[64];     /* a ?SPOS is pending on this channel */      /* ?SPOS / ?GPOS, in records       */
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
        if (verbose) fprintf(stderr, "\n[?%s, AC0=%04X]\n",
                             code == SC_KILL ? "KILL" : "RETURN", AC[0]);
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
            /* ?ISTI bit ?OFOT asks for output.  Grant it only inside the
             * save directory; a game file always opens read-only. */
            int wants_out = (M[(pkt + P_ISTI) & AMASK] & OFOT) != 0;
            save_path(path, sizeof path, name);
            if (wants_out && file_exists(path))      chan[ch] = fopen(path, "r+b");
            else if (wants_out && !file_exists(path)) {
                read_path(path, sizeof path, name);
                chan[ch] = fopen(path, "rb");
            } else {
                read_path(path, sizeof path, name);
                chan[ch] = fopen(path, "rb");
            }
            if (!chan[ch]) {
                if (verbose) fprintf(stderr, "[?OPEN failed: %s]\n", name);
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
            fprintf(stderr, "   [?READ ch=%d fmt=%d rcl=%d flg=%04X mrs=%04X rec=%u]\n",
                    ch, fmt, rcl, M[(pkt + P_IFLG) & AMASK], M[(pkt + P_IMRS) & AMASK],
                    (unsigned)((M[(pkt+P_IRNH)&AMASK] << 16) | M[(pkt+P_IRNL)&AMASK]));
        if (!chan[ch]) return 0;
        if (rcl <= 0 || rcl > (int)sizeof buf) rcl = (int)sizeof buf;

        if (fmt == RF_DS) {
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
            /* Fixed or dynamic.  ?IPST in ?IFLG means the record number in
             * ?IRNH/?IRNL is absolute, so seek; otherwise read on from where
             * the channel already stands. */
            size_t got;
            long off;
            if (chan_console[ch]) return 0;
            /* ?IRNH/?IRNL address the record directly: the game reads
             * THISSALA.DB6 record 36 straight after DB5 records 0..3, so
             * these are absolute, not a running position.  A logical 2048
             * byte database record is four of these 512 byte ones. */
            /* ?IMRS is only set up by ?OPEN and holds junk in these
             * packets, so the record size is ?IRCL itself. */
            /* Where to read from.  ?IRNH/?IRNL address the record directly
             * -- that is how THISSALA.DB5/DB6 are read, the first read on a
             * channel being record 3 and not record 0, and PARU's ?IPST is
             * clear even there, so the flag cannot be what decides it.
             * ADVENTURE.PR instead calls ?SPOS and then reads with ?IRNL=0,
             * so a ?SPOS since the last transfer is what wins: obey it, and
             * fall back on the record number when there was none. */
            recsize = rcl;
            if (!chan_spos[ch]) {
                off = (((long)M[(pkt + P_IRNH) & AMASK] << 16) |
                               M[(pkt + P_IRNL) & AMASK]) * recsize;
                if (fseek(chan[ch], off, SEEK_SET)) return 0;
            }
            chan_spos[ch] = 0;
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
        if (fmt != RF_DS && !chan_console[ch] && chan[ch] && !chan_spos[ch]) {
            /* Fixed records are addressed the same way on the way out as on
             * the way in, by ?IRNH/?IRNL; writing them sequentially instead
             * leaves the save file the wrong length.  Same ?IPST rule as
             * ?READ -- without it the write goes where the channel stands. */
            long off = (((long)M[(pkt + P_IRNH) & AMASK] << 16) |
                                M[(pkt + P_IRNL) & AMASK]) * (long)rcl;
            if (fseek(chan[ch], off, SEEK_SET)) return 0;
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
        if (chan[ch]) fwrite(buf, 1, (size_t)n, chan[ch]);
        /* A NUL just ends the transfer -- "Initializing ", ".", ".", "."
         * and " Initialized\n" are five records that make one line.  Only a
         * record that runs to ?IRCL without any delimiter at all gets a
         * NEW LINE supplied for it. */
        if (fmt == RF_DS && delim < 0 && chan[ch]) fputc('\n', chan[ch]);
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
        M[USTBL] = (word)cur_pages;
        AC[1] = (word)(cur_pages * PAGEW - 1);
        return 1;
    }

    case SC_GHRZ:
        /* The runtime indexes a five-group table at 0362 with AC0 to get its
         * ticks-per-second and ticks-per-minute constants.  The live machine
         * ends up with (100, 10, 600) at 2FAC..2FAE, which is group 1. */
        AC[0] = 1;
        return 1;

    case SC_LEFE: lef_mode = 1; return 1;
    case SC_LEFD: lef_mode = 0; return 1;
    case SC_LEFS: AC[0] = (word)lef_mode; return 1;

    case SC_IFPU:                   /* no FPU is needed by the game        */
        return 1;

    case SC_GTMES:                  /* no CLI arguments                    */
        AC[0] = 0; AC[1] = 0;
        return 1;

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
        time_t t = time(NULL);
        struct tm *lt = localtime(&t);
        AC[0] = (word)lt->tm_sec; AC[1] = (word)lt->tm_min; AC[2] = (word)lt->tm_hour;
        return 1;
    }

    case SC_GDAY: {                 /* day, month, year - 1900           */
        time_t t = time(NULL);
        struct tm *lt = localtime(&t);
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
         * does real arithmetic in it.  FPSH/FPOP stay no-ops: they save and
         * restore FPAC state around a call, and since nothing here clobbers
         * an FPAC behind the caller's back, doing neither is symmetric. */
        case E1_RTN:  do_rtn(); return;
        case E1_POPJ: PC = (word)(pop() & AMASK); return;
        case E1_POPB: do_popb(); return;
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
        case E1_PSHR: push((word)((C ? 0x8000 : 0) | (AC[3] & AMASK))); return;
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
            w2 = M[PC]; PC = (word)((PC + 1) & AMASK);
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
                       return; }
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
            case E1_MSP: SP = (word)((SP + AC[acd]) & AMASK); return;
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
                       return; }
        }
        if (fp_exec(ir, at)) return;
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
      "aosvs16 -- runs an AOS/VS 16-bit program on a Data General Eclipse\n"
      "           emulator.  The program and its data files are the originals.\n"
      "\n"
      "  aosvs16 [options] [<program>.PR]\n"
      "\n"
      "  -d <dir>   directory holding the .PR and its data files\n"
      "  -s <dir>   where the game saves and restores (default: .)\n"
      "  -e <hex>   entry point (default: taken from the startup scan)\n"
      "  -g         enable the # debug verbs (#help lists them)\n"
      "  -v         trace system calls, overlay loads and file I/O\n"
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
        else if (argv[i][0] != '-') pr = argv[i];
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
