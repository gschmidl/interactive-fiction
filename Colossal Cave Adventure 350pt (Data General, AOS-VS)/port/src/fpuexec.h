/* fpuexec.h -- the Eclipse FPU itself.  Included by cpu.c once M[], AC[],
 * PC, indirect() and die() exist; see fpu.h for the opcodes and for how the
 * operand encoding was established. */

static uint64_t FPAC[4];
static uint64_t FPSR;
static int fp_z, fp_n;          /* the two condition bits the FS* skips read */

#define FP_FRAC 0x00FFFFFFFFFFFFFFull

static void fp_setcc(uint64_t v)
{
    fp_z = (v & FP_FRAC) == 0;              /* zero fraction is zero      */
    fp_n = !fp_z && ((v >> 63) & 1);
}

static double dg_to_d(uint64_t v)
{
    uint64_t frac = v & FP_FRAC;
    int exp = (int)((v >> 56) & 0x7F);
    double m;
    if (!frac) return 0.0;
    m = ldexp((double)frac, -56);           /* 0.fraction                 */
    m = ldexp(m, 4 * (exp - 64));           /* times 16^(exp-64)          */
    return ((v >> 63) & 1) ? -m : m;
}

static uint64_t d_to_dg(double d)
{
    int sign = 0, exp = 64;
    uint64_t frac;
    if (!(d == d) || d == 0.0) return 0;    /* NaN and zero -> true zero  */
    if (d < 0) { sign = 1; d = -d; }
    while (d >= 1.0)      { d /= 16.0; exp++; }
    while (d < 1.0/16.0)  { d *= 16.0; exp--; }
    frac = (uint64_t)(d * 72057594037927936.0 + 0.5);   /* 2^56           */
    if (frac > FP_FRAC) { frac >>= 4; exp++; }          /* rounded up out */
    if (exp < 0)   return 0;                            /* underflow      */
    if (exp > 127) { exp = 127; frac = FP_FRAC; }       /* overflow       */
    return ((uint64_t)sign << 63) | ((uint64_t)(exp & 0x7F) << 56) | frac;
}

/* A single-precision result keeps only the top 24 bits of fraction. */
static uint64_t fp_single(uint64_t v) { return v & 0xFFFFFFFF00000000ull; }

static uint64_t fp_rd(word a, int dbl)
{
    uint64_t v = ((uint64_t)M[a & AMASK] << 48) |
                 ((uint64_t)M[(a + 1) & AMASK] << 32);
    if (dbl) v |= ((uint64_t)M[(a + 2) & AMASK] << 16) | M[(a + 3) & AMASK];
    return v;
}

static void fp_wr(word a, uint64_t v, int dbl)
{
    M[a & AMASK]       = (word)(v >> 48);
    M[(a + 1) & AMASK] = (word)(v >> 32);
    if (dbl) {
        M[(a + 2) & AMASK] = (word)(v >> 16);
        M[(a + 3) & AMASK] = (word)v;
    }
}

static int fptrace;

