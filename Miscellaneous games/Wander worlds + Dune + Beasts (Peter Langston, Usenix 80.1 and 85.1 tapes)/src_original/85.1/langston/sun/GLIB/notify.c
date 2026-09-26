#include    <stdio.h>
/*
**      NOTIFY psl 6/82
** Send "msg" to  any users in "listfil" that are logged in
*/

#include	"../gamesdef.h"

/* one of these should have been selected in ../gamesdef.h */
/*#define	SELECTV8			/* for Research V8 */
/*#define	SELECTBSD			/* for 4.2 BSD */
/*#define	FIONRE				/* for 4.1 BSD and others */

/* one of these should have been selected in ../gamesdef.h */
/*#define	V6UTMP			/* if you have a V6 /etc/utmp */
/*#define	V7UTMP			/* if you have a V7 /etc/utmp */

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

notify(msg, listfil)
char    *msg, *listfil;
{
	register short i, n, fh;
	short msglen;
	char user[16], devnam[16], *cp;
	struct utmp ut[64];          /* make sure enough room for all ttys */
	FILE *fp;

	if ((fp = fopen(listfil, "r")) == NULL)
	    return;
	if ((fh = open("/etc/utmp", 0)) < 0) {
	    fclose(fp);
	    return;
	}
	msglen = copy(msg, msg) - msg;
	for (n = 0; read(fh, &ut[n], sizeof ut[0]) == sizeof ut[0]; )
	    if (ut[n].ut_name[0])
		n++;
	close(fh);
	if (fork() != 0) {
	    fclose(fp);
	    return;
	}		/* the rest done in background */
	while (fgets(user, sizeof user, fp) != NULL) {
	    for (i = 0; i < n; i++) {
		if (samename(ut[i].ut_name, user)) {
#ifdef  V6UTMP
		    cp = copy("/dev/tty", devnam);
		    *cp++ = ut[i].ut_tty;
		    *cp++ = '\0';
#endif
#ifdef  V7UTMP
		    cp = copy("/dev/", devnam);
		    copy(ut[i].ut_line, cp);
#endif
		    if ((fh = open(devnam, 1)) >= 0) {
			if (writable(fh))
			    write(fh, msg, msglen);
			close(fh);
		    }
		}
	    }
	}
	exit(0);
}

#ifdef SELECTV8
#include	<sys/param.h>
#include	<sys/types.h>
writable(fh)
{
	fd_set wfds;

	FD_ZERO(wfds);
	FD_SET(fh, wfds);
	return(select(NOFILE, 0, &wfds, 0));
}
#endif	SELECTV8

#ifdef	SELECTBSD
#include	<sys/time.h>

writable(fh)
{
	int wfds, tout;

	wfds = 1 << fh;
	tout = 0;
	return(select(fh + 1, 0, &wfds, 0, &tout) != 0);
}
#endif	SELECTBSD

#ifdef	FIONRE
writable(fh)
{
	return(1);
}
#endif	FIONRE

samename(a, b)
char    *a, *b;
{
	register char *ap, *bp, *ep;

	ap = a;
	bp = b;
	ep = &a[8];
	while (*ap++ == *bp++)
	    if (ap >= ep)
		return(1);
	if ((*--ap == ' ' || *ap == '\0') && *--bp == '\n')
	    return(1);
	return(0);
}
