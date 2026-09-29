/* main.c -- ADVENTURE 751 for Windows.
 *
 * Loads the game image, hands the machine to the CPU, and gets out of
 * the way.  Everything the player sees is produced by the original
 * program running on the emulated processor.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdp10.h"

extern int opt_delays;
extern int opt_echo;
extern int opt_faketime;
extern const char *opt_player;
extern void jsys_init(void);

static void usage(void)
{
    fputs(
"adv751 -- ADVENTURE < 6.1/ 3>, the 751 point Colossal Cave.\n"
"\n"
"usage: adv751 [options]\n"
"\n"
"  -p, --player NAME   play as NAME (default: your Windows user name).\n"
"                      The name goes in the scoreboard and is what the\n"
"                      game greets you by.\n"
"      --no-delays     do not pause where the game pauses.\n"
"  -e, --echo          echo typed lines, as the terminal did.  On by\n"
"      --no-echo       default when input is a file, off at a console,\n"
"                      which echoes for itself.\n"
"      --time HH:MM    pretend it is this time of day.  It goes in the\n"
"                      gripe log and seeds the game's randomness.\n"
"  -v, --verbose       trace operating system calls to stderr.\n"
"  -h, --help          this.\n"
"\n", stdout);
}

int main(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) opt_verbose++;
        else if (!strcmp(a, "-e") || !strcmp(a, "--echo")) opt_echo = 1;
        else if (!strcmp(a, "--no-echo")) opt_echo = 0;
        else if (!strcmp(a, "--no-delays")) opt_delays = 0;
        else if ((!strcmp(a, "-p") || !strcmp(a, "--player")) && i + 1 < argc)
            opt_player = argv[++i];
        else if (!strcmp(a, "--time") && i + 1 < argc) {
            int h = 0, m = 0;
            if (sscanf(argv[++i], "%d:%d", &h, &m) >= 1)
                opt_faketime = h * 60 + m;
        }
        else if (!strcmp(a, "-t") || !strcmp(a, "--trace")) trace = 1;
        else {
            fprintf(stderr, "adv751: unknown option %s (try --help)\n", a);
            return 2;
        }
    }

    setvbuf(stdout, NULL, _IOFBF, 8192);
    monitor_init();
    jsys_init();
    cpu_reset();
    cpu_run();
    monitor_shutdown();
    return exit_code;
}
