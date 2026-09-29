/* cis.h -- the Eclipse C-series Commercial Instruction Set, the part of it
 * the AOS FORTRAN 5 runtime uses.  Included by cpu.c after fpuexec.h.
 *
 * DG.PR's runtime has a row of one-instruction stubs at 78E7..7910 --
 * CMP, CMV, then 8158/8958/9158/9958, then 87A8..9FA8 and A7A8..BFA8, each
 * followed by POPJ -- through which its formatted I/O converts numbers
 * between FPACs and decimal strings.  They are the commercial instructions
 * of the C/300 and C/150: an S/140 has the FPU but not these, which is why
 * Dungeons only ran on a SIMH Eclipse set to C/150.
 *
 * Source: Programmer's Reference Manual, ECLIPSE Line Computers
 * (015-000024-04, 1975), "Commercial Instruction Set" 3-54..3-58 and the
 * attribute specifier on 2-26; the data types and the sign characters are
 * the ones the 32-bit Eclipse Principles of Operation (014-000704-03)
 * still tabulates on 2-16..2-19.
 *
 *   LDI  fpac   103650 (FPAC in bits 3-4)   decimal string -> FPAC
 *   STI  fpac   123650                      FPAC -> decimal string
 *   FINT fpac   100530                      FPAC = its integer part
 *   LSN         177650                      sign code of a decimal string
 *
 * The attribute specifier word, AC1: bits 8-10 the data type, bits 11-15
 * the size --
 *   0 unpacked, sign combined with the last digit     bytes = size+1
 *   1 unpacked, sign combined with the first digit    bytes = size+1
 *   2 unpacked, trailing sign byte                    bytes = size+1
 *   3 unpacked, leading sign byte                     bytes = size+1
 *   4 unpacked, unsigned                              bytes = size+1
 *   5 packed decimal, size = number of digits, a sign nibble last
 *   6 two's complement integer                        bytes = size+1
 *   7 DG floating point, sign and exponent first      bytes = size+1
 * (For 6 and 7 the 32-bit manual has bytes = size; the 16-bit one is
 * followed here, and -v reports every use so a wrong guess shows.)
 * AC3 is a byte pointer to the high-order byte; afterwards AC2 holds that
 * pointer and AC3 points past the field.  Commercial faults are not
 * raised: an invalid digit counts as 0 and -v reports it. */

#define CIS_LDI_MASK  0xE7FFu
#define CIS_LDI       0x87A8u
#define CIS_STI       0xA7A8u
#define CIS_FINT      0x8158u
#define CIS_LSN       0xFFA8u
#define CIS_LDIX      0xC7A8u
#define CIS_STIX      0xC7E8u
#define CIS_EDIT      0xF7A8u

static int cis_bytes(unsigned type, unsigned size)
{
    if (type == 5) return (int)((size | 1u) + 1u) / 2;   /* digits odd, + sign */
    return (int)size + 1;
}

/* Table 2.16: a digit and its sign in one character.  Returns the digit,
 * sets *neg; -1 for a character that is neither. */
static int cis_zoned(int c, int *neg)
{
    *neg = 0;
    if (c >= '0' && c <= '9') return c - '0';
    if (c == ' ' || c == '+' || c == '{') return 0;
    if (c >= 'A' && c <= 'I') return c - 'A' + 1;
    *neg = 1;
    if (c == '-' || c == '}') return 0;
    if (c >= 'J' && c <= 'R') return c - 'J' + 1;
    *neg = 0;
    return -1;
}

/* Table 2.17: a digit alone. */
static int cis_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c == ' ') return 0;
    return -1;
}

static void cis_bad(const char *what, word bp, int c)
{
    if (verbose)
        fprintf(stderr, "   [CIS: invalid %s %03o at byte %04X, PC=%04X]\n", what, c, bp, PC);
}

/* Read the number described by attr at byte pointer bp.  Integer types give
 * *iv and *neg; type 7 gives the DG float in *fv.  Returns the field length
 * in bytes. */
static int cis_load(word attr, word bp, unsigned long long *iv, int *neg, uint64_t *fv)
{
    unsigned type = (attr >> 5) & 7, size = attr & 31;
    int n = cis_bytes(type, size), k, d, sneg;
    unsigned long long v = 0;
    *neg = 0;
    switch (type) {
    case 0: case 1: case 2: case 3: case 4:
        for (k = 0; k < n; k++) {
            int c = bget((word)(bp + k));
            int first = k == 0, last = k == n - 1;
            if ((type == 0 && last) || (type == 1 && first)) {
                d = cis_zoned(c, &sneg);
                if (d < 0) { cis_bad("sign digit", (word)(bp + k), c); d = 0; }
                if (sneg) *neg = 1;
            } else if ((type == 2 && last) || (type == 3 && first)) {
                if (c == '-') *neg = 1;
                else if (c != '+' && c != ' ') cis_bad("sign", (word)(bp + k), c);
                continue;
            } else {
                d = cis_digit(c);
                if (d < 0) { cis_bad("digit", (word)(bp + k), c); d = 0; }
            }
            v = v * 10 + (unsigned)d;
        }
        break;
    case 5: {
        int nib = 2 * n, j;
        for (j = 0; j < nib; j++) {
            int b = bget((word)(bp + j / 2));
            int x = (j & 1) ? (b & 15) : (b >> 4);
            if (j == nib - 1) { *neg = (x == 0xD || x == 0xB); break; }
            if (x > 9) { cis_bad("packed digit", (word)(bp + j / 2), b); x = 0; }
            v = v * 10 + (unsigned)x;
        }
        break;
    }
    case 6: {
        long long s = (bget(bp) & 0x80) ? -1 : 0;
        for (k = 0; k < n; k++) s = (long long)((unsigned long long)s << 8) | bget((word)(bp + k));
        *neg = s < 0;
        v = (unsigned long long)(s < 0 ? -s : s);
        break;
    }
    case 7: {
        uint64_t f = 0;
        for (k = 0; k < n && k < 8; k++) f |= (uint64_t)bget((word)(bp + k)) << (56 - 8 * k);
        *fv = f;
        break;
    }
    }
    *iv = v;
    if (verbose)
        fprintf(stderr, "   [CIS load type %u size %u (%d bytes) at %04X: %s%llu]\n",
                type, size, n, bp, *neg ? "-" : "", v);
    return n;
}

