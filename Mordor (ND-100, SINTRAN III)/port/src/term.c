/*
 * term.c - the SINTRAN terminal on a Windows console or a pipe.
 *
 * The programs print 7-bit ASCII in a national variant: Norwegian (NS 4551,
 * [ \ ] { | } are Æ Ø Å æ ø å) or Swedish (SEN 850200, [ \ ] { | } are
 * Ä Ö Å ä ö å, @ ` ^ ~ are É é Ü ü).  On a console these are shown as the
 * letters and the letters typed are sent back as the 7-bit codes.  A pipe
 * gets UTF-8 the same way unless raw mode is on, which passes the ND bytes
 * untouched (that is what the reference transcripts in tests\ are compared
 * against).
 *
 * A program written for the club's Facit screens (term_set_facit) moves the
 * cursor and sets video attributes with Facit escape codes; they are turned
 * into VT sequences, and the arrow and Home keys are sent the Facit way.
 *
 * Input is one keystroke at a time with no host echo: the program's own
 * runtime echoes and edits.
 */
#include <stdio.h>
#include <string.h>
#include "term.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static int raw_mode, charset, facit;
static unsigned char obuf[4096];
static int olen;

#ifdef _WIN32
static HANDLE hin, hout;
static DWORD old_in_mode, old_out_mode;
static int in_console, out_console;
static UINT old_cp;
#endif

static const char nd_norwegian[] = "[\\]{|}";
static const wchar_t uni_norwegian[] = L"ÆØÅæøå";
static const char nd_swedish[] = "[\\]{|}@`^~";
static const wchar_t uni_swedish[] = L"ÄÖÅäöåÉéÜü";

static const char *nd_national(void)
{
    return charset == CS_SWEDISH ? nd_swedish : nd_norwegian;
}

static const wchar_t *uni_national(void)
{
    return charset == CS_SWEDISH ? uni_swedish : uni_norwegian;
}

int term_is_console(void)
{
#ifdef _WIN32
    return in_console;
#else
    return 0;
#endif
}

void term_set_facit(int on)
{
    facit = on;
}

static int escape_on = 1;

void term_set_escape(int on)
{
    escape_on = on;
}

void term_init(int raw, int cs)
{
    raw_mode = raw;
    charset = cs;
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    hin = GetStdHandle(STD_INPUT_HANDLE);
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    in_console = GetConsoleMode(hin, &old_in_mode) != 0;
    out_console = GetConsoleMode(hout, &old_out_mode) != 0;
    if (in_console)
        SetConsoleMode(hin, ENABLE_EXTENDED_FLAGS | (old_in_mode & ENABLE_QUICK_EDIT_MODE));
    if (out_console && facit && !raw_mode)
        SetConsoleMode(hout, old_out_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    old_cp = GetConsoleOutputCP();
#endif
}

/* Started from Explorer rather than a shell, the process owns its console
   alone and the window closes the moment it ends -- taking the final score
   with it.  Hold it open in that case only. */
void term_hold(void)
{
#ifdef _WIN32
    DWORD pids[4];
    INPUT_RECORD r;
    DWORD n;
    if (!in_console || GetConsoleProcessList(pids, 4) != 1)
        return;
    term_puts("\r\n[press any key]");
    term_flush();
    while (ReadConsoleInputW(hin, &r, 1, &n) && n)
        if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown)
            break;
#endif
}

void term_restore(void)
{
    if (facit && !raw_mode)
        term_puts("\033[0m\033(B");
    term_flush();
#ifdef _WIN32
    if (in_console)
        SetConsoleMode(hin, old_in_mode);
    if (out_console)
        SetConsoleMode(hout, old_out_mode);
#endif
}

/* ---- output ---- */

