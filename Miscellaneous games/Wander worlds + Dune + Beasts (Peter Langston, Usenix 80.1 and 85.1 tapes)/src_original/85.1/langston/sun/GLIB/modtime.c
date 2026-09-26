#include    <sys/types.h>
#include    <sys/stat.h>

long
modtime(file)		/* return the modification time of file */
char    *file;
{
	struct stat sb;

	if (stat(file, &sb) == -1)
	    return(-1L);
	return(sb.st_mtime);
}
