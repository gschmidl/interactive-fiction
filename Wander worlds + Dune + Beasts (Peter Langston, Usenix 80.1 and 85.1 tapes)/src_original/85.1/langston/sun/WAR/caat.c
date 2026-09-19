main(argc, argv)
char	*argv[];
{
	register int fh, skip;
	char buf[16];

	if (argc > 1)
	    skip = atoi(argv[1]) - 1;
	while (read(0, buf, 1) == 1) {
	    write(1, buf, 1);
	    if (*buf != ']')
		continue;
	    if (skip > 0)
		--skip;
	    else {
		while (read(0, buf, 1) == 1 && *buf != '\n')
		    write(1, buf, 1);
		read(2, buf, 16);
	    }
	}
}
