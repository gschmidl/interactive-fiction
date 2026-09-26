/*
**      GETSPD -- get tty output speed
*/

#define SGTTY       /* change this to V3.0 for Bell 3.0 or 4.0 */

#ifdef  SGTTY

#include    <sgtty.h>

struct	sgttyb	sg;

getspd(fh)
{
	
	if (ioctl(fh, TIOCGETP, &sg) == -1)
	    return(-1);
	return(sg.sg_ospeed);
}

#endif

#ifdef  V3.0

#include <termio.h>

struct termio oldmod;

getspd(fh)
{

	if (ioctl(fh, TCGETA, &oldmod) == -1)
	    return(-1);
	return(oldmod.c_cflag & CBAUD);
}

#endif
