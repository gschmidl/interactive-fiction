#include    <stdio.h>
#include    "oracle.h"
/*
**      ORAGLB -- Globals for oracle
*/

static  char    *sccsid = "%W%  %G% -- (c) psl 1978";
static  char    *h_sccs = H_SCCS;

#include	"../gamesdef.h"
#define PATH(x) GAMESPATH(ORACLE/x)
char    *indexfile  = PATH(oracleindex);
char    *q_afile    = PATH(oracleq+a);
char    *tmpfile    = PATH(oracletmp);
char    *privname   = PRVLOG;	 /* person to whom error messages are sent */
short   privuid     = PRVUID;	/* person with special snooping priveleges */

/*
**      NOTE: there are four trick answers to the first question asked:
**          "sn"    lets you snoop on unanswered questions
**          "sno"   lets you snoop on all questions
**          "an"    lets you answer a question without asking one
**          "pu"    purges answered questions from the file
**      None of these work unless your uid is that in privuid.
*/

char    *hmm[]  = {                       /* miscellaneous thinking sounds */
	"Hmmmmm... ",
	"Well, let me see ... ",
	"Sometimes I wonder if people are taking me seriously.  In any case\n",
	"Boy!  That's a toughie!  ",
	"Enlightenment shall not come easily on this matter.\n",
	"Golly, Gee Whizzikers!  ",
	"That should be obvious; but in case there's some hidden subtlety involved\n",
	"My oh my, I guess ",
	"I hope you really need to know the answer to that one ...\n",
};

extern	int	bye();

struct	lockstr lck	= {
	PATH(.onode),		/* The "permanent" name for the file */
	PATH(o.lock),		/* The "temporary" link to l_node */
	"Please excuse the delay, but I'm very busy at the moment!\n",
				/* Msg to warn of lock condition */
	600,			/* how long before a lock is stale */
	121,			/* how many times to try */
	5,			/* how long to wait between tries */
	bye,			/* Routine to handle interrupts while locked */
};

