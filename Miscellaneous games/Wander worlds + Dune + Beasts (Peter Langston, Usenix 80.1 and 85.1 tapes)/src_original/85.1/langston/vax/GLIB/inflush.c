#include	<sgtty.h>
/*
**      INFLUSH -- System dependent input flusher
*/

inflush(fh)
{
	ioctl(fh, TIOCFLUSH, 0);
}
