/* PDP-10 user-mode interpreter: enough of a KA10/KI10 to run a SAIL program */
#include "fisk.h"
#include <math.h>

word mem[MEMSIZ];
int  pc;
int  flags;
int  trace = 0;
long long icount = 0;

static int halted;
static int skipped;      /* set by execute() when the instruction skipped */
static int jumped;       /* set by execute() when the instruction jumped  */

/* ---------------------------------------------------------------- helpers */

static word rd(int a)         { return mem[a & (MEMSIZ - 1)]; }
static void wr(int a, word v) { mem[a & (MEMSIZ - 1)] = v & WORDM; }

static sword sx(word w)       /* sign-extend 36 -> 64 */
{
    return (w & SIGN) ? (sword)(w | ~WORDM) : (sword)w;
}
static word neg(word w) { return (~w + 1) & WORDM; }

static int ea(word inst)
{
    int y = (int)(inst & RMASK), x = (int)((inst >> 18) & 017), i = (int)((inst >> 22) & 1);
    int guard = 0;
    if (x) y = (y + (int)(rd(x) & RMASK)) & (int)RMASK;
    while (i) {
        word w = rd(y);
        y = (int)(w & RMASK); x = (int)((w >> 18) & 017); i = (int)((w >> 22) & 1);
        if (x) y = (y + (int)(rd(x) & RMASK)) & (int)RMASK;
        if (++guard > 200) break;
    }
    return y;
}

/* ------------------------------------------------------------ arithmetic */

static word add36(word a, word b)
{
    word r = (a + b) & WORDM;
    if ((a ^ r) & (b ^ r) & SIGN) flags |= F_OV;
    return r;
}
static word sub36(word a, word b) { return add36(a, neg(b)); }

/* --------------------------------------------------------- floating point */

static double w2d(word w)
{
    int ng = 0, exp; word frac;
    if (w & SIGN) { w = neg(w); ng = 1; }
    if (w == 0) return 0.0;
    exp  = (int)((w >> 27) & 0377);
    frac = w & 0777777777ULL;
    return (ng ? -1.0 : 1.0) * ldexp((double)frac, exp - 128 - 27);
}

static word d2w(double v)
{
    int ng = 0, e2; double f; word frac, w;
    if (v == 0.0 || !(v == v)) return 0;
    if (v < 0) { v = -v; ng = 1; }
    f = frexp(v, &e2);                       /* v = f * 2^e2, .5 <= f < 1  */
    frac = (word)(ldexp(f, 27) + 0.5);
    if (frac >= 01000000000ULL) { frac >>= 1; e2++; }
    if (e2 + 128 > 255) { flags |= F_FOV; e2 = 127; frac = 0777777777ULL; }
    if (e2 + 128 < 0) return 0;
    w = (((word)(e2 + 128) & 0377) << 27) | (frac & 0777777777ULL);
    return ng ? neg(w) : w;
}

/* ------------------------------------------------------------ byte fields */

static int bp_addr(word p)
{
    /* the address part of a byte pointer, indexed and indirected as usual */
    return ea(p & 000777777777ULL);
}

static word ldb_ptr(word p)
{
    int pos = (int)((p >> 30) & 077), siz = (int)((p >> 24) & 077);
    word m;
    if (siz == 0 || siz > 36) return 0;
    m = (siz >= 36) ? WORDM : ((1ULL << siz) - 1);
    return (rd(bp_addr(p)) >> pos) & m;
}

static void dpb_ptr(word p, word v)
{
    int pos = (int)((p >> 30) & 077), siz = (int)((p >> 24) & 077);
    int a; word m;
    if (siz == 0 || siz > 36) return;
    a = bp_addr(p);
    m = ((siz >= 36) ? WORDM : ((1ULL << siz) - 1)) << pos;
    wr(a, (rd(a) & ~m) | ((v << pos) & m));
}

