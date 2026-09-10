/* pdp10.h -- 36-bit DECsystem-10 CPU emulation.
 *
 * Taken unchanged from the EXPLOR port and extended for LANDS OF ZARAST:
 * the TBA (Tymshare BASIC) runtime needs double-precision floating point,
 * the double-word moves, FSC, IBP and MAP, and real disk files.
 */
#ifndef PDP10_H
#define PDP10_H

#include <stdint.h>
#include <stdio.h>

typedef uint64_t w36;

#define WMASK   0777777777777ULL      /* 36 bits                      */
#define SIGNBIT 0400000000000ULL      /* bit 0                        */
#define HMASK   0777777               /* one 18-bit halfword          */
#define MEMSIZE 0400000               /* 18-bit word address space... */
#define MEMTOP  01000000              /* ...but a hi seg lives up top */

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
extern long long insn_count;
extern int trace;

/* Memory access (ACs 0..17 are just low core, as on real hardware). */
#define AC(n)   M[(n) & 017]

void  cpu_reset(void);
void  cpu_run(void);
int   cpu_step(void);                 /* returns 0 to keep going      */
int   effective_address(w36 inst);

/* Supplied by monitor.c */
int   monitor_uuo(w36 inst, int op, int ac, int e);
void  monitor_init(void);
void  monitor_shutdown(void);

/* Supplied by images_NN.c -- the original .SHR tape files, embedded
 * verbatim, and the table of programs this build offers.  There is one
 * such file per version of the game; the emulator is the same for both. */
typedef struct { const char *name; const unsigned char *data; unsigned len;
                 unsigned hole; } shrfile_t;
typedef struct { const char *cmd; const char *img; const char *what; } progentry_t;
extern const shrfile_t   shrfiles[];
extern const progentry_t progs[];
extern const char       *port_banner;

/* Supplied by load.c */
int  load_shr(const char *name);      /* returns 0 on failure          */
extern int image_start;

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

#endif
