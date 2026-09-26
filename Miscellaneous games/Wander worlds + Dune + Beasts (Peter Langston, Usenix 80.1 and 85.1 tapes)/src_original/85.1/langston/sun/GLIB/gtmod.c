/*
**      GTMOD -- Return mode of specified file or -1 on error.
*/

#include    <sys/types.h>
#include    <sys/stat.h>

short
gtmod(file)        /* return file mode */
{
	struct stat st;

	if (stat(file, &st) == -1)
	    return(-1);			/* could be fooled, but ... */
	return(st.st_mode);
}
