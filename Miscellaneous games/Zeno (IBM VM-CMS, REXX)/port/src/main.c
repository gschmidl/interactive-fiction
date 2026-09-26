/* ------------------------------------------------------------------
 * ZENO (1983) - Dave Mitchell, ZENO at WINVMB.
 *
 * "An investigation of the limits to REX."  This program is the VM/CMS
 * environment the game needs: it hands the author's own ZENO EXEC to a
 * REXX interpreter untouched and answers the commands it issues.
 * ------------------------------------------------------------------ */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
extern void screen_set_script(const char *);

#define INCL_RXSUBCOM
#define INCL_RXSHV
#include "rexxsaa.h"
#include "zeno.h"

extern APIRET APIENTRY zeno_subcom(PRXSTRING, PUSHORT, PRXSTRING);
extern void cms_set_dirs(const char *g, const char *s);
extern int  screen_colour_3279;
extern int  screen_verify;
extern int  screen_intro_pause;

#define ENVNAME "ZENOCMS"

static char *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    char *b;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    b = malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    n = (long)fread(b, 1, (size_t)n, f);
    b[n] = 0;
    fclose(f);
    *len = n;
    return b;
}

/* Where are we?  Everything hangs off the directory holding the exe. */
static void exe_dir(char *out, int outsz)
{
    char path[MAX_PATH];
    char *p;
    GetModuleFileNameA(NULL, path, MAX_PATH);
    p = strrchr(path, '\\');
    if (p) *p = 0;
    snprintf(out, outsz, "%s", path);
}

static void usage(void)
{
    printf("ZENO (1983) - Dave Mitchell.  Native port.\n\n");
    printf("  zeno [options]\n\n");
    printf("  -3279     use real 3279 terminal colours (blue/white/green/red)\n");
    printf("  -fast     do not hold the start-up panel\n");
    printf("  -debug    write zeno.log\n");
    printf("  -dir DIR  take game files from DIR\n\n");
    printf("Keys: F1-F12 are the 3270 PF keys, TAB moves between fields,\n");
    printf("      ENTER transmits, ESC is PA1.\n");
}

int main(int argc, char **argv)
{
    char base[MAX_PATH], game[MAX_PATH + 32], saves[MAX_PATH + 32], exec[MAX_PATH + 64];
    const char *execname = "ZENOFIX.EXEC";
    char *src;
    long srclen;
    RXSTRING instore[2], result;
    char rbuf[256];
    SHORT rc2 = 0;
    APIRET rc;
    int i;

    exe_dir(base, sizeof base);
    snprintf(game,  sizeof game,  "%s\\game",  base);
    snprintf(saves, sizeof saves, "%s\\saves", base);

    for (i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-3279")) screen_colour_3279 = 1;
        else if (!strcmp(argv[i], "-fast")) screen_intro_pause = 0;
        else if (!strcmp(argv[i], "-debug")) zeno_debug = 1;
        else if (!strcmp(argv[i], "-original")) execname = "ZENO.EXEC";
        else if (!strcmp(argv[i], "-verify")) screen_verify = 1;
        else if (!strcmp(argv[i], "-script") && i + 1 < argc) screen_set_script(argv[++i]);
        else if (!strcmp(argv[i], "-dir") && i + 1 < argc)
            snprintf(game, sizeof game, "%s", argv[++i]);
        else { usage(); return 0; }
    }

    _mkdir(saves);
    cms_set_dirs(game, saves);
    panel_set_libdir(game);

    snprintf(exec, sizeof exec, "%s\\%s", game, execname);
    if (!(src = slurp(exec, &srclen))) {
        fprintf(stderr, "zeno: cannot read %s\n", exec);
        return 1;
    }

    rc = RexxRegisterSubcomExe(ENVNAME, (RexxSubcomHandler *)zeno_subcom, NULL);
    if (rc) { fprintf(stderr, "zeno: cannot register subcom environment (%lu)\n",
                      (unsigned long)rc); return 1; }

    if (!con_open()) { fprintf(stderr, "zeno: cannot open a console screen\n"); return 1; }

    instore[0].strptr = src; instore[0].strlength = (ULONG)srclen;
    instore[1].strptr = NULL; instore[1].strlength = 0;
    MAKERXSTRING(result, rbuf, sizeof rbuf);

    rc = RexxStart(0, NULL, "ZENO", instore, ENVNAME, RXCOMMAND, NULL, &rc2, &result);

    con_close();
    RexxDeregisterSubcom(ENVNAME, NULL);
    free(src);

    if (rc) fprintf(stderr, "\nzeno: REXX terminated, rc=%ld\n", (long)rc);
    return 0;
}
