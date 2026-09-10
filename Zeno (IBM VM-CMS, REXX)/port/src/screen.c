/* ------------------------------------------------------------------
 * screen.c - a 3270 display, on a Windows console.
 *
 * Zeno was written for a 24x80 IBM 3270: protected text, unprotected
 * fields you tab between, attribute bytes that occupy a screen position
 * of their own, and twelve program-function keys.  All of that is here.
 * Panels are drawn into an alternate console screen buffer so that any
 * line-mode output from the exec (SAY, and the syntax trap) survives on
 * the original buffer and is still readable after the game exits.
 * ------------------------------------------------------------------ */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zeno.h"

static HANDLE hout = INVALID_HANDLE_VALUE;   /* alternate buffer */
static HANDLE hin  = INVALID_HANDLE_VALUE;
static HANDLE horig= INVALID_HANDLE_VALUE;
static int    opened;

int  screen_colour_3279 = 0;                 /* -3279: real terminal colours */
int  screen_intro_pause = 1500;              /* ms to hold a no-read panel */

/* ---- field navigation ---------------------------------------------------- */

typedef struct {
    int idx[MAXFIELDS];      /* field numbers, in screen order */
    int n;
} Inputs;

static void collect_inputs(Screen *s, Inputs *in)
{
    int i;
    in->n = 0;
    for (i = 0; i < s->nf; i++)
        if ((s->f[i].flags & F_INPUT) && s->f[i].len > 0)
            in->idx[in->n++] = i;
}

#define GREEN   (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define WHITE   (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define CYAN    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define YELLOW  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define BLUE    (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define RED     (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define DKGREEN (FOREGROUND_GREEN)

static void   paint(Screen *s);
static HANDLE screen_handle(void);

/* ---- scripted mode ------------------------------------------------------
 * -script FILE replaces the console with a text transcript, so the whole
 * game can be driven and diffed without a terminal.  Used for testing.
 */
int   screen_batch = 0;
int   screen_verify = 0;   /* real console painting, scripted input */
static FILE *scriptf;
static int   dump_seq;
static int   eof_reads;

void screen_set_script(const char *path)
{
    scriptf = fopen(path, "r");
    screen_batch = 1;
}

/* Read the alternate screen buffer back out of the console and print what
 * is actually on it, colours included.  This exercises paint(), which the
 * text harness never touches. */
static void dump_console(void)
{
    static CHAR_INFO buf[SCELLS];
    COORD sz = { SCOLS, SROWS }, org = { 0, 0 };
    SMALL_RECT r = { 0, 0, SCOLS - 1, SROWS - 1 };
    int row, col;
    if (!ReadConsoleOutputA(screen_handle(), buf, sz, org, &r)) {
        printf("[verify] ReadConsoleOutput failed, err=%lu\n", (unsigned long)GetLastError());
        return;
    }
    printf("=== console %d ===\n", ++dump_seq);
    for (row = 0; row < SROWS; row++) {
        char t[SCOLS + 1], a[SCOLS + 1];
        int last = -1;
        for (col = 0; col < SCOLS; col++) {
            CHAR_INFO *c = &buf[row * SCOLS + col];
            t[col] = c->Char.AsciiChar ? c->Char.AsciiChar : ' ';
            switch (c->Attributes & 0x0F) {
            case GREEN:  a[col] = 'g'; break;
            case WHITE:  a[col] = 'W'; break;
            case CYAN:   a[col] = 'c'; break;
            case YELLOW: a[col] = 'y'; break;
            case BLUE:   a[col] = 'b'; break;
            case RED:    a[col] = 'r'; break;
            default:     a[col] = '?'; break;
            }
            if (t[col] != ' ') last = col;
        }
        t[SCOLS] = a[SCOLS] = 0;
        if (last < 0) continue;
        printf("%2d|%.*s\n", row + 1, last + 1, t);
        printf("  |%.*s\n", last + 1, a);
    }
    fflush(stdout);
}

static void paint(Screen *s);
static HANDLE screen_handle(void);

