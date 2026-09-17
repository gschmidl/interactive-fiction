/*
 * cpu.c - ND-100/CX user-mode instruction set.
 *
 * One 64K-word address space, no paging (a one-bank :PROG program sees
 * the same memory through the normal and the alternative page table).
 * Privileged instructions and IOX trap back to the caller, as they would
 * trap to SINTRAN on the real machine.
 */
#include "nd100.h"

#define A  (c->r[R_A])
#define D  (c->r[R_D])
#define T  (c->r[R_T])
#define X  (c->r[R_X])
#define B  (c->r[R_B])
#define L  (c->r[R_L])
#define P  (c->r[R_P])
#define STS (c->r[R_STS])
#define M(a) (c->mem[(uint16_t)(a)])

static void setf(Cpu *c, uint16_t bit, int on)
{
    if (on) STS |= bit; else STS &= (uint16_t)~bit;
}

/* 16-bit add with ND flag rules: C = carry out, O sticky, Q = this overflow */
static uint16_t add16(Cpu *c, uint16_t a, uint16_t b, unsigned k)
{
    uint32_t t = (uint32_t)a + b + k;
    uint16_t r = (uint16_t)t;
    setf(c, S_C, t > 0xFFFF);
    if (!((a ^ b) & 0x8000) && ((a ^ r) & 0x8000)) {
        STS |= S_O | S_Q;
    } else {
        STS &= (uint16_t)~S_Q;
    }
    return r;
}

/* reading/writing <sr>/<dr> register fields: field 0 reads 0, writes vanish */
static uint16_t rget(Cpu *c, int r) { return r ? c->r[r] : 0; }
static void rput(Cpu *c, int r, uint16_t v) { if (r) c->r[r] = v; }

/*
 * Effective address.  pc is the address of the instruction itself.
 *   0 P+d   1 B+d   2 (P+d)   3 (B+d)   4 X+d   5 B+d+X   6 (P+d)+X   7 (B+d)+X
 */
static uint16_t ea(Cpu *c, uint16_t w, uint16_t pc)
{
    int16_t d = (int8_t)(w & 0xFF);
    switch ((w >> 8) & 7) {
    case 0: return (uint16_t)(pc + d);
    case 1: return (uint16_t)(B + d);
    case 2: return M(pc + d);
    case 3: return M(B + d);
    case 4: return (uint16_t)(X + d);
    case 5: return (uint16_t)(B + d + X);
    case 6: return (uint16_t)(M(pc + d) + X);
    default: return (uint16_t)(M(B + d) + X);
    }
}

static int skip_cond(Cpu *c, uint16_t w)
{
    uint16_t s = rget(c, (w >> 3) & 7);
    uint16_t d = rget(c, w & 7);
    uint16_t diff = (uint16_t)(d - s);
    int z = diff == 0;
    int sg = (diff >> 15) & 1;
    int16_t sd = (int16_t)d, ss = (int16_t)s, sgr = (int16_t)diff;
    int o = ((sd & ~ss & ~sgr) | (~sd & ss & sgr)) < 0;
    int cy = d >= s;
    switch ((w >> 8) & 7) {
    case 0: return z;          /* EQL  */
    case 1: return !sg;        /* GEQ  */
    case 2: return !(sg ^ o);  /* GRE  */
    case 3: return cy;         /* MGRE */
    case 4: return !z;         /* UEQ  */
    case 5: return sg;         /* LSS  */
    case 6: return sg ^ o;     /* LST  */
    default: return !cy;       /* MLST */
    }
}

