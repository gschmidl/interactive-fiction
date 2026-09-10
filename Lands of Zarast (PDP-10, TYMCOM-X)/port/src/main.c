/* main.c -- driver for the LANDS OF ZARAST ports.
 *
 * Loads one of the original TYMCOM-X .SHR images and runs it on the
 * emulated DECsystem-10.  On Tymshare these were separate programs in one
 * directory, and you moved between them by typing RUN; here they are one
 * executable that takes the program name as an argument, and the directory
 * they share is whatever -d points at.
 *
 * Which set of images is built in comes from images_84.c or images_87.c --
 * the 1984 game or the later one.  Everything below is the same either way.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir_one(p) _mkdir(p)
#else
#include <sys/stat.h>
#define mkdir_one(p) mkdir((p), 0777)
#endif
#include "pdp10.h"

extern int  opt_verbose;
extern int  opt_faketime;
extern int  opt_delays;
extern int  opt_dedupe_echo;
extern int  opt_echo_input;
extern int  opt_lcfold;
extern const char *opt_dir;
extern char run_next[16];
void monitor_init(void);
void monitor_shutdown(void);
void monitor_cleanup_scratch(void);

static const char *progname = "zarast";

static void usage(void)
{
    int i;
    fprintf(stderr, "%s -- %s\n\n", progname, port_banner);
    fprintf(stderr, "usage: %s [options] [program]\n\nprograms:\n", progname);
    for (i = 0; progs[i].cmd; i++)
        fprintf(stderr, "  %-10s %s%s\n", progs[i].cmd, progs[i].what,
                i == 0 ? "  (default)" : "");
    fputs("\noptions:\n"
          "  -d DIR    keep characters and the world in DIR (default: .)\n"
          "  -q        do not pause where the game pauses\n"
          "  -e        echo what you type, as the Tymshare monitor did\n"
          "  --lower   pass lowercase through instead of folding it up\n"
          "  -t MIN    freeze the clock at MIN minutes past midnight\n"
          "  -v        report monitor calls the port does not implement\n"
          "  -T        trace every instruction (very loud)\n"
          "  -h        this message\n"
          "\nThe first run builds the world and rolls up a character for you.\n",
          stderr);
}

/* Any character files in the save directory?  The game cannot be played
 * without one and offers no way to make one: answering YES to "DO YOU
 * WANT TO STOP AND ROLL UP A CHARACTER" ends the program, because on
 * Tymshare that is where you typed RUN CHARC.  Do that part for them. */
static int have_character(const char *dir)
{
    DIR *d = opendir(dir);
    struct dirent *de;
    int found = 0;
    if (!d) return 0;
    while (!found && (de = readdir(d)) != NULL) {
        const char *dot = strrchr(de->d_name, '.');
        if (dot && (!strcmp(dot, ".adv") || !strcmp(dot, ".ADV"))
            && strncmp(de->d_name, "track", 5)
            && strncmp(de->d_name, "TRACK", 5))
            found = 1;
    }
    closedir(d);
    return found;
}

static int file_exists(const char *dir, const char *name)
{
    char path[512];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* The image a command names, or NULL. */
static const char *image_for(const char *cmd)
{
    int i;
    for (i = 0; progs[i].cmd; i++)
        if (!strcmp(cmd, progs[i].cmd) || !strcmp(cmd, progs[i].img))
            return progs[i].img;
    return NULL;
}

/* Is this one of the programs you play, as opposed to a character
 * generator or the world builder?  Only those need a character to exist
 * before they will get anywhere. */
static int is_the_game(const char *img)
{
    return strcmp(img, "filer") && strcmp(img, "charc")
        && strcmp(img, "charac") && strcmp(img, "cr");
}

static void run_image(const char *img)
{
    for (;;) {
        int i;
        if (!load_shr(img)) fatal("cannot load image %s", img);
        cpu_reset();
        PC = image_start;
        halted = 0;
        run_next[0] = 0;
        cpu_run();
        if (!run_next[0]) return;
        for (i = 0; progs[i].cmd; i++)
            if (!strcmp(run_next, progs[i].img)) break;
        if (!progs[i].cmd) return;    /* it asked for something we lack */
        img = progs[i].img;           /* the program chained to another */
    }
}

int main(int argc, char **argv)
{
    const char *img = progs[0].img;
    const char *newchar = image_for("newchar");
    int i, setup = 1;

    if (argc > 0 && argv[0]) {
        const char *p = strrchr(argv[0], '/');
        const char *q = strrchr(argv[0], '\\');
        if (q > p) p = q;
        progname = p ? p + 1 : argv[0];
    }

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else if (!strcmp(a, "-v")) opt_verbose++;
        else if (!strcmp(a, "-T")) trace = 1;
        else if (!strcmp(a, "-e")) opt_echo_input = 1;
        else if (!strcmp(a, "--lower")) opt_lcfold = 0;
        else if (!strcmp(a, "-q")) opt_delays = 0;
        else if (!strcmp(a, "--no-setup")) setup = 0;
        else if (!strcmp(a, "-d") && i + 1 < argc) opt_dir = argv[++i];
        else if (!strcmp(a, "-t") && i + 1 < argc) opt_faketime = atoi(argv[++i]);
        else if (a[0] == '-') { usage(); return 2; }
        else {
            const char *m = image_for(a);
            if (!m) { fprintf(stderr, "%s: no program named '%s'\n", progname, a);
                      usage(); return 2; }
            img = m;
        }
    }

    if (strcmp(opt_dir, ".")) mkdir_one(opt_dir);
    monitor_init();

    /* "BEFORE YOU CAN PLAY THIS GAME YOU MUST GENERATE A DATA FILE CALLED
     * 'NEWADV.DAT' YOU CAN RECIEVE ONE OF THESE BY RUNNING THE PROGRAM
     * CALLED 'FILER.SHR'", says DUNGEN.HLP.  Do it for them.  FILER is
     * deterministic and wants no input, so this is the same file every
     * time, and it is the file the game expects. */
    if (setup && strcmp(img, "filer") && !file_exists(opt_dir, "newadv.dat")) {
        fprintf(stderr, "[building the world file -- this happens once]\n");
        run_image("filer");
    }

    if (setup && newchar && is_the_game(img) && !have_character(opt_dir)) {
        fprintf(stderr, "[no characters here yet -- rolling one up first]\n");
        run_image(newchar);
        if (!have_character(opt_dir)) {
            monitor_shutdown();
            monitor_cleanup_scratch();
            fprintf(stderr,
                    "\n%s: no character was saved, so there is nothing"
                    " to play with.\n        Try again with:"
                    " %s newchar\n", progname, progname);
            return 1;
        }
        fputc(10, stderr);
    }

    run_image(img);
    monitor_shutdown();
    monitor_cleanup_scratch();

    if (is_the_game(img) && !have_character(opt_dir))
        fprintf(stderr,
                "\n%s: there are no characters in %s to play with.\n"
                "        Roll one up with: %s newchar\n",
                progname, opt_dir, progname);
    return 0;
}
