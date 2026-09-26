/*
**	EQUAL -- Return 1 if string args exactly equal, 0 otherwise
*/

equal(ap, bp)
register char *ap, *bp;
{
	while (*ap++ == *bp)
	    if (*bp++ == '\0')
		return(1);
	return(0);
}
