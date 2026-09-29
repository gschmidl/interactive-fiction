#include	<stdio.h>
#include	<time.h>
#include	"../gamesdef.h"
/*
**	QOTD -- Quote Of The Day
**	File named in qfile must have entries of the form:
**	mmdd{quote}	where mm is 2 digit month and begins a new line,
**			dd is 2 digit day, and "quote" can contain any
**			character but '}', including newline.
**	qfile should be mode 600; qotd should be mode 4755.
*/

static  char    *whatsccs = "@(#)qotd.c	1.1 9/8/84 -- (c) psl 1983";
static	char	*qfile	= GAMESPATH(TODAY/qotd.txt);

main(argc, argv)
char	*argv[];
{
	register char *bp, *cp;
	register int m, d;
	char buf[512], pat[8];
	long now;
	struct tm *tp, *localtime();
	FILE *qfp;

	if ((qfp = fopen(qfile, "r")) == 0) {
	    perror(qfile);
	    exit(3);
	}
	time(&now);
	if (argc == 2 && getuid() == PRVUID)
	    now = now + 60l * 60l * 24l * atoi(argv[1]);
	tp = localtime(&now);
	m = tp->tm_mon + 1;
	d = tp->tm_mday;
	sprintf(pat, "%d%d%d%d", m / 10, m % 10, d / 10, d % 10);
	while (fgets(buf, sizeof buf, qfp) != 0) {
	    if (buf[4] == '{'
	     && buf[3] == pat[3]
	     && buf[2] == pat[2]
	     && buf[1] == pat[1]
	     && buf[0] == pat[0]) {
		cp = &buf[5];
		do {
		    for (bp = cp; *bp && *bp != '}'; bp++);
		    if (*bp == '}')
			break;
		    fputs(cp, stdout);
		} while (fgets(cp = buf, sizeof buf, qfp) != 0);
		*bp++ = '\n';
		*bp = '\0';
		fputs(cp, stdout);
		exit(0);
	    }
	}
	exit(1);
}
