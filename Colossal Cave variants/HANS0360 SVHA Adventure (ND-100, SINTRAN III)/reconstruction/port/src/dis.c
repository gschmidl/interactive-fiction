/* dis.c - one-line ND-100 disassembly for traces */
#include <string.h>
#include "nd100.h"

static const char *regn[8] = { "0", "D", "P", "B", "L", "A", "T", "X" };
static const char *memop[24] = { "STZ", "STA", "STT", "STX", "STD", "LDD", "STF", "LDF",
    "MIN", "LDA", "LDT", "LDX", "ADD", "SUB", "AND", "ORA", "FAD", "FSB", "FMU", "FDV",
    "MPY", "JMP", "?", "JPL" };
static const char *cjp[8] = { "JAP", "JAN", "JAZ", "JAF", "JPC", "JNC", "JXZ", "JXN" };
static const char *skp[8] = { "EQL", "GEQ", "GRE", "MGRE", "UEQ", "LSS", "LST", "MLST" };
static const char *arg[8] = { "SAB", "SAA", "SAT", "SAX", "AAB", "AAA", "AAT", "AAX" };
static const char *bop[16] = { "BSET ZRO", "BSET ONE", "BSET BCM", "BSET BAC", "BSKP ZRO",
    "BSKP ONE", "BSKP BCM", "BSKP BAC", "BSTC", "BSTA", "BLDC", "BLDA", "BANC", "BAND",
    "BORC", "BORA" };

const char *cpu_dis(uint16_t w, uint16_t addr, char *buf)
{
    int op = w >> 11, d = (int8_t)(w & 0xFF), mode = (w >> 8) & 7;
    if (op < 24 && op != 026) {
        uint16_t loc = (uint16_t)(addr + d);
        switch (mode) {
        case 0: sprintf(buf, "%s %06o", memop[op], loc); break;
        case 1: sprintf(buf, "%s %d,B", memop[op], d); break;
        case 2: sprintf(buf, "%s I %06o", memop[op], loc); break;
        case 3: sprintf(buf, "%s I %d,B", memop[op], d); break;
        case 4: sprintf(buf, "%s %d,X", memop[op], d); break;
        case 5: sprintf(buf, "%s %d,B,X", memop[op], d); break;
        case 6: sprintf(buf, "%s I %06o,X", memop[op], loc); break;
        default: sprintf(buf, "%s I %d,B,X", memop[op], d); break;
        }
        return buf;
    }
    if (op == 026) { sprintf(buf, "%s %06o", cjp[mode], (uint16_t)(addr + d)); return buf; }
    if (op == 030 && (w & 0300) == 0) {
        sprintf(buf, "SKP IF D%s %s S%s", regn[w & 7], skp[mode], regn[(w >> 3) & 7]);
        return buf;
    }
    if (op == 031) {
        const char *n;
        if ((w >> 10) & 1) n = ((w & 01700) == 0100) ? "COPY" : "RADD";
        else n = ((const char *[]){ "SWAP", "RAND", "REXO", "RORA" })[(w >> 8) & 3];
        sprintf(buf, "%s%s%s%s%s S%s D%s", n,
                ((w >> 10) & 1) && (w & 01000) ? " ADC" : "",
                ((w >> 10) & 1) && (w & 0400) ? " AD1" : "",
                (w & 0200) ? " CM1" : "",
                ((w & 0100) && strcmp(n, "COPY")) ? " CLD" : "",
                regn[(w >> 3) & 7], regn[w & 7]);
        return buf;
    }
    if (op == 033) {
        static const char *rg[4] = { "SHT", "SHD", "SHA", "SAD" };
        static const char *ty[4] = { "", " ROT", " ZIN", " LIN" };
        int n = w & 077;
        if (n & 040) n -= 0100;
        sprintf(buf, "%s%s %d", rg[(w >> 7) & 3], ty[(w >> 9) & 3], n);
        return buf;
    }
    if (op == 036) { sprintf(buf, "%s %d", arg[mode], d); return buf; }
    if (op == 037) {
        int dr = w & 7, bn = (w >> 3) & 017;
        if (dr == 0) sprintf(buf, "%s STS%o", bop[(w >> 7) & 017], bn);
        else sprintf(buf, "%s %o D%s", bop[(w >> 7) & 017], bn, regn[dr]);
        return buf;
    }
    switch (w & 0177400) {
    case 0153000: sprintf(buf, "MON %o", w & 0377); return buf;
    case 0151400: sprintf(buf, "NLZ %d", d); return buf;
    case 0152000: sprintf(buf, "DNZ %d", d); return buf;
    }
    switch (w & 0177700) {
    case 0140600: sprintf(buf, "EXR S%s", regn[(w >> 3) & 7]); return buf;
    case 0141200: sprintf(buf, "RMPY S%s D%s", regn[(w >> 3) & 7], regn[w & 7]); return buf;
    case 0141600: sprintf(buf, "RDIV S%s", regn[(w >> 3) & 7]); return buf;
    case 0142200: sprintf(buf, "LBYT"); return buf;
    case 0142600: sprintf(buf, "SBYT"); return buf;
    }
    switch (w) {
    case 0140130: return strcpy(buf, "BFILL");
    case 0140131: return strcpy(buf, "MOVB");
    case 0140132: return strcpy(buf, "MOVBF");
    case 0140134: return strcpy(buf, "INIT");
    case 0140135: return strcpy(buf, "ENTR");
    case 0140136: return strcpy(buf, "LEAVE");
    case 0140137: return strcpy(buf, "ELEAV");
    }
    sprintf(buf, "%06o", w);
    return buf;
}
