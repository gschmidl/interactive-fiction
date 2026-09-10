/* mvops.h -- the ECLIPSE MV instruction table, extracted from MASM.PR.
 *
 * MASM.PR is the 32-bit macro assembler out of the AOS/VS :UTIL dump, and it
 * carries its mnemonic table in the clear.  Each entry is 24 bytes:
 *
 *   [opcode: 2][0x0001: 2][format type: 2][0x001B: 2][name length: 2][name to 16]
 *
 * The opcode comes BEFORE the name, not after it -- reading it the other way
 * round shifts every mnemonic one record along, which silently renames half
 * the instruction set (XJSR becomes XJMP, XPEF becomes XJSR) and makes
 * compiled code disassemble into nonsense.  The parse is checked against
 * EBID.SR, DG own Eclipse instruction definitions: 50 Eclipse opcodes
 * spot-checked, all exact.
 *
 * Only the extension space is here: bit 15 set and a low nibble of 8 or 9.
 * Nibble 9 is where the MV put its wide instructions -- on an Eclipse that
 * encoding is an ALC with no-load and a skip field of 1, which does nothing
 * useful -- and nibble 8 is the older extension space the X and L form loads
 * and stores share with the Eclipse instructions.
 *
 * Operand placement by format type:
 *   0F,10               two accumulators, bits 14-13 and 12-11
 *   08,0B,0C,11,12      one accumulator, bits 12-11
 *   09                  accumulator bits 12-11, an immediate of 1..4 in 14-13
 *   05                  a frame size word follows
 *   2B,2C,18,19         X and L forms with no accumulator: index mode is in
 *                       bits 12-11
 *   26,27,28,2E,13,14,15,1A   X and L forms with one: the accumulator is bits
 *                       12-11 and the index mode bits 14-13
 *   16                  LCALL: a 32-bit target then an argument count
 *   1E,1F               no operand field
 *
 * Three opcodes carry TWO records, an Eclipse one and, much later in the
 * table among the MV additions, the instruction that replaced it.  The
 * later record is the right one for these programs: 8018 is XNADD, not
 * XOP; 8038 is WBR, not XOP1; 8748 is SVC, not SYC.  Taking the first
 * record hid WBR, the machine's short branch, behind an encoding that
 * also reads as a harmless no-load ALC -- so every short branch in every
 * 32-bit program quietly fell through instead of branching.
 *
 * The length column is derived from the format type, not carried by MASM.
 * Types 06 and 07 (the Eclipse floating memory references, and FSST/FLST)
 * and 0A (CIOI) are two-word instructions even though they name no index
 * register in the way types 04 and 05 do -- FLDS 0,[3420] in ADVENTURE.PR
 * is 8428 3420.  Getting these wrong makes a skip over one land on its
 * displacement word.
 *
 * X form displacements are 15 bits signed with bit 15 meaning indirect, and
 * PC-relative is relative to the displacement word.
 */
#ifndef MVOPS_H
#define MVOPS_H

typedef struct { unsigned short op; const char *name; unsigned char type, len; } mvop;

