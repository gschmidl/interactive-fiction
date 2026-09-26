#include        <stdio.h>
#define LINELEN 80
#define LTLEN   (LINELEN - 12)	/* normal line length (room for tab, etc) */
#define	MAXENT	2048		/* won't handle entries longer than this */
#define	MAXFLD	1024		/* maximum number of fields we allow */

int     pflg    = 0;		/* output "packed" form, 1 line per entry */
				/* and delete comments & blank lines */
int	sflg	= 0;		/* sort the capability fields */
int	nllen	= LTLEN;	/* length we're trying for */

int	fldcomp();

extern	char	*copy();

main(argc, argv)
char    *argv[];
{
	register int i, c;
	register char *cp;
	char buf[MAXENT];

	while (--argc > 0) {
	    if (argv[argc][0] != '-') {
syntax:
		fprintf(stderr, "Usage: %s [-expand] [-pack] [-sort]\n",
		 argv[0]);
		exit(2);
	    }
	    switch (argv[argc][1]) {
	    case 'e':	/* expanded; output capabilities as single lines */
		nllen = 1;
		break;
	    case 'p':	/* packed; output entries as single lines */
		pflg++;
		break;
	    case 's':	/* sorted; alphabetize the fields in each entry */
		sflg++;
		break;
	    default:
		goto syntax;
	    }
	}
	while (fgets(cp = buf, sizeof buf, stdin) != NULL) {
	    do {
		i = strlen(buf) - 2;
		if (i < 0 || buf[i] != '\\')
		    break;
		while ((c = getc(stdin)) != EOF && c != ':');
		if (c == EOF)
		    break;
	    } while (fgets(cp = &buf[i], sizeof buf - i, stdin) != NULL);
	    if (*buf == '#' || *buf < ' ') {
		if (pflg == 0)
		    fputs(buf, stdout);
	    } else {
		if (sflg)
		    dosort(buf);
		if (pflg)
		    fputs(buf, stdout);
		else
		    dolines(buf);
	    }
	}
}

dolines(buf)
char    *buf;
{
	register char *bp, *cp, *ep, *pp;

	for (cp = buf; *cp != ':'; ) {
	    if (*cp++ == '\0') {
		fputs("ERROR IN LINE THAT SAYS: ", stderr);
		fputs(buf, stderr);
		exit(1);
	    }
	}
	*cp++ = '\0';
	fputs(buf, stdout);
	fputs(":\\\n\t:", stdout);
	pp = bp = cp;
	for (ep = &bp[nllen]; *cp >= ' '; cp++) {
	    if (cp >= ep && pp != bp) {
		*pp++ = '\0';
		fputs(bp, stdout);
		fputs(":\\\n\t:", stdout);
		bp = pp;
		ep = &bp[nllen];
	    } else if (*cp == ':')
		pp = cp;
	}
	fputs(bp, stdout);
}

dosort(buf)
char	*buf;
{
	register char *bp;
	register int nf, i;
	char buf2[MAXENT], *flds[MAXFLD];

	nf = 0;
	for (bp = buf; *bp; ) {
	    if (*bp++ == ':') {
		bp[-1] = '\0';
		if (*bp != '\n')
		    flds[nf++] = bp;
	    }
	}
	qsort(flds, nf, sizeof flds[0], fldcomp);
	bp = copy(buf, buf2);		/* pick up terminal name */
	for (i = 0; i < nf; i++) {
	    *bp++ = ':';
	    bp = copy(flds[i], bp);
	}
	copy(":\n", bp);
	copy(buf2, buf);
}

fldcomp(f1, f2)
char	**f1, **f2;
{
	register char *cp1, *cp2;

	cp1 = *f1;
	if (cp1[0] == 't' && cp1[1] == 'c' && cp1[2] == '=')
	    return(1);
	cp2 = *f2;
	if (cp2[0] == 't' && cp2[1] == 'c' && cp2[2] == '=')
	    return(-1);
	return(strcmp(cp1, cp2));
}

char    *
copy(from, to)
register char *from, *to;
{
	while (*to++ = *from++);
	return(--to);
}
