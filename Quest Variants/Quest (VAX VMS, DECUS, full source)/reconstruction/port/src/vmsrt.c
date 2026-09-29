/*
 *  vmsrt.c -- the VMS services QUEST used, reimplemented for Windows.
 *
 *  QUEST talked to its terminal three different ways at once:
 *
 *    * FORMAT and INCHK (FORMAT.MAR, INP.MAR) drove the terminal with
 *      $QIOW, straight past RMS;
 *    * TRIMMER / OUTNUM / VARFORMAT / SINGLE / PROMPT wrote through
 *      FORTRAN unit 6 with a '+' carriage control and a '$' descriptor,
 *      which together mean "print here and leave the cursor alone";
 *    * a handful of statements wrote ordinary carriage-controlled records.
 *
 *  Everything therefore goes through one unbuffered writer here, so the
 *  three kinds of output interleave in the order the program emits them.
 *
 *  VMS carriage control, as the FORTRAN RTL rendered it on a terminal:
 *
 *      ' '   line feed before the record, carriage return after
 *      '0'   two line feeds before, carriage return after
 *      '1'   form feed before,       carriage return after
 *      '+'   nothing before,         carriage return after
 *      '$'   line feed before,       nothing after
 *      NUL   nothing before,         nothing after
 *
 *  A VT100 line feed moves down without returning the carriage, so the
 *  console is put in ENABLE_VIRTUAL_TERMINAL_PROCESSING |
 *  DISABLE_NEWLINE_AUTO_RETURN mode, where LF behaves the same way.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <termios.h>
#  include <sys/select.h>
#endif

#include "vmsrt.h"

/* ------------------------------------------------------------------ */
/*  console                                                            */
/* ------------------------------------------------------------------ */

static int tty_ready = 0;
static int in_is_console = 0;
static int out_is_console = 0;

#ifdef _WIN32
static HANDLE hout, hin;
static DWORD in_mode_saved, out_mode_saved;

static void tty_restore(void)
{
    if (in_is_console)
        SetConsoleMode(hin, in_mode_saved);
    if (out_is_console)
        SetConsoleMode(hout, out_mode_saved);
}
#else
static struct termios tio_saved;
static void tty_restore(void)
{
    if (in_is_console)
        tcsetattr(0, TCSANOW, &tio_saved);
}
#endif

void tty_init(void)
{
    if (tty_ready)
        return;
    tty_ready = 1;
#ifdef _WIN32
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin = GetStdHandle(STD_INPUT_HANDLE);
    /*  input and output are tested separately: either one may be a pipe  */
    if (GetConsoleMode(hout, &out_mode_saved)) {
        DWORD om = out_mode_saved | 0x0004 /* VIRTUAL_TERMINAL_PROCESSING */
                                  | 0x0008 /* DISABLE_NEWLINE_AUTO_RETURN */;
        /* DISABLE_NEWLINE_AUTO_RETURN is only honoured together with VT
           processing; if the pair is refused fall back to VT alone. */
        if (!SetConsoleMode(hout, om))
            SetConsoleMode(hout, out_mode_saved | 0x0004);
        out_is_console = 1;
    }
    if (GetConsoleMode(hin, &in_mode_saved)) {
        /*  INCHK read single characters with no echo and no terminator
            filtering; ENABLE_PROCESSED_INPUT stays on so that Ctrl-C
            still stops the game.  */
        SetConsoleMode(hin, (in_mode_saved & ~(ENABLE_LINE_INPUT |
                                               ENABLE_ECHO_INPUT))
                            | ENABLE_PROCESSED_INPUT);
        in_is_console = 1;
    }
#else
    out_is_console = isatty(1);
    if (isatty(0) && tcgetattr(0, &tio_saved) == 0) {
        struct termios t = tio_saved;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &t);
        in_is_console = 1;
    }
#endif
    atexit(tty_restore);
}

/* Every byte the game prints goes through here, unbuffered. */
void tty_put(const char *s, int n)
{
    if (n <= 0)
        return;
    tty_init();
#ifdef _WIN32
    if (out_is_console) {
        DWORD done;
        WriteFile(hout, s, (DWORD) n, &done, NULL);
        return;
    }
#endif
    fwrite(s, 1, (size_t) n, stdout);
    fflush(stdout);
}

static void tty_puts(const char *s)
{
    tty_put(s, (int) strlen(s));
}

/* ------------------------------------------------------------------ */
/*  input                                                              */
/* ------------------------------------------------------------------ */

/*  Read one byte.  Returns -1 on end of input, -2 if `timeout_ms`
    elapsed first (INCHK gives the terminal 24 seconds).  */