static const mvop mvops[] = {
    { 0x8008, "ADI", 0x09, 1 },
    { 0x8009, "XFAMS", 0x2E, 2 },
    { 0x8018, "XNADD", 0x2E, 2 },
    { 0x8019, "XFAMD", 0x2E, 2 },
    { 0x8028, "FAS", 0x0F, 1 },
    { 0x8029, "XFMMS", 0x2E, 2 },
    { 0x8038, "WBR", 0x23, 1 },
    { 0x8039, "XFMMD", 0x2E, 2 },
    { 0x8048, "SBI", 0x09, 1 },
    { 0x8049, "NADD", 0x0F, 1 },
    { 0x8058, "XNSUB", 0x2E, 2 },
    { 0x8059, "NSUB", 0x0F, 1 },
    { 0x8068, "FAD", 0x0F, 1 },
    { 0x8069, "NMUL", 0x0F, 1 },
    { 0x8079, "NDIV", 0x0F, 1 },
    { 0x8088, "DAD", 0x0F, 1 },
    { 0x8089, "WPOP", 0x0F, 1 },
    { 0x8098, "XNMUL", 0x2E, 2 },
    { 0x8099, "WUSGE", 0x10, 1 },
    { 0x80A8, "FSS", 0x0F, 1 },
    { 0x80A9, "WUSGT", 0x10, 1 },
    { 0x80B9, "WSEQ", 0x10, 1 },
    { 0x80C8, "DSB", 0x0F, 1 },
    { 0x80C9, "LFAMS", 0x1A, 3 },
    { 0x80D8, "XNDIV", 0x2E, 2 },
    { 0x80D9, "LFAMD", 0x1A, 3 },
    { 0x80E8, "FSD", 0x0F, 1 },
    { 0x80E9, "LFSMS", 0x1A, 3 },
    { 0x80F9, "LFSMD", 0x1A, 3 },
    { 0x8108, "IOR", 0x0F, 1 },
    { 0x8109, "XFSMS", 0x2E, 2 },
    { 0x8118, "XWADD", 0x2E, 2 },
    { 0x8119, "XFSMD", 0x2E, 2 },
    { 0x8128, "FMS", 0x0F, 1 },
    { 0x8129, "XFDMS", 0x2E, 2 },
    { 0x8139, "XFDMD", 0x2E, 2 },
    { 0x8148, "XOR", 0x0F, 1 },
    { 0x8149, "WADD", 0x0F, 1 },
    { 0x8158, "XWSUB", 0x2E, 2 },
    { 0x8159, "WSUB", 0x0F, 1 },
    { 0x8168, "FMD", 0x0F, 1 },
    { 0x8169, "WMUL", 0x0F, 1 },
    { 0x8179, "WDIV", 0x0F, 1 },
    { 0x8188, "ANC", 0x0F, 1 },
    { 0x8189, "WSNE", 0x10, 1 },
    { 0x8198, "XWMUL", 0x2E, 2 },
    { 0x8199, "WSGE", 0x10, 1 },
    { 0x81A8, "FDS", 0x0F, 1 },
    { 0x81A9, "WSLE", 0x10, 1 },
    { 0x81B9, "WSGT", 0x10, 1 },
    { 0x81C8, "XCH", 0x0F, 1 },
    { 0x81C9, "LFMMS", 0x1A, 3 },
    { 0x81D8, "XWDIV", 0x2E, 2 },
    { 0x81D9, "LFMMD", 0x1A, 3 },
    { 0x81E8, "FDD", 0x0F, 1 },
    { 0x81E9, "LFDMS", 0x1A, 3 },
    { 0x81F9, "LFDMD", 0x1A, 3 },
    { 0x8208, "SGT", 0x10, 1 },
    { 0x8209, "XFLDS", 0x2E, 2 },
    { 0x8218, "LNADD", 0x1A, 3 },
    { 0x8219, "XFLDD", 0x2E, 2 },
    { 0x8228, "FAMS", 0x06, 2 },
    { 0x8229, "XFSTS", 0x2E, 2 },
    { 0x8239, "XFSTD", 0x2E, 2 },
    { 0x8248, "SGE", 0x10, 1 },
    { 0x8249, "WADC", 0x0F, 1 },
    { 0x8258, "LNSUB", 0x1A, 3 },
    { 0x8259, "WINC", 0x0F, 1 },
    { 0x8268, "FAMD", 0x06, 2 },
    { 0x8269, "WNEG", 0x0F, 1 },
    { 0x8279, "WASH", 0x0F, 1 },
    { 0x8288, "LSH", 0x0F, 1 },
    { 0x8289, "WSLT", 0x10, 1 },
    { 0x8298, "LNMUL", 0x1A, 3 },
    { 0x8299, "WBTO", 0x0F, 1 },
    { 0x82A8, "FSMS", 0x06, 2 },
    { 0x82A9, "WBTZ", 0x0F, 1 },
    { 0x82B9, "WSZB", 0x10, 1 },
    { 0x82C8, "DLSH", 0x0F, 1 },
    { 0x82C9, "LFLDS", 0x1A, 3 },
    { 0x82D8, "LNDIV", 0x1A, 3 },
    { 0x82D9, "LFLDD", 0x1A, 3 },
    { 0x82E8, "FSMD", 0x06, 2 },
    { 0x82E9, "LFSTS", 0x1A, 3 },
    { 0x82F9, "LFSTD", 0x1A, 3 },
    { 0x8308, "HXL", 0x09, 1 },
    { 0x8309, "XWLDA", 0x2E, 2 },
    { 0x8318, "LWADD", 0x1A, 3 },
    { 0x8319, "XWSTA", 0x2E, 2 },
    { 0x8328, "FMMS", 0x06, 2 },
    { 0x8329, "XNLDA", 0x2E, 2 },
    { 0x8339, "XNSTA", 0x2E, 2 },
    { 0x8348, "HXR", 0x09, 1 },
    { 0x8349, "SEX", 0x0F, 1 },
    { 0x8358, "LWSUB", 0x1A, 3 },
    { 0x8359, "ZEX", 0x0F, 1 },
    { 0x8368, "FMMD", 0x06, 2 },
    { 0x8369, "WXCH", 0x0F, 1 },
    { 0x8379, "WMOV", 0x0F, 1 },
    { 0x8388, "DHXL", 0x09, 1 },
    { 0x8389, "WSNB", 0x10, 1 },
    { 0x8398, "LWMUL", 0x1A, 3 },
    { 0x8399, "WSZBO", 0x10, 1 },
    { 0x83A8, "FDMS", 0x06, 2 },
    { 0x83A9, "WLOB", 0x0F, 1 },
    { 0x83B9, "WLRB", 0x0F, 1 },
    { 0x83C8, "DHXR", 0x09, 1 },
    { 0x83C9, "LNLDA", 0x1A, 3 },
    { 0x83D8, "LWDIV", 0x1A, 3 },
    { 0x83D9, "LNSTA", 0x1A, 3 },
    { 0x83E8, "FDMD", 0x06, 2 },
    { 0x83E9, "LLEF", 0x1A, 3 },
    { 0x83F9, "LWLDA", 0x1A, 3 },
    { 0x8408, "BTO", 0x0F, 1 },
    { 0x8409, "XLEF", 0x2E, 2 },
    { 0x8418, "XNADI", 0x28, 2 },
    { 0x8419, "XLDB", 0x26, 2 },
    { 0x8428, "FLDS", 0x06, 2 },
    { 0x8429, "XSTB", 0x26, 2 },
    { 0x8438, "EJMP", 0x03, 2 },
    { 0x8439, "XLEFB", 0x26, 2 },
    { 0x8448, "BTZ", 0x0F, 1 },
    { 0x8449, "WAND", 0x0F, 1 },
    { 0x8458, "XNSBI", 0x28, 2 },
    { 0x8459, "WCOM", 0x0F, 1 },
    { 0x8468, "FLDD", 0x06, 2 },
    { 0x8469, "WIOR", 0x0F, 1 },
    { 0x8478, "ELDB", 0x02, 2 },
    { 0x8479, "WXOR", 0x0F, 1 },
    { 0x8488, "SZB", 0x10, 1 },
    { 0x8489, "WCOB", 0x0F, 1 },
    { 0x8498, "XNDO", 0x2A, 3 },
    { 0x8499, "WFFAD", 0x0F, 1 },
    { 0x84A8, "FSTS", 0x06, 2 },
    { 0x84A9, "WFLAD", 0x0F, 1 },
    { 0x84B8, "PSHJ", 0x03, 2 },
    { 0x84B9, "WADI", 0x09, 1 },
    { 0x84C8, "SZBO", 0x10, 1 },
    { 0x84C9, "LLDB", 0x13, 3 },
    { 0x84D8, "FRDS", 0x0F, 1 },
    { 0x84D9, "LSTB", 0x13, 3 },
    { 0x84E8, "FSTD", 0x06, 2 },
    { 0x84E9, "LLEFB", 0x13, 3 },
    { 0x84F8, "CLM", 0x0F, 1 },
    { 0x84F9, "LWSTA", 0x1A, 3 },
    { 0x8508, "LOB", 0x0F, 1 },
    { 0x8509, "NNEG", 0x0F, 1 },
    { 0x8518, "XWADI", 0x28, 2 },
    { 0x8519, "LDSP", 0x1A, 3 },
    { 0x8528, "FLAS", 0x0F, 1 },
    { 0x8529, "WLDB", 0x0F, 1 },
    { 0x8539, "WSTB", 0x0F, 1 },
    { 0x8548, "LRB", 0x0F, 1 },
    { 0x8549, "WANC", 0x0F, 1 },
    { 0x8558, "XWSBI", 0x28, 2 },
    { 0x8559, "WLSH", 0x0F, 1 },
    { 0x8568, "FLMD", 0x06, 2 },
    { 0x8569, "WCLM", 0x0F, 1 },
    { 0x8579, "WPSH", 0x0F, 1 },
    { 0x8588, "COB", 0x0F, 1 },
    { 0x8589, "WSBI", 0x09, 1 },
    { 0x8598, "XWDO", 0x2A, 3 },
    { 0x8599, "NADI", 0x09, 1 },
    { 0x85A8, "FFAS", 0x0F, 1 },
    { 0x85A9, "NSBI", 0x09, 1 },
    { 0x85B9, "WLSI", 0x09, 1 },
    { 0x85C8, "LDB", 0x0F, 1 },
    { 0x85D9, "PIO", 0x0F, 1 },
    { 0x85E8, "FFMD", 0x06, 2 },
    { 0x85E9, "CIO", 0x0F, 1 },
    { 0x85F8, "SNB", 0x10, 1 },
    { 0x85F9, "CIOI", 0x0A, 2 },
    { 0x8608, "STB", 0x0F, 1 },
    { 0x8609, "XCALL", 0x29, 3 },
    { 0x8618, "LNADI", 0x15, 3 },
    { 0x8619, "XPSHJ", 0x2B, 2 },
    { 0x8628, "FNOM", 0x08, 1 },
    { 0x8629, "XPEF", 0x2B, 2 },
    { 0x8639, "XNISZ", 0x2C, 2 },
    { 0x8648, "PSH", 0x0F, 1 },
    { 0x8649, "LDATS", 0x08, 1 },
    { 0x8658, "LNSBI", 0x15, 3 },
    { 0x8659, "STATS", 0x08, 1 },
    { 0x8668, "FSCAL", 0x08, 1 },
    { 0x8688, "POP", 0x0F, 1 },
    { 0x8689, "WADDI", 0x11, 3 },
    { 0x8698, "LNDO", 0x17, 3 },
    { 0x8699, "WANDI", 0x11, 3 },
    { 0x86A8, "FNS", 0x1E, 1 },
    { 0x86A9, "WIORI", 0x11, 3 },
    { 0x86B9, "WXORI", 0x11, 3 },
    { 0x86C9, "LNISZ", 0x19, 3 },
    { 0x86D9, "LNDSZ", 0x19, 3 },
    { 0x86E8, "FSST", 0x07, 2 },
    { 0x86E9, "LWISZ", 0x19, 3 },
    { 0x86F8, "MSP", 0x08, 1 },
    { 0x86F9, "LWDSZ", 0x19, 3 },
    { 0x8718, "LWADI", 0x15, 3 },
    { 0x8728, "FCMP", 0x0F, 1 },
    { 0x8729, "WSSVR", 0x05, 2 },
    { 0x8739, "WSSVS", 0x05, 2 },
    { 0x8748, "SVC", 0x1E, 1 },
    { 0x8749, "PBX", 0x1E, 1 },
    { 0x8758, "LWSBI", 0x15, 3 },
    { 0x8759, "LCPID", 0x1E, 1 },
    { 0x8768, "FMOV", 0x0F, 1 },
    { 0x8769, "WCTR", 0x1E, 1 },
    { 0x8779, "WCMV", 0x1E, 1 },
    { 0x8789, "WPOPJ", 0x1E, 1 },
    { 0x8798, "LWDO", 0x17, 3 },
    { 0x8799, "WRSTR", 0x1E, 1 },
    { 0x87A8, "LDI", 0x08, 1 },
    { 0x87A9, "WRTN", 0x1E, 1 },
    { 0x87B9, "WFPSH", 0x1E, 1 },
    { 0x87C8, "PSHR", 0x1E, 1 },
    { 0x87C9, "LMRF", 0x1E, 1 },
    { 0x87D9, "SMRF", 0x1E, 1 },
    { 0x87E8, "FLOGD", 0x1E, 1 },
    { 0x87E9, "LPHY", 0x1F, 1 },
    { 0x87F8, "IORI", 0x0B, 2 },
    { 0x87F9, "WDPOP", 0x1E, 1 },
    { 0x8C38, "EJSR", 0x03, 2 },
    { 0x8EA8, "FSA", 0x1F, 1 },
    { 0x8F09, "DERR", 0x22, 1 },
    { 0x8F49, "WSKBO", 0x24, 1 },
    { 0x8F89, "WSKBZ", 0x24, 1 },
    { 0x8FC8, "POPB", 0x1E, 1 },
    { 0x8FE8, "FSQRS", 0x1E, 1 },
    { 0x9438, "EISZ", 0x03, 2 },
    { 0x96A8, "FSEQ", 0x1F, 1 },
    { 0x9708, "LMP", 0x1E, 1 },
    { 0x97C8, "BAM", 0x1E, 1 },
    { 0x9C38, "EDSZ", 0x03, 2 },
    { 0x9EA8, "FSNE", 0x1F, 1 },
    { 0x9FC8, "POPJ", 0x1E, 1 },
    { 0x9FE8, "FLOGS", 0x1E, 1 },
    { 0xA438, "ELDA", 0x04, 2 },
    { 0xA478, "ESTB", 0x02, 2 },
    { 0xA609, "XNDSZ", 0x2C, 2 },
    { 0xA619, "XWISZ", 0x2C, 2 },
    { 0xA628, "FRH", 0x08, 1 },
    { 0xA629, "XPEFB", 0x27, 2 },
    { 0xA639, "XWDSZ", 0x2C, 2 },
    { 0xA649, "LDASP", 0x08, 1 },
    { 0xA659, "STASP", 0x08, 1 },
    { 0xA668, "FEXP", 0x08, 1 },
    { 0xA669, "LDASL", 0x08, 1 },
    { 0xA679, "STASL", 0x08, 1 },
    { 0xA689, "WSANA", 0x12, 3 },
    { 0xA699, "WSALA", 0x12, 3 },
    { 0xA6A8, "FSLT", 0x1F, 1 },
    { 0xA6A9, "WSANM", 0x12, 3 },
    { 0xA6B9, "WSALM", 0x12, 3 },
    { 0xA6C9, "LCALL", 0x16, 4 },
    { 0xA6D9, "LJMP", 0x18, 3 },
    { 0xA6E8, "FLST", 0x07, 2 },
    { 0xA6E9, "LJSR", 0x18, 3 },
    { 0xA6F8, "XCT", 0x08, 1 },
    { 0xA6F9, "LPEF", 0x18, 3 },
    { 0xA709, "WXOP", 0x25, 1 },
    { 0xA719, "WWCS", 0x05, 2 },
    { 0xA729, "WSAVR", 0x05, 2 },
    { 0xA739, "WSAVS", 0x05, 2 },
    { 0xA749, "WCMT", 0x1E, 1 },
    { 0xA759, "WCMP", 0x1E, 1 },
    { 0xA769, "WEDIT", 0x1E, 1 },
    { 0xA779, "FXTD", 0x1E, 1 },
    { 0xA789, "WFPOP", 0x1E, 1 },
    { 0xA799, "LPSR", 0x1E, 1 },
    { 0xA7A8, "STI", 0x08, 1 },
    { 0xA7A9, "SPSR", 0x1E, 1 },
    { 0xA7B9, "SNOVR", 0x1F, 1 },
    { 0xA7C8, "SAVZ", 0x1E, 1 },
    { 0xA7C9, "CRYTO", 0x1E, 1 },
    { 0xA7D9, "CRYTZ", 0x1E, 1 },
    { 0xA7E8, "FPLYD", 0x1E, 1 },
    { 0xA7E9, "CRYTC", 0x1E, 1 },
    { 0xA7F8, "XORI", 0x0B, 2 },
    { 0xA7F9, "WLMP", 0x1E, 1 },
    { 0xAEA8, "FSGE", 0x1F, 1 },
    { 0xAF08, "LCSF", 0x1E, 1 },
    { 0xAF48, "SCL", 0x1E, 1 },
    { 0xAFC8, "RTN", 0x1E, 1 },
    { 0xAFE8, "FEXPS", 0x1E, 1 },
    { 0xB6A8, "FSLE", 0x1F, 1 },
    { 0xB7C8, "BLM", 0x1E, 1 },
    { 0xB7E8, "FSIND", 0x1E, 1 },
    { 0xBEA8, "FSGT", 0x1F, 1 },
    { 0xBFC8, "DIVX", 0x1E, 1 },
    { 0xBFE8, "FSINS", 0x1E, 1 },
    { 0xC438, "ESTA", 0x04, 2 },
    { 0xC478, "DSPA", 0x04, 2 },
    { 0xC609, "XJMP", 0x2B, 2 },
    { 0xC619, "XJSR", 0x2B, 2 },
    { 0xC628, "FAB", 0x08, 1 },
    { 0xC629, "NLDAI", 0x0B, 2 },
    { 0xC639, "NADDI", 0x0B, 2 },
    { 0xC649, "LDASB", 0x08, 1 },
    { 0xC659, "STASB", 0x08, 1 },
    { 0xC668, "FINT", 0x08, 1 },
    { 0xC669, "LDAFP", 0x08, 1 },
    { 0xC679, "STAFP", 0x08, 1 },
    { 0xC689, "WLDAI", 0x11, 3 },
    { 0xC699, "WUGTI", 0x12, 3 },
    { 0xC6A8, "FSNM", 0x1F, 1 },
    { 0xC6A9, "WASHI", 0x0B, 2 },
    { 0xC6B9, "WULEI", 0x12, 3 },
    { 0xC6C9, "LPSHJ", 0x18, 3 },
    { 0xC6D9, "LFLST", 0x18, 3 },
    { 0xC6E8, "FTE", 0x1E, 1 },
    { 0xC6E9, "LFSST", 0x18, 3 },
    { 0xC6F8, "HLV", 0x08, 1 },
    { 0xC6F9, "LPEFB", 0x14, 3 },
    { 0xC709, "XVCT", 0x05, 2 },
    { 0xC719, "QSCAN", 0x05, 2 },
    { 0xC749, "FXTE", 0x1E, 1 },
    { 0xC759, "WLDIX", 0x1E, 1 },
    { 0xC769, "WSTIX", 0x1E, 1 },
    { 0xC779, "WLSN", 0x1E, 1 },
    { 0xC789, "BKPT", 0x1E, 1 },
    { 0xC799, "VBP", 0x1F, 1 },
    { 0xC7A8, "LDIX", 0x1E, 1 },
    { 0xC7A9, "VWP", 0x1F, 1 },
    { 0xC7B9, "LSBRA", 0x1E, 1 },
    { 0xC7C8, "MUL", 0x1E, 1 },
    { 0xC7C9, "ISZTS", 0x1F, 1 },
    { 0xC7D9, "DSZTS", 0x1F, 1 },
    { 0xC7E8, "FPLYS", 0x1E, 1 },
    { 0xC7E9, "ENQH", 0x1F, 1 },
    { 0xC7F8, "ANDI", 0x0B, 2 },
    { 0xC7F9, "ENQT", 0x1F, 1 },
    { 0xCEA8, "FSND", 0x1F, 1 },
    { 0xCEE8, "FTD", 0x1E, 1 },
    { 0xCFA8, "STIX", 0x1E, 1 },
    { 0xCFC8, "MULS", 0x1E, 1 },
    { 0xCFE8, "FEXPD", 0x1E, 1 },
    { 0xD6A8, "FSNU", 0x1F, 1 },
    { 0xD6E8, "FCLE", 0x1E, 1 },
    { 0xD7A8, "CMV", 0x1E, 1 },
    { 0xD7C8, "DIV", 0x1E, 1 },
    { 0xD7E8, "FCOSD", 0x1E, 1 },
    { 0xDEA8, "FSNUD", 0x1F, 1 },
    { 0xDFA8, "CMP", 0x1E, 1 },
    { 0xDFC8, "DIVS", 0x1E, 1 },
    { 0xDFE8, "FCOSS", 0x1E, 1 },
    { 0xE438, "ELEF", 0x04, 2 },
    { 0xE609, "NSALA", 0x0C, 2 },
    { 0xE619, "NSALM", 0x0C, 2 },
    { 0xE628, "FNEG", 0x08, 1 },
    { 0xE629, "NSANA", 0x0C, 2 },
    { 0xE639, "NSANM", 0x0C, 2 },
    { 0xE649, "WMSP", 0x08, 1 },
    { 0xE659, "WHLV", 0x08, 1 },
    { 0xE668, "FHLV", 0x08, 1 },
    { 0xE669, "CVWN", 0x08, 1 },
    { 0xE679, "WLDI", 0x08, 1 },
    { 0xE689, "WSGTI", 0x0C, 2 },
    { 0xE699, "WMOVR", 0x08, 1 },
    { 0xE6A8, "FSNO", 0x1F, 1 },
    { 0xE6A9, "WSLEI", 0x0C, 2 },
    { 0xE6B9, "WSTI", 0x08, 1 },
    { 0xE6C9, "WSEQI", 0x0C, 2 },
    { 0xE6D9, "WLSHI", 0x0B, 2 },
    { 0xE6E8, "FPSH", 0x1E, 1 },
    { 0xE6E9, "WSNEI", 0x0C, 2 },
    { 0xE6F9, "WNADI", 0x0B, 2 },
    { 0xE709, "WCST", 0x1E, 1 },
    { 0xE719, "WMESS", 0x1F, 1 },
    { 0xE729, "SPTE", 0x1F, 1 },
    { 0xE739, "LPTE", 0x1F, 1 },
    { 0xE749, "WBLM", 0x1E, 1 },
    { 0xE759, "WMULS", 0x1E, 1 },
    { 0xE769, "WDIVS", 0x1E, 1 },
    { 0xE779, "WPOPB", 0x1E, 1 },
    { 0xE789, "LSBRS", 0x1E, 1 },
    { 0xE799, "PATU", 0x1E, 1 },
    { 0xE7A8, "CTR", 0x1E, 1 },
    { 0xE7A9, "RRFB", 0x1E, 1 },
    { 0xE7B9, "ORFB", 0x1E, 1 },
    { 0xE7C8, "SAVE", 0x05, 2 },
    { 0xE7C9, "DEQUE", 0x1F, 1 },
    { 0xE7D9, "SSPT", 0x1E, 1 },
    { 0xE7E8, "FSQRD", 0x1E, 1 },
    { 0xE7F8, "ADDI", 0x0B, 2 },
    { 0xEEA8, "FSNOD", 0x1F, 1 },
    { 0xEEE8, "FPOP", 0x1E, 1 },
    { 0xEFA8, "CMT", 0x1E, 1 },
    { 0xEFC8, "RSTR", 0x1E, 1 },
    { 0xF6A8, "FSNUO", 0x1F, 1 },
    { 0xF7A8, "EDIT", 0x1E, 1 },
    { 0xFEA8, "FSNER", 0x1F, 1 },
    { 0xFFA8, "LSN", 0x1E, 1 },
    { 0xFFC8, "ECLID", 0x1E, 1 },
};
#define NMVOPS ((int)(sizeof mvops / sizeof mvops[0]))

