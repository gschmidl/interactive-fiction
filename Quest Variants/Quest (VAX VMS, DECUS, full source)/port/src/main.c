/*
 *  main.c -- the image dispatcher.
 *
 *  On the VAX, QUEST was five separate images that handed control to one
 *  another with LIB$RUN_PROGRAM, passing the 252 byte player record
 *  through the process common (LIB$PUT_COMMON / LIB$GET_COMMON):
 *
 *      QUEST.EXE   time-of-day check, terminal setup   -> QUEST1
 *      QUEST1.Q7R  character menu: create/run/kill/... -> QUEST2 or QUEST3
 *      QUEST2.Q7R  the city of Exeter: shop, cleric    -> QUEST3 or QUEST1
 *      QUEST3.Q7R  dungeon adventuring                 -> QUEST2 or QUEST1
 *      DNDOP.EXE   operator tool (author's account)    -> QUEST1 or QUEST3
 *
 *  Here they are five subroutines in one executable.  CHAIN longjmps back
 *  to this loop, which closes everything the abandoned image had open --
 *  a new VMS image would have started with no files of its own -- and
 *  enters the next one.  The process common survives, exactly as it did.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

#include "vmsrt.h"

int no_delay = 0;

static jmp_buf chain_env;
static int next_image = IMG_QUEST;
static int running = 0;

/*  the five images, now subroutines  */
void quest_(void);
void quest1_(void);
void quest2_(void);
void quest3_(void);
void dndop_(void);
void closeall_(void);            /* closes units 21..27, 99 (in lib.f) */

/* ------------------------------------------------------------------ */
/*  where the data files live                                          */
/* ------------------------------------------------------------------ */

static char datadir[1024];

static void find_datadir(const char *argv0)
{
    const char *e = getenv("QUEST_DATA");
    char exe[1024];
    size_t n;

    if (e && *e) {
        strncpy(datadir, e, sizeof datadir - 2);
        datadir[sizeof datadir - 2] = '\0';
        n = strlen(datadir);
        if (n && datadir[n - 1] != '/' && datadir[n - 1] != '\\')
            strcat(datadir, "/");
        return;
    }
#ifdef _WIN32
    if (GetModuleFileNameA(NULL, exe, sizeof exe - 1) == 0)
        exe[0] = '\0';
#else
    strncpy(exe, argv0 ? argv0 : "", sizeof exe - 1);
    exe[sizeof exe - 1] = '\0';
#endif
    (void) argv0;
    n = strlen(exe);
    while (n > 0 && exe[n - 1] != '/' && exe[n - 1] != '\\')
        n--;
    exe[n] = '\0';
    snprintf(datadir, sizeof datadir, "%sdata/", exe);
}

/*
 *  QPATH(NAME,PATH) -- turn a bare data file name into a path.  The
 *  sources name their files BSU$USER_2:[00CKKELLE.QUEST]<name>, the
 *  directory they lived in on the Ball State VAX; each OPEN in the port
 *  keeps the file name and asks for it here.
 */
void qpath_(const char *name, char *path, size_t nlen, size_t plen)
{
    char buf[1024];
    size_t i, n = nlen;

    while (n > 0 && name[n - 1] == ' ')
        n--;
    snprintf(buf, sizeof buf, "%s%.*s", datadir, (int) n, name);
    for (i = 0; i < plen; i++)
        path[i] = i < strlen(buf) ? buf[i] : ' ';
}

/* ------------------------------------------------------------------ */
/*  chaining                                                           */
/* ------------------------------------------------------------------ */

/*  LIB$RUN_PROGRAM, called by SUBROUTINE CHAIN.  */
void runprog_(const char *file, size_t len)
{
    char name[64];
    size_t i, j = 0;

    /*  keep the file name out of 'dev:[dir]NAME.TYPE'  */
    i = 0;
    for (j = 0; j < len; j++)
        if (file[j] == ']' || file[j] == ':')
            i = j + 1;
    for (j = 0; i < len && j < sizeof name - 1; i++) {
        if (file[i] == ' ' || file[i] == '.')
            break;
        name[j++] = file[i];
    }
    name[j] = '\0';

    if (!strcmp(name, "QUEST1"))      next_image = IMG_QUEST1;
    else if (!strcmp(name, "QUEST2")) next_image = IMG_QUEST2;
    else if (!strcmp(name, "QUEST3")) next_image = IMG_QUEST3;
    else if (!strcmp(name, "DNDOP"))  next_image = IMG_DNDOP;
    else {
        fprintf(stderr, "\r\n%%QUEST-F-NOIMAGE, cannot chain to %s\r\n", name);
        exit(1);
    }
    longjmp(chain_env, 1);
}

/*  FOR$EXIT, and SYS$DELPRC behind SUBROUTINE LOGOUT  */
void forexit_(void)
{
    exit(0);
}

void delprc_(void)   /* SYS$DELPRC */
{
    exit(0);
}

/*  The console reached end of input.  A scripted run simply stops; an
    interactive one cannot get here.  */
static int eof_run = 0;

void eof_seen(void)
{
    /*  A menu that treats a 24 second timeout as "H" will spin forever
        once a script runs out of keystrokes, so a short run of them ends
        the session instead of filling the transcript.  */
    if (++eof_run > 8) {
        const char *msg = "\r\n%QUEST-I-EOF, end of input\r\n";
        tty_put(msg, (int) strlen(msg));
        exit(0);
    }
}

void input_seen(void)
{
    eof_run = 0;
}

/* ------------------------------------------------------------------ */

static void usage(void)
{
    printf(
"QUEST -- Chris Kelley, Ball State University, 1984-85 (DECUS VAX85A)\n"
"\n"
"usage: quest [options]\n"
"  -f, --fast         skip the timed pauses the VAX used for pacing\n"
"  -s, --seed N       start the generator from N instead of the clock\n"
"      --freeze T     pin the clock, \"YYYY-MM-DD hh:mm:ss\"\n"
"  -h, --help         this text\n"
"\n"
"environment: QUEST_DATA, QUEST_USERNAME, QUEST_UIC, QUEST_SEED,\n"
"             QUEST_FREEZE, QUEST_FAST\n");
}

int main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fast"))
            no_delay = 1;
        else if ((!strcmp(argv[i], "-s") || !strcmp(argv[i], "--seed")) && i + 1 < argc) {
            static char buf[64];
            snprintf(buf, sizeof buf, "QUEST_SEED=%s", argv[++i]);
            putenv(buf);
        } else if (!strcmp(argv[i], "--freeze") && i + 1 < argc) {
            static char buf[128];
            snprintf(buf, sizeof buf, "QUEST_FREEZE=%s", argv[++i]);
            putenv(buf);
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "quest: unknown option %s\n", argv[i]);
            usage();
            return 2;
        }
    }
    if (getenv("QUEST_FAST"))
        no_delay = 1;

    find_datadir(argv[0]);
    tty_init();

    for (;;) {
        if (setjmp(chain_env) != 0) {
            closeall_();                 /* the abandoned image's files */
            kclose_();
        }
        running = next_image;
        switch (running) {
        case IMG_QUEST:  quest_();  break;
        case IMG_QUEST1: quest1_(); break;
        case IMG_QUEST2: quest2_(); break;
        case IMG_QUEST3: quest3_(); break;
        case IMG_DNDOP:  dndop_();  break;
        }
        break;                           /* an image that simply returns */
    }
    tty_put("\r\n", 2);
    return 0;
}
