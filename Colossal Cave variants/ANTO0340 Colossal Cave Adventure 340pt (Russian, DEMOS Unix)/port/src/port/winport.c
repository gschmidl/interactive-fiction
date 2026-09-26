/* ====================================================================== *
 *  winport.c -- the Windows side of the port.                            *
 *                                                                        *
 *  Two jobs:                                                             *
 *                                                                        *
 *  1. Files.  The 1984 sources open everything in text mode, look for    *
 *     their data in the current directory (or /usr/lib/games), and name  *
 *     the save file "adv:frozen" -- a colon, which Windows reads as an   *
 *     NTFS stream separator.  port_open/port_fopen fix all three.        *
 *                                                                        *
 *  2. Text.  The game speaks KOI8-R and nothing else: its message file,  *
 *     its vocabulary and its "make this letter lower case" routine all   *
 *     assume that encoding.  The Linux build hands the problem to        *
 *     "luit -encoding KOI8-R"; there is no luit on Windows, so fd 1 and  *
 *     fd 0 are bridged here -- KOI8-R out to UTF-16 for WriteConsoleW,   *
 *     and the console's UTF-16 back to KOI8-R on the way in.            *
 * ====================================================================== */

#ifndef PORT_IMPLEMENTATION
#define PORT_IMPLEMENTATION
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincon.h>
#include <stdarg.h>
#include "portcompat.h"

/* ---------------------------------------------------------------- *
 *  KOI8-R -> Unicode.  Bytes 00..7F are ASCII; this covers 80..FF.  *
 * ---------------------------------------------------------------- */
static const unsigned short koi8r_uni[128] = {
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,   /* 80 */
    0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,   /* 88 */
    0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248,   /* 90 */
    0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7,   /* 98 */
    0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,   /* A0 */
    0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,   /* A8 */
    0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,   /* B0 */
    0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9,   /* B8 */
    0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,   /* C0 */
    0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,   /* C8 */
    0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,   /* D0 */
    0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,   /* D8 */
    0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,   /* E0 */
    0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,   /* E8 */
    0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,   /* F0 */
    0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A    /* F8 */
};

/* Unicode -> KOI8-R.  Returns 0 for anything the encoding cannot hold. */
static int uni_koi8(unsigned u)
{
    int i;
    if (u < 0x80) return (int) u;
    for (i = 0; i < 128; ++i)
        if (koi8r_uni[i] == u) return i + 0x80;
    /* Cyrillic typed as the "wrong" case of a lookalike, plus the two
       common apostrophe substitutes, still land somewhere sensible. */
    switch (u) {
        case 0x2019: case 0x02BC: return '\'';
        case 0x2014: case 0x2013: return '-';
    }
    return 0;
}

/* ---------------------------------------------------------------- *
 *  Where the files live                                             *
 * ---------------------------------------------------------------- */
static char exedir[MAX_PATH];

static void find_exedir(void)
{
    char *p;
    DWORD n = GetModuleFileNameA(NULL, exedir, sizeof exedir);
    if (n == 0 || n >= sizeof exedir) { exedir[0] = '\0'; return; }
    p = strrchr(exedir, '\\');
    if (p) p[1] = '\0'; else exedir[0] = '\0';
}

/* Strip any Unix directory part and turn "adv:frozen" into "adv.frozen":
   a colon in a Windows path names an alternate data stream. */
static void basename_safe(const char *name, char *out, size_t outsz)
{
    const char *b = name, *p;
    size_t i;
    for (p = name; *p; ++p)
        if (*p == '/' || *p == '\\') b = p + 1;
    for (i = 0; i + 1 < outsz && b[i]; ++i)
        out[i] = (b[i] == ':') ? '.' : b[i];
    out[i] = '\0';
}

static int is_database(const char *base)
{
    return !strcmp(base, "adv.text")
        || !strcmp(base, "adv.data")
        || !strcmp(base, "adv.common");
}

/* Reading: current directory first (so the game can be run out of a work
   tree, and so a modified database wins), then the .exe's own directory.
   Writing: the three database files are output of the "ini" build tool and
   belong in the current directory, exactly as under Unix; the save file
   goes next to the .exe so it is found again wherever the game is run. */
static const char *resolve(const char *name, int writing)
{
    static char path[MAX_PATH * 2];
    char base[MAX_PATH];

    basename_safe(name, base, sizeof base);

    if (writing && is_database(base)) return strcpy(path, base);

    if (!writing && GetFileAttributesA(base) != INVALID_FILE_ATTRIBUTES)
        return strcpy(path, base);

    if (exedir[0]) { strcpy(path, exedir); strcat(path, base); }
    else strcpy(path, base);
    return path;
}

