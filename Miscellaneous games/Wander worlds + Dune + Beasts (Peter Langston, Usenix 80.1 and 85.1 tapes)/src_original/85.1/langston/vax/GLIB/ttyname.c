/*
**      TTYNAME -- Imitation of V7 ttyname()
**      NOTE: delete this from the archive if you're on a V7 system
*/

#ifdef V6SYS
char    *
ttyname(fh)
{
	register char *name;

	name = "/dev/ttyx";
	name[8] = ttyn(fh);
	return(name);
}
#endif
