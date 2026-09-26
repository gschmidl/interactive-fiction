/* mvfpu.h -- floating point on the ECLIPSE MV.
 *
 * The MV keeps the Eclipse FPU as it stands -- same four FPACs, same format,
 * same opcodes, which is why nearly all of this is the 16-bit emulator's
 * ../src/fpuexec.h with 32-bit addresses -- and adds X and L addressed forms
 * of every memory reference.  So there are three addressing shapes here:
 *
 *   type 06,07   the Eclipse two-word form: index mode in bits 14-13, FPAC in
 *                bits 12-11, a displacement word with bit 15 indirect
 *   type 2E      X form, two words
 *   type 1A      L form, three words
 *
 * and for the last two the placement is the machine's ordinary one, index
 * mode in bits 14-13 and FPAC in bits 12-11 (mv_idx_hi says so for both).
 * The register forms take two FPACs in the ACS/ACD fields, and the single
 * operand ones an FPAC in bits 12-11.
 *
 * Format: sign in bit 63, a 7-bit excess-64 exponent to base 16 in bits
 * 62-56, then a hexadecimal-normalised fraction -- 56 bits for a double, the
 * top 24 for a single.  Value = 0.fraction * 16^(exp-64); a zero fraction is
 * zero whatever the exponent says.
 *
 * Included by wideexec.h, after xea(), rdw(), wrw() and wskip().
 */
#ifndef MVFPU_H
#define MVFPU_H

static uint64_t FPAC[4];
static uint64_t FPSR;
static int fp_z, fp_n;          /* the two bits the FS* skips read */
static int fptrace;

#define FP_FRAC 0x00FFFFFFFFFFFFFFull

static void fp_setcc(uint64_t v)
{
    fp_z = (v & FP_FRAC) == 0;
    fp_n = !fp_z && ((v >> 63) & 1);
}

static double dg_to_d(uint64_t v)
{
    uint64_t frac = v & FP_FRAC;
    int exp = (int)((v >> 56) & 0x7F);
    double m;
    if (!frac) return 0.0;
    m = ldexp((double)frac, -56);           /* 0.fraction        */
    m = ldexp(m, 4 * (exp - 64));           /* times 16^(exp-64) */
    return ((v >> 63) & 1) ? -m : m;
}

static uint64_t d_to_dg(double d)
{
    int sign = 0, exp = 64;
    uint64_t frac;
    if (!(d == d) || d == 0.0) return 0;
    if (d < 0) { sign = 1; d = -d; }
    while (d >= 1.0)     { d /= 16.0; exp++; }
    while (d < 1.0/16.0) { d *= 16.0; exp--; }
    frac = (uint64_t)(d * 72057594037927936.0 + 0.5);   /* 2^56 */
    if (frac > FP_FRAC) { frac >>= 4; exp++; }
    if (exp < 0)   return 0;                            /* underflow */
    if (exp > 127) { exp = 127; frac = FP_FRAC; }       /* overflow  */
    return ((uint64_t)sign << 63) | ((uint64_t)(exp & 0x7F) << 56) | frac;
}

static uint64_t fp_single(uint64_t v) { return v & 0xFFFFFFFF00000000ull; }

static uint64_t fp_rd(dword a, int dbl)
{
    uint64_t v = ((uint64_t)M[MADDR(a)] << 48) | ((uint64_t)M[MADDR(a + 1)] << 32);
    if (dbl) v |= ((uint64_t)M[MADDR(a + 2)] << 16) | M[MADDR(a + 3)];
    return v;
}

static void fp_wr(dword a, uint64_t v, int dbl)
{
    M[MADDR(a)]     = (word)(v >> 48);
    M[MADDR(a + 1)] = (word)(v >> 32);
    if (dbl) { M[MADDR(a + 2)] = (word)(v >> 16); M[MADDR(a + 3)] = (word)v; }
}

/* The Eclipse floating memory reference (format types 06 and 07): one
 * displacement word, index mode in bits 14-13, bit 15 of the displacement
 * indirect in the absolute mode.  Same shape ../src/fpu.h works out from
 * ADVENTURE.PR, widened to 32-bit addresses. */
