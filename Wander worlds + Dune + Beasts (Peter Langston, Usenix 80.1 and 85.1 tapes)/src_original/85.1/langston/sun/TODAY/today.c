#include	<stdio.h>
#include	<time.h>
#include	"../gamesdef.h"
/*
**	TODAY -- Reveal intteresting facts about today.
**	File named in dfile must have entries of the form:
**	mm/dd\tfact	where "mm" is 2 digit month and begins a new line,
**			"dd" is 2 digit day, "\t" is a tab, and "fact" can
**			contain any text delimited by a newline.
**	Lines will be examined for dates in the form: {19XX}  If such a date
**	exists it will be checked for currency.
**
**	dfile should be mode 600; today should be mode 4755.
*/

static  char    *whatsccs = "@(#)today.c	1.2 9/17/84 -- (c) psl 1983";
static	char	*dfile	= GAMESPATH(TODAY/dates);

main(argc, argv)
char	*argv[];
{
	register char *cp;
	register int m, d, yrok;
	char buf[512], mdpat[8], yrpat[8];
	long now;
	struct tm *tp, *localtime();
	FILE *dfp;

	if ((dfp = fopen(dfile, "r")) == 0) {
	    perror(dfile);
	    exit(3);
	}
	time(&now);
	if (argc == 2 && getuid() == PRVUID)
	    now = now + 60l * 60l * 24l * atoi(argv[1]);
	tp = localtime(&now);
	m = tp->tm_mon + 1;
	d = tp->tm_mday;
	sprintf(mdpat, "%d%d/%d%d", m / 10, m % 10, d / 10, d % 10);
	sprintf(yrpat, "%02d", tp->tm_year);
	while (fgets(buf, sizeof buf, dfp) != 0) {
	    if (buf[5] == '\t'
	     && buf[4] == mdpat[4]
	     && buf[3] == mdpat[3]
	     && buf[2] == mdpat[2]
	     && buf[1] == mdpat[1]
	     && buf[0] == mdpat[0]) {
		yrok = 1;
		for (cp = &buf[6]; *cp; cp++) {
		    if (cp[0] == '}'
		     && cp[-5] == '{'
		     && cp[-4] == '1'
		     && cp[-3] == '9') {
			if (cp[-2] == yrpat[0]
			 && cp[-1] == yrpat[1]) {
			    yrok = 1;
			    break;
			} else
			    yrok = 0;
		    }
		}
		if (yrok)
		    fputs(&buf[6], stdout);
	    }
	}
	exit(1);
}