static word ibp(word p)
{
    int pos = (int)((p >> 30) & 077), siz = (int)((p >> 24) & 077);
    pos -= siz;
    if (pos < 0) { pos = 36 - siz; p = (p & ~RMASK) | (((p & RMASK) + 1) & RMASK); }
    return (p & ~(077ULL << 30)) | (((word)pos & 077) << 30);
}

static word adjbp(sword adj, word p)
{
    int pos = (int)((p >> 30) & 077), siz = (int)((p >> 24) & 077);
    int n, idx; sword total, wadj;
    if (siz == 0 || siz > 36) return p;
    n = 36 / siz;
    if (n == 0) return p;
    idx   = (36 - pos) / siz - 1;
    total = (sword)idx + adj;
    wadj  = (total >= 0) ? (total / n) : -(((-total) + n - 1) / n);
    idx   = (int)(total - wadj * n);
    pos   = 36 - siz * (idx + 1);
    p = (p & ~(077ULL << 30)) | (((word)pos & 077) << 30);
    p = (p & ~RMASK) | (((p & RMASK) + (word)wadj) & RMASK);
    return p;
}

/* ------------------------------------------------------------- conditions */

static int cond(int c, sword v)
{
    switch (c & 7) {
    case 0: return 0;
    case 1: return v <  0;
    case 2: return v == 0;
    case 3: return v <= 0;
    case 4: return 1;
    case 5: return v >= 0;
    case 6: return v != 0;
    default: return v > 0;
    }
}

/* ------------------------------------------------------------------- core */

void cpu_reset(void)
{
    int i;
    memset(mem, 0, sizeof mem);
    for (i = 0; i < image->low_len;  i++) mem[image->low_base  + i] = image->low[i];
    for (i = 0; i < image->high_len; i++) mem[image->high_base + i] = image->high[i];
    flags  = F_USER;
    halted = 0;
    pc = (int)(mem[0120] & RMASK);          /* .JBSA start address */
}

#define AC     mem[ac]
#define ACn(k) mem[(ac + (k)) & 017]

