#include    <stdio.h>
/*
**      ANNOUNCE psl@DPW 6/82
** Send "msg" to specified user on all terminals where he/she is logged in.
** Return a count of successes.
*/

	/* define one of these two? */
/*#define V6UTMP          /* if you have a V6 /etc/utmp */
#define V7UTMP          /* if you have a V7 /etc/utmp */

#ifdef  V6UTMP
struct utmp {
    char    ut_name[8];     /* user login name */
    char    ut_tty;         /* last letter of tty name */
    char    ut_acct;        /* account */
    long    ut_time;        /* login time */
    short   ut_fill;
};
#endif

#ifdef  V7UTMP
#include    <utmp.h>
#endif

extern	char	*copy();

announce(msg, user)
char    *msg, *user;
{
	register short i, n, ufh, dfh;
	short msglen;
	char devnam[16], *cp;
	struct utmp ut;

	if ((ufh = open("/etc/utmp", 0)) < 0)
	    return(-1);
	msglen = copy(msg, msg) - msg;
	for (n = 0; read(ufh, &ut, sizeof ut) == sizeof ut; ) {
	    if (ut.ut_name[0] == 0)
		continue;
	    if (samename(ut.ut_name, user)) {
#ifdef  V6UTMP
		cp = copy("/dev/tty", devnam);
		*cp++ = ut.ut_tty;
		*cp++ = '\0';
#endif
#ifdef  V7UTMP
		cp = copy("/dev/", devnam);
		copy(ut.ut_line, cp);
#endif
		dfh = open(devnam, 1);
		if (dfh >= 0) {
		    write(dfh, msg, msglen);
		    close(dfh);
		    n++;
		}
	    }
	}
	close(ufh);
	return(n);
}

samename(a, b)
char    *a, *b;
{
	register char *ap, *bp, *ep;

	ap = a;
	bp = b;
	ep = &a[8];
	while (*ap++ == *bp++ && bp[-1])
	    if (ap >= ep)
		return(1);
	if ((*--ap == ' ' || *ap == '\0')
	 && (*--bp == '\n' || *bp == '\0'))
	    return(1);
	return(0);
}
