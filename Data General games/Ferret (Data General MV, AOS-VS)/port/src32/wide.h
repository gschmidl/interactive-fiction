/* wide.h -- instruction execution for the MV.
 *
 * Included by mv32.c.  Two halves: narrow_exec for the Eclipse instructions
 * the MV kept (they work on the low 16 bits of the wide accumulators and
 * leave the high half alone), and wide_exec for the 32-bit ones.
 */

/* ---- Eclipse decode, the same fields as the 16-bit machine ------------ */
#define MR_OP(ir)     ((ir) >> 11)
#define MR_IND(ir)    ((ir) & 0x0400u)
#define MR_IDX(ir)    (((ir) >> 8) & 3u)
#define MR_DISP(ir)   ((ir) & 0x00FFu)
#define ALC_ACS(ir)   (((ir) >> 13) & 3u)
#define ALC_ACD(ir)   (((ir) >> 11) & 3u)
#define ALC_OP(ir)    (((ir) >> 8)  & 7u)
#define ALC_SH(ir)    (((ir) >> 6)  & 3u)
#define ALC_CY(ir)    (((ir) >> 4)  & 3u)
#define ALC_NL(ir)    ((ir) & 0x0008u)
#define ALC_SKIP(ir)  ((ir) & 0x0007u)

static int disp8(word ir) { int d = ir & 0xFF; return (d & 0x80) ? d - 256 : d; }

/* low / high halves of a wide accumulator */
#define LO(n)     ((word)AC[n])
#define SETLO(n,v) (AC[n] = (AC[n] & 0xFFFF0000u) | (word)(v))

static dword nea(word ir, dword at)
{
    dword a;
    switch (MR_IDX(ir)) {
    case 0:  a = MR_DISP(ir); break;
    case 1:  a = (at + disp8(ir)) & OFFMASK; break;
    case 2:  a = (AC[2] + disp8(ir)) & OFFMASK; break;
    default: a = (AC[3] + disp8(ir)) & OFFMASK; break;
    }
    if (MR_IND(ir)) {
        int hops = 0;
        for (;;) {
            word v = M[MADDR(a)];
            if (!(v & 0x8000)) { a = v; break; }
            a = v & 0x7FFF;
            if (++hops > 16) { die("indirection loop", ir); break; }
        }
    }
    return a & OFFMASK;
}

static void narrow_exec(word ir, dword at)
{
    if (ir & 0x8000) {                       /* arithmetic / logic       */
        unsigned acs = ALC_ACS(ir), acd = ALC_ACD(ir);
        uint32_t r;
        int cy = C;
        switch (ALC_CY(ir)) {
        case 1: cy = 0; break;
        case 2: cy = 1; break;
        case 3: cy = !C; break;
        }
        switch (ALC_OP(ir)) {
        case 0: r = (uint16_t)~LO(acs); break;                      /* COM */
        case 1: r = (uint16_t)(-(int)LO(acs)); if (!LO(acs)) cy = !cy; break;
        case 2: r = LO(acs); break;                                 /* MOV */
        case 3: r = (uint16_t)(LO(acs) + 1); if (LO(acs) == 0xFFFF) cy = !cy; break;
        case 4: r = (uint32_t)(uint16_t)~LO(acs) + LO(acd);         /* ADC */
                if (r > 0xFFFF) cy = !cy; break;
        case 5: r = (uint32_t)LO(acd) + (uint16_t)~LO(acs) + 1;     /* SUB */
                if (r > 0xFFFF) cy = !cy; break;
        case 6: r = (uint32_t)LO(acd) + LO(acs);                    /* ADD */
                if (r > 0xFFFF) cy = !cy; break;
        default: r = LO(acd) & LO(acs); break;                      /* AND */
        }
        r &= 0xFFFF;
        switch (ALC_SH(ir)) {                                       /* shift */
        case 1: { unsigned t = (r << 1) | (unsigned)cy; cy = (t >> 16) & 1;
                  r = t & 0xFFFF; break; }                          /* L */
        case 2: { unsigned t = r & 1; r = (r >> 1) | ((unsigned)cy << 15);
                  cy = (int)t; break; }                             /* R */
        case 3: r = ((r >> 8) | (r << 8)) & 0xFFFF; break;          /* S */
        }
        if (!ALC_NL(ir)) { SETLO(acd, r); C = cy; }
        switch (ALC_SKIP(ir)) {
        case 1: PC = (PC + 1) & OFFMASK; break;                     /* SKP */
        case 2: if (!cy) PC = (PC + 1) & OFFMASK; break;            /* SZC */
        case 3: if (cy)  PC = (PC + 1) & OFFMASK; break;            /* SNC */
        case 4: if (!r)  PC = (PC + 1) & OFFMASK; break;            /* SZR */
        case 5: if (r)   PC = (PC + 1) & OFFMASK; break;            /* SNR */
        case 6: if (!cy || !r) PC = (PC + 1) & OFFMASK; break;      /* SEZ */
        case 7: if (cy && r)   PC = (PC + 1) & OFFMASK; break;      /* SBN */
        }
        return;
    }

    {                                        /* memory reference          */
        unsigned op = MR_OP(ir);
        dword a = nea(ir, at);
        switch (op) {
        case 0: PC = a; return;                                     /* JMP */
        case 1: AC[3] = RING | PC; PC = a; return;                  /* JSR */
        case 2: M[MADDR(a)] = (word)(M[MADDR(a)] + 1);
                if (!M[MADDR(a)]) PC = (PC + 1) & OFFMASK; return;  /* ISZ */
        case 3: M[MADDR(a)] = (word)(M[MADDR(a)] - 1);
                if (!M[MADDR(a)]) PC = (PC + 1) & OFFMASK; return;  /* DSZ */
        default:
            if (op < 8)  { SETLO(op - 4, M[MADDR(a)]); return; }    /* LDA */
            if (op < 12) { M[MADDR(a)] = LO(op - 8); return; }      /* STA */
            AC[op - 12] = RING | a; return;              /* I/O space = LEF */
        }
    }
}

