/* con_win.c -- Windows console front end.
 *
 * Mirrors the workstation's 24x80 character plane into a console screen
 * buffer, takes the cursor position from the attribute plane, and turns
 * host key presses back into Wang keyboard scan codes.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "wang.h"

static HANDLE hout, hin;
static CHAR_INFO shadow[SCR_ROWS * SCR_COLS];
static int shadow_valid = 0;
static int last_cur_row = -1, last_cur_col = -1;

static int   have_console = 0;
static DWORD saved_in_mode = 0;
static int   saved_in_mode_ok = 0;
static CONSOLE_SCREEN_BUFFER_INFO saved_sbi;
static CONSOLE_CURSOR_INFO saved_ci;

void con_init(void)
{
    COORD size;
    SMALL_RECT win;
    CONSOLE_CURSOR_INFO ci;

    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin  = GetStdHandle(STD_INPUT_HANDLE);

    have_console = GetConsoleScreenBufferInfo(hout, &saved_sbi) != 0;
    if (have_console) GetConsoleCursorInfo(hout, &saved_ci);

    SetConsoleTitleA("WANG 928 ADVENTURE (Version 2.1)");
    SetConsoleOutputCP(437);

    if (have_console) {
        /* Shrink the window before resizing the buffer, then grow it back. */
        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        size.X = SCR_COLS; size.Y = SCR_ROWS;
        SetConsoleScreenBufferSize(hout, size);
        win.Left = 0; win.Top = 0; win.Right = SCR_COLS - 1; win.Bottom = SCR_ROWS - 1;
        SetConsoleWindowInfo(hout, TRUE, &win);

        ci.dwSize = 20; ci.bVisible = TRUE;
        SetConsoleCursorInfo(hout, &ci);
    }

    saved_in_mode_ok = GetConsoleMode(hin, &saved_in_mode) != 0;
    if (saved_in_mode_ok)
        SetConsoleMode(hin, saved_in_mode
                       & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
}

void con_refresh(void)
{
    CHAR_INFO buf[SCR_ROWS * SCR_COLS];
    COORD size, org;
    SMALL_RECT dst;
    int r, c, changed = 0;
    int cursor_row = 0, cursor_col = 0, have_cursor = 0;

    if (!have_console) return;

    for (r = 0; r < SCR_ROWS; r++) {
        for (c = 0; c < SCR_COLS; c++) {
            int i = r * SCR_COLS + c;
            uint8_t at = mem[SCR_ATTR + r * 0x100 + c];
            WORD attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;

            /* The program uses two attribute bits: 80h marks the 32-column
               keyboard entry field on the bottom line, and 20h rides along
               on the one cell that holds the cursor. */
            if (at & 0x80) attr |= COMMON_LVB_UNDERSCORE;
            if (at & 0x20) { cursor_row = r; cursor_col = c; have_cursor = 1; }

            buf[i].Char.AsciiChar = wang_to_ascii(mem[SCR_CHAR + r * 0x100 + c]);
            buf[i].Attributes = attr;
            if (!shadow_valid ||
                shadow[i].Char.AsciiChar != buf[i].Char.AsciiChar ||
                shadow[i].Attributes != buf[i].Attributes)
                changed = 1;
        }
    }

    if (!have_cursor) { cursor_row = cur_row; cursor_col = cur_col; }
    if (!changed && cursor_row == last_cur_row && cursor_col == last_cur_col) return;

    if (changed) {
        memcpy(shadow, buf, sizeof buf);
        shadow_valid = 1;

        size.X = SCR_COLS; size.Y = SCR_ROWS;
        org.X = 0; org.Y = 0;
        dst.Left = 0; dst.Top = 0; dst.Right = SCR_COLS - 1; dst.Bottom = SCR_ROWS - 1;
        WriteConsoleOutputA(hout, buf, size, org, &dst);
    }

    {
        COORD p;
        p.X = (SHORT)cursor_col;
        p.Y = (SHORT)cursor_row;
        SetConsoleCursorPosition(hout, p);
        last_cur_row = cursor_row;
        last_cur_col = cursor_col;
    }
}

void con_poll(void)
{
    INPUT_RECORD rec[32];
    DWORD avail = 0, got = 0, i;

    if (!GetNumberOfConsoleInputEvents(hin, &avail) || avail == 0) return;
    if (avail > 32) avail = 32;
    if (!ReadConsoleInputA(hin, rec, avail, &got)) return;

    for (i = 0; i < got; i++) {
        KEY_EVENT_RECORD *k;
        unsigned char ch;
        uint8_t sc;

        if (rec[i].EventType != KEY_EVENT) continue;
        k = &rec[i].Event.KeyEvent;
        if (!k->bKeyDown) continue;

        if (k->wVirtualKeyCode == VK_ESCAPE) { running = 0; return; }

        ch = (unsigned char)k->uChar.AsciiChar;
        if (ch == 0) continue;
        if (ch == 3) { running = 0; return; }          /* Ctrl+C */
        if (ch == '\n') ch = '\r';
        if (ch == 0x7F) ch = 0x08;                     /* backspace */

        sc = key_scancode(ch);
        if (sc) kq_push(sc);
    }
}

void con_idle(void)
{
    Sleep(5);
}

void con_shutdown(void)
{
    if (saved_in_mode_ok) SetConsoleMode(hin, saved_in_mode);
    if (!have_console) return;

    SetConsoleCursorInfo(hout, &saved_ci);
    {
        /* Put the console back the way we found it. */
        SMALL_RECT win;
        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        SetConsoleScreenBufferSize(hout, saved_sbi.dwSize);
        SetConsoleWindowInfo(hout, TRUE, &saved_sbi.srWindow);
    }
    {
        COORD p;
        p.X = 0; p.Y = (SHORT)(SCR_ROWS - 1);
        SetConsoleCursorPosition(hout, p);
    }
    printf("\n");
}
