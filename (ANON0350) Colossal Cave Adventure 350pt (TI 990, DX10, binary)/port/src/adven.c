/* adven.c - "ADVENTURE  Wandering adventure thru a Cave" from the TI 990
 * DX10 GAMES library: the ADVENTUR command procedure and the ADVEN task.
 *
 * The task is the original program, run on an emulated TI 990/10
 * (cpu990.c) under an emulation of the DX10 supervisor calls it makes
 * (dx10.c).  This file does what SCI did around it: the procedure's
 * questions about a save file (ADVENTUR, CAVE$), bidding the task, and the
 * SDT that shows the date and time when it ends. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cpu990.h"
#include "dx10.h"
#include "images.h"

static const char *prog = "adventure";

static void usage(FILE *f)
{
    fprintf(f,
"Usage: %s [OPTION]...\n"
"Colossal Cave Adventure (350 points) from the TI 990 DX10 GAMES library,\n"
"the original program run on an emulated 990/10.\n"
"\n"
"  -u, --unlimited  the cave never closes and a suspended game can be resumed\n"
"                   at once (the program's own prime-time and 90-minute rules)\n"
"      --no-fixes   run the original exactly, bugs included\n"
"      --clock=TIME start the clock at TIME (YYYY-MM-DD HH:MM:SS, or HH:MM:SS\n"
"                   today) and advance it one second each time it is read\n"
"      --trace      list the DX10 supervisor calls on stderr\n"
"  -h, --help       show this help and exit\n"
"\n"
"The game starts with the ADVENTUR procedure's question \"Will/did you save\n"
"your game?\".  Answer YES and give a file name to save (SAVE) or to resume a\n"
"saved game (RESTORE); answer NO (or just Enter) to play without one.\n",
            prog);
}

static void bad_option(const char *msg, const char *arg)
{
    fprintf(stderr, "%s: %s '%s'\nTry '%s --help' for more information.\n",
            prog, msg, arg, prog);
    exit(2);
}

static int parse_clock(const char *s)
{
    struct tm t;
    time_t now = time(NULL);
    int y, mo, d, h, mi, se;
    char sep;

    t = *localtime(&now);
    if (sscanf(s, "%d-%d-%d%c%d:%d:%d", &y, &mo, &d, &sep, &h, &mi, &se) == 7 &&
        (sep == ' ' || sep == 'T')) {
        t.tm_year = y - 1900;
        t.tm_mon = mo - 1;
        t.tm_mday = d;
    } else if (sscanf(s, "%d:%d:%d", &h, &mi, &se) != 3)
        return -1;
    if (h < 0 || h > 23 || mi < 0 || mi > 59 || se < 0 || se > 59)
        return -1;
    t.tm_hour = h;
    t.tm_min = mi;
    t.tm_sec = se;
    t.tm_isdst = -1;
    dx10.clock = mktime(&t);
    dx10.virtual_clock = 1;
    return dx10.clock == (time_t)-1 ? -1 : 0;
}

static void options(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "-u") || !strcmp(a, "--unlimited"))
            dx10.unlimited = 1;
        else if (!strcmp(a, "--no-fixes"))
            dx10.fixes = 0;
        else if (!strcmp(a, "--trace"))
            dx10.trace = 1;
        else if (!strncmp(a, "--clock=", 8)) {
            if (parse_clock(a + 8) < 0)
                bad_option("invalid time in", a);
        } else if (!strcmp(a, "--clock")) {
            if (++i >= argc)
                bad_option("option requires an argument --", "clock");
            if (parse_clock(argv[i]) < 0)
                bad_option("invalid time", argv[i]);
        } else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            exit(0);
        } else if (a[0] == '-' && a[1] == '-')
            bad_option("unrecognized option", a);
        else if (a[0] == '-')
            bad_option("invalid option", a);
        else
            bad_option("unexpected argument", a);
    }
}

/* the end of the input during the procedure: SCI would still be waiting */
static void hang_up(void)
{
    term_end();
    exit(0);
}