static int tty_getbyte(int timeout_ms)
{
    tty_init();
#ifdef _WIN32
    if (in_is_console) {
        for (;;) {
            INPUT_RECORD ir;
            DWORD got = 0;
            if (WaitForSingleObject(hin, timeout_ms < 0 ? INFINITE
                                                        : (DWORD) timeout_ms) != WAIT_OBJECT_0)
                return -2;
            if (!ReadConsoleInputA(hin, &ir, 1, &got) || got == 0)
                return -1;
            if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown &&
                ir.Event.KeyEvent.uChar.AsciiChar)
                return (unsigned char) ir.Event.KeyEvent.uChar.AsciiChar;
        }
    }
    {   /* redirected input: a pipe never times out */
        int c = getchar();
        return c == EOF ? -1 : c;
    }
#else
    if (timeout_ms >= 0) {
        fd_set r;
        struct timeval tv;
        FD_ZERO(&r);
        FD_SET(0, &r);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        if (select(1, &r, NULL, NULL, &tv) <= 0)
            return -2;
    }
    {
        unsigned char c;
        return read(0, &c, 1) == 1 ? c : -1;
    }
#endif
}

/*
 *  INCHK(NUMBER) -- INP.MAR.  One character, no echo, no terminator
 *  filtering, 24 second timeout; lower case is folded to upper; a
 *  timeout returns 0.  End of input is reported as 0 too, which makes a
 *  script that runs out of keystrokes leave the game the same way an
 *  idle terminal did.
 */
void inchk_(int *n)
{
    int c = tty_getbyte(24000);
    if (c < 0) {
        *n = 0;
        if (c == -1)
            eof_seen();
        return;
    }
    if (c >= 'a' && c <= 'z')
        c -= 32;
    input_seen();
    *n = c;
}

/*  Read a line into a blank padded FORTRAN string.  `echo` drives the
    terminal the way the VMS terminal driver did for a FORTRAN READ.  */
static void read_line(char *buf, size_t len, int echo)
{
    size_t n = 0;
    char *tmp = (char *) malloc(len + 1);

    for (;;) {
        int c = tty_getbyte(-1);
        if (c < 0) {
            if (c == -1)
                eof_seen();
            break;
        }
        input_seen();
        if (c == '\r' || c == '\n')
            break;
        if (c == 8 || c == 127) {               /* DEL and BS both rub out */
            if (n > 0) {
                n--;
                if (echo)
                    tty_puts("\b \b");
            }
            continue;
        }
        if (c == 21) {                          /* ^U -- kill the line */
            while (n > 0) {
                n--;
                if (echo)
                    tty_puts("\b \b");
            }
            continue;
        }
        if (c < 32)
            continue;
        if (n < len) {
            tmp[n++] = (char) c;
            if (echo) {
                char ch = (char) c;
                tty_put(&ch, 1);
            }
        }
    }
    if (echo)
        tty_puts("\r\n");
    memset(buf, ' ', len);
    memcpy(buf, tmp, n);
    free(tmp);
}

/*  GETINPUT_NOECHO(string) -- INP.MAR.  */
void getinput_noecho_(char *buf, size_t len)
{
    read_line(buf, len, 0);
}

/*  The terminal read behind SUBROUTINE ASCII and SUBROUTINE INPUTNUMBER,
    which on VMS were READ(5,...) statements and so echoed.  */
void ttyget_(char *buf, size_t len)
{
    read_line(buf, len, 1);
}

/*  INPUTNUMBER's READ(5,'(I)') I.  *err is set when the field will not
    convert, which is the ERR= branch the source takes.  */
void ttygetnum_(int *value, int *err)
{
    char line[128], *p = line, *end;
    long v;

    read_line(line, sizeof line, 1);
    line[sizeof line - 1] = '\0';
    while (*p == ' ')
        p++;
    if (*p == '\0') {                       /* an all blank field reads as 0 */
        *value = 0;
        *err = 0;
        return;
    }
    v = strtol(p, &end, 10);
    while (*end == ' ')
        end++;
    if (end == p || *end != '\0') {
        *value = 0;
        *err = 1;
        return;
    }
    *value = (int) v;
    *err = 0;
}

/* ------------------------------------------------------------------ */
/*  output primitives                                                  */
/* ------------------------------------------------------------------ */

/*
 *  FORMAT(number_of_line_feeds, string) -- FORMAT.MAR.
 *
 *  The MACRO wrote the first control character as a carriage return and
 *  every one after it as a line feed, then handed the string to $FAO.
 *  Only three FAO directives occur in QUEST: !/ (carriage return, line
 *  feed), !_ (horizontal tab) and !! (a literal exclamation mark).
 */
