/*
**      SM_DEF.H -- Definitions for Screen Manager
**      psl 12/80
** @(#)sm_def.h	1.8 10/17/83 -- PSL misc
*/

/* tokens for sm_spcl() calls */
#define SM_CLEAR    0		/* clear whole screen */
#define SM_ERASE    1           /* erase window (needs FLUSH) */
#define SM_REFRESH  2           /* redraw, assuming garbage */
#define SM_FLUSH    3           /* output buffered changes */
#define SM_CTLMAP   4           /* map control characters */
#define SM_REDRAW   5           /* redraw, assuming blanks */
#define SM_FINI     6           /* finished, free buffer & clean-up */
#define SM_REUSE    7           /* reuse buffer, new fh & type */

struct  smstr   {
	char    sm_type;        /* terminal type */
	char    sm_fh;          /* where to send output */
	char    sm_ospeed;      /* output speed (for padding) */
	char    sm_x0, sm_y0;   /* upper left corner */
	char    sm_sw, sm_sh;   /* window width & height */
	char    **sm_cntab;     /* control char map table */
	char    sm_lox, sm_hix; /* low & high changed cols */
	char    sm_loy, sm_hiy; /* low & high changed lines */
	char    *sm_oscrn;      /* old screen image */
	char    *sm_nscrn;      /* new screen image */
};

extern  struct  smstr   *sm_char();
extern  struct  smstr   *sm_init();
extern  struct  smstr   *sm_spcl();
extern  struct  smstr   *sm_str();
