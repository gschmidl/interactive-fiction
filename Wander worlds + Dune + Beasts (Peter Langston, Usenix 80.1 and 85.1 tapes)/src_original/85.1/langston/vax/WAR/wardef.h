#define DEF_SCCS    "@(#)wardef.h	1.10 10/20/84 -- (c) psl 1976"
#include	"../lock.h"

#define MAX_X		20                          /* 2 larger for borders */
#define MAX_Y		20                          /* 2 larger for borders */
#define PARA_COST	4
#define SHELL_COST	8

#define C_TYPE		3
#define C_HOME0		1
#define C_HOME1		2
#define C_MTN		3

#define C_BAT		004
#define C_BAT_ANY	034
#define C_BAT_N		004
#define C_BAT_E		014
#define C_BAT_S		024
#define C_BAT_W		034

#define C_P0_SOON	000040
#define C_P1_SOON	000100
#define C_P0_NOW	000200
#define C_P1_NOW	000400
#define C_S0_SOON	001000
#define C_S1_SOON	002000
#define C_S0_NOW	004000
#define C_S1_NOW	010000
#define	C_ACTION	017740

#define SAFE		1
#define SORRY		0
#define LOCKED		1
#define UNLOCKED	0

#define DISP_ALL	0                        /* redisplay entire board */
#define DISP_NEW	1                          /* only display changes */

#define	W_IDLE		0			     /* values for b_state */
#define	W_P0_TKN	1			      /* p0 now setting up */
#define	W_P1_TKN	2			      /* p1 now setting up */
#define	W_P0_RDY	4		      /* p0 ready for war to begin */
#define	W_P1_RDY	8		      /* p1 ready for war to begin */
#define	W_AT_WAR	16			  /* battle already raging */
#define	W_DONE		32			/* battle already finished */

struct	cellstr {
	char    c_occ;                           /* who occupies this cell */
	char    c_mil;                                /* how many military */
	short   c_flg;                                            /* flags */
};

struct	boardstr {
	char	b_xmax, b_ymax;		     /* x and y sizes of the board */
	char	b_state;	/* whether waiting for players, done, etc. */
	char	b_hcap[2];		   /* command rate for each player */
	struct	cellstr b_cell[MAX_X * MAX_Y];
};

extern	char	*boardfil;
extern	char	*warinitprog;
extern  int     bfh, cfh, bdlock, crtflg, crtyp, ttymod[4];
extern	int	onum, tnum, our_home, their_home;
extern  int     fcell, lcell;
extern  int     para, shell;
extern  int     lastpara, lastshell, lastus, lastthem;
extern  int     delta[];
extern  struct  boardstr board;
extern  struct  boardstr disp_board;
extern  struct  lockstr lck;
