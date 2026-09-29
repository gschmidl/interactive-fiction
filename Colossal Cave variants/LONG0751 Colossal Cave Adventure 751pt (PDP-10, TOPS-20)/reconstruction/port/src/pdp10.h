/* pdp10.h -- 36-bit DECsystem-10 CPU emulation for the ADVENTURE 751 port.
 *
 * The program being run is <GAMES>ADVENTURE.EXE from a TOPS-20 pack: a
 * FORTRAN-10 game that pulls the FORTRAN object time system in at 0400000
 * with GETSEG and then talks to the operating system two ways at once --
 * TOPS-10 UUOs through FOROTS, and a handful of TOPS-20 JSYSes of its own.
 * monitor.c answers the first, jsys.c the second.
 */
#ifndef PDP10_H
#define PDP10_H

#include <stdint.h>
#include <stdio.h>

typedef uint64_t w36;

#define WMASK   0777777777777ULL      /* 36 bits                      */
#define SIGNBIT 0400000000000ULL      /* bit 0                        */
#define HMASK   0777777               /* one 18-bit halfword          */
#define MEMTOP  01000000              /* the whole 18-bit address space */

/* Processor flags (kept in the left half of the "PC word"). */
#define F_AROV  0400000               /* arithmetic overflow          */
#define F_CRY0  0200000
#define F_CRY1  0100000
#define F_FOV   0040000               /* floating overflow            */
#define F_FPD   0020000
#define F_USER  0010000
#define F_UIO   0004000
#define F_TRAP2 0002000
#define F_TRAP1 0001000
#define F_FXU   0000100               /* floating underflow           */
#define F_DCK   0000040               /* divide check                 */

extern w36 M[MEMTOP];                 /* core                         */
extern int PC;
extern int FLAGS;
extern int halted;
extern int exit_code;
extern long long insn_count;
extern int trace;

/* Accumulators 0..17 are just low core, as on real hardware. */
#define AC(n)   M[(n) & 017]

void  cpu_reset(void);
void  cpu_run(void);
int   cpu_step(void);                 /* returns 0 to keep going      */
int   effective_address(w36 inst);

/* monitor.c -- the TOPS-10 side */
int   monitor_uuo(w36 inst, int op, int ac, int e);
void  monitor_init(void);
void  monitor_shutdown(void);
void  monitor_getseg(void);           /* map FOROTS at 0400000        */

/* jsys.c -- the TOPS-20 side */
int   do_jsys(int number);            /* returns instructions to skip */

/* Console, shared by both. */
void  tty_out(int c);
int   tty_in(void);
int   tty_avail(void);
void  tty_flush(void);
extern int tty_eof;

/* image.c (generated) */
extern const w36 image_data[];
extern const w36 forots_data[];
extern const int image_start;

/* data.c (generated) */
typedef struct { const char *name; const w36 *words; int nwords; } builtin_file_t;
extern const builtin_file_t builtin_files[];

/* Helpers shared with the monitor. */
static inline long long sx36(w36 w) {
    return (w & SIGNBIT) ? (long long)w - (1LL << 36) : (long long)w;
}
static inline int sx18(unsigned h) {
    return (h & 0400000) ? (int)h - 01000000 : (int)h;
}
#define LH(w) ((unsigned)(((w) >> 18) & HMASK))
#define RH(w) ((unsigned)((w) & HMASK))
#define XWD(l, r) ((((w36)(l) & HMASK) << 18) | ((w36)(r) & HMASK))

void fatal(const char *fmt, ...);
extern int opt_verbose;

#endif
