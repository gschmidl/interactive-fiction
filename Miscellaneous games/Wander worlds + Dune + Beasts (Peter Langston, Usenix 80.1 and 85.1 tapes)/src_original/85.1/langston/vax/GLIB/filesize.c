#include    <sys/types.h>
#include    <sys/stat.h>

long
filesize(file)		/* return the size of file */
char    *file;
{
	struct stat sb;

	if (stat(file, &sb) == -1)
	    return(-1L);
	return(sb.st_size);
}
