/* d200.h -- a Data General Dasher D200 on the player's console.
 *
 * QUEST has to be run on a D200 ("set tti d200 / set tto d200"): it draws
 * with the D200's control codes and reads with DG screen-edit reads.  Here
 * the player's @INPUT/@OUTPUT go through a small D200: the codes the game
 * writes update a virtual 80x24 screen, and the screen is shown one of two
 * ways --
 *
 *   ANSI   on a real console: only the cells that changed are redrawn, so
 *          the display stays right whatever size the window is.
 *   SNAP   when output is not a console (a pipe, a test): each time the
 *          game waits for a key the screen is printed as plain text.  That
 *          is what makes a scripted session readable and diffable.
 *
 * The D200 codes, from the Dasher programming reference as DG terminals
 * implement them:
 *
 *   003/004 blink enable/disable   005 read cursor address -> 037 col row
 *   007 bell   010 home   012 new line   013 erase to end of line
 *   014 erase screen   015 carriage return   016/017 blink on/off
 *   020 col row  write cursor address   021 print form
 *   022/023 roll enable/disable   024/025 underscore on/off
 *   027/030/031/032 cursor up/right/left/down   034/035 dim on/off
 *   036 D / 036 E  reverse video on/off
 *
 * Only the player's console is translated.  QUP.CLI ran the server with
 * /out=quest.out, so the server's @OUTPUT goes to QUEST.OUT in the save
 * directory and its @INPUT is empty.
 */
#ifndef D200_H
#define D200_H

#include <conio.h>
#include <io.h>
#include <signal.h>

/* Three Win32 calls, declared here rather than by including windows.h:
 * winuser.h defines SC_CLOSE and friends, which are this shim's names for
 * AOS/VS system calls. */
__declspec(dllimport) void *__stdcall GetStdHandle(unsigned long);
__declspec(dllimport) int   __stdcall GetConsoleMode(void *, unsigned long *);
__declspec(dllimport) int   __stdcall SetConsoleMode(void *, unsigned long);
__declspec(dllimport) void  __stdcall Sleep(unsigned long);

enum { TM_ANSI = 1, TM_SNAP, TM_RAW };
static int term_d200;          /* translate the player's console           */
static int term_mode;          /* TM_*                                     */
static int kb_console;         /* stdin is a keyboard: read raw keys       */

#define VR 24
#define VC 80
#define AT_DIM 1
#define AT_UND 2
#define AT_BLK 4
#define AT_REV 8

static unsigned char vch[VR][VC], vat[VR][VC];   /* the D200's screen       */
static unsigned char rch[VR][VC], rat[VR][VC];   /* what is on the console  */
static int vx, vy, vattr, vroll = 1;
static int d2st, d2col;
static int v_dirty, v_started;

static int kbq[64], kbq_n;     /* keys the terminal itself generates        */

/* ---- the screen ------------------------------------------------------- */
static void v_clear(void)
{ memset(vch, ' ', sizeof vch); memset(vat, 0, sizeof vat); }

static void v_newline(void)
{
    vx = 0;
    if (vy < VR - 1) { vy++; return; }
    if (!vroll) { vy = 0; return; }
    memmove(vch, vch + 1, (VR - 1) * VC);
    memmove(vat, vat + 1, (VR - 1) * VC);
    memset(vch[VR - 1], ' ', VC);
    memset(vat[VR - 1], 0, VC);
}

static void v_put(int c)
{
    vch[vy][vx] = (unsigned char)c;
    vat[vy][vx] = (unsigned char)vattr;
    if (++vx >= VC) v_newline();
}

static void d2_putc(int c)
{
    c &= 0x7F;
    v_dirty = 1;
    switch (d2st) {
    case 1: d2col = c; d2st = 2; return;
    case 2: vx = d2col % VC; vy = c % VR; d2st = 0; return;
    case 3:
        d2st = 0;
        if (c == 'D') vattr |= AT_REV;
        else if (c == 'E') vattr &= ~AT_REV;
        else if (verbose) fprintf(stderr, "   [d200: 036 %03o ignored]\n", c);
        return;
    }
    switch (c) {
    case 000: case 003: case 004: case 021: return;
    case 005:                             /* read cursor address */
        if (kbq_n + 3 <= (int)(sizeof kbq / sizeof kbq[0])) {
            kbq[kbq_n++] = 037; kbq[kbq_n++] = vx; kbq[kbq_n++] = vy;
        }
        return;
    case 007: if (term_mode == TM_ANSI) fputc(7, stdout); return;
    case 010: vx = vy = 0; return;
    case 012: v_newline(); return;
    case 013: memset(vch[vy] + vx, ' ', (size_t)(VC - vx));
              memset(vat[vy] + vx, 0, (size_t)(VC - vx)); return;
    case 014: v_clear(); vx = vy = 0; return;
    case 015: vx = 0; return;
    case 016: vattr |= AT_BLK; return;
    case 017: vattr &= ~AT_BLK; return;
    case 020: d2st = 1; return;
    case 022: vroll = 1; return;
    case 023: vroll = 0; return;
    case 024: vattr |= AT_UND; return;
    case 025: vattr &= ~AT_UND; return;
    case 027: vy = (vy + VR - 1) % VR; return;
    case 030: if (++vx >= VC) { vx = 0; vy = (vy + 1) % VR; } return;
    case 031: if (vx > 0) vx--; else { vx = VC - 1; vy = (vy + VR - 1) % VR; }
              return;
    case 032: vy = (vy + 1) % VR; return;
    case 034: vattr |= AT_DIM; return;
    case 035: vattr &= ~AT_DIM; return;
    case 036: d2st = 3; return;
    }
    if (c >= 040 && c < 0177) { v_put(c); return; }
    if (verbose) fprintf(stderr, "   [d200: code %03o ignored]\n", c);
}

