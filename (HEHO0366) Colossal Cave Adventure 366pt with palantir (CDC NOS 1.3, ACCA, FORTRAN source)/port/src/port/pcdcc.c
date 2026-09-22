/* The one thing the port needs from the host: where its own .exe is, so
   that adventure.txt - TAPE1, the database - is found however the game is
   started.  Same helper as the other ports in this collection. */
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

void pgmdir_(char *buf, size_t len)
{
    size_t i, n = 0;

    memset(buf, ' ', len);
#ifdef _WIN32
    {
        char path[MAX_PATH];
        DWORD k = GetModuleFileNameA(NULL, path, sizeof path);
        for (i = 0; i < k; i++)
            if (path[i] == '\\' || path[i] == '/')
                n = i + 1;
        if (n > len)
            n = 0;
        memcpy(buf, path, n);
    }
#endif
    (void)n;
}
