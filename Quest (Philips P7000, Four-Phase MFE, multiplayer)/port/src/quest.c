/* quest.c - "QUEST UNDER MFE": QUEST version 1, the multi-player cave
 * game, as installed on a Philips P7000 - a Four-Phase Systems IV/90 - at
 * a Danish site.
 *
 * The port boots the site's disc pack on an emulated IV/90 Model 2 (cpu.c,
 * io.c) and does what the operator did: it starts MFE/7000 from IDOS, gives
 * MFE the time and the date, starts QUEST and tells it the number of
 * players.  The site's MFE drives six 7200 screens.  Terminal 0, the system
 * console, is the player in this window; every other player runs this
 * program in a window of their own, which joins the game over TCP as
 * terminal 1, 2, ... (net.c).  Each player signs on with Q, as QUEST's
 * manual says, and the port types that Q.  With --transcript (or when stdin
 * or stdout is not a console) terminal 0 is followed as a log and its lines
 * are read from stdin. */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include "fp4.h"
#include "net.h"

static const char *prog = "quest";

#define ROWS 24
#define COLS 81
#define LOGROWS 23                      /* QUEST's display: the lines above the input line */
#define MAXTERM 6                       /* the site's MFE drives six screens */
#define SETTLE 120                      /* transcript: clock ticks to wait before a line */

/* ---- options ---------------------------------------------------------------- */

static int opt_players = 1, opt_unlimited, opt_easy, opt_lan, opt_transcript, opt_fixed_clock;
static int opt_port = 7000;
static const char *opt_join;

static void usage(FILE *f)
{
    fprintf(f,
"Usage: %s [OPTION]...\n"
"QUEST version 1, the multi-player cave game of a Danish Philips P7000\n"
"(Four-Phase IV/90) site, run under MFE/7000 from the site's own disc pack on\n"
"an emulated IV/90 Model 2.\n"
"\n"
"  -p, --players=N       the number of players, 1 to 6 (default 1).  This\n"
"                        window starts the game; every other player starts\n"
"                        %s in a window of their own, which joins it\n"
"  -u, --unlimited       open the cave at any time: QUEST does not start on\n"
"                        weekdays from 9 to 12 and from 13 to 17 unless the\n"
"                        operator gives the site's password, and -u gives it\n"
"      --easy            play with the 'EASY' library; the site had the\n"
"                        'HARD' one installed as QLIB\n"
"      --port=N          the TCP port the players' windows use (default 7000)\n"
"      --lan             let players on other computers join (the game's\n"
"                        window shows the computer's name to give --join)\n"
"      --join=HOST[:PORT]  be a player in the game started on computer HOST,\n"
"                        a name or an address ([ADDRESS]:PORT for IPv6; a\n"
"                        window started without --join joins a game that\n"
"                        is waiting for players on this computer)\n"
"      --transcript      follow terminal 0 as a log and read its lines from\n"
"                        stdin (the default when stdin or stdout is not a\n"
"                        console)\n"
"      --fixed-clock     let the 60 Hz clock count instructions and give MFE\n"
"                        a fixed time and date (12:00, 9 April 1980), so that\n"
"                        a run can be repeated exactly\n"
"      --trace=FILE      write an instruction trace to FILE (debugging)\n"
"  -h, --help            show this help and exit\n"
"\n"
"Keys: Enter gives QUEST the line; Backspace, Left, Right, Home, Insert and\n"
"Delete edit it (Shift+Left deletes and Shift+Right inserts, as on the 7200);\n"
"Esc blanks it; Ctrl+Enter logs you off, as QUIT and STOP do; Ctrl+C closes\n"
"the window, and in the window that started the game it ends the game.\n"
"QHELP.txt is QUEST's own manual.\n",
            prog, prog);
}

static void bad_option(const char *msg, const char *arg)
{
    fprintf(stderr, "%s: %s '%s'\nTry '%s --help' for more information.\n", prog, msg, arg,
            prog);
    exit(2);
}

static int number(const char *s, int lo, int hi, const char *what)
{
    char *e;
    long v = strtol(s, &e, 10);

    if (!*s || *e || v < lo || v > hi) {
        fprintf(stderr, "%s: %s must be %d to %d, not '%s'\nTry '%s --help' for more "
                "information.\n", prog, what, lo, hi, s, prog);
        exit(2);
    }
    return (int)v;
}

static void long_option(const char *name, const char *val)
{
    if (!strcmp(name, "--players"))
        opt_players = number(val, 1, MAXTERM, "the number of players");
    else if (!strcmp(name, "--port"))
        opt_port = number(val, 1, 65535, "the port");
    else if (!strcmp(name, "--join")) {
        if (!*val)
            bad_option("missing computer name in", name);
        opt_join = val;
    } else if (!strcmp(name, "--trace")) {
        trace_fp = fopen(val, "w");
        if (!trace_fp)
            bad_option("cannot write", val);
    }
}

