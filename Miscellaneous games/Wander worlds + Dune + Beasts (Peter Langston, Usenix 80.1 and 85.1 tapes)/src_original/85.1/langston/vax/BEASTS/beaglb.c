#include	"beasts.h"
/*
**      BEAGLB -- Globals for them beasts
** (c) P. Langston 1979
*/

static  char    *whatsccs = "@(#)beaglb.c	1.8  last mod 2/25/85 -- (c) psl 1979";
static  char    *h_sccs =   H_SCCS;

#define N_BEAST     1024                /* max num of beasts */
#define NBI         ((N_BEAST+15)/16)   /* space for bit map */

short   bmap[NBI];

#include	"../gamesdef.h"
#define PATH(x)		GAMESPATH(BEASTS/x)
char    *beastfile	= PATH(beastfile);
char    *questfile	= PATH(questfile);
char    *oopsfile	= PATH(oops);
/* Set the following to a string or 0.  PRVLOG is from ../gamesdef.h */
char	*mnblog		= PRVLOG;	/* who to notify of new beasts */
char	*mnqlog		= PRVLOG;	/* who to notify of new questions */

extern	int	done();


struct	lockstr	lck = {
	PATH(.bnode),			/* linking spot */
	PATH(b.lock),			/* file to link */
	"Hmmm, I hope you don't mind waiting a minute;\nI can't seem to find something I need...\n",
	10*60,				/* how long a lock can lie around */
	12,				/* how many tries before giving up */
	5,				/* time to sleep after each failure */
	done,				/* routine to catch signals */
};
