/*
** Atoip - convert from octal or decimal string to integer.
**	- use pointer to pointer to chars and update it.
*/

atoip(ptrptr)
char **ptrptr;
{
	register num, base;
	register char *cp;
	int neg;

	cp = *ptrptr;
	num = 0;
	base = 10;
	neg = 0;
loop:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '-') {
		neg++;
		cp++;
		goto loop;
	}
	if (*cp == '0') {
		base = 8;
		cp++;
	}
	while (*cp >= '0' && *cp <= '9')
		num = num * base + *cp++ - '0';
	*ptrptr = cp;
	return(neg ? -num : num);
}
