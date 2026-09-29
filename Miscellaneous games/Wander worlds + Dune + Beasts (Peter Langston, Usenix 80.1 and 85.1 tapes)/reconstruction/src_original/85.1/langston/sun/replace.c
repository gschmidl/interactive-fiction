#include    <stdio.h>
/*
**      REPLACE -- Replace all occurrences of "old" with "new"
**          in specified files.
**          Will do multiple files, preserves links and makes NO backup
**      Copyright (c) 1976, by Peter S. Langston
*/

static  char    *whatline = "@(#)replace.c	1.5 1/30/83 -- P.S.L. misc";

#define EX_OK   0
#define EX_BAD  1
#define EX_SYN  2
#define EX_SYS  3

#define SPLUR(n) (n == 1? "" : "s")

char	*old	= 0;
char	*new	= 0;
int	ffile	= 0;
int	iflg	= 0;
int	qflg	= 0;
FILE    *ifp, *ofp;

char    *unesc();

main(argc, argv)
char    *argv[];
{
	register char *file;
	char tmpfil[16];
	int argn, count, total;

	for (argn = 1; argn < argc; argn++) {
	    if (argv[argn][0] == '-') {
		switch (argv[argn][1]) {
		case 'q':				/* -q => quiet */
		    qflg++;
		    break;
		case 'i':				/* -i => ignore unwritable files */
		    iflg++;
		    break;
		default:
		    goto oops;
		}
	    } else {
		if (old == 0)
		    old = unesc(argv[argn]);
		else if (new == 0)
		    new = unesc(argv[argn]);
		else
		    break;
	    }
	}
	if (new == 0) {
oops:
	    printf("Usage: %s [-i] [-q] old new file1 [ file2 ... ]\n", argv[0]);
	    exit(EX_SYN);
	}
	setbuf(stdout, 0);
	total = 0;
	sprintf(tmpfil, "replace%d", getpid());
	while (argn < argc) {
	    file = argv[argn++];
	    if (access(file, 6) == -1) {
		perror(file);
		if (iflg)
		    continue;
		exit(EX_SYS);
	    }
	    if ((ifp = fopen(file, "r")) == NULL) {
		perror(file);
		exit(EX_SYS);
	    }
	    if (qflg == 0)
		printf("%s: ", file);
	    if ((ofp = fopen(tmpfil, "w")) == NULL) {
		perror(tmpfil);
		exit(EX_SYS);
	    }
	    count = replace(ifp, ofp);
	    total += count;
	    fclose(ifp);
	    fclose(ofp);
	    if (count > 0) {
		if (qflg == 0)
		    printf("%d replacement%s", count, SPLUR(count));
		fcopy(tmpfil, file);
	    }
	    if (unlink(tmpfil) < 0) {
		perror(tmpfil);
		exit(EX_SYS);
	    }
	    if (qflg == 0)
		printf("\n");
	}
	if (qflg == 0)
	    printf("        Total replacements : %d\n", total);
	exit(EX_OK);
}

replace(inp, outp)
FILE	*inp, *outp;
{
	register char *cp, *ccp;
	register int c;
	int count;

	count = 0;
	cp = old;
	c = getc(inp);
	do {
	    if (c != *cp) {
		putc(c, outp);
		c = getc(inp);
		continue;
	    }
	    while ((c = getc(inp)) == *++cp && *cp);
	    if (*cp) {
		ccp = old;
		while (ccp < cp)
		    putc(*ccp++, outp);
	    } else {
		cp = new;
		while (*cp)
		    putc(*cp++, outp);
		count++;
	    }
	    cp = old;
	} while (c != -1);
	return(count);
}

char    *
unesc(str)
char    *str;
{
	register char *sp, *cp;

	for (cp = sp = str; *cp = *sp++; cp++) {
	    if (*cp == '\\') {
		*cp = hex(*sp++) << 4;
		*cp |= hex(*sp++);
	    }
	}
	return(str);
}

hex(c)
register char c;
{
	if (c >= '0' && c <= '9')
	    return(c - '0');
	return((c | 040) - 'a' + 10);
}

fcopy(from, to)
char	*from, *to;
{
	register int i, ffh, tfh;
	char buf[1024];

	if ((ffh = open(from, 0)) < 0) {
	    perror(from);
	    exit(EX_SYS);
	}
	if ((tfh = creat(to, 0600)) < 0) {
	    perror(to);
	    exit(EX_SYS);
	}
	while ((i = read(ffh, buf, sizeof buf)) > 0) {
	    if (write(tfh, buf, i) != i) {
		perror(to);
		exit(EX_SYS);
	    }
	}
	close(ffh);
	close(tfh);
}
