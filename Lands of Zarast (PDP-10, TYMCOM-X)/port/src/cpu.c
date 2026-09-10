/* cpu.c -- DECsystem-10 (KI10-ish) processor emulation.
 *
 * Enough of the architecture to run a FORTRAN-10 program linked with
 * FOROTS: the full non-privileged instruction set, single and double
 * precision floating point, byte instructions, and UUO dispatch.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "pdp10.h"

w36 M[MEMTOP];
int PC;
int FLAGS;
int halted;
long long insn_count;
int trace;

void fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fflush(stdout);
    fprintf(stderr, "\n[explor: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, " at PC=%06o after %lld instructions]\n", PC, insn_count);
    va_end(ap);
    monitor_shutdown();
    exit(2);
}

/* ------------------------------------------------------------------ */
/* effective address                                                    */
/* ------------------------------------------------------------------ */
static w36 last_ea_word;                 /* for JRSTF flag restoration */

int effective_address(w36 inst)
{
    int i = (int)((inst >> 22) & 1);
    int x = (int)((inst >> 18) & 017);
    int y = (int)(inst & HMASK);
    int guard = 0;

    last_ea_word = inst;
    for (;;) {
        if (x) {
            last_ea_word = AC(x);
            y = (y + (int)RH(AC(x))) & HMASK;
        }
        if (!i)
            return y;
        if (++guard > 1000)
            fatal("indirect address loop");
        {
            w36 w = M[y];
            last_ea_word = w;
            i = (int)((w >> 22) & 1);
            x = (int)((w >> 18) & 017);
            y = (int)(w & HMASK);
        }
    }
}

/* ------------------------------------------------------------------ */
/* single precision floating point                                      */
/* ------------------------------------------------------------------ */
/* A PDP-10 float is  frac * 2^(exp-128-27), frac a 27 bit magnitude,
 * exponent excess-128 in bits 1..8.  Negative values are the two's
 * complement of the whole 36 bit word.                                */

static void funpack(w36 w, int *sign, int *exp, uint64_t *frac)
{
    int neg = (w & SIGNBIT) != 0;
    if (neg)
        w = (~w + 1) & WMASK;
    *sign = neg;
    *exp  = (int)((w >> 27) & 0377);
    *frac = w & 0777777777ULL;
}

/* Assemble sign * mag * 2^(exp-128-27-guard) into a packed word. */
static w36 fpack(int sign, int64_t mag, int exp, int guard, int round)
{
    int want, top;
    if (mag == 0)
        return 0;
    if (mag < 0) { sign ^= 1; mag = -mag; }

    want = 26 + guard;
    top = 63;
    while (top > 0 && !((uint64_t)mag >> top))
        top--;
    if (top > want) { int d = top - want; mag >>= d; exp += d; }
    else if (top < want) { int d = want - top; mag <<= d; exp -= d; }

    if (guard) {
        if (round)
            mag += (int64_t)1 << (guard - 1);
        mag >>= guard;
        if (mag >> 27) { mag >>= 1; exp++; }
    }

    if (exp > 0377) { FLAGS |= F_AROV | F_FOV; exp &= 0377; }
    if (exp < 0)    { FLAGS |= F_AROV | F_FOV | F_FXU; exp &= 0377; }

    {
        w36 r = ((w36)(exp & 0377) << 27) | ((w36)mag & 0777777777ULL);
        return sign ? ((~r + 1) & WMASK) : r;
    }
}

#define FGUARD 30

static w36 fadd(w36 a, w36 b, int sub, int round)
{
    int sa, sb, ea, eb, emax, d;
    uint64_t fa, fb;
    int64_t ma, mb;

    funpack(a, &sa, &ea, &fa);
    funpack(b, &sb, &eb, &fb);
    if (sub) sb ^= 1;
    if (fa == 0) ea = eb;
    if (fb == 0) eb = ea;

    emax = ea > eb ? ea : eb;
    ma = sa ? -(int64_t)fa : (int64_t)fa;
    mb = sb ? -(int64_t)fb : (int64_t)fb;
    ma <<= FGUARD; mb <<= FGUARD;
    d = emax - ea; ma = (d >= 63) ? 0 : (ma >> d);
    d = emax - eb; mb = (d >= 63) ? 0 : (mb >> d);
    return fpack(0, ma + mb, emax, FGUARD, round);
}

static w36 fmul(w36 a, w36 b, int round)
{
    int sa, sb, ea, eb;
    uint64_t fa, fb;
    funpack(a, &sa, &ea, &fa);
    funpack(b, &sb, &eb, &fb);
    if (fa == 0 || fb == 0)
        return 0;
    return fpack(sa ^ sb, (int64_t)(fa * fb), ea + eb - 128, 27, round);
}