static void regop(Cpu *c, uint16_t w)
{
    int sr = (w >> 3) & 7, dr = w & 7;
    int cm1 = (w >> 7) & 1, cld = (w >> 6) & 1;
    uint16_t src = rget(c, sr);
    uint16_t dst = cld ? 0 : rget(c, dr);

    if (!((w >> 10) & 1)) {
        switch ((w >> 8) & 3) {
        case 0: {                                   /* SWAP */
            uint16_t old = rget(c, dr);
            rput(c, dr, cm1 ? (uint16_t)~src : src);
            rput(c, sr, cld ? 0 : old);
            break;
        }
        case 1: rput(c, dr, dst & (cm1 ? (uint16_t)~src : src)); break;          /* RAND */
        case 2: rput(c, dr, cm1 ? (uint16_t)(dst | (uint16_t)~src) : dst ^ src); break; /* REXO */
        default: rput(c, dr, dst | (cm1 ? (uint16_t)~src : src)); break;          /* RORA */
        }
        return;
    }
    {
        uint16_t r = rget(c, dr);
        switch ((w >> 7) & 7) {
        case 0: r = add16(c, dst, src, 0); break;                     /* RADD         */
        case 1: r = add16(c, dst, (uint16_t)~src, 0); break;          /* RADD CM1     */
        case 2: r = add16(c, dst, src, 1); break;                     /* RADD AD1     */
        case 3: r = add16(c, dst, (uint16_t)~src, 1); break;          /* RADD AD1 CM1 */
        case 4: r = add16(c, dst, src, (STS & S_C) ? 1 : 0); break;   /* RADD ADC     */
        case 5: r = add16(c, dst, (uint16_t)~src, (STS & S_C) ? 1 : 0); break;
        default: break;                                               /* no-op        */
        }
        rput(c, dr, r);
    }
}

static int getbit(Cpu *c, int r, int bn) { return (rget(c, r) >> bn) & 1; }
static void putbit(Cpu *c, int r, int bn, int v)
{
    uint16_t x = rget(c, r);
    if (v) x |= (uint16_t)(1u << bn); else x &= (uint16_t)~(1u << bn);
    if (r == R_STS) STS = x; else rput(c, r, x);
}

static void bitop(Cpu *c, uint16_t w)
{
    int bn = (w >> 3) & 017, dr = w & 7;
    int k = (STS & S_K) ? 1 : 0;
    int b = (dr == R_STS) ? ((STS >> bn) & 1) : getbit(c, dr, bn);
    switch ((w >> 7) & 017) {
    case 0:  putbit(c, dr, bn, 0); break;                   /* BSET ZRO */
    case 1:  putbit(c, dr, bn, 1); break;                   /* BSET ONE */
    case 2:  putbit(c, dr, bn, !b); break;                  /* BSET BCM */
    case 3:  putbit(c, dr, bn, k); break;                   /* BSET BAC */
    case 4:  if (!b) P++; break;                            /* BSKP ZRO */
    case 5:  if (b) P++; break;                             /* BSKP ONE */
    case 6:  if ((!b) == k) P++; break;                      /* BSKP BCM */
    case 7:  if (b == k) P++; break;                        /* BSKP BAC */
    case 8:  putbit(c, dr, bn, !k); setf(c, S_K, 1); break; /* BSTC */
    case 9:  putbit(c, dr, bn, k); setf(c, S_K, 0); break;  /* BSTA */
    case 10: setf(c, S_K, !b); break;                       /* BLDC */
    case 11: setf(c, S_K, b); break;                        /* BLDA */
    case 12: setf(c, S_K, !b && k); break;                  /* BANC */
    case 13: setf(c, S_K, b && k); break;                   /* BAND */
    case 14: setf(c, S_K, !b || k); break;                  /* BORC */
    default: setf(c, S_K, b || k); break;                   /* BORA */
    }
}

