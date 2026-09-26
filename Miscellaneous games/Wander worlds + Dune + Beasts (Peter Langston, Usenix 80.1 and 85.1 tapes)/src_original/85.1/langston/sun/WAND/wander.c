#include    "wanddef.h"
static	char    *sccs = "@(#)wander.c	1.1 7/6/84 -- (c) psl 1984";

main(argc, argv)
char	*argv[];
{
	char wrldpath[128];

	execl(WANDPATH(wander), "wander", WANDPATH(a3), argv[1], argv[2], 0);
	write(2, "Wander not exec'ed\n", 19);
}
