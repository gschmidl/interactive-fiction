/*
**	SREAD(buffer, maxlength)
**		Read up to maxl-many characters into buffer with
**              echo supressed.  Return # chars read.
** Delete or comment out the vrsions that don't apply to your system.
*/

#define	BSD		/* 4.1BSD, that is */
/*#define	V6		/* misc V6 systems */
/*#define	V7		/* misc V7 systems */
/*#define	V3.0		/* Bell 3.0, 4.0 & probably 5.0 */

#ifdef	BSD
#include	<sgtty.h>

sread(buf, maxl)
char *buf;
{
	register short i, len;
	int sigs[16];
	struct sgttyb ttm;

	for (i = 16; --i > 0; sigs[i] = signal(i, 1));	/* ignore signals */
	ioctl(0, TIOCGETP, &ttm);
	i = ttm.sg_flags;
	ttm.sg_flags &= ~ECHO;
	ioctl(0, TIOCSETP, &ttm);
	ttm.sg_flags = i;
	len = read(0, buf, maxl);
	ioctl(0, TIOCSETP, &ttm);
	for (i = 16; --i > 0; signal(i, sigs[i]));	/* reset signals */
	return(len);
}
#endif

#ifdef	V6
#define	ECHO	010

sread(buf, maxl)
char *buf;
{
	register int i;
	int ttm[3], sigs[16];

	for (i = 16; --i > 0; sigs[i] = signal(i, 1));	/* ignore signals */
	gtty(0, ttm);
	i = ttm[2];
	ttm[2] &= ~ECHO;
	stty(0, ttm);
	ttm[2] = i;
	i = read(0, buf, maxl);
	stty(0, ttm);
	for (i = 16; --i > 0; signal(i, sigs[i]));	/* reset signals */
	return(i);
}
#endif

#ifdef	V7
#include	<sgtty.h>

sread(buf, maxl)
char *buf;
{
	register short i, len;
	int sigs[16];
	struct sgttyb ttm;

	for (i = 16; --i > 0; sigs[i] = signal(i, 1));	/* ignore signals */
	gtty(0, &ttm);
	i = ttm.sg_flags;
	ttm.sg_flags &= ~ECHO;
	stty(0, &ttm);
	ttm.sg_flags = i;
	len = read(0, buf, maxl);
	stty(0, &ttm);
	for (i = 16; --i > 0; signal(i, sigs[i]));	/* reset signals */
	return(len);
}
#endif


#ifdef	V3.0
#include <termio.h>

sread(buf, maxl)
char *buf;
{
	register int i;
	struct termio tp1, tp2;
	int sigs[16];

	for (i = 16; --i > 0; sigs[i] = signal(i, 1));	/* ignore signals */
	ioctl(0, TCGETA, &tp1);
	ioctl(0, TCGETA, &tp2);
	tp1.c_lflag &= ~ECHO;
	ioctl(0, TCSETA, &tp1);
	i = read(0, buf, maxl);
	ioctl(0, TCSETA, &tp2);
	for (i = 16; --i > 0; signal(i, sigs[i]));	/* reset signals */
	return(i);
}
#endif