void format_(int *nlf, const char *str, size_t len)
{
    char out[4096];
    size_t o = 0;
    int i;

    for (i = 0; i < *nlf && o < sizeof out; i++)
        out[o++] = (char) (i == 0 ? '\r' : '\n');

    for (i = 0; (size_t) i < len && o + 2 < sizeof out; i++) {
        if (str[i] == '!' && (size_t) (i + 1) < len) {
            char d = str[i + 1];
            if (d == '/') { out[o++] = '\r'; out[o++] = '\n'; i++; continue; }
            if (d == '_') { out[o++] = '\t';                  i++; continue; }
            if (d == '!') { out[o++] = '!';                   i++; continue; }
        }
        out[o++] = str[i];
    }
    tty_put(out, (int) o);
}

/*
 *  Write one record with VMS FORTRAN carriage control.  `cc` is the
 *  carriage control character the record carried in column 1.
 */
void ttyrec_(const int *cc, const char *buf, size_t len)
{
    char pre[4] = {0, 0, 0, 0};
    size_t n = 0;
    int suffix = 1;

    switch (*cc) {
    case ' ':  pre[n++] = '\n';                  break;
    case '0':  pre[n++] = '\n'; pre[n++] = '\n'; break;
    case '1':  pre[n++] = '\f';                  break;
    case '+':                                    break;
    case '$':  pre[n++] = '\n'; suffix = 0;      break;
    default:   suffix = 0;                       break;   /* NUL: neither */
    }
    tty_put(pre, (int) n);
    tty_put(buf, (int) len);
    if (suffix)
        tty_put("\r", 1);
}

/*  Write a string exactly as given, no carriage control at all: the
    '+'/'$' pair the source used for TRIMMER, OUTNUM, VARFORMAT,
    SINGLE and PROMPT.  */
void ttyraw_(const char *buf, size_t len)
{
    tty_put(buf, (int) len);
}

/*  N empty records with a blank carriage control: line feed before the
    (empty) record, carriage return after.  This is what the bare
    WRITE(6,FMT='()') and WRITE(6,FMT='(//)') statements produce.  */
void ttynl_(const int *n)
{
    int i;
    for (i = 0; i < *n; i++)
        tty_put("\n\r", 2);
}

/*  LIB$ERASE_PAGE(1,1) -- erase from the home position to the end of
    the screen and leave the cursor there.  */
void clrscr_(void)
{
    tty_puts("\033[1;1H\033[0J");
}

/*  LIB$SET_SCROLL(top,bottom) -- DECSTBM, then park the cursor at the
    top of the new region (LIB$SET_SCROLL homes the cursor).  */
void setscroll_(const int *top, const int *bot)
{
    char b[32];
    sprintf(b, "\033[%d;%dr\033[%d;1H", *top, *bot, *top);
    tty_puts(b);
}

/* ------------------------------------------------------------------ */
/*  VAX FORTRAN intrinsics                                             */
/* ------------------------------------------------------------------ */

/*
 *  A frozen clock.  QUEST seeds its generator from SECNDS(0.0), so the
 *  clock is the first thing that makes two runs diverge; QUEST_FREEZE
 *  ("YYYY-MM-DD hh:mm:ss") pins the date and the time of day, which is
 *  what makes a transcript reproducible and comparable against the
 *  original running under a VAX emulator.
 */
