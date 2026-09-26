/* d200.h -- Data General Dasher D200 terminals for the players.
 *
 * QUEST has to be run on a D200 ("set tti d200 / set tto d200"): it draws
 * with the D200's control codes and reads with DG screen-edit reads.  Here
 * each player's @INPUT/@OUTPUT go through a small D200 of their own: the
 * codes the game writes update a virtual 80x24 screen, and the screen is
 * shown one of these ways --
 *
 *   ANSI   on a real console or over a network connection: only the cells
 *          that changed are redrawn, so the display stays right whatever
 *          size the window is.
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
 * Only the players' consoles are translated.  QUP.CLI ran the server with
 * /out=quest.out, so the server's @OUTPUT goes to QUEST.OUT in the save
 * directory and its @INPUT is empty.
 *
 * The terminal a process talks to is T.  The scheduler points T at the
 * running process's own terminal whenever it switches.
 */
#ifndef D200_H
#define D200_H

#include <conio.h>
#include <io.h>
#include <signal.h>
#include <stdarg.h>

/* Win32 calls declared here rather than by including windows.h: winuser.h
 * defines SC_CLOSE and friends, which are this shim's names for AOS/VS
 * system calls. */
__declspec(dllimport) void *__stdcall GetStdHandle(unsigned long);
__declspec(dllimport) int   __stdcall GetConsoleMode(void *, unsigned long *);
__declspec(dllimport) int   __stdcall SetConsoleMode(void *, unsigned long);
__declspec(dllimport) void  __stdcall Sleep(unsigned long);

enum { TM_ANSI = 1, TM_SNAP, TM_RAW };

/* What a terminal is attached to:
 *   STDIO    this program's own stdin/stdout, read by blocking -- one player
 *            at a console, or a scripted session through a pipe
 *   FILE     a scripted extra player (-p): keys from one file, screens to
 *            another
 *   CONSOLE  the player sitting at the host's console, in a multiplayer game
 *   SOCKET   a player who joined over a network connection
 * The last two never block the machine: their keys arrive in a queue. */
enum { TK_NONE = 0, TK_STDIO, TK_FILE, TK_CONSOLE, TK_SOCKET };

static int term_d200;          /* translate the players' consoles          */
static int term_mode_opt;      /* -T                                       */

#define VR 24
#define VC 80
#define AT_DIM 1
#define AT_UND 2
#define AT_BLK 4
#define AT_REV 8

#define MAXTERM 24
#define KQ      4096

typedef struct {
    int   used, kind, mode;
    int   proc;                        /* the process it belongs to, or -1  */
    int   conn;                        /* SOCKET: the connection            */
    FILE *in, *out;                    /* STDIO and FILE                    */
    int   kb_console;                  /* STDIO: stdin is a keyboard        */
    unsigned char vch[VR][VC], vat[VR][VC];   /* the D200's screen          */
    unsigned char rch[VR][VC], rat[VR][VC];   /* what the viewer sees       */
    int   vx, vy, vattr, vroll, d2st, d2col;
    int   dirty, started, done;
    int   gen[8], gen_n;               /* keys the terminal itself sends    */
    unsigned char kq[KQ];              /* keys typed and not yet read       */
    int   kq_head, kq_n;
    int   reading;                     /* a read has begun: cursor placed   */
    char  line[512];                   /* a line read so far                */
    int   line_n;
    int   hungup;                      /* the line has dropped              */
    int   tn, lastcr;                  /* SOCKET: telnet state, CR LF       */
    int   sb_opt, sb_n;                /* SOCKET: a telnet subnegotiation   */
    char  sb[8];
    int   god;                         /* the player here asked for --god   */
    int   esc, escn;                   /* SOCKET: an escape sequence so far */
    char  escb[8];
    unsigned long long esc_at;
    char *ob;                          /* SOCKET: output not yet sent       */
    int   ob_n, ob_cap;
    char *intro;                       /* the title typed first (-c)        */
    long  intro_len, intro_pos;
    unsigned long long intro_t0;
    int   note;                        /* the logon note is on row 23       */
    char  peer[64];
} Term;

static Term  term[MAXTERM];
static Term  term_null;                /* the server's: never shown         */
static Term *T = &term_null;

static int term_interactive(const Term *t)
{ return t->kind == TK_CONSOLE || t->kind == TK_SOCKET; }

static int term_ready(const Term *t)
{ return t->gen_n > 0 || t->kq_n > 0 || t->hungup; }

/* ---- output ----------------------------------------------------------- */
static void t_write(const char *s, int n)
{
    if (n <= 0) return;
    if (T->kind == TK_SOCKET) {
        if (T->ob_n + n > T->ob_cap) {
            int cap = T->ob_cap ? T->ob_cap : 8192;
            char *nb;
            while (cap < T->ob_n + n) cap *= 2;
            nb = (char *)realloc(T->ob, (size_t)cap);
            if (!nb) return;
            T->ob = nb;
            T->ob_cap = cap;
        }
        memcpy(T->ob + T->ob_n, s, (size_t)n);
        T->ob_n += n;
    } else if (T->out) {
        fwrite(s, 1, (size_t)n, T->out);
    }
}