static const mvop *mvfind(unsigned short ir)
{
    /* WBR first.  Its displacement occupies the two accumulator fields and
     * the shift field, so it needs a mask of its own (0x873F), and the carry
     * masks below would otherwise hand 80F8 to DSB, which shares the space
     * with a carry field of zero. */
    static const mvop wbr = { 0x8038, "WBR", 0x23, 1 };
    if ((ir & 0x843F) == 0x8038) return &wbr;

    /* WSKBO and WSKBZ carry a five-bit bit number split across bits 14-12
     * and 5-4, so they need their own mask too.  Without it 0xDF49 -- skip
     * on bit 20 -- collapses under 0xE7FF onto 0xC749, which is FXTE. */
    /* WCLM names the same accumulator twice when its two limits follow
     * inline as doublewords -- the same "against itself" convention the
     * skips use -- and is then five words long, not one.  Every instance in
     * the utilities that names one accumulator twice is followed by a
     * plausible pair: 61/7A for a-z, 30/39 for 0-9, C4/15A for the range of
     * system call numbers the PL/I runtime's own .SYSTM thunk intercepts. */
    {
        static const mvop clm1 = { 0x8569, "WCLM", 0x0F, 1 };
        static const mvop clm5 = { 0x8569, "WCLM", 0x0F, 5 };
        if ((ir & 0x87FF) == 0x8569)
            return ((ir >> 13) & 3) == ((ir >> 11) & 3) ? &clm5 : &clm1;
    }

    {
        static const mvop skbo = { 0x8F49, "WSKBO", 0x24, 1 };
        static const mvop skbz = { 0x8F89, "WSKBZ", 0x24, 1 };
        if ((ir & 0x8FCF) == 0x8F49) return &skbo;
        if ((ir & 0x8FCF) == 0x8F89) return &skbz;
    }

    /* Bits 5-4 are a carry field on the ALC-space instructions the MV
     * inherited, so a base that does not match with them intact matches once
     * they are masked out.  That accounts for 1594 of the 1597 extension
     * words in ZORK.PR that the four tighter masks leave unresolved. */
    static const unsigned short masks[8] = { 0xFFFF, 0xE7FF, 0x9FFF, 0x87FF,
                                             0xFFCF, 0xE7CF, 0x9FCF, 0x87CF };
    int m, i;
    for (m = 0; m < 8; m++) {
        unsigned short b = ir & masks[m];
        for (i = 0; i < NMVOPS; i++) if (mvops[i].op == b) return &mvops[i];
    }
    return 0;
}

/* Does this format type carry an accumulator in bits 12-11, putting the index
 * mode up in bits 14-13? */
/* Which field holds the index mode: bits 14-13 when the instruction also
 * names an accumulator, bits 12-11 when it does not.
 *
 * Types 14 and 27 are LPEFB and XPEFB, and they belong in the second group
 * even though the rest of their format class is in the first: like LPEF and
 * XPEF they push an address rather than load one into an accumulator, so
 * there is no accumulator field to displace the index.  ZORK.PR at 7D3B8
 * pushes the byte pointer to its heap header with
 *
 *     LPEFB 0,[0394]        ; 0394 is 01CA doubled
 *
 * and reading bits 14-13 as the index makes that 2, so the pointer comes out
 * measured from AC2 and the heap is read into the middle of the shared
 * area instead of into the header buffer the version check reads. */
static int mv_idx_hi(unsigned char type)
{
    switch (type) {
    case 0x13: case 0x15: case 0x1A:
    case 0x26: case 0x28: case 0x2E: return 1;
    default: return 0;
    }
}

#endif
