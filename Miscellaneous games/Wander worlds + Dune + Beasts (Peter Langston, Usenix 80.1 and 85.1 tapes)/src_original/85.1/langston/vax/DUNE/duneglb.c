/*
**      DUNEGLB -- Globals for "dune"
*/

static	char	*whatsccs	= "@(#)duneglb.c	1.2 6/1/84 -- (c) psl 1980";

/*	If you change these characters remember to change */
/*	"dune.doc" to specify the new control characters. */
char	m_up		= '8';
char	m_down		= '2';
char	m_left		= '4';
char	m_right		= '6';
char	m_hor		= 'h';
char	m_ver		= 'v';
char	m_ndhor		= 'H';
char	m_ndver		= 'V';
char	m_drop		= '$';
char	m_redraw	= '^';
char	m_display	= 'd';
char	m_nodisp	= 'n';

char	huntchar	= 'U';
char	wbodychar	= 'o';
char	wheadchar	= '@';
char	spicechar	= '$';
char	holechar	= 'O';

int	wlength		= 6;		/* must not exceed 16 */

#include	"../gamesdef.h"
char    *recfil = GAMESPATH(DUNE/dunerec);	/* where records are kept */
char    *infofile = GAMESPATH(DUNE/dune.doc);	/* instructions */