int port_open(const char *name, int flags)
{
    return _open(resolve(name, (flags & (_O_WRONLY | _O_RDWR)) != 0),
                 flags | _O_BINARY);
}

FILE *port_fopen(const char *name, const char *mode)
{
    char m[8];
    size_t i, j = 0;
    int writing = 0;

    for (i = 0; mode[i] && j + 2 < sizeof m; ++i) {
        if (mode[i] == 't' || mode[i] == 'b') continue;
        if (mode[i] == 'w' || mode[i] == 'a' || mode[i] == '+') writing = 1;
        m[j++] = mode[i];
    }
    m[j++] = 'b';                       /* never text mode: the database   */
    m[j]   = '\0';                      /* is binary and contains 0x1A     */

    return fopen(resolve(name, writing), m);
}

int port_unlink(const char *name)
{
    return _unlink(resolve(name, 1));
}

/* ---------------------------------------------------------------- *
 *  Console                                                          *
 * ---------------------------------------------------------------- */
static HANDLE hIn, hOut;
static int in_is_console, out_is_console;

/* The legacy console defaults to a raster font that has no Cyrillic at
   all -- every letter would come out as a box.  Switch to a TrueType face
   if that is what we find. */
static void ensure_unicode_font(void)
{
    CONSOLE_FONT_INFOEX fi;
    if (!out_is_console) return;
    memset(&fi, 0, sizeof fi);
    fi.cbSize = sizeof fi;
    if (!GetCurrentConsoleFontEx(hOut, FALSE, &fi)) return;
    if (fi.FontFamily & TMPF_TRUETYPE) return;      /* already fine */

    memset(&fi, 0, sizeof fi);
    fi.cbSize      = sizeof fi;
    fi.dwFontSize.X = 0;
    fi.dwFontSize.Y = 18;
    fi.FontFamily  = FF_DONTCARE;
    fi.FontWeight  = FW_NORMAL;
    wcscpy(fi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &fi);
}

void port_init(void)
{
    DWORD mode;
    find_exedir();
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    in_is_console  = (hIn  != INVALID_HANDLE_VALUE && GetConsoleMode(hIn,  &mode));
    out_is_console = (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode));
    ensure_unicode_font();
    if (!out_is_console) {
        /* Redirected: emit UTF-8, and in binary mode -- the CRT would
           otherwise turn every LF the game writes into CR LF, so a piped
           transcript would not be what the game actually produced. */
        SetConsoleOutputCP(CP_UTF8);
        _setmode(1, _O_BINARY);
        _setmode(2, _O_BINARY);
    }
    if (!in_is_console) _setmode(0, _O_BINARY);
}

/* run before main(), so neither advent.c nor init_adv.c has to change */
__attribute__((constructor)) static void port_ctor(void) { port_init(); }

/* --- output ---------------------------------------------------------- */

/* Convert KOI8-R to the UTF-16 the console wants, turning the game's bare
   LF into CR LF so it lands in column 0 under conhost and under Windows
   Terminal alike.  Returns the number of UTF-16 units produced and reports
   how many input bytes were consumed through *used.  Exposed (not static)
   so tests/koi8test.c can check the mapping independently. */
unsigned koi8_to_utf16(const unsigned char *p, unsigned n,
                       WCHAR *w, unsigned wmax, unsigned *used)
{
    unsigned i, k = 0;
    for (i = 0; i < n; ++i) {
        if (k + 2 > wmax) break;
        if (p[i] == 0x0A) w[k++] = 0x000D;
        w[k++] = (p[i] < 0x80) ? (WCHAR) p[i] : (WCHAR) koi8r_uni[p[i] - 0x80];
    }
    *used = i;
    return k;
}

static int write_console(const unsigned char *p, unsigned n)
{
    WCHAR w[1024];
    unsigned off = 0;

    while (off < n) {
        unsigned used, k;
        DWORD done;
        k = koi8_to_utf16(p + off, n - off, w,
                          (unsigned)(sizeof w / sizeof w[0]), &used);
        if (used == 0) break;
        WriteConsoleW(hOut, w, k, &done, NULL);
        off += used;
    }
    return (int) n;
}

