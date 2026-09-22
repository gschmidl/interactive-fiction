/* cpu.c - the Four-Phase IV/70 processor (the base instruction set of the
 * IV/90).  Semantics from the IV/70 Computer Reference Manual (SIV/70-11-1C,
 * 1972), checked against its worked examples; see docs/ISA_NOTES.md.  With
 * cpu_model 90 it is an IV/90 Model 2: the memory mapper, MAP, MVEL, IOXW,
 * BDEC, DBIN and BYTE, their behaviour inferred from IDOS, MFE and QUEST
 * (ISA_NOTES, "IV/90 behaviour inferred").  BIT is not emulated. */
#include <stdarg.h>
#include <string.h>
#include "fp4.h"

word mem[PHYSWORDS];
word mapper[256 * 32];
int cur_win;
int phys_pages = 128;                   /* 128K words; MFE finds the end itself */
word reg[8];
int cc_o, cc_z, cc_m, cc_c;
int cpu_model = 70;
unsigned long long icount;
word console_keys;
int inhibit;
word last_pc;
int stop_code;
char stop_text[256];

void stop(int code, const char *fmt, ...)
{
    va_list ap;

    if (stop_code)
        return;
    stop_code = code;
    va_start(ap, fmt);
    vsnprintf(stop_text, sizeof stop_text, fmt, ap);
    va_end(ap);
}

/* ---- memory: through the mapper on the IV/90 --------------------------- */

static inline unsigned long paddr(int w, word a)
{
    a &= A15;
    if (cpu_model != 90)
        return a;
    if (w < 0)
        w = cur_win;
    return (unsigned long)(mapper[(w & 0377) * 32 + (a >> 10)] & 0377) << 10 | (a & 01777);
}

/* memory that is not there reads all ones and ignores writes */
word wrd(int w, word a)
{
    unsigned long p = paddr(w, a);

    return p < (unsigned long)phys_pages << 10 ? mem[p] : W24;
}

void wwr(int w, word a, word v)
{
    unsigned long p = paddr(w, a);

    if (p < (unsigned long)phys_pages << 10)
        mem[p] = v & W24;
}

word vrd(word a)
{
    return wrd(-1, a);
}

void vwr(word a, word v)
{
    wwr(-1, a, v);
}

static inline word rd(word a)
{
    return wrd(-1, a);
}

static inline void wr(word a, word v)
{
    wwr(-1, a, v);
}

static inline word getreg(int n)
{
    return n == 0 ? 0 : n == 1 ? 1 : reg[n];
}

static inline void setreg(int n, word v)
{
    if (n == 2)
        RP = v & A15;                   /* RP as a destination: a branch */
    else if (n > 2)
        reg[n] = v & W24;
}

static inline void jump(word ea)
{
    RP = ea & A15;
}

static inline void skip(void)
{
    RP = (RP + 1) & A15;
}

static inline void zm(word v)
{
    cc_z = (v & W24) == 0;
    cc_m = (v >> 23) & 1;
}

static word add(word a, word b)
{
    word r = (a + b) & W24;

    cc_c = ((a + b) >> 24) & 1;
    if (!((a ^ b) & SIGN) && ((r ^ a) & SIGN))
        cc_o = 1;
    zm(r);
    return r;
}

static word sub(word a, word b)         /* a - b */
{
    word r = (a - b) & W24;

    cc_c = a < b;
    if (((a ^ b) & SIGN) && ((r ^ a) & SIGN))
        cc_o = 1;
    zm(r);
    return r;
}

static inline int32_t sx(word v)        /* sign-extend 24 bits */
{
    return (int32_t)(v << 8) >> 8;
}

static word rotl24(word v, int n)
{
    n %= 24;
    v &= W24;
    return n ? ((v << n) | (v >> (24 - n))) & W24 : v;
}

static word rotr24(word v, int n)
{
    return rotl24(v, (24 - n % 24) % 24);
}

static const word bmask[8] = {
    0, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
};

static inline word merge(word d, word v, int b)
{
    return ((d & ~bmask[b]) | (v & bmask[b])) & W24;
}

static word status_word(void)
{
    return (word)cc_o << 21 | (word)cc_z << 20 | (word)cc_m << 19 | (word)cc_c << 18;
}

static word ea1(word ir)
{
    word a = ir & A15;

    switch ((ir >> 15) & 7) {
    case 1: return rd(a) & A15;
    case 2: return (a + X1) & A15;
    case 3: return rd((a + X1) & A15) & A15;
    case 4: return (a + X2) & A15;
    case 5: return rd((a + X2) & A15) & A15;
    case 6: return (a + X3) & A15;
    default: return a;
    }
}

/* the arithmetic trap: the instruction at 041 is executed (a BRM) */
static void arith_trap(void)
{
    cpu_exec(rd(041), 0);
}

/* ---- shifts ---------------------------------------------------------- */

