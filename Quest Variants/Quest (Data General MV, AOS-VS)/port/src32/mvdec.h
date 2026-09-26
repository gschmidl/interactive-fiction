/* mvdec.h -- the commercial decimal instructions: WLDI, WSTI and WEDIT.
 *
 * These are what the PL/I runtime's number formatter is built on, so nothing
 * prints a number until they work.  `X.DC` is the formatter; ?INTEGER_TO_CHAR
 * in SCOM.PR shows the whole idiom in eleven instructions:
 *
 *      XWLDA 0,@-12,3      ; the integer argument
 *      WFLAD 0,0           ; float it into FPAC0
 *      NLDAI 1,171         ; 0xAB -- eleven digits
 *      XLEFB 3,24,3        ; a buffer in the frame
 *      WSTI  0             ; FPAC0 -> eleven decimal digits there
 *      NLDAI 0,11
 *      LCALL X.DC,0        ; edit them into characters
 *
 * so WSTI stores the FPAC named in bits 12-11 as the decimal datum at the
 * byte pointer in AC3, described by AC1, and WLDI is its inverse.  The
 * descriptor is 0xA0 | size, size being the number of digits; every use in
 * these programs builds it that way, with WANDI 255 then WIORI 160.
 *
 * The digits are held one per byte, zoned: 0xF0 | digit, with the last byte
 * carrying the sign in its high nibble, 0xD for negative.  That is the usual
 * shape of an unpacked decimal datum and it is what makes the edit
 * sub-programs below read sensibly; it is not, however, proved from the dump,
 * because nothing outside these three instructions looks at the bytes.
 *
 * WEDIT interprets an edit sub-program, DG's little language for turning a
 * decimal datum into characters.  ECID.SR in :UTIL -- "ECLIPSE COMMERCIAL
 * INSTRUCTION DEFINITIONS" -- names the opcodes:
 *
 *      DEND 0  DNDF 1  DSTK 2  DDTK 3  DSSZ 4  DSSO 5  DSTZ 6  DMVO 7
 *      DMVN 8  DSTO 9  DINT 10 DAPT 11 DMVC 12 DMVA 13 DINS 14 DAPS 15
 *      DINC 16 DICI 17 DADI 18 DASI 19 DMVF 20 DIMC 21 DMVS 22 DAPU 23
 *
 * and X.DCA carries three of them inline, choosing between them on whether
 * the scale is zero, equal to the digit count, or between.  In FERRET.PR they
 * sit at 7ED63, 7ED6C and 7ED73:
 *
 *   mixed     10 20  14 FC 20 20 2D  01 20 2D  08 01  10 2E  08 FD  00
 *   integer   15 02 20  14 FC 20 20 2D  01 20 2D  08 01  00
 *   fraction  0E 20 2D  10 30  10 2E  08 FD  00
 *
 * Reading those three against what they have to produce settles the encoding.
 * The fraction one is ".ddd": insert the sign (' ' if positive, '-' if
 * negative), insert '0', insert '.', move the digits.  The integer one is two
 * spaces, then a zero-suppressed run, then one digit that always prints --
 * which is how you format an integer without losing a lone zero -- and the
 * mixed one is the same with '.' and a second run after it.
 *
 *      00           end
 *      01 p m       sign here if nothing significant has been moved yet
 *      08 n         move n digits
 *      0E p m       insert the sign
 *      10 c         insert the character c
 *      14 n f p m   move n digits, suppressing leading zeros with f and
 *                   floating the sign p/m in front of the first real digit
 *      15 n c       insert n copies of c
 *
 * A count of 0xFC or 0xFD is not a count but a stack reference: X.DCA pushes
 * the two it computed -- digits-before-the-point and scale -- before pushing
 * the pointers, and a negative count byte is the doubleword at that offset,
 * WSP + 2*d + 1.  -4 reaches the first and -3 the second, which is exactly
 * where they land.
 *
 * Included by wideexec.h, after the stack helpers and the FPU.
 */
#ifndef MVDEC_H
#define MVDEC_H

#define DEC_MAXD 32