static int write_utf8(int fd, const unsigned char *p, unsigned n)
{
    unsigned char b[3072];
    unsigned i, k = 0;

    for (i = 0; i < n; ++i) {
        unsigned u = (p[i] < 0x80) ? p[i] : koi8r_uni[p[i] - 0x80];
        if (k + 4 > sizeof b) { _write(fd, b, k); k = 0; }
        if (u < 0x80) {
            b[k++] = (unsigned char) u;
        } else if (u < 0x800) {
            b[k++] = (unsigned char)(0xC0 | (u >> 6));
            b[k++] = (unsigned char)(0x80 | (u & 0x3F));
        } else {
            b[k++] = (unsigned char)(0xE0 | (u >> 12));
            b[k++] = (unsigned char)(0x80 | ((u >> 6) & 0x3F));
            b[k++] = (unsigned char)(0x80 | (u & 0x3F));
        }
    }
    if (k) _write(fd, b, k);
    return (int) n;
}

int port_write(int fd, const void *buf, unsigned n)
{
    if (fd != 1 && fd != 2) return _write(fd, buf, n);
    if (out_is_console)     return write_console((const unsigned char *) buf, n);
    return write_utf8(fd, (const unsigned char *) buf, n);
}

int port_printf(const char *fmt, ...)
{
    char buf[4096];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return n;
    if (n > (int) sizeof buf - 1) n = (int) sizeof buf - 1;
    return port_write(1, buf, (unsigned) n);
}

/* --- input ----------------------------------------------------------- *
 *  read(0,...) on the original ran against a tty in cooked mode: one     *
 *  call returned exactly one line.  Both paths below keep that, so a     *
 *  piped script behaves the same as somebody typing.                     *
 * ---------------------------------------------------------------- */

static unsigned char linebuf[512];
static unsigned line_len, line_pos;
static int at_eof;

static void push(unsigned u)
{
    int c = uni_koi8(u);
    if (c == 0) c = '?';
    if (c == '\r') return;
    if (line_len + 1 < sizeof linebuf) linebuf[line_len++] = (unsigned char) c;
}

static void fill_from_console(void)
{
    WCHAR w[256];
    DWORD got = 0;
    unsigned i;

    if (!ReadConsoleW(hIn, w, sizeof w / sizeof w[0], &got, NULL) || got == 0) {
        at_eof = 1;
        return;
    }
    for (i = 0; i < got; ++i) {
        if (w[i] == 0x1A) { at_eof = 1; break; }        /* Ctrl-Z */
        push(w[i]);
    }
    if (line_len == 0 || linebuf[line_len - 1] != '\n') push('\n');
}

/* Redirected input is taken as UTF-8; a line that is not valid UTF-8 is
   passed through unchanged, so a KOI8-R script still works. */
static void fill_from_pipe(void)
{
    unsigned char raw[512];
    unsigned n = 0, i;
    int valid = 1;

    for (;;) {
        unsigned char c;
        int r = _read(0, &c, 1);
        if (r <= 0) { if (n == 0) at_eof = 1; break; }
        if (c == '\r') continue;
        if (n < sizeof raw) raw[n++] = c;
        if (c == '\n') break;
    }
    if (n == 0) return;

    for (i = 0; i < n; ) {                              /* validate UTF-8 */
        unsigned char c = raw[i];
        if (c < 0x80) { ++i; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < n && (raw[i+1] & 0xC0) == 0x80) i += 2;
        else if ((c & 0xF0) == 0xE0 && i + 2 < n
                 && (raw[i+1] & 0xC0) == 0x80 && (raw[i+2] & 0xC0) == 0x80) i += 3;
        else { valid = 0; break; }
    }

    if (!valid) {
        for (i = 0; i < n; ++i)
            if (line_len + 1 < sizeof linebuf) linebuf[line_len++] = raw[i];
        return;
    }
    for (i = 0; i < n; ) {
        unsigned char c = raw[i];
        unsigned u;
        if (c < 0x80)              { u = c;                                i += 1; }
        else if ((c & 0xE0) == 0xC0) { u = ((c & 0x1Fu) << 6) | (raw[i+1] & 0x3Fu); i += 2; }
        else                       { u = ((c & 0x0Fu) << 12)
                                       | ((raw[i+1] & 0x3Fu) << 6)
                                       |  (raw[i+2] & 0x3Fu);                i += 3; }
        push(u);
    }
}

int port_read(int fd, void *buf, unsigned n)
{
    unsigned char *out = (unsigned char *) buf;
    unsigned k = 0;

    if (fd != 0) return _read(fd, buf, n);

    if (line_pos >= line_len) {
        line_pos = line_len = 0;
        if (at_eof) return 0;
        if (in_is_console) fill_from_console(); else fill_from_pipe();
        if (line_len == 0) return 0;
    }
    while (k < n && line_pos < line_len) {
        out[k] = linebuf[line_pos++];
        if (out[k++] == '\n') break;                    /* one line per read */
    }
    return (int) k;
}