static void shift(int op, int n)
{
    uint64_t d = (uint64_t)RA << 24 | RB;       /* RA:RB, 48 bits */
    int i;

    switch (op) {
    case 050:                                   /* SLR */
        RA = rotl24(RA, n);
        break;
    case 051:                                   /* SLRD */
        n %= 48;
        if (n)
            d = ((d << n) | (d >> (48 - n))) & 0xFFFFFFFFFFFFull;
        RA = (word)(d >> 24) & W24;
        RB = (word)d & W24;
        break;
    case 052: {                                 /* SLA */
        word s = RA & SIGN, mag = RA & 037777777;

        for (i = 0; i < n; i++) {
            if (((mag >> 22) & 1) != (s >> 23))
                cc_o = 1;
            mag = (mag << 1) & 037777777;
        }
        RA = s | mag;
        break;
    }
    case 053: {                                 /* SLAD */
        uint64_t s = (uint64_t)(RA & SIGN) << 24;
        uint64_t mag = d & 0x7FFFFFFFFFFFull;   /* 47 bits */

        for (i = 0; i < n; i++) {
            if (((mag >> 46) & 1) != (s >> 47))
                cc_o = 1;
            mag = (mag << 1) & 0x7FFFFFFFFFFFull;
        }
        d = s | mag;
        RA = (word)(d >> 24) & W24;
        RB = (word)d & W24;
        break;
    }
    case 054:                                   /* SRL */
        RA = n >= 24 ? 0 : RA >> n;
        break;
    case 055:                                   /* SRLD */
        d = n >= 48 ? 0 : d >> n;
        RA = (word)(d >> 24) & W24;
        RB = (word)d & W24;
        break;
    case 056:                                   /* SRA */
        RA = (word)(sx(RA) >> (n > 23 ? 23 : n)) & W24;
        break;
    case 057: {                                 /* SRAD */
        int64_t sd = (int64_t)(d << 16) >> 16;

        sd >>= n > 47 ? 47 : n;
        RA = (word)((uint64_t)sd >> 24) & W24;
        RB = (word)sd & W24;
        break;
    }
    }
}

/* ---- multiply, divide, floating point --------------------------------- */

static void mpy(int c)
{
    __int128 p;

    if (RA == SIGN) {
        arith_trap();
        return;
    }
    p = (__int128)sx(RA) * sx(X2);
    p <<= 23 - c;
    RA = (word)(p >> 23) & W24;
    RB = (word)((p & 037777777) << 1) & W24;
    cc_z = p == 0;
    cc_m = p < 0;
    cc_c = 0;
}

static void divide(int c)
{
    int64_t d = (int64_t)sx(RA) * (1LL << 23) + (RB >> 1);
    int64_t div, q, r;
    int k = 23 - c;
    int32_t a = sx(RA), x = sx(X2);

    if ((x < 0 ? -(int64_t)x : x) <= (a < 0 ? -(int64_t)a : a)) {
        arith_trap();
        return;
    }
    div = (int64_t)x * (1LL << k);
    q = d / div;
    r = d - q * div;
    RA = (word)(((uint64_t)r & ((1ULL << k) - 1)) << (c + 1) |
                ((uint64_t)q & ((2ULL << c) - 1))) & W24;
    RB = (word)(r >> k) & W24;
    zm(RA);
    cc_c = 0;
}

/* a 48-bit fraction (scale 2^46, sign in bit 47) and exponent -> RA:RB, X1 */
static void fstore(int64_t f, int32_t e, int normalize)
{
    const int64_t top = 1LL << 46;

    if (f >= top || f < -top) {                 /* the add overflowed */
        f >>= 1;
        e++;
    }
    if (normalize && f != 0) {
        while (f < top / 2 && f >= -top / 2) {
            f <<= 1;
            e--;
        }
    }
    RA = (word)(f >> 23) & W24;
    RB = (word)((f & 037777777) << 1) & W24;
    X1 = (word)e & W24;
    zm(RA);
    cc_c = 0;
}

static void fadd(int negate, int normalize)
{
    int64_t f1 = (int64_t)sx(RA) << 23, f2 = (int64_t)sx(X2) << 23;
    int32_t e1 = sx(X1), e2 = sx(X3);
    int64_t fl, fs, d64 = (int64_t)e1 - e2;
    int32_t el, diff;

    /* The IV/70 forms the exponent difference in 24 bits and traps when it
     * overflows.  IDOS's CPU probe relies on this: UFA of exponents 0 and
     * 40000000 traps on an IV/70 (type 0). */
    if (cpu_model == 70 && (d64 > 037777777 || d64 < -040000000)) {
        arith_trap();
        return;
    }
    if (negate)
        f2 = -f2;
    if (e1 >= e2) {
        fl = f1; fs = f2; el = e1; diff = e1 - e2;
        X2 = RA;
    } else {
        fl = f2; fs = f1; el = e2; diff = e2 - e1;
        X2 = (word)(f2 >> 23) & W24;
    }
    fs = diff > 47 ? (fs < 0 ? -1 : 0) : fs >> diff;
    fstore(fl + fs, el, normalize);
}

static void fmul(void)
{
    int64_t p = (int64_t)sx(RA) * sx(X2);       /* scale 2^46 */

    fstore(p, sx(X1) + sx(X3), 1);
}

static void fdiv(void)
{
    int64_t d = (int64_t)sx(RA) * (1LL << 23) + (RB >> 1);    /* scale 2^46 */
    int64_t f2 = sx(X2), q, r;
    int32_t e = sx(X1) - sx(X3);

    if (f2 == 0) {
        arith_trap();
        return;
    }
    if ((d < 0 ? -d : d) >= (f2 < 0 ? -f2 : f2) * (1LL << 23)) {
        d /= 2;
        e++;
    }
    q = d / f2;                                 /* scale 2^23 */
    r = d - q * f2;
    fstore(q << 23, e, 1);
    RB = (word)r & W24;
}

