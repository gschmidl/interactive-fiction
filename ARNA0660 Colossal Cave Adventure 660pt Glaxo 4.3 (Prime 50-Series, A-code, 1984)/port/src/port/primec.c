/* primec.c - what the A-code executive's PRIMOS calls need that Fortran
 * cannot do: the program's own directory, the clock and the login name
 * (TIMDAT), and deleting a saved game (TSRC$$).
 *
 * The port works in the directory of adventure4.exe: the four ADVINIT
 * files are there, and the saved games go to its saves\ (PINIT).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <direct.h>
#include <windows.h>

/* PINIT: into the program's directory, and a saves\ there */
void pinit_(void)
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, sizeof path);
    if (n > 0 && n < sizeof path) {
        char *p = strrchr(path, '\\');
        if (p) {
            *p = 0;
            if (_chdir(path) != 0) {
                fprintf(stderr, "adventure4: cannot go to %s\n", path);
                exit(1);
            }
        }
    }
    _mkdir("saves");
}

/* PLOAD(NAME, B, N, MAX): the file NAME (a Fortran string, blank padded)
   into B, its length in bytes into N */
void pload_(const char *name, uint8_t *b, int32_t *n, const int32_t *max,
            size_t namelen)
{
    char fn[64];
    size_t k = namelen < sizeof fn - 1 ? namelen : sizeof fn - 1;
    FILE *f;
    long got;
    memcpy(fn, name, k);
    while (k > 0 && fn[k - 1] == ' ') k--;
    fn[k] = 0;
    f = fopen(fn, "rb");
    if (!f) {
        fprintf(stderr, "adventure4: %s is missing (it belongs beside "
                "adventure4.exe)\n", fn);
        exit(1);
    }
    got = (long)fread(b, 1, (size_t)*max, f);
    if (!feof(f) && fgetc(f) != EOF) {
        fprintf(stderr, "adventure4: %s is too big\n", fn);
        exit(1);
    }
    fclose(f);
    *n = (int32_t)got;
}

/* TIMDAT(BUF, N): PRIMOS's fifteen halfwords -
 *   1-3   the date, "MMDDYY"
 *   4     minutes since midnight
 *   5     seconds
 *   6     ticks (330 a second)
 *   7-8   CPU time, 9-10 disk time (seconds, ticks)
 *   11    ticks a second
 *   12    user number
 *   13-15 the login name, six characters
 * The executive keeps words 1-4 with a saved game, to tell how long ago
 * it was saved (EXEC 9), and puts the login name in the save file's name.
 * The date is kept as characters, so its halfwords are the Prime's
 * byte-swapped - they are only ever compared with each other. */
void timdat_(int16_t *buf, const int16_t *n)
{
    int16_t w[15];
    char *c = (char *)w;
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char name[64] = "";
    DWORD len = sizeof name;
    int i, k;
    memset(w, 0, sizeof w);
    c[0] = (char)('0' + (lt->tm_mon + 1) / 10);
    c[1] = (char)('0' + (lt->tm_mon + 1) % 10);
    c[2] = (char)('0' + lt->tm_mday / 10);
    c[3] = (char)('0' + lt->tm_mday % 10);
    c[4] = (char)('0' + lt->tm_year % 100 / 10);
    c[5] = (char)('0' + lt->tm_year % 10);
    w[3] = (int16_t)(lt->tm_hour * 60 + lt->tm_min);
    w[4] = (int16_t)lt->tm_sec;
    w[10] = 330;
    w[11] = 1;
    if (!GetUserNameA(name, &len)) name[0] = 0;
    for (i = 0, k = 0; k < 6; i++) {
        int ch = (unsigned char)name[i];
        if (!ch) break;
        c[24 + k++] = isalnum(ch) && ch < 128 ? (char)toupper(ch) : '_';
    }
    if (k == 0) {
        memcpy(c + 24, "PLAYER", 6);
        k = 6;
    }
    for (; k < 6; k++) c[24 + k] = ' ';
    k = *n < 15 ? *n : 15;
    memcpy(buf, w, (size_t)k * 2);
}

/* TSRC$$(K$DELE, NAME, 0, CHRPOS, TYPE, CODE): delete the saved game
   NAME(1:CHRPOS) */
void ptsrc_(const int16_t *key, const char *name, const int16_t *unused,
            const int32_t *chrpos, int16_t *type, int16_t *code,
            size_t namelen)
{
    char fn[80];
    size_t k = (size_t)*chrpos;
    (void)key; (void)unused; (void)type;
    if (k > namelen) k = namelen;
    if (k > sizeof fn - 8) k = sizeof fn - 8;
    while (k > 0 && name[k - 1] == ' ') k--;
    strcpy(fn, "saves/");
    memcpy(fn + 6, name, k);
    fn[6 + k] = 0;
    *code = remove(fn) == 0 ? 0 : 1;
}
