/* Minimal but complete Z80 core for the Wang OIS 928 workstation emulation. */
#ifndef Z80_H
#define Z80_H
#include <stdint.h>

typedef struct {
    uint8_t  a, f, b, c, d, e, h, l;
    uint8_t  a_, f_, b_, c_, d_, e_, h_, l_;
    uint16_t ix, iy, sp, pc;
    uint8_t  i, r;
    uint8_t  iff1, iff2, im;
    uint8_t  halted;
    uint64_t cycles;
} Z80;

/* Provided by the machine layer. */
uint8_t  mem_rd(uint16_t addr);
void     mem_wr(uint16_t addr, uint8_t v);
uint8_t  io_in(uint16_t port);
void     io_out(uint16_t port, uint8_t v);

void z80_reset(Z80 *z);
void z80_step(Z80 *z);
/* Request a mode-0 interrupt whose bus opcode is RST 00h. */
int  z80_interrupt(Z80 *z);

#define FLAG_C 0x01
#define FLAG_N 0x02
#define FLAG_P 0x04
#define FLAG_H 0x10
#define FLAG_Z 0x40
#define FLAG_S 0x80

#endif