/* ---- word moves -------------------------------------------------------- */

/* MVE: the 1972 manual says RB holds the intermediate words, but IDOS AD33
 * (the memory clear for CPU types 0 and 1) saves location 0 in RB across
 * four MVEs and puts it back afterwards; with RB overwritten the clock's INR
 * at location 0 would be wiped at every boot.  So RB is left as it was. */
static void mve(int c)
{
    int i;

    for (i = 0; i <= c; i++) {
        X2 = (X2 + 1) & W24;
        wr(X2 + X3, rd(X2));
    }
}

static void mvr(int b, int c)
{
    int i;

    for (i = 0; i <= c; i++) {
        RB = RA;
        X2 = (X2 + 1) & W24;
        RA = rotr24(rd(X2), 8);
        RB = merge(RB, RA, b);
        wr(X2 + X3, RB);
    }
}

static void mvl(int b, int c)
{
    int i;

    for (i = 0; i <= c; i++) {
        RB = RA;
        X2 = (X2 - 1) & W24;
        RA = rotl24(rd(X2), 8);
        RB = merge(RB, RA, b);
        wr(X2 + X3, RB);
    }
}

/* ---- character instructions (tables of pointer/control words) ------- */

static void charload(word ea, int left)
{
    word e = ea & ~1u & A15, t = rd(e) & A15, p = rd(e | 1);
    word ctl = rd(t | 1), v = rd(p);
    int cnt = ctl & 077, right = (ctl >> 23) & 1;

    wr(e, rd(t));                               /* the next table entry */
    v = right ? rotr24(v, cnt) : rotl24(v, cnt);
    RA = v & 0xFF0000;
    if (left) {
        if (!right && cnt == 0)
            wr(e | 1, p - 1);
    } else if (right)
        wr(e | 1, p + 1);
    cc_z = 1;
}

static void charstore(word ea, int left)
{
    word e = ea & ~1u & A15, t = rd(e) & A15, p = rd(e | 1);
    word ctl = rd(t | 1);
    int cnt = ctl & 077, lft = (ctl >> 23) & 1, b = (ctl >> 6) & 7;

    wr(e, rd(t));
    RA = lft ? rotl24(RA, cnt) : rotr24(RA, cnt);
    RA = merge(RA, rd(p), b);
    wr(p, RA);
    if (left) {
        if (cnt == 0)
            wr(e | 1, p - 1);
    } else if (lft)
        wr(e | 1, p + 1);
    cc_z = 1;
}

static void loadpar(word ea, int left)
{
    word e = ea & ~1u & A15, t = rd(e) & A15, p = rd(e | 1);
    word ctl = rd(t | 1);
    int cnt = ctl & 077, bit0 = (ctl >> 23) & 1, b = (ctl >> 6) & 7;

    RA = rd(p);
    if (left) {
        RA = merge(RA, rd(p - 1), b);
        wr(e | 1, p - 1);
        if (!bit0)
            RA = cnt == 0 ? rotl24(RA, 8) : rotr24(RA, 8);
    } else {
        RA = merge(RA, rd(p + 1), b);
        wr(e | 1, p + 1);
        RA = bit0 ? rotr24(RA, cnt) : rotl24(RA, cnt);
    }
    cc_z = 1;
}

/* ---- lists --------------------------------------------------------------- */

static void list_up(word ea)
{
    word g = rd(ea), a = g & A15, c = rd(a);

    RA = c & 0xFF0000;
    RB = (c & 0xFF0000) | a;
    if (a) {
        wr(ea, c);
        skip();
    }
}

static void list_down(word ea)
{
    word t = rd(ea);

    RA = (t & ~0xFF0000u & W24) | (RB & 0xFF0000);
    wr(RB & A15, (RB & 0xFF0000) | (t & A15));
    wr(ea, RB);
}

static void list_in(word ea)
{
    word g = rd(ea), a = g & A15;

    if (a == 0) {
        RA = RB & 0xFF0000;
        wr(ea, RB);
        wr(RB & A15, RB & 0xFF0000);
        return;
    } else {
        word c = rd(a), r = RB;

        RA = (c & ~0xFF0000u & W24) | (r & 0xFF0000);
        RB = (r & ~0xFF0000u & W24) | (c & 0xFF0000);
        wr(a, (c & 0xFF0000) | (r & A15));
        wr(ea, (c & 0xFF0000) | (r & A15));
        wr(r & A15, (r & 0xFF0000) | (c & A15));
        skip();
    }
}

static void trt(word ea)
{
    word e = ea, w = 0;
    int i;

    X3 = 0;
    for (i = 0; i < 3; i++) {
        e = (e + (RB >> 16)) & A15;
        w = rd(e);
        if (w & 1) {                            /* odd: a BRM to the routine */
            X3 = (word)i;
            wr(w & A15, status_word() | RP);
            RP = (w + 1) & A15;
            inhibit = 1;
            return;
        }
        RA = (RA + w) & W24;
        RB = rotl24((RB & ~0xFF0000u) | (w & 0xFF0000), 8);
        e = w & A15;
    }
    X3 = w;
}

/* ---- the decimal option ------------------------------------------------- */

static int getbyte(long k)
{
    long w = k / 3;
    int s = 16 - 8 * (int)(k % 3);

    return (int)(rd((word)w) >> s) & 0xFF;
}

