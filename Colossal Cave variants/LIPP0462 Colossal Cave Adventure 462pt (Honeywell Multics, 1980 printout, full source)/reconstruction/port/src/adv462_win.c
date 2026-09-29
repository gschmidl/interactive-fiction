/*
 * adv462_win.c -- the "site-supplied" routines of Gary Palter's Adventure,
 * for the Windows port of adv462: Adventure 1.2 (462 points), the Colossal
 * Cave of Honeywell's Phoenix Multics, 1980.
 *
 * Adapted from Jim Lippard's unix/adv462_util.c (written 2026-09-27 by Claude
 * Opus 5.5 at his direction; BSD licence, see src_original/adv462/LICENSE),
 * which it follows routine for routine.  The Multics originals of these
 * routines are in src_original/adv462/Multics/adv462_io_.pl1.
 *
 * The game keeps its whole state in eleven common blocks.  At startup it
 * calls addr and size to record where each block is and how long it is,
 * then:
 *   ldcomn(.true., ...)        loads the "system" image adventure.newgame
 *                              (the database already read in), if it exists;
 *   ldcomn(.false., name, ...) loads a game saved by SUSPEND (RESTORE);
 *   svcomn(.false., name, ...) saves the current game (SUSPEND);
 *   svcomn(.true., ...)        saves a new system image (magic mode).
 *
 * What is different on Windows:
 *   - The game directory is the folder that holds adv462.exe.  The program
 *     changes to it before the Fortran main program starts, so
 *     adventure.data, adventure.newgame and saves\ are found there, whatever
 *     folder the game is started from.
 *   - Suspended games are saves\<name>.adv462 in the game directory.  Multics
 *     kept them in the player's home directory, the Unix port in ~/.adv462.
 *   - In the name, a character Windows does not allow becomes "_" (as "."
 *     and "/" do on Unix), and a DOS device name (CON, NUL, COM1, ...) gets a
 *     "_" after it.
 *   - The messages are printed by the Fortran subroutine advmsg, so that they
 *     come out in order with the game's own text, with the Multics wording.
 *   - ADV462_CLOCK sets the clock for test runs (see advclk below).
 * As on Unix, ADV462_DATA names the database, ADV462_DIR the folder of
 * adventure.newgame (read, and written by magic mode) and ADV462_SAVEDIR the
 * folder of suspended games.  Relative paths in them are taken from the game
 * directory.
 *
 * The program is compiled with -fdefault-integer-8, so every Fortran INTEGER
 * and LOGICAL is 8 bytes.  A saved image is simply the bytes of the eleven
 * blocks, one after another; a file whose length doesn't match the current
 * program's blocks is refused.
 *
 * The file also builds on Linux (without the change of folder), for checking
 * the Windows program against a Linux build of the same source.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

#define NBLOCKS 11
#define NAMECHARS 10

typedef int64_t fint;           /* Fortran INTEGER with -fdefault-integer-8 */

#ifdef _WIN32
#define SEP "\\"
#define make_dir(d) _mkdir(d)

/*
 * Change to the folder holding adv462.exe before the Fortran main program
 * runs.  The wide-character calls work whatever characters the path holds.
 */
static void __attribute__((constructor))
to_game_dir(void)
{
	static wchar_t path[32768];
	DWORD n;
	wchar_t *slash;

	n = GetModuleFileNameW(NULL, path, sizeof(path) / sizeof(path[0]));
	if (n == 0 || n >= sizeof(path) / sizeof(path[0]))
		return;
	slash = wcsrchr(path, L'\\');
	if (slash == NULL)
		return;
	/* keep the backslash of a drive's root: "C:\" */
	slash[slash == path + 2 && path[1] == L':' ? 1 : 0] = L'\0';
	(void)_wchdir(path);
}
#else
#define SEP "/"
#define make_dir(d) mkdir(d, 0755)
#endif

/* Fortran: subroutine advmsg(text), added by mkwin.py. */
extern void advmsg_(const char *text, size_t len);

static void
say(const char *fmt, ...)
{
	char buf[1200];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	advmsg_(buf, strlen(buf));
}

/*
 * call advdat(path): the default database path, blank-padded into a Fortran
 * CHARACTER variable (gfortran passes its length as a hidden trailing
 * argument).  The program is in the game directory, so a plain name does.
 */
void
advdat_(char *buf, size_t len)
{
	const char *path = "adventure.data";
	size_t n = strlen(path);

	if (n > len)
		n = len;
	memcpy(buf, path, n);
	memset(buf + n, ' ', len - n);
}

/* call addr(x, cmadrs(1,n)): store the address of x in the first word. */
void
addr_(void *x, fint *where)
{
	where[0] = (fint)(intptr_t)x;
}

/* size(first, last): number of words from first through last inclusive. */
fint
size_(void *first, void *last)
{
	return (fint)(((char *)last - (char *)first) / (long)sizeof(fint)) + 1;
}

/*
 * advclk(d, t): if ADV462_CLOCK is "<day> <minute>", set d and t from it and
 * return 1; else return 0, and datime reads the real clock.  The day is
 * counted from 1 January 1977 (a Saturday), as datime counts it, and the
 * minute from midnight.  The clock moves on one minute each time the game
 * reads it, so the check at the 45th turn for a clock that stands still
 * ("he's cheating") is not set off.  The clock seeds ran, so a run with
 * ADV462_CLOCK set repeats exactly.
 */
