/* fp4.h - a Four-Phase Systems IV/70 (the base instruction set of the
 * IV/90, sold by Philips as the P7000) with the devices IDOS needs.
 *
 * Words are 24 bits; bit 0 is the most significant.  Octal throughout. */
#ifndef FP4_H
#define FP4_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t word;

#define W24  077777777u
#define A15  077777u
#define SIGN 040000000u

#define MEMWORDS 0100000u               /* 32K words */

extern word mem[MEMWORDS];
extern word reg[8];                     /* R0 R1 RP RA RB X1 X2 X3 */
#define RP reg[2]
#define RA reg[3]
#define RB reg[4]
#define X1 reg[5]
#define X2 reg[6]
#define X3 reg[7]
extern int cc_o, cc_z, cc_m, cc_c;
extern int cpu_model;                   /* 70: IV/70 (IDOS CPU type 0) */
extern unsigned long long icount;
extern word console_keys;

/* why the run stopped */
extern int stop_code;
extern char stop_text[256];
enum { STOP_NONE, STOP_HALT, STOP_ILLEGAL, STOP_IO, STOP_LIMIT, STOP_USER };
void stop(int code, const char *fmt, ...);

/* cpu.c */
void cpu_reset(void);
void cpu_step(void);                    /* one instruction, or one interrupt */
void cpu_exec(word ir, int intr);
extern int inhibit;                     /* no interrupt before the next instruction */
int cpu_idle(void);                     /* spinning in a short loop (waiting) */
extern word last_pc;

/* the screen a front end follows (IDOS: 24 lines of 32 words at 0140), and
 * a hook called just before a scroll moves its line 1 onto line 0 */
extern word screen_base;
extern void (*scroll_hook)(void);

/* io.c - the interrupt system and the devices */
void io_reset(void);
void io_tick(void);                     /* device timers; called per instruction */
int  irq_pending(void);                 /* the level to take, or -1 */
int  irq_take(int level, word *ir);     /* the instruction to run; returns the unit */
void irq_raise(int level, int unit);
void irq_arm(word mask, int on);
void irq_reset_levels(word mask);
void irq_debreak(void);
void irq_mark_active(int level);
void io_exec(word ea, int bytes);       /* IO / IOB */
void io_boot(word select);              /* BOOT */
void io_exct(word v);
int  io_exsn(word v);

/* the 60 Hz clock: by default one tick per 8000 instructions (repeatable);
 * in real time it follows the host's clock */
void clock_set_realtime(int on);

/* the disc image; with WRITABLE every write goes to the file at once */
int disc_open(const char *path, int writable);
void disc_close(void);
int disc_busy(void);                    /* a seek or transfer is under way */
word disc_peek(long w);                 /* word W of the pack (256 a sector) */
void disc_copy(long from, long to, long count);     /* sectors, from outside */

/* the keyboards: characters typed at keyboard KB */
void kbd_type(int kb, const char *s, int n);
int kbd_pending(int kb);
int kbd_waiting(void);                  /* the program is waiting for a key */
extern unsigned long long kbd_gap, kbd_start;
extern word kbd_word;                   /* the program's key word (learned) */
extern int kbd_learn;
extern unsigned long long kbd_polled;

/* the trace */
extern FILE *trace_fp;
extern unsigned long long trace_from;
void trace_insn(word pc, word ir);
void trace_dump_ring(FILE *f, int n);
const char *disasm(word ir, char *buf);

#endif
