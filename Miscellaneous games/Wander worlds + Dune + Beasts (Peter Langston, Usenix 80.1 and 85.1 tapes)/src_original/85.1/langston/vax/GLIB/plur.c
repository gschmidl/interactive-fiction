char *
plur(n, singular, plural)
char	*singular, *plural;
{
	return(n == 1? singular : plural);
}

char	*
splur(n)
{
	return(n==1? "" : "s");
}

char	*
esplur(n)
{
	return(n==1? "" : "es");
}

char	*
iesplur(n)
{
	return(n==1? "y" : "ies");
}