static void t_puts(const char *s) { t_write(s, (int)strlen(s)); }
static void t_putc(int c) { char b = (char)c; t_write(&b, 1); }

static void t_printf(const char *fmt, ...)
{
    char b[160];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (n >= (int)sizeof b) n = (int)sizeof b - 1;
    t_write(b, n);
}

static void t_flush(void)
{
    if (T->kind == TK_SOCKET) {
        if (T->ob_n && net_send(T->conn, T->ob, T->ob_n) < 0) T->hungup = 1;
        T->ob_n = 0;
    } else if (T->out) {
        fflush(T->out);
    }
}

/* ---- the screen ------------------------------------------------------- */
static void v_clear(void)
{ memset(T->vch, ' ', sizeof T->vch); memset(T->vat, 0, sizeof T->vat); }

static void v_newline(void)
{
    T->vx = 0;
    if (T->vy < VR - 1) { T->vy++; return; }
    if (!T->vroll) { T->vy = 0; return; }
    memmove(T->vch, T->vch + 1, (VR - 1) * VC);
    memmove(T->vat, T->vat + 1, (VR - 1) * VC);
    memset(T->vch[VR - 1], ' ', VC);
    memset(T->vat[VR - 1], 0, VC);
}

static void v_put(int c)
{
    T->vch[T->vy][T->vx] = (unsigned char)c;
    T->vat[T->vy][T->vx] = (unsigned char)T->vattr;
    if (++T->vx >= VC) v_newline();
}

static void d2_putc(int c)
{
    c &= 0x7F;
    T->dirty = 1;
    switch (T->d2st) {
    case 1: T->d2col = c; T->d2st = 2; return;
    case 2: T->vx = T->d2col % VC; T->vy = c % VR; T->d2st = 0; return;
    case 3:
        T->d2st = 0;
        if (c == 'D') T->vattr |= AT_REV;
        else if (c == 'E') T->vattr &= ~AT_REV;
        else if (verbose) fprintf(stderr, "   [d200: 036 %03o ignored]\n", c);
        return;
    }
    switch (c) {
    case 000: case 003: case 004: case 021: return;
    case 005:                             /* read cursor address */
        if (T->gen_n + 3 <= (int)(sizeof T->gen / sizeof T->gen[0])) {
            T->gen[T->gen_n++] = 037;
            T->gen[T->gen_n++] = T->vx;
            T->gen[T->gen_n++] = T->vy;
        }
        return;
    case 007: if (T->mode == TM_ANSI) t_putc(7); return;
    case 010: T->vx = T->vy = 0; return;
    case 012: v_newline(); return;
    case 013: memset(T->vch[T->vy] + T->vx, ' ', (size_t)(VC - T->vx));
              memset(T->vat[T->vy] + T->vx, 0, (size_t)(VC - T->vx)); return;
    case 014: v_clear(); T->vx = T->vy = 0; return;
    case 015: T->vx = 0; return;
    case 016: T->vattr |= AT_BLK; return;
    case 017: T->vattr &= ~AT_BLK; return;
    case 020: T->d2st = 1; return;
    case 022: T->vroll = 1; return;
    case 023: T->vroll = 0; return;
    case 024: T->vattr |= AT_UND; return;
    case 025: T->vattr &= ~AT_UND; return;
    case 027: T->vy = (T->vy + VR - 1) % VR; return;
    case 030: if (++T->vx >= VC) { T->vx = 0; T->vy = (T->vy + 1) % VR; } return;
    case 031: if (T->vx > 0) T->vx--;
              else { T->vx = VC - 1; T->vy = (T->vy + VR - 1) % VR; }
              return;
    case 032: T->vy = (T->vy + 1) % VR; return;
    case 034: T->vattr |= AT_DIM; return;
    case 035: T->vattr &= ~AT_DIM; return;
    case 036: T->d2st = 3; return;
    }
    if (c >= 040 && c < 0177) { v_put(c); return; }
    if (verbose) fprintf(stderr, "   [d200: code %03o ignored]\n", c);
}

/* ---- showing it ------------------------------------------------------- */
static void sgr(int a)
{
    t_puts("\x1b[0");
    if (a & AT_DIM) t_puts(";2");
    if (a & AT_UND) t_puts(";4");
    if (a & AT_BLK) t_puts(";5");
    if (a & AT_REV) t_puts(";7");
    t_putc('m');
}

