/* The bare minimum needed to make the untouched 1985 sources RUN on a
   64-bit host: nothing here changes behaviour, it only stops undeclared
   functions from being assumed to return int (which truncates every
   pointer they hand back).  Used for the Linux reference build only --
   the Windows port gets the same thing from port/portcompat.h. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
char *conv();
int   advtolower();
#define tolower advtolower
