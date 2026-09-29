#
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            Created by Peter Langston

        WANDER - Non-deterministic fantasy story tool
                 Version 2.2

Global definitions

Copyright (c) 1978 by Peter S. Langston - New  York,  N.Y.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include    <stdio.h>

/*      CAVEAT: Only those defines marked as "(MODIFY)" may be changed.
*/


#define MAXLOCS     128     /* max locs (MODIFY) */
#define MAXACTS     100     /* max acts in each state (MODIFY) */
#define MAXFIELDS   8       /* max fields in each act (MODIFY) */
#define MAXACTWDS   5       /* max words in an action (MODIFY) */
#define BUFSIZE     1024    /* size of line buffers */

#define FIELDELIM   ' '     /* delimit fields (MODIFY) */
#define LINEDELIM   '\n'    /* delimit lines (MODIFY) */
#define ESCHAR      '\\'    /* escape <bs>, <lf>, <cr>, <sp> and <ht> (MODIFY) */
#define VARCHAR     '%'     /* indicate variable substitution (MODIFY) */
#define DOTCHAR     '.'     /* make "dotted pairs" as in s=3.2 (MODIFY) */
#define ATCHAR      '@'     /*  "at loc pairs" as in o+card@5 (MODIFY) */
#define COMCHAR     ':'     /* indicate comment line (must be first char) */

#define BASESTATE   -1      /* stuff common to all states of a loc */
#define FLD1_VAR    0200    /* used in field types */
#define FLD2_VAR    0100    /* used in field types */
#define TYPEONLY    0077    /* used to strip previos two from type */
#define NO_WORD     0       /* indicates no word specified (in acts) */

        /* return codes from carry_out() and check_act() */
#define COM_UNREC   0       /* command not recognized */
#define COM_RECOG   1       /* command recognized */
#define COM_DONE    2       /* command successful */
#define COM_DESC    4       /* command needs new prloc() */
#define COM_NDOBJ   8       /* command needed an object to match */
#define COM_COMPLETE 16     /* command needs no further attention */

        /* quit codes (all locs < 0 imply quitting) */
#define QUIT_SCORE  -1
#define QUIT_QUIET  -2

	/* special variables */
#define CUR_LOC     100     /* current location */
#define PREV_LOC    101     /* previous location */
#define INP_W1      102     /* hash of first recognized word in inp comm */
#define INP_W2      103     /* hash of second recog word from inp comm */
#define INP_W3      104     /* hash of third recog word from inp comm */
#define INP_W4      105     /* hash of fourth recog word from inp comm */
#define INP_W5      106     /* hash of fifth recog word from inp comm */
#define MAXINPWD    (INP_W1 + MAXACTWDS - 1)    /* should = prev define */
#define INP_WC      107     /* number of words in input comm */
#define NUM_CARRY   108     /* # of things being carried */
#define MAX_CARRY   109     /* # of thing poss. to carry at once */
#define NOW_YEAR    110     /* year of decade (0:99) */
#define NOW_MONTH   111     /* month of year (1:12) */
#define NOW_DOM     112     /* day of month (1:31) */
#define NOW_DOW     113     /* day of week (0:6) */
#define NOW_HOUR    114     /* hour of day (0:23) */
#define NOW_MIN     115     /* minute of hour (0:59) */
#define NOW_SEC     116     /* second of minute (0:59) */
#define NOW_ET      117     /* elapsed time in Wander (seconds) */

        /* field types */
#define F_VOID      0
#define FT_OBJ      1
#define FT_NOBJ     2
#define FT_TOOL     3
#define FT_NTOOL    4
#define FT_STATE    5
#define FT_NSTATE   6
#define FT_EVAR     7
#define FT_NVAR     8
#define FT_GVAR     9
#define FT_LVAR     10
#define FT_ODDS     11

#define FR_GOBJ     20
#define FR_LOBJ     21
#define FR_GTOOL    22
#define FR_LTOOL    23
#define FR_SSTATE   24
#define FR_ISTATE   25
#define FR_DSTATE   26
#define FR_SVAR     27
#define FR_IVAR     28
#define FR_DVAR     29
#define FR_MVAR     30
#define FR_QVAR     31
#define FR_CSUB     32
#define FR_WORLD    33


struct  placestr {
	int  p_loc;
        char p_state;
        char p_sdesc[128];
	char p_ldesc[BUFSIZE];
	struct  actstr {
		int a_wrd[MAXACTWDS];   /* command words */
		int a_rloc;             /* result location */
		char a_rcont;           /* 1 ==> continue to other actions */
		struct  fieldstr {
			char    f_type;
			int     f_fld1;
			int     f_fld2;
		} a_field[MAXFIELDS];   /* tests & results */
		char *a_message;        /* result message */
	} p_acts[MAXACTS];
};

struct  liststr {
        char *l_word;
        int l_syn;
	int l_loc;
};

struct  msgstr {
        char *msgbuf;
        char *msgbp;
        int msgleft;
};

struct  indexstr {
	int  i_loc;
        char i_state;
        long i_addr;
};

extern  struct  indexstr    index[];
extern  struct  placestr    place;
extern  struct  actstr  pre_acts[], post_acts[];
extern  struct  liststr wrds[], spvars[];
extern  char    *thereis[], *aansome[];
extern  char    fldels[], vardel[], wrdels[];
extern  char    listunused[];
extern  char    locfile[], miscfile[], tmonfil[], monfile[];
extern  char    pmsgbuf[], gmsgbuf[];
extern  struct  msgstr  pmsgs, gmsgs;
extern  int     pmsgsleft, gmsgsleft;
extern  char    curfile[];
extern  char    *stdpath, *savfile, *defmfile;
extern  int     maxwrds, maxactwds, maxinpwd, maxlocs, maxindex, maxacts;
extern  int     maxpreacts, maxpostacts, maxfields, maxvars;
extern  int     ldescfreq;
extern  char    fieldelim, linedelim;
extern  char    eschar, varchar, dotchar, atchar, comchar;
extern  int     monitor, monloc, monstate;
extern  int     max_carry;
extern  char    inwrd[][32];
extern  char    locseen[], locstate[], carrying[];
extern  int     var[];

