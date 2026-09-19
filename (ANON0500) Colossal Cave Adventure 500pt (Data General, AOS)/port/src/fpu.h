/* fpu.h -- Data General Eclipse floating point.
 *
 * Opcodes are DG's own, transcribed from EBID.SR (:UTIL).  The operand
 * encoding is not spelled out there, so it was read back out of the code
 * ADVENTURE.PR actually executes:
 *
 *   6FA0  E7C8 000D   SAVE 13
 *   6FA2  8428 3420   FLDS 0,[3420]      index bits 14-13 = 0, absolute
 *   6FA4  E628        FNEG 0             FPAC in bits 12-11
 *   6FA6  E4A8 0008   FSTS 0,8,AC3       index 3 = AC3, i.e. a local
 *   ...
 *   6FB1  C428 0000   FLDS 0,0,AC2       index 2 = AC2, just computed above
 *   6FB3  8C28 340A   FLDS 1,[340A]
 *   6FB5  A728        FCMP 1,0
 *   6FB6  A6A8        FSLT
 *
 * so for the floating memory references the index mode is bits 14-13 and the
 * FPAC bits 12-11 -- the opposite way round from ELDA/ESTA, whose index is in
 * bits 9-8.  It has to be: bit 9 is part of the opcode here (FAMS 0101050 vs
 * FLDS 0102050), and reading 6FB1 the other way makes it load from absolute
 * address 0 while throwing away the AC2 the two instructions before it just
 * computed.  For the register forms (FAS, FCMP, FMOV ...) the fields are the
 * ordinary ACS/ACD pair, bits 14-13 and 12-11.
 *
 * Format: sign in bit 63, a 7-bit excess-64 exponent to base 16 in bits
 * 62-56, and a hexadecimal-normalised fraction below that -- 56 bits for a
 * double, the top 24 for a single.  Value = 0.fraction * 16^(exp-64), and a
 * fraction of zero is zero whatever the exponent says.
 */
#ifndef FPU_H
#define FPU_H

/* fixed, no operand field */
#define F_FNS   0x86A8u
#define F_FSA   0x8EA8u
#define F_FSEQ  0x96A8u
#define F_FSNE  0x9EA8u
#define F_FSLT  0xA6A8u
#define F_FSGE  0xAEA8u
#define F_FSLE  0xB6A8u
#define F_FSGT  0xBEA8u
#define F_FSNM  0xC6A8u
#define F_FSND  0xCEA8u
#define F_FSNU  0xD6A8u
#define F_FSNUD 0xDEA8u
#define F_FSNO  0xE6A8u
#define F_FSNOD 0xEEA8u
#define F_FSNUO 0xF6A8u
#define F_FSNER 0xFEA8u
#define F_FSST  0x86E8u
#define F_FLST  0xA6E8u
#define F_FTE   0xC6E8u
#define F_FTD   0xCEE8u
#define F_FCLE  0xD6E8u
#define F_FPSH  0xE6E8u
#define F_FPOP  0xEEE8u

/* one FPAC in bits 12-11 (mask 0xE7FF) */
#define F_MASK1 0xE7FFu
#define F_FNOM  0x8628u
#define F_FRH   0xA628u
#define F_FAB   0xC628u
#define F_FNEG  0xE628u
#define F_FSCAL 0x8668u
#define F_FEXP  0xA668u
#define F_FINT  0xC668u
#define F_FHLV  0xE668u

/* two fields in bits 14-13 / 12-11 (mask 0x87FF) */
#define F_MASK2 0x87FFu
/* ... memory forms: index, FPAC, then a displacement word */
#define F_FAMS  0x8228u
#define F_FAMD  0x8268u
#define F_FSMS  0x82A8u
#define F_FSMD  0x82E8u
#define F_FMMS  0x8328u
#define F_FMMD  0x8368u
#define F_FDMS  0x83A8u
#define F_FDMD  0x83E8u
#define F_FLDS  0x8428u
#define F_FLDD  0x8468u
#define F_FSTS  0x84A8u
#define F_FSTD  0x84E8u
#define F_FLMD  0x8568u
#define F_FFMD  0x85E8u
/* ... register forms: ACS, ACD */
#define F_FAS   0x8028u
#define F_FAD   0x8068u
#define F_FSS   0x80A8u
#define F_FSD   0x80E8u
#define F_FMS   0x8128u
#define F_FMD   0x8168u
#define F_FDS   0x81A8u
#define F_FDD   0x81E8u
#define F_FLAS  0x8528u
#define F_FFAS  0x85A8u
#define F_FCMP  0x8728u
#define F_FMOV  0x8768u

#endif /* FPU_H */
