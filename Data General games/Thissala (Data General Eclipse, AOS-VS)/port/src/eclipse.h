/* eclipse.h -- Data General Eclipse (16-bit) decode tables.
 *
 * Shared by the disassembler and the emulator.  DG numbers instruction bits
 * 0..15 from the MSB; everything here uses ordinary C masks instead.
 *
 * Word-addressed machine, 32K words.  Byte pointers are (word << 1) | half,
 * where half 0 is the HIGH byte of the word.
 */
#ifndef ECLIPSE_H
#define ECLIPSE_H

#include <stdint.h>

typedef uint16_t word;

#define MEMWORDS  0x8000u        /* 32K words of address space            */
#define AMASK     0x7FFFu        /* address mask                          */

/* ---- memory-reference (bit15 == 0) ------------------------------------ */
#define MR_OP(ir)     ((ir) >> 11)          /* 0 JMP 1 JSR 2 ISZ 3 DSZ
                                               4-7 LDA ac  8-11 STA ac
                                               12-15 I/O                   */
#define MR_IND(ir)    ((ir) & 0x0400u)      /* indirect                    */
#define MR_IDX(ir)    (((ir) >> 8) & 3u)    /* 0 page-zero 1 PC 2 AC2 3 AC3*/
#define MR_DISP(ir)   ((ir) & 0x00FFu)

/* ---- arithmetic/logic (bit15 == 1) ------------------------------------ */
#define ALC_ACS(ir)   (((ir) >> 13) & 3u)
#define ALC_ACD(ir)   (((ir) >> 11) & 3u)
#define ALC_OP(ir)    (((ir) >> 8)  & 7u)   /* COM NEG MOV INC ADC SUB ADD AND */
#define ALC_SH(ir)    (((ir) >> 6)  & 3u)   /* -- L R S                    */
#define ALC_CY(ir)    (((ir) >> 4)  & 3u)   /* -- Z O C                    */
#define ALC_NL(ir)    ((ir) & 0x0008u)      /* no-load ('#')               */
#define ALC_SKIP(ir)  ((ir) & 0x0007u)

/* An ALC with no-load set and no skip is a no-op on a NOVA; Eclipse reused
 * that hole for its extended instructions.                                */
#define IS_ECLIPSE_EXT(ir) (((ir) & 0x8000u) && ALC_NL(ir) && !ALC_SKIP(ir))

/* ---- Eclipse two-word memory reference --------------------------------
 * Second word is the displacement.  AC in bits 12-11, index in bits 9-8.
 * Masks/matches transcribed from simh NOVA/eclipse_cpu.c.                  */
#define E2_MASK_AC    0xE4FFu    /* instructions that carry an accumulator */
#define E2_ELDA       0xA438u
#define E2_ESTA       0xC438u
#define E2_ELEF       0xE438u
#define E2_ELDB       0x8478u
#define E2_ESTB       0xA478u
#define E2_MASK_NOAC  0xFCFFu    /* no accumulator field                   */
#define E2_EJMP       0x8438u
#define E2_EJSR       0x8C38u
#define E2_EISZ       0x9438u
#define E2_EDSZ       0x9C38u

/* ---- Eclipse one-word extended (mask 0x87FF, ACS/ACD masked out) ------ */
#define E1_MASK       0x87FFu
#define E1_ADI        0x8008u
#define E1_SBI        0x8048u
#define E1_DAD        0x8088u
#define E1_DSB        0x80C8u
#define E1_IOR        0x8108u
#define E1_XOR        0x8148u
#define E1_ANC        0x8188u
#define E1_XCH        0x81C8u
#define E1_SGT        0x8208u
#define E1_SGE        0x8248u
#define E1_LSH        0x8288u
#define E1_DLSH       0x82C8u
#define E1_HXL        0x8308u
#define E1_HXR        0x8348u
#define E1_DHXL       0x8388u
#define E1_DHXR       0x83C8u
#define E1_BTO        0x8408u
#define E1_BTZ        0x8448u
#define E1_SZB        0x8488u
#define E1_SZBO       0x84C8u
#define E1_CLM        0x84F8u
#define E1_LOB        0x8508u
#define E1_LRB        0x8548u
#define E1_COB        0x8588u
#define E1_LDB        0x85C8u
#define E1_SNB        0x85F8u
#define E1_STB        0x8608u
#define E1_PSH        0x8648u
#define E1_POP        0x8688u
#define E1_MSP_MASK   0xE7FFu
#define E1_MSP        0x86F8u    /* 0103370 modify stack pointer          */
#define E1_XCT        0xA6F8u    /* 0123370 execute                       */
#define E1_HLV        0xC6F8u    /* 0143370 halve                         */

/* Fixed full-word opcodes (no operand fields at all). */
#define E1_MUL        0xC7C8u    /* 0143710 */
#define E1_MULS       0xCFC8u    /* 0147710 */
#define E1_DIV        0xD7C8u    /* 0153710 */
#define E1_DIVS       0xDFC8u    /* 0157710 */

/* SAVE is TWO words: the opcode then the frame size.  Every runtime routine
 * reached through the page-zero vector table starts with it, and missing its
 * second word is what desynchronises a naive linear disassembly. */
#define E1_SAVE       0xE7C8u    /* 0163710  save regs + allocate frame  */
#define E1_RTN        0xAFC8u    /* 0127710  return, restore frame       */
#define E1_POPJ       0x9FC8u    /* 0117710  pop PC and jump             */
#define E1_POPB       0x8FC8u    /* 0107710  pop PC,flags,AC3..AC0       */
#define E1_PSHR       0x87C8u    /* 0103710  push return address         */
#define E1_RSTR       0xEFC8u    /* 0167710  restore full context        */
#define E1_BAM        0x97C8u    /* 0113710  block add and move          */
#define E1_BLM        0xB7C8u    /* 0133710  block move                  */
#define E1_DIVX       0xBFC8u    /* 0137710  sign-extend and divide      */
/* Floating-point control words that carry no operand fields.  Taken from
 * EBID.SR, Data General's own Eclipse instruction definitions. */
#define E1_FCLE       0xD6E8u    /* 0153350  clear FP errors             */
#define E1_FTE        0xC6E8u    /* 0143350  FP trap enable              */
#define E1_FTD        0xCEE8u    /* 0147350  FP trap disable             */
#define E1_FPSH       0xE6E8u    /* 0163350  push FP state               */
#define E1_FPOP       0xEEE8u    /* 0167350  pop FP state                */
#define E1_FNS        0x86A8u    /* 0103250  no skip                     */

/* Two-word immediate family: bits 14-13 opcode, bits 12-11 destination AC,
 * second word a 16-bit immediate.  From simh eclipse_cpu.c. */
#define EI_MASK       0xE7FFu    /* 0163777 */
#define EI_IORI       0x87F8u    /* 0103770 */
#define EI_XORI       0xA7F8u    /* 0123770 */
#define EI_ANDI       0xC7F8u    /* 0143770 */
#define EI_ADDI       0xE7F8u    /* 0163770 */

/* Sign-extend an 8-bit displacement. */
static inline int disp8(word ir) {
    int d = ir & 0xFF;
    return (d & 0x80) ? d - 256 : d;
}

/* Effective address for a memory-reference instruction (no indirection). */
static inline word ea_mr(word ir, word pc) {
    switch (MR_IDX(ir)) {
    case 0:  return (word)(MR_DISP(ir));                 /* page zero      */
    case 1:  return (word)((pc + disp8(ir)) & AMASK);    /* PC-relative    */
    default: return 0;                                   /* AC2/AC3: caller*/
    }
}

#endif /* ECLIPSE_H */