static void setbyte(long k, int v)
{
    long w = k / 3;
    int s = 16 - 8 * (int)(k % 3);

    wr((word)w, (rd((word)w) & ~(0xFFu << s)) | ((word)(v & 0xFF) << s));
}

static long byteaddr(word x, int sb, word xreg)
{
    if (sb == 3)                                /* IV/90: the byte from X2/X3 */
        sb = (int)((xreg >> 15) & 3);
    return (long)(x & A15) * 3 + sb;
}

static void decimal(word ir)
{
    int op = ir >> 18, sbs = (ir >> 13) & 3, sbd = (ir >> 11) & 3;
    long s = byteaddr(X2, sbs, X2), d = byteaddr(X3, sbd, X3);

    switch (op) {
    case 061: {                                 /* MVCR */
        int n = ir & 0377, i;

        for (i = 0; i <= n; i++)
            setbyte(d + i, getbyte(s + i));
        cc_z = 1;
        return;
    }
    case 066: {                                 /* MVCL */
        int n = ir & 0377, i;

        for (i = 0; i <= n; i++)
            setbyte(d - i, getbyte(s - i));
        cc_z = 1;
        return;
    }
    case 063: {                                 /* CPL */
        int n = ir & 0377, i;

        cc_z = 1;
        for (i = 0; i <= n; i++) {
            int a = getbyte(s + i), b = getbyte(d + i);

            if (a != b) {
                int x = a ^ b, bit = 0x80;

                while (!(x & bit))
                    bit >>= 1;
                cc_z = 0;
                cc_c = (a & bit) != 0;
                return;
            }
        }
        return;
    }
    default: {                                  /* DADD, DSUB, CPN */
        int l2 = ir & 077, l1 = (ir >> 6) & 037;
        int ls = l2 + 1, ld = ls + l1, i;
        int neg_s, neg_d, carry = 0, neg_r, zero = 1;
        int sd[128], dd[128], r[128];
        int sub_op = op != 062;

        neg_s = ((getbyte(s) >> 4) & 7) == 5;
        neg_d = ((getbyte(d) >> 4) & 7) == 5;
        for (i = 0; i < ld; i++) {
            dd[i] = getbyte(d - i) & 017;
            sd[i] = i < ls ? getbyte(s - i) & 017 : 0;
        }
        if (sub_op)
            neg_s = !neg_s;
        if (neg_s == neg_d) {                   /* magnitudes add */
            for (i = 0; i < ld; i++) {
                int v = dd[i] + sd[i] + carry;

                carry = v >= 10;
                r[i] = v % 10;
            }
            neg_r = neg_d;
            cc_c = 0;
            if (carry) {
                cc_o = 1;
                cc_c = 1;
            }
        } else {                                /* magnitudes subtract */
            int borrow = 0;

            for (i = 0; i < ld; i++) {
                int v = dd[i] - sd[i] - borrow;

                borrow = v < 0;
                r[i] = borrow ? v + 10 : v;
            }
            neg_r = neg_d;
            cc_c = 0;
            if (borrow) {                       /* recomplement */
                int b2 = 0;

                for (i = 0; i < ld; i++) {
                    int v = -r[i] - b2;

                    b2 = v < 0;
                    r[i] = b2 ? v + 10 : v;
                }
                neg_r = !neg_d;
                cc_c = 1;
            }
        }
        for (i = 0; i < ld; i++)
            if (r[i])
                zero = 0;
        cc_z = zero;
        cc_m = neg_r && !zero;
        if (op == 060)                          /* CPN: compare only */
            return;
        for (i = 0; i < ld; i++) {
            int b = getbyte(d - i);

            b = (b & 0360) | r[i];
            if (i == 0 && neg_r != neg_d)
                b = (b & 0217) | (neg_r ? 0120 : 060);
            setbyte(d - i, b);
        }
        return;
    }
    }
}

/* ---- ODD: M = S, R = 77777777; C times: M = M rotated left 1, R ^= M;
 * then R -> the selected bytes of D.  With C = 7, bit 0 of each byte is the
 * odd parity of the byte's bits 1-7; IDOS also uses it for constants
 * (ODD R0,RA,7,0 = -1, ODD R1,RA,7,1 = -3, ODD R1,RA,7,47 = -2). */

static void oddparity(int s, int d, int b, int c)
{
    word m = getreg(s), r = W24;
    int i;

    for (i = 0; i < c; i++) {
        m = rotl24(m, 1);
        r ^= m;
    }
    setreg(d, merge(getreg(d), r, b));
}

/* ---- the processor ------------------------------------------------------- */

/* an entry with its parity bit: odd parity over the 10 bits */
static word map_entry(word e)
{
    word v = e & 0777;
    int n = 0;

    for (; v; v >>= 1)
        n += v & 1;
    return (e & 0777) | (n % 2 ? 0 : 01000);
}

void cpu_reset(void)
{
    int w, p;

    memset(reg, 0, sizeof reg);
    cc_o = cc_z = cc_m = cc_c = 0;
    inhibit = 0;
    /* the mapper's RAM holds whatever it holds at power-on; here every
     * window starts out mapping logical page p to physical page p */
    cur_win = 0;
    for (w = 0; w < 256; w++)
        for (p = 0; p < 32; p++)
            mapper[w * 32 + p] = map_entry((word)p);
}

