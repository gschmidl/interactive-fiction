#include    "wanddef.h"
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      WANDER - Non-deterministic fantasy story tool
	       Global storage allocations

Compile: cc -c -O -q wandglb.c

Copyright (c) 1978 by Peter S. Langston - New  York,  N.Y.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

char    *whatglb "@(#)wandglb.c	2.4  last mod 9/23/79 -- (c) psl 1978";

/* the following defines are used only in wandglb.c and may be changed */

#define MAXWRDS     200        /* max words incl ones mentioned in actions */

#define MAXINDEX    400                     /* max states total (all locs) */
#define MAXPREACTS  24                                  /* max pre actions */
#define MAXPOSTACTS 64                                 /* max post actions */
#define MAXVARS     128             /* number of variables, must be == 128 */


struct  indexstr    index[MAXINDEX];

struct  placestr    place;

struct  actstr  pre_acts[MAXPREACTS];
struct  actstr  post_acts[MAXPOSTACTS];

struct  liststr wrds[MAXWRDS] {
	listunused,     0,  MAXWRDS,  /* hopefully nothing will match this */
        "inventory",    0,  0,
	"take",         0,  0,
	"pick",         1,  0,
	"drop",         0,  0,
	"quit",         0,  0,
	"save",         0,  0,
	"look",         0,  0,
	"re-start",     0,  0,
        "north",        0,  0,
	"n",            1,  0,
        "south",        0,  0,
	"s",            1,  0,
	"east",         0,  0,
	"e",            1,  0,
	"west",         0,  0,
	"w",            1,  0,
	"up",           0,  0,
	"u",            1,  0,
	"down",         0,  0,
	"d",            1,  0,
        "northeast",    0,  0,
	"ne",           1,  0,
        "southeast",    0,  0,
	"se",           1,  0,
        "southwest",    0,  0,
	"sw",           1,  0,
        "northwest",    0,  0,
	"nw",           1,  0,  /* this must be the last direction verb */
	"~snoop",       0,  0,  /* these three only work if you are owner */
	"~goto",        0,  0,
	"~vars",        0,  0,
	"*",            0,  0,
	"all",          0,  0,          /* used in "take all" & "drop all" */
	0,              0,  0,      /* "all" must be the last defined here */
};

struct  liststr spvars[] {           /* special construct & thier meanings */
	"CUR_LOC",      0,  100,
	"PREV_LOC",     0,  101,
	"INP_W1",       0,  102,
	"INP_W2",       0,  103,
	"INP_W3",       0,  104,
	"INP_W4",       0,  105,
	"INP_W5",       0,  106,
	"INP_WC",       0,  107,
	"NUM_CARRY",    0,  108,
	"MAX_CARRY",    0,  109,
	"NOW_YEAR",     0,  110,
	"NOW_MONTH",    0,  111,
	"NOW_DOM",      0,  112,
	"NOW_DOW",      0,  113,
	"NOW_HOUR",     0,  114,
	"NOW_MIN",      0,  115,
	"NOW_SEC",      0,  116,
	"NOW_ET",       0,  117,
	0,              0,  0,
};

char *thereis[] {
        "", "There is a",   "There is an", "There are some",
};

char *aansome[] {
        "", "a", "an",  "some",
};

char fldels[] { FIELDELIM, LINEDELIM, 0, };             /* delimits fields */
char vardel[] { VARCHAR, 0, };                     /* terminates variables */
char wrdels[] {                                       /* to separate words */
	' ', ' ' | 0200, ',', '.', ';', '!', '?', 0,
};

char listunused[] "\b\b\b\b";           /* used to mark empty list entries */

char locfile[64], miscfile[64], tmonfil[64], monfile[64];

char pmsgbuf[MAXACTS * 40];                              /* for local acts */
char gmsgbuf[(MAXPREACTS + MAXPOSTACTS) * 48];        /* for pre/post acts */

struct  msgstr  pmsgs { pmsgbuf, pmsgbuf, sizeof pmsgbuf, };
struct  msgstr  gmsgs { gmsgbuf, gmsgbuf, sizeof gmsgbuf, };

int     pmsgsleft   sizeof pmsgbuf;       /* size used for re-init */
int     gmsgsleft   sizeof gmsgbuf;       /* size used for re-init */

char    *stdpath    "/sys/games/.../wand/";      /* where std. worlds live */
char    curfile[64] "a3";                                 /* default world */
char    *savfile    "wand.save";                              /* save name */
char    *defmfile   "/sys/games/.../wand/wand.mon"; /* def. mon. file name */

int     maxwrds     = MAXWRDS;
int     maxactwds   = MAXACTWDS;
int     maxinpwd    = MAXINPWD;
int     maxlocs     = MAXLOCS;
int     maxindex    = MAXINDEX;
int     maxacts     = MAXACTS;
int     maxpreacts  = MAXPREACTS;
int     maxpostacts = MAXPOSTACTS;
int     maxfields   = MAXFIELDS;
int     maxvars     = MAXVARS;
int     ldescfreq   = 5;                 /* how often long desc is printed */

char    fieldelim   = FIELDELIM;
char    linedelim   = LINEDELIM;
char    eschar      = ESCHAR;
char    varchar     = VARCHAR;
char    dotchar     = DOTCHAR;
char    atchar      = ATCHAR;
char    comchar     = COMCHAR;

int     monitor     = -1;                     /* -1 => monitor, 0 => don't */
int     monloc, monstate;

int     max_carry   = 8;           /* default max objects to carry at once */

char    inwrd[5][32];                               /* current input words */
char    locseen[MAXLOCS], locstate[MAXLOCS], carrying[MAXWRDS];
int     var[MAXVARS];

	/* SYSTEM DEPENDENT ROUTINES */

int myruid()        /* return "real" user id */
{
	return(getuid() & 0377);
}

int myeuid()        /* return "effective" user id */
{
	return(getuid() >> 8 & 0377);
}

int myttyn()        /* return string identifying current tty */
{
	static char buf[2];

	buf[0] = ttyn(2);
	buf[1] = '\0';
	return(buf);
}
