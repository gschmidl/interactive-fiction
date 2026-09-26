#include "sd_def.h"
/*
**      STARDREK -- Interface to Stardrek program
**          (Used principally to setuid to sd_mother's uid)
**      (c) P. Langston, NYC, 1979
*/

char    *whatsccs = "@(#)stardrek.c	2.6  last mod 3/13/84 -- (c) psl 1979";

main(argc, argv)
char	*argv[];
{
	sysinit("stardrek", argc, argv);
	execl(log_prog, "st", 0);
	perror(log_prog);
	exit(3);
}
