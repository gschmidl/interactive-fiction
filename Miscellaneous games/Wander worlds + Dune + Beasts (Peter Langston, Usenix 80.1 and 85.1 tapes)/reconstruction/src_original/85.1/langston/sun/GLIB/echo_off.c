#include	<sgtty.h>
/*
**	System-dependent echo on/off routines
*/

struct	sgttyb	mod, old;

echo_off(fh)                                              /* turn off echo */
{
	ioctl(fh, TIOCGETP, &old);
	ioctl(fh, TIOCGETP, &mod);
	mod.sg_flags &= ~ECHO;
	ioctl(fh, TIOCSETP, &mod);
}

echo_reset(fh)                             /* reset echo if it was diddled */
{
	if (old.sg_flags)
	    ioctl(fh, TIOCSETP, &old);
}