static dword fpea(word ir, dword at, unsigned *used)
{
    unsigned ix = (ir >> 13) & 3;
    word w = M[MADDR(at + 1)];
    dword a;
    *used = 2;
    switch (ix) {
    case 0:  a = w & 0x7FFFu;
             if (w & 0x8000u) {
                 int hops = 0;
                 for (;;) { dword v = rdw(a); a = v & OFFMASK;
                            if (!(v & 0x80000000u)) break;
                            if (++hops > 16) break; }
             }
             break;
    case 1:  a = (dword)((int32_t)(at + 1) + (int16_t)w); break;
    case 2:  a = (dword)((int32_t)AC[2] + (int16_t)w); break;
    default: a = (dword)((int32_t)AC[3] + (int16_t)w); break;
    }
    return a & OFFMASK;
}

/* Is this opcode one the FPU handles?  Checked against the format type as
 * well, so a mis-typed table entry shows up as an unimplemented instruction
 * rather than as a wrong address. */
static int is_fp_op(unsigned op)
{
    switch (op) {
    case 0x8028: case 0x8068: case 0x80A8: case 0x80E8:   /* FAS FAD FSS FSD */
    case 0x8128: case 0x8168: case 0x81A8: case 0x81E8:   /* FMS FMD FDS FDD */
    case 0x8528: case 0x85A8: case 0x8728: case 0x8768:   /* FLAS FFAS FCMP FMOV */
    case 0x84D8:                                          /* FRDS */
    case 0x8628: case 0x8668: case 0xA628: case 0xA668:   /* FNOM FSCAL FRH FEXP */
    case 0xC628: case 0xC668: case 0xE628: case 0xE668:   /* FAB FINT FNEG FHLV */
    case 0x86A8: case 0x8EA8: case 0x96A8: case 0x9EA8:
    case 0xA6A8: case 0xAEA8: case 0xB6A8: case 0xBEA8:
    case 0xC6A8: case 0xCEA8: case 0xD6A8: case 0xDEA8:
    case 0xE6A8: case 0xEEA8: case 0xF6A8: case 0xFEA8:
    case 0xC6E8: case 0xCEE8: case 0xD6E8: case 0xE6E8: case 0xEEE8:
    case 0x86E8: case 0xA6E8:                             /* FSST FLST */
    case 0xC6D9: case 0xC6E9:                             /* LFLST LFSST */
    /* the memory references, in all three addressing shapes */
    case 0x8228: case 0x8268: case 0x82A8: case 0x82E8:
    case 0x8328: case 0x8368: case 0x83A8: case 0x83E8:
    case 0x8428: case 0x8468: case 0x84A8: case 0x84E8:
    case 0x8568: case 0x85E8:
    case 0x8009: case 0x8019: case 0x8029: case 0x8039:
    case 0x8109: case 0x8119: case 0x8129: case 0x8139:
    case 0x8209: case 0x8219: case 0x8229: case 0x8239:
    case 0x80C9: case 0x80D9: case 0x80E9: case 0x80F9:
    case 0x81C9: case 0x81D9: case 0x81E9: case 0x81F9:
    case 0x82C9: case 0x82D9: case 0x82E9: case 0x82F9:
        return 1;
    default:
        return 0;
    }
}

/* Which arithmetic, and single or double, taken from the mnemonic rather
 * than from the opcode bits.  The name says both, in every one of the four
 * spellings: F{A,S,M,D}{S,D} for the register form, F{A,S,M,D}M{S,D} for the
 * Eclipse memory one, and those with an X or L in front for the two MV forms.
 * The opcode bits do not survive the change of form -- bit 6 marks a double
 * in the Eclipse block (FAMS 8228, FAMD 8268) but is already set in the L
 * block for a single (LFAMS 80C9), where bit 4 is the one that moves. */
static void fp_kind(const char *n, int *what, int *dbl)
{
    const char *e;
    if (*n == 'X' || *n == 'L') n++;            /* past the form letter */
    *what = n[1] == 'A' ? 0 : n[1] == 'S' ? 1 : n[1] == 'M' ? 2 : 3;
    for (e = n; e[1]; e++) ;
    *dbl = (*e == 'D');
}

/* Add, subtract, multiply or divide y into the FPAC, rounding the result to
 * single precision if the instruction was a single one. */
