#include	"daemon.h"
/*
**	DGLB -- Globals for daemon for labyrinth game
**		P. Langston, 6/82
*/

char	sb[128];		/* scroll buffer used in sprintfs */
int	maxp	= MAXP;		/* max # of players */
int	maxl	= MAXL;		/* max # of levels */
int	maxo	= MAXO;		/* max # of others per level */
int	maxg	= MAXG;		/* amount of gold per level */
int	maxt	= MAXT;		/* max concurrent talismans */
int	livelim	= 16;		/* minimum health to be alive */
int	seelim	= 50;		/* minimum health to be able to see */
int	minpdam	= 15;		/* min damage done by player's shots */
int	minodam	= 15;		/* min damage done by other's shots */
int	minfdam	= 66;		/* min damage done by fire talisman */
int	dx[4]	= { 0, 1, 0, -1, };
int	dy[4]	= { -1, 0, 1, 0, };
int	maxpnum	= 0;		/* highest player number used so far + 1 */
int	dlocked	= -1;		/* we own dmnlock if != -1 */
int	glocked	= -1;		/* we own gridlock if != -1 */
int	nalive	= 0;		/* num of players still living */
int	numnew	= 0;		/* num that have entered the game */
int	npfh;			/* file handle for newplayer file */

struct	levstr	lev[MAXL];

struct	plyrstr	p[MAXP];
