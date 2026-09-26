/*
**      NAMUID -- Return uid for given name
**      MYLUID -- Return "login uid" for invoking user
** (c) P.Langston 1979
*/
#define	BSD
#ifdef	BSD
#include	<pwd.h>
#endif

namuid(name)            /* return uid for logname "name" */
char    *name;
{
#ifdef	BSD
	struct passwd *pp;
	extern struct passwd *getpwnam();

	pp = getpwnam(name);
	if (pp == 0)
	    return(-1);
	return(pp->pw_uid);
#else
	register char *cp, *np;
	register int c;
	static char buf[128];
	static int pwf;

	if (pwf <= 0 && (pwf = open("/etc/passwd", 0)) < 0) {
	    perror("/etc/passwd");
	    return(-1);
	}
	lseek(pwf, (long) (0), 0);
	do {
	    for (cp = buf; (c = getch(pwf)) != '\n' && c != -1; *cp++ = c);
	    *cp == '\0';
	    cp = buf;
	    np = name;
	    while (*cp++ == *np++);
	    if (cp[-1] < '=' && np[-1] == '\0') {
		while (*cp++ != ':');
		return(atoi(cp));
	    }
	} while (c != -1);
	return(-1);
#endif
}

myluid()
{
	return(namuid(myrlog()));
}