static void v_render_ansi(void)
{
    int r, c, cur = -1;
    if (!T->started) {
        t_puts("\x1b[0m\x1b[2J\x1b[H");
        memset(T->rch, 0, sizeof T->rch);
        memset(T->rat, 0xFF, sizeof T->rat);
        T->started = 1;
    }
    for (r = 0; r < VR; r++) {
        int lo = -1, hi = -1;
        for (c = 0; c < VC; c++)
            if (T->vch[r][c] != T->rch[r][c] || T->vat[r][c] != T->rat[r][c])
                { if (lo < 0) lo = c; hi = c; }
        if (lo < 0) continue;
        t_printf("\x1b[%d;%dH", r + 1, lo + 1);
        for (c = lo; c <= hi; c++) {
            if (T->vat[r][c] != cur) { sgr(T->vat[r][c]); cur = T->vat[r][c]; }
            t_putc(T->vch[r][c]);
            T->rch[r][c] = T->vch[r][c];
            T->rat[r][c] = T->vat[r][c];
        }
    }
    if (cur) t_puts("\x1b[0m");
    t_printf("\x1b[%d;%dH", T->vy + 1, T->vx + 1);
    t_flush();
}

static void v_render_snap(void)
{
    int r, last = -1;
    for (r = 0; r < VR; r++) {
        int c;
        for (c = 0; c < VC; c++) if (T->vch[r][c] != ' ') { last = r; break; }
    }
    t_printf("+--------------------------------------------------------------------------------+ %d,%d\n",
             T->vy, T->vx);
    for (r = 0; r <= last; r++) {
        t_putc('|');
        t_write((const char *)T->vch[r], VC);
        t_puts("|\n");
    }
    t_puts("+--------------------------------------------------------------------------------+\n");
    t_flush();
}

/* `waiting` is true when the game is about to wait for a key: that is the
 * moment a snapshot means something. */
static void v_show(int waiting)
{
    if (T->mode == TM_ANSI) { if (T->dirty) v_render_ansi(); T->dirty = 0; }
    else if (T->mode == TM_SNAP && waiting && T->dirty) { v_render_snap(); T->dirty = 0; }
    else if (T->mode == TM_RAW) t_flush();
}

static void d2_write(const char *b, int n)
{
    int i;
    if (T->mode == TM_RAW) { t_write(b, n); return; }
    for (i = 0; i < n; i++) d2_putc((unsigned char)b[i]);
    v_show(0);
}

/* ---- the keyboard ----------------------------------------------------- *
 * A PC keyboard mapped onto the D200's: the arrow keys send the D200's
 * cursor codes, Home its HOME, Enter its NEW LINE and Backspace its RUBOUT.
 * Piped input is taken byte for byte, with a line feed as NEW LINE.
 *
 * Returns a key, -1 at the end of the input (a hung-up line), -2 for a key
 * the D200 has no equivalent of, and -3 when an interactive terminal has
 * nothing typed yet. */
