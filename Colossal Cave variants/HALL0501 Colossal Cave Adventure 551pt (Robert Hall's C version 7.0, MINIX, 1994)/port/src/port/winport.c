/* ======================================================================
 *  winport.c - see portcompat.h
 * ====================================================================== */
#define PORT_IMPLEMENTATION
#include "portcompat.h"
#ifdef _WIN32
#include <windows.h>
#endif

/* The game opens advent1.dat .. advent4.dat by bare name.  If a file is not
   in the current directory, look beside the executable. */
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

/* Options.  A saved game's name stays in argv for the game, as before; an
   unknown --option is refused, exit 2. */
int port_fixes = 1;

void port_options(int *argc, char ***argv)
{
	char **v = *argv;
	int i, n = 1;

	for (i = 1; i < *argc; i++) {
		if (strcmp(v[i], "--no-fixes") == 0)
			port_fixes = 0;
		else if (strcmp(v[i], "-h") == 0 || strcmp(v[i], "--help") == 0) {
			printf("usage: advent [--no-fixes] [-h] [SAVEDGAME]\n\n");
			printf("  SAVEDGAME    start from a game saved with SAVE (advent.sav)\n");
			printf("  --no-fixes   the 1994 program as it was: no dwarf ever blocks the way\n");
			printf("  -h, --help   this help\n\n");
			printf("Robert R. Hall's Generic Adventure 7.0 (MINIX, 1994).  SAVE and RESTORE\n");
			printf("use advent.sav in the current folder; QUIT ends the game.\n");
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

#ifdef TESTRAND
/* Test builds only: one generator for both platforms, seeded from the
   environment variable ADV_SEED (default 1) whatever the game asks for. */
static unsigned long rnd = 1;

void port_srand(unsigned seed)
{
	const char *s = getenv("ADV_SEED");
	(void)seed;
	rnd = s ? strtoul(s, NULL, 10) : 1;
}

int port_rand(void)
{
	rnd = (rnd * 1103515245UL + 12345UL) & 0xFFFFFFFFUL;
	return (int)((rnd >> 16) & 0x7FFF);
}
#endif
