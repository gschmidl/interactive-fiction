/*
**	VOPEN -- Verbose open
*/

vopen(name, mode)
{
	register int fh;

	fh = open(name, mode);
	if (fh < 0)
		perror(name);
	return(fh);
}
