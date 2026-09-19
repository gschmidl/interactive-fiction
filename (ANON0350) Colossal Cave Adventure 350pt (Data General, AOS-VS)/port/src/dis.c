/* dis.c -- Data General Eclipse (16-bit) disassembler for THISSALA.
 *
 *   dis <image> <base> <from> <to> [-p <poolimage> <poolbase>]
 *
 * <base>, <from>, <to> are WORD addresses in hex.  PLOT.PR is a flat image
 * of words 0x400..0x7FFF, so `dis PLOT.PR 400 4560 4574` dumps the CMD3
 * literal pool.  PLOT.OL holds the overlay code; its load base is not yet
 * known, so pass 0 and read the offsets as file-relative.
 *
 * -p supplies a second image used only to resolve absolute operands, so
 * overlay code can be disassembled with PLOT.PR's data visible.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eclipse.h"

static word img[MEMWORDS];
static char present[MEMWORDS];
static word pool[MEMWORDS];
static char pool_present[MEMWORDS];

static const char *ALU[8] = {"COM","NEG","MOV","INC","ADC","SUB","ADD","AND"};
static const char *SH[4]  = {"","L","R","S"};
static const char *CY[4]  = {"","Z","O","C"};
static const char *SKP[8] = {"","SKP","SZC","SNC","SZR","SNR","SEZ","SBN"};
static const char *MRN[4] = {"JMP","JSR","ISZ","DSZ"};
static const char *IDX[4] = {"",",PC",",2",",3"};

static int load(const char *path, word *dst, char *flag, unsigned base)
{
    FILE *f = fopen(path, "rb");
    unsigned char b[2];
    unsigned a = base;
    if (!f) { perror(path); return -1; }
    while (fread(b, 1, 2, f) == 2 && a < MEMWORDS) {
        dst[a] = (word)((b[0] << 8) | b[1]);
        flag[a] = 1;
        a++;
    }
    fclose(f);
    fprintf(stderr, "loaded %s at %04X..%04X\n", path, base, a ? a - 1 : 0);
    return 0;
}

/* Describe an absolute operand: a small constant looks like data, a word
 * that points into the pool gets its value shown. */
static void annotate(char *out, size_t n, unsigned a)
{
    if (a < MEMWORDS && pool_present[a]) {
        word v = pool[a];
        /* A string descriptor is <byte pointer><len><len>; the byte pointer
         * of a literal always equals (its own word address + 3) * 2. */
        if (a + 2 < MEMWORDS && pool_present[a+2] &&
            pool[a+1] == pool[a+2] && pool[a+1] > 0 && pool[a+1] < 128 &&
            v == (word)((a + 3) * 2)) {
            char s[130]; unsigned i, len = pool[a+1];
            for (i = 0; i < len; i++) {
                word w = pool[a + 3 + (i >> 1)];
                int c = (i & 1) ? (w & 0xFF) : (w >> 8);
                s[i] = (c >= 32 && c < 127) ? (char)c : '.';
            }
            s[len] = 0;
            snprintf(out, n, "   ; -> \"%s\" (len %u)", s, len);
            return;
        }
        snprintf(out, n, "   ; [%04X] = %u", a, v);
        return;
    }
    snprintf(out, n, "   ; = %u", a);
}