static int kb_get(void)
{
    int c;
    if (T->gen_n) {
        c = T->gen[0];
        memmove(T->gen, T->gen + 1, (size_t)(--T->gen_n) * sizeof T->gen[0]);
        return c;
    }
    if (term_interactive(T)) {
        if (T->kq_n) {
            c = T->kq[T->kq_head];
            T->kq_head = (T->kq_head + 1) % KQ;
            T->kq_n--;
            return c;
        }
        return T->hungup ? -1 : -3;
    }
    if (T->kind == TK_STDIO && T->kb_console) {
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
    if (!T->in) return -1;
    for (;;) {
        c = fgetc(T->in);
        if (c == EOF) return -1;
        if (c == 13) continue;
        return c == 10 ? 012 : c;
    }
}

/* A key for an interactive terminal's queue. */
static void term_key(Term *t, int c)
{
    if (c < 0 || c > 0377) return;
    if (t->kq_n >= KQ) return;
    t->kq[(t->kq_head + t->kq_n) % KQ] = (unsigned char)c;
    t->kq_n++;
}

/* A key from the host's console (see con_poll). */
static int console_key(int k)
{
    switch (k) {
    case NK_UP:    return 027;
    case NK_RIGHT: return 030;
    case NK_LEFT:  return 031;
    case NK_DOWN:  return 032;
    case NK_HOME:  return 010;
    case NK_DEL:   return 0177;
    case 13:       return 012;
    case 8:        return 0177;
    }
    return k < 0200 ? k : -1;
}

/* A byte from a network connection.  Telnet clients negotiate (IAC ...),
 * send Enter as CR LF or CR NUL, and send the arrow keys as ANSI escape
 * sequences; the join client (net_join) sends the same.  A lone ESC is
 * QUEST's own "leave the game" key, so an ESC that nothing follows within a
 * moment is passed on as a key -- see term_esc_due.
 *
 * `quest --join --god` asks for god mode with a subnegotiation no telnet
 * client uses: IAC SB 198 "GOD" IAC SE. */
#define TELOPT_QUEST 198

static void sock_byte(Term *t, unsigned char b)
{
    switch (t->tn) {
    case 1: t->tn = (b >= 251 && b <= 254) ? 2 : (b == 250) ? 3 : 0; return;
    case 2: t->tn = 0; return;
    case 3: t->sb_opt = b; t->sb_n = 0; t->tn = 5; return;     /* SB option  */
    case 5: if (b == 255) t->tn = 4;                              /* SB data    */
            else if (t->sb_n < (int)sizeof t->sb) t->sb[t->sb_n++] = (char)b;
            return;
    case 4: if (b == 240) {                                       /* IAC SE     */
                t->tn = 0;
                if (t->sb_opt == TELOPT_QUEST && t->sb_n == 3 && !memcmp(t->sb, "GOD", 3))
                    t->god = 1;
            } else t->tn = 5;
            return;
    }
    if (b == 255) { t->tn = 1; return; }

    if (t->esc == 1) {
        if (b == '[' || b == 'O') { t->esc = 2; t->escn = 0; return; }
        t->esc = 0;
        term_key(t, 033);
    } else if (t->esc == 2) {
        if ((b >= '0' && b <= '9') || b == ';') {
            if (t->escn < (int)sizeof t->escb - 1) t->escb[t->escn++] = (char)b;
            return;
        }
        t->esc = 0;
        t->escb[t->escn] = 0;
        switch (b) {
        case 'A': term_key(t, 027); break;
        case 'B': term_key(t, 032); break;
        case 'C': term_key(t, 030); break;
        case 'D': term_key(t, 031); break;
        case 'H': term_key(t, 010); break;
        case '~':
            if (!strcmp(t->escb, "1") || !strcmp(t->escb, "7")) term_key(t, 010);
            else if (!strcmp(t->escb, "3")) term_key(t, 0177);
            break;
        }
        return;
    }
    if (b == 033) { t->esc = 1; t->esc_at = net_now(); return; }
    if (b == 015) { t->lastcr = 1; term_key(t, 012); return; }
    if ((b == 012 || b == 0) && t->lastcr) { t->lastcr = 0; return; }
    t->lastcr = 0;
    if (b == 012) { term_key(t, 012); return; }
    if (b == 010 || b == 0177) { term_key(t, 0177); return; }
    if (b < 0200) term_key(t, b);
}

static void term_esc_due(Term *t, unsigned long long now)
{
    if (t->esc == 1 && now - t->esc_at >= 60) { t->esc = 0; term_key(t, 033); }
}

/* ---- terminals come and go ---------------------------------------------- */
static int term_new(int kind)
{
    int k;
    for (k = 1; k < MAXTERM; k++) if (!term[k].used) break;
    if (k >= MAXTERM) return -1;
    memset(&term[k], 0, sizeof term[k]);
    term[k].used = 1;
    term[k].kind = kind;
    term[k].mode = TM_ANSI;
    term[k].proc = -1;
    term[k].conn = -1;
    term[k].vroll = 1;
    memset(term[k].vch, ' ', sizeof term[k].vch);
    return k;
}

static void term_free(Term *t)
{
    free(t->ob);
    free(t->intro);
    if (t->kind == TK_FILE) {
        if (t->in) fclose(t->in);
        if (t->out) fclose(t->out);
    }
    memset(t, 0, sizeof *t);
}

/* term[0] is this program's own stdin and stdout, for a single player at a
 * console or a scripted session. */
static void term_init(void)
{
    Term *t = &term[0];
    void *h;
    unsigned long mode;
    memset(t, 0, sizeof *t);
    t->used = 1;
    t->kind = TK_STDIO;
    t->proc = -1;
    t->conn = -1;
    t->in = stdin;
    t->out = stdout;
    t->vroll = 1;
    h = GetStdHandle((unsigned long)-11);          /* STD_OUTPUT_HANDLE */
    if (term_mode_opt) t->mode = term_mode_opt;
    else if (GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | 0x0004);            /* virtual terminal  */
        t->mode = TM_ANSI;
    } else t->mode = TM_SNAP;
    h = GetStdHandle((unsigned long)-10);          /* STD_INPUT_HANDLE  */
    t->kb_console = GetConsoleMode(h, &mode) != 0;
    memset(t->vch, ' ', sizeof t->vch);
}

static void term_done(void)
{
    if (!term_d200 || T->done || !T->used) return;
    T->done = 1;
    if (T->mode == TM_ANSI) {
        T->dirty = 1; v_show(1);
        t_printf("\x1b[0m\x1b[%d;1H\n", VR);
    } else if (T->mode == TM_SNAP) {
        T->dirty = 1; v_show(1);
    }
    t_flush();
}

#endif /* D200_H */