/* Read a decimal datum: n zoned digits at the byte pointer bp. */
static double dec_read(dword bp, int n, int *neg)
{
    double v = 0;
    int i;
    *neg = 0;
    for (i = 0; i < n; i++) {
        unsigned b = rdbyte(bp + (dword)i);
        v = v * 10 + (b & 0x0F);
        if (i == n - 1 && (b >> 4) == 0xD) *neg = 1;
    }
    return *neg ? -v : v;
}

static void dec_write(dword bp, int n, double v)
{
    int digits[DEC_MAXD], i, neg = (v < 0);
    if (neg) v = -v;
    for (i = n - 1; i >= 0; i--) {
        double q = floor(v / 10.0);
        digits[i] = (int)(v - q * 10.0);
        v = q;
    }
    for (i = 0; i < n; i++)
        wrbyte(bp + (dword)i,
               (word)(((i == n - 1 ? (neg ? 0xD : 0xC) : 0xF) << 4) | digits[i]));
}

/* A count byte in an edit sub-program.  A negative one is not a count but a
 * doubleword on the stack at that offset: X.DCA pushes digits-before-the-
 * point and then scale before it pushes the two pointers, so -4 reaches the
 * first and -3 the second. */
static int dec_count(unsigned b)
{
    int d = (int)(int8_t)(unsigned char)b;
    if (d >= 0) return d;
    return (int)rdw((DW(WSP_A) + (dword)(2 * d) + 2) & OFFMASK);
}

/* WEDIT.  AC0 is the sub-program, AC1 the source descriptor, AC2 the
 * destination and AC3 the source, all byte pointers except the descriptor. */
static void wedit(void)
{
    dword prog = AC[0], dst = AC[2], src = AC[3];
    int size = (int)(AC[1] & 0x1F);
    int pos = 0, sig = 0, neg = 0, i;
    unsigned p = 0;

    /* the sign lives in the last digit's zone */
    if (size > 0) neg = (rdbyte(src + (dword)(size - 1)) >> 4) == 0xD;

    for (;;) {
        unsigned op = rdbyte(prog + p++);
        int n;
        unsigned c, cp, cm;
        if (op == 0x00) break;
        switch (op) {
        case 0x01:                                   /* sign if not yet significant */
            cp = rdbyte(prog + p++); cm = rdbyte(prog + p++);
            if (!sig) wrbyte(dst++, (word)(neg ? cm : cp));
            break;
        case 0x0E:                                   /* insert the sign */
            cp = rdbyte(prog + p++); cm = rdbyte(prog + p++);
            wrbyte(dst++, (word)(neg ? cm : cp));
            break;
        case 0x10:                                   /* insert a character */
            c = rdbyte(prog + p++);
            wrbyte(dst++, (word)c);
            break;
        case 0x15:                                   /* insert n of a character */
            n = dec_count(rdbyte(prog + p++));
            c = rdbyte(prog + p++);
            for (i = 0; i < n; i++) wrbyte(dst++, (word)c);
            break;
        case 0x08:                                   /* move n digits */
            n = dec_count(rdbyte(prog + p++));
            for (i = 0; i < n && pos < size; i++, pos++) {
                unsigned d = rdbyte(src + (dword)pos) & 0x0F;
                wrbyte(dst++, (word)(0x30 + d));
                sig = 1;
            }
            break;
        case 0x14: {                                 /* move n, zero-suppressed */
            unsigned fill;
            n  = dec_count(rdbyte(prog + p++));
            fill = rdbyte(prog + p++);
            cp = rdbyte(prog + p++);
            cm = rdbyte(prog + p++);
            for (i = 0; i < n && pos < size; i++, pos++) {
                unsigned d = rdbyte(src + (dword)pos) & 0x0F;
                if (!sig && d == 0) { wrbyte(dst++, (word)fill); continue; }
                if (!sig) { wrbyte(dst++, (word)(neg ? cm : cp)); sig = 1; }
                wrbyte(dst++, (word)(0x30 + d));
            }
            break; }
        default:
            /* An opcode from ECID.SR that these programs do not use.  Stop
             * rather than run on through the rest as if it were data. */
            die("unimplemented EDIT sub-program opcode", (word)op);
            return;
        }
    }
    AC[2] = dst;
    AC[3] = src + (dword)size;
}

#endif /* MVDEC_H */
