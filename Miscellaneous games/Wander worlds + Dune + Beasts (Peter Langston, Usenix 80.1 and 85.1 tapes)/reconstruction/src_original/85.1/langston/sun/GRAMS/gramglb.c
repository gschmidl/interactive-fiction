#include    "gramdef.h"
/*
**      GRAMGLB -- Anagram exerciser globals
** (c) P. Langston
*/

static  char    *whatsccs   = "@(#)gramglb.c	1.4 9/15/84 -- (c) psl 1979";
static  char    *h_sccs     = H_SCCS;

#include	"../gamesdef.h"

char    *dictionary = GAMESPATH(BOG/bogwds);	  /* computer's vocabulary */
char    *infofil    = GAMESPATH(GRAMS/gramsinfo);	   /* instructions */
char    *recfil     = GAMESPATH(GRAMS/gramsrec);		/* records */

int     def_len = 5;                                /* default word length */
int     max_len = 20;                               /* maximum word length */
int     nwpr    = 10;                         /* number of words per round */
