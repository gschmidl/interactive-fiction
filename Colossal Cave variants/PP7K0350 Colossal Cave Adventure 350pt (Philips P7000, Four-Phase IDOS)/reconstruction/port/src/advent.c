/* advent.c - "ADVENT UNDER IDOS": the 350-point Colossal Cave Adventure
 * ("ADVENTURE   07 JUNE 1978") as installed on a Philips P7000 - a Four-Phase
 * Systems System IV computer - at a Danish site.
 *
 * The port boots that site's IDOS disc pack on an emulated IV/70 (cpu.c,
 * io.c) and starts the game with "// ADVENT", as the operator did.  The
 * machine's 24 x 81 video screen is mirrored in the console window; with
 * --transcript (or when stdin or stdout is not a console) the screen is
 * followed as a scrolling log instead, and input lines are read from stdin. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include "fp4.h"

static const char *prog = "advent";

#define ROWS 24
#define COLS 81

/* ---- options -------------------------------------------------------------- */

static int opt_unlimited, opt_transcript, opt_fixed_clock, opt_entry_top;
static const char *opt_pack = "advent.pack";
static unsigned long long trace_to;     /* debugging: end the trace here */
static unsigned long long stop_at;      /* debugging: stop the machine here */
static long watch_pc = -1;              /* debugging: report each visit to here */

static void finish(int code);

static void usage(FILE *f)
{
    fprintf(f,
"Usage: %s [OPTION]...\n"
"Colossal Cave Adventure (350 points, 7 June 1978) as it ran under IDOS on a\n"
"Philips P7000 (Four-Phase System IV): the site's own disc pack booted on an\n"
"emulated IV/70.\n"
"\n"
"  -u, --unlimited   clear the wizard's prime-time hours and the wait before\n"
"                    a suspended game may go on; this build enforces neither,\n"
"                    and HOURS and SUSPEND then say so\n"
"      --transcript  follow the screen as a scrolling log and read lines from\n"
"                    stdin (the default when stdin or stdout is not a console)\n"
"      --fixed-clock let the 60 Hz clock count instructions instead of real\n"
"                    time, so that a run can be repeated exactly\n"
"      --entry-line-top  show what you type on the top line of the screen,\n"
"                    where the P7000 showed it (by default the console shows\n"
"                    that line at the bottom, under the game's text)\n"
"      --pack=FILE   the working copy of the disc pack (default advent.pack;\n"
"                    made from p7000.pack next to the program when missing).\n"
"                    Suspended games are kept on it.\n"
"      --trace=FILE  write an instruction trace to FILE (debugging)\n"
"  -h, --help        show this help and exit\n"
"\n"
"In the game, type commands and press Enter.  Backspace deletes a character.\n"
"Ctrl+C leaves at once; QUIT or SUSPEND end the game as the original did.\n",
            prog);
}

static void bad_option(const char *msg, const char *arg)
{
    fprintf(stderr, "%s: %s '%s'\nTry '%s --help' for more information.\n", prog, msg,
            arg, prog);
    exit(2);
}

static void options(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "-u") || !strcmp(a, "--unlimited"))
            opt_unlimited = 1;
        else if (!strcmp(a, "--transcript"))
            opt_transcript = 1;
        else if (!strcmp(a, "--fixed-clock"))
            opt_fixed_clock = 1;
        else if (!strcmp(a, "--entry-line-top"))
            opt_entry_top = 1;
        else if (!strncmp(a, "--pack=", 7)) {
            if (!a[7])
                bad_option("missing file name in", a);
            opt_pack = a + 7;
        } else if (!strncmp(a, "--trace=", 8)) {
            trace_fp = fopen(a + 8, "w");
            if (!trace_fp)
                bad_option("cannot write", a + 8);
        } else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            exit(0);
        } else if (!strcmp(a, "--pack") || !strcmp(a, "--trace"))
            bad_option("option requires an argument (use --NAME=VALUE):", a);
        else if (a[0] == '-' && a[1] == '-')
            bad_option("unrecognized option", a);
        else if (a[0] == '-')
            bad_option("invalid option", a);
        else
            bad_option("unexpected argument", a);
    }
}

/* ---- the disc pack ---------------------------------------------------------- */

