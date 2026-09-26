#include    <stdio.h>
/*
**      BPACK -- Packing scheme particularly suited to word lists.
**          Packed form contains no weird characters (ok to psl/net).
**          Word list must not contain any words starting with a digit.
**          Result is usually 55-65% of original size.
*/

static  char    *whatsccs = "@(#)bpack.c	1.2 2/18/85 -- (c) psl 1981";

#define DEFLEN  3       /* most common repeat length */

main(argc, argv)
char    *argv[];
{
	if (argc == 2 && argv[1][0] == '-') {
	    if (argv[1][1] == 'p')
		pack();
	    else if (argv[1][1] == 'u')
		unpack();
	}
	fprintf(stderr, "Usage: %s -pack <unpacked >packed\n", argv[0]);
	fprintf(stderr, "   or: %s -unpack <packed >unpacked\n", argv[0]);
	exit(2);
}

pack()
{
	register int next, last, i;
	char buf[2][128];

	next = 1;
	last = 0;
	buf[last][0] = '\0';
	while (fgets(buf[next], sizeof buf[0], stdin) != NULL) {
	    for (i = 0; buf[next][i] == buf[last][i]; i++);
	    if (i != DEFLEN)
		printf("%d", i);
	    fputs(&buf[next][i], stdout);
	    last = next;
	    next ^= 1;
	}
	exit(0);
}

unpack()
{
	register char *ip, *op;
	register int i;
	char ibuf[128], obuf[128];

	while (fgets(ibuf, sizeof ibuf, stdin) != NULL) {
	    for (ip = ibuf; *ip >= '0' && *ip <= '9'; ip++);
	    if (ip == ibuf)
		i = DEFLEN;
	    else
		i = atoi(ibuf);
	    for (op = &obuf[i]; *op++ = *ip++; );
	    fputs(obuf, stdout);
	}
	exit(0);
}