/* SCI's YESNO prompt: Y, YES, N, NO, or Enter for the initial value NO */
static int ask_yesno(const char *prompt)
{
    char ans[128];
    int n;

    for (;;) {
        term_puts(prompt);
        n = term_gets(ans, (int)sizeof ans - 1);
        if (n < 0)
            hang_up();
        ans[n] = 0;
        if (!n || !strcmp(ans, "N") || !strcmp(ans, "NO"))
            return 0;
        if (!strcmp(ans, "Y") || !strcmp(ans, "YES"))
            return 1;
        term_puts("\r\n**** INVALID PARAMETER VALUE ****");
    }
}

/* ADVENTUR, then CAVE$: "SAVE/RESTORE PATHNAME", created as an empty file
 * if it is new; an existing one is used only if the player says so */
static void procedure(void)
{
    static char path[1024];
    char ans[128];
    int n;

    if (!ask_yesno("Adventure thru a Cave\r\n Will/did you save your game?: NO  "))
        return;
    term_puts("\r\nAdventure thru a Cave\r\n SAVE/RESTORE PATHNAME:   ");
    for (;;) {
        FILE *f;

        do {
            n = term_gets_raw(path, (int)sizeof path - 1);
            if (n < 0)
                hang_up();
            path[n] = 0;
        } while (!n);
        f = fopen(path, "rb");
        if (f) {                        /* DX10 error >0026: it exists */
            fclose(f);
            term_puts("\r\nThe file exists, do you want to use it? (Y/N) : ");
            n = term_gets(ans, (int)sizeof ans - 1);
            if (n < 0)
                hang_up();
            ans[n] = 0;
            if (strcmp(ans, "Y") >= 0)  /* .UNTIL @ANS,GE,Y */
                break;
            term_puts("\r\n(DX10 Error 0026)   RE-ENTER SAVE/RESTORE PATHNAME: ");
            continue;
        }
        f = fopen(path, "wb");
        if (f) {
            fclose(f);
            break;
        }
        term_puts("\r\n(");
        term_puts(strerror(errno));
        term_puts(")   RE-ENTER SAVE/RESTORE PATHNAME: ");
    }
    dx10.save_path = path;
}

/* SDT: "20:10:49 SATURDAY, SEP 19, 2026 (262)" */
static void show_date_time(void)
{
    static const char *day[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
                                "THURSDAY", "FRIDAY", "SATURDAY"};
    static const char *mon[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    struct tm t;
    char line[80];

    dx10_now(&t);
    snprintf(line, sizeof line, "\r\n%02d:%02d:%02d %s, %s %d, %d (%d)", t.tm_hour,
             t.tm_min, t.tm_sec, day[t.tm_wday], mon[t.tm_mon], t.tm_mday,
             t.tm_year + 1900, t.tm_yday + 1);
    term_puts(line);
}

int main(int argc, char **argv)
{
    int r;

    options(argc, argv);
    procedure();
    memcpy(mem + PROC_BASE, adven_proc, PROC_SIZE);
    memcpy(mem + TASK_BASE, adven_task, TASK_SIZE);
    cpu_wp = rdw(TASK_BASE);            /* the transfer vector */
    cpu_pc = rdw(TASK_BASE + 2);
    cpu_st = 0x010F;                    /* user mode, interrupts enabled */
    cpu_svc = dx10_svc;
    cpu_watchdog = 500000000ULL;
    r = cpu_run();
    switch (r) {
    case DX10_END_TASK:
    case DX10_END_PROGRAM:
        show_date_time();
        term_end();
        return 0;
    case DX10_HANGUP:
        term_end();
        return 0;
    case CPU_FAULT:
        term_end();
        fprintf(stderr, "%s: the task stopped: %s\n", prog, cpu_fault);
        return 1;
    default:
        term_end();
        fprintf(stderr, "%s: the task stopped at >%04X\n", prog, cpu_pc);
        return 1;
    }
}
