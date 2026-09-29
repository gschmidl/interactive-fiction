/* PORT: the directory the executable sits in, so that the game finds
   advtdata.txt / advent.ini / saves\ wherever it is started from.  On MVS
   these were DD cards in the JCL (FT01F001, FT02F001, FT03F001).

   Called from Fortran as  CALL PGMDIR(BUF,LEN)  with BUF CHARACTER*(*);
   gfortran passes the length of BUF as a hidden trailing argument.  The
   returned text ends with the path separator, or is empty if the
   directory cannot be worked out (then the files are looked for in the
   current directory, which is what the build does anyway).  */

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define PSEP '\\'
#else
#include <unistd.h>
#define PSEP '/'
#endif

void pgmdir_(char *buf, int *len, size_t buflen)
{
    char path[4096];
    size_t n = 0;
    char *p;

    path[0] = '\0';
#ifdef _WIN32
    n = (size_t) GetModuleFileNameA(NULL, path, (DWORD) sizeof path);
    if (n >= sizeof path) n = 0;
#else
    {
        ssize_t k = readlink("/proc/self/exe", path, sizeof path - 1);
        if (k > 0) { path[k] = '\0'; n = (size_t) k; }
    }
#endif
    path[n] = '\0';
    p = strrchr(path, PSEP);
#ifdef _WIN32
    { char *q = strrchr(path, '/'); if (q > p) p = q; }
#endif
    if (p == NULL) n = 0; else n = (size_t) (p - path) + 1;
    if (n > buflen) n = 0;

    memcpy(buf, path, n);
    memset(buf + n, ' ', buflen - n);
    *len = (int) n;
}
