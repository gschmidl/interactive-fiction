#include	"wardef.h"

static  char    *warinitid = "@(#)warglb.c	1.14 2/25/85 -- (c) psl 1976";
static  char    *wardefid  = DEF_SCCS;

/* The "boardfil" file is the communication link between the two programs
** playing the game.  Note that it is NOT specified as a full pathname,
** thus it exists in the current working directory and several "war"s can
** be runnning simultaneously.
*/
char	*boardfil =	"warboard";

/* The following structure defines parameters for a lock mechanism.
** The first two strings are file names and need not be pre-existing
** files; they will be stomped on by the lock mechanism.
** The third string is the message printed after the second failure to link.
*/
struct	lockstr	lck	= {
        ".bnode",			/* lock node name */
	"b.lock",			/* lock file name */
	"waiting on board lock",	/* patience message */
	60,				/* how long before lock ignored */
	99,				/* how many tries before giving up */
	1,				/* how long to sleep between tries */
	0,				/* die from signals while locked */
};

#include	"../gamesdef.h"

/* The following needs to point at an executable copy of the initialization
** program.  It is called by "war" when needed.
*/
char	*warinitprog	= GAMESPATH(WAR/warinit);

/* erlogfil contains diagnostic messages from program failures.
** Define it as "0" if you don't want to collect these.
*/
char	*erlogfil	= GAMESPATH(WAR/oops);

int     bfh, cfh, bdlock, crtflg, crtyp, ttymod[4];
int     onum, tnum, our_home, their_home;
int     fcell, lcell;
int     para, shell;
int     lastpara    = -1;
int     lastshell   = -1;
int     lastus      = -1;
int     lastthem    = -1;
int     delta[] = {
	-MAX_X, 1, MAX_X, -1,
};
struct  boardstr    board;
struct  boardstr    disp_board;
