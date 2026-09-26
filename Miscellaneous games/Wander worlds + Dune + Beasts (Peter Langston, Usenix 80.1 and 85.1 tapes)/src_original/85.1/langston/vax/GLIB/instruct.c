#include <stdio.h>
/*
**      INSTRUCT -- Type given file on standard output, pausing for
**          <RETURN>s after every screenful.
*/

instruct(file)
char    *file;
{
	char buf[255], line;
	FILE *ifp;

	if ((ifp = fopen(file, "r")) == NULL) {
	    perror(file);
	    return(-1);
        }
	for (line = 2; fgets(buf, 255, ifp) != NULL; line++) {
	    if (line > 22) {
		fputs(buf, stdout);
		buf[0] = '\n';
	    }
	    if ((buf[0] == '\n' && line > 16) || buf[0] == 014) {
		read(0, buf, 255);
		line = 0;
	    } else
		fputs(buf, stdout);
	}
	fclose(ifp);
	if (line > 1)
	    read(0, buf, 255);
	return(0);
}
