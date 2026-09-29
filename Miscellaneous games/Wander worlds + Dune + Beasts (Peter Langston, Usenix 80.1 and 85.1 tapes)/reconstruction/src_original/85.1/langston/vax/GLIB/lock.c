#include	<stdio.h>
#include	"../lock.h"

/*
**	LOCK/UNLOCK -- Set & reset locks using links
*/

lock(lp)
struct	lockstr	*lp;
{
	register int i;
	long now, age;
	extern int errno;
	extern long modtime();

	if (lp->l_count == 0) {
	    if (lp->l_done)
		for (i = NSSIG; --i > 0; )
		    lp->l_sigs[i] = signal(i, *(lp->l_done));
	    i = 0;
	    while (link(lp->l_node, lp->l_file) == -1) {
		switch (errno) {
		case 2:				/* node doesn't exist */
		    if (creat(lp->l_node, 0666) == -1)
			return(-2);
		    break;
		case 17:			/* file already exists */
		    time(&now);
		    age = now - modtime(lp->l_file);
		    if (age > lp->l_delay)		/* stale lock */
			unlink(lp->l_file);
		    else if (i++ > lp->l_limit)		/* too long a wait */
			return(-1);
		    else {				/* sleep then retry */
			if (i == 2 && lp->l_warn)
			    fputs(lp->l_warn,  stderr);
			sleep(lp->l_secs);
		    }
		    break;
		case 18:			/* node & file devices differ */
		    return(-3);
		default:			/* ??? */
		    return(-4);
		}
	    }
	}
	close(creat(lp->l_file, 0666));	/* touch file to set mod time */
	return(++lp->l_count);
}

unlock(lp)
struct	lockstr	*lp;
{
	register int i;

	if (--lp->l_count < 0) {
	    lp->l_count = 0;
	    return(-1);
	}
	if (lp->l_count == 0) {
	    if (unlink(lp->l_file) == -1)
		return(-2);
	    if (lp->l_done)
		for (i = NSSIG; --i > 0; signal(i, lp->l_sigs[i]));
	}
	return(lp->l_count);
}