static w36 fdiv(w36 a, w36 b, int round, int *ok)
{
    int sa, sb, ea, eb;
    uint64_t fa, fb;
    funpack(a, &sa, &ea, &fa);
    funpack(b, &sb, &eb, &fb);
    *ok = 1;
    if (fb == 0) { FLAGS |= F_AROV | F_FOV | F_DCK; *ok = 0; return 0; }
    if (fa == 0) return 0;
    return fpack(sa ^ sb, (int64_t)(((uint64_t)fa << 29) / fb),
                 ea - eb + 155, 29, round);
}

static int64_t ffix(w36 w, int round)
{
    int s, e; uint64_t f; int64_t v; int sh;
    funpack(w, &s, &e, &f);
    if (f == 0) return 0;
    sh = e - 128 - 27;
    if (sh >= 0) {
        if (sh > 36) { FLAGS |= F_AROV; return 0; }
        v = (int64_t)(f << sh);
    } else if (sh > -64) {
        uint64_t half = (uint64_t)1 << (-sh - 1);
        uint64_t drop = f & (((uint64_t)1 << (-sh)) - 1);
        v = (int64_t)(f >> (-sh));
        if (round && drop >= half)
            v++;
    } else
        v = 0;
    return s ? -v : v;
}

static w36 fflt(int64_t v, int round)
{
    if (v == 0) return 0;
    return fpack(0, v, 128 + 27, 0, round);
}

/* ------------------------------------------------------------------ */
/* double precision (KI10 format: 62 bit fraction over two words)       */
/* ------------------------------------------------------------------ */
static void dunpack(w36 hi, w36 lo, int *sign, int *exp, uint64_t *frac)
{
    int neg = (hi & SIGNBIT) != 0;
    if (neg) {
        w36 l = (~lo + 1) & WMASK;
        w36 h = (~hi + (l == 0 ? 1 : 0)) & WMASK;
        hi = h; lo = l;
    }
    *sign = neg;
    *exp  = (int)((hi >> 27) & 0377);
    *frac = ((hi & 0777777777ULL) << 35) | (lo & 0377777777777ULL);
}

static void dpack(int sign, int64_t mag, int exp, w36 *hi, w36 *lo)
{
    int top;
    if (mag == 0) { *hi = *lo = 0; return; }
    if (mag < 0) { sign ^= 1; mag = -mag; }
    top = 63;
    while (top > 0 && !((uint64_t)mag >> top)) top--;
    if (top > 61) { int d = top - 61; mag >>= d; exp += d; }
    else if (top < 61) { int d = 61 - top; mag <<= d; exp -= d; }
    if (exp > 0377 || exp < 0) { FLAGS |= F_AROV | F_FOV; exp &= 0377; }
    *hi = ((w36)(exp & 0377) << 27) | ((w36)((uint64_t)mag >> 35) & 0777777777ULL);
    *lo = (w36)((uint64_t)mag & 0377777777777ULL);
    if (sign) {
        w36 l = (~*lo + 1) & WMASK;
        w36 h = (~*hi + (l == 0 ? 1 : 0)) & WMASK;
        *hi = h; *lo = l;
    }
}

/* ------------------------------------------------------------------ */
/* byte pointers                                                        */
/* ------------------------------------------------------------------ */
static void bp_incr(w36 *bp)
{
    int p = (int)((*bp >> 30) & 077);
    int s = (int)((*bp >> 24) & 077);
    p -= s;
    if (p < 0) {
        p = 36 - s;
        *bp = (*bp & ~(w36)HMASK) | ((*bp + 1) & HMASK);
    }
    *bp = (*bp & ~((w36)077 << 30)) | ((w36)(p & 077) << 30);
}

static w36 bp_load(w36 bp)
{
    int p = (int)((bp >> 30) & 077);
    int s = (int)((bp >> 24) & 077);
    int e = effective_address(bp);
    w36 mask;
    if (s == 0 || p > 35) return 0;
    mask = (s >= 36) ? WMASK : (((w36)1 << s) - 1);
    return (M[e] >> p) & mask;
}

static void bp_store(w36 bp, w36 val)
{
    int p = (int)((bp >> 30) & 077);
    int s = (int)((bp >> 24) & 077);
    int e = effective_address(bp);
    w36 mask;
    if (s == 0 || p > 35) return;
    mask = (s >= 36) ? WMASK : (((w36)1 << s) - 1);
    M[e] = ((M[e] & ~(mask << p)) | ((val & mask) << p)) & WMASK;
}

