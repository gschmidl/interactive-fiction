#include    <pwd.h>
/*
**      UIDNAM -- Return name from uid
*/

#define	RV8

#ifdef	RV8		/* also probably works for V6 */

char    *
uidnam(uid)
{
	register char *bp;
	static char lastbuf[128];
	static int lastuid;

	if (uid == lastuid && uid != 0)
	    return(lastbuf);
	lastuid = uid;
	bp = lastbuf;
	if (getpw(uid, bp) == 0)
	    for (; *bp && *bp != ':'; bp++);
	*bp = '\0';
	return(lastbuf);
}
#endif	RV8

#ifdef	BSD
char    *
uidnam(uid)
{
	register char *bp;
	register char *cp;				/* V7 */
	static char lastbuf[128];
	static int lastuid;
	struct passwd *pwp;				/* V7 */
	extern struct passwd *getpwuid();		/* V7 */

	if (uid == lastuid && uid != 0)
	    return(lastbuf);
	lastuid = uid;
	bp = lastbuf;
	if ((pwp = getpwuid(uid)) == 0)			/* V7 */
	    *bp = '\0';					/* V7 */
	else						/* V7 */
	    for (cp = pwp->pw_name; *bp++ = *cp++; );	/* V7 */
/*	if (getpw(uid, bp) == 0)			/* V6 */
/*	    for (; *bp && *bp != ':'; bp++);		/* V6 */
/*	*bp = '\0';					/* V6 */
	return(lastbuf);
}
#endif	BSD