/* Execute one instruction.  Returns 0 normally, 2 to leave the interpreter. */
static int execute(word inst, int frompc)
{
    int op = (int)((inst >> 27) & 0777);
    int ac = (int)((inst >> 23) & 017);
    int e  = ea(inst);
    int i;

    skipped = 0;
    jumped  = 0;

    switch (op) {

    /* ----------------------------------------------------------- local UUO */
    case 000: case 001: case 002: case 003: case 004: case 005: case 006:
    case 007: case 010: case 011: case 012: case 013: case 014: case 015:
    case 016: case 017: case 020: case 021: case 022: case 023: case 024:
    case 025: case 026: case 027: case 030: case 031: case 032: case 033:
    case 034: case 035: case 036: case 037:
        wr(040, (inst & 0777740000000ULL) | (word)(e & (int)RMASK));
        pc = 041;                              /* the monitor JSRs from here */
        jumped = 1;
        break;

    /* --------------------------------------------------------- monitor UUO */
    case 040: case 041: case 042: case 043: case 044: case 045: case 046:
    case 047: case 050: case 051: case 052: case 053: case 054: case 055:
    case 056: case 057: case 060: case 061: case 062: case 063: case 064:
    case 065: case 066: case 067: case 070: case 071: case 072: case 073:
    case 074: case 075: case 076: case 077: {
        int r = uuo(op, ac, e);
        if (r == 1) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        else if (r == 2) return 2;
        break;
    }

    /* ------------------------------------------------------------ dbl move */
    case 0120: AC = rd(e); ACn(1) = rd(e + 1); break;
    case 0121: {
        word hi = rd(e), lo = rd(e + 1);
        word nlo = neg(lo) & 0377777777777ULL;
        word nhi = ~hi & WORDM;
        if (lo == 0) { nhi = neg(hi); nlo = 0; }
        AC = nhi; ACn(1) = nlo;
        break;
    }
    case 0124: wr(e, AC); wr(e + 1, ACn(1)); break;

    /* --------------------------------------------------------- fix / float */
    case 0122: case 0126: {
        double v = w2d(rd(e));
        sword n = (op == 0126) ? (sword)floor(v + 0.5) : (sword)v;
        AC = (word)n & WORDM;
        break;
    }
    case 0127: AC = d2w((double)sx(rd(e))); break;

    /* -------------------------------------------------------------- shifts */
    case 0132: {                                                     /* FSC */
        int sc = e; word w = AC;
        if (sc & 0400000) sc -= 01000000;
        if (w) {
            int ng = (w & SIGN) != 0; word a = ng ? neg(w) : w;
            int ex = (int)((a >> 27) & 0377) + sc;
            if (ex < 0) a = 0;
            else a = (((word)ex & 0377) << 27) | (a & 0777777777ULL);
            AC = ng ? neg(a) : a;
        }
        break;
    }

    /* ------------------------------------------------------------ byte ops */
    case 0133:
        if (ac == 0) wr(e, ibp(rd(e)));
        else AC = adjbp(sx(AC), rd(e));
        break;
    case 0134: { word p = ibp(rd(e)); wr(e, p); AC = ldb_ptr(p); break; }
    case 0135: AC = ldb_ptr(rd(e)); break;
    case 0136: { word p = ibp(rd(e)); wr(e, p); dpb_ptr(p, AC); break; }
    case 0137: dpb_ptr(rd(e), AC); break;

    /* --------------------------------------------------------------- moves */
    case 0200: case 0201: case 0202: case 0203:
    case 0204: case 0205: case 0206: case 0207:
    case 0210: case 0211: case 0212: case 0213:
    case 0214: case 0215: case 0216: case 0217: {
        int fn = (op >> 2) & 3, md = op & 3;
        word src = (md == 1) ? (word)e : (md == 2) ? AC : rd(e);
        word r;
        switch (fn) {
        case 0: r = src; break;
        case 1: r = ((src << 18) | (src >> 18)) & WORDM; break;
        case 2: r = neg(src); if (src == SIGN) flags |= F_OV; break;
        default: r = (src & SIGN) ? neg(src) : src; break;
        }
        if (md <= 1) AC = r;
        else if (md == 2) wr(e, r);
        else { wr(e, r); if (ac) AC = r; }
        break;
    }

    /* ----------------------------------------------------- multiply/divide */
    case 0220: case 0221: case 0222: case 0223: {                   /* IMUL */
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        word r = (word)(sx(AC) * sx(src)) & WORDM;
        if (md <= 1) AC = r; else if (md == 2) wr(e, r); else { wr(e, r); AC = r; }
        break;
    }
    case 0224: case 0225: case 0226: case 0227: {                   /* MUL */
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        __int128 p = (__int128)sx(AC) * (__int128)sx(src);
        word hi = (word)((p >> 35) & (__int128)WORDM);
        word lo = (word)(p & (__int128)0377777777777ULL);
        if (p < 0) lo |= SIGN;
        if (md <= 1) { AC = hi; ACn(1) = lo; }
        else if (md == 2) wr(e, hi);
        else { wr(e, hi); AC = hi; ACn(1) = lo; }
        break;
    }
    case 0230: case 0231: case 0232: case 0233: {                   /* IDIV */
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        sword d = sx(src), n = sx(AC), q, r;
        if (d == 0) { flags |= F_OV; pc = (pc + 1) & (int)RMASK; skipped = 1; break; }
        q = n / d; r = n % d;
        if (md <= 1) { AC = (word)q & WORDM; ACn(1) = (word)r & WORDM; }
        else if (md == 2) wr(e, (word)q & WORDM);
        else { wr(e, (word)q & WORDM); AC = (word)q & WORDM; ACn(1) = (word)r & WORDM; }
        break;
    }
    case 0234: case 0235: case 0236: case 0237: {                   /* DIV */
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        __int128 n = ((__int128)sx(AC) << 35) | (__int128)(ACn(1) & 0377777777777ULL);
        sword d = sx(src); __int128 q, r;
        if (d == 0) { flags |= F_OV; pc = (pc + 1) & (int)RMASK; skipped = 1; break; }
        q = n / d; r = n % d;
        if (q > (__int128)0377777777777LL || q < -(__int128)0400000000000LL) {
            flags |= F_OV; pc = (pc + 1) & (int)RMASK; skipped = 1; break;
        }
        if (md <= 1) { AC = (word)q & WORDM; ACn(1) = (word)r & WORDM; }
        else if (md == 2) wr(e, (word)q & WORDM);
        else { wr(e, (word)q & WORDM); AC = (word)q & WORDM; ACn(1) = (word)r & WORDM; }
        break;
    }

    /* -------------------------------------------------------------- shifts */
    case 0240: {                                                    /* ASH */
        int sc = e; word w = AC; sword v = sx(w);
        if (sc & 0400000) {                       /* right: arithmetic     */
            sc = 01000000 - sc;
            if (sc >= 36) v = (v < 0) ? -1 : 0; else v >>= sc;
            AC = (word)v & WORDM;
        } else {                                  /* left: sign preserved  */
            word mag = w & 0377777777777ULL;
            mag = (sc >= 35) ? 0 : ((mag << sc) & 0377777777777ULL);
            AC = (w & SIGN) | mag;
        }
        break;
    }
    case 0241: {                                                    /* ROT */
        int sc = e; int n; word w = AC;
        if (sc & 0400000) sc = 01000000 - sc, n = (36 - (sc % 36)) % 36;
        else n = sc % 36;
        AC = n ? (((w << n) | (w >> (36 - n))) & WORDM) : w;
        break;
    }
    case 0242: {                                                    /* LSH */
        int sc = e; word w = AC;
        if (sc & 0400000) { sc = 01000000 - sc; AC = (sc >= 36) ? 0 : (w >> sc); }
        else AC = (sc >= 36) ? 0 : ((w << sc) & WORDM);
        break;
    }
    case 0243: {                                                    /* JFFO */
        word w = AC; int n = 0;
        if (w == 0) { ACn(1) = 0; break; }
        while (!(w & SIGN)) { w = (w << 1) & WORDM; n++; }
        ACn(1) = (word)n; pc = e; jumped = 1;
        break;
    }
    case 0244: {                                                    /* ASHC */
        int sc = e; int ng = (AC & SIGN) != 0;
        __int128 full = ((__int128)(AC & WORDM) << 36) | (word)(ACn(1) & WORDM);
        __int128 mag;
        if (ng) full = (((__int128)1 << 72) - full);
        mag = (((full >> 36) & WORDM) << 35) | ((full & WORDM) & 0377777777777ULL);
        if (sc & 0400000) { sc = 01000000 - sc; mag = (sc >= 70) ? 0 : (mag >> sc); }
        else { mag = (sc >= 70) ? 0 : (mag << sc); mag &= (((__int128)1 << 70) - 1); }
        {
            word hi = (word)((mag >> 35) & 0377777777777ULL);
            word lo = (word)(mag & 0377777777777ULL);
            if (ng) {
                __int128 t = (((__int128)1 << 71) - (((__int128)hi << 35) | lo));
                hi = (word)((t >> 35) & 0377777777777ULL) | SIGN;
                lo = (word)(t & 0377777777777ULL) | SIGN;
            }
            AC = hi; ACn(1) = lo;
        }
        break;
    }
    case 0245: {                                                    /* ROTC */
        int sc = e; int n;
        __int128 v = ((__int128)AC << 36) | ACn(1);
        if (sc & 0400000) { sc = 01000000 - sc; n = (72 - (sc % 72)) % 72; }
        else n = sc % 72;
        if (n) v = ((v << n) | (v >> (72 - n)));
        v &= (((__int128)1 << 72) - 1);
        AC = (word)((v >> 36) & WORDM); ACn(1) = (word)(v & WORDM);
        break;
    }
    case 0246: {                                                    /* LSHC */
        int sc = e;
        __int128 v = ((__int128)AC << 36) | ACn(1);
        if (sc & 0400000) { sc = 01000000 - sc; v = (sc >= 72) ? 0 : (v >> sc); }
        else v = (sc >= 72) ? 0 : (v << sc);
        v &= (((__int128)1 << 72) - 1);
        AC = (word)((v >> 36) & WORDM); ACn(1) = (word)(v & WORDM);
        break;
    }

    /* -------------------------------------------------------- misc control */
    case 0250: { word t = AC; AC = rd(e); wr(e, t); break; }        /* EXCH */
    case 0251: {                                                    /* BLT  */
        int s = (int)((AC >> 18) & RMASK), d = (int)(AC & RMASK);
        for (;;) { wr(d, rd(s)); if (d >= e) break; s++; d++; }
        AC = (((word)s & RMASK) << 18) | ((word)d & RMASK);
        break;
    }
    case 0252:                                                      /* AOBJP */
        AC = (AC + 01000001ULL) & WORDM;
        if (!(AC & SIGN)) { pc = e; jumped = 1; }
        break;
    case 0253:                                                      /* AOBJN */
        AC = (AC + 01000001ULL) & WORDM;
        if (AC & SIGN) { pc = e; jumped = 1; }
        break;
    case 0254:                                                      /* JRST  */
        switch (ac) {
        case 4:
            fprintf(stderr, "\n[program halted at %06o]\n", frompc);
            halted = 1;
            break;
        case 2:                                                     /* JRSTF */
            flags = (int)((inst >> 18) & 0777400) | F_USER;
            pc = e; jumped = 1;
            break;
        default:
            pc = e; jumped = 1;
            break;
        }
        break;
    case 0255: {                                                    /* JFCL */
        int m = 0;
        if (ac & 010) m |= F_OV;
        if (ac & 004) m |= F_CRY0;
        if (ac & 002) m |= F_CRY1;
        if (ac & 001) m |= F_FOV;
        if (flags & m) { flags &= ~m; pc = e; jumped = 1; }
        break;
    }
    case 0256: {                                                    /* XCT */
        static int depth;
        int r;
        if (depth > 16) break;
        depth++;
        r = execute(rd(e), e);
        depth--;
        if (r == 2) return 2;
        break;
    }

    /* ----------------------------------------------------------- stack ops */
    case 0260:                                                      /* PUSHJ */
        AC = (AC + 01000001ULL) & WORDM;
        wr((int)(AC & RMASK), (((word)flags & 0777777ULL) << 18) | (word)pc);
        pc = e; jumped = 1;
        break;
    case 0261:                                                      /* PUSH */
        AC = (AC + 01000001ULL) & WORDM;
        wr((int)(AC & RMASK), rd(e));
        break;
    case 0262:                                                      /* POP */
        wr(e, rd((int)(AC & RMASK)));
        AC = (AC - 01000001ULL) & WORDM;
        break;
    case 0263: {                                                    /* POPJ */
        word t = rd((int)(AC & RMASK));
        AC = (AC - 01000001ULL) & WORDM;
        pc = (int)(t & RMASK); jumped = 1;
        break;
    }
    case 0264:                                                      /* JSR */
        wr(e, (((word)flags & 0777777ULL) << 18) | (word)pc);
        pc = e + 1; jumped = 1;
        break;
    case 0265:                                                      /* JSP */
        if (e == 0504775 || e == 0504767) {
            extern int montrace;
            if (montrace)
                fprintf(stderr, "[%06o BOUNDS trap ac2=%012llo(%lld) ac3=%012llo ac4=%012llo]\n",
                        frompc, mem[2], (long long)sx(mem[2]), mem[3], mem[4]);
        }
        AC = (((word)flags & 0777777ULL) << 18) | (word)pc;
        pc = e; jumped = 1;
        break;
    case 0266:                                                      /* JSA */
        wr(e, (AC & LMASK) | (word)pc);
        AC = (((word)e & RMASK) << 18) | (word)pc;
        pc = e + 1; jumped = 1;
        break;
    case 0267:                                                      /* JRA */
        AC = rd((int)((AC >> 18) & RMASK));
        pc = e; jumped = 1;
        break;

    /* ---------------------------------------------------------- add / sub */
    case 0270: case 0271: case 0272: case 0273: {
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        word r = add36(AC, src);
        if (md <= 1) AC = r; else if (md == 2) wr(e, r); else { wr(e, r); AC = r; }
        break;
    }
    case 0274: case 0275: case 0276: case 0277: {
        int md = op & 3;
        word src = (md == 1) ? (word)e : rd(e);
        word r = sub36(AC, src);
        if (md <= 1) AC = r; else if (md == 2) wr(e, r); else { wr(e, r); AC = r; }
        break;
    }

    /* -------------------------------------------------------- compare/jump */
    case 0300: case 0301: case 0302: case 0303:
    case 0304: case 0305: case 0306: case 0307:
        if (cond(op & 7, sx(AC) - (sword)e)) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        break;
    case 0310: case 0311: case 0312: case 0313:
    case 0314: case 0315: case 0316: case 0317:
        if (cond(op & 7, sx(AC) - sx(rd(e)))) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        break;
    case 0320: case 0321: case 0322: case 0323:
    case 0324: case 0325: case 0326: case 0327:
        if (cond(op & 7, sx(AC))) { pc = e; jumped = 1; }
        break;
    case 0330: case 0331: case 0332: case 0333:
    case 0334: case 0335: case 0336: case 0337: {
        word v = rd(e);
        if (ac) AC = v;
        if (cond(op & 7, sx(v))) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        break;
    }
    case 0340: case 0341: case 0342: case 0343:
    case 0344: case 0345: case 0346: case 0347:
        AC = add36(AC, 1);
        if (cond(op & 7, sx(AC))) { pc = e; jumped = 1; }
        break;
    case 0350: case 0351: case 0352: case 0353:
    case 0354: case 0355: case 0356: case 0357: {
        word v = add36(rd(e), 1);
        wr(e, v); if (ac) AC = v;
        if (cond(op & 7, sx(v))) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        break;
    }
    case 0360: case 0361: case 0362: case 0363:
    case 0364: case 0365: case 0366: case 0367:
        AC = sub36(AC, 1);
        if (cond(op & 7, sx(AC))) { pc = e; jumped = 1; }
        break;
    case 0370: case 0371: case 0372: case 0373:
    case 0374: case 0375: case 0376: case 0377: {
        word v = sub36(rd(e), 1);
        wr(e, v); if (ac) AC = v;
        if (cond(op & 7, sx(v))) { pc = (pc + 1) & (int)RMASK; skipped = 1; }
        break;
    }

    default:
        if (op >= 0140 && op <= 0177) {                             /* floating */
            int fn = (op >> 3) & 3;   /* 14x FAD  15x FSB  16x FMP  17x FDV */
            int md = op & 7;
            word src = (md == 5) ? (((word)e << 18) & WORDM) : rd(e);
            double a = w2d(AC), b = w2d(src), r = 0;
            switch (fn) {
            case 0: r = a + b; break;
            case 1: r = a - b; break;
            case 2: r = a * b; break;
            default:
                if (b == 0) { flags |= F_FOV; r = 0; } else r = a / b;
                break;
            }
            {
                word rw = d2w(r);
                switch (md) {
                case 0: case 4: case 5: AC = rw; break;
                case 1: AC = rw; ACn(1) = 0; break;
                case 2: case 6: wr(e, rw); break;
                default: wr(e, rw); AC = rw; break;
                }
            }
            break;
        }
        if (op >= 0400 && op <= 0477) {                             /* boolean */
            int fn = (op >> 2) & 017, md = op & 3;
            word a = AC, m = (md == 1) ? (word)e : rd(e), r;
            switch (fn) {
            case 000: r = 0; break;
            case 001: r = a & m; break;
            case 002: r = ~a & m; break;
            case 003: r = m; break;
            case 004: r = a & ~m; break;
            case 005: r = a; break;
            case 006: r = a ^ m; break;
            case 007: r = a | m; break;
            case 010: r = ~a & ~m; break;
            case 011: r = ~(a ^ m); break;
            case 012: r = ~a; break;
            case 013: r = ~a | m; break;
            case 014: r = ~m; break;
            case 015: r = a | ~m; break;
            case 016: r = ~a | ~m; break;
            default:  r = WORDM; break;
            }
            r &= WORDM;
            if (md <= 1) AC = r; else if (md == 2) wr(e, r); else { wr(e, r); AC = r; }
            break;
        }
        if (op >= 0500 && op <= 0577) {                             /* half word */
            int n = op & 077;
            int dstRight = n & 040, mod = (n >> 3) & 3, srcOther = n & 004, md = n & 3;
            word srcw = (md == 1) ? (word)e : (md == 2) ? AC : rd(e);
            word dsto = (md <= 1) ? AC : rd(e);
            word h, other, r;
            if (!dstRight) h = srcOther ? (srcw & RMASK) : ((srcw >> 18) & RMASK);
            else           h = srcOther ? ((srcw >> 18) & RMASK) : (srcw & RMASK);
            switch (mod) {
            case 0: other = dstRight ? ((dsto >> 18) & RMASK) : (dsto & RMASK); break;
            case 1: other = 0; break;
            case 2: other = RMASK; break;
            default: other = (h & 0400000) ? RMASK : 0; break;
            }
            r = dstRight ? ((other << 18) | h) : ((h << 18) | other);
            if (md <= 1) AC = r;
            else if (md == 2) wr(e, r);
            else { wr(e, r); if (ac) AC = r; }
            break;
        }
        if (op >= 0600 && op <= 0677) {                             /* test */
            int n = op & 077;
            int mod = (n >> 4) & 3, direct = n & 010, swapl = n & 001;
            int sk = (n >> 1) & 3;
            word m, v;
            if (!direct) m = swapl ? (((word)e << 18) & WORDM) : (word)e;
            else { m = rd(e); if (swapl) m = ((m << 18) | (m >> 18)) & WORDM; }
            v = AC & m;
            switch (sk) {
            case 1: if (v == 0) { pc = (pc + 1) & (int)RMASK; skipped = 1; } break;
            case 2: pc = (pc + 1) & (int)RMASK; skipped = 1; break;
            case 3: if (v != 0) { pc = (pc + 1) & (int)RMASK; skipped = 1; } break;
            default: break;
            }
            switch (mod) {
            case 1: AC &= ~m; break;
            case 2: AC ^= m; break;
            case 3: AC |= m; break;
            default: break;
            }
            AC &= WORDM;
            break;
        }
        fprintf(stderr, "\n?unimplemented opcode %03o at %06o (%012llo)\n",
                op, frompc, inst);
        halted = 1;
        return 2;
    }

    (void)i;
    return 0;
}

int cpu_run(void)
{
    while (!halted && !exiting) {
        word inst;
        int frompc = pc;
        if (pc < 0 || pc >= MEMSIZ) { fprintf(stderr, "\n?PC out of range\n"); break; }
        inst = mem[pc];
        pc = (pc + 1) & (int)RMASK;
        icount++;
        if (trace) fprintf(stderr, "%06o/ %012llo\n", frompc, inst);
        if (execute(inst, frompc) == 2) break;
    }
    tty_flush();
    if (trace)
        fprintf(stderr, "[loop end: halted=%d exiting=%d pc=%06o]\n", halted, exiting, pc);
    return exit_code;
}