static void dump_text(Screen *s)
{
    int r, c, i;
    printf("--- screen %d ---------------------------------------"
           "---------------------------\n", ++dump_seq);
    for (r = 0; r < SROWS; r++) {
        char line[SCOLS + 1];
        int last = -1;
        for (c = 0; c < SCOLS; c++) {
            int p = r * SCOLS + c;
            unsigned char fl = s->fl[p];
            char ch = s->ch[p];
            if (fl & C_ATTR) ch = (fl & C_INPUT) ? '[' : ' ';
            line[c] = ch ? ch : ' ';
            if (line[c] != ' ') last = c;
        }
        line[last + 1] = 0;
        printf("%2d|%s\n", r + 1, line);
    }
    for (i = 1; i <= 12; i++) if (s->pf[i][0]) printf("   PF%d=%s", i, s->pf[i]);
    printf("\n");
    fflush(stdout);
}

static int batch_read(Screen *s, int *aux)
{
    Inputs in;
    char line[512];
    int cur = 0;

    collect_inputs(s, &in);
    if (in.n && s->cursor_field > 0 && s->cursor_field <= in.n)
        cur = s->cursor_field - 1;
    if (screen_verify) { paint(s); dump_console(); }
    else dump_text(s);

    while (scriptf && fgets(line, sizeof line, scriptf)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (!line[0] || line[0] == '#') continue;
        printf(">> %s\n", line);
        if (line[0] == 'k' && line[1] == ' ') {
            char *k = line + 2;
            if (!strcmp(k, "enter")) return K_ENTER;
            if (!strcmp(k, "esc"))   { *aux = 1; return K_PA; }
            if (k[0] == 'p' && k[1] == 'f') { *aux = atoi(k + 2); return K_PF; }
            if (!strcmp(k, "tab")) { if (in.n) cur = (cur + 1) % in.n; continue; }
        } else if (line[0] == 't' && line[1] == ' ') {
            int f, p;
            if (!in.n) continue;
            f = in.idx[cur];
            for (p = 0; p < s->f[f].len; p++)
                s->ch[s->f[f].attrpos + 1 + p] = line[2 + p] ? line[2 + p] : ' ';
            s->f[f].mdt = 1;
            continue;
        } else if (line[0] == 'f' && line[1] == ' ') {
            int n = atoi(line + 2), f, p;
            char *txt = strchr(line + 2, ' ');
            txt = txt ? txt + 1 : (char *)"";
            if (n < 1 || n > in.n) continue;
            f = in.idx[n - 1];
            for (p = 0; p < s->f[f].len; p++)
                s->ch[s->f[f].attrpos + 1 + p] = txt[p] ? txt[p] : ' ';
            s->f[f].mdt = 1;
            continue;
        } else if (!strcmp(line, "q")) {
            return K_QUIT;
        }
    }
    /* script exhausted: ask to leave, but do not spin if the panel has no
     * way out (the editor has no QUIT key, only FILE) */
    if (++eof_reads > 3) { printf("[script exhausted]\n"); exit(0); }
    return K_QUIT;
}

/* ---- colours ---------------------------------------------------------- */


static WORD cell_colour(unsigned char fl)
{
    int input  = (fl & C_INPUT)  != 0;
    int intens = (fl & C_INTENS) != 0;
    if (screen_colour_3279)                       /* 3279 base colour */
        return input ? (intens ? RED : GREEN) : (intens ? WHITE : BLUE);
    /* default: a green screen that still shows you where you may type */
    if (input)  return intens ? YELLOW : CYAN;
    return intens ? WHITE : GREEN;
}

/* ---- open / close ------------------------------------------------------ */

static HANDLE screen_handle(void) { return hout; }

int con_open(void)
{
    COORD size;
    if (screen_batch && !screen_verify) { opened = 1; return 1; }
    {
    SMALL_RECT win;
    CONSOLE_CURSOR_INFO ci;

    horig = GetStdHandle(STD_OUTPUT_HANDLE);
    hin   = GetStdHandle(STD_INPUT_HANDLE);
    hout  = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (hout == INVALID_HANDLE_VALUE) return 0;

    size.X = SCOLS; size.Y = SROWS;
    win.Left = 0; win.Top = 0; win.Right = SCOLS - 1; win.Bottom = SROWS - 1;
    SetConsoleWindowInfo(hout, TRUE, &win);
    SetConsoleScreenBufferSize(hout, size);
    SetConsoleWindowInfo(hout, TRUE, &win);

    ci.dwSize = 25; ci.bVisible = TRUE;
    SetConsoleCursorInfo(hout, &ci);

    SetConsoleActiveScreenBuffer(hout);
    SetConsoleMode(hin, ENABLE_WINDOW_INPUT);
    SetConsoleTitleA("ZENO - COMPUZZ Deep Space Station");
    }
    opened = 1;
    return 1;
}