static void emit_host(const unsigned char *s, int n)
{
#ifdef _WIN32
    if (out_console) {
        wchar_t w[4096];
        int wn = MultiByteToWideChar(CP_UTF8, 0, (const char *)s, n, w, 4096);
        DWORD done;
        WriteConsoleW(hout, w, (DWORD)wn, &done, NULL);
        return;
    }
#endif
    fwrite(s, 1, (size_t)n, stdout);
}

void term_flush(void)
{
    if (olen) {
        emit_host(obuf, olen);
        olen = 0;
    }
    fflush(stdout);
}

static void put_byte(unsigned char b)
{
    if (olen >= (int)sizeof obuf - 4)
        term_flush();
    obuf[olen++] = b;
}

static void put_str(const char *s)
{
    while (*s)
        put_byte((unsigned char)*s++);
}

/* a Facit escape sequence, complete once args[] holds what it needs */
static void facit_escape(int cmd, const int *args)
{
    char buf[32];
    switch (cmd) {
    case 'Y':                                   /* ESC Y row col, both offset by 32 */
        snprintf(buf, sizeof buf, "\033[%d;%dH", args[0] - 31, args[1] - 31);
        put_str(buf);
        break;
    case 'S':                                   /* set video attributes; '@' = none */
        put_str("\033[0m");
        if (args[0] & 0x01) put_str("\033[7m");
        if (args[0] & 0x02) put_str("\033[5m");
        if (args[0] & 0x04) put_str("\033[4m");
        if (args[0] & 0x08) put_str("\033[2m");
        break;
    case '\'': put_str("\033[7m"); break;       /* reverse video field */
    case '(':  put_str("\033[27m"); break;
    case ')':  put_str("\033[5m"); break;       /* blinking field */
    case '*':  put_str("\033[25m"); break;
    case '%':  put_str("\033[4m"); break;       /* underline field */
    case '&':  put_str("\033[24m"); break;
    case ',':  put_str("\033[2m"); break;       /* reduced intensity */
    case '+':  put_str("\033[22m"); break;
    case 'A':  put_str("\033[A"); break;
    case 'B':  put_str("\033[B"); break;
    case 'C':  put_str("\033[C"); break;
    case 'D':  put_str("\033[D"); break;
    case 'H':  put_str("\033[H"); break;
    case 'I':  put_str("\033M"); break;         /* reverse line feed */
    case 'J':  put_str("\033[J"); break;        /* erase to end of screen */
    case 'K':  put_str("\033[K"); break;        /* erase to end of line */
    case 'L': case 'M': case '`':               /* home and clear */
        put_str("\033[H\033[2J");
        break;
    case 'F':  put_str("\033(0"); break;        /* graphic mode: the same line-drawing letters */
    case 'G':  put_str("\033(B"); break;
    case 'T':  put_str("\033[L"); break;        /* insert line */
    case 'U':  put_str("\033[M"); break;        /* delete line */
    default:   break;                           /* keyclick, wrap, printer, ...: nothing to show */
    }
}

static int esc_state;         /* 0, or the Facit command letter being collected */
static int esc_args[2], esc_nargs;

static int facit_argc(int cmd)
{
    switch (cmd) {
    case 'Y': return 2;
    case 'S': case '.': case '/': return 1;
    default: return 0;
    }
}

static void put_text(int ch)
{
    const char *p;
    if (ch < 32) {
        /* keep what a terminal acts on; the start-up EM (031) and the
           kill-line echo (004) show nothing on the terminal either */
        if (ch == '\r' || ch == '\n' || ch == '\a' || ch == '\b' || ch == '\t')
            put_byte((unsigned char)ch);
        else if (ch == 014 && facit)
            put_str("\033[H\033[2J");
        return;
    }
    if (ch == 0x7F)
        return;
    if (charset != CS_ASCII && (p = strchr(nd_national(), ch)) != NULL) {
        wchar_t u = uni_national()[p - nd_national()];
        put_byte((unsigned char)(0xC0 | (u >> 6)));
        put_byte((unsigned char)(0x80 | (u & 0x3F)));
        return;
    }
    put_byte((unsigned char)ch);
}

