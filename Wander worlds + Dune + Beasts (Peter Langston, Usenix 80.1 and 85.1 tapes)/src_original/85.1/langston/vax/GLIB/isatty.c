/*
**	ISATTY -- Return 1 if arg is a tty; else 0.
*/

isatty(fh)
{
	if (ttyn(fh) == 'x')
	    return(0);
	return(1);
}
