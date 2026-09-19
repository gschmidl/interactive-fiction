#include	<stdio.h>

main(argc, argv)
char	*argv[];
{
	char buf[128];

	printf("You only get 1 line (128 chars)...\n");
	fgets(buf, sizeof buf, stdin);
	anonmail(argv[1], argv[2], buf);
}
