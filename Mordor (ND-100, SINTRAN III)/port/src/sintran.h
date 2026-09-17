/* sintran.h - SINTRAN III monitor calls for a background program */
#ifndef SINTRAN_H
#define SINTRAN_H

#include <stdio.h>
#include <time.h>
#include "nd100.h"

enum {
    SIN_RUN = 0,       /* carry on */
    SIN_EXIT,          /* MON 0 */
    SIN_BREAK,         /* the user pressed ESC */
    SIN_EOF,           /* terminal input ran out */
    SIN_FATAL          /* unimplemented call or a call that terminates */
};

#define SIN_MAXFILES 18     /* SINTRAN allows 18 open files */
#define SIN_FIRSTFILE 0100  /* file numbers handed out by OPEN start at 100B */

typedef struct {
    FILE    *f;
    int      access;        /* OPEN access code */
    int      blocksize;     /* in words; 256 until SETBS */
    long     next_block;    /* for block number -1 */
    char     name[80];      /* as the program gave it */
    int      writable;      /* opened with a write access code */
    int      last_op;       /* 'r' or 'w': stdio needs a seek between the two */
} SinFile;

typedef struct {
    time_t fixed_clock;     /* nonzero: the host time the program sees (-Z) */
    int    year_shift;      /* years subtracted from the host date */
    int    verbose;         /* log monitor calls on stderr */
    int    no_hold;         /* HOLD returns at once */
    int    allow_write;     /* files may be opened for writing */
    int    sintran_echo;    /* SINTRAN echoes the terminal (the FORTRAN runtimes echo for themselves) */
    char   command_rest[80];/* what device 0 still holds of the command line (its CR) */
    int    command_rest_len;
    char   typeahead[16];   /* typed at the terminal before the program started */
    int    typeahead_len;
    int    escape_enabled;
    int    echo_strategy;
    int    break_strategy;
    int    line_edit;       /* hold a line being typed in the port, to rub it out */
    char   line[128];       /* the line held: line_at of line_len handed over */
    int    line_len;
    int    line_at;
    uint16_t terminal_type; /* what GetTerminalType answers for the user's terminal */
    uint16_t exit_pc;       /* address of the MON that stopped the program */
    const char *data_dir;   /* where the user's files are: (USER)NAME:TYPE -> NAME.TYPE */
    SinFile files[SIN_MAXFILES];
    char   out_tail[256];   /* terminal output since the last line feed */
    int    out_tail_len;
    /* called with each character typed at the terminal before the program
       gets it; nonzero means the hook has dealt with it and replaced the
       machine state, so the monitor call must not touch the registers */
    int  (*input_hook)(Cpu *c, int ch);
} Sintran;

extern Sintran snt;

void sintran_init(void);
int  sintran_mon(Cpu *c, int n);
void sintran_close_all(void);
/* open a file again as it was: slot index, SINTRAN name, access, block size, position */
int  sintran_reopen(int index, const char *name, int access, int blocksize, long next_block);

#endif
