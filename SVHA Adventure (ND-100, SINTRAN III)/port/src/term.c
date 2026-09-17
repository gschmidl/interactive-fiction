/*
 * term.c - the SINTRAN terminal on a Windows console or a pipe.
 *
 * The game prints 7-bit ASCII in the Norwegian national variant (NS 4551):
 * [ \ ] { | } are Æ Ø Å æ ø å.  On a console these are shown as the letters
 * and the letters typed are sent back as the 7-bit codes.  A pipe gets UTF-8
 * the same way unless raw mode is on, which passes the ND bytes untouched
 * (that is what the reference transcripts in tests\ are compared against).
 *
 * Input is one keystroke at a time with no host echo: the game's own
 * runtime echoes and edits (Ctrl-A deletes a character, Ctrl-Q the line).
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

static int raw_mode, charset;
static unsigned char obuf[4096];
static int olen;

#ifdef _WIN32
static HANDLE hin, hout;
static DWORD old_in_mode, old_out_mode;
static int in_console, out_console;
static UINT old_cp;
#endif

static const char nd_national[] = "[\\]{|}";
static const wchar_t uni_national[] = L"ÆØÅæøå";

int term_is_console(void)
{
#ifdef _WIN32
    return in_console;
#else
    return 0;
#endif
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
    term_flush();
#ifdef _WIN32
    if (in_console)
        SetConsoleMode(hin, old_in_mode);
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

void term_putc(int ch)
{
    const char *p;
    ch &= 0x7F;
    if (raw_mode) {
        put_byte((unsigned char)ch);
        return;
    }
    if (ch < 32) {
        /* keep what a terminal acts on; the start-up EM (031) and the
           kill-line echo (004) show nothing on the terminal either */
        if (ch == '\r' || ch == '\n' || ch == '\a' || ch == '\b' || ch == '\t')
            put_byte((unsigned char)ch);
        return;
    }
    if (ch == 0x7F)
        return;
    if (charset == CS_NORWEGIAN && (p = strchr(nd_national, ch)) != NULL) {
        wchar_t u = uni_national[p - nd_national];
        put_byte((unsigned char)(0xC0 | (u >> 6)));
        put_byte((unsigned char)(0x80 | (u & 0x3F)));
        return;
    }
    put_byte((unsigned char)ch);
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
    if (charset == CS_NORWEGIAN || !raw_mode) {
        for (p = uni_national; *p; p++)
            if (*p == u)
                return nd_national[p - uni_national];
        /* Ä and Ö as typed on Swedish/German keyboards stand in for Æ and Ø */
        if (u == 0xC4) return '[';
        if (u == 0xE4) return '{';
        if (u == 0xD6) return '\\';
        if (u == 0xF6) return '|';
    }
    return -2;   /* not on an ND terminal: ignore */
}

#ifdef _WIN32
static int console_key(void)
{
    INPUT_RECORD r;
    DWORD n;
    static int pending, repeat;
    for (;;) {
        if (repeat > 0) {
            repeat--;
            return pending;
        }
        if (!ReadConsoleInputW(hin, &r, 1, &n) || n == 0)
            return -1;
        if (r.EventType != KEY_EVENT || !r.Event.KeyEvent.bKeyDown)
            continue;
        {
            KEY_EVENT_RECORD *k = &r.Event.KeyEvent;
            unsigned u = k->uChar.UnicodeChar;
            int c;
            if (k->wVirtualKeyCode == VK_BACK)
                c = 1;                         /* the ND delete key was Ctrl-A */
            else if (k->wVirtualKeyCode == VK_RETURN)
                c = '\r';
            else if (u == 0)
                continue;
            else if ((c = map_unicode(u)) < 0)
                continue;
            if (c == '\n')
                c = '\r';
            pending = c;
            repeat = k->wRepeatCount > 0 ? k->wRepeatCount - 1 : 0;
            return c;
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
