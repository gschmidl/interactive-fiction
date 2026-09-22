/* cpu990.h - a TI 990/10 processor in user (non-privileged) mode, enough to
 * run one DX10 task: the TMS 9900 instruction set, a flat 64K logical address
 * space, and XOP 15 (the DX10 supervisor call) handed to the host. */
#ifndef CPU990_H
#define CPU990_H

#include <stdint.h>

/* status register bits (bit 0 is the most significant) */
#define ST_LGT  0x8000
#define ST_AGT  0x4000
#define ST_EQ   0x2000
#define ST_C    0x1000
#define ST_OV   0x0800
#define ST_OP   0x0400
#define ST_X    0x0200

extern uint8_t mem[65536];
extern uint16_t cpu_wp, cpu_pc, cpu_st;
extern unsigned long long cpu_count;
extern unsigned long long cpu_watchdog;

/* the host's supervisor call: EA is the address of the call block.
 * A nonzero return stops cpu_run with that value. */
typedef int (*svc_fn)(uint16_t ea);
extern svc_fn cpu_svc;

/* why cpu_run stopped, when it returns CPU_FAULT */
extern char cpu_fault[128];
#define CPU_FAULT (-1)

int cpu_run(void);

static inline uint16_t rdw(uint16_t a)
{
    a &= 0xFFFE;
    return (uint16_t)(mem[a] << 8 | mem[a + 1]);
}

static inline void wrw(uint16_t a, uint16_t v)
{
    a &= 0xFFFE;
    mem[a] = (uint8_t)(v >> 8);
    mem[a + 1] = (uint8_t)v;
}

#endif