static void type1(word ir, int intr)
{
    int op = ir >> 18;
    word e = ea1(ir), v;

    if (e == kbd_word)
        kbd_polled = icount;
    if (kbd_learn && op == 043) {               /* the key routine's STA */
        kbd_word = e;
        kbd_learn = 0;
    }
    switch (op) {
    case 000:
        stop(STOP_HALT, "HLT %05o at %05o", ir & A15, last_pc);
        break;
    case 001: RA = rd(e); X1 = rd(e | 1); break;
    case 002: X2 = rd(e); X3 = rd(e | 1); break;
    case 003: RA = rd(e); break;
    case 004: RB = rd(e); break;
    case 005: X1 = rd(e); break;
    case 006: X2 = rd(e); break;
    case 007: X3 = rd(e); break;
    case 010: v = RA | rd(e); wr(e, v); zm(v); cc_c = 0; break;
    case 011:                                   /* INR */
        v = (rd(e) + 1) & W24;
        wr(e, v);
        if (v == 0) {
            if (intr)
                irq_raise(4, 0);                /* "int on Z" */
            else
                skip();
        }
        break;
    case 012: wr(e, add(RA, rd(e))); break;
    case 013: RA = add(RA, rd(e)); break;
    case 014: RA |= rd(e); zm(RA); break;
    case 015: X1 = add(X1, rd(e)); break;
    case 016: X2 = add(X2, rd(e)); break;
    case 017: X3 = add(X3, rd(e)); break;
    case 020: v = RA & rd(e); wr(e, v); zm(v); cc_c = 0; break;
    case 021:                                   /* DEC */
        v = (rd(e) - 1) & W24;
        wr(e, v);
        if (v == 0)
            skip();
        break;
    case 022: if (rd(e) & SIGN) skip(); break;  /* SKN */
    case 023: RA = sub(RA, rd(e)); break;
    case 024: RA &= rd(e); zm(RA); break;
    case 025: X1 = sub(X1, rd(e)); break;
    case 026: X2 = sub(X2, rd(e)); break;
    case 027: X3 = sub(X3, rd(e)); break;
    case 030: v = RA ^ rd(e); wr(e, v); zm(v); cc_c = 0; break;
    case 031: wr(e, RA); wr(e | 1, X1); break;
    case 032: wr(e, X2); wr(e | 1, X3); break;
    case 033: sub(RA, rd(e)); break;
    case 034: RA ^= rd(e); zm(RA); break;
    case 035: sub(X1, rd(e)); break;
    case 036: sub(X2, rd(e)); break;
    case 037: sub(X3, rd(e)); break;
    case 040: wr(e, 0); break;
    case 041:                                   /* SAM */
        v = rd(e);
        wr(e, (v & ~A15) | (RA & A15));
        RA = (RA & A15) | (v & ~A15 & W24);
        break;
    case 042: wr(e, RP); break;
    case 043: wr(e, RA); break;
    case 044: wr(e, RB); break;
    case 045: wr(e, X1); break;
    case 046: wr(e, X2); break;
    case 047: wr(e, X3); break;
    case 050: case 051: case 052: case 053:
    case 054: case 055: case 056: case 057:
        shift(op, (int)(e & 077));
        break;
    case 060: if (rd(e) == 0) skip(); break;    /* SKZ */
    case 061: zm(rd(e)); cc_c = 0; break;       /* MCC */
    case 062: {                                 /* BOF */
        int t = cc_o;

        cc_o = 0;
        if (t)
            jump(e);
        break;
    }
    case 063: if (cc_z) jump(e); break;
    case 064: if (cc_m) jump(e); break;
    case 065: if (cc_c) jump(e); break;
    case 066: X2 = RP; jump(e); inhibit = 1; break;         /* BAL */
    case 067: if (!cc_z && !cc_c) jump(e); break;
    case 070: cpu_exec(rd(e), 0); inhibit = 1; break;       /* XEC */
    case 071:                                               /* BRM */
        wr(e, status_word() | RP);
        RP = (e + 1) & A15;
        inhibit = 1;
        break;
    case 072: jump(e); break;
    case 073: if (!cc_z) jump(e); break;
    case 074: if (!cc_m) jump(e); break;
    case 075: X1 = (X1 + 1) & W24; if (X1) jump(e); break;
    case 076: X2 = (X2 + 1) & W24; if (X2) jump(e); break;
    case 077: X3 = (X3 + 1) & W24; if (X3) jump(e); break;
    }
}

/* ---- the IV/90 mapper instructions (RS 18-19) --------------------------------
 * The register format: bits 1-8 the window, 9-13 the logical page (the map
 * address), 14 P, 15 RO, 16-23 the physical page (the map contents).
 * OPT = the instruction's bits 20-23: RMMPR 010, RMPR 04, ALTER 02, MODE 01. */
static void map_op(int s, int d, int opt)
{
    word sv = getreg(s), addr = sv & 037776000u;
    int w = (int)(sv >> 15) & 0377, idx = w * 32 + (int)(sv >> 10 & 037);

    if (opt & 014) {                            /* RMAPP, RMEMP: no parity errors */
        setreg(d, 0);
        return;
    }
    if (opt & 01) {                             /* the mapper memory */
        word old = mapper[idx];

        if (opt & 02) {                         /* WMAP, SWMAP: the parity is made here */
            mapper[idx] = map_entry(sv);
            if (d)
                setreg(d, addr | old);
        } else                                  /* RMAP */
            setreg(d, addr | old);
    } else {                                    /* the window register */
        int old = cur_win;

        if (opt & 02) {
            cur_win = w;
            if (d == 2)                         /* BRAWIN: to the address in the new window */
                RP = sv & A15;
            else if (d)                         /* SWWIN */
                setreg(d, (word)old << 15);
        } else                                  /* RWIN */
            setreg(d, (word)old << 15);
    }
}

