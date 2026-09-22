/* fp4.h - a Four-Phase Systems IV/90 Model 2 (sold by Philips as the P7000)
 * with the devices MFE and QUEST need: the IV/70 instruction set, the IV/90
 * memory mapper and instructions, the 8231 disc, the 7200 terminals.
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

#define MEMWORDS 0100000u               /* 32K words: the logical address space */
#define PHYSWORDS 01000000u             /* 256 pages of 1K: the IV/90 physical memory */

extern word mem[PHYSWORDS];             /* physical memory (IV/70: the first 32K) */

/* the IV/90 memory mapper: 256 windows of 32 logical pages of 1K words.  An
 * entry (bits 14-23 of the MAP format): P = odd parity (01000), RO = read-only
 * (0400), the physical page (0377).  cpu_model 70 does not map. */
extern word mapper[256 * 32];
extern int cur_win;                     /* the window register */
extern int phys_pages;                  /* memory present, in 1K pages */
word vrd(word a);                       /* logical address A in the current window */
void vwr(word a, word v);
word wrd(int w, word a);                /* ... in window W (< 0: the current one) */
void wwr(int w, word a, word v);
extern int io_cross;                    /* IOXW: the buffer word names its window */
extern word reg[8];                     /* R0 R1 RP RA RB X1 X2 X3 */
#define RP reg[2]
#define RA reg[3]
#define RB reg[4]
#define X1 reg[5]
#define X2 reg[6]
#define X3 reg[7]
extern int cc_o, cc_z, cc_m, cc_c;
extern int cpu_model;                   /* 70: IV/70 (IDOS CPU type 0); 90: IV/90 (type 2) */
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
/* a loop known to wait for work (window IDLE_WIN, IDLE_LO..IDLE_HI): the
 * instructions run there are counted in IDLE_HITS */
extern int idle_win;
extern word idle_lo, idle_hi;
extern unsigned long idle_hits;
/* called just before an MVEL writes into lines 0-22 of a terminal's screen:
 * PAGE is the screen's physical page, which is the terminal's number, and
 * SCROLL says the move is line 1 onto line 0 (a scroll up) */
extern void (*screen_hook)(int page, int scroll);

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
 * in real time it follows the host's clock.  clock_ticks counts the ticks
 * given to the machine. */
void clock_set_realtime(int on);
extern unsigned long long clock_ticks;
unsigned long long clock_due(void);     /* the ticks due by now */

/* the disc image, kept in memory; with WRITABLE it is written back at the end */
int disc_open(const char *path, int writable);
void disc_close(void);
int disc_busy(void);                    /* a seek or transfer is under way */
word disc_peek(long w);                 /* word W of the pack (256 a sector) */
void disc_poke(long w, word v);

/* the keyboards: characters typed at keyboard KB */
void kbd_type(int kb, const char *s, int n);
int kbd_pending(int kb);
extern unsigned long long kbd_gap, kbd_start;
extern int kbd_free;                    /* type at kbd_gap intervals, ungated (MFE) */
extern int kbd_beeps[32];               /* alarms sounded at each terminal */
extern word kbd_word;                   /* the program's key word (learned) */
extern int kbd_learn;
extern unsigned long long kbd_polled;
int kbd_waiting(void);                  /* an IDOS program is waiting for a key */

/* the trace */
extern FILE *trace_fp;
extern unsigned long long trace_from;
extern int trace_win;                   /* trace only this window (-1: all) */
void trace_insn(word pc, word ir);
void trace_dump_ring(FILE *f, int n);
const char *disasm(word ir, char *buf);

#endif
