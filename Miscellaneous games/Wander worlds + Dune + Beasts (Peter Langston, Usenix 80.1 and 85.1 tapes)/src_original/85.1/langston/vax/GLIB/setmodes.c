/*
**      SETMODES/RESETMODES -- set/reset misc tty modes
**              (c) Peter S. Langston 1981
*/
#define BSD		/* for 4.1BSD */
/*#define	V3.0		/* for Bell 3.0 or 4.0 */
/*#define	SGTTY		/* for the rest */

#include	<sys/param.h>
#define	MAXFH	NOFILE		/* how large file descriptors can be */

static char old_ok[MAXFH];  /* non-zero if "old[fh]" has been initialized */

#ifdef  BSD

#include <sgtty.h>

static	struct	sgttyb	oldttymodes[MAXFH];

setmodes(fh, style)
char style;
{
	struct	sgttyb	spc;

	if (ioctl(fh, TIOCGETP, &spc) == -1)
	    return(-1);
	if (fh < MAXFH) {
	    oldttymodes[fh] = spc;
	    old_ok[fh] = 1;
	}
	if (style == 'R') {                 /* "RAW" & no "ECHO" */
	    spc.sg_flags &= ~(XTABS | CRMOD | ECHO | LCASE | CBREAK | TANDEM);
	    spc.sg_flags |= RAW;
	} else if (style == 'e') {		/* no "ECHO" */
	    spc.sg_flags &= ~ECHO;
	} else
	    return(-1);
	return(ioctl(fh, TIOCSETP, &spc));
}

resetmodes(fh)                       /* however they were at startup */
{
	if (fh < MAXFH && old_ok[fh] == 1)
	    return(ioctl(fh, TIOCSETP, &oldttymodes[fh]));
	else
	    return(1);
}
#endif

#ifdef  V3.0

#include <termio.h>

struct termio oldttymodes[MAXFH], spcmod;

setmodes(fh, style)
char style;
{
	if (ioctl(fh, TCGETA, &spcmod) == -1)
	    return(-1);
	if (fh < MAXFH) {
	    ioctl(fh, TCGETA, &oldttymodes[fh]);
	    old_ok[fh] = 1;
	}
	if (style == 'R') {                 /* "RAW" & no "ECHO" */
	    spcmod.c_iflag = IGNBRK | IGNPAR | ISTRIP;
	    spcmod.c_oflag = 0;
	    spcmod.c_cflag |= HUPCL;
	    spcmod.c_lflag = 0;
	    spcmod.c_cc[VMIN] = 1;
	    spcmod.c_cc[VTIME] = 0;
	} else if (style == 'e') {
	    spcmod.c_lflag &= ~ECHO;
	} else
	    return(-1);
	return(ioctl(fh, TCSETA, &spcmod));
}

resetmodes(fh)                       /* however they were at startup */
{
	if (fh < MAXFH && old_ok[fh] == 1)
	    return(ioctl(fh, TCSETA, &oldttymodes[fh]));
	else
	    return(1);
}
#endif

#ifdef  SGTTY

#include    <sgtty.h>

short	ttymod[MAXFH][3], spcmod[3];

setmodes(fh, style)
char style;
{
	
	if (gtty(fh, spcmod) == -1)
	    return(-1);
	if (fh < MAXFH) {
	    gtty(fh, ttymod[fh]);
	    old_ok[fh] = 1;
	}
	if (style == 'R') {			/* "RAW" & no "ECHO" */
	    spcmod[2] &= ~(CRMOD | ECHO | LCASE | XTABS);
	    spcmod[2] |= RAW;
	} else if (style == 'e') {		/* no "ECHO" */
	    spcmod[2] &= ~ECHO;
	} else
	    return(-1);
	return(stty(fh, spcmod));
}

resetmodes(fh)                       /* however they were at startup */
{
	if (fh < MAXFH && old_ok[fh] == 1)
	    return(stty(fh, ttymod[fh]));
	else 
	    return(1);
}
#endif