/* SHT SHD SHA SAD.  Count is 6-bit signed; the hardware counter is 5 bits. */
static void shift(Cpu *c, uint16_t w)
{
    int neg = (w >> 5) & 1;
    unsigned n = neg ? (unsigned)((-(int)((w & 077) | ~077)) & 037) : (unsigned)(w & 077);
    int type = (w >> 9) & 3;
    int which = (w >> 7) & 3;
    int width = which == 3 ? 32 : 16;
    uint32_t top = (uint32_t)1 << (width - 1);
    uint32_t mask = width == 32 ? 0xFFFFFFFFu : 0xFFFFu;
    uint32_t v;
    int m = (STS & S_M) ? 1 : 0, out = m;
    unsigned i;

    switch (which) {
    case 0: v = T; break;
    case 1: v = D; break;
    case 2: v = A; break;
    default: v = ((uint32_t)A << 16) | D; break;
    }
    for (i = 0; i < n; i++) {
        int msb = (v & top) != 0;
        if (neg) {
            out = v & 1;
            v >>= 1;
            switch (type) {
            case 0: if (msb) v |= top; break;     /* arithmetic */
            case 1: if (out) v |= top; break;     /* ROT */
            case 2: break;                        /* ZIN */
            default: if (m) v |= top; break;      /* LIN */
            }
        } else {
            out = msb;
            v = (v << 1) & mask;
            switch (type) {
            case 1: v |= (uint32_t)out; break;
            case 3: v |= (uint32_t)m; break;
            default: break;
            }
        }
    }
    setf(c, S_M, out);
    switch (which) {
    case 0: T = (uint16_t)v; break;
    case 1: D = (uint16_t)v; break;
    case 2: A = (uint16_t)v; break;
    default: A = (uint16_t)(v >> 16); D = (uint16_t)v; break;
    }
}

static void mpy(Cpu *c, uint16_t m)
{
    int32_t r = (int32_t)(int16_t)A * (int16_t)m;
    STS &= (uint16_t)~S_Q;
    if (r > 32767 || r < -32767)
        STS |= S_Q | S_O;
    A = (uint16_t)r;
}

static void rmpy(Cpu *c, uint16_t w)
{
    int16_t s = (int16_t)rget(c, (w >> 3) & 7);
    int16_t d = (int16_t)rget(c, w & 7);
    int32_t as = s < 0 ? -(int32_t)s : s, ad = d < 0 ? -(int32_t)d : d;
    uint32_t r = (uint32_t)(as * ad);
    if ((s < 0) != (d < 0)) {
        uint16_t low = (uint16_t)r;
        setf(c, S_C, low == 0);
        setf(c, S_Q, low == 0x8000);
        if (low == 0x8000) STS |= S_O;
        r = (uint32_t)(-(int32_t)r);
    }
    A = (uint16_t)(r >> 16);
    D = (uint16_t)r;
}

static void rdiv(Cpu *c, uint16_t w)
{
    int32_t dividend = (int32_t)(((uint32_t)A << 16) | D);
    int16_t divisor = (int16_t)rget(c, (w >> 3) & 7);
    int dneg = dividend < 0;
    uint32_t dmag, q, rem;
    uint16_t vmag, hi;
    if (dneg) {
        uint16_t low = D;
        setf(c, S_C, low == 0);
        setf(c, S_Q, low == 0x8000);
        if (low == 0x8000) STS |= S_O;
    }
    dmag = dneg ? 0u - (uint32_t)dividend : (uint32_t)dividend;
    vmag = (uint16_t)(divisor < 0 ? -(int32_t)divisor : divisor);
    hi = (uint16_t)(dmag >> 16);
    if (vmag == 0 || hi >= vmag) {
        A = (uint16_t)(hi - vmag);
        D = (uint16_t)dmag;
        STS |= S_Z;
        return;
    }
    q = dmag / vmag;
    rem = dmag % vmag;
    {
        int qneg = dneg ^ (divisor < 0);
        A = (uint16_t)(qneg ? 0u - q : q);
        D = (uint16_t)(dneg ? 0u - rem : rem);
        if (qneg ? q > 0x8000u : q > 0x7FFFu)
            STS |= S_Z;
    }
}

/* byte addressing: T = base word, X = byte index (even = left byte) */
static void lbyt(Cpu *c)
{
    uint16_t v = M(T + (X >> 1));
    A = (X & 1) ? (v & 0xFF) : (v >> 8);
}

static void sbyt(Cpu *c)
{
    uint16_t a = (uint16_t)(T + (X >> 1));
    if (X & 1) M(a) = (uint16_t)((M(a) & 0xFF00) | (A & 0xFF));
    else M(a) = (uint16_t)((M(a) & 0x00FF) | ((A & 0xFF) << 8));
}

