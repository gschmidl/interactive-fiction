/*
**  STMTCH  -  match minimal abbreviation from a struct array
**          n = stmtch(sp, strp, strlen);
**	    int n, strlen; char *sp, strp;
**	sp points at a string to be matched
**	strp points to a pointer to the first match string (i.e. the
**		char pointer in the first struct)
**	strlen is the length (in bytes) of the struct,
**		i.e. the number of bytes to the next char pointer
**	return int is the index into the struct or
**	NO_MATCH if no match was found or
**	AMBIGUOUS if the string was ambiguous
**
**	struct foo {				an example
**		int	f_val;			some junk
**		char	*f_mtch;		the thing to be matched
**	} t[] = { {  20, "able", },		some test data
**		  {  30, "baker", },		 "
**		  {  10, "charlie", },		 "
**		  {  0,	0, },			0 in f_mtch ends the search
**	};
**	n = stmtch("bake", t[0].foo_mtch, sizeof t[0]);
**				(n should now be set to 1)
**
**  SEQEQ  -  check for string equality or initial sequence equality
**          n = seqeq(sp1, sp2);
**	    char *sp1, sp2;
**	sp1 and sp2 point at the strings to be compared
**	return int is:
**	NOT_EQUAL is the strings are different,
**	HEAD_EQUAL if one string is an initial sequence of the other, or
**	EXACT_EQUAL if the strings are exactly equal
*/

#include	"../stmtch.h"

stmtch(sp, strp, strlen)
char *sp, **strp;
{
	register char *cp;
	register int retval, i, j;

	retval = -1;
	cp = sp;
	i = 0;
	do {
	    if ((j = seqeq(cp, *strp)) != NOT_EQUAL) {
		if (j == EXACT_EQUAL)
		    return(i);
		if (retval != NO_MATCH)
		    return(AMBIGUOUS);
		retval = i;
	    }
	    i++;
		/* a sleazy, but necessary, hack */
	    strp = (char **) ((char *) strp + strlen);
	} while (*strp);
	return(retval);
}

seqeq(s1, s2)
char *s1, *s2;
{
        register char *cp1, *cp2;

        cp1 = s1;
        cp2 = s2;
	do {
            if (*cp1++ != *cp2++)
                return(0);
	} while (*cp2 != ' ' && *cp1 != '\0');
	if (*cp1 == '\0' && (*cp2 == ' ' || *cp2 == '\0'))
	    return(2);                                            /* exact */
	return(1);                                          /* abbreviated */
}
