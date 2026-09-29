/*
** Atoi - convert from octal or decimal string to integer.
*/

atoi(ptr)
char *ptr;
{
	register num, base;
	register char *cp;
	int neg;

	cp = ptr;
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
	return(neg ? -num : num);
}
