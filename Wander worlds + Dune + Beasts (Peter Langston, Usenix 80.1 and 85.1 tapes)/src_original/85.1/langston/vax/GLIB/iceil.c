iceil(arg)
double arg;
{
	register int i;

	i = arg;
	return(i >= arg? i : i + 1);
}

ifloor(arg)
double arg;
{
	register int i;

	i = arg;
	return(i <= arg? i : i - 1);
}
