/* trace.c - a disassembler and an instruction trace for the emulator */
#include <string.h>
#include "fp4.h"

FILE *trace_fp;
unsigned long long trace_from;

static const char *type1[64] = {
    "HLT", "LDA1", "LD23", "LDA", "LDB", "LD1", "LD2", "LD3",
    "ORM", "INR", "ADM", "ADA", "ORA", "AD1", "AD2", "AD3",
    "ANM", "DEC", "SKN", "SBA", "ANA", "SB1", "SB2", "SB3",
    "XOM", "STA1", "ST23", "CPA", "XOA", "CP1", "CP2", "CP3",
    "STZ", "SAM", "STP", "STA", "STB", "ST1", "ST2", "ST3",
    "SLR", "SLRD", "SLA", "SLAD", "SRL", "SRLD", "SRA", "SRAD",
    "SKZ", "MCC", "BOF", "BZO", "BMI", "BCR", "BAL", "BGT",
    "XEC", "BRM", "BRA", "BNZ", "BPL", "BC1", "BC2", "BC3",
};

static const char *type2[64] = {
    "LCL", "LPL", "RCL", "RLC", "LCR", "LPR", "RCR", "RRC",
    "ROR", "RADD", "MPY", "MVL", "UFA", "FAD", "FMP", "MVR",
    "RAND", "RSUB", "DIV", "MVE", "CDA2", "FSB", "FDV", "IOB",
    "RXOR", "RCM2", "POP", "UP", "IN", "TRT", "?367", "BOOT",
    "SCL", "SPL", "PUSH", "DOWN", "SCR", "SPR", "TRAP", "ECS",
    "BRD", "BRR", "EXCT", "EXSN", "PIA", "PID", "PIR", "IOID",
    "CPN", "MVCR", "DADD", "CPL", "DSUB", "ODD", "MVCL", "IO",
    "BDEC", "DBIN", "BYTE", "MAP", "IOXW", "MVEL", "BIT", "?777",
};

static const char *rn[8] = { "R0", "R1", "RP", "RA", "RB", "X1", "X2", "X3" };

/* which type-2 op codes take a memory address */
static int addr2(int op)
{
    static const int ops[] = {
        000, 001, 004, 005, 027, 032, 033, 034, 035, 040, 041, 042, 043, 044, 045,
        046, 050, 051, 052, 053, 054, 055, 056, 057, 067, 074, -1
    };
    int i;

    for (i = 0; ops[i] >= 0; i++)
        if (ops[i] == op)
            return 1;
    return 0;
}

const char *disasm(word ir, char *buf)
{
    int op = ir >> 18, mod = (ir >> 15) & 7;
    word a = ir & A15;

    if (mod != 7) {
        static const char *m[7] = { "", "*", ",X1", "*,X1", ",X2", "*,X2", ",X3" };

        if (mod & 1)
            sprintf(buf, "%-5s *%05o%s", type1[op], a, m[mod] + 1);
        else
            sprintf(buf, "%-5s %05o%s", type1[op], a, m[mod]);
        return buf;
    }
    if (addr2(op)) {
        sprintf(buf, "%-5s %05o", type2[op], a);
        return buf;
    }
    switch (op) {
    case 012: case 022: case 023: case 014: case 015: case 016: case 025: case 026:
        sprintf(buf, "%-5s %d", type2[op], (int)(ir & 077));
        break;
    case 013: case 017:
        sprintf(buf, "%-5s %o,%d", type2[op], (int)(ir >> 6) & 7, (int)(ir & 077));
        break;
    default:
        sprintf(buf, "%-5s %s,%s,%o,%d", type2[op], rn[(ir >> 12) & 7],
                rn[(ir >> 9) & 7], (int)(ir >> 6) & 7, (int)(ir & 077));
        break;
    }
    return buf;
}

/* ---- a ring of the last instructions, and the full trace ------------- */

#define RING 256

static struct {
    unsigned long long n;
    word pc, ir, ra, rb, x1, x2, x3;
    int cc;
} ring[RING];
static unsigned ring_pos;

int trace_win = -1;                     /* trace only this window (-1: all) */

void trace_insn(word pc, word ir)
{
    unsigned k = ring_pos++ % RING;

    ring[k].n = icount;
    ring[k].pc = pc;
    ring[k].ir = ir;
    ring[k].ra = RA;
    ring[k].rb = RB;
    ring[k].x1 = X1;
    ring[k].x2 = X2;
    ring[k].x3 = X3;
    ring[k].cc = cc_o << 3 | cc_z << 2 | cc_m << 1 | cc_c;
    if (trace_fp && icount >= trace_from && (trace_win < 0 || cur_win == trace_win)) {
        char b[64];

        fprintf(trace_fp, "%10llu w%03o %05o %08o %-24s RA %08o RB %08o X1 %08o X2 %08o X3 %08o %d%d%d%d\n",
                icount, cur_win, pc, ir, disasm(ir, b), RA, RB, X1, X2, X3,
                cc_o, cc_z, cc_m, cc_c);
    }
}

void trace_dump_ring(FILE *f, int n)
{
    unsigned i, start = ring_pos > (unsigned)n ? ring_pos - (unsigned)n : 0;

    for (i = start; i < ring_pos; i++) {
        unsigned k = i % RING;
        char b[64];

        fprintf(f, "%10llu %05o %08o %-24s RA %08o RB %08o X1 %08o X2 %08o X3 %08o %o\n",
                ring[k].n, ring[k].pc, ring[k].ir, disasm(ring[k].ir, b), ring[k].ra,
                ring[k].rb, ring[k].x1, ring[k].x2, ring[k].x3, ring[k].cc);
    }
}
