/*
 * fpu.c - ND-100 48-bit floating point (FPP48).
 *
 * Format: word 0 = sign (bit 15) + exponent biased by 040000 (bits 14-0),
 * words 1-2 = 32-bit mantissa, normalised 0.5 <= m < 1.  Zero is all zero.
 * The floating accumulator is T,A,D.
 *
 * The arithmetic follows the integer algorithms the SIMH ND-100 simulator
 * and nd100x use (guard bit on alignment, one-step renormalisation, round
 * up on a divide remainder), which those projects checked against hardware.
 */
#include "nd100.h"

typedef struct { int s, e; uint64_t m; } Fp;

static void unpack(Fp *f, const uint16_t *w)
{
    f->s = (w[0] >> 15) & 1;
    f->e = (int)(w[0] & 0x7FFF) - 16384;
    f->m = ((uint64_t)w[1] << 16) | w[2];
}

static void pack(uint16_t *w, int s, int e, uint64_t m)
{
    w[0] = (uint16_t)(((e + 16384) & 0x7FFF) | (s << 15));
    w[1] = (uint16_t)(m >> 16);
    w[2] = (uint16_t)m;
}

static void addsame(uint16_t *r, Fp f1, Fp f2)
{
    int sc, g;
    uint64_t m;
    if (f2.e > f1.e) { Fp t = f1; f1 = f2; f2 = t; }
    sc = f1.e - f2.e;
    if (sc > 31) { pack(r, f1.s, f1.e, f1.m); return; }
    g = sc ? ((((uint64_t)1 << sc) - 1) & f2.m) != 0 : 0;
    f2.m >>= sc;
    m = (f1.m + f2.m) | (uint64_t)g;
    if (m > 0xFFFFFFFFull) { m >>= 1; f1.e++; }
    pack(r, f1.s, f1.e, m);
}

static void subdiff(uint16_t *r, Fp f1, Fp f2)
{
    int sc, g;
    uint64_t m;
    if (f2.e > f1.e) { Fp t = f1; f1 = f2; f2 = t; }
    sc = f1.e - f2.e;
    if (sc > 31) { pack(r, f1.s, f1.e, f1.m); return; }
    g = sc ? ((((uint64_t)1 << sc) - 1) & f2.m) != 0 : 0;
    f2.m >>= sc;
    f2.e = f1.e;
    if (f2.m > f1.m) { Fp t = f1; f1 = f2; f2 = t; }
    m = (f1.m - f2.m) | (uint64_t)g;
    if (m == 0) { r[0] = r[1] = r[2] = 0; return; }
    while (!(m & 0x80000000ull)) { m <<= 1; f1.e--; }
    pack(r, f1.s, f1.e, m);
}

void fp_add(uint16_t *a, const uint16_t *b)
{
    Fp f1, f2;
    unpack(&f1, a); unpack(&f2, b);
    if (f1.s ^ f2.s) subdiff(a, f1, f2); else addsame(a, f1, f2);
}

void fp_sub(uint16_t *a, const uint16_t *b)
{
    Fp f1, f2;
    unpack(&f1, a); unpack(&f2, b);
    f2.s ^= 1;
    if (f1.s ^ f2.s) subdiff(a, f1, f2); else addsame(a, f1, f2);
}

void fp_mul(uint16_t *a, const uint16_t *b)
{
    Fp f1, f2;
    uint64_t m;
    int e, s;
    unpack(&f1, a); unpack(&f2, b);
    m = f1.m * f2.m;
    e = f1.e + f2.e;
    s = f1.s ^ f2.s;
    if (!(m & (1ull << 63))) { m <<= 1; e--; }
    a[1] = (uint16_t)(m >> 48);
    a[2] = (uint16_t)(m >> 32);
    a[0] = (uint16_t)(((e + 16384) & 0x7FFF) | (s << 15));
    if (m == 0 || e < -16383) a[0] = a[1] = a[2] = 0;
}

int fp_div(uint16_t *a, const uint16_t *b)
{
    Fp dv, dd;
    uint64_t m;
    int e, s;
    unpack(&dv, b);
    unpack(&dd, a);
    if (dv.m == 0) {
        a[0] = (uint16_t)(a[0] | 0x7FFF);
        a[1] = 0xFFFF;
        a[2] = 0xFFFF;
        return 1;
    }
    dd.m <<= 32;
    s = dv.s ^ dd.s;
    e = dd.e - dv.e;
    m = dd.m / dv.m;
    if (dd.m % dv.m) m++;
    if (m >= (1ull << 32)) { m >>= 1; e++; }
    a[1] = (uint16_t)(m >> 16);
    a[2] = (uint16_t)m;
    a[0] = (uint16_t)(((e + 16384) & 0x7FFF) | (s << 15));
    if (dd.m == 0 || e < -16383) a[0] = a[1] = a[2] = 0;
    if (e + 16384 < 0 || e + 16384 > 32767) {
        /* The exponents are subtracted in the 15-bit field, so a result out
         * of range - a zero dividend always gives one, its own field being
         * 0 - borrows or carries out of that field, and the ND-100 makes
         * that an error.  The BASIC run-time clears Z, divides, and reports
         * error 315 "Overflow in division.  Result set to zero" if the
         * division set it; that is its one Z check on a floating operation.
         * SINTRAN III on the reference machine does this (0/16 errors);
         * SIMH and nd100x flag division by zero only. */
        a[0] = a[1] = a[2] = 0;
        return 1;
    }
    return 0;
}

/* NLZ: integer in A to float in T,A,D; scale +16 for integers */
void fp_nlz(Cpu *c, int scale)
{
    int32_t v;
    int sh, s = 0;
    c->r[R_D] = 0;
    if (c->r[R_A] == 0) { c->r[R_T] = 0; return; }
    v = (int16_t)c->r[R_A];
    sh = 16384 + scale;
    if (v < 0) { v = -v; s = 0x8000; }
    if (v > 32767) { v >>= 1; sh++; }
    while (!(v & 0x8000)) { v <<= 1; sh--; }
    c->r[R_T] = (uint16_t)(sh + s);
    c->r[R_A] = (uint16_t)v;
}

/* DNZ: float in T,A,D to integer in A; scale -16 for integers.  Truncates. */
void fp_dnz(Cpu *c, int scale)
{
    int32_t v = 0;
    int sh = (int)(c->r[R_T] & 0x7FFF) - 16384 + scale;
    if (sh < 0) {
        v = (-sh >= 32) ? 0 : (int32_t)(c->r[R_A] >> -sh);
    } else {
        /* sh == 0 or more: the value does not fit a signed 16-bit word */
        int64_t big = (int64_t)c->r[R_A] << (sh > 32 ? 32 : sh);
        if (big > 32767) c->r[R_STS] |= S_Z;
        v = (int32_t)big;
    }
    if (c->r[R_T] & 0x8000) v = -v;
    c->r[R_T] = 0;
    c->r[R_D] = 0;
    c->r[R_A] = (uint16_t)v;
}