/* the working copy: made from p7000.pack next to the program when missing */
static void open_pack(const char *argv0)
{
    FILE *f = fopen(opt_pack, "rb");
    char src[4096];

    if (f)
        fclose(f);
    else {
        char *p;
        FILE *in, *out;
        size_t n;
        char buf[65536];

#ifdef _WIN32
        if (!GetModuleFileNameA(NULL, src, sizeof src))
            snprintf(src, sizeof src, "%s", argv0);
#else
        snprintf(src, sizeof src, "%s", argv0);
#endif
        p = strrchr(src, '\\');
        if (!p || strrchr(src, '/') > p)
            p = strrchr(src, '/');
        if (p)
            p[1] = 0;
        else
            src[0] = 0;
        strncat(src, "p7000.pack", sizeof src - strlen(src) - 1);
        in = fopen(src, "rb");
        if (!in) {
            fprintf(stderr, "%s: cannot read the disc pack %s: %s\n", prog, src, strerror(errno));
            exit(1);
        }
        out = fopen(opt_pack, "wb");
        if (!out) {
            fprintf(stderr, "%s: cannot make %s: %s\n", prog, opt_pack, strerror(errno));
            exit(1);
        }
        while ((n = fread(buf, 1, sizeof buf, in)) > 0)
            fwrite(buf, 1, n, out);
        fclose(in);
        if (fclose(out)) {
            fprintf(stderr, "%s: cannot write %s\n", prog, opt_pack);
            exit(1);
        }
    }
    if (disc_open(opt_pack, 1) < 0) {
        fprintf(stderr, "%s: cannot open %s: %s\n", prog, opt_pack, strerror(errno));
        exit(1);
    }
}

/* ---- the screen ----------------------------------------------------------------- */

/* the character at row R, column C of the IDOS screen (7 bits; 032 is the
 * block cursor) */
static int screen_char(int r, int c)
{
    word v = mem[(screen_base + (word)(r * 32 + c / 3)) & A15];

    return (int)(v >> (16 - 8 * (c % 3))) & 0177;
}

/* row R as text, trailing blanks dropped */
static void screen_row(int r, char *out)
{
    int c, last = 0;

    for (c = 0; c < COLS; c++) {
        int ch = screen_char(r, c);

        if (ch < 040 || ch == 0177)
            ch = ' ';
        out[c] = (char)ch;
        if (ch != ' ')
            last = c + 1;
    }
    out[last] = 0;
}

/* ---- the transcript: the screen followed as a scrolling log ------------------- */

static int transcript;                  /* follow the screen, read stdin */
static int tr_on;                       /* the game has started */
static long nscroll;                    /* scrolls so far: row R = log line nscroll + R */
static long printed;                    /* log lines before this are out */
static int stdin_tty;

static void emit(const char *s)
{
    fputs(s, stdout);
    fputc('\n', stdout);
}

/* a scroll is about to push row 0 off the screen */
static void on_scroll(void)
{
    if (tr_on && nscroll >= printed) {
        char b[COLS + 1];

        screen_row(0, b);
        emit(b);
        printed = nscroll + 1;
    }
    nscroll++;
}

/* everything on the screen now is old */
static void transcript_mark(void)
{
    printed = nscroll + ROWS;
    tr_on = 1;
}

static int last_row(void)
{
    char b[COLS + 1];
    int r;

    for (r = ROWS - 1; r > 0; r--) {
        screen_row(r, b);
        if (b[0])
            break;
    }
    return r;
}

/* print the lines not yet out; at an input point the last row is the prompt
 * and is left without a newline */
static void transcript_flush(int final)
{
    char b[COLS + 1];
    int last = last_row(), r;

    for (r = (int)(printed - nscroll); r < last; r++)
        if (r >= 0) {
            screen_row(r, b);
            emit(b);
        }
    if (last >= printed - nscroll) {
        screen_row(last, b);
        if (final)
            emit(b);
        else
            fputs(b, stdout);
    }
    printed = nscroll + last + 1;
    fflush(stdout);
}

/* ---- -u: the wizard's hours ------------------------------------------------------ */

/* The game keeps the wizard's settings in COMMON: WKDAY 05412, WKEND 05413
 * and HOLID 05414 (prime-time hour masks), LATNCY 05421 (the minutes to wait
 * before a suspended game may go on).  This build never enforces them - its
 * START does no checks and its DATIME always answers day 273, 07:51 - but
 * HOURS and SUSPEND print them.  POOF sets them (from 053140) at every start,
 * and a resumed game may bring back other values, so -u clears the live
 * words whenever the game is in memory: HOURS then shows the cave open all
 * day and SUSPEND asks for no wait, which is what the game does anyway. */