static void options(int argc, char **argv)
{
    static const char *with_arg[] = { "--players", "--port", "--join", "--trace", NULL };
    static const char *flags[] = { "--unlimited", "--easy", "--lan", "--transcript",
                                   "--fixed-clock", "--help", NULL };
    int i, k;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (a[0] == '-' && a[1] == '-' && a[2]) {
            const char *eq = strchr(a, '=');
            char name[40];
            size_t len = eq ? (size_t)(eq - a) : strlen(a);

            if (len >= sizeof name)
                bad_option("unrecognized option", a);
            memcpy(name, a, len);
            name[len] = 0;
            for (k = 0; with_arg[k] && strcmp(with_arg[k], name); k++)
                ;
            if (with_arg[k]) {
                if (eq)
                    long_option(name, eq + 1);
                else if (i + 1 < argc)
                    long_option(name, argv[++i]);
                else
                    bad_option("option requires an argument:", name);
                continue;
            }
            for (k = 0; flags[k] && strcmp(flags[k], name); k++)
                ;
            if (!flags[k])
                bad_option("unrecognized option", a);
            if (eq)
                bad_option("option does not take an argument:", a);
            if (!strcmp(name, "--unlimited"))
                opt_unlimited = 1;
            else if (!strcmp(name, "--easy"))
                opt_easy = 1;
            else if (!strcmp(name, "--lan"))
                opt_lan = 1;
            else if (!strcmp(name, "--transcript"))
                opt_transcript = 1;
            else if (!strcmp(name, "--fixed-clock"))
                opt_fixed_clock = 1;
            else {
                usage(stdout);
                exit(0);
            }
        } else if (a[0] == '-' && a[1] && a[1] != '-') {
            const char *s;

            for (s = a + 1; *s; s++) {
                char opt[2] = { *s, 0 };

                if (*s == 'u')
                    opt_unlimited = 1;
                else if (*s == 'h') {
                    usage(stdout);
                    exit(0);
                } else if (*s == 'p') {
                    if (s[1])
                        long_option("--players", s + 1);
                    else if (i + 1 < argc)
                        long_option("--players", argv[++i]);
                    else
                        bad_option("option requires an argument --", "p");
                    break;
                } else
                    bad_option("invalid option --", opt);
            }
        } else
            bad_option("unexpected argument", a);
    }
}

/* ---- the disc pack ------------------------------------------------------------ */

