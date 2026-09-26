#include    <kbd.h>
/* Compile: cc -O -q -c kbd1.c
**      KBDREAD -- Keyboard input using termcap
**      Usage: count = kbdread(fh, buf, type)
**          inputs to kbdread():
**              int  fh         file descriptor
**              char *buf       buffer for output
**              int  type       term type
**          output from kbdread():
**              int  count      number of chars left in buf (usually 1)
**              char *buf       <NUL> terminated char(s) or token read
** (c) P. Langston 1981
*/

static  char    *sccsid = "%W% %G% -- PSL misc";

extern	char	*copy();

kbdread(fh, buf, type)
char    *buf;
{
	register char *rbp;
	register int i;
	char rawbuf[8];

	rbp = rawbuf;
	for (;;) {
	    if (read(fh, rbp, 1) != 1)
		return(0);
	    *rbp++ &= 0177;
	    *rbp = '\0';
	    i = kbdmtch(type, rawbuf);
	    if (i < 0) {                                  /* found a token */
		*buf++ = i;
		*buf = '\0';
		return(1);
	    }
	    if (i == 0)                                /* can't be a token */
		return(copy(rawbuf, buf) - buf);
	}
}