static void unlimited(void)
{
    if (mem[053140] == 003053166u && mem[053141] == 043005412u && mem[053162] == 043005421u) {
        mem[05412] = 0;
        mem[05413] = 0;
        mem[05414] = 0;
        mem[05421] = 0;
    }
}

/* ---- a new game or the suspended one ------------------------------------------- */

/* The game keeps its whole state in the file ADSAVE: it is loaded at every
 * start, and SUSPEND writes it back.  Word 0 says whether it holds a game to
 * play (1: a fresh one or a suspended one); a game that ends clears it, and
 * a start from a cleared ADSAVE tries to build the database afresh, which
 * this installation cannot do ("INITIALIZING...", then error 5).  The site
 * ran two job files: OLD ("// ADVENT") to go on, and NEW, which first did
 * "// COPY /I=ADNEW,O=ADSAVE." to start from the fresh state ADNEW.  The
 * port does NEW's copy itself, in place: IDOS's COPY writes a new file
 * after the last one each time, and the pack is full after four new games
 * (the site will have compacted it now and then). */

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

static void new_or_old(void)
{
    long nsave, nnew, save = pack_file("ADSAVE", &nsave), fresh = pack_file("ADNEW", &nnew);

    if (save < 0 || fresh < 0 || nsave != nnew) {
        fprintf(stderr, "%s: %s does not hold the game's files ADSAVE and ADNEW\n", prog,
                opt_pack);
        finish(1);
    }
    if (disc_peek(save * 256) == 0)             /* the last game is over: NEW */
        disc_copy(fresh, save, nsave);
}

/* ---- the operator: start the game ---------------------------------------------- */

enum { S_BOOT, S_START, S_GAME };
static int state = S_BOOT;

static int row_has(int r, const char *s)
{
    char b[COLS + 1];

    screen_row(r, b);
    return strstr(b, s) != NULL;
}

/* debugging: ADVENT_CODES=FILE counts, at every step once the game runs, the
 * screen bytes that are not plain ASCII (the high bit, control codes other
 * than the cursor; the entry line's last column holds a 012) and appends the
 * counts, where they were and when to FILE at the end.  In play there are
 * none: only while IDOS loads the game (about 3.2M-4.7M instructions) do the
 * top five rows hold the saved memory image passing through. */
static FILE *codes_fp;
static unsigned long code_count[256], code_where[ROWS][COLS];
static unsigned long long code_first, code_last;

static void count_codes(void)
{
    int r, c, n = 0;

    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++) {
            word v = mem[(screen_base + (word)(r * 32 + c / 3)) & A15];
            int b = (int)(v >> (16 - 8 * (c % 3))) & 0377;

            if ((b >= 0177 || (b < 040 && b != 0 && b != 032)) && !(r == 0 && c == COLS - 1)) {
                code_count[b]++;
                code_where[r][c]++;
                n++;
            }
        }
    if (n) {
        if (!code_first) {
            code_first = icount;
            fprintf(codes_fp, "first at %llu, pc %05o\n", icount, last_pc);
        }
        code_last = icount;
    }
}

static void operator_step(void)
{
    if (codes_fp && state == S_GAME)
        count_codes();
    switch (state) {
    case S_BOOT:                                /* $BATCH is up */
        if (row_has(1, "// $BATCH") && kbd_waiting() && !kbd_pending(0)) {
            static const char job[] = "// ADVENT\215//\215";

            kbd_type(0, job, (int)sizeof job - 1);
            if (transcript)
                transcript_mark();
            state = S_START;
        }
        break;
    case S_START:
        if (!kbd_pending(0))
            state = S_GAME;
        break;
    }
    if (opt_unlimited)
        unlimited();
    if (trace_to && icount >= trace_to && trace_fp) {
        fclose(trace_fp);
        trace_fp = NULL;
    }
    if (stop_at && icount >= stop_at)
        stop(STOP_LIMIT, "stopped at instruction %llu (ADVENT_STOP_AT)", icount);
}

/* ---- keys --------------------------------------------------------------------- */