/* ---- byte pointers (RS 21): bits 7-8 the byte (0-2), bits 9-23 the word ---- */

static word bp_get(word p)
{
    int b = (int)(p >> 15) & 3;

    return rd(p & A15) >> (16 - 8 * (b > 2 ? 2 : b)) & 0377;
}

static void bp_put(word p, word v)
{
    int b = (int)(p >> 15) & 3, sh = 16 - 8 * (b > 2 ? 2 : b);
    word a = p & A15;

    wr(a, (rd(a) & ~(0377u << sh)) | (v & 0377) << sh);
}

static word bp_next(word p)
{
    int b = (int)(p >> 15) & 3;

    if (b >= 2)
        return (p & ~(3u << 15) & ~A15) | ((p + 1) & A15);
    return (p & ~(3u << 15)) | (word)(b + 1) << 15;
}

static word bp_prev(word p)
{
    int b = (int)(p >> 15) & 3;

    if (b == 0)
        return (p & ~A15) | (2u << 15) | ((p - 1) & A15);
    return (p & ~(3u << 15)) | (word)(b - 1) << 15;
}

/* BDEC (IV/90, RS 15): RA as C+1 decimal characters ending at the byte
 * pointer X3 (the units digit), written right to left as the decimal
 * option's pointers go; X3 ends before them.  Digits take the zone DZ, and a
 * negative number's units digit the zone MZ.  Leading zeros take BZ as their
 * zone (2: blanks, 3: zeros); BZ = 0 keeps them.  (Found from MFE's use:
 * "SCREEN  0" with BZ = 2, and its clock "hh:mm:ss" built from two-digit
 * fields with X3 at the second byte of a word, BZ = 3.) */
static void bdec(word ir)
{
    int bz = (int)(ir >> 11) & 017, dz = (int)(ir >> 7) & 017, mz = (int)(ir >> 3) & 017;
    int n = (int)(ir & 07) + 1, i, lead = 1;
    int32_t v = sx(RA);
    unsigned long m = v < 0 ? (unsigned long)(-(long)v) : (unsigned long)v;
    int dig[8], zon[8];

    for (i = n - 1; i >= 0; i--) {
        dig[i] = (int)(m % 10);
        m /= 10;
    }
    for (i = 0; i < n; i++) {                   /* the zones, left to right */
        zon[i] = dz;
        if (bz && lead && dig[i] == 0 && i < n - 1)
            zon[i] = bz;
        else
            lead = 0;
    }
    if (v < 0)
        zon[n - 1] = mz;
    for (i = n - 1; i >= 0; i--) {              /* stored right to left */
        bp_put(X3, (word)(zon[i] << 4 | dig[i]));
        X3 = bp_prev(X3);
    }
}

/* DBIN (IV/90, RS 15): C+1 decimal characters from the byte pointer X3, left
 * to right (X3 ends past them) -> RA.  A character that is not a digit in
 * the zone DZ sets C, and with HC ends the conversion there; with ZD a units
 * digit in the zone MZ makes the number negative.  Z and M from RA.  (Found
 * from MFE's console: the hour typed as "12" at a word's byte 0, then BCR to
 * the error path.) */
static void dbin(word ir)
{
    int hc = (int)(ir >> 12) & 1, zd = (int)(ir >> 11) & 1;
    int dz = (int)(ir >> 7) & 017, mz = (int)(ir >> 3) & 017, n = (int)(ir & 07) + 1, i;
    long v = 0;
    int neg = 0, bad = 0;

    for (i = 0; i < n; i++) {
        word ch = bp_get(X3);
        int zone = (int)(ch >> 4) & 017, dig = (int)ch & 017;

        if (dig > 9 || (zone != dz && !(zd && i == n - 1 && zone == mz))) {
            bad = 1;
            if (hc)
                break;
            dig = 0;
        } else if (zd && i == n - 1 && zone == mz && mz != dz)
            neg = 1;
        v = v * 10 + dig;
        X3 = bp_next(X3);
    }
    RA = (word)(neg ? -v : v) & W24;
    zm(RA);
    cc_c = bad;
}

/* BYTE (IV/90, RS 16-17).  Fields: SREG 9-11 (the pointer), DREG 12-14
 * (receives the stepped or converted pointer; 0 = none), OREG 15-17 (a byte
 * offset register), SF 18 / DF 19 (the source / destination format: 0 = byte
 * pointer, 1 = byte address), D 20, I 21 (step the pointer by one byte), ST
 * 22 (RA's left byte to the byte), LD 23 (the byte to RA's left byte, the
 * rest cleared); ST+LD swaps.  The step comes after the access.  (Not
 * documented; found from QUEST, which loads a byte, rotates RA left 8 to work
 * on it, rotates it back right 8 and stores it.) */
static unsigned long byte_num(word p, int addr_fmt)      /* the byte's number */
{
    if (addr_fmt)
        return (unsigned long)(p & 0777777) % 0300000;
    return (unsigned long)(p & A15) * 3 + ((p >> 15) & 3);
}

static word byte_fmt(unsigned long n, int addr_fmt)
{
    n %= 0300000;
    if (addr_fmt)
        return (word)n;
    return (word)((n % 3) << 15 | (n / 3));
}

