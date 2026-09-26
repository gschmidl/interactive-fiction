/* ======================================================================
 *  winport.c - the few C library routines the 1984/1990 sources expect
 *  and a modern C library lacks or does differently.  See portcompat.h.
 * ====================================================================== */
#define PORT_IMPLEMENTATION
#include "portcompat.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

void port_exit(int code)
{
	fflush(stdout);
	exit(code);
}

/* ----------------------------------------------------------------------
 *  fopen: the games open advent1.txt .. advent4.txt from the current
 *  directory.  If a file is not there, look beside the executable.
 * ---------------------------------------------------------------------- */
FILE *port_fopen(const char *name, const char *mode)
{
	FILE *f = fopen(name, mode);
#ifdef _WIN32
	if (!f && mode[0] == 'r' && !strpbrk(name, "\\/:")) {
		char path[MAX_PATH + 64];
		DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
		if (n > 0 && n < MAX_PATH) {
			char *slash = strrchr(path, '\\');
			if (slash && strlen(name) < 64) {
				strcpy(slash + 1, name);
				f = fopen(path, mode);
			}
		}
	}
#endif
	return f;
}

int port_ltoa(long value, char *s, ...)
{
	sprintf(s, "%ld", value);
	return (int)strlen(s);
}

int port_putw(int w, FILE *f)
{
	if (fputc(w & 0xFF, f) == EOF || fputc((w >> 8) & 0xFF, f) == EOF)
		return EOF;
	return w;
}

int port_getw(FILE *f)
{
	int lo = fgetc(f), hi;
	if (lo == EOF)
		return EOF;
	hi = fgetc(f);
	if (hi == EOF)
		return EOF;
	return (short)(lo | (hi << 8));
}

int setmem(void *p, unsigned n, int c)
{
	memset(p, c, n);
	return 0;
}

/* ----------------------------------------------------------------------
 *  Options.  The authors' own flags (-r, -d) are left in argv for their
 *  loop; the port's long ones are taken out.  An unknown --option is
 *  refused, exit 2.
 * ---------------------------------------------------------------------- */
int port_fixes = 1;

static void port_usage(void)
{
#ifdef PORT_POHL
	printf("usage: advent [-r] [-d -d -d] [--restore] [--no-fixes] [-h]\n\n");
	printf("  -r, --restore  start from a saved game (the game asks its name)\n");
	printf("  -d -d -d       the author's debug output (three times, or nothing)\n");
	printf("  --no-fixes     the 1984 program as it was: no dwarf ever blocks the way\n");
	printf("  -h, --help     this help\n\n");
	printf("Jaeger/Pohl C Adventure, 12 June 1984.  SUSPEND saves the game as NAME.adv\n");
	printf("and stops; QUIT ends it with the score.\n");
#else
	printf("usage: advent [-r] [-d -d] [--restore] [--no-fixes] [-h]\n\n");
	printf("  -r, --restore  start from a saved game (the game asks its name)\n");
	printf("  -d -d          Daimler's debug output (twice, or nothing)\n");
	printf("  --no-fixes     the 1990 program as it ran under DOS: no dwarf blocks the\n");
	printf("                 way, the pirate goes anywhere, messages above 127 come out\n");
	printf("                 garbled, LOG LAMP stops the game (README.md, Fixes 1-5)\n");
	printf("  -h, --help     this help\n\n");
	printf("Daimler's Turbo C 2.0 Adventure, 1990.  SUSPEND saves the game as NAME.adv\n");
	printf("and stops; QUIT ends it with the score.\n");
#endif
}

void port_options(int *argc, char ***argv)
{
	char **v = *argv;
	int i, n = 1;

	for (i = 1; i < *argc; i++) {
		if (strcmp(v[i], "--no-fixes") == 0)
			port_fixes = 0;
		else if (strcmp(v[i], "--restore") == 0)
			v[n++] = "-r";
		else if (strcmp(v[i], "-h") == 0 || strcmp(v[i], "--help") == 0) {
			port_usage();
			exit(0);
		} else if (strncmp(v[i], "--", 2) == 0) {
			fprintf(stderr, "advent: unknown option '%s' (try --help)\n", v[i]);
			exit(2);
		} else
			v[n++] = v[i];
	}
	v[n] = NULL;
	*argc = n;
}