/* ---- showing it ------------------------------------------------------- */
static void sgr(int a)
{
    fputs("\x1b[0", stdout);
    if (a & AT_DIM) fputs(";2", stdout);
    if (a & AT_UND) fputs(";4", stdout);
    if (a & AT_BLK) fputs(";5", stdout);
    if (a & AT_REV) fputs(";7", stdout);
    fputc('m', stdout);
}

static void v_render_ansi(void)
{
    int r, c, cur = -1;
    if (!v_started) {
        fputs("\x1b[0m\x1b[2J\x1b[H", stdout);
        memset(rch, 0, sizeof rch); memset(rat, 0xFF, sizeof rat);
        v_started = 1;
    }
    for (r = 0; r < VR; r++) {
        int lo = -1, hi = -1;
        for (c = 0; c < VC; c++)
            if (vch[r][c] != rch[r][c] || vat[r][c] != rat[r][c])
                { if (lo < 0) lo = c; hi = c; }
        if (lo < 0) continue;
        printf("\x1b[%d;%dH", r + 1, lo + 1);
        for (c = lo; c <= hi; c++) {
            if (vat[r][c] != cur) { sgr(vat[r][c]); cur = vat[r][c]; }
            fputc(vch[r][c], stdout);
            rch[r][c] = vch[r][c]; rat[r][c] = vat[r][c];
        }
    }
    if (cur) fputs("\x1b[0m", stdout);
    printf("\x1b[%d;%dH", vy + 1, vx + 1);
    fflush(stdout);
}

static void v_render_snap(void)
{
    int r, last = -1;
    for (r = 0; r < VR; r++) {
        int c;
        for (c = 0; c < VC; c++) if (vch[r][c] != ' ') { last = r; break; }
    }
    printf("+--------------------------------------------------------------------------------+ %d,%d\n",
           vy, vx);
    for (r = 0; r <= last; r++) {
        int c;
        fputc('|', stdout);
        for (c = 0; c < VC; c++) fputc(vch[r][c], stdout);
        fputs("|\n", stdout);
    }
    printf("+--------------------------------------------------------------------------------+\n");
    fflush(stdout);
}

/* `waiting` is true when the game is about to wait for a key: that is the
 * moment a snapshot means something. */
static void v_show(int waiting)
{
    if (term_mode == TM_ANSI) { if (v_dirty) v_render_ansi(); v_dirty = 0; }
    else if (term_mode == TM_SNAP && waiting && v_dirty) { v_render_snap(); v_dirty = 0; }
    else if (term_mode == TM_RAW) fflush(stdout);
}

static void d2_write(const char *b, int n)
{
    int i;
    if (term_mode == TM_RAW) { fwrite(b, 1, (size_t)n, stdout); return; }
    for (i = 0; i < n; i++) d2_putc((unsigned char)b[i]);
    v_show(0);
}

/* ---- the keyboard ----------------------------------------------------- *
 * A PC keyboard mapped onto the D200's: the arrow keys send the D200's
 * cursor codes, Home its HOME, Enter its NEW LINE and Backspace its RUBOUT.
 * Piped input is taken byte for byte, with a line feed as NEW LINE. */
static int kb_get(void)
{
    int c;
    if (kbq_n) {
        c = kbq[0];
        memmove(kbq, kbq + 1, (size_t)(--kbq_n) * sizeof kbq[0]);
        return c;
    }
    if (kb_console) {
        c = _getch();
        if (c == 0 || c == 0xE0) {
            switch (_getch()) {
            case 72: return 027;              /* up    */
            case 77: return 030;              /* right */
            case 75: return 031;              /* left  */
            case 80: return 032;              /* down  */
            case 71: return 010;              /* home  */
            case 83: return 0177;             /* del   */
            default: return -2;
            }
        }
        if (c == 13) return 012;
        if (c == 8) return 0177;
        return c;
    }
    for (;;) {
        c = fgetc(stdin);
        if (c == EOF) return -1;
        if (c == 13) continue;
        return c == 10 ? 012 : c;
    }
}

static void term_init(void)
{
    void *h;
    unsigned long mode;
    h = GetStdHandle((unsigned long)-11);          /* STD_OUTPUT_HANDLE */
    if (!term_mode) {
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | 0x0004);        /* virtual terminal  */
            term_mode = TM_ANSI;
        } else term_mode = TM_SNAP;
    }
    h = GetStdHandle((unsigned long)-10);          /* STD_INPUT_HANDLE  */
    kb_console = GetConsoleMode(h, &mode) != 0;
    v_clear();
}

static void term_done(void)
{
    static int done;
    if (!term_d200 || done) return;
    done = 1;
    if (term_mode == TM_ANSI) {
        v_dirty = 1; v_show(1);
        printf("\x1b[0m\x1b[%d;1H\n", VR);
    } else if (term_mode == TM_SNAP) {
        v_dirty = 1; v_show(1);
    }
    fflush(stdout);
}

#endif /* D200_H */
