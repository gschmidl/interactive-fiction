/*
**	ERLOGM	-- Mail an error message to some specific user
**	psl 2/85
*/

erlog(string, erlogname)
char	*string, *erlogname;
{
	if (erlogname != 0 && *erlogname != '\0')
	    sendtext(erlogname, string);
}
