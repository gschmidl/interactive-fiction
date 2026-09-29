#include    <stdio.h>

/*
**      QUESTN -- Ask question and strip '\n' from response
**          (c) Peter S. Langston 1978
*/

char *
questn(quest, resp, max)
char    *quest, *resp;
{
	register char *cp, *bp;
	char buf[512];
	int len;

	printf(quest);
	if (max > sizeof buf) {
	    if (fgets(resp, max, stdin) == NULL) {
		*resp = '\0';
		return(0);
	    }
	    for (cp = resp; *cp != '\n'; cp++);
	} else {                   /* for short fields read into buf first */
	    bp = buf;
	    if (fgets(bp, sizeof buf, stdin) == NULL) {
		*resp = '\0';
		return(0);
	    }
	    --max;
	    for (cp = resp; (*cp = *bp++) && *cp != '\n'; cp++)
		if (cp - resp >= max)
		    break;
	}
	*cp = '\0';
	return(resp);
}