static void putbyte(Cpu *c, uint16_t word, int right, uint16_t v)
{
    if (right) M(word) = (uint16_t)((M(word) & 0xFF00) | (v & 0xFF));
    else M(word) = (uint16_t)((M(word) & 0x00FF) | ((v & 0xFF) << 8));
}

static uint16_t getbyte(Cpu *c, uint16_t word, int right)
{
    return right ? (M(word) & 0xFF) : (M(word) >> 8);
}

/* BFILL: fill T&07777 bytes at X (T bit 15 = start in right byte) with A's low byte */
static void bfill(Cpu *c)
{
    unsigned len = T & 07777, right = (T >> 15) & 1, i;
    for (i = 0; i < len; i++)
        putbyte(c, (uint16_t)(X + ((i + right) >> 1)), (i + right) & 1, A);
    X = (uint16_t)(X + ((len + right) >> 1));
    T = (uint16_t)((T & 070000) | (((len + right) & 1) << 15));
    P++;
}

/* MOVB: source A/D, destination X/T; count = the smaller field length */
static void movb(Cpu *c, int forward_only)
{
    unsigned ls = D & 07777, ld = T & 07777, len = ls < ld ? ls : ld;
    unsigned sr = (D >> 15) & 1, dr = (T >> 15) & 1;
    uint16_t src = A, dst = X;
    int i;
    if (forward_only) {
        /* MOVBF refuses a destination that overlaps the unread source */
        uint16_t se = (uint16_t)(src + ((len + sr) >> 1));
        if (!(src > dst) && se > dst && len) {
            return;                         /* no skip = error */
        }
    }
    if (!forward_only && src < dst) {
        for (i = (int)len - 1; i >= 0; i--)
            putbyte(c, (uint16_t)(dst + ((i + dr) >> 1)), (i + dr) & 1,
                    getbyte(c, (uint16_t)(src + ((i + sr) >> 1)), (i + sr) & 1));
    } else {
        for (i = 0; i < (int)len; i++)
            putbyte(c, (uint16_t)(dst + ((i + dr) >> 1)), (i + dr) & 1,
                    getbyte(c, (uint16_t)(src + ((i + sr) >> 1)), (i + sr) & 1));
    }
    A = (uint16_t)(src + ((len + sr) >> 1));
    X = (uint16_t)(dst + ((len + dr) >> 1));
    if (forward_only) {
        D = (uint16_t)((D & 0x7000 & 0xEFFF) | (((len + sr) & 1) << 15) | ((ls - len) & 07777));
        T = (uint16_t)((T & 0x7000 & 0xCFFF) | (((len + dr) & 1) << 15) | ((ld - len) & 07777));
    } else {
        D = (uint16_t)((D & 0x7000) | (((len + dr) & 1) << 15));
        T = (uint16_t)((T & 0x7000) | (((len + dr) & 1) << 15) | (len & 07777));
    }
    P++;
}

/* stack instructions (CX) */
static void st_init(Cpu *c)
{
    uint16_t demand = M(P), start = M(P + 1), max = M(P + 2), flag = M(P + 3);
    if ((uint16_t)(start + 128 + demand - 122) > (uint16_t)(start + max) ||
        (flag & 1) != (STS & 1)) {
        P = (uint16_t)(P + 5);
        return;
    }
    M(start) = (uint16_t)(L + 1);
    M(start + 1) = B;
    M(start + 3) = (uint16_t)(start + max);
    B = (uint16_t)(start + 128);
    M(start + 2) = (uint16_t)(B + demand - 122);
    P = (uint16_t)(P + 6);
}

static void st_entr(Cpu *c)
{
    uint16_t demand = M(P), smax = M(B - 125), stp, oldb;
    if ((uint16_t)(B + demand - 122) > smax) {
        P = (uint16_t)(P + 1);
        return;
    }
    stp = M(B - 126);
    oldb = B;
    B = (uint16_t)(stp + 128);
    M(B - 128) = (uint16_t)(L + 1);
    M(B - 127) = oldb;
    M(B - 125) = smax;
    M(B - 126) = (uint16_t)(B + demand - 122);
    P = (uint16_t)(P + 2);
}

static void st_leave(Cpu *c)
{
    uint16_t link = M(B - 128);
    B = M(B - 127);
    P = link;
}