void con_close(void)
{
    if (!opened) return;
    if (screen_batch && !screen_verify) { opened = 0; return; }
    SetConsoleActiveScreenBuffer(horig);
    if (hout != INVALID_HANDLE_VALUE) CloseHandle(hout);
    hout = INVALID_HANDLE_VALUE;
    opened = 0;
}

void con_message(const char *msg)
{
    DWORD n;
    if (screen_batch) { printf("[msg] %s\n", msg); return; }
    if (horig == INVALID_HANDLE_VALUE) horig = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(horig, msg, (DWORD)strlen(msg), &n, NULL);
    WriteFile(horig, "\r\n", 2, &n, NULL);
}

/* ---- painting ----------------------------------------------------------- */

static void paint(Screen *s)
{
    static CHAR_INFO buf[SCELLS];
    COORD sz = { SCOLS, SROWS }, org = { 0, 0 };
    SMALL_RECT r = { 0, 0, SCOLS - 1, SROWS - 1 };
    int i;

    for (i = 0; i < SCELLS; i++) {
        unsigned char fl = s->fl[i];
        char ch = s->ch[i];
        if ((fl & C_ATTR) || (fl & C_NONDISP)) ch = ' ';
        buf[i].Char.AsciiChar = ch ? ch : ' ';
        buf[i].Attributes = cell_colour(fl);
    }
    WriteConsoleOutputA(hout, buf, sz, org, &r);
}

void con_show(Screen *s)
{
    if (!opened) return;
    if (screen_batch) { dump_text(s); return; }
    paint(s);
    if (s->alarm) Beep(880, 120);
    if (screen_intro_pause) Sleep(screen_intro_pause);
}


static int fld_pos(Screen *s, int f, int off) { return s->f[f].attrpos + 1 + off; }

/* Move to the input field nearest (row, col) in direction dir (-1 up, +1 down) */
static int nearest_field(Screen *s, Inputs *in, int cur, int col, int dir)
{
    int i, best = -1, bestrow = -1;
    int currow = fld_pos(s, in->idx[cur], 0) / SCOLS;
    for (i = 0; i < in->n; i++) {
        int r = fld_pos(s, in->idx[i], 0) / SCOLS;
        if (dir < 0 && r >= currow) continue;
        if (dir > 0 && r <= currow) continue;
        if (best < 0 ||
            (dir < 0 && r > bestrow) ||
            (dir > 0 && r < bestrow)) { best = i; bestrow = r; }
        else if (r == bestrow) {
            int c1 = fld_pos(s, in->idx[i], 0) % SCOLS;
            int c2 = fld_pos(s, in->idx[best], 0) % SCOLS;
            if (abs(c1 - col) < abs(c2 - col)) best = i;
        }
    }
    return best < 0 ? cur : best;
}

static void field_text(Screen *s, int f, char *out)
{
    int p;
    for (p = 0; p < s->f[f].len; p++) out[p] = s->ch[s->f[f].attrpos + 1 + p];
    out[s->f[f].len] = 0;
}

static void field_put(Screen *s, int f, int off, char c)
{
    int p = fld_pos(s, f, off);
    if (off < 0 || off >= s->f[f].len) return;
    s->ch[p] = c;
    s->f[f].mdt = 1;
}

/* ---- the read loop -------------------------------------------------------- */