/* ------------------------------------------------------------------ */
/* helpers                                                              */
/* ------------------------------------------------------------------ */
static w36 addw(w36 a, w36 b)
{
    w36 r = (a + b) & WMASK;
    w36 c = (a & b) | ((a ^ b) & ~r);
    int c0 = (int)((c >> 35) & 1);
    int c1 = (int)((c >> 34) & 1);
    if (c0) FLAGS |= F_CRY0;
    if (c1) FLAGS |= F_CRY1;
    if (c0 != c1) FLAGS |= F_AROV;
    return r;
}

static w36 negw(w36 a)
{
    w36 r = (~a + 1) & WMASK;
    if (a == 0) FLAGS |= F_CRY0 | F_CRY1;
    else if (a == SIGNBIT) FLAGS |= F_CRY1 | F_AROV;
    return r;
}

static int cond(int c, long long v)
{
    switch (c & 7) {
    case 0: return 0;
    case 1: return v <  0;
    case 2: return v == 0;
    case 3: return v <= 0;
    case 4: return 1;
    case 5: return v >= 0;
    case 6: return v != 0;
    default:return v >  0;
    }
}

static w36 swaphalves(w36 w) { return (((w << 18) | (w >> 18)) & WMASK); }

static void execute(w36 inst, int depth);

/* ------------------------------------------------------------------ */
int watch_addr = -1;

int cpu_step(void)
{
    w36 inst, before = 0;
    int at = PC;
    if (PC < 0 || PC >= MEMTOP) fatal("PC out of range");
    inst = M[PC];
    PC = (PC + 1) & HMASK;
    insn_count++;
    if (watch_addr >= 0) before = M[watch_addr];
    execute(inst, 0);
    if (watch_addr >= 0 && M[watch_addr] != before) {
        fflush(stdout);
        fprintf(stderr, "[watch] %06o: %012llo -> %012llo  by %06o (%012llo)\n",
                watch_addr, (unsigned long long)before,
                (unsigned long long)M[watch_addr], at,
                (unsigned long long)inst);
    }
    return halted;
}

void cpu_run(void)
{
    while (!halted)
        cpu_step();
}

/* Core already holds the image -- load_shr() put it there, .JBREL and
 * .JBFF included -- so all that is left is the job data area the monitor
 * itself would have filled in, and the processor state. */
void cpu_reset(void)
{
    /* .JBS41 (JOBDAT 122) is where SAVE stashes the program's LUUO trap
     * instruction; location 41 itself is rewritten by GET/RUN, so the
     * loader puts it back.  EXPLOR is compiled by F40, whose code calls
     * the object time system through LUUOs (opcodes 001-037) some six
     * hundred times over, so without this the first library call traps
     * into an empty location 41.  Nothing in the image ever stores into
     * 41, and 122 holds a JSR to the handler that picks the LUUO up out
     * of 40 and dispatches on its opcode field. */
    if (M[0122])
        M[041] = M[0122];

    PC = image_start;
    FLAGS = F_USER;
    halted = 0;
}