void term_putc(int ch)
{
    ch &= 0x7F;
    if (raw_mode) {
        put_byte((unsigned char)ch);
        return;
    }
    if (!facit) {
        put_text(ch);
        return;
    }
    if (esc_state == 033) {                     /* the command letter */
        if (facit_argc(ch)) {
            esc_state = ch;
            esc_nargs = 0;
        } else {
            esc_state = 0;
            facit_escape(ch, esc_args);
        }
        return;
    }
    if (esc_state) {                            /* an argument */
        esc_args[esc_nargs++] = ch;
        if (esc_nargs == facit_argc(esc_state)) {
            facit_escape(esc_state, esc_args);
            esc_state = 0;
        }
        return;
    }
    if (ch == 033) {
        esc_state = 033;
        return;
    }
    put_text(ch);
}

void term_puts(const char *s)
{
    while (*s)
        put_byte((unsigned char)*s++);
}

/* ---- input ---- */

static int map_unicode(unsigned u)
{
    const wchar_t *p;
    if (u < 0x80)
        return (int)u;
    if (charset != CS_ASCII || !raw_mode) {
        for (p = uni_national(); *p; p++)
            if (*p == u)
                return nd_national()[p - uni_national()];
        if (charset == CS_NORWEGIAN) {
            /* Ä and Ö as typed on Swedish/German keyboards stand in for Æ and Ø */
            if (u == 0xC4) return '[';
            if (u == 0xE4) return '{';
            if (u == 0xD6) return '\\';
            if (u == 0xF6) return '|';
        }
    }
    return -2;   /* not on an ND terminal: ignore */
}

#ifdef _WIN32
static int queue[8], qhead, qlen;
#define FKEY 0x100              /* in the queue: part of an arrow or Home key */

static void enqueue(int c)
{
    if (qlen < (int)(sizeof queue / sizeof queue[0]))
        queue[(qhead + qlen++) % 8] = c;
}

/* The arrow and Home keys send ESC and a letter.  The game reads them on its
   hero screen, with Esc disabled; anywhere else the ESC would break the
   program at once -- a key pressed once too often on that screen, or held
   down, broke the game just after it.  So they are dropped while Esc is
   enabled, whenever they were struck. */
static int console_key(void)
{
    INPUT_RECORD r;
    DWORD n;
    for (;;) {
        if (qlen > 0) {
            int c = queue[qhead];
            qhead = (qhead + 1) % 8;
            qlen--;
            if (c & FKEY) {
                if (escape_on)
                    continue;
                c &= ~FKEY;
            }
            return c;
        }
        if (!ReadConsoleInputW(hin, &r, 1, &n) || n == 0)
            return -1;
        if (r.EventType != KEY_EVENT || !r.Event.KeyEvent.bKeyDown)
            continue;
        {
            KEY_EVENT_RECORD *k = &r.Event.KeyEvent;
            unsigned u = k->uChar.UnicodeChar;
            int c, times = k->wRepeatCount > 0 ? k->wRepeatCount : 1, fkey = 0;
            if (facit) {
                switch (k->wVirtualKeyCode) {
                case VK_UP:    fkey = 'A'; break;
                case VK_DOWN:  fkey = 'B'; break;
                case VK_RIGHT: fkey = 'C'; break;
                case VK_LEFT:  fkey = 'D'; break;
                case VK_HOME:  fkey = 'H'; break;
                default: break;
                }
            }
            if (fkey) {
                while (times-- > 0 && qlen < 7 && !escape_on) {
                    enqueue(FKEY | 033);
                    enqueue(FKEY | fkey);
                }
                continue;
            }
            if (k->wVirtualKeyCode == VK_BACK)
                c = facit ? 0x7F : 1;          /* Facit sends DEL; the ND delete key was Ctrl-A */
            else if (k->wVirtualKeyCode == VK_RETURN)
                c = '\r';
            else if (u == 0)
                continue;
            else if ((c = map_unicode(u)) < 0)
                continue;
            if (c == '\n')
                c = '\r';
            while (times-- > 0)
                enqueue(c);
        }
    }
}
#endif

