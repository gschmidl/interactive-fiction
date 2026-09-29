/*
 * nd100.h - ND-100 user-mode CPU for running SINTRAN III :PROG images.
 *
 * Written for the Skattejakt port.  Instruction semantics follow
 * ND-06.029.1 (ND-110 Instruction Set) and ND-06.014 (ND-100 Reference
 * Manual); where those are unclear, the microcode-validated behaviour
 * documented in nd100x was used as the reference (read, not copied).
 */
#ifndef ND100_H
#define ND100_H

#include <stdint.h>
#include <stdio.h>

/* register numbers as used in <sr>/<dr> fields */
enum { R_STS = 0, R_D = 1, R_P = 2, R_B = 3, R_L = 4, R_A = 5, R_T = 6, R_X = 7 };

/* STS low byte */
#define S_PTM 0x01
#define S_TG  0x02   /* floating rounding */
#define S_K   0x04   /* one-bit accumulator */
#define S_Z   0x08   /* error indicator */
#define S_Q   0x10   /* dynamic overflow */
#define S_O   0x20   /* static overflow */
#define S_C   0x40   /* carry */
#define S_M   0x80   /* multishift link */

/* why step() stopped */
enum {
    TRAP_NONE = 0,
    TRAP_MON,        /* monitor call; cpu.mon holds the number, P is past it */
    TRAP_ILLEGAL,    /* undefined instruction */
    TRAP_PRIV,       /* privileged instruction in a user program */
    TRAP_UNIMPL,     /* defined instruction this emulator does not do */
    TRAP_EXR_EXR     /* EXR of an EXR */
};

typedef struct {
    uint16_t r[8];
    uint16_t mem[65536];
    uint16_t mon;         /* number of the last MON */
    uint16_t trap_pc;     /* address of the trapping instruction */
    uint16_t trap_word;
    uint64_t icount;
    int      zset;        /* Z went from 0 to 1 during the last step */
} Cpu;

int  cpu_step(Cpu *c);
const char *cpu_dis(uint16_t w, uint16_t addr, char *buf);

/* 48-bit floating point helpers (fpu.c) */
void fp_add(uint16_t *a, const uint16_t *b);
void fp_sub(uint16_t *a, const uint16_t *b);
void fp_mul(uint16_t *a, const uint16_t *b);
int  fp_div(uint16_t *a, const uint16_t *b);   /* returns 1 on divide by zero */
void fp_nlz(Cpu *c, int scale);
void fp_dnz(Cpu *c, int scale);

#endif