long vms_now(void)
{
    static int looked = 0;
    static long frozen = -1;

    if (!looked) {
        const char *e = getenv("QUEST_FREEZE");
        struct tm t;
        looked = 1;
        memset(&t, 0, sizeof t);
        if (e && sscanf(e, "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon,
                        &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {
            t.tm_year -= 1900;
            t.tm_mon -= 1;
            t.tm_isdst = -1;
            frozen = (long) mktime(&t);
        }
    }
    return frozen >= 0 ? frozen : (long) time(NULL);
}

/*
 *  RAN(I) -- the VAX FORTRAN generator, reproduced exactly:
 *
 *      I = 69069 * I + 1   (modulo 2**32)
 *      RAN = I * 2**-32    (I taken as unsigned)
 *
 *  Keeping it bit for bit means a given seed deals the same dungeon,
 *  the same monsters and the same dice as it did on the VAX.
 */
float vaxran_(int *seed)
{
    unsigned int s = (unsigned int) *seed;
    float r;

    s = s * 69069u + 1u;
    *seed = (int) s;
    r = (float) ((double) s / 4294967296.0);

    /*  RAN is documented to return a value in [0,1), but F_floating
        carries only 24 bits of fraction, so the largest seeds round up to
        exactly 1.0 -- and DICE computes INT(RAN(SEED1)*J)+1, which would
        then be J+1.  COLOR(DICE(1,11)) is dimensioned 11.  */
    if (r >= 1.0f)
        r = 0.99999994f;                     /* the float below 1 */
    return r;
}

/*  SECNDS(x) -- seconds since midnight, less x.  */
float vsecnd_(float *x)
{
    time_t now = (time_t) vms_now();
    struct tm *t = localtime(&now);
    const char *e = getenv("QUEST_SEED");

    /*  QUEST_SEED pins what the three images turn into SEED1 by
        SEED1=INT(SECNDS(0.0)); SEED1=SEED1.OR.1  */
    if (e && *e)
        return (float) atoi(e) - *x;
    return (float) (t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec) - *x;
}

/*
 *  LIB$DAY -- days since 17 November 1858, the VMS base date.
 *
 *  QUEST uses it two ways: DAYS in the player record, which is how SORT
 *  decides a character has not been run for 21 days, and MOD(DAY,7) in
 *  QUEST.FOR.  17-NOV-1858 was a Wednesday, so that remainder is 3 on a
 *  Saturday and 4 on a Sunday -- the two days ACCESS.FIL is ignored.
 */
void libday_(int *days)
{
    time_t now = (time_t) vms_now();
    struct tm *t = localtime(&now);
    long y = t->tm_year + 1900, m = t->tm_mon + 1, d = t->tm_mday;
    long a = (14 - m) / 12;
    long yy = y + 4800 - a;
    long mm = m + 12 * a - 3;
    long jdn = d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100
               + yy / 400 - 32045;

    *days = (int) (jdn - 2400001L);          /* 17-NOV-1858 = JDN 2400001 */
}

/*  CALL TIME(buf) -- VAX returns 'hh:mm:ss'; QUEST hands it a
    CHARACTER*5 and so keeps 'hh:mm'.  */
void vmstim_(char *buf, size_t len)
{
    char s[16];
    time_t now = (time_t) vms_now();
    struct tm *t = localtime(&now);
    size_t i;

    sprintf(s, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    for (i = 0; i < len; i++)
        buf[i] = i < strlen(s) ? s[i] : ' ';
}

/*  BAS$SLEEP, behind SUBROUTINE SLEEP (renamed QSLEEP here).  */
void basslp_(const int *secs)
{
    if (*secs <= 0)
        return;
    if (no_delay)
        return;
#ifdef _WIN32
    Sleep((DWORD) (*secs) * 1000u);
#else
    sleep((unsigned) *secs);
#endif
}

/* ------------------------------------------------------------------ */
/*  process identity                                                   */
/* ------------------------------------------------------------------ */

/*
 *  GETNAME(uic,username) -- GETNAME.MAR, which asked $GETJPI for the
 *  UIC and the user name.  USERINFO then renders the UIC as six
 *  characters, group first.
 *
 *  QUEST_USERNAME and QUEST_UIC override both, which is how you reach
 *  the pieces of the game that tested for the author's own account
 *  ('00CKKELLEY', UIC 065244): the DNDOP operator program, and the
 *  checks that let him run and rename other people's characters.
 */
void getname_(short *uic, char *username, size_t ulen)
{
    const char *env = getenv("QUEST_USERNAME");
    char name[64];
    size_t i, n;
    int group = 1, member = 100;

    if (!env || !*env) {
#ifdef _WIN32
        DWORD sz = sizeof name;
        if (!GetUserNameA(name, &sz))
            strcpy(name, "PLAYER");
#else
        const char *u = getenv("USER");
        strncpy(name, u && *u ? u : "PLAYER", sizeof name - 1);
        name[sizeof name - 1] = '\0';
#endif
    } else {
        strncpy(name, env, sizeof name - 1);
        name[sizeof name - 1] = '\0';
    }
    n = strlen(name);
    for (i = 0; i < n; i++)
        if (name[i] >= 'a' && name[i] <= 'z')
            name[i] -= 32;
    for (i = 0; i < ulen; i++)
        username[i] = i < n ? name[i] : ' ';

    env = getenv("QUEST_UIC");
    if (env && strlen(env) == 6) {
        group = (env[0] - '0') * 100 + (env[1] - '0') * 10 + (env[2] - '0');
        member = (env[3] - '0') * 100 + (env[4] - '0') * 10 + (env[5] - '0');
    }
    uic[0] = (short) member;                 /* TEMPUIC1(1) -- member */
    uic[1] = (short) group;                  /* TEMPUIC1(2) -- group  */
}

/* ------------------------------------------------------------------ */
/*  inter-image common, and chaining                                   */
/* ------------------------------------------------------------------ */

/*  LIB$PUT_COMMON / LIB$GET_COMMON -- the 252 byte process common the
    chained images passed the player through.  */
static char common_area[252];

void putcommon_(const char *buf, size_t len)
{
    size_t n = len < sizeof common_area ? len : sizeof common_area;
    memset(common_area, ' ', sizeof common_area);
    memcpy(common_area, buf, n);
}

void getcommon_(char *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        buf[i] = i < sizeof common_area ? common_area[i] : ' ';
}