static void byteop(word ir)
{
    int s = (int)(ir >> 12) & 7, d = (int)(ir >> 9) & 7, o = (int)(ir >> 6) & 7;
    int sf = (int)(ir >> 5) & 1, df = (int)(ir >> 4) & 1, dec = (int)(ir >> 3) & 1;
    int inc = (int)(ir >> 2) & 1, st = (int)(ir >> 1) & 1, ld = (int)ir & 1;
    unsigned long n = byte_num(getreg(s), sf);

    if (st || ld) {
        unsigned long e = (n + (o ? (unsigned long)sx(getreg(o)) : 0)) % 0300000;
        word a = (word)(e / 3), old;
        int sh = 16 - 8 * (int)(e % 3);

        old = rd(a) >> sh & 0377;
        if (st)
            wr(a, (rd(a) & ~(0377u << sh)) | (RA >> 16 & 0377) << sh);
        if (ld) {
            RA = old << 16;
            zm(RA);
        }
    }
    if (inc)
        n = (n + 1) % 0300000;
    if (dec)
        n = (n + 0300000 - 1) % 0300000;
    if (d && (inc || dec || sf != df))
        setreg(d, byte_fmt(n, df));
}

/* MVEL: RA words from X2 to X3, each a window (bits 1-8) and an address */
void (*screen_hook)(int page, int scroll);

static void mvel(void)
{
    word n = RA, i;
    int sw = (int)(X2 >> 15) & 0377, dw = (int)(X3 >> 15) & 0377;

    /* A terminal's screen is at 0140 in its own physical page.  QUEST
     * scrolls its display (lines 0-22) up a line with one MVEL from line 1
     * to line 0 and clears it with another; MFE puts its own screen back
     * with MVELs when a player leaves. */
    if (screen_hook && n > 0) {
        unsigned long ps = paddr(sw, X2), pd = paddr(dw, X3), off = pd & 01777;

        if (pd >> 10 < 040 && off + n > 0140 && off < 0140 + 23 * 040)
            screen_hook((int)(pd >> 10), ps == pd + 040 && off == 0140);
    }
    for (i = 0; i < n && i < MEMWORDS; i++)
        wwr(dw, X3 + i, wrd(sw, X2 + i));
    X2 = (X2 & ~A15) | ((X2 + n) & A15);
    X3 = (X3 & ~A15) | ((X3 + n) & A15);
    RA = 0;
}