/* a host key as a 7200 keyboard code: letters in capitals (the program
 * prints in capitals), Enter the NEW LINE key, Backspace the DEL key */
static int key_code(int ch)
{
    if (ch == '\r' || ch == '\n')
        return 0215;
    if (ch == '\b' || ch == 0177)
        return 0201;
    if (ch >= 040 && ch < 0177)
        return toupper(ch);
    return -1;
}

static void type_line(const char *s)
{
    char b[512];
    int n = 0;

    for (; *s && n < (int)sizeof b - 1; s++) {
        int k = key_code((unsigned char)*s);

        if (k >= 040 && k < 0177)
            b[n++] = (char)k;
    }
    b[n++] = (char)0215;
    kbd_type(0, b, n);
}

/* ---- the console ------------------------------------------------------------------ */

#ifdef _WIN32
static HANDLE hout, hin;
static int have_console;
static DWORD saved_in_mode;
static int saved_in_mode_ok;
static CONSOLE_SCREEN_BUFFER_INFO saved_sbi;
static CONSOLE_CURSOR_INFO saved_ci;
static CHAR_INFO shadow[(ROWS + 1) * COLS];
static int shadow_valid, cur_r = -1, cur_c = -1;
static const char *status = "ADVENT UNDER IDOS - Philips P7000 (Four-Phase IV/70), emulated.  Ctrl+C quits.";