static int pipe_prev;

static int pipe_key(void)
{
    for (;;) {
        int b = getchar();
        if (b == EOF)
            return -1;
        if (b == '\n') {
            if (pipe_prev == '\r') { pipe_prev = b; continue; }
            pipe_prev = b;
            return '\r';
        }
        pipe_prev = b;
        if (b < 0x80)
            return b;
        if (raw_mode)
            continue;
        /* UTF-8 */
        if ((b & 0xE0) == 0xC0) {
            int b2 = getchar();
            int c;
            if (b2 == EOF) return -1;
            c = map_unicode((unsigned)(((b & 0x1F) << 6) | (b2 & 0x3F)));
            if (c >= 0) return c;
            continue;
        }
        /* other 8-bit bytes cannot come from an ND terminal: drop them */
    }
}

int term_pending(void)
{
#ifdef _WIN32
    INPUT_RECORD r[64];
    DWORD n, i;
    int keys = qlen;
    if (in_console && PeekConsoleInputW(hin, r, 64, &n)) {
        for (i = 0; i < n; i++)
            if (r[i].EventType == KEY_EVENT && r[i].Event.KeyEvent.bKeyDown &&
                (r[i].Event.KeyEvent.uChar.UnicodeChar || r[i].Event.KeyEvent.wVirtualKeyCode == VK_BACK))
                keys++;
        return keys;
    }
#endif
    return 0;
}

/* Esc or Ctrl-C struck while the program computes: SINTRAN breaks at once.
   Looks at the console's waiting keys; if one of them is Esc (when esc_counts)
   or Ctrl-C, everything up to it is consumed and 1 returned.  Other keys stay
   typed ahead.  A pipe never breaks this way. */
int term_break_pending(int esc_counts)
{
#ifdef _WIN32
    INPUT_RECORD r[64];
    DWORD n, i, got;
    if (!in_console || !PeekConsoleInputW(hin, r, 64, &n))
        return 0;
    for (i = 0; i < n; i++) {
        KEY_EVENT_RECORD *k = &r[i].Event.KeyEvent;
        if (r[i].EventType != KEY_EVENT || !k->bKeyDown)
            continue;
        if ((esc_counts && k->wVirtualKeyCode == VK_ESCAPE) || k->uChar.UnicodeChar == 3) {
            ReadConsoleInputW(hin, r, i + 1, &got);
            return 1;
        }
    }
#else
    (void)esc_counts;
#endif
    return 0;
}

int term_getc(void)
{
    term_flush();
#ifdef _WIN32
    if (in_console)
        return console_key();
#endif
    return pipe_key();
}

int term_gets(char *buf, int n)
{
    int i = 0;
    term_flush();
#ifdef _WIN32
    if (in_console) {
        wchar_t w[256];
        DWORD got;
        SetConsoleMode(hin, old_in_mode);
        if (!ReadConsoleW(hin, w, 255, &got, NULL)) got = 0;
        SetConsoleMode(hin, ENABLE_EXTENDED_FLAGS | (old_in_mode & ENABLE_QUICK_EDIT_MODE));
        w[got] = 0;
        i = WideCharToMultiByte(CP_UTF8, 0, w, (int)got, buf, n - 1, NULL, NULL);
        if (i < 0) i = 0;
        buf[i] = 0;
        while (i > 0 && (buf[i - 1] == '\r' || buf[i - 1] == '\n')) buf[--i] = 0;
        return i;
    }
#endif
    for (;;) {
        int c = getchar();
        if (c == EOF) { if (i == 0) return -1; break; }
        if (c == '\n') break;
        if (c == '\r') continue;
        if (i < n - 1) buf[i++] = (char)c;
    }
    buf[i] = 0;
    return i;
}