static void fp_arith(unsigned f2, int what, int dbl, double y)
{
    double x = dg_to_d(FPAC[f2]);
    switch (what) {
    case 0: x += y; break;
    case 1: x -= y; break;
    case 2: x *= y; break;
    default: x = (y == 0.0) ? 0.0 : x / y; break;
    }
    FPAC[f2] = d_to_dg(x);
    if (!dbl) FPAC[f2] = fp_single(FPAC[f2]);
    fp_setcc(FPAC[f2]);
}

/* Executes one floating point instruction and sets PC past it.  The caller
 * has already established that is_fp_op(m->op). */
static void fp_exec32(word ir, dword at, const mvop *m)
{
    unsigned f1 = (ir >> 13) & 3, f2 = (ir >> 11) & 3;
    unsigned op = m->op, used = m->len;
    dword a = 0;
    int skip = -1, dbl, what;

    if (fptrace)
        fprintf(stderr, "  FP %-6s %04X @%07X  f0=%g f1=%g f2=%g f3=%g  z%d n%d\n",
                m->name, ir, at, dg_to_d(FPAC[0]), dg_to_d(FPAC[1]),
                dg_to_d(FPAC[2]), dg_to_d(FPAC[3]), fp_z, fp_n);

    switch (op) {
    /* ---- no operand field ------------------------------------------- */
    case 0x86A8: break;                                          /* FNS  */
    case 0x8EA8: skip = 1; break;                                /* FSA  */
    case 0x96A8: skip = fp_z; break;                             /* FSEQ */
    case 0x9EA8: skip = !fp_z; break;                            /* FSNE */
    case 0xA6A8: skip = fp_n; break;                             /* FSLT */
    case 0xAEA8: skip = !fp_n; break;                            /* FSGE */
    case 0xB6A8: skip = fp_n || fp_z; break;                     /* FSLE */
    case 0xBEA8: skip = !fp_n && !fp_z; break;                   /* FSGT */
    /* "skip if no <error>": nothing here raises one, so they all skip */
    case 0xC6A8: case 0xCEA8: case 0xD6A8: case 0xDEA8:
    case 0xE6A8: case 0xEEA8: case 0xF6A8: case 0xFEA8:
        skip = 1; break;
    case 0xC6E8: case 0xCEE8: case 0xD6E8: FPSR = 0; break;      /* FTE FTD FCLE */
    case 0xE6E8: case 0xEEE8: break;                             /* FPSH FPOP */

    /* ---- one FPAC, bits 12-11 --------------------------------------- */
    case 0xE628: FPAC[f2] ^= 0x8000000000000000ull; fp_setcc(FPAC[f2]); break;
    case 0xC628: FPAC[f2] &= 0x7FFFFFFFFFFFFFFFull; fp_setcc(FPAC[f2]); break;
    case 0x8628: fp_setcc(FPAC[f2]); break;                      /* FNOM */
    case 0xE668: FPAC[f2] = d_to_dg(dg_to_d(FPAC[f2]) / 2.0);    /* FHLV */
                 fp_setcc(FPAC[f2]); break;
    case 0xC668: { double d = dg_to_d(FPAC[f2]);                 /* FINT */
                   FPAC[f2] = d_to_dg(d < 0 ? ceil(d) : floor(d));
                   fp_setcc(FPAC[f2]); break; }
    /* FRH reads the FPAC's high-order word out to AC0; FEXP puts AC0's
     * exponent byte back, leaving the sign and fraction alone. */
    case 0xA628: SETLO(0, (word)(FPAC[f2] >> 48)); break;
    case 0xA668: FPAC[f2] = (FPAC[f2] & ~0x7F00000000000000ull) |
                            ((uint64_t)((LO(0) >> 8) & 0x7F) << 56);
                 break;
    case 0x8668: {                                               /* FSCAL */
        int want = (LO(0) >> 8) & 0x7F;
        int have = (int)((FPAC[f2] >> 56) & 0x7F);
        uint64_t frac = FPAC[f2] & FP_FRAC;
        int d = want - have;
        if      (d >= 14 || d <= -14) frac = 0;
        else if (d > 0)  frac >>= 4 * d;
        else if (d < 0)  frac = (frac << (4 * -d)) & FP_FRAC;
        FPAC[f2] = (FPAC[f2] & 0x8000000000000000ull) |
                   ((uint64_t)want << 56) | frac;
        fp_setcc(FPAC[f2]); break; }

    /* ---- two FPACs, bits 14-13 and 12-11 ---------------------------- */
    case 0x8768: FPAC[f2] = FPAC[f1]; fp_setcc(FPAC[f2]); break; /* FMOV */
    case 0x84D8: FPAC[f2] = fp_single(FPAC[f1]);                 /* FRDS */
                 fp_setcc(FPAC[f2]); break;
    case 0x8728: { double x = dg_to_d(FPAC[f2]), y = dg_to_d(FPAC[f1]);
                   fp_z = (x == y); fp_n = (x < y); break; }     /* FCMP */
    case 0x8528: FPAC[f2] = d_to_dg((double)(int16_t)LO(f1));    /* FLAS */
                 fp_setcc(FPAC[f2]); break;
    /* FFAS puts the integer in the AC named by bits 14-13 while taking the
     * FPAC from 12-11, the opposite way round from FLAS; see ../src/fpu.h. */
    case 0x85A8: SETLO(f1, (word)(int16_t)dg_to_d(FPAC[f2])); break;
    case 0x8028: case 0x8068: case 0x80A8: case 0x80E8:
    case 0x8128: case 0x8168: case 0x81A8: case 0x81E8:
        fp_kind(m->name, &what, &dbl);
        fp_arith(f2, what, dbl, dg_to_d(FPAC[f1]));
        break;

    /* ---- the FP status register ------------------------------------- */
    case 0x86E8: case 0xA6E8: case 0xC6D9: case 0xC6E9: {
        /* FSST/FLST carry no FPAC, so bits 12-11 are their index mode; the
         * L forms LFSST/LFLST are type 18, index in 12-11 as well.  The
         * transfer is the two status words only -- ADVENTURE.PR at 7862 does
         * FSST 8,AC2 then zeroes AC2+10..13 by hand. */
        int store = (op == 0x86E8 || op == 0xC6E9);
        if (op == 0x86E8 || op == 0xA6E8) {
            unsigned ix = f2;
            word w = M[MADDR(at + 1)];
            used = 2;
            switch (ix) {
            case 0:  a = w & 0x7FFFu; break;
            case 1:  a = (dword)((int32_t)(at + 1) + (int16_t)w); break;
            case 2:  a = (dword)((int32_t)AC[2] + (int16_t)w); break;
            default: a = (dword)((int32_t)AC[3] + (int16_t)w); break;
            }
            a &= OFFMASK;
        } else {
            a = xea(ir, at, 1, 0, &used);
        }
        if (store) { M[MADDR(a)] = (word)(FPSR >> 48);
                     M[MADDR(a + 1)] = (word)(FPSR >> 32); }
        else       { FPSR = ((uint64_t)M[MADDR(a)] << 48) |
                            ((uint64_t)M[MADDR(a + 1)] << 32); }
        break; }

    /* ---- the memory references, all three addressing shapes ---------- */
    default: {
        int lform = (m->type == 0x1A);
        if (m->type == 0x06) a = fpea(ir, at, &used);
        else                 a = xea(ir, at, lform, 1, &used);

        switch (op) {
        case 0x8428: case 0x8209: case 0x82C9:               /* FLDS  load S */
            FPAC[f2] = fp_rd(a, 0); fp_setcc(FPAC[f2]); break;
        case 0x8468: case 0x8219: case 0x82D9:               /* FLDD  load D */
            FPAC[f2] = fp_rd(a, 1); fp_setcc(FPAC[f2]); break;
        case 0x84A8: case 0x8229: case 0x82E9:               /* FSTS store S */
            fp_wr(a, FPAC[f2], 0); break;
        case 0x84E8: case 0x8239: case 0x82F9:               /* FSTD store D */
            fp_wr(a, FPAC[f2], 1); break;
        case 0x8568: FPAC[f2] = d_to_dg((double)(int32_t)rdw(a));  /* FLMD */
                     fp_setcc(FPAC[f2]); break;
        case 0x85E8: wrw(a, (dword)(int32_t)dg_to_d(FPAC[f2])); break;  /* FFMD */
        default:
            fp_kind(m->name, &what, &dbl);
            fp_arith(f2, what, dbl, dg_to_d(fp_rd(a, dbl)));
            break;
        }
        break; }
    }

    PC = (at + used) & OFFMASK;
    if (skip > 0) wskip();
}

#endif /* MVFPU_H */