fint
advclk_(fint *d, fint *t)
{
	static int state = -1;          /* -1 unread, 0 real clock, 1 set */
	static long long day, minute;
	const char *s;

	if (state < 0) {
		s = getenv("ADV462_CLOCK");
		state = s != NULL &&
		    sscanf(s, "%lld%*[ ,:]%lld", &day, &minute) == 2 &&
		    day >= 0 && minute >= 0 && minute < 1440;
	}
	if (!state)
		return 0;
	*d = (fint)day;
	*t = (fint)minute;
	if (++minute == 1440) {
		minute = 0;
		day++;
	}
	return 1;
}

/* Is name (lower case) a DOS device name, which Windows won't use for a file? */
static int
is_device(const char *name)
{
	static const char *const dev[] = { "con", "prn", "aux", "nul" };
	size_t i;

	for (i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
		if (strcmp(name, dev[i]) == 0)
			return 1;
	return (strncmp(name, "com", 3) == 0 || strncmp(name, "lpt", 3) == 0) &&
	    isdigit((unsigned char)name[3]) && name[4] == '\0';
}

/*
 * Build the file's name (for the messages) and path.  fname holds ten
 * characters in Fortran A1 format: one character in the first byte of each
 * 8-byte word.  Returns 0, or -1 if the path does not fit.
 */
static int
image_path(fint system, const fint *fname, char *ename, size_t elen,
    char *path, size_t len)
{
	const char *dir;
	char name[NAMECHARS + 2];
	int i, n = 0;

	if (system) {
		snprintf(ename, elen, "adventure.newgame");
		dir = getenv("ADV462_DIR");
		if (dir == NULL || *dir == '\0')
			return snprintf(path, len, "%s", ename) < (int)len ? 0 : -1;
		return snprintf(path, len, "%s" SEP "%s", dir, ename) <
		    (int)len ? 0 : -1;
	}

	for (i = 0; i < NAMECHARS; i++) {
		unsigned char c = *(const unsigned char *)&fname[i];

		if (c == ' ' || c == '\0')
			break;
		/* keep saves in the save folder, and out of Windows' way */
		if (c < ' ' || c > '~' || strchr("\\/:*?\"<>|.", c) != NULL)
			c = '_';
		name[n++] = (char)tolower(c);
	}
	name[n] = '\0';
	if (n == 0)
		snprintf(name, sizeof(name), "game");
	else if (is_device(name))
		strcat(name, "_");
	snprintf(ename, elen, "%s.adv462", name);

	dir = getenv("ADV462_SAVEDIR");
	if (dir == NULL || *dir == '\0') {
		dir = "saves";
		(void)make_dir(dir);    /* may exist already; fopen reports failure */
	}
	return snprintf(path, len, "%s" SEP "%s", dir, ename) < (int)len ? 0 : -1;
}

static long
total_bytes(const fint *cmszes)
{
	long total = 0;
	int i;

	for (i = 0; i < NBLOCKS; i++)
		total += (long)cmszes[i] * (long)sizeof(fint);
	return total;
}

/*
 * ldcomn(l, fname, cmadrs, cmszes).  On any failure the common blocks are
 * left untouched, so the player simply continues in a fresh game (as the
 * RESTORE comment in the main program says).
 */
void
ldcomn_(fint *l, fint *fname, fint *cmadrs, fint *cmszes)
{
	char ename[32], path[1100];
	FILE *fp;
	char *buf, *p;
	long want, got;
	int i;

	if (image_path(*l, fname, ename, sizeof(ename), path, sizeof(path)) != 0)
		return;
	if ((fp = fopen(path, "rb")) == NULL) {
		if (!*l)
			say("I can't find a suspended game called %s.", ename);
		return;
	}
	want = total_bytes(cmszes);
	if ((buf = malloc(want + 1)) == NULL) {
		fclose(fp);
		return;
	}
	got = (long)fread(buf, 1, want + 1, fp);
	fclose(fp);
	if (got != want) {
		say("%s is not a saved game for this version of adventure.", ename);
		free(buf);
		return;
	}
	for (i = 0, p = buf; i < NBLOCKS; i++) {
		size_t n = (size_t)cmszes[i] * sizeof(fint);

		memcpy((void *)(intptr_t)cmadrs[4 * i], p, n);
		p += n;
	}
	free(buf);
}

/* svcomn(l, fname, cmadrs, cmszes). */
void
svcomn_(fint *l, fint *fname, fint *cmadrs, fint *cmszes)
{
	char ename[32], path[1100];
	FILE *fp;
	int i, ok = 1;

	if (image_path(*l, fname, ename, sizeof(ename), path, sizeof(path)) != 0 ||
	    (fp = fopen(path, "wb")) == NULL) {
		say("I am sorry, but I can't create or find your file.");
		return;
	}
	for (i = 0; i < NBLOCKS; i++) {
		size_t n = (size_t)cmszes[i] * sizeof(fint);

		if (fwrite((void *)(intptr_t)cmadrs[4 * i], 1, n, fp) != n)
			ok = 0;
	}
	if (fclose(fp) != 0)
		ok = 0;
	if (!ok)
		say("I am sorry, but I couldn't save your game.");
}