/* Returns instruction length in words; fills text. */
static int decode(unsigned pc, char *txt, size_t n)
{
    word ir = img[pc];
    word w2 = (pc + 1 < MEMWORDS) ? img[pc + 1] : 0;
    char note[160];

    /* ---- two-word immediate: IORI / XORI / ANDI / ADDI ---------------- */
    if ((ir & EI_MASK) == EI_IORI || (ir & EI_MASK) == EI_XORI ||
        (ir & EI_MASK) == EI_ANDI || (ir & EI_MASK) == EI_ADDI) {
        const char *nm = (ir & EI_MASK) == EI_IORI ? "IORI" :
                         (ir & EI_MASK) == EI_XORI ? "XORI" :
                         (ir & EI_MASK) == EI_ANDI ? "ANDI" : "ADDI";
        snprintf(txt, n, "%s %04X,%u", nm, w2, (unsigned)((ir >> 11) & 3));
        return 2;
    }

    /* ---- Eclipse two-word memory reference --------------------------- */
    if ((ir & 0x00FF) == 0x38 && (ir & 0x8000)) {
        unsigned ac = (ir >> 11) & 3, ix = (ir >> 8) & 3;
        const char *nm = NULL; int hasac = 1;
        switch (ir & E2_MASK_AC) {
        case E2_ELDA: nm = "ELDA"; break;
        case E2_ESTA: nm = "ESTA"; break;
        case E2_ELEF: nm = "ELEF"; break;
        case E2_ELDB: nm = "ELDB"; break;
        case E2_ESTB: nm = "ESTB"; break;
        }
        if (!nm) {
            hasac = 0;
            switch (ir & E2_MASK_NOAC) {
            case E2_EJMP: nm = "EJMP"; break;
            case E2_EJSR: nm = "EJSR"; break;
            case E2_EISZ: nm = "EISZ"; break;
            case E2_EDSZ: nm = "EDSZ"; break;
            }
        }
        if (nm) {
            note[0] = 0;
            if (ix == 0) annotate(note, sizeof note, w2);
            else if (ix == 1)
                snprintf(note, sizeof note, "   ; -> %04X",
                         (unsigned)((pc + 1 + (int16_t)w2) & AMASK));
            if (hasac) snprintf(txt, n, "%s %u,%04X%s%s", nm, ac, w2, IDX[ix], note);
            else       snprintf(txt, n, "%s %04X%s%s", nm, w2, IDX[ix], note);
            return 2;
        }
    }

    /* ---- Eclipse one-word extended ----------------------------------- */
    if (IS_ECLIPSE_EXT(ir)) {
        unsigned acs = ALC_ACS(ir), acd = ALC_ACD(ir);
        const char *nm = NULL;
        switch (ir) {                      /* fixed full-word opcodes */
        case E1_MUL:  nm = "MUL";  break;
        case E1_MULS: nm = "MULS"; break;
        case E1_DIV:  nm = "DIV";  break;
        case E1_DIVS: nm = "DIVS"; break;
        case E1_RTN:  nm = "RTN";  break;
        case E1_POPJ: nm = "POPJ"; break;
        case E1_POPB: nm = "POPB"; break;
        case E1_PSHR: nm = "PSHR"; break;
        case E1_RSTR: nm = "RSTR"; break;
        case E1_BLM:  nm = "BLM";  break;
        case E1_BAM:  nm = "BAM";  break;
        case E1_DIVX: nm = "DIVX"; break;
        }
        if (nm) { snprintf(txt, n, "%s", nm); return 1; }
        if (ir == E1_SAVE) {            /* two words: opcode + frame size */
            snprintf(txt, n, "SAVE %u             ; frame", w2);
            return 2;
        }
        if ((ir & E1_MSP_MASK) == E1_MSP) { snprintf(txt, n, "MSP %u", acs); return 1; }
        switch (ir & E1_MASK) {
        case E1_ADI:  nm = "ADI";  break;  case E1_SBI:  nm = "SBI";  break;
        case E1_DAD:  nm = "DAD";  break;  case E1_DSB:  nm = "DSB";  break;
        case E1_IOR:  nm = "IOR";  break;  case E1_XOR:  nm = "XOR";  break;
        case E1_ANC:  nm = "ANC";  break;  case E1_XCH:  nm = "XCH";  break;
        case E1_SGT:  nm = "SGT";  break;  case E1_SGE:  nm = "SGE";  break;
        case E1_LSH:  nm = "LSH";  break;  case E1_DLSH: nm = "DLSH"; break;
        case E1_HXL:  nm = "HXL";  break;  case E1_HXR:  nm = "HXR";  break;
        case E1_DHXL: nm = "DHXL"; break;  case E1_DHXR: nm = "DHXR"; break;
        case E1_BTO:  nm = "BTO";  break;  case E1_BTZ:  nm = "BTZ";  break;
        case E1_SZB:  nm = "SZB";  break;  case E1_SZBO: nm = "SZBO"; break;
        case E1_CLM:  nm = "CLM";  break;  case E1_LOB:  nm = "LOB";  break;
        case E1_LRB:  nm = "LRB";  break;  case E1_COB:  nm = "COB";  break;
        case E1_LDB:  nm = "LDB";  break;  case E1_SNB:  nm = "SNB";  break;
        case E1_STB:  nm = "STB";  break;  case E1_PSH:  nm = "PSH";  break;
        case E1_POP:  nm = "POP";  break;
        }
        if (nm) { snprintf(txt, n, "%s %u,%u", nm, acs, acd); return 1; }
        snprintf(txt, n, "?ECL %04X          <<< unknown extended", ir);
        return 1;
    }

    /* ---- plain ALC ---------------------------------------------------- */
    if (ir & 0x8000) {
        snprintf(txt, n, "%s%s%s%s %u,%u%s%s",
                 ALU[ALC_OP(ir)], CY[ALC_CY(ir)], SH[ALC_SH(ir)],
                 ALC_NL(ir) ? "#" : "", ALC_ACS(ir), ALC_ACD(ir),
                 ALC_SKIP(ir) ? "," : "", SKP[ALC_SKIP(ir)]);
        return 1;
    }

    /* ---- memory reference --------------------------------------------- */
    {
        unsigned op = MR_OP(ir), ix = MR_IDX(ir);
        /* JSR @12 is the overlay procedure call and carries exactly ONE
         * inline argument word.  The routine at 7D15 reads it (LDA 2,0,3 /
         * LDA 2,0,2) and adds one to the return address to step over it.
         * Missing this is what desynchronised linear disassembly. */
        if (ir == 0x0C0C) {
            snprintf(txt, n, "JSR @12            ; overlay call, arg %04X",
                     img[(pc + 1) & AMASK]);
            return 2;
        }
        const char *ind = MR_IND(ir) ? "@" : "";
        int d = ix ? disp8(ir) : (int)MR_DISP(ir);
        char tgt[64];
        tgt[0] = 0;
        if (ix == 1) snprintf(tgt, sizeof tgt, "   ; -> %04X",
                              (unsigned)((pc + d) & AMASK));
        if (op < 4)       snprintf(txt, n, "%s %s%d%s%s", MRN[op], ind, d, IDX[ix], tgt);
        else if (op < 8)  snprintf(txt, n, "LDA %u,%s%d%s%s", op - 4, ind, d, IDX[ix], tgt);
        else if (op < 12) snprintf(txt, n, "STA %u,%s%d%s%s", op - 8, ind, d, IDX[ix], tgt);
        else              snprintf(txt, n, "LEF %u,%s%d%s%s", op - 12, ind, d, IDX[ix], tgt);
        return 1;
    }
}

int main(int argc, char **argv)
{
    unsigned base, from, to, pc;
    if (argc < 5) {
        fprintf(stderr,
            "usage: dis <image> <base> <from> <to> [-p <poolimage> <poolbase>]\n"
            "       addresses are hex WORD addresses\n");
        return 2;
    }
    base = (unsigned)strtoul(argv[2], NULL, 16);
    from = (unsigned)strtoul(argv[3], NULL, 16);
    to   = (unsigned)strtoul(argv[4], NULL, 16);
    if (load(argv[1], img, present, base)) return 1;

    if (argc >= 8 && !strcmp(argv[5], "-p"))
        load(argv[6], pool, pool_present, (unsigned)strtoul(argv[7], NULL, 16));
    else
        { memcpy(pool, img, sizeof pool); memcpy(pool_present, present, sizeof pool_present); }

    for (pc = from; pc < to && pc < MEMWORDS; ) {
        char txt[256];
        int len = decode(pc, txt, sizeof txt);
        if (len == 2) printf("%04X  %04X %04X   %s\n", pc, img[pc], img[pc+1], txt);
        else          printf("%04X  %04X        %s\n", pc, img[pc], txt);
        pc += len;
    }
    return 0;
}