static void type2(word ir, int intr)
{
    int op = ir >> 18;
    int s = (ir >> 12) & 7, d = (ir >> 9) & 7, b = (ir >> 6) & 7, c = ir & 077;
    word ea = ir & A15, v, r;

    (void)intr;
    switch (op) {
    case 000: charload(ea, 1); break;           /* LCL */
    case 001: loadpar(ea, 1); break;            /* LPL */
    case 002:                                   /* RCL */
        setreg(d, rotl24(merge(getreg(d), getreg(s), b), c));
        break;
    case 003:                                   /* RLC */
        setreg(d, merge(getreg(d), rotl24(getreg(s), c), b));
        break;
    case 004: charload(ea, 0); break;           /* LCR */
    case 005: loadpar(ea, 0); break;            /* LPR */
    case 006:                                   /* RCR, RCPY, NOP */
        setreg(d, rotr24(merge(getreg(d), getreg(s), b), c));
        break;
    case 007:                                   /* RRC */
        setreg(d, merge(getreg(d), rotr24(getreg(s), c), b));
        break;
    case 010:                                   /* ROR */
        v = merge(getreg(d), getreg(s) | getreg(d), b);
        zm(v);
        setreg(d, v);
        break;
    case 011:                                   /* RADD, RCC */
        r = add(getreg(d), getreg(s));
        setreg(d, merge(getreg(d), r, b));
        break;
    case 012: mpy(c); break;
    case 013: mvl(b, c); break;
    case 014: fadd(0, 0); break;                /* UFA */
    case 015: fadd(0, 1); break;                /* FAD */
    case 016: fmul(); break;                    /* FMP */
    case 017: mvr(b, c); break;
    case 020:                                   /* RAND */
        v = merge(getreg(d), getreg(s) & getreg(d), b);
        zm(v);
        setreg(d, v);
        break;
    case 021:                                   /* RSUB */
        r = sub(getreg(d), getreg(s));
        setreg(d, merge(getreg(d), r, b));
        break;
    case 022: divide(c); break;
    case 023: mve(c); break;
    case 024: X2 = RA; X3 = X1; break;          /* CDA2 */
    case 025: fadd(1, 1); break;                /* FSB */
    case 026: fdiv(); break;                    /* FDV */
    case 027: io_exec(ea, 1); break;            /* IOB */
    case 030:                                   /* RXOR */
        v = merge(getreg(d), getreg(s) ^ getreg(d), b);
        zm(v);
        setreg(d, v);
        break;
    case 031:                                   /* RCM2 */
        v = getreg(s);
        r = (~v + 1) & W24;
        cc_c = v == 0;
        if (v == SIGN)
            cc_o = 1;
        zm(r);
        setreg(d, r);
        break;
    case 032: {                                 /* POP */
        word p = rd(ea);

        RA = rd(p);
        if (p != rd(ea - 1)) {
            p = (p - 1) & W24;
            wr(ea, p);
            RB = p;
            skip();
        }
        break;
    }
    case 033: list_up(ea); break;               /* UP */
    case 034: list_in(ea); break;               /* IN */
    case 035: trt(ea); break;                   /* TRT */
    case 037:                                   /* BOOT */
        io_boot(getreg(d));
        cc_z = 1;
        break;
    case 040: charstore(ea, 1); break;          /* SCL */
    case 041: {                                 /* SPL */
        word p = rd(ea);

        wr(p, RA);
        p = (p - 1) & W24;
        wr(ea, p);
        RB = p;
        break;
    }
    case 042: {                                 /* PUSH */
        word p = rd(ea);

        if (((p + 1) & W24) != rd(ea + 1)) {
            p = (p + 1) & W24;
            wr(ea, p);
            RB = p;
            wr(p, RA);
            skip();
        }
        break;
    }
    case 043: list_down(ea); break;             /* DOWN */
    case 044: charstore(ea, 0); break;          /* SCR */
    case 045: {                                 /* SPR */
        word p = rd(ea);

        wr(p, RA);
        p = (p + 1) & W24;
        wr(ea, p);
        RB = p;
        break;
    }
    case 046:                                   /* TRAP */
        cpu_exec(rd(041), 0);
        inhibit = 1;
        break;
    case 047:                                   /* ECS */
        setreg(d, merge(getreg(d), console_keys, b));
        break;
    case 050:                                   /* BRD */
    case 051:                                   /* BRR */
        v = rd(ea);
        cc_o = (v >> 21) & 1;
        cc_z = (v >> 20) & 1;
        cc_m = (v >> 19) & 1;
        cc_c = (v >> 18) & 1;
        jump(v);
        if (op == 050)
            irq_debreak();
        break;
    case 052: io_exct(rd(ea)); break;           /* EXCT */
    case 053: if (io_exsn(rd(ea))) skip(); break;   /* EXSN */
    case 054: irq_arm(rd(ea), 1); inhibit = 1; break;   /* PIA */
    case 055: irq_arm(rd(ea), 0); break;        /* PID */
    case 056: irq_reset_levels(rd(ea)); inhibit = 1; break; /* PIR */
    case 057:                                   /* IOID outside an interrupt */
        stop(STOP_ILLEGAL, "IOID %05o executed outside an interrupt at %05o", ea, last_pc);
        break;
    case 060: case 061: case 062: case 063: case 064: case 066:
        decimal(ir);
        break;
    case 065: oddparity(s, d, b, c); break;     /* ODD */
    case 067: io_exec(ea, 0); break;            /* IO */
    case 070:                                   /* BDEC (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        bdec(ir);
        break;
    case 071:                                   /* DBIN (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        dbin(ir);
        break;
    case 072:                                   /* BYTE (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        byteop(ir);
        break;
    case 073:                                   /* MAP (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        map_op(s, d, (int)ir & 017);
        break;
    case 074:                                   /* IOXW (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        io_cross = 1;
        io_exec(ea, 0);
        io_cross = 0;
        break;
    case 075:                                   /* MVEL (IV/90) */
        if (cpu_model != 90)
            goto illegal;
        mvel();
        break;
    illegal:
    default:
        stop(STOP_ILLEGAL, "instruction %08o (op %03o) at %05o is not emulated",
             ir, (op << 3) | 7, last_pc);
        break;
    }
}

void cpu_exec(word ir, int intr)
{
    if (((ir >> 15) & 7) == 7)
        type2(ir, intr);
    else
        type1(ir, intr);
}

/* Idle detection: every 64 instructions, count the distinct addresses
 * executed; a program that spends 256 instructions in a loop of a dozen
 * addresses is waiting (for a key, as a rule). */
static word pc_hist[64];
static int idle_runs;

int cpu_idle(void)
{
    return idle_runs >= 8;
}

static void idle_track(word pc)
{
    unsigned k = (unsigned)(icount & 63);

    pc_hist[k] = pc;
    if (k == 63) {
        int i, j, distinct = 0;

        for (i = 0; i < 64 && distinct <= 12; i++) {
            for (j = 0; j < i && pc_hist[j] != pc_hist[i]; j++)
                ;
            if (j == i)
                distinct++;
        }
        idle_runs = distinct <= 12 ? idle_runs + 1 : 0;
    }
}

/* a loop the front end knows to be waiting for work: the instructions run
 * there are counted */
int idle_win = -1;
word idle_lo, idle_hi;
unsigned long idle_hits;

void cpu_step(void)
{
    word ir, pc;
    int lvl;

    icount++;
    idle_track(RP);
    io_tick();
    if (!inhibit && (lvl = irq_pending()) >= 0) {
        word iw;

        irq_take(lvl, &iw);
        if (trace_fp && icount >= trace_from)
            fprintf(trace_fp, "** interrupt level %d: %08o\n", lvl, iw);
        /* a BRM leaves the level active until the routine's BRD */
        if ((iw >> 18) == 071 && ((iw >> 15) & 7) != 7)
            irq_mark_active(lvl);
        inhibit = 0;
        last_pc = RP;
        cpu_exec(iw, 1);
        return;
    }
    inhibit = 0;
    pc = RP;
    last_pc = pc;
    if (cur_win == idle_win && pc - idle_lo <= idle_hi - idle_lo)
        idle_hits++;
    ir = rd(pc);
    RP = (pc + 1) & A15;
    trace_insn(pc, ir);
    cpu_exec(ir, 0);
}