static void open_pack(const char *argv0)
{
    char path[4096], *p;

    if (exe_path(path, sizeof path) < 0)
        snprintf(path, sizeof path, "%s", argv0);
    p = strrchr(path, '\\');
    if (!p || strrchr(path, '/') > p)
        p = strrchr(path, '/');
    if (p)
        p[1] = 0;
    else
        path[0] = 0;
    strncat(path, "p7000.pack", sizeof path - strlen(path) - 1);
    /* read only: nothing QUEST or MFE writes needs to outlive a game */
    if (disc_open(path, 0) < 0) {
        fprintf(stderr, "%s: cannot read the disc pack %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
}

/* the first sector and the length of file NAME, from the IDOS directory
 * ($DIR, sectors 0-037: entries of 8 words - the name in words 0-1, the
 * length - 1 in word 3 bits 15-23 and word 4 bits 0-8, the start in word 5) */
static long pack_file(const char *name, long *count)
{
    long s, k;

    for (s = 0; s < 040; s++)
        for (k = 0; k < 256; k += 8) {
            long w = s * 256 + k;
            char n[7];
            int j;

            for (j = 0; j < 3; j++) {
                n[j] = (char)(disc_peek(w) >> (16 - 8 * j) & 0177);
                n[j + 3] = (char)(disc_peek(w + 1) >> (16 - 8 * j) & 0177);
            }
            for (j = 6; j > 0 && n[j - 1] == ' '; j--)
                ;                               /* names are padded with blanks */
            n[j] = 0;
            if (!strcmp(n, name)) {
                *count = (long)((disc_peek(w + 3) & 0777) << 9 | disc_peek(w + 4) >> 15) + 1;
                return (long)(disc_peek(w + 5) & 0777777);
            }
        }
    return -1;
}

/* The site had copied the hard library QLHRDS over QLIB (whose first
 * record says "THIS IS THE 'HARD' QUEST VERSION 1 LIBRARY") and kept the
 * one QUEST came with as QLIBSV ("... 'EASY' ...").  --easy puts QLIBSV's
 * data back into QLIB, as the site would have with COPY - in memory only.
 * Both are chained files of the same length; a sector's word 1 (the chain)
 * and the low 12 bits of word 0 depend on where it lies, so QLIB keeps its
 * own. */
static int library_is(long sector, const char *kind)
{
    char text[3 * 40 + 1];
    int i;

    for (i = 0; i < 40; i++) {
        word v = disc_peek(sector * 256 + 2 + i);

        text[3 * i] = (char)(v >> 16 & 0177);
        text[3 * i + 1] = (char)(v >> 8 & 0177);
        text[3 * i + 2] = (char)(v & 0177);
    }
    text[sizeof text - 1] = 0;
    for (i = 0; i < (int)sizeof text - 1; i++)
        if (!text[i])
            text[i] = ' ';
    return strstr(text, kind) != NULL;
}

static void easy_library(void)
{
    long nlib, nsv, lib = pack_file("QLIB", &nlib), sv = pack_file("QLIBSV", &nsv), k, w;
    long first = lib;

    if (lib < 0 || sv < 0 || nlib != nsv || !library_is(sv, "'EASY'")) {
        fprintf(stderr, "%s: the pack does not hold QUEST's libraries QLIB and QLIBSV\n", prog);
        exit(1);
    }
    for (k = 0; k < nlib; k++) {
        long d = lib * 256, s = sv * 256;

        disc_poke(d, (disc_peek(s) & ~07777u) | (disc_peek(d) & 07777u));
        for (w = 2; w < 256; w++)
            disc_poke(d + w, disc_peek(s + w));
        lib = (long)(disc_peek(d + 1) & 07777);         /* the next sectors */
        sv = (long)(disc_peek(s + 1) & 07777);
        if ((!lib || !sv) && k + 1 < nlib) {
            fprintf(stderr, "%s: the chains of QLIB and QLIBSV do not match\n", prog);
            exit(1);
        }
    }
    if (!library_is(first, "'EASY'")) {
        fprintf(stderr, "%s: QLIB did not take the easy library\n", prog);
        exit(1);
    }
}

/* ---- the screens ---------------------------------------------------------------- */

/* the byte at row R, column C of terminal T's screen: 24 lines of 32 words
 * (81 characters used) at 0140 in physical page T */
static int scr_byte(int t, int r, int c)
{
    word v = mem[((word)t << 10) + 0140 + (word)(r * 32 + c / 3)];

    return (int)(v >> (16 - 8 * (c % 3))) & 0377;
}

/* row R of terminal T as text, from column C0, trailing blanks dropped */
static void row_text(int t, int r, int c0, char *out)
{
    int c, n = 0, last = 0;

    for (c = c0; c < COLS; c++) {
        int b = scr_byte(t, r, c);

        b = b >= 0300 ? ' ' : b & 0177;
        if (b < 040 || b == 0177)
            b = ' ';
        out[n++] = (char)b;
        if (b != ' ')
            last = n;
    }
    out[last] = 0;
}

static int row_has(int t, int r, const char *s)
{
    char b[COLS + 1];

    row_text(t, r, 0, b);
    return strstr(b, s) != NULL;
}

static int screen_has(int t, const char *s)
{
    int r;

    for (r = 0; r < ROWS; r++)
        if (row_has(t, r, s))
            return 1;
    return 0;
}

/* terminal T shows MFE's own screen, not QUEST's */
static int in_mfe(int t)
{
    return row_has(t, 23, "MFE/7000 IS READY");
}

/* ---- showing a screen: ANSI text for a console ------------------------------------ */

enum { L_NORMAL, L_BRIGHT, L_HIDDEN, L_STATUS };

typedef struct {
    unsigned char ch, look;
} Cell;

typedef struct {
    Cell shown[ROWS + 1][COLS];         /* row ROWS: the port's status line */
    int valid, cur_r, cur_c;
    int beeps;                          /* the terminal's alarms passed on */
} View;

typedef struct {
    char *p;
    int n, cap;
} Buf;

static void put(Buf *b, const char *s, int n)
{
    if (n < 0)
        n = (int)strlen(s);
    if (b->n + n > b->cap) {
        int cap = b->cap ? b->cap * 2 : 8192;

        while (cap < b->n + n)
            cap *= 2;
        b->p = realloc(b->p, (size_t)cap);
        if (!b->p) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, (size_t)n);
    b->n += n;
}

static void putf(Buf *b, const char *fmt, ...)
{
    char s[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(s, sizeof s, fmt, ap);
    va_end(ap);
    put(b, s, -1);
}

/* The site's 7200s use the "300" attributes: a byte 0300-0377 shows as a
 * blank and sets the look of what follows (bits 4-5: 00 or 01 normal, 10
 * bright, 11 hidden) up to the next one.  The screen is refreshed from the
 * top, so the top has the look of the last attribute on the screen (MFE
 * ends its status line with 0310, QUEST starts every line with it).  Other
 * bytes show as 7-bit characters; 032 is the cursor. */
static int attr_look(int b)
{
    switch (b >> 2 & 3) {
    case 2: return L_BRIGHT;
    case 3: return L_HIDDEN;
    default: return L_NORMAL;
    }
}

static void screen_cells(int t, Cell cell[ROWS + 1][COLS], int *cur_r, int *cur_c)
{
    int r, c, i, look = L_NORMAL;

    for (i = ROWS * COLS - 1; i >= 0; i--) {
        int b = scr_byte(t, i / COLS, i % COLS);

        if (b >= 0300) {
            look = attr_look(b);
            break;
        }
    }
    *cur_r = *cur_c = -1;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++) {
            int b = scr_byte(t, r, c);

            if (b >= 0300) {
                look = attr_look(b);
                b = ' ';
            } else {
                b &= 0177;
                if (b == 032) {
                    *cur_r = r;
                    *cur_c = c;
                    b = ' ';
                } else if (b < 040 || b == 0177)
                    b = ' ';
            }
            cell[r][c].ch = (unsigned char)(look == L_HIDDEN ? ' ' : b);
            cell[r][c].look = (unsigned char)look;
        }
}

static const char *const sgr[] = { "\033[0;32m", "\033[0;92m", "\033[0;32m", "\033[0;30;42m" };

/* the changes since the last time, as ANSI text */
static void render(View *v, int t, const char *status, Buf *out)
{
    Cell cell[ROWS + 1][COLS];
    int r, c, cr, cc, look = -1, at_r = -1, at_c = -1, hidden = 0;

    screen_cells(t, cell, &cr, &cc);
    for (c = 0; c < COLS; c++) {
        cell[ROWS][c].ch = (unsigned char)(c < COLS - 1 && c < (int)strlen(status) ? status[c] : ' ');
        cell[ROWS][c].look = L_STATUS;
    }
    if (!v->valid) {
        put(out, "\033[?25l\033[0m\033[2J", -1);
        memset(v->shown, 0, sizeof v->shown);
        v->cur_r = v->cur_c = -2;
        v->valid = hidden = 1;
    }
    for (r = 0; r <= ROWS; r++)
        for (c = 0; c < COLS; c++) {
            Cell *n = &cell[r][c], *o = &v->shown[r][c];

            if (r == ROWS && c == COLS - 1)
                continue;                       /* never the last cell: no scrolling */
            if (n->ch == o->ch && n->look == o->look)
                continue;
            if (!hidden) {
                put(out, "\033[?25l", -1);
                hidden = 1;
            }
            if (at_r != r || at_c != c)
                putf(out, "\033[%d;%dH", r + 1, c + 1);
            if (n->look != look) {
                put(out, sgr[n->look], -1);
                look = n->look;
            }
            put(out, (const char *)&n->ch, 1);
            *o = *n;
            at_r = r;
            at_c = c + 1;
        }
    if (hidden || cr != v->cur_r || cc != v->cur_c) {
        if (cr >= 0)
            putf(out, "\033[%d;%dH\033[?25h", cr + 1, cc + 1);
        else if (!hidden)
            put(out, "\033[?25l", -1);
        v->cur_r = cr;
        v->cur_c = cc;
    }
    if (kbd_beeps[t] != v->beeps) {
        put(out, "\a", 1);
        v->beeps = kbd_beeps[t];
    }
}

/* ---- the players ------------------------------------------------------------------ */

enum { T_IDLE, T_SIGNON, T_PLAYING, T_LEFT };

typedef struct {
    int local;                          /* the player at this window (terminal 0) */
    int conn;                           /* the player's connection, or -1 */
    int state;
    unsigned long long mfe_since;       /* clock tick its screen went back to MFE's */
    int iac;                            /* a telnet command being skipped */
    int logoff;                         /* logging off a player whose window closed */
    unsigned long long logoff_at;       /* ... the clock tick of the last step */
    long logoff_mark;                   /* ... the first display line newer than that */
    View view;
} Term;

static Term term[MAXTERM];
static int nterm;                       /* the terminals in the game: --players */
static int local_console;               /* this window shows terminal 0 */
static int listening;
static volatile long ctrl_stop;

static int attached(const Term *p)
{
    return p->local || p->conn >= 0;
}

static int netlog;                      /* debugging: QUEST_NETLOG, the players' comings and goings */

static void note(const char *fmt, ...)
{
    va_list ap;

    if (!netlog)
        return;
    fprintf(stderr, "[%llu tick %llu] ", icount, clock_ticks);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

/* a key from a player, as net.h codes it, for terminal T */
static void term_key(int t, int k)
{
    char b[COLS + 2];
    int n = 0, i;

    if (k >= 040 && k <= 0176)
        b[n++] = (char)k;               /* MFE takes small letters as capitals */
    else if (k == K_ENTER)
        b[n++] = (char)0215;
    else if (k == K_BACKSPACE) {
        b[n++] = (char)0201;            /* the cursor back, then DELETE */
        b[n++] = (char)0220;
    } else if (k == K_ESC) {
        /* MODE (ATTN) blanks the line - but at terminal 0 it is the System
         * Console key, and MFE would take the screen over; blank the line
         * with HOME and DELETEs there */
        if (t == 0) {
            b[n++] = (char)0210;
            for (i = 0; i < COLS - 1; i++)
                b[n++] = (char)0220;
        } else
            b[n++] = (char)0205;
    } else if (k == K_UP || k == K_LEFT || k == K_RIGHT || k == K_DOWN || k == K_HOME ||
               k == K_INSERT || k == K_DELETE || k == K_LOGOFF)
        b[n++] = (char)k;
    if (n)
        kbd_type(t, b, n);
}

static void type_text(int t, const char *s)
{
    kbd_type(t, s, (int)strlen(s));
}

/* ---- the transcript: terminal 0 followed as a scrolling log --------------------------- */

static int transcript;                  /* follow terminal 0, read stdin */
static int tr_on;                       /* the log has begun */
static long nscroll;                    /* scrolls so far: row R = log line nscroll + R */
static long printed;                    /* log lines before this are out */
static unsigned long long tr_ready;     /* the clock tick to read the next line at */
static int stdin_tty;

static void emit(const char *s)
{
    fputs(s, stdout);
    fputc('\n', stdout);
}

/* a log line is a row of QUEST's display from column 1: column 0 holds its
 * attribute */
static void log_row(int r, char *out)
{
    row_text(0, r, 1, out);
}

/* everything on the screen now is old */
static void transcript_mark(void)
{
    printed = nscroll + LOGROWS;
    tr_on = 1;
}

static void transcript_flush(void);

static long tscroll[MAXTERM];           /* each terminal's display scrolls so far */

/* an MVEL is about to write into terminal PAGE's display: to scroll it up
 * a line (line 1 onto line 0), or to clear it or put MFE's screen back */
static void on_screen(int page, int scroll)
{
    if (scroll && page < MAXTERM)
        tscroll[page]++;
    if (page != 0)
        return;
    if (!scroll) {
        if (tr_on) {
            transcript_flush();
            transcript_mark();
        }
        return;
    }
    if (tr_on && nscroll >= printed) {
        char b[COLS + 1];

        log_row(0, b);
        emit(b);
        printed = nscroll + 1;
    }
    nscroll++;
}

static void transcript_flush(void)
{
    char b[COLS + 1];
    int last, r;

    if (!tr_on)
        return;
    for (last = LOGROWS - 1; last >= 0; last--) {
        log_row(last, b);
        if (b[0])
            break;
    }
    for (r = (int)(printed - nscroll); r <= last; r++)
        if (r >= 0) {
            log_row(r, b);
            emit(b);
        }
    if (last >= printed - nscroll)
        printed = nscroll + last + 1;
    fflush(stdout);
}

/* terminal 0's input line holds nothing typed (the cursor blinks: MFE
 * puts the 032 in and takes it out again) */
static int input_line_empty(void)
{
    char b[COLS + 1];

    row_text(0, 23, 1, b);
    return !b[0];
}

static void the_end(const char *why, int code);

/* a line from stdin for terminal 0, once QUEST has been waiting for one for
 * SETTLE clock ticks.  QUEST holds one line per player: until it has taken
 * the last one (an action takes time), it refuses every key with a beep and
 * shows none of them.  A line of which nothing showed is typed again, as a
 * player would. */
static char tr_line[512];               /* the line typed last */
static int tr_watch, tr_shown;          /* ... not yet known to be taken; some of it showed */

static void transcript_step(void)
{
    size_t n, i;

    if (term[0].state != T_SIGNON && term[0].state != T_PLAYING)
        return;
    if (tr_watch && !input_line_empty())
        tr_shown = 1;
    if (kbd_pending(0) || !input_line_empty()) {
        tr_ready = 0;
        return;
    }
    if (!tr_ready) {
        tr_ready = clock_ticks + SETTLE;
        return;
    }
    if (clock_ticks < tr_ready)
        return;
    transcript_flush();
    if (tr_watch && !tr_shown) {
        if (netlog)
            note("terminal 0: QUEST refused the line \"%s\"; typing it again", tr_line);
    } else
        switch (stdin_line(tr_line, sizeof tr_line)) {
        case 0:
            tr_watch = 0;
            return;                             /* not yet: the others play on */
        case -1:
            the_end(NULL, 0);
        }
    tr_ready = 0;
    n = strlen(tr_line);
    tr_watch = strspn(tr_line, " \t") < n;      /* a blank line shows nothing */
    tr_shown = 0;
    for (i = 0; i < n; i++)
        term_key(0, (unsigned char)tr_line[i]);
    term_key(0, K_ENTER);
}

/* ---- the operator: MFE and QUEST started as at the site ------------------------------------ */

enum { OP_BOOT, OP_IDOS, OP_HOUR, OP_MINUTE, OP_DATE, OP_COMMAND, OP_START, OP_COUNT, OP_PLAY };
static int op = OP_BOOT;
static unsigned long long op_tick;      /* when the operator may go on */
static int password_given;
static char end_text[160];              /* why the game could not be played */

/* the time and the date for MFE: now, or 12:00 on 9 April 1980 (the date
 * the site's utility wrote the tape); for tests, QUEST_TIME=HH,MM,DDMMYY */
static void now_text(int what, char *out)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    const char *fake = getenv("QUEST_TIME");
    int hh, mm;
    char date[7];

    if (fake && sscanf(fake, "%2d,%2d,%6[0-9]", &hh, &mm, date) == 3 && strlen(date) == 6) {
        if (what == 0)
            sprintf(out, "%02d", hh);
        else if (what == 1)
            sprintf(out, "%02d", mm);
        else
            strcpy(out, date);
    } else if (opt_fixed_clock || !tm)
        strcpy(out, what == 0 ? "12" : what == 1 ? "00" : "090480");
    else if (what == 0)
        sprintf(out, "%02d", tm->tm_hour);
    else if (what == 1)
        sprintf(out, "%02d", tm->tm_min);
    else
        sprintf(out, "%02d%02d%02d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100);
}

static void answer(const char *s)
{
    char b[32];

    snprintf(b, sizeof b, "%s\215", s);
    type_text(0, b);
}

/* SHOWN: the operator's prompt (of this kind) is on the screen.  True once
 * it has been there for half a second: a program prints the whole question
 * before it reads, and a key typed sooner goes to MFE's command line. */
static unsigned long long seen_at, rsp_at, msg_at;
static int seen_kind;

static int prompt(int shown)
{
    if (!shown || shown != seen_kind) {
        seen_kind = shown;
        seen_at = shown ? clock_ticks : 0;
        return 0;
    }
    if (clock_ticks < seen_at + 30)
        return 0;
    seen_kind = 0;
    return 1;
}

static void operator_step(void)
{
    char b[16];
    int which;

    if (op == OP_PLAY || kbd_pending(0) || clock_ticks < op_tick)
        return;
    switch (op) {
    case OP_BOOT:                               /* IDOS's $BATCH is up: the site's job */
        if (row_has(0, 1, "// $BATCH") && kbd_waiting()) {
            type_text(0, "// MFE\215/C=DAMHUS\215//\215");
            op = OP_IDOS;
        }
        break;
    case OP_IDOS:                               /* MFE is up: to its console */
        if (in_mfe(0)) {
            kbd_free = 1;
            type_text(0, "\205");
            op = OP_HOUR;
            /* MFE/7000 BN03-C waits for work in a loop at 06004-06012 of
             * window 020 (IN 707, UP 706, ..., BRA 6004); time spent there
             * lets the port sleep */
            if (wrd(020, 06004) == 034700707u && wrd(020, 06006) == 033700706u &&
                wrd(020, 06012) == 072006004u) {
                idle_win = 020;
                idle_lo = 06004;
                idle_hi = 06012;
            }
        }
        break;
    case OP_HOUR:
        if (prompt(screen_has(0, "ENTER CURRENT HOUR"))) {
            now_text(0, b);
            answer(b);
            op = OP_MINUTE;
        }
        break;
    case OP_MINUTE:
        if (prompt(screen_has(0, "ENTER CURRENT MINUTE"))) {
            now_text(1, b);
            answer(b);
            op = OP_DATE;
        }
        break;
    case OP_DATE:
        if (prompt(screen_has(0, "TYPE DDMMYY"))) {
            now_text(2, b);
            answer(b);
            op = OP_COMMAND;
        }
        break;
    case OP_COMMAND:
        if (prompt(row_has(0, 23, "COMMAND"))) {
            answer("START,QUEST");
            op = OP_START;
        }
        break;
    case OP_START:
        if (screen_has(0, "QUEST FATAL ERROR")) {
            snprintf(end_text, sizeof end_text, "QUEST stopped with a fatal error (see the "
                     "screen); QHELP.txt lists them.");
            break;
        }
        /* MFE's console shows RSP when a program waits for a response, and
         * MSG when more messages are queued: the Arrow Up key shows the
         * next (after the password, QUEST's question waits behind MSG).
         * Both blink, so each counts for a second after it was seen. */
        if (row_has(0, 22, "RSP"))
            rsp_at = clock_ticks + 1;
        if (row_has(0, 22, "MSG"))
            msg_at = clock_ticks + 1;
        if (!rsp_at || clock_ticks > rsp_at + 60)
            which = msg_at && clock_ticks <= msg_at + 60 ? 3 : 0;
        else if (!password_given && screen_has(0, "(ENTER PASSWORD OR CURSOR RETURN)"))
            which = 2;
        else
            which = screen_has(0, "THE LIMIT IS TWENTY PLAYERS.") ? 1 : 0;
        if (!prompt(which))
            break;
        rsp_at = msg_at = 0;
        if (which == 3)
            type_text(0, "\200");
        else if (which == 1) {                  /* the number of players */
            sprintf(b, "%d", nterm);
            answer(b);
            op = OP_COUNT;
            op_tick = clock_ticks + 60;
        } else {
            /* "SORRY -- THE CAVE IS CLOSED NOW." (weekdays 9-12 and 13-17).
             * The password is in QUEST's code, in a field after "NOTE TO SE:
             * USE CRTDMP TO CHANGE PASSWORD:". */
            if (opt_unlimited)
                answer("VISION S@");
            else {
                answer("");
                snprintf(end_text, sizeof end_text, "The cave is closed now: QUEST does not run "
                         "on weekdays 9-12 and 13-17.  -u opens it.");
            }
            password_given = 1;
        }
        break;
    case OP_COUNT:                              /* the console back to terminal 0 */
        type_text(0, "\205");
        op = OP_PLAY;
        if (smc_fp && !ran_map) {               /* debugging: QUEST_SMC watches from here */
            ran_map = calloc(PHYSWORDS, 1);
            if (getenv("QUEST_WATCH"))
                watch_arm(getenv("QUEST_WATCH"));
        }
        break;
    }
}

/* ---- the players' terminals ------------------------------------------------------------------ */

static int game_started(void)
{
    int t;

    for (t = 0; t < nterm; t++)
        if (term[t].state == T_IDLE || term[t].state == T_SIGNON)
            return 0;
    return 1;
}

static char lan_name[64];               /* --lan: this computer's name, for --join */

static const char *status_of(int t)
{
    static char s[256];
    int t2, here = 0;

    for (t2 = 0; t2 < nterm; t2++)
        here += attached(&term[t2]) || term[t2].state >= T_PLAYING;
    if (end_text[0])
        snprintf(s, sizeof s, " %s", end_text);
    else if (op < OP_PLAY)
        snprintf(s, sizeof s, " The operator is starting MFE and QUEST (terminal %d).", t);
    else if (term[t].state == T_LEFT && term[t].local)
        snprintf(s, sizeof s, " You have left QUEST; it goes on for the others.  Ctrl+C ends "
                 "it for all.");
    else if (term[t].state == T_LEFT)
        snprintf(s, sizeof s, " You have left QUEST.  Press a key to close the window.");
    else if (here < nterm && lan_name[0])
        snprintf(s, sizeof s, " %d of %d players here: the others run %s --join=%s.  Ctrl+C "
                 "quits.", here, nterm, prog, lan_name);
    else if (here < nterm)
        snprintf(s, sizeof s, " QUEST for %d players, %d here: the others start %s to join.  "
                 "Ctrl+C quits.", nterm, here, prog);
    else
        snprintf(s, sizeof s, " QUEST, terminal %d.  Ctrl+Enter or QUIT logs off; Ctrl+C "
                 "closes the window.", t);
    s[COLS - 1] = 0;
    return s;
}

/* show terminal T to its player */
static void draw(int t)
{
    Term *p = &term[t];
    Buf b = { 0 };

    if (!(p->local && local_console) && p->conn < 0)
        return;
    render(&p->view, t, status_of(t), &b);
    if (b.n) {
        if (p->local)
            con_write(b.p, b.n);
        else if (net_send(p->conn, b.p, b.n) < 0) {
            net_close(p->conn);
            p->conn = -1;
        }
    }
    free(b.p);
}

/* the player at terminal T has logged off, or QUEST has ended */
static void left_game(int t)
{
    Term *p = &term[t];

    note("terminal %d: back at MFE's screen, the player has left", t);
    if (p->conn >= 0) {
        draw(t);
        net_close(p->conn);                     /* the window waits for a key */
        p->conn = -1;
    }
}

static void logoff_step(int t);

static void terms_step(void)
{
    int t;

    for (t = 0; t < nterm; t++) {
        Term *p = &term[t];

        switch (p->state) {
        case T_IDLE:                            /* the player signs on with Q */
            if (op == OP_PLAY && attached(p) && in_mfe(t) && !kbd_pending(t)) {
                type_text(t, "Q");
                p->state = T_SIGNON;
                if (t == 0 && transcript)
                    transcript_mark();
            }
            break;
        case T_SIGNON:                          /* QUEST's screen is up */
            if (!in_mfe(t))
                p->state = T_PLAYING;
            break;
        case T_PLAYING:                         /* back at MFE's screen for half a second */
            if (p->logoff)
                logoff_step(t);
            if (!in_mfe(t))
                p->mfe_since = 0;
            else if (!p->mfe_since)
                p->mfe_since = clock_ticks + 1;
            else if (clock_ticks >= p->mfe_since + 30) {
                p->state = T_LEFT;
                if (t == 0)
                    tr_on = 0;                  /* the log ends with QUEST's screen */
                left_game(t);
            }
            break;
        }
    }
}

/* everyone who signed on has left, so QUEST has ended */
static int game_over(void)
{
    int t, left = 0;

    for (t = 0; t < nterm; t++) {
        if (term[t].state == T_SIGNON || term[t].state == T_PLAYING)
            return 0;
        left += term[t].state == T_LEFT;
    }
    return left > 0;
}

/* ---- players joining over the network ----------------------------------------------------------- */

static void refuse(int id, const char *why)
{
    char s[256];

    snprintf(s, sizeof s, "\033[0m\033[2J\033[H%s\r\n", why);
    net_send(id, s, (int)strlen(s));
    net_close(id);
}

static void new_player(int id)
{
    int t, pick = -1;

    for (t = 1; t < nterm && pick < 0; t++)
        if (!attached(&term[t]) && term[t].state == T_IDLE)
            pick = t;
    for (t = 1; t < nterm && pick < 0; t++)     /* one who left before signing on */
        if (!attached(&term[t]) && term[t].state == T_SIGNON)
            pick = t;
    if (pick < 0) {
        char s[160];

        snprintf(s, sizeof s, "QUEST: this game is for %d player%s, and %s.", nterm,
                 nterm == 1 ? "" : "s", game_started() ? "it has begun" : "they are all here");
        note("connection %d refused", id);
        refuse(id, s);
        return;
    }
    note("connection %d is terminal %d (state %d)", id, pick, term[pick].state);
    term[pick].conn = id;
    term[pick].iac = 0;
    term[pick].view.valid = 0;
    term[pick].view.beeps = kbd_beeps[pick];
}

/* Logging off a player whose window has closed: MODE blanks the line and
 * Control CURSOR RETURN makes QUEST ask "Do you really want to quit the
 * game?".  The YES goes in only once that question is on the screen (a YES
 * typed ahead of it can be lost), and after five seconds with no question
 * the port starts again. */
static void logoff_start(int t)
{
    Term *p = &term[t];

    type_text(t, "\205\376");
    p->logoff = 1;
    p->logoff_at = clock_ticks;
    p->logoff_mark = tscroll[t] + LOGROWS;
}

static void logoff_step(int t)
{
    Term *p = &term[t];
    int r;

    if (kbd_pending(t))
        return;
    if (p->logoff == 1)
        for (r = 0; r < LOGROWS; r++)
            if (tscroll[t] + r >= p->logoff_mark &&
                row_has(t, r, "Do you really want to quit the game?")) {
                type_text(t, "YES\215");
                p->logoff = 2;
                p->logoff_at = clock_ticks;
                return;
            }
    if (clock_ticks >= p->logoff_at + 300) {
        note("terminal %d: still in the game; logging off again", t);
        logoff_start(t);
    }
}

/* a player's window has closed */
static void gone(int t)
{
    note("terminal %d: the connection closed (state %d, keys pending %d)", t, term[t].state,
         kbd_pending(t));
    net_close(term[t].conn);
    term[t].conn = -1;
    if (term[t].state == T_PLAYING)
        logoff_start(t);
}

static void net_step(void)
{
    int id, t;

    while ((id = net_accept()) >= 0)
        new_player(id);
    for (t = 1; t < nterm; t++) {
        Term *p = &term[t];
        unsigned char buf[512];
        int n, i;

        while (p->conn >= 0 && (n = net_recv(p->conn, buf, sizeof buf)) != 0) {
            if (n < 0) {
                gone(t);
                break;
            }
            for (i = 0; i < n; i++) {
                int k = buf[i];

                if (p->iac) {                   /* telnet: IAC cmd [opt], IAC SB .. IAC SE */
                    if (p->iac == 1)
                        p->iac = k >= 0373 && k <= 0376 ? 2 : k == 0372 ? 3 : 0;
                    else if (p->iac == 2)
                        p->iac = 0;
                    else if (p->iac == 3 && k == 0377)
                        p->iac = 4;
                    else if (p->iac == 4)
                        p->iac = k == 0360 ? 0 : 3;
                    continue;
                }
                if (k == 0377) {
                    p->iac = 1;
                    continue;
                }
                if (p->state == T_SIGNON || p->state == T_PLAYING)
                    term_key(t, k);
            }
        }
    }
    net_service();
}

/* ---- the run -------------------------------------------------------------------------------------- */

static void finish(int code)
{
    if (local_console)
        con_close();
    if (listening)
        net_shutdown();
    disc_close();
    if (trace_fp)
        fclose(trace_fp);
    exit(code);
}

static void fault(void)
{
    int t;

    if (local_console)
        con_close();
    local_console = 0;
    fprintf(stderr, "%s: the machine stopped: %s\n", prog, stop_text);
    if (trace_fp)
        trace_dump_ring(trace_fp, 200);
    for (t = 1; t < nterm; t++)
        if (term[t].conn >= 0)
            refuse(term[t].conn, "QUEST: the machine has stopped; the game is over.");
    finish(1);
}

static void run_some(int n)
{
    int i;

    for (i = 0; i < n && !stop_code; i++)
        cpu_step();
}

static int keys_pending(void)
{
    int t;

    for (t = 0; t < nterm; t++)
        if (kbd_pending(t))
            return 1;
    return 0;
}

static int quitting;                    /* Ctrl+C: no key to wait for */
static unsigned long long max_lag;      /* real time: the most clock ticks the machine was behind */

/* the end: the last screens, and the players' windows closed */
static void the_end(const char *why, int code)
{
    int t;

    if (why && !end_text[0])
        snprintf(end_text, sizeof end_text, "%s", why);
    if (!opt_fixed_clock)
        note("the clock: %llu ticks given, %llu due; in the game the machine was %llu ticks behind "
             "at most", clock_ticks, clock_due(), max_lag);
    for (t = 0; t < nterm; t++)
        if (term[t].conn >= 0) {
            draw(t);
            net_close(term[t].conn);
            term[t].conn = -1;
        }
    if (listening) {                            /* let the last screens go out */
        unsigned long long until = net_ms() + 2000;

        while (net_pending() && net_ms() < until) {
            net_service();
            net_wait(20, 0);
        }
    }
    if (transcript) {
        transcript_flush();
        if (end_text[0] && code)
            fprintf(stderr, "%s: %s\n", prog, end_text);
    } else if (local_console && !quitting) {
        draw(0);
        con_wait_key();
    }
    finish(code);
}

static unsigned long long stop_at;      /* debugging: QUEST_STOP_AT */

/* debugging: the state and every screen, on stderr */
static void debug_stop(void)
{
    int t, r;

    fprintf(stderr, "stopped at %llu (tick %llu): operator %d, window %o, RP %05o\n", icount,
            clock_ticks, op, cur_win, RP);
    for (t = 0; t < nterm || t < 2; t++) {
        fprintf(stderr, "---- terminal %d: state %d, keys pending %d\n", t, term[t].state,
                kbd_pending(t));
        for (r = 0; r < ROWS; r++) {
            char b[COLS + 1];
            int c;

            row_text(t, r, 0, b);
            fprintf(stderr, "%2d|%s\n", r, b);
            for (c = 0; c < COLS; c++)
                if ((scr_byte(t, r, c) & 0177) == 032 || (scr_byte(t, r, c) >= 0200 &&
                                                         scr_byte(t, r, c) != 0310))
                    fprintf(stderr, "   [%d,%d] = %03o\n", r, c, scr_byte(t, r, c));
        }
    }
    if (local_console)
        con_close();
    exit(3);
}

/* In real time the machine runs as fast as the host lets it until the game
 * is on.  Then it gets THROTTLE instructions a clock tick, four times what
 * the fixed clock gives it: QUEST's main loop never stops looking for work,
 * and the machine it was written for was no faster. */
#define THROTTLE 32000UL

static void run(void)
{
    unsigned long long next_draw = 0, budget_tick = 0;
    unsigned long budget = 0;
    unsigned step = 0;

    for (;;) {
        unsigned long long now;
        int t, idle, held = 0;

        if (!opt_fixed_clock && op == OP_PLAY) {
            unsigned long long due = clock_due();

            if (clock_ticks != budget_tick) {
                budget_tick = clock_ticks;
                budget = 0;
            }
            held = budget >= THROTTLE && due <= clock_ticks;
            if (due > clock_ticks && due - clock_ticks > max_lag)
                max_lag = due - clock_ticks;
        }
        idle_hits = 0;
        if (!held) {
            run_some(8192);
            budget += 8192;
        }
        /* waiting: a short loop, or a quarter of the time in MFE's */
        idle = cpu_idle() || idle_hits >= 8192 / 4;
        if (stop_code)
            fault();
        if (stop_at && icount >= stop_at)
            debug_stop();
        if ((++step & 3) == 0)
            operator_step();
        terms_step();
        if (end_text[0] && op != OP_PLAY && !kbd_pending(0))
            the_end(NULL, 1);
        if (game_over())
            the_end("The game is over.  Press a key to close the window.", 0);
        if (ctrl_stop) {
            quitting = 1;
            the_end("The game was ended in the window that started it.", 0);
        }
        if (listening)
            net_step();
        if (local_console) {
            unsigned char keys[64];
            int ctrl_c = 0, k = con_keys(keys, 64, &ctrl_c), i;

            if (ctrl_c) {
                quitting = 1;
                the_end("The game was ended in the window that started it.", 0);
            }
            for (i = 0; i < k; i++)
                if (term[0].state == T_SIGNON || term[0].state == T_PLAYING)
                    term_key(0, keys[i]);
        }
        if (transcript)
            transcript_step();
        now = net_ms();
        if (now >= next_draw) {
            for (t = 0; t < nterm; t++)
                draw(t);
            next_draw = now + 30;
        }
        if (held || (!opt_fixed_clock && idle && !keys_pending() && !disc_busy()))
            net_wait(1, local_console);
    }
}

int main(int argc, char **argv)
{
    int t;

    options(argc, argv);
    if (getenv("QUEST_TRACE_FROM"))             /* debugging: --trace from instruction N */
        trace_from = strtoull(getenv("QUEST_TRACE_FROM"), NULL, 10);
    if (getenv("QUEST_STOP_AT"))                /* ... show the screens at instruction N */
        stop_at = strtoull(getenv("QUEST_STOP_AT"), NULL, 10);
    netlog = getenv("QUEST_NETLOG") != NULL;    /* ... log the players' comings and goings */
    if (getenv("QUEST_SMC")) {                  /* ... report writes over code that has run */
        smc_fp = fopen(getenv("QUEST_SMC"), "w");
        if (!smc_fp) {
            fprintf(stderr, "%s: QUEST_SMC: cannot open %s\n", prog, getenv("QUEST_SMC"));
            return 2;
        }
        setvbuf(smc_fp, NULL, _IOLBF, 4096);
    }
    if (opt_join) {                             /* HOST, HOST:PORT, [IPv6] or [IPv6]:PORT */
        char host[256], *h = host, *colon;
        int port = opt_port;

        snprintf(host, sizeof host, "%s", opt_join);
        if (*h == '[') {
            colon = strchr(h, ']');
            if (!colon || (colon[1] && colon[1] != ':'))
                bad_option("--join: not HOST, HOST:PORT or [ADDRESS]:PORT:", opt_join);
            *colon++ = 0;
            h++;
            if (*colon == ':')
                port = number(colon + 1, 1, 65535, "the port");
        } else if ((colon = strchr(h, ':')) != NULL && colon == strrchr(h, ':')) {
            *colon = 0;                         /* more than one colon: an IPv6 address */
            port = number(colon + 1, 1, 65535, "the port");
        }
        return net_join(h, port);
    }
    stdin_tty = isatty(fileno(stdin));
    transcript = opt_transcript || !stdin_tty || !isatty(fileno(stdout)) || !con_interactive();
    /* a window started while a game waits for players joins that game (a
     * scripted game for one is always a game of its own) */
    switch (opt_players > 1 || !transcript ? net_listen(opt_port, opt_lan) : -1) {
    case 1:
        if (transcript) {
            fprintf(stderr, "%s: a game is running on port %d; a player's terminal needs a "
                    "console (or --join)\n", prog, opt_port);
            return 1;
        }
        return net_join("127.0.0.1", opt_port);
    case 0:
        listening = 1;
        break;
    default:
        if (opt_players > 1) {
            fprintf(stderr, "%s: cannot listen on port %d for the other players\n", prog,
                    opt_port);
            return 1;
        }
        break;
    }
    if (listening && opt_players == 1) {
        net_shutdown();                         /* nobody can join a game for one */
        listening = 0;
    }
    if (listening && opt_lan && net_host_name(lan_name, sizeof lan_name))
        lan_name[0] = 0;
    nterm = opt_players;
    for (t = 0; t < MAXTERM; t++)
        term[t].conn = -1;
    term[0].local = 1;
    open_pack(argv[0]);
    if (opt_easy)
        easy_library();
    net_on_ctrl(&ctrl_stop);
    if (!transcript) {
        local_console = 1;
        con_open("QUEST UNDER MFE (Philips P7000)", COLS, ROWS + 1);
    }
    screen_hook = on_screen;
    clock_set_realtime(!opt_fixed_clock);
    kbd_gap = 20000;
    cpu_model = 90;
    cpu_reset();
    io_reset();
    console_keys = 037705121;                   /* BOOT R0,X1: disc 0, channel 2 */
    X1 = console_keys;
    io_boot(X1);
    run();
    return 0;
}
