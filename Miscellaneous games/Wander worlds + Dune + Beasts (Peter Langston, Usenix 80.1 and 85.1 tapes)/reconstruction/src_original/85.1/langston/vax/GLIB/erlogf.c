/*
**	ERLOGF	-- Append an error message to an error log file
**	psl 2/85
*/

#include	<stdio.h>

int	nocreate;		/* != 0 to avoid creating erlogfil */

extern	char	*erlogfil;	/* must be defined by mainline */

erlogf(string)
char    *string;
{
	char buf[128];
	long now;
	FILE *efp;

	if (nocreate) {
	    if ((efp = fopen(erlogfil, "r")) == NULL)
		return;
	    fclose(efp);
	}
	if ((efp = fopen(erlogfil, "a")) != (FILE *) NULL) {
	    time(&now);
	    fprintf(efp, "%19.19s | %s\n", ctime(&now), string);
	    fclose(efp);
	}
}