static void con_init(void)
{
    COORD size;
    SMALL_RECT win;
    CONSOLE_CURSOR_INFO ci;

    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin = GetStdHandle(STD_INPUT_HANDLE);
    have_console = GetConsoleScreenBufferInfo(hout, &saved_sbi) != 0;
    if (have_console) {
        GetConsoleCursorInfo(hout, &saved_ci);
        SetConsoleTitleA("ADVENT UNDER IDOS (Philips P7000)");
        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        size.X = COLS; size.Y = ROWS + 1;
        SetConsoleScreenBufferSize(hout, size);
        win.Right = COLS - 1; win.Bottom = ROWS;
        SetConsoleWindowInfo(hout, TRUE, &win);
        ci.dwSize = 100;
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(hout, &ci);
    }
    saved_in_mode_ok = GetConsoleMode(hin, &saved_in_mode) != 0;
    if (saved_in_mode_ok)
        SetConsoleMode(hin, saved_in_mode &
                       ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
}

/* IDOS and ADVENT's run-time library take keys in the top line of the
 * screen, the entry line: ADVENT's input call passes 0140 (the word after
 * its BAL 63601 at 061602), and the library blanks that line, puts the
 * cursor in it and echoes each key there; only when NEW LINE ends the line
 * does the game print it after its "==>".  The whole screen scrolls up, and
 * the next entry blanks whatever reached the top line.  So the console
 * shows lines 1-23 in order and, below them, the entry line while the
 * program has its cursor in it: what the player types appears under the
 * game's text.  --entry-line-top shows the machine's own layout. */
static int entry_active;                /* the cursor has been in line 0 since the last scroll */

static void con_scroll(void)
{
    entry_active = 0;
}

/* the screen line shown at console row R, or -1 for a blank row */
static int shown_line(int r)
{
    if (opt_entry_top)
        return r;
    if (r < ROWS - 1)
        return r + 1;
    return entry_active ? 0 : -1;
}

static void con_refresh(void)
{
    CHAR_INFO buf[(ROWS + 1) * COLS];
    int r, c, changed = 0, cr = cur_r, cc = cur_c;

    if (!have_console)
        return;
    for (c = 0; c < COLS; c++)
        if (screen_char(0, c) == 032)
            entry_active = 1;
    for (r = 0; r <= ROWS; r++)
        for (c = 0; c < COLS; c++) {
            int i = r * COLS + c, ch, line = r < ROWS ? shown_line(r) : -1;
            WORD attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;

            if (r == ROWS) {                    /* the port's own status line */
                ch = c < (int)strlen(status) ? status[c] : ' ';
                attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            } else if (line < 0)
                ch = ' ';
            else {
                ch = screen_char(line, c);
                if (ch == 032) {                /* the block cursor */
                    cr = r;
                    cc = c;
                    ch = ' ';
                } else if (ch < 040 || ch == 0177)
                    ch = ' ';
            }
            buf[i].Char.AsciiChar = (CHAR)ch;
            buf[i].Attributes = attr;
            if (!shadow_valid || shadow[i].Char.AsciiChar != buf[i].Char.AsciiChar ||
                shadow[i].Attributes != buf[i].Attributes)
                changed = 1;
        }
    if (changed) {
        COORD size, org;
        SMALL_RECT dst;

        memcpy(shadow, buf, sizeof buf);
        shadow_valid = 1;
        size.X = COLS; size.Y = ROWS + 1;
        org.X = 0; org.Y = 0;
        dst.Left = 0; dst.Top = 0; dst.Right = COLS - 1; dst.Bottom = ROWS;
        WriteConsoleOutputA(hout, buf, size, org, &dst);
    }
    if (cr >= 0 && (cr != cur_r || cc != cur_c)) {
        COORD p;

        p.X = (SHORT)cc;
        p.Y = (SHORT)cr;
        SetConsoleCursorPosition(hout, p);
        cur_r = cr;
        cur_c = cc;
    }
}

/* returns 1 if the player asked to leave */
static int con_poll(void)
{
    INPUT_RECORD rec[32];
    DWORD avail = 0, got = 0, i;

    if (!GetNumberOfConsoleInputEvents(hin, &avail) || !avail)
        return 0;
    if (avail > 32)
        avail = 32;
    if (!ReadConsoleInputA(hin, rec, avail, &got))
        return 0;
    for (i = 0; i < got; i++) {
        KEY_EVENT_RECORD *k;
        int ch, code;

        if (rec[i].EventType != KEY_EVENT)
            continue;
        k = &rec[i].Event.KeyEvent;
        if (!k->bKeyDown)
            continue;
        ch = (unsigned char)k->uChar.AsciiChar;
        if (ch == 3)
            return 1;                           /* Ctrl+C */
        if (!ch || ch >= 0200 || state != S_GAME)
            continue;
        code = key_code(ch);
        if (code >= 0) {
            char b = (char)code;

            kbd_type(0, &b, 1);
        }
    }
    return 0;
}

static void con_wait_key(void)
{
    INPUT_RECORD rec;
    DWORD got;

    FlushConsoleInputBuffer(hin);
    for (;;) {
        if (!ReadConsoleInputA(hin, &rec, 1, &got) || !got)
            return;
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
            return;
    }
}

static void con_done(void)
{
    if (saved_in_mode_ok)
        SetConsoleMode(hin, saved_in_mode);
    if (have_console) {
        SMALL_RECT win;
        COORD p;

        SetConsoleCursorInfo(hout, &saved_ci);
        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        SetConsoleScreenBufferSize(hout, saved_sbi.dwSize);
        SetConsoleWindowInfo(hout, TRUE, &saved_sbi.srWindow);
        p.X = 0;
        p.Y = ROWS;
        SetConsoleCursorPosition(hout, p);
    }
    printf("\n");
}
#endif

/* ---- the run ----------------------------------------------------------------------- */

static void finish(int code)
{
    const char *dump = getenv("ADVENT_DUMP");   /* debugging: memory at the end */

    if (dump) {
        FILE *f = fopen(dump, "wb");
        word a;

        for (a = 0; f && a < MEMWORDS; a++) {
            fputc((int)(mem[a] >> 16) & 0xFF, f);
            fputc((int)(mem[a] >> 8) & 0xFF, f);
            fputc((int)mem[a] & 0xFF, f);
        }
        if (f)
            fclose(f);
    }
    if (codes_fp) {
        int b;

        fprintf(codes_fp, "last at %llu, end at %llu\n", code_last, icount);
        for (b = 0; b < 256; b++)
            if (code_count[b])
                fprintf(codes_fp, "%03o %lu\n", b, code_count[b]);
        for (b = 0; b < ROWS * COLS; b++)
            if (code_where[b / COLS][b % COLS])
                fprintf(codes_fp, "at %d,%d %lu\n", b / COLS, b % COLS,
                        code_where[b / COLS][b % COLS]);
        fclose(codes_fp);
    }
    disc_close();
    if (trace_fp)
        fclose(trace_fp);
    exit(code);
}

static void fault(void)
{
    fprintf(stderr, "%s: the machine stopped: %s\n", prog, stop_text);
    if (trace_fp)
        trace_dump_ring(trace_fp, 200);
    finish(1);
}

/* How the game ends.  QUIT, and death without reincarnation, end with the
 * FORTRAN STOP: the score, then a halt (HLT at 043244).  SUSPEND, and a
 * STOP continued with RUN, go to the run-time library's EXIT: its routine at
 * 062612 reads IDOS's bootstrap from sector 0 into location 1 and enters it
 * with a pointer to the next program's name in X1 - but this bootstrap
 * wants the BOOT instruction there (as the console keys give it on a cold
 * start), so the machine ends up sounding the keyboard alarm in a loop at
 * 00062, and the operator booted again.  The port ends the run as the game
 * enters that routine, before the bootstrap overwrites the top of the
 * screen. */
#define EXIT_PC   062612u
#define EXIT_WORD 046062620u                    /* ST2 62620 */

static int game_over;                           /* the game has ended */

/* run N instructions (fewer if the machine stops or the game ends) */
static void run_some(int n)
{
    int i;

    for (i = 0; i < n && !stop_code; i++) {
        cpu_step();
        if (state == S_GAME && (stop_code == STOP_HALT ||
                                (last_pc == EXIT_PC && mem[EXIT_PC] == EXIT_WORD))) {
            game_over = 1;
            break;
        }
        if (last_pc == (word)watch_pc)
            fprintf(stderr, "watch %05lo at %llu: RA %08o RB %08o X1 %08o X2 %08o X3 %08o\n",
                    watch_pc, icount, RA, RB, X1, X2, X3);
    }
}

static void run_transcript(void)
{
    char line[512];

    scroll_hook = on_scroll;
    for (;;) {
        run_some(4096);
        if (game_over) {
            transcript_flush(1);
            finish(0);
        }
        if (stop_code)
            fault();
        operator_step();
        if (state == S_GAME && kbd_waiting() && !kbd_pending(0)) {
            size_t n;

            transcript_flush(0);
            if (!fgets(line, sizeof line, stdin)) {
                fputc('\n', stdout);
                finish(0);
            }
            n = strcspn(line, "\r\n");
            line[n] = 0;
            if (!stdin_tty)
                emit(line);
            type_line(line);
        }
    }
}

#ifdef _WIN32
static void run_console(void)
{
    scroll_hook = con_scroll;
    con_init();
    for (;;) {
        run_some(4096);
        if (game_over) {
            status = "The game is over.  Press a key to close the window.";
            con_refresh();
            con_wait_key();
            con_done();
            finish(0);
        }
        if (stop_code) {
            con_done();
            fault();
        }
        operator_step();
        con_refresh();
        if (con_poll()) {
            con_done();
            finish(0);
        }
        if (cpu_idle() && !kbd_pending(0) && !disc_busy())
            Sleep(1);
    }
}
#endif

int main(int argc, char **argv)
{
    options(argc, argv);
    if (getenv("ADVENT_TRACE_FROM"))            /* debugging: --trace from instruction N */
        trace_from = strtoull(getenv("ADVENT_TRACE_FROM"), NULL, 10);
    if (getenv("ADVENT_TRACE_TO"))              /* ... up to about instruction N */
        trace_to = strtoull(getenv("ADVENT_TRACE_TO"), NULL, 10);
    if (getenv("ADVENT_STOP_AT"))               /* stop the machine at about N */
        stop_at = strtoull(getenv("ADVENT_STOP_AT"), NULL, 10);
    if (getenv("ADVENT_WATCH"))                 /* report the registers at PC (octal) */
        watch_pc = strtol(getenv("ADVENT_WATCH"), NULL, 8);
    if (getenv("ADVENT_CODES"))                 /* count the screen bytes not plain ASCII */
        codes_fp = fopen(getenv("ADVENT_CODES"), "a");
    stdin_tty = isatty(fileno(stdin));
    transcript = opt_transcript || !stdin_tty || !isatty(fileno(stdout));
#ifndef _WIN32
    transcript = 1;
#endif
    open_pack(argv[0]);
    new_or_old();
    clock_set_realtime(!opt_fixed_clock);
    cpu_reset();
    io_reset();
    console_keys = 037705121;                   /* BOOT R0,X1: disc 0, channel 2 */
    X1 = console_keys;
    io_boot(X1);
#ifdef _WIN32
    if (!transcript)
        run_console();
#endif
    run_transcript();
    return 0;
}