/* Returns 1 if the instruction was a floating-point one and was executed. */
static int fp_exec(word ir, word at)
{
    if (fptrace) {
        fprintf(stderr, "  FP %04X @%04X  f0=%g f1=%g f2=%g f3=%g  AC0=%04X z%d n%d\n",
                ir, at, dg_to_d(FPAC[0]), dg_to_d(FPAC[1]), dg_to_d(FPAC[2]),
                dg_to_d(FPAC[3]), AC[0], fp_z, fp_n);
    }
{
    unsigned f2 = (ir >> 11) & 3;           /* FPAC / ACD                 */
    unsigned f1 = (ir >> 13) & 3;           /* index mode / ACS           */
    uint64_t v;
    double x, y;
    int dbl, skip = -1;

    switch (ir) {                           /* no operand field at all    */
    case F_FNS:   return 1;
    case F_FSA:   skip = 1; break;
    case F_FSEQ:  skip = fp_z; break;
    case F_FSNE:  skip = !fp_z; break;
    case F_FSLT:  skip = fp_n; break;
    case F_FSGE:  skip = !fp_n; break;
    case F_FSLE:  skip = fp_n || fp_z; break;
    case F_FSGT:  skip = !fp_n && !fp_z; break;
    /* "skip if no <error>": nothing here ever raises one, so they all do */
    case F_FSNM: case F_FSND: case F_FSNU: case F_FSNUD:
    case F_FSNO: case F_FSNOD: case F_FSNUO: case F_FSNER:
        skip = 1; break;
    case F_FTE: case F_FTD: case F_FCLE:    /* trap enable/disable/clear  */
        FPSR = 0; return 1;
    case F_FPSH: case F_FPOP:               /* see the note in step()     */
        return 1;
    }
    if (skip >= 0) {
        if (skip) PC = (word)((PC + 1) & AMASK);
        return 1;
    }

    /* FSST / FLST save and restore the FP status register.  These carry no
     * FPAC, so bits 12-11 are the index instead and bits 14-13 pick load or
     * store -- ADVENTURE.PR at 7862 does LDA 2,@90 / FSST 8,AC2 / FCLE, and
     * then zeroes AC2+10..13, so the transfer is the two status words only
     * and not the four it would take to cover them. */
    switch (ir & F_MASK1) {
    case F_FSST: case F_FLST: {
        word w2 = M[PC & AMASK], a;
        PC = (word)((PC + 1) & AMASK);
        switch (f2) {
        case 0:  a = (word)(w2 & AMASK);
                 if (w2 & 0x8000) a = indirect(a);
                 break;
        case 1:  a = (word)((at + 1 + (int16_t)w2) & AMASK); break;
        case 2:  a = (word)((AC[2] + (int16_t)w2) & AMASK); break;
        default: a = (word)((AC[3] + (int16_t)w2) & AMASK); break;
        }
        if ((ir & F_MASK1) == F_FSST) {
            M[a & AMASK]       = (word)(FPSR >> 48);
            M[(a + 1) & AMASK] = (word)(FPSR >> 32);
        } else {
            FPSR = ((uint64_t)M[a & AMASK] << 48) |
                   ((uint64_t)M[(a + 1) & AMASK] << 32);
        }
        return 1;
    }
    }

    switch (ir & F_MASK1) {                 /* one FPAC, bits 12-11       */
    case F_FNEG: FPAC[f2] ^= 0x8000000000000000ull; fp_setcc(FPAC[f2]); return 1;
    case F_FAB:  FPAC[f2] &= 0x7FFFFFFFFFFFFFFFull; fp_setcc(FPAC[f2]); return 1;
    case F_FNOM: fp_setcc(FPAC[f2]); return 1;   /* already normalised    */
    /* FRH reads the FPAC's high-order word out to AC0 -- ADVENTURE.PR's
     * float-to-decimal routine does FRH 0 then MOV# 0,0,SNR to look at the
     * sign and exponent it just read. */
    case F_FRH:  AC[0] = (word)(FPAC[f2] >> 48); return 1;
    /* FEXP replaces the FPAC's exponent with AC0's, leaving the fraction and
     * the sign alone.  ADVENTURE.PR's rounding step is FLDD 1,[half-ulp] /
     * MOVL# 0,0,SZC / FNEG 1 / FEXP 1 / FAD 1,0 -- it loads 0.5 ulp at
     * exponent 0, gives it the sign of the value by hand, scales it to the
     * value's magnitude with FEXP and adds it.  That the sign is applied
     * separately is what says FEXP must not carry it. */
    case F_FEXP: FPAC[f2] = (FPAC[f2] & ~0x7F00000000000000ull) |
                            ((uint64_t)((AC[0] >> 8) & 0x7F) << 56);
                 return 1;
    /* FSCAL denormalises the FPAC to the exponent AC0 carries: one hex digit
     * of fraction shift per unit of exponent, then the exponent is replaced.
     * The float-to-decimal routine uses it to line a value up against the
     * power-of-ten table at 79A9. */
    case F_FSCAL: {
        int want = (AC[0] >> 8) & 0x7F;
        int have = (int)((FPAC[f2] >> 56) & 0x7F);
        uint64_t frac = FPAC[f2] & FP_FRAC;
        int d = want - have;
        if      (d >= 14 || d <= -14) frac = 0;
        else if (d > 0)  frac >>= 4 * d;
        else if (d < 0)  frac = (frac << (4 * -d)) & FP_FRAC;
        FPAC[f2] = (FPAC[f2] & 0x8000000000000000ull) |
                   ((uint64_t)want << 56) | frac;
        fp_setcc(FPAC[f2]);
        return 1;
    }
    case F_FHLV: FPAC[f2] = d_to_dg(dg_to_d(FPAC[f2]) / 2.0);
                 fp_setcc(FPAC[f2]); return 1;
    case F_FINT: { double d = dg_to_d(FPAC[f2]);
                   FPAC[f2] = d_to_dg(d < 0 ? ceil(d) : floor(d));
                   fp_setcc(FPAC[f2]); return 1; }
    }

    switch (ir & F_MASK2) {
    /* ---- register to register ------------------------------------- */
    case F_FMOV: FPAC[f2] = FPAC[f1]; fp_setcc(FPAC[f2]); return 1;
    case F_FCMP: /* flags from FPACD compared against FPACS */
        x = dg_to_d(FPAC[f2]); y = dg_to_d(FPAC[f1]);
        fp_z = (x == y); fp_n = (x < y);
        return 1;
    case F_FLAS: /* integer in the ordinary AC -> FPAC */
        FPAC[f2] = d_to_dg((double)(int16_t)AC[f1]);
        fp_setcc(FPAC[f2]); return 1;
    case F_FFAS: /* FPAC -> integer in the ordinary AC.  The AC is the
                  * bits-14-13 field and the FPAC the bits-12-11 one, the
                  * opposite way round from FLAS: the digit loop at 7ACC is
                  * FMD 1,0 / FFAS(E5A8) / FSCAL 0 / STA 3,67,2, so E5A8 --
                  * 14-13 = 3, 12-11 = 0 -- has to leave int(FPAC0) in AC3
                  * for that STA to have a digit to store. */
        AC[f1] = (word)(int16_t)dg_to_d(FPAC[f2]);
        return 1;
    case F_FAS: case F_FAD: case F_FSS: case F_FSD:
    case F_FMS: case F_FMD: case F_FDS: case F_FDD: {
        unsigned op = ir & F_MASK2;
        dbl = (op == F_FAD || op == F_FSD || op == F_FMD || op == F_FDD);
        x = dg_to_d(FPAC[f2]); y = dg_to_d(FPAC[f1]);
        if      (op == F_FAS || op == F_FAD) x += y;
        else if (op == F_FSS || op == F_FSD) x -= y;
        else if (op == F_FMS || op == F_FMD) x *= y;
        else                                 x = (y == 0.0) ? 0.0 : x / y;
        FPAC[f2] = d_to_dg(x);
        if (!dbl) FPAC[f2] = fp_single(FPAC[f2]);
        fp_setcc(FPAC[f2]); return 1;
    }
    /* ---- memory forms: index in bits 14-13, then a displacement ---- */
    case F_FLDS: case F_FLDD: case F_FSTS: case F_FSTD:
    case F_FAMS: case F_FAMD: case F_FSMS: case F_FSMD:
    case F_FMMS: case F_FMMD: case F_FDMS: case F_FDMD:
    case F_FLMD: case F_FFMD: {
        unsigned op = ir & F_MASK2;
        word w2 = M[PC & AMASK], a;
        PC = (word)((PC + 1) & AMASK);
        switch (f1) {
        case 0:  a = (word)(w2 & AMASK);
                 if (w2 & 0x8000) a = indirect(a);
                 break;
        case 1:  a = (word)((at + 1 + (int16_t)w2) & AMASK); break;
        case 2:  a = (word)((AC[2] + (int16_t)w2) & AMASK); break;
        default: a = (word)((AC[3] + (int16_t)w2) & AMASK); break;
        }
        dbl = (op == F_FLDD || op == F_FSTD || op == F_FAMD ||
               op == F_FSMD || op == F_FMMD || op == F_FDMD);
        switch (op) {
        case F_FLDS: FPAC[f2] = fp_rd(a, 0); fp_setcc(FPAC[f2]); return 1;
        case F_FLDD: FPAC[f2] = fp_rd(a, 1); fp_setcc(FPAC[f2]); return 1;
        case F_FSTS: fp_wr(a, FPAC[f2], 0); return 1;
        case F_FSTD: fp_wr(a, FPAC[f2], 1); return 1;
        /* a 32-bit integer in memory, floated / fixed */
        case F_FLMD: FPAC[f2] = d_to_dg((double)(int32_t)
                         (((uint32_t)M[a & AMASK] << 16) | M[(a + 1) & AMASK]));
                     fp_setcc(FPAC[f2]); return 1;
        case F_FFMD: { int32_t k = (int32_t)dg_to_d(FPAC[f2]);
                       M[a & AMASK] = (word)((uint32_t)k >> 16);
                       M[(a + 1) & AMASK] = (word)k;
                       return 1; }
        default:
            v = fp_rd(a, dbl);
            x = dg_to_d(FPAC[f2]); y = dg_to_d(v);
            if      (op == F_FAMS || op == F_FAMD) x += y;
            else if (op == F_FSMS || op == F_FSMD) x -= y;
            else if (op == F_FMMS || op == F_FMMD) x *= y;
            else                                   x = (y == 0.0) ? 0.0 : x / y;
            FPAC[f2] = d_to_dg(x);
            if (!dbl) FPAC[f2] = fp_single(FPAC[f2]);
            fp_setcc(FPAC[f2]); return 1;
        }
    }
    }
    return 0;
}
}