/* Store a signed integer as attr describes; returns 1 if high-order digits
 * had to be dropped (carry). */
static int cis_store(word attr, word bp, long long val, uint64_t fval)
{
    unsigned type = (attr >> 5) & 7, size = attr & 31;
    int n = cis_bytes(type, size), k, lost = 0, neg = val < 0;
    unsigned long long v = (unsigned long long)(neg ? -val : val);
    if (verbose)
        fprintf(stderr, "   [CIS store type %u size %u (%d bytes) at %04X: %lld]\n",
                type, size, n, bp, val);
    switch (type) {
    case 0: case 1: case 2: case 3: case 4: {
        int lo = 0, hi = n - 1;                 /* the digit positions */
        if (type == 2) hi = n - 2;
        if (type == 3) lo = 1;
        for (k = hi; k >= lo; k--) {
            int d = (int)(v % 10);
            v /= 10;
            if ((type == 0 && k == n - 1) || (type == 1 && k == 0))
                bput((word)(bp + k), neg ? (d ? 'J' + d - 1 : '}') : '0' + d);
            else
                bput((word)(bp + k), '0' + d);
        }
        if (type == 2) bput((word)(bp + n - 1), neg ? '-' : '+');
        if (type == 3) bput(bp, neg ? '-' : '+');
        lost = v != 0;
        break;
    }
    case 5: {
        int nib = 2 * n, j;
        for (j = nib - 1; j >= 0; j--) {
            word p = (word)(bp + j / 2);
            int x, b = bget(p);
            if (j == nib - 1) x = neg ? 0xD : 0xC;
            else { x = (int)(v % 10); v /= 10; }
            b = (j & 1) ? ((b & 0xF0) | x) : ((b & 0x0F) | (x << 4));
            bput(p, b);
        }
        lost = v != 0;
        break;
    }
    case 6: {
        unsigned long long u = (unsigned long long)val;
        for (k = n - 1; k >= 0; k--) { bput((word)(bp + k), (int)(u & 0xFF)); u >>= 8; }
        break;
    }
    case 7:
        for (k = 0; k < n; k++)
            bput((word)(bp + k), k < 8 ? (int)((fval >> (56 - 8 * k)) & 0xFF) : 0);
        break;
    }
    return lost;
}

/* Returns 1 if ir was a commercial instruction and has been executed. */
static int cis_exec(word ir)
{
    unsigned f = (ir >> 11) & 3;
    word bp = AC[3];

    switch (ir & CIS_LDI_MASK) {
    case CIS_LDI: {
        unsigned long long iv; int neg; uint64_t fv = 0;
        int n = cis_load(AC[1], bp, &iv, &neg, &fv);
        if (((AC[1] >> 5) & 7) == 7) FPAC[f] = fv;
        else                         FPAC[f] = d_to_dg(neg ? -(double)iv : (double)iv);
        fp_setcc(FPAC[f]);
        AC[2] = bp;
        AC[3] = (word)(bp + n);
        return 1;
    }
    case CIS_STI: {
        double x = dg_to_d(FPAC[f]);
        long long val = llround(x);
        int n = cis_bytes((AC[1] >> 5) & 7, AC[1] & 31);
        C = cis_store(AC[1], bp, val, FPAC[f]);
        AC[2] = bp;
        AC[3] = (word)(bp + n);
        return 1;
    }
    case CIS_FINT: {
        double x = dg_to_d(FPAC[f]);
        FPAC[f] = d_to_dg(trunc(x));
        fp_setcc(FPAC[f]);
        return 1;
    }
    }

    switch (ir) {
    case CIS_LSN: {
        unsigned long long iv; int neg; uint64_t fv = 0;
        int n = cis_load(AC[1], bp, &iv, &neg, &fv);
        if (((AC[1] >> 5) & 7) == 7) { neg = (int)(fv >> 63); iv = (fv & FP_FRAC) != 0; }
        AC[1] = iv ? (word)(neg ? 0xFFFF : 1) : (word)(neg ? 0xFFFE : 0);
        AC[2] = bp;
        AC[3] = (word)(bp + n);
        return 1;
    }
    case CIS_LDIX: case CIS_STIX: case CIS_EDIT:
        die("commercial instruction LDIX/STIX/EDIT is not implemented", ir);
        return 1;
    }
    return 0;
}
