/*
 * File: csub.c
 * ------------
 * This file contains a few C-based functions for the newadv
 * application.
 *
 * PORT: changes for the Windows build are marked PORT:.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32                       /* PORT: no <sys/file.h>; binary files */
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#define OPEN_BINARY O_BINARY
#else
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#define OPEN_BINARY 0
#endif
#include "params.h"

#define FILENAME "newadv.sav"

#define common(symbol,name,size) char symbol[(size+3) & -4]
#include "common.h"
#undef common

/*
 * PORT: the options, read before the FORTRAN main program starts:
 * --no-fixes turns the fixes off, -h and --help list the options, and any
 * other --option is refused (exit 2).
 */
int port_fixes = 1;

static void port_option(const char *a) {
    if (strcmp(a, "--no-fixes") == 0)
        port_fixes = 0;
    else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
        printf("usage: newadv [--no-fixes] [-h]\n\n");
        printf("  --no-fixes  the 2010 program as it was: SAVE forgets what is in the\n");
        printf("              containers (README.md, Fix 1)\n");
        printf("  -h, --help  this help\n\n");
        printf("Eric Roberts' Adventure V6.2 (Wellesley, 655 points).  SAVE writes\n");
        printf("newadv.sav in the current folder; QUIT ends the game.\n");
        exit(0);
    } else if (strncmp(a, "--", 2) == 0) {
        fprintf(stderr, "newadv: unknown option '%s' (try --help)\n", a);
        exit(2);
    }
}

#ifdef _WIN32
static void port_options(void) __attribute__((constructor));
static void port_options(void) {
    int i;
    for (i = 1; i < __argc; i++) port_option(__argv[i]);
}
#else
static void port_options(int argc, char **argv) __attribute__((constructor));
static void port_options(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) port_option(argv[i]);
}
#endif

/*
 * PORT: the program never calls srand(), so every game drew the same
 * numbers from the C library's rand().  Roberts built it on Mac OS X,
 * whose rand() is the Park-Miller "minimal standard" generator (FreeBSD
 * rand.c, RAND_MAX 0x7fffffff), seeded with 1.  It is reproduced here so
 * that the Windows build draws those numbers instead of the Windows C
 * library's 15-bit ones.
 */
static unsigned long macnext = 1;

static long macrand(void) {
    long hi, lo, x;

    if (macnext == 0) macnext = 123459876;
    hi = (long) (macnext / 127773);
    lo = (long) (macnext % 127773);
    x = 16807 * lo - 2836 * hi;
    if (x < 0) x += 0x7fffffff;
    macnext = (unsigned long) x;
    return (long) (macnext % 0x80000000UL);
}

float crand_() {
    return macrand() / ((double) 0x7fffffff + 1);
}

void csave_(char cbuf[]) {
    int ochan;

    ochan = open(FILENAME, O_WRONLY|O_CREAT|O_TRUNC|OPEN_BINARY, 0777);
    if (ochan == -1) exit(1);

#define common(symbol,name,size) write(ochan, symbol, size)
#include "volatile.h"
#undef common
    /* PORT: Fix 1 - volatile.fg names "hdlcom" for /HLDCOM/ (HOLDER and
       HLINK, what is in which container), so SAVE never wrote it and a
       restored game had the containers as they were at the start */
#define common(symbol,name,size) if (port_fixes) write(ochan, symbol, size)
#include "holder.h"
#undef common

    close(ochan);
    printf("%s saved\n", FILENAME);
}

void crest_(char cbuf[]) {
    int ochan;

    ochan = open(FILENAME, O_RDONLY|OPEN_BINARY, 0);
    if (ochan == -1) {
        printf("Can't open the file %s\n", FILENAME);
        exit(1);
    }

#define common(symbol,name,size) read(ochan, symbol, size)
#include "volatile.h"
#undef common
    /* PORT: Fix 1 - the containers, if the file has them (a game saved
       with --no-fixes has not, and they stay as they are) */
#define common(symbol,name,size) if (port_fixes) read(ochan, symbol, size)
#include "holder.h"
#undef common

    close(ochan);
}

void readln_(char cbuf[]) {
    int i, ch, semi;

    semi = -1;
    for (i = 0; ; i++) {
        ch = getchar();
        if (ch == EOF) {
            printf("\n");
            exit(0);
        }
        if (ch == '\r' || ch == '\n') break;
        if (ch == ';') semi = i;
        if (i < BUFMAX - 1) cbuf[i] = ch;
    }
    if (i > BUFMAX - 1) i = BUFMAX - 1;
    cbuf[i] = '\0';
    if (!isatty(0)) printf("%s\n", cbuf);
    if (semi >= 0) i = semi;
    while (i < BUFMAX) cbuf[i++] = ' ';
}

/*
 * PORT: CALL CGAMDIR(DIR, LEN) puts the directory the program lives in
 * (with a trailing separator) into the CHARACTER variable DIR and its
 * length into LEN.  The billboard and the notebook were read from
 * /usr/games/billboard.txt and /usr/games/notebook.txt.
 */
void cgamdir_(char *dir, int *len, size_t dirlen) {
    char path[1024];
    size_t n = 0;

#ifdef _WIN32
    DWORD got = GetModuleFileNameA(NULL, path, sizeof(path));
    if (got > 0 && got < sizeof(path)) {
        char *slash = strrchr(path, '\\');
        if (slash) n = (size_t) (slash - path) + 1;
    }
#else
    ssize_t got = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (got > 0) {
        char *slash;
        path[got] = '\0';
        slash = strrchr(path, '/');
        if (slash) n = (size_t) (slash - path) + 1;
    }
#endif
    if (n > dirlen) n = 0;
    memset(dir, ' ', dirlen);
    memcpy(dir, path, n);
    *len = (int) n;
}
