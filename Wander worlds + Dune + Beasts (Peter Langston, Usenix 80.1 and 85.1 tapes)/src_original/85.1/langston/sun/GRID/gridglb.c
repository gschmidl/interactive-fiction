#include	"grid.h"
#include	<signal.h>
/*
**	GRIDGLB -- Globals for all parts of labyrinth game
**		P. Langston, 6/82
*/

#define	PATH(x)	GAMESPATH(GRID/x) /* GAMESPATH() defined in ../gamesdef.h */

char	*infofil	= PATH(gridinfo);   /* instructions for the game */
char	*slfil		= PATH(sucker.list);/* habitual players */
char	*recfil		= PATH(gridrec);    /* records */
char	*npfile		= PATH(gridnp);	    /* new players entering the grid */
char	*erlogfil	= PATH(oops);	    /* daemon diagnostics (erlogf()) */
char	*tfilfmt	= PATH(gtmp%d);	    /* format for disenroll temp */
#ifdef SUN
char	*daemon		= PATH(sundaemon);  /* location of the daemon binary */
#else
char	*daemon		= PATH(daemon);	    /* location of the daemon binary */
#endif

int	wizuid	= PRVUID;		/* uid of priveleged user */

struct	lockstr	dlock	= {
	PATH(.dnode),
	PATH(d.file),
	0,
	300,
	1,
	0,
	SIG_IGN,
};

struct	lockstr	glock	= {
	PATH(.gnode),
	PATH(g.file),
	"Waiting on gridlock",
	120,
	8,
	3,
	SIG_IGN,
};
