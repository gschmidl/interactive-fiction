/* brtc.c - what the run-time (brt.f90) needs from C: the folder of
 * dungeon.exe, whether stdin is a console, and making a folder. */
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define isatty _isatty
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/* the folder of this program, ending in a separator, blank-padded */
void bexep_(char *buf, size_t len)
{
    char p[1024];
    char *s;
    size_t n;

#ifdef _WIN32
    n = GetModuleFileNameA(NULL, p, sizeof p);
    if (n == 0 || n >= sizeof p)
        strcpy(p, ".\\x");
#else
    ssize_t k = readlink("/proc/self/exe", p, sizeof p - 1);

    if (k <= 0)
        strcpy(p, "./x");
    else
        p[k] = 0;
#endif
    s = strrchr(p, '\\');
    if (!s || (strrchr(p, '/') && strrchr(p, '/') > s))
        s = strrchr(p, '/');
    if (s)
        s[1] = 0;
    else
        strcpy(p, "./");
    n = strlen(p);
    if (n > len)
        n = len;
    memcpy(buf, p, n);
    memset(buf + n, ' ', len - n);
}

long long bisatty_(void)
{
    return isatty(0) ? 1 : 0;
}

void bmkdir_(const char *path, size_t len)
{
    char p[1024];

    if (len >= sizeof p)
        len = sizeof p - 1;
    memcpy(p, path, len);
    p[len] = 0;
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0777);
#endif
}
