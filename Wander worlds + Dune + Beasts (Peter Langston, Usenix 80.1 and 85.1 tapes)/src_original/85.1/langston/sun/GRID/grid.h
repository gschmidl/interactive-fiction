/*
**	GRID.H -- Header file for all parts of labyrinth game
**		P. Langston, 6/82
*/

#include	"../gamesdef.h"
#include	"../lock.h"

struct	recstr	{
	char	r_name[16];
	char	r_why[38];
	int	r_lev;
	long	r_score;
	long	r_date;
};

extern	char	*locknode;	/* MUST EXIST; used for locking */
extern	char	*grdlock;	/* locks the npfile */
extern	char	*dmnlock;	/* set by grid, cleared by daemon */
extern	char	*infofil;	/* instructions for the game */
extern	char	*slfil;		/* habitual players (sucker list) */
extern	char	*recfil;	/* records */
extern	char	*npfile;	/* new players entering the grid */
extern	char	*erlogfil;	/* daemon diagnostic output (erlogf()) */
extern	char	*tfilfmt;	/* pattern for temp file for disenroll */
extern	char	*daemon;	/* location of the daemon binary */
extern	int	wizuid;		/* uid for priveleged user */
extern	struct	lockstr	dlock;	/* daemon lock */
extern	struct	lockstr	glock;	/* grid lock */
