/*
**      GETCH -- Return a 7bit char from the given file handle
**      in sloppy style.
*/

getch(fh)
{
	char buf[1];

	if (read(fh, buf, 1) != 1)
	    return(-1);
	return(*buf & 0177);
}
