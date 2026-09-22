/* PORT: the three site-supplied routines of Palter's conversion guide, and
   the file transfer that stands in for PRIMOS's PRWF$$.

   The main program builds a table of the eleven COMMON block ranges with
   ADDR and SIZE, and the save and restore routines hand that table to the
   system to be written or read.  ADDR's comment in the source says CMADRS
   "allows four contiguous INTEGER variables for each pointer", which is just
   as well: a pointer here is eight bytes, and it goes in the first two of
   them.

   The Prime was a big-endian machine whose LOC returned a 16-bit word
   address, so its SIZE counted halfwords and PRWF$$ transferred halfwords -
   two per INTEGER, compiled -INTL.  SIZE here counts 4-byte words and PXFER
   transfers those, which comes to the same number of bytes, so the original
   ADVCOM file converts to this one by swapping each group of four bytes
   (tests\advcom.py).  */

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define PSEP '\\'
#else
#include <unistd.h>
#define PSEP '/'
#endif

/* SUBROUTINE ADDR(A,B) - return the address of A in B */
void addr_(void *a, void *b)
{
    void *p = a;
    memcpy(b, &p, sizeof p);
}

/* INTEGER*4 FUNCTION SIZE(A,B) - words in the range LOCF(A):LOCF(B) */
int size_(int *a, int *b)
{
    return (int) (b - a) + 1;
}

/* SUBROUTINE PXFER(PATH,CMADDR,CMSIZE,N,RW,OK) - the eleven ranges to or
   from a file.  OK is 0 for done and 1 for could not open.  */
void pxfer_(char *path, const int *cmaddr, const int *cmsize, const int *n,
            const int *rw, int *ok, size_t pathlen)
{
    char name[4096];
    FILE *f;
    int i;

    *ok = 1;
    if (pathlen >= sizeof name) return;
    memcpy(name, path, pathlen);
    name[pathlen] = '\0';
    while (pathlen > 0 && name[pathlen - 1] == ' ') name[--pathlen] = '\0';

    f = fopen(name, *rw ? "wb" : "rb");
    if (f == NULL) return;

    for (i = 0; i < *n; i++) {
        void *p;
        size_t words = (size_t) (cmsize[i] > 0 ? cmsize[i] : 0);
        memcpy(&p, cmaddr + 4 * i, sizeof p);
        if (words == 0) continue;
        if (*rw) {
            if (fwrite(p, 4, words, f) != words) break;
        } else {
            if (fread(p, 4, words, f) != words) break;
        }
    }
    fclose(f);
    *ok = 0;
}

/* SUBROUTINE PGMDIR(BUF,LEN) - the directory the executable sits in, so the
   game finds adv.data, adv.line and saves\ wherever it is started from. */
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
        if (k > 0) n = (size_t) k;
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
