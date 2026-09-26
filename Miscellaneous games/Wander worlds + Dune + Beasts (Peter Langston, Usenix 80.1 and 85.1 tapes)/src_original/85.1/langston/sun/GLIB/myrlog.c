/*
** Get the "real" login name of the current user
*/
/* Define one of these or hack up the code under "OTHER" */

/*#define	PWB		/**/
#define	BSD		/**/

#include <stdio.h>
#ifdef	OTHER
#include <utmp.h>
#endif

char	*
myrlog()
{
#ifdef	PWB
	return(logname());
#endif
#ifdef	BSD
	char *cp;
	extern char *getlogin(), *getpwuid();

	cp = getlogin();
	if (cp == NULL)
	    cp = getpwuid(getuid());
	return(cp);
#endif
#ifdef OTHER
	char *tty, *ttyname();
	FILE *fp, *fopen();
	static struct utmp ut;

	tty = ttyname(2);	/* assumes result is "/dev/XXXXXX" */
	tty += 5;		/* strip "/dev/" off of tty */
	if ((fp = fopen("/etc/utmp", "r")) == NULL)
	    return("???");
	while (fread(&ut, sizeof(ut), 1, fp) == 1) {
	    if (equal(tty, ut.ut_line)) {
		fclose(fp);
		ut.ut_name[8] = '\0';	/* a little sneaky, but safe... */
		return(ut.ut_name);
	    }
	}
	fclose(fp);
	return("???");
#endif
}