/* ------------------------------------------------------------------ */
static void execute(w36 inst, int depth)
{
    int op = (int)((inst >> 27) & 0777);
    int ac = (int)((inst >> 23) & 017);
    int e;

    if (depth > 64) fatal("XCT nesting too deep");

    e = effective_address(inst);

    if (trace)
        fprintf(stderr, "%06o %03o %02o %06o  ac=%012llo\n",
                (PC - 1) & HMASK, op, ac, e, (unsigned long long)AC(ac));

    /* ---- UUOs and user mode I/O traps ---------------------------- */
    if (op >= 0700) { monitor_uuo(inst, op, ac, e); return; }
    if (op == 0) fatal("illegal instruction 000");
    if (op <= 037) {                        /* LUUO                    */
        M[040] = (((w36)op << 27) | ((w36)ac << 23) | (w36)e) & WMASK;
        execute(M[041], depth + 1);
        return;
    }
    if (op <= 077) {                        /* MUUO -- monitor call    */
        int skip = monitor_uuo(inst, op, ac, e);
        PC = (PC + skip) & HMASK;
        return;
    }

    switch (op) {

    /* ---- double word moves --------------------------------------- */
    case 0120: AC(ac) = M[e]; AC(ac + 1) = M[(e + 1) & HMASK]; return;
    case 0124: M[e] = AC(ac); M[(e + 1) & HMASK] = AC(ac + 1); return;
    case 0121: {
        w36 lo = (~M[(e + 1) & HMASK] + 1) & WMASK;
        w36 hi = (~M[e] + (lo == 0 ? 1 : 0)) & WMASK;
        AC(ac) = hi; AC(ac + 1) = lo & 0377777777777ULL; return; }
    case 0125: {
        w36 lo = (~AC(ac + 1) + 1) & WMASK;
        w36 hi = (~AC(ac) + (lo == 0 ? 1 : 0)) & WMASK;
        M[e] = hi; M[(e + 1) & HMASK] = lo & 0377777777777ULL; return; }

    /* ---- double precision floating point -------------------------- */
    case 0110: case 0111: {
        int s1, s2, e1, e2, emax, d; uint64_t f1, f2; int64_t m1, m2; w36 h, l;
        dunpack(AC(ac), AC(ac + 1), &s1, &e1, &f1);
        dunpack(M[e], M[(e + 1) & HMASK], &s2, &e2, &f2);
        if (op == 0111) s2 ^= 1;
        if (f1 == 0) e1 = e2;
        if (f2 == 0) e2 = e1;
        emax = e1 > e2 ? e1 : e2;
        m1 = s1 ? -(int64_t)f1 : (int64_t)f1;
        m2 = s2 ? -(int64_t)f2 : (int64_t)f2;
        d = emax - e1; m1 = (d >= 63) ? 0 : (m1 >> d);
        d = emax - e2; m2 = (d >= 63) ? 0 : (m2 >> d);
        dpack(0, m1 + m2, emax, &h, &l);
        AC(ac) = h; AC(ac + 1) = l; return; }
    case 0112: {
        int s1, s2, e1, e2; uint64_t f1, f2; w36 h, l;
        unsigned __int128 p;
        dunpack(AC(ac), AC(ac + 1), &s1, &e1, &f1);
        dunpack(M[e], M[(e + 1) & HMASK], &s2, &e2, &f2);
        if (f1 == 0 || f2 == 0) { AC(ac) = 0; AC(ac + 1) = 0; return; }
        p = (unsigned __int128)f1 * (unsigned __int128)f2;
        dpack(s1 ^ s2, (int64_t)(uint64_t)(p >> 62), e1 + e2 - 128, &h, &l);
        AC(ac) = h; AC(ac + 1) = l; return; }
    /* Double-word fixed point.  DSUB is the only one the TBA runtime
     * reaches, in its integer-overflow check, but they come as a set. */
    case 0114: case 0115: {                                   /* DADD/DSUB */
        unsigned __int128 a = ((unsigned __int128)AC(ac) << 35)
                            | (AC(ac + 1) & 0377777777777ULL);
        unsigned __int128 b = ((unsigned __int128)M[e] << 35)
                            | (M[(e + 1) & HMASK] & 0377777777777ULL);
        unsigned __int128 r = (op == 0114) ? a + b : a - b;
        AC(ac) = (w36)((r >> 35) & WMASK);
        AC(ac + 1) = (w36)(r & 0377777777777ULL);
        return; }

    case 0113: {
        int s1, s2, e1, e2; uint64_t f1, f2; w36 h, l;
        unsigned __int128 num;
        dunpack(AC(ac), AC(ac + 1), &s1, &e1, &f1);
        dunpack(M[e], M[(e + 1) & HMASK], &s2, &e2, &f2);
        if (f2 == 0) { FLAGS |= F_AROV | F_FOV | F_DCK; return; }
        if (f1 == 0) { AC(ac) = 0; AC(ac + 1) = 0; return; }
        num = (unsigned __int128)f1 << 61;
        dpack(s1 ^ s2, (int64_t)(uint64_t)(num / f2), e1 - e2 + 129, &h, &l);
        AC(ac) = h; AC(ac + 1) = l; return; }

    case 0122: AC(ac) = (w36)ffix(M[e], 0) & WMASK; return;   /* FIX  */
    case 0126: AC(ac) = (w36)ffix(M[e], 1) & WMASK; return;   /* FIXR */
    case 0127: AC(ac) = fflt(sx36(M[e]), 1); return;          /* FLTR */

    case 0130: AC(ac + 1) = fadd(AC(ac), M[e], 0, 0); return; /* UFA  */
    case 0131: {                                              /* DFN  */
        w36 lo = (~M[e] + 1) & WMASK;
        w36 hi = (~AC(ac) + (lo == 0 ? 1 : 0)) & WMASK;
        AC(ac) = hi; M[e] = lo & 0377777777777ULL; return; }
    case 0132: {                                              /* FSC  */
        int s, ex; uint64_t f;
        funpack(AC(ac), &s, &ex, &f);
        if (f == 0) { AC(ac) = 0; return; }
        AC(ac) = fpack(s, (int64_t)f, ex + sx18(e), 0, 0); return; }

    case 0133:
        if (ac == 0) { bp_incr(&M[e]); return; }              /* IBP  */
        fatal("ADJBP not implemented");
        /* fall through */
    case 0134: bp_incr(&M[e]); AC(ac) = bp_load(M[e]); return;/* ILDB */
    case 0135: AC(ac) = bp_load(M[e]); return;                /* LDB  */
    case 0136: bp_incr(&M[e]); bp_store(M[e], AC(ac)); return;/* IDPB */
    case 0137: bp_store(M[e], AC(ac)); return;                /* DPB  */

    /* ---- integer multiply and divide ------------------------------ */
    case 0220: case 0221: case 0222: case 0223: {             /* IMUL */
        w36 b = (op == 0221) ? (w36)e : M[e];
        long long p = sx36(AC(ac)) * sx36(b);
        w36 r = (w36)p & WMASK;
        if (op == 0220 || op == 0221) AC(ac) = r;
        else if (op == 0222) M[e] = r;
        else { AC(ac) = r; M[e] = r; }
        return; }
    case 0224: case 0225: case 0226: case 0227: {             /* MUL  */
        w36 b = (op == 0225) ? (w36)e : M[e];
        __int128 p = (__int128)sx36(AC(ac)) * (__int128)sx36(b);
        long long hi = (long long)(p >> 35);
        w36 lo = (w36)((unsigned __int128)p & 0377777777777ULL);
        w36 hw = (w36)hi & WMASK;
        if (p < 0) lo |= SIGNBIT;
        if (op == 0226) { M[e] = hw; }
        else { AC(ac) = hw; AC(ac + 1) = lo; if (op == 0227) M[e] = hw; }
        return; }
    case 0230: case 0231: case 0232: case 0233: {             /* IDIV */
        w36 b = (op == 0231) ? (w36)e : M[e];
        long long d = sx36(b), n = sx36(AC(ac));
        w36 q, r;
        if (d == 0) { FLAGS |= F_AROV | F_DCK; return; }
        q = (w36)(n / d) & WMASK;
        r = (w36)(n % d) & WMASK;
        if (op == 0232) M[e] = q;
        else { AC(ac) = q; AC(ac + 1) = r; if (op == 0233) M[e] = q; }
        return; }
    case 0234: case 0235: case 0236: case 0237: {             /* DIV  */
        w36 b = (op == 0235) ? (w36)e : M[e];
        long long d = sx36(b);
        __int128 n;
        if (d == 0) { FLAGS |= F_AROV | F_DCK; return; }
        n = ((__int128)sx36(AC(ac)) << 35) | (__int128)(AC(ac + 1) & 0377777777777ULL);
        {
            __int128 q = n / d, r = n % d;
            if (q > (__int128)0377777777777LL || q < -((__int128)1 << 35)) {
                FLAGS |= F_AROV | F_DCK; return;
            }
            if (op == 0236) M[e] = (w36)q & WMASK;
            else {
                AC(ac) = (w36)q & WMASK;
                AC(ac + 1) = (w36)r & WMASK;
                if (op == 0237) M[e] = (w36)q & WMASK;
            }
        }
        return; }

    /* ---- shifts --------------------------------------------------- */
    case 0240: {                                              /* ASH  */
        int n = sx18(e);
        long long v = sx36(AC(ac));
        if (n > 0) { if (n > 63) v = (v < 0) ? -1 : 0; else v <<= n; }
        else if (n < 0) { n = -n; if (n > 63) n = 63; v >>= n; }
        AC(ac) = (w36)v & WMASK; return; }
    case 0241: {                                              /* ROT  */
        int n = sx18(e) % 36; w36 v = AC(ac);
        if (n < 0) n += 36;
        if (n) AC(ac) = ((v << n) | (v >> (36 - n))) & WMASK;
        return; }
    case 0242: {                                              /* LSH  */
        int n = sx18(e); w36 v = AC(ac);
        if (n > 0)  AC(ac) = (n > 35) ? 0 : ((v << n) & WMASK);
        else if (n < 0) { n = -n; AC(ac) = (n > 35) ? 0 : (v >> n); }
        return; }
    case 0243: {                                              /* JFFO */
        w36 v = AC(ac); int n = 0;
        if (v == 0) { AC(ac + 1) = 0; return; }
        while (!(v & SIGNBIT)) { v <<= 1; n++; }
        AC(ac + 1) = (w36)n; PC = e; return; }
    case 0244: {                                              /* ASHC */
        int n = sx18(e);
        __int128 v = ((__int128)sx36(AC(ac)) << 35) |
                     (__int128)(AC(ac + 1) & 0377777777777ULL);
        if (n > 0) { if (n > 100) v = (v < 0) ? -1 : 0; else v <<= n; }
        else if (n < 0) { n = -n; if (n > 100) n = 100; v >>= n; }
        AC(ac) = (w36)((v >> 35) & (__int128)WMASK);
        AC(ac + 1) = (w36)(v & 0377777777777ULL) |
                     ((AC(ac) & SIGNBIT) ? SIGNBIT : 0);
        return; }
    case 0245: {                                              /* ROTC */
        int n = sx18(e) % 72; w36 h = AC(ac), l = AC(ac + 1);
        unsigned __int128 v = ((unsigned __int128)h << 36) | l;
        if (n < 0) n += 72;
        if (n) v = ((v << n) | (v >> (72 - n))) &
                   ((((unsigned __int128)1) << 72) - 1);
        AC(ac) = (w36)(v >> 36) & WMASK;
        AC(ac + 1) = (w36)v & WMASK;
        return; }
    case 0246: {                                              /* LSHC */
        int n = sx18(e);
        unsigned __int128 v = ((unsigned __int128)AC(ac) << 36) | AC(ac + 1);
        if (n > 0) v = (n > 71) ? 0 : (v << n);
        else if (n < 0) { n = -n; v = (n > 71) ? 0 : (v >> n); }
        AC(ac) = (w36)(v >> 36) & WMASK;
        AC(ac + 1) = (w36)v & WMASK;
        return; }

    /* ---- misc ----------------------------------------------------- */
    case 0250: { w36 t = AC(ac); AC(ac) = M[e]; M[e] = t; return; }   /* EXCH */
    case 0251: {                                              /* BLT  */
        /* The pointer lives in the AC itself, so a BLT whose destination
         * range covers the AC (the standard "restore all accumulators"
         * idiom, HRLZI 16,SAVE / BLT 16,16) leaves the transferred word
         * in it rather than a bumped pointer.  Emulate that literally. */
        long n = 0;
        for (;;) {
            int src = (int)LH(AC(ac)), dst = (int)RH(AC(ac));
            M[dst] = M[src];
            if (dst == e) break;
            AC(ac) = XWD(src + 1, dst + 1);
            if (++n > MEMTOP) fatal("runaway BLT");
        }
        return; }
    /* AOBJP/AOBJN increment the two halves independently: a carry out of
     * the right half does not run into the left.  That is the KI-10
     * behaviour, and F40's object time system checks for it on the way
     * in -- SETO 0, / AOBJN 0,.+1 / JUMPE, which only reaches zero if
     * the halves are separate -- and refuses to run with "?KI-10 CODE
     * WILL NOT RUN ON A KA-10" if they are not.  It makes no difference
     * to an ordinary AOBJN loop, where the right half is an address that
     * never wraps. */
    case 0252: {                                              /* AOBJP */
        AC(ac) = XWD(LH(AC(ac)) + 1, RH(AC(ac)) + 1);
        if (!(AC(ac) & SIGNBIT)) PC = e;
        return; }
    case 0253: {                                              /* AOBJN */
        AC(ac) = XWD(LH(AC(ac)) + 1, RH(AC(ac)) + 1);
        if (AC(ac) & SIGNBIT) PC = e;
        return; }
    case 0254:                                                /* JRST  */
        switch (ac) {
        case 0: case 1: PC = e; return;
        case 2: FLAGS = (int)LH(last_ea_word); PC = e; return;   /* JRSTF */
        case 4: halted = 1; PC = e; return;                      /* HALT  */
        case 010: case 012: PC = e; return;                      /* JEN   */
        default: PC = e; return;
        }
    case 0255: {                                              /* JFCL  */
        int mask = 0;
        if (ac & 010) mask |= F_AROV;
        if (ac & 004) mask |= F_CRY0;
        if (ac & 002) mask |= F_CRY1;
        if (ac & 001) mask |= F_FOV;
        if (FLAGS & mask) { FLAGS &= ~mask; PC = e; }
        return; }
    case 0256: execute(M[e], depth + 1); return;              /* XCT   */
    case 0257: return;                                        /* MAP   */

    case 0260: {                                              /* PUSHJ */
        AC(ac) = (AC(ac) + XWD(1, 1)) & WMASK;
        M[RH(AC(ac))] = XWD(FLAGS, PC);
        if (!(AC(ac) & SIGNBIT) && LH(AC(ac)) == 0) FLAGS |= F_TRAP2;
        PC = e; return; }
    case 0261: {                                              /* PUSH  */
        AC(ac) = (AC(ac) + XWD(1, 1)) & WMASK;
        M[RH(AC(ac))] = M[e];
        return; }
    case 0262: {                                              /* POP   */
        M[e] = M[RH(AC(ac))];
        AC(ac) = (AC(ac) - XWD(1, 1)) & WMASK;
        return; }
    case 0263: {                                              /* POPJ  */
        PC = (int)RH(M[RH(AC(ac))]);
        AC(ac) = (AC(ac) - XWD(1, 1)) & WMASK;
        return; }
    case 0264: M[e] = XWD(FLAGS, PC); PC = (e + 1) & HMASK; return;   /* JSR */
    case 0265: AC(ac) = XWD(FLAGS, PC); PC = e; return;              /* JSP */
    case 0266: M[e] = AC(ac); AC(ac) = XWD(e, PC);
               PC = (e + 1) & HMASK; return;                         /* JSA */
    case 0267: AC(ac) = M[LH(AC(ac))]; PC = e; return;               /* JRA */

    case 0270: case 0271: case 0272: case 0273: {             /* ADD  */
        w36 b = (op == 0271) ? (w36)e : M[e];
        w36 r = addw(AC(ac), b);
        if (op == 0272) M[e] = r;
        else { AC(ac) = r; if (op == 0273) M[e] = r; }
        return; }
    case 0274: case 0275: case 0276: case 0277: {             /* SUB  */
        w36 b = (op == 0275) ? (w36)e : M[e];
        w36 r = addw(AC(ac), (~b + 1) & WMASK);
        if (op == 0276) M[e] = r;
        else { AC(ac) = r; if (op == 0277) M[e] = r; }
        return; }

    default: break;
    }

    /* ---- single precision floating point (140..177) --------------- */
    if (op >= 0140 && op <= 0177) {
        int kind = (op - 0140) >> 3;        /* 0=FAD 1=FSB 2=FMP 3=FDV */
        int round = (op & 4) != 0;
        int mode = op & 3;
        int ok = 1;
        w36 a = AC(ac), b, r;
        if (mode == 1 && round) b = ((w36)e << 18) & WMASK;   /* xxxRI  */
        else b = M[e];
        switch (kind) {
        case 0: r = fadd(a, b, 0, round); break;
        case 1: r = fadd(a, b, 1, round); break;
        case 2: r = fmul(a, b, round); break;
        default:r = fdiv(a, b, round, &ok); break;
        }
        if (!ok) return;
        switch (mode) {
        case 0: AC(ac) = r; break;
        case 1: AC(ac) = r; if (!round) AC(ac + 1) = 0; break;
        case 2: M[e] = r; break;
        default:AC(ac) = r; M[e] = r; break;
        }
        return;
    }

    /* ---- full word moves (200..217) ------------------------------- */
    if (op >= 0200 && op <= 0217) {
        int kind = (op >> 2) & 3;
        int mode = op & 3;
        w36 src, r;
        switch (mode) {
        case 1:  src = (w36)e; break;
        case 2:  src = AC(ac); break;
        default: src = M[e];   break;
        }
        switch (kind) {
        case 0: r = src; break;
        case 1: r = swaphalves(src); break;
        case 2: r = negw(src); break;
        default:r = (src & SIGNBIT) ? negw(src) : src; break;
        }
        if (mode == 0 || mode == 1) AC(ac) = r;
        else if (mode == 2) M[e] = r;
        else { M[e] = r; if (ac) AC(ac) = r; }
        return;
    }

    /* ---- compare / jump / skip groups (300..377) ------------------ */
    if (op >= 0300 && op <= 0377) {
        int grp = (op - 0300) >> 3;         /* 0..7 within 300..377    */
        int c = op & 7;
        switch (grp) {
        case 0:  /* CAI */
            if (cond(c, sx36(AC(ac)) - (long long)e)) PC = (PC + 1) & HMASK;
            return;
        case 1:  /* CAM */
            if (cond(c, sx36(AC(ac)) - sx36(M[e]))) PC = (PC + 1) & HMASK;
            return;
        case 2:  /* JUMP */
            if (cond(c, sx36(AC(ac)))) PC = e;
            return;
        case 3:  /* SKIP */
            if (ac) AC(ac) = M[e];
            if (cond(c, sx36(M[e]))) PC = (PC + 1) & HMASK;
            return;
        case 4:  /* AOJ */
            AC(ac) = addw(AC(ac), 1);
            if (cond(c, sx36(AC(ac)))) PC = e;
            return;
        case 5:  /* AOS */
            M[e] = addw(M[e], 1);
            if (ac) AC(ac) = M[e];
            if (cond(c, sx36(M[e]))) PC = (PC + 1) & HMASK;
            return;
        case 6:  /* SOJ */
            AC(ac) = addw(AC(ac), WMASK);
            if (cond(c, sx36(AC(ac)))) PC = e;
            return;
        default: /* SOS */
            M[e] = addw(M[e], WMASK);
            if (ac) AC(ac) = M[e];
            if (cond(c, sx36(M[e]))) PC = (PC + 1) & HMASK;
            return;
        }
    }

    /* ---- boolean group (400..477) --------------------------------- */
    if (op >= 0400 && op <= 0477) {
        int f = (op - 0400) >> 2;
        int mode = op & 3;
        w36 a = AC(ac), m, r;
        m = (mode == 1) ? (w36)e : M[e];
        switch (f) {
        case 0:  r = 0; break;
        case 1:  r = a & m; break;
        case 2:  r = ~a & m; break;
        case 3:  r = m; break;
        case 4:  r = a & ~m; break;
        case 5:  r = a; break;
        case 6:  r = a ^ m; break;
        case 7:  r = a | m; break;
        case 8:  r = ~a & ~m; break;
        case 9:  r = ~(a ^ m); break;
        case 10: r = ~a; break;
        case 11: r = ~a | m; break;
        case 12: r = ~m; break;
        case 13: r = a | ~m; break;
        case 14: r = ~a | ~m; break;
        default: r = WMASK; break;
        }
        r &= WMASK;
        if (mode == 0 || mode == 1) AC(ac) = r;
        else if (mode == 2) M[e] = r;
        else { M[e] = r; if (ac) AC(ac) = r; }
        return;
    }

    /* ---- half word moves (500..577) ------------------------------- */
    if (op >= 0500 && op <= 0577) {
        int g = (op - 0500) >> 2;           /* 0..15                    */
        int mode = op & 3;
        int dest_right = (g >= 8);
        int src_right = ((g & 1) != 0) ^ dest_right;
        int fill = (g >> 1) & 3;            /* 0 keep 1 zero 2 one 3 ext*/
        w36 src, dst, moved, r;

        switch (mode) {
        case 1:  src = (w36)e;  dst = AC(ac); break;
        case 2:  src = AC(ac);  dst = M[e];   break;
        case 3:  src = M[e];    dst = M[e];   break;
        default: src = M[e];    dst = AC(ac); break;
        }
        moved = src_right ? RH(src) : LH(src);
        if (dest_right) {
            w36 other;
            switch (fill) {
            case 1:  other = 0; break;
            case 2:  other = HMASK; break;
            case 3:  other = (moved & 0400000) ? HMASK : 0; break;
            default: other = LH(dst); break;
            }
            r = (other << 18) | moved;
        } else {
            w36 other;
            switch (fill) {
            case 1:  other = 0; break;
            case 2:  other = HMASK; break;
            case 3:  other = (moved & 0400000) ? HMASK : 0; break;
            default: other = RH(dst); break;
            }
            r = (moved << 18) | other;
        }
        r &= WMASK;
        if (mode == 0 || mode == 1) AC(ac) = r;
        else if (mode == 2) M[e] = r;
        else { M[e] = r; if (ac) AC(ac) = r; }
        return;
    }

    /* ---- test group (600..677) ------------------------------------ */
    if (op >= 0600 && op <= 0677) {
        int d = op - 0600;
        int modify = d >> 4;                /* 0 N, 1 Z, 2 C, 3 O       */
        int frommem = (d >> 3) & 1;
        int swap = d & 1;
        int c = (d >> 1) & 3;               /* 0 -, 1 E, 2 A, 3 N       */
        w36 mask, bits;

        if (frommem) { mask = M[e]; if (swap) mask = swaphalves(mask); }
        else mask = swap ? (((w36)e << 18) & WMASK) : (w36)e;

        bits = AC(ac) & mask;
        switch (modify) {
        case 1: AC(ac) = AC(ac) & ~mask; break;
        case 2: AC(ac) = AC(ac) ^ mask; break;
        case 3: AC(ac) = AC(ac) | mask; break;
        default: break;
        }
        AC(ac) &= WMASK;
        if ((c == 1 && bits == 0) || c == 2 || (c == 3 && bits != 0))
            PC = (PC + 1) & HMASK;
        return;
    }

    fatal("unimplemented opcode %03o", op);
}
