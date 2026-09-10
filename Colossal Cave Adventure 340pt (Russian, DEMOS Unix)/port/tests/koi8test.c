/* Feed a KOI8-R file through the port's console encoder and write the
   UTF-16LE units the console would receive, so the mapping can be checked
   against an independent decoder.  Used by tests/verify.sh. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

unsigned koi8_to_utf16(const unsigned char *p, unsigned n,
                       WCHAR *w, unsigned wmax, unsigned *used);

int main(int argc, char **argv)
{
    static unsigned char in[262144];
    static WCHAR out[262144];
    FILE *f;
    size_t n;
    unsigned used, k, off = 0;

    if (argc != 3) { fprintf(stderr, "usage: koi8test in.koi8 out.utf16\n"); return 2; }
    if (!(f = fopen(argv[1], "rb"))) { perror(argv[1]); return 1; }
    n = fread(in, 1, sizeof in, f);
    fclose(f);

    if (!(f = fopen(argv[2], "wb"))) { perror(argv[2]); return 1; }
    while (off < n) {
        k = koi8_to_utf16(in + off, (unsigned)(n - off), out,
                          (unsigned)(sizeof out / sizeof out[0]), &used);
        if (!used) break;
        fwrite(out, sizeof(WCHAR), k, f);
        off += used;
    }
    fclose(f);
    return 0;
}
