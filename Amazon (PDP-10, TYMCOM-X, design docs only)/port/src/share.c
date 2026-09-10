/*  share.c -- the shared Amazon River Valley.
 *
 *  AMAZON.TXT calls itself "A game of fun, daring, suprises, and multiple
 *  players", and PRTEST.SAI (03-Sep-77) shows how that was meant to work:
 *  one SAIL process is SPROUTed per TTY, every table is dimensioned
 *  [1:16], and a ring of user!message[0:40] carries lines between them.
 *
 *  There are no TTYs to sprout here, so instead every copy of AMAZON.EXE
 *  opens the same world file, takes an exclusive lock on it for the
 *  duration of a turn, and drops the lock while the player is typing.
 *  Turns take seconds and locks take microseconds, so the contention is
 *  the same as it was on a KI10 with sixteen people on it.
 *
 *  The 40-entry message ring and the 16-player limit are PRTEST.SAI's.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "amazon.h"

static HANDLE hfile = INVALID_HANDLE_VALUE;
static World  cache;
static int    locked  = 0;
static int    solomode = 0;
static char   wpath[MAX_PATH];

const char *world_path(void) { return wpath; }

int world_open(const char *path, int solo)
{
    DWORD got = 0;
    LARGE_INTEGER sz;

    solomode = solo;
    strncpy(wpath, path, sizeof wpath - 1);
    wpath[sizeof wpath - 1] = 0;

    hfile = CreateFileA(wpath, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "AMAZON: cannot open world file %s (error %lu)\n",
                wpath, (unsigned long)GetLastError());
        return 0;
    }

    if (!GetFileSizeEx(hfile, &sz)) sz.QuadPart = 0;
    if (sz.QuadPart < (LONGLONG)sizeof(World)) {
        World *w = world_lock();
        if (!w) return 0;
        world_reset(w);
        world_unlock();
    } else {
        World *w = world_lock();
        if (!w) return 0;
        if (w->magic != WORLD_MAGIC || w->vers != WORLD_VERS) {
            printf("[world file is from a different version -- starting a "
                   "fresh valley]\n");
            world_reset(w);
        }
        world_unlock();
    }
    (void)got;
    return 1;
}

World *world_lock(void)
{
    OVERLAPPED ov;
    DWORD got = 0;
    LARGE_INTEGER zero;

    if (hfile == INVALID_HANDLE_VALUE) return NULL;
    if (locked) return &cache;

    memset(&ov, 0, sizeof ov);
    if (!LockFileEx(hfile, LOCKFILE_EXCLUSIVE_LOCK, 0,
                    (DWORD)sizeof(World), 0, &ov)) {
        fprintf(stderr, "AMAZON: cannot lock world file (error %lu)\n",
                (unsigned long)GetLastError());
        return NULL;
    }
    locked = 1;

    zero.QuadPart = 0;
    SetFilePointerEx(hfile, zero, NULL, FILE_BEGIN);
    memset(&cache, 0, sizeof cache);
    if (!ReadFile(hfile, &cache, (DWORD)sizeof cache, &got, NULL))
        got = 0;
    if (got < sizeof cache || cache.magic != WORLD_MAGIC)
        world_reset(&cache);
    return &cache;
}

void world_unlock(void)
{
    OVERLAPPED ov;
    DWORD put = 0;
    LARGE_INTEGER zero;

    if (!locked) return;

    zero.QuadPart = 0;
    SetFilePointerEx(hfile, zero, NULL, FILE_BEGIN);
    WriteFile(hfile, &cache, (DWORD)sizeof cache, &put, NULL);
    FlushFileBuffers(hfile);

    memset(&ov, 0, sizeof ov);
    UnlockFileEx(hfile, 0, (DWORD)sizeof(World), 0, &ov);
    locked = 0;
}

void world_close(void)
{
    if (locked) world_unlock();
    if (hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(hfile);
        hfile = INVALID_HANDLE_VALUE;
    }
    if (solomode) DeleteFileA(wpath);
}