static void st_eleav(Cpu *c)
{
    uint16_t link = (uint16_t)(M(B - 128) - 1);
    M(B - 128) = link;
    M(B - 123) = A;
    B = M(B - 127);
    P = link;
}

static int exec(Cpu *c, uint16_t w, uint16_t pc, int depth);

int cpu_step(Cpu *c)
{
    uint16_t pc = P;
    uint16_t w = M(pc);
    int r, z0 = STS & S_Z;
    P = (uint16_t)(pc + 1);
    c->icount++;
    r = exec(c, w, pc, 0);
    c->zset = !z0 && (STS & S_Z);
    c->trap_pc = pc;
    c->trap_word = w;
    return r;
}

static int exec(Cpu *c, uint16_t w, uint16_t pc, int depth)
{
    uint16_t e, fa[3], fb[3];

    switch (w >> 11) {
    case 000: e = ea(c, w, pc); M(e) = 0; return TRAP_NONE;                   /* STZ */
    case 001: e = ea(c, w, pc); M(e) = A; return TRAP_NONE;                   /* STA */
    case 002: e = ea(c, w, pc); M(e) = T; return TRAP_NONE;                   /* STT */
    case 003: e = ea(c, w, pc); M(e) = X; return TRAP_NONE;                   /* STX */
    case 004: e = ea(c, w, pc); M(e) = A; M(e + 1) = D; return TRAP_NONE;     /* STD */
    case 005: e = ea(c, w, pc); A = M(e); D = M(e + 1); return TRAP_NONE;     /* LDD */
    case 006: e = ea(c, w, pc); M(e) = T; M(e + 1) = A; M(e + 2) = D; return TRAP_NONE; /* STF */
    case 007: e = ea(c, w, pc); T = M(e); A = M(e + 1); D = M(e + 2); return TRAP_NONE; /* LDF */
    case 010:                                                                 /* MIN */
        e = ea(c, w, pc);
        M(e) = (uint16_t)(M(e) + 1);
        if (M(e) == 0) P++;
        return TRAP_NONE;
    case 011: e = ea(c, w, pc); A = M(e); return TRAP_NONE;                   /* LDA */
    case 012: e = ea(c, w, pc); T = M(e); return TRAP_NONE;                   /* LDT */
    case 013: e = ea(c, w, pc); X = M(e); return TRAP_NONE;                   /* LDX */
    case 014: e = ea(c, w, pc); A = add16(c, A, M(e), 0); return TRAP_NONE;   /* ADD */
    case 015: e = ea(c, w, pc); A = add16(c, A, (uint16_t)~M(e), 1); return TRAP_NONE; /* SUB */
    case 016: e = ea(c, w, pc); A &= M(e); return TRAP_NONE;                  /* AND */
    case 017: e = ea(c, w, pc); A |= M(e); return TRAP_NONE;                  /* ORA */
    case 020: case 021: case 022: case 023:                                   /* FAD FSB FMU FDV */
        e = ea(c, w, pc);
        fa[0] = T; fa[1] = A; fa[2] = D;
        fb[0] = M(e); fb[1] = M(e + 1); fb[2] = M(e + 2);
        switch (w >> 11) {
        case 020: fp_add(fa, fb); break;
        case 021: fp_sub(fa, fb); break;
        case 022: fp_mul(fa, fb); break;
        default: if (fp_div(fa, fb)) STS |= S_Z; break;
        }
        T = fa[0]; A = fa[1]; D = fa[2];
        return TRAP_NONE;
    case 024: e = ea(c, w, pc); mpy(c, M(e)); return TRAP_NONE;               /* MPY */
    case 025: P = ea(c, w, pc); return TRAP_NONE;                             /* JMP */
    case 026: {                                                               /* conditional jumps */
        int t;
        switch ((w >> 8) & 7) {
        case 0: t = !(A & 0x8000); break;              /* JAP */
        case 1: t = (A & 0x8000) != 0; break;          /* JAN */
        case 2: t = A == 0; break;                     /* JAZ */
        case 3: t = A != 0; break;                     /* JAF */
        case 4: X++; t = !(X & 0x8000); break;         /* JPC */
        case 5: X++; t = (X & 0x8000) != 0; break;     /* JNC */
        case 6: t = X == 0; break;                     /* JXZ */
        default: t = (X & 0x8000) != 0; break;         /* JXN */
        }
        if (t) P = (uint16_t)(pc + (int8_t)(w & 0xFF));
        return TRAP_NONE;
    }
    case 027:                                                                 /* JPL */
        e = ea(c, w, pc);
        if (((w >> 8) & 7) != 5) L = P;
        P = e;
        return TRAP_NONE;

    case 030:                                                                 /* 140000-143777 */
        if ((w & 0300) == 0) {
            if (skip_cond(c, w)) P++;
            return TRAP_NONE;
        }
        switch (w) {
        case 0140130: bfill(c); return TRAP_NONE;
        case 0140131: movb(c, 0); return TRAP_NONE;
        case 0140132: movb(c, 1); return TRAP_NONE;
        case 0140134: st_init(c); return TRAP_NONE;
        case 0140135: st_entr(c); return TRAP_NONE;
        case 0140136: st_leave(c); return TRAP_NONE;
        case 0140137: st_eleav(c); return TRAP_NONE;
        case 0140123: A = M(T); M(T) = 0xFFFF; return TRAP_NONE;              /* TSET */
        case 0140127: A = M(T); return TRAP_NONE;                             /* RDUS */
        case 0140120: case 0140121: case 0140122: case 0140124:
        case 0140125: case 0140126: case 0142700:
            return TRAP_UNIMPL;                                               /* decimal, GECO */
        case 0140133: return TRAP_ILLEGAL;                                    /* VERSN: ND-110 only */
        default: break;
        }
        switch (w & 0177700) {
        case 0140600: {                                                       /* EXR */
            uint16_t iw = rget(c, (w >> 3) & 7);
            if ((iw & 0177700) == 0140600) return TRAP_EXR_EXR;
            return exec(c, iw, pc, depth + 1);
        }
        case 0141200: rmpy(c, w); return TRAP_NONE;
        case 0141600: rdiv(c, w); return TRAP_NONE;
        case 0142200: lbyt(c); return TRAP_NONE;
        case 0142600: sbyt(c); return TRAP_NONE;
        case 0143200: X = (uint16_t)((A - 1) * 3); return TRAP_NONE;          /* MIX3 */
        case 0143100: return TRAP_UNIMPL;                                     /* MOVEW */
        default: break;
        }
        if ((w & 0177700) == 0140200 || (w & 0177760) == 0140500)
            return TRAP_ILLEGAL;
        return TRAP_PRIV;

    case 031:                                                                 /* 144000-147777 */
        regop(c, w);
        return TRAP_NONE;

    case 032:                                                                 /* 150000-153777 */
        switch (w & 0177400) {
        case 0151400: fp_nlz(c, (int8_t)(w & 0xFF)); return TRAP_NONE;
        case 0152000: fp_dnz(c, (int8_t)(w & 0xFF)); return TRAP_NONE;
        case 0153000: c->mon = w & 0377; return TRAP_MON;
        default: return TRAP_PRIV;
        }

    case 033:                                                                 /* shifts */
        shift(c, w);
        return TRAP_NONE;

    case 034: case 035:                                                       /* IOT / IOX */
        return TRAP_PRIV;

    case 036: {                                                               /* argument instructions */
        uint16_t arg = (uint16_t)(int8_t)(w & 0xFF);
        switch ((w >> 8) & 7) {
        case 0: B = arg; break;
        case 1: A = arg; break;
        case 2: T = arg; break;
        case 3: X = arg; break;
        case 4: B = add16(c, B, arg, 0); break;
        case 5: A = add16(c, A, arg, 0); break;
        case 6: T = add16(c, T, arg, 0); break;
        default: X = add16(c, X, arg, 0); break;
        }
        return TRAP_NONE;
    }
    default:                                                                  /* bit operations */
        bitop(c, w);
        return TRAP_NONE;
    }
}
