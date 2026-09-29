/* pharrisc.c - what ABENTEUER's Harris routines need that Fortran cannot
 * do: the program's own directory (PINIT), and IO - reading and writing
 * the eleven stretches of COMMON the main program lists by address and
 * length (PCLOAD, PCSAVE, for LDCOMN and SVCOMN in pharris.f).
 *
 * A saved game is those stretches one after another, in this port's
 * layout (src\neuspiel.py turns the site's own NEUSPIEL into it); it is
 * read back only if its length is exactly theirs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <direct.h>
#include <windows.h>

/* PINIT: into the program's directory (ADV.DATA and NEUSPIEL.DAT are
   there), and a saves\ there */
void pinit_(void)
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, sizeof path);
    if (n > 0 && n < sizeof path) {
        char *p = strrchr(path, '\\');
        if (p) {
            *p = 0;
            if (_chdir(path) != 0) {
                fprintf(stderr, "abenteuer: cannot go to %s\n", path);
                exit(1);
            }
        }
    }
    _mkdir("saves");
}

static void cname(char *out, size_t size, const char *name, size_t len)
{
    size_t k = len < size - 1 ? len : size - 1;
    memcpy(out, name, k);
    while (k > 0 && out[k - 1] == ' ') k--;
    out[k] = 0;
}

/* CMADDR(4,11): the address of each stretch in word 1 of its column;
   CMSIZE(11): its length in bytes */
static int64_t total(const int64_t *cmsize)
{
    int64_t t = 0;
    int i;
    for (i = 0; i < 11; i++) t += cmsize[i];
    return t;
}

void pcload_(const char *name, int64_t *cmaddr, int64_t *cmsize,
             int32_t *ierr, size_t namelen)
{
    char fn[260];
    FILE *f;
    long len;
    int i;
    cname(fn, sizeof fn, name, namelen);
    *ierr = 1;
    f = fopen(fn, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len != total(cmsize)) {
        fprintf(stderr, "abenteuer: %s is not a saved game of this "
                "program (%ld bytes, not %lld)\n", fn, len,
                (long long)total(cmsize));
        fclose(f);
        return;
    }
    for (i = 0; i < 11; i++) {
        void *a = (void *)(intptr_t)cmaddr[4 * i];
        if (fread(a, 1, (size_t)cmsize[i], f) != (size_t)cmsize[i]) {
            fclose(f);
            return;
        }
    }
    fclose(f);
    *ierr = 0;
}

void pcsave_(const char *name, int64_t *cmaddr, int64_t *cmsize,
             int32_t *ierr, size_t namelen)
{
    char fn[260];
    FILE *f;
    int i;
    cname(fn, sizeof fn, name, namelen);
    *ierr = 1;
    f = fopen(fn, "wb");
    if (!f) return;
    for (i = 0; i < 11; i++) {
        const void *a = (const void *)(intptr_t)cmaddr[4 * i];
        if (fwrite(a, 1, (size_t)cmsize[i], f) != (size_t)cmsize[i]) {
            fclose(f);
            return;
        }
    }
    if (fclose(f) == 0) *ierr = 0;
}
