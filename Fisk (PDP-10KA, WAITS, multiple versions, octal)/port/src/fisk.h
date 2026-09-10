/* FisK - PDP-10 (WAITS) core-image interpreter */
#ifndef FISK_H
#define FISK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long word;   /* 36 significant bits          */
typedef long long sword;           /* sign-extended 36-bit value   */

#define WORDM  0777777777777ULL    /* 36 ones                      */
#define SIGN   0400000000000ULL    /* bit 0                        */
#define RMASK  0777777ULL          /* right half                   */
#define LMASK  0777777000000ULL    /* left half                    */
#define MEMSIZ (1 << 18)

extern word mem[MEMSIZ];
extern int  pc;
extern int  flags;                 /* PC flags, bits 0-12 as in LH */

/* PC flag bits, positioned as they sit in the left half of a stored PC */
#define F_OV    0400000
#define F_CRY0  0200000
#define F_CRY1  0100000
#define F_FOV   0040000
#define F_FPD   0020000
#define F_USER  0010000
#define F_UIO   0004000
#define F_PUB   0002000
#define F_AFI   0001000
#define F_TRP2  0000400
#define F_TRP1  0000200
#define F_FXU   0000100
#define F_DCK   0000040

/* generated images ------------------------------------------------------ */
typedef struct {
    const char *name;
    const unsigned long long *low;   int low_base,  low_len;
    const unsigned long long *high;  int high_base, high_len;
    const unsigned char      *txt;   int txt_len;
} Image;

extern const Image fisk_images[];
extern const int   fisk_nimages;
extern const Image *image;         /* the one selected on the command line  */

/* cpu ------------------------------------------------------------------- */
void cpu_reset(void);
int  cpu_run(void);                /* returns exit code                    */
extern int trace;
extern long long icount;

/* monitor --------------------------------------------------------------- */
int  uuo(int op, int ac, int e);   /* 0 = no skip, 1 = skip, 2 = exit      */
void mon_init(void);
void mon_exit(int code);
void mon_flush(void);
extern int exit_code, exiting, montrace;

/* terminal -------------------------------------------------------------- */
void tty_out(int c);
void tty_flush(void);
void tty_peek(void);
int  tty_in(int wait);             /* -1 when nothing buffered             */

#endif
