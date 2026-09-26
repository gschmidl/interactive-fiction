/*
**	SENDTEXT -- Mail text to specified user
**	Uses "sendmsg()" from glib.a
*/

sendtext(user, text)
char	*user, *text;
{
	register char *cp;
	register int count, savc;
	int pfh[2];

	pipe(pfh);		/* this fails for VERY long messages */
	for (cp = text, count = 4095; *cp && --count > 0; cp++);
	savc = cp[-1];
	if (cp[-1] != '\n' && count > 0) {
	    savc = cp[0];
	    *cp++ = '\n';
	}
	write(pfh[1], text, cp - text);
	close(pfh[1]);
	sendmsg(user, pfh[0]);
	close(pfh[0]);
	cp[-1] = savc;
}
