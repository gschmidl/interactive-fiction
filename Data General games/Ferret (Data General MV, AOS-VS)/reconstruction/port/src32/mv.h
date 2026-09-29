/* mv.h -- Data General ECLIPSE MV (32-bit) decode.
 *
 * A 32-bit AOS/VS program runs in ring 7: every address the program handles
 * is 0x7xxxxxxx, and the offset below that is a WORD address.  ZORK.PR and
 * FERRET.PR both end their shared half exactly at word 0x80000, so half a
 * megaword covers the whole space.
 *
 * The MV keeps the whole Eclipse instruction set and adds the "wide" ones in
 * the hole the Eclipse left: an ALC with the no-load bit set AND a skip field
 * of 1 does nothing useful on an Eclipse, so on the MV every instruction with
 * bit 15 set and a low nibble of 9 is a wide one.
 */
#ifndef MV_H
#define MV_H
#include <stdint.h>

typedef uint16_t word;
typedef uint32_t dword;

#define MEMWORDS   0x80000u        /* 512K words = the whole ring          */
#define RING       0x70000000u     /* ring 7, where AOS/VS runs user code  */
#define OFFMASK    0x0FFFFFFFu
#define MADDR(a)   ((a) & (MEMWORDS - 1))

/* A wide instruction: bit 15 set, low nibble 9. */
/* The whole extension space: bit 15 set with a low nibble of 8 (the Eclipse
 * extensions, which the MV kept and extended with its X and L forms) or 9
 * (the MV wide instructions).  Everything else with bit 15 set is a plain
 * ALC and everything without it a memory reference. */
#define IS_WIDE(ir)  (((ir) & 0x8000u) &&                       (((ir) & 0x000Fu) == 8u || ((ir) & 0x000Fu) == 9u))

/* System calls arrive as XJSR @6 followed by the call number. */
#define MV_XJSR     0xC619u

#endif /* MV_H */