int con_read(Screen *s, int *aux)
{
    Inputs in;
    int cur = 0, off = 0, insert = 0;
    INPUT_RECORD ir;
    DWORD got;

    *aux = 0;
    if (!opened) return K_QUIT;
    if (screen_batch) return batch_read(s, aux);
    collect_inputs(s, &in);

    if (in.n && s->cursor_field > 0 && s->cursor_field <= in.n)
        cur = s->cursor_field - 1;

    for (;;) {
        int p;
        paint(s);
        if (in.n) {
            COORD c;
            p = fld_pos(s, in.idx[cur], off);
            c.X = (SHORT)(p % SCOLS); c.Y = (SHORT)(p / SCOLS);
            SetConsoleCursorPosition(hout, c);
            s->cursor_row = c.Y; s->cursor_col = c.X;
        } else {
            COORD c = { 0, SROWS - 1 };
            SetConsoleCursorPosition(hout, c);
            s->cursor_row = SROWS - 1; s->cursor_col = 0;
        }
        if (s->alarm) { Beep(880, 120); s->alarm = 0; }

        if (!ReadConsoleInputA(hin, &ir, 1, &got) || got == 0) return K_QUIT;
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;

        {
            WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
            char ch = ir.Event.KeyEvent.uChar.AsciiChar;
            DWORD st = ir.Event.KeyEvent.dwControlKeyState;
            int shift = (st & SHIFT_PRESSED) != 0;
            int f     = in.n ? in.idx[cur] : -1;
            int len   = f >= 0 ? s->f[f].len : 0;

            if (vk >= VK_F1 && vk <= VK_F12) {
                *aux = vk - VK_F1 + 1 + (shift ? 12 : 0);
                return K_PF;
            }
            switch (vk) {
            case VK_RETURN: return K_ENTER;
            case VK_ESCAPE: *aux = 1; return K_PA;
            case VK_TAB:
                if (!in.n) break;
                cur = shift ? (cur + in.n - 1) % in.n : (cur + 1) % in.n;
                off = 0;
                break;
            case VK_LEFT:
                if (!in.n) break;
                if (--off < 0) { cur = (cur + in.n - 1) % in.n; off = s->f[in.idx[cur]].len - 1; }
                break;
            case VK_RIGHT:
                if (!in.n) break;
                if (++off >= len) { cur = (cur + 1) % in.n; off = 0; }
                break;
            case VK_UP:
            case VK_DOWN: {
                int col, nc;
                if (!in.n) break;
                col = fld_pos(s, f, off) % SCOLS;
                nc = nearest_field(s, &in, cur, col, vk == VK_UP ? -1 : 1);
                if (nc != cur) {
                    int nf = in.idx[nc], c0 = s->f[nf].attrpos % SCOLS + 1;
                    cur = nc;
                    off = col - c0;
                    if (off < 0) off = 0;
                    if (off >= s->f[nf].len) off = s->f[nf].len - 1;
                }
                break;
            }
            case VK_HOME: off = 0; break;
            case VK_END: {
                char t[SCOLS + 1];
                int k;
                if (!in.n) break;
                field_text(s, f, t);
                for (k = len; k > 0 && t[k - 1] == ' '; k--) ;
                off = k < len ? k : len - 1;
                break;
            }
            case VK_INSERT: insert = !insert; break;
            case VK_BACK:
                if (!in.n || off <= 0) break;
                {
                    int k;
                    for (k = off - 1; k < len - 1; k++)
                        s->ch[fld_pos(s, f, k)] = s->ch[fld_pos(s, f, k + 1)];
                    s->ch[fld_pos(s, f, len - 1)] = ' ';
                    s->f[f].mdt = 1;
                    off--;
                }
                break;
            case VK_DELETE:
                if (!in.n) break;
                {
                    int k;
                    for (k = off; k < len - 1; k++)
                        s->ch[fld_pos(s, f, k)] = s->ch[fld_pos(s, f, k + 1)];
                    s->ch[fld_pos(s, f, len - 1)] = ' ';
                    s->f[f].mdt = 1;
                }
                break;
            default:
                if (!in.n) break;
                if ((unsigned char)ch >= 32 && (unsigned char)ch < 127) {
                    if (insert) {
                        int k;
                        for (k = len - 1; k > off; k--)
                            s->ch[fld_pos(s, f, k)] = s->ch[fld_pos(s, f, k - 1)];
                    }
                    field_put(s, f, off, ch);
                    off++;
                    if (off >= len) {
                        if (s->f[f].flags & F_SKIP) { cur = (cur + 1) % in.n; off = 0; }
                        else off = len - 1;
                    }
                }
                break;
            }
        }
    }
}
