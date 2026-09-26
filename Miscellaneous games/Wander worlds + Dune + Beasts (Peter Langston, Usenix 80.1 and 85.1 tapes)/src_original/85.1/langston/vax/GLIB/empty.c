#include	"../gamesdef.h"

/* one of these should have been selected in ../gamesdef.h */
/*#define	SELECTV8			/* for Research V8 */
/*#define	SELECTBSD			/* for 4.2 BSD */
/*#define	FIONRE				/* for 4.1 BSD and others */

#ifdef	SELECTV8
#include	<sys/param.h>
#include	<sys/types.h>

empty(fh)
{
	fd_set rfds;

	FD_ZERO(rfds);
	FD_SET(fh, rfds);
	return(select(NOFILE, &rfds, 0, 0) == 0);
}
#endif	SELECTV8

#ifdef	SELECTBSD
#include	<sys/time.h>

empty(fh)
{
	int rfds, tout;

	rfds = 1 << fh;
	tout = 0;
	return(select(fh + 1, &rfds, 0, 0, &tout) == 0);
}
#endif	SELECTBSD

#ifdef	FIONRE
#include	<sgtty.h>

empty(fh)
{
	long lng;

	ioctl(fh, FIONREAD, &lng);
	return(lng == 0);
}
#endif	FIONRE
