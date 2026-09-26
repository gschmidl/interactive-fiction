/*
**      MESGOFF/MESGON -- System dependent routines to disallow/allow messages
*/

mesgoff()
{
	system("mesg n");
}

mesgon()    /* really should remember whether it was on before... */
{
	system("mesg y");
}
