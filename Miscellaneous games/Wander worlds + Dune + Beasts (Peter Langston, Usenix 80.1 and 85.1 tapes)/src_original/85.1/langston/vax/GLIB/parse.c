/*
**	PARSE(string) -- Parse "string" into up to 32 individual words
**		and return a pointer to a static vector of pointers
**		to the words.
** Compile: cc -O -c -q parse.c; ar r ../glib.a parse.o
**      (c) psl 10/77
**          Modified to allow quoting of multi-word args 9/79, psl
*/

char **
parse(string)
char *string;
{
	register char *bp, *cp;
	register int argc;
	int quoted;
	static char *argp[32], argbuf[128];

	cp = string;
	bp = argbuf;
	argc = 0;
	do {
	    quoted = 0;
	    argp[argc++] = bp;
	    while (*cp == ' ')
		    cp++;
	    while (*cp != '\0'
	     && (*cp != ' ' || quoted == 1)) {
		if (*cp == '"') {
		    quoted ^= 1;
		    cp++;
		} else
		    *bp++ = *cp++;
	    }
	    *bp++ = '\0';
	} while (*cp++ != '\0' && argc < 32);
	while (argc < 32)
		argp[argc++] = "";
	return(argp);
}
