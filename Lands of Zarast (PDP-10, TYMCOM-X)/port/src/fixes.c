/* fixes.c -- optional repairs to VENTUR ("Land of Fred"), and a debug mode.
 *
 * Nothing here changes the tape images.  The programs still run exactly as
 * compiled; this module only watches for the start of BASIC statements.
 * Every TYMBASIC statement begins with JSP 16,<statement routine> followed by
 * a word holding its line number, so when the processor arrives at that
 * routine the line about to run is M[RH(AC16)].  For a handful of line
 * numbers the hooks below read or write the program's own variables, or send
 * control to another line, exactly as a GOTO would.
 *
 * Two independent switches, both off by default so the game is the one on
 * the tape:
 *
 *   --fix    repairs the bugs players hit in VENTUR (see fix_* below and
 *            docs/FIXES.md).  The 1984 set is the only one with VENTUR.
 *
 *   --debug  god-mode commands typed at any prompt, starting with '#':
 *            #GOD toggles invulnerability, #STATS sets maximum stats,
 *            #GOLD sets the largest safe amount of gold, #HELP lists them.
 *            Works in VENTUR and in the dungeon game of both versions.
 *
 * Every address below was read out of the images with the disassembly tools
 * in work/, and fixes_image_loaded() checks a sample of them against the
 * loaded image before switching anything on.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include "pdp10.h"

int opt_fix = 0;
int opt_debug = 0;
int fix_hook_pc = -1;             /* statement routine while hooks are live */
int fix_restore_pending = 0;

extern const char *opt_dir;
extern int opt_echo_input;
void monitor_print(const char *s);
void monitor_reprint_line(void);
void monitor_discard_input(void);
int  monitor_save_channels(FILE *f);
int  monitor_restore_channels(FILE *f);

/* ------------------------------------------------------------------ */
/* the running image                                                    */
/* ------------------------------------------------------------------ */
enum { PROG_OTHER, PROG_VENTUR, PROG_DUNGEON };
static int prog = PROG_OTHER;
static int fixes_on = 0;          /* --fix accepted for this image          */
static int debug_on = 0;          /* --debug accepted for this image        */

static int  nstmt;
static int *stmt_addr, *stmt_line;
static int  last_line;            /* line of the most recent statement      */
static int  in_guest;             /* inside guest_call()                    */

/* Find the statement routine and index every statement by line number. */
static void index_statements(void)
{
    int top = (int)LH(M[0115]), a, best = -1, bestn = 0, i;
    int cand[64], candn[64], nc = 0;
    if (top <= 0400000 || top >= MEMTOP) top = MEMTOP - 1;
    for (a = 0400000; a < top; a++) {
        w36 x = M[a];
        if ((x >> 18) == 0265700 && M[a + 1] > 0 && M[a + 1] < 100000) {
            int t = (int)RH(x);
            for (i = 0; i < nc && cand[i] != t; i++) ;
            if (i == nc) { if (nc == 64) continue; cand[nc] = t; candn[nc++] = 0; }
            candn[i]++;
        }
    }
    for (i = 0; i < nc; i++) if (candn[i] > bestn) { bestn = candn[i]; best = cand[i]; }
    free(stmt_addr); free(stmt_line);
    stmt_addr = stmt_line = NULL; nstmt = 0;
    if (best < 0) return;
    stmt_addr = (int *)malloc(sizeof(int) * (size_t)bestn);
    stmt_line = (int *)malloc(sizeof(int) * (size_t)bestn);
    if (!stmt_addr || !stmt_line) fatal("out of memory indexing statements");
    for (a = 0400000; a < top && nstmt < bestn; a++)
        if (M[a] == (XWD(0265700, best)) && M[a + 1] > 0 && M[a + 1] < 100000) {
            stmt_addr[nstmt] = a;
            stmt_line[nstmt++] = (int)M[a + 1];
        }
    fix_hook_pc = best;
}

/* Address of the statement that begins line n, or -1. */
static int line_addr(int n)
{
    int lo = 0, hi = nstmt - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (stmt_line[mid] == n) return stmt_addr[mid];
        if (stmt_line[mid] < n) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

/* Continue at line n instead of the statement about to run. */
static void goto_line(int n)
{
    int a = line_addr(n);
    if (a < 0) fatal("fixes: no line %d in this program", n);
    PC = a;
}

/* Skip the statement about to run. */
static void next_statement(void)
{
    int lo = 0, hi = nstmt - 1, here = (int)RH(AC(016)) - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (stmt_addr[mid] == here) { if (mid + 1 < nstmt) PC = stmt_addr[mid + 1]; return; }
        if (stmt_addr[mid] < here) lo = mid + 1; else hi = mid - 1;
    }
}

/* Does some word of line n's code equal w? */
static int line_has(int n, w36 w)
{
    int a = line_addr(n), i;
    if (a < 0) return 0;
    for (i = 2; i < 200 && M[a + i] != XWD(0265700, fix_hook_pc); i++)
        if (M[a + i] == w) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* numbers: TYMBASIC keeps every numeric variable as a single float     */
/* ------------------------------------------------------------------ */
static double f2d(w36 x)
{
    int neg = (x & SIGNBIT) != 0, e;
    double f;
    if (neg) x = (~x + 1) & WMASK;
    if (!x) return 0.0;
    e = (int)((x >> 27) & 0377);
    f = (double)(x & 0777777777) / 134217728.0;
    return ldexp(neg ? -f : f, e - 128);
}

static w36 d2f(double v)
{
    int neg = v < 0, e;
    double m;
    w36 f, w;
    if (v == 0) return 0;
    if (neg) v = -v;
    m = frexp(v, &e);
    f = (w36)llround(m * 134217728.0);
    if (f >= 01000000000) { f >>= 1; e++; }
    w = ((w36)(e + 128) << 27) | f;
    return neg ? ((~w + 1) & WMASK) : w;
}

static double getv(int a)            { return a < 0 ? 0.0 : f2d(M[a]); }
static void   setv(int a, double v)  { if (a >= 0) M[a] = d2f(v); }
static int    same(double a, double b) { return fabs(a - b) < 0.5; }

/* Element address of a one-dimensional numeric array, reached through the
 * little access routine the compiler builds for it in low core:
 *
 *   thunk-5  descriptor, right half = where the elements are
 *   thunk    HRREI 0,<lower bound>
 *   thunk+1  HRREI 3,<upper bound>
 */
static int arr(int thunk, int i)
{
    w36 lo = M[thunk], hi = M[thunk + 1];
    int base = (int)RH(M[thunk - 5]), l, h;
    if (LH(lo) != 0571000 || LH(hi) != 0571140 || !base) return -1;
    l = sx18(RH(lo)); h = sx18(RH(hi));
    if (i < l || i > h) return -1;
    return base + i - l;
}
static int    upper(int thunk)                  { return sx18(RH(M[thunk + 1])); }
static double ga(int thunk, int i)              { return getv(arr(thunk, i)); }
static void   sa(int thunk, int i, double v)    { setv(arr(thunk, i), v); }

/* Call a runtime routine the way compiled code does: PUSHJ 17,addr with
 * arguments in AC1, AC2 and AC5.  Returns AC1.  The statement is not
 * disturbed: every AC and the PC are put back afterwards. */
#define SENTINEL 0777700
static w36 guest_call(int addr, w36 a1, w36 a2, w36 a5)
{
    w36 save[16], r;
    int pc = PC, flags = FLAGS, i;
    long n = 0;
    for (i = 0; i < 16; i++) save[i] = M[i];
    AC(1) = a1; AC(2) = a2; AC(5) = a5;
    AC(017) = (AC(017) + XWD(1, 1)) & WMASK;
    M[RH(AC(017))] = XWD(FLAGS, SENTINEL);
    PC = addr;
    in_guest++;
    while (PC != SENTINEL && !halted && n++ < 2000000) cpu_step();
    in_guest--;
    r = AC(1);
    for (i = 0; i < 16; i++) M[i] = save[i];
    PC = pc; FLAGS = flags;
    return r;
}

/* Print a string literal out of the image (descriptor address d). */
static void print_literal(int d)
{
    char buf[256];
    int len = (int)RH(M[d + 1]), i, a = d + 2;
    if (len < 0 || len > 250) return;
    for (i = 0; i < len; i++)
        buf[i] = (char)((M[a + i / 5] >> (29 - 7 * (i % 5))) & 0177);
    buf[len] = 0;
    monitor_print(buf);
    monitor_print("\n");
}

static void printf_game(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    monitor_print(buf);
}

/* The largest amount the programs can hold and still show and store as a
 * whole number.  TYMBASIC prints seven significant digits before it goes
 * to E-notation (9999999 prints as 9999999, 16777215 as .1677722E+08), and
 * DUNGEN writes gold and abilities to the character file through a
 * seven-digit "DDDDDDD" field.  9,999,999 satisfies both, and a
 * single-precision float holds it exactly. */
#define MAX_SAFE 9999999.0

/* ================================================================== */
/* VENTUR                                                              */
/* ================================================================== */

/* scalars */
#define V_LAMPCNT 016256          /* moves made with the lamp on            */
#define V_TURNS   016257
#define V_GROWTH  016260          /* strength still to be regained          */
#define V_KILLS   016267
#define V_STR     016270          /* strength, which is also hit points     */
#define V_NCARRY  016271          /* objects carried, plus one              */
#define V_LAMP    016274          /* LAMP$: "OFF", "ON" or "DEAD"           */
#define V_GOLD    016277
#define V_P       016300
#define V_PP      016301
#define V_OILCNT  016303          /* moves since the oil was lit            */
#define V_OILST   016304          /* 0, -1 lit, -2 too late, -3 thrown      */
#define V_VNUM    016314
#define V_NNUM    016316
#define V_KILLED  016350
#define V_DAMBONUS 016352         /* added to this attack's damage          */
#define V_HITBONUS 016353         /* added to this attack's to-hit roll     */
#define V_INUM    016317          /* the weapon: "KILL x WITH <this>"       */
#define V_CRIT    016330          /* which critical hit, 1 to 3             */
/* arrays, by access routine */
#define VT_NLOC   015275          /* where each noun is; -2 carried         */
#define VT_KNAMES 015315          /* names of the monsters killed           */
#define VT_HP     015435
#define VT_HPMAX  015455
#define VT_VCODE  016125
/* string literals and runtime routines */
#define VS_OFF    0442701
#define VS_ON     0442752
#define VS_DEAD   0442717
#define VS_ROOM11A 0443223        /* "YOU ARE IN A 30 FOOT BY 30 FOOT ROOM." */
#define VS_ROOM11B 0443235        /* "THERE ARE FOUR DOORS,ONE ON EACH WALL." */
#define VS_TOOLATE 0451332        /* "YOU DID NOT THROW THE OIL FLASK IN TIME!" */
#define VR_STREQ  0462527
#define VR_STRSET 0471653
/* nouns */
#define N_TORCH 11
#define N_LAMP  12
#define N_OIL   13
#define N_TITAN 89
#define N_MACE  90
#define N_AXE   91
#define CARRIED (-2.0)
#define IN_STOCK (-1.0)
#define STORE_ROOM 1
#define TORCH_LIFE 100            /* the lamp lasts 255 moves               */

/* State the fixes keep beside the program's own.  It goes into the save
 * file with the core image. */
static struct {
    int torch_lit, torch_moves;
    int throwing;
    double thrown;
    int save_pending;
    int extra_kills, skip_name, showing_total;
} vx;

static int  god = 0;              /* invulnerable                           */
static double god_str;            /* VENTUR strength to hold                */
static double god_level[7], god_s[7];

static int v_streq(int var, int lit) { return guest_call(VR_STREQ, (w36)var, (w36)lit, 0) != 0; }
static void v_strset(int var, int lit) { guest_call(VR_STRSET, (w36)lit, (w36)var, 0); }

static int v_lit(void)
{
    double p = getv(V_P), l = ga(VT_NLOC, N_LAMP), t = ga(VT_NLOC, N_TORCH);
    int lamp = (same(l, CARRIED) || same(l, p)) && v_streq(V_LAMP, VS_ON);
    int torch = vx.torch_lit && (same(t, CARRIED) || same(t, p));
    return lamp || torch;
}
static int v_dungeon(void) { double p = getv(V_P); return p > 10.5 && p < 60.5; }

/* An object leaves the player's hands for good. */
static void v_uncarry(int noun)
{
    if (same(ga(VT_NLOC, noun), CARRIED)) setv(V_NCARRY, getv(V_NCARRY) - 1);
}

static void v_save(void);
static int  v_restore(void);

static void ventur_fix(int line)
{
    switch (line) {

    /* Play again starts over at line 380, past the lines that zero the
     * lamp counter, the turn count and the strength still to be regained,
     * and nothing ever resets the burning-oil fuse. */
    case 380:
        setv(V_LAMPCNT, 0); setv(V_TURNS, 0); setv(V_GROWTH, 0);
        setv(V_OILCNT, 0); setv(V_OILST, 0);
        memset(&vx, 0, sizeof vx);
        break;

    /* The hit-point DATA has one value too many in the monsters and one
     * too few in the objects after them: the titan reads 27, the mace --
     * the first object -- reads the titan's 55, and anything with two or
     * more hit points is a monster, so a dropped mace wandered and
     * attacked.  Every other per-noun column lines up, and the shift can
     * start no earlier than noun 81 (the three trolls' 26/32/41 rise with
     * their ratings and damage, the mezzodemon's 49 matches its kind), so
     * the least change that fits is the titan's own slot.
     *
     * Line 1740 runs once the DATA is read, and again after RESTORE.  The
     * maxima are never written by the game, so they say whether this has
     * been done; a titan already wounded keeps its wounds. */
    case 1740:
        if (same(ga(VT_HPMAX, N_TITAN), 27) && same(ga(VT_HPMAX, N_MACE), 55)) {
            sa(VT_HP, N_TITAN, ga(VT_HP, N_TITAN) + 28);
            sa(VT_HPMAX, N_TITAN, 55);
        }
        sa(VT_HP, N_MACE, 1); sa(VT_HPMAX, N_MACE, 1);
        break;

    /* "YOUR LAMP IS DEAD." is printed whenever the counter is 255, and a
     * dead lamp no longer counts, so it said so every turn forever. */
    case 1860:
        if (same(getv(V_LAMPCNT), 255)) setv(V_LAMPCNT, 256);
        break;

    /* A lit torch burns down as the lamp does, only faster. */
    case 2120:
        if (vx.torch_lit && !same(getv(V_P), getv(V_PP)) && ++vx.torch_moves >= TORCH_LIFE) {
            double t = ga(VT_NLOC, N_TORCH);
            if (same(t, CARRIED) || same(t, getv(V_P)))
                monitor_print("YOUR TORCH HAS BURNED OUT.\n");
            v_uncarry(N_TORCH);
            sa(VT_NLOC, N_TORCH, IN_STOCK);        /* the store has another */
            vx.torch_lit = vx.torch_moves = 0;
        }
        break;

    /* Darkness.  The game called a room dark only when the lamp was off
     * AND not carried AND not here, so an unlit lamp in your pack lit the
     * dungeon, OFF saved nothing, and a torch could never matter. */
    case 2260:
        if (v_dungeon() && !v_lit()) goto_line(4580);
        else goto_line(2280);
        break;

    /* Room 11 described itself only if the lamp was carried and on, and
     * then line 2580 repeated the old darkness test. */
    case 2540:
        if (same(getv(V_P), 11)) { print_literal(VS_ROOM11A); print_literal(VS_ROOM11B); }
        goto_line(2600);
        break;

    /* Objects and monsters are listed in the dungeon only if there is
     * light -- which used to mean the lamp, lit, carried or here. */
    case 4640:
        if (!v_dungeon()) goto_line(4680);
        else if (v_lit()) PC = line_addr(4700) - 1;   /* JSP 1, list objects */
        else goto_line(4700);
        break;

    /* SAVE had no code at all.  It is written at the next prompt, so the
     * saved game is one waiting for a command. */
    case 4820:
        if (vx.save_pending) { vx.save_pending = 0; v_save(); }
        break;
    case 6620: {
        int v = (int)getv(V_VNUM);
        if (v > 0 && same(ga(VT_VCODE, v), 25)) vx.save_pending = 1;
        break; }

    /* Buying.  There is one of each object, so a used-up one has to go
     * back into stock before it can be sold again, and the store would
     * take your money a second time for something already on its floor. */
    case 12440: {
        int n = (int)getv(V_NNUM);
        if (n == N_LAMP && v_streq(V_LAMP, VS_DEAD)) {
            if (same(ga(VT_NLOC, N_LAMP), CARRIED))
                monitor_print("THE MERCHANT TAKES YOUR DEAD LAMP IN TRADE.\n");
            v_uncarry(N_LAMP);
            sa(VT_NLOC, N_LAMP, IN_STOCK);
            v_strset(V_LAMP, VS_OFF);
            setv(V_LAMPCNT, 0);
        }
        if (n >= 6 && n <= 13 && same(ga(VT_NLOC, n), getv(V_P))) {
            monitor_print("YOU ALREADY BOUGHT THAT. IT IS RIGHT HERE.\n");
            goto_line(12620);
        }
        break; }

    /* LIGHT only knew the lamp. */
    case 13320:
        if ((int)getv(V_NNUM) == N_TORCH) {
            if (!same(ga(VT_NLOC, N_TORCH), CARRIED))
                { monitor_print("BUT YOU AREN'T CARRYING IT!\n"); goto_line(13500); }
            else if (vx.torch_lit)
                { monitor_print("IT IS ALREADY LIT.\n"); goto_line(13500); }
            else { vx.torch_lit = 1; goto_line(13460); }   /* "OK." and a look */
        }
        break;

    /* OFF turned the lamp off only while it lay on the floor. */
    case 13600: {
        int n = (int)getv(V_NNUM);
        double p = getv(V_P);
        if (n == N_TORCH) {
            double t = ga(VT_NLOC, N_TORCH);
            if (vx.torch_lit && (same(t, CARRIED) || same(t, p)))
                { vx.torch_lit = 0; monitor_print("THE TORCH IS NOW OUT.\n"); }
            else monitor_print("THE TORCH ISN'T LIT.\n");
            goto_line(13680);
        } else {
            double l = ga(VT_NLOC, N_LAMP);
            if (same(l, CARRIED) || same(l, p)) goto_line(13640);
        }
        break; }

    /* The kill list holds 84 names, and the 85th kill stopped the game
     * with "Array subscript out of bounds in line 21340".  Count on past
     * 84, keep the first 84 names. */
    /* The axe was meant to add 5 to damage and 5 to the to-hit roll, but
     * line 20700 reads DAMBONUS=DAMBONUS+5 AND HITBONUS=HITBONUS+5, which
     * TYMBASIC compiles as one assignment of a comparison: the damage bonus
     * became 0 and the to-hit bonus was never touched. */
    case 20700:
        if ((int)getv(V_INUM) == N_AXE) {
            setv(V_DAMBONUS, getv(V_DAMBONUS) + 5);
            setv(V_HITBONUS, getv(V_HITBONUS) + 5);
            next_statement();
        }
        break;

    /* A natural 20 picks one of three critical hits -- split in two,
     * head chopped off, or "YOU GOT A GOOD HIT!" for 1-30 extra damage --
     * with ON A GOTO, but line 22140 rolls A=INT(RND*2+1), which is never
     * 3.  Every critical killed outright and the good hit never happened. */
    case 22160:
        setv(V_CRIT, 1 + rand() % 3);
        break;

    case 21140:
        if (same(getv(V_KILLED), -1)) {
            /* Monsters never carried money, and there was nothing else to
             * sell or find, so the only gold was what you started with. */
            int n = (int)getv(V_NNUM), top = (int)ga(VT_HPMAX, n), g;
            if (top < 1) top = 1;
            g = 1 + rand() % top;
            setv(V_GOLD, fmin(MAX_SAFE, getv(V_GOLD) + g));
            printf_game("IT WAS CARRYING %d GOLD PIECES.\n", g);
        }
        break;
    case 21160: {
        int cap = upper(VT_KNAMES);
        double k = getv(V_KILLS);
        if (k > cap + 0.5) { vx.extra_kills += (int)(k - cap); setv(V_KILLS, cap); vx.skip_name = 1; }
        break; }
    case 21320: case 21340:
        if (vx.skip_name) next_statement();
        break;
    case 21360:
        vx.skip_name = 0;
        break;
    case 17860:
        if (vx.extra_kills) { setv(V_KILLS, getv(V_KILLS) + vx.extra_kills); vx.showing_total = 1; }
        break;
    case 17880:
        if (vx.showing_total) { setv(V_KILLS, getv(V_KILLS) - vx.extra_kills); vx.showing_total = 0; }
        break;

    /* The fuse counted from wherever the last flask had left it. */
    case 23100:
        if (!same(getv(V_OILST), -1)) setv(V_OILCNT, 0);
        break;

    /* Throwing.  The fire loop reuses the "object thrown" variable for
     * each victim, so afterwards the thrown object was the last monster
     * killed: it was put back in the room, and the flask never left your
     * hands.  Nor did anything thrown ever free the space it took. */
    case 26087:
        vx.throwing = 1; vx.thrown = getv(V_NNUM);
        break;
    case 26090:
        if (vx.throwing) {
            vx.throwing = 0;
            setv(V_NNUM, vx.thrown);
            v_uncarry((int)vx.thrown);
        }
        break;
    case 26210:
        if (same(getv(V_OILST), -3)) { setv(V_OILST, 0); setv(V_OILCNT, 0); }
        break;
    }
}

/* Debug mode never lets a number grow past what prints and saves whole.
 * Without this, blows shrugged off under #GOD still feed VENTUR's pool of
 * strength to regain, and #STATS strength creeps into E-notation. */
static void ventur_debug(void)
{
    if (getv(V_STR) > MAX_SAFE) setv(V_STR, MAX_SAFE);
    if (getv(V_GOLD) > MAX_SAFE) setv(V_GOLD, MAX_SAFE);
}

static void ventur_god(int line, int redirected)
{
    double s = getv(V_STR);
    if (s > god_str) god_str = s;
    else if (s < god_str) setv(V_STR, god_str);

    /* The one death that is not a matter of strength: the lit flask. */
    if (line == 25040 && !redirected) {
        print_literal(VS_TOOLATE);
        monitor_print("[DEBUG] IT BURNS OUT HARMLESSLY IN YOUR HAND.\n");
        v_uncarry(N_OIL);
        sa(VT_NLOC, N_OIL, IN_STOCK);
        setv(V_OILST, 0); setv(V_OILCNT, 0);
        goto_line(25500);
    }
}

/* ------------------------------------------------------------------ */
/* VENTUR's saved game: the low segment, the processor flags, the open  */
/* channels and the fixes' own state.  Restoring resumes at line 1740,  */
/* which describes where you are and returns to the command loop.       */
/* ------------------------------------------------------------------ */
static const char SAVE_MAGIC[16] = "ZARAST VENTUR 1";

static void save_path(char *buf, size_t n) { snprintf(buf, n, "%s/ventur.sav", opt_dir); }

static void v_save(void)
{
    char path[512];
    FILE *f;
    unsigned low = RH(M[044]) + 1, i;
    unsigned char b[8];
    save_path(path, sizeof path);
    f = fopen(path, "wb");
    if (!f) { monitor_print("SORRY, THE GAME COULD NOT BE SAVED.\n"); return; }
    fwrite(SAVE_MAGIC, 1, sizeof SAVE_MAGIC, f);
    fwrite(&low, sizeof low, 1, f);
    fwrite(&FLAGS, sizeof FLAGS, 1, f);
    for (i = 0; i < low; i++) {
        w36 w = M[i];
        int k;
        for (k = 0; k < 8; k++) b[k] = (unsigned char)(w >> (8 * k));
        fwrite(b, 1, 8, f);
    }
    fwrite(&vx, sizeof vx, 1, f);
    monitor_save_channels(f);
    if (fclose(f) != 0) { monitor_print("SORRY, THE GAME COULD NOT BE SAVED.\n"); return; }
    monitor_print("GAME SAVED.\n");
}

static unsigned char *restore_buf;
static long restore_len;

/* Read and check the file now; apply it after the current instruction. */
static int v_restore(void)
{
    char path[512];
    FILE *f;
    long n;
    unsigned low;
    save_path(path, sizeof path);
    f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    free(restore_buf);
    restore_buf = (unsigned char *)malloc((size_t)n + 1);
    if (!restore_buf || fread(restore_buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); return 0; }
    fclose(f);
    restore_len = n;
    if (n < (long)(sizeof SAVE_MAGIC + sizeof low) || memcmp(restore_buf, SAVE_MAGIC, sizeof SAVE_MAGIC)) return -1;
    memcpy(&low, restore_buf + sizeof SAVE_MAGIC, sizeof low);
    if (low == 0 || low > 0400000 ||
        n < (long)(sizeof SAVE_MAGIC + sizeof low + sizeof FLAGS + 8L * low + sizeof vx)) return -1;
    fix_restore_pending = 1;
    return 1;
}

void fixes_do_restore(void)
{
    unsigned low, i;
    long off = sizeof SAVE_MAGIC;
    FILE *mf;
    fix_restore_pending = 0;
    memcpy(&low, restore_buf + off, sizeof low); off += sizeof low;
    memcpy(&FLAGS, restore_buf + off, sizeof FLAGS); off += sizeof FLAGS;
    for (i = 0; i < low; i++, off += 8) {
        w36 w = 0;
        int k;
        for (k = 0; k < 8; k++) w |= (w36)restore_buf[off + k] << (8 * k);
        M[i] = w & WMASK;
    }
    memcpy(&vx, restore_buf + off, sizeof vx); off += sizeof vx;
    mf = tmpfile();
    if (mf) {
        fwrite(restore_buf + off, 1, (size_t)(restore_len - off), mf);
        rewind(mf);
        monitor_restore_channels(mf);
        fclose(mf);
    }
    free(restore_buf); restore_buf = NULL;
    monitor_discard_input();
    vx.save_pending = 0;
    god_str = getv(V_STR);
    goto_line(1740);
}

/* ================================================================== */
/* the dungeon game: DUNGEN (1984), PUB and B (1987)                   */
/* The three share every address used here.                           */
/* ================================================================== */
#define D_CNUM2   020136
#define D_DEAD    020206
#define DT_NHITS  015401
#define DT_CLOC   016161
#define DT_HP     016221
#define DT_HPTOT  016241
#define DT_S      016261
#define DT_D      016301
#define DT_C      016321
#define DT_II     016341
#define DT_W      016361
#define DT_CH     016401
#define DT_GOLD   016441
#define DT_LEVEL  017101
#define DT_MANA   017161
#define DT_MANATOT 017201
#define ABILITY_MAX 18            /* CHARC rolls 4-18 and rejects above 18 */

static int d_in_play(int i) { return ga(DT_CLOC, i) > 0.5; }

static void dungeon_debug(void)
{
    int i;
    for (i = 1; i <= 6; i++)
        if (d_in_play(i) && ga(DT_GOLD, i) > MAX_SAFE) sa(DT_GOLD, i, MAX_SAFE);
}

static void dungeon_god(int line)
{
    int i;
    for (i = 1; i <= 6; i++) {
        double hp, v;
        if (!d_in_play(i)) { god_level[i] = god_s[i] = 0; continue; }
        hp = ga(DT_HPTOT, i);
        if (ga(DT_NHITS, i) < hp) sa(DT_NHITS, i, hp);
        if (ga(DT_HP, i) < hp) sa(DT_HP, i, hp);
        v = ga(DT_LEVEL, i);
        if (v > god_level[i]) god_level[i] = v; else if (v < god_level[i]) sa(DT_LEVEL, i, god_level[i]);
        v = ga(DT_S, i);
        if (v > 0.5) god_s[i] = v;
    }
    /* Poison and a few other deaths decide on the spot and zero strength
     * without touching hit points.  They all end in QDONE with S at 0. */
    if (line == 31380) {
        int c = (int)getv(D_CNUM2);
        if (c >= 1 && c <= 6 && ga(DT_S, c) < 0.5) {
            sa(DT_S, c, god_s[c] > 0.5 ? god_s[c] : ABILITY_MAX);
            sa(DT_NHITS, c, ga(DT_HPTOT, c));
            sa(DT_HP, c, ga(DT_HPTOT, c));
            setv(D_DEAD, 0);
            monitor_print("[DEBUG] DEATH PREVENTED.\n");
            goto_line(32030);                       /* ENDF QDONE */
        }
    }
}

/* ================================================================== */
/* entry points                                                        */
/* ================================================================== */

/* Called by cpu_step() when the processor reaches the statement routine. */
void fixes_statement(void)
{
    int line;
    if (in_guest) return;
    line = (int)M[RH(AC(016))];
    last_line = line;
    if (prog == PROG_VENTUR) {
        if (fixes_on) ventur_fix(line);
        if (debug_on) ventur_debug();
        if (debug_on && god) ventur_god(line, PC != fix_hook_pc);
    } else if (prog == PROG_DUNGEON) {
        if (debug_on) dungeon_debug();
        if (debug_on && god) dungeon_god(line);
    }
}

void fixes_image_loaded(const char *img)
{
    fix_hook_pc = -1;
    prog = PROG_OTHER;
    fixes_on = debug_on = 0;
    last_line = 0;
    if (!opt_fix && !opt_debug) return;
    if (!strcmp(img, "ventur")) prog = PROG_VENTUR;
    else if (!strcmp(img, "dungen") || !strcmp(img, "pub") || !strcmp(img, "b")) prog = PROG_DUNGEON;
    else return;

    index_statements();
    if (fix_hook_pc < 0) { prog = PROG_OTHER; return; }

    /* Check the image is the one these addresses came from. */
    if (prog == PROG_VENTUR) {
        int ok = line_has(4820, 0201100445530ULL)          /* PRINT "]"          */
              && line_has(13440, 0201040442752ULL)         /* LAMP$="ON"         */
              && line_has(2120, 0201040016274ULL)          /* LAMP$ counter test */
              && line_has(26095, 0260740015275ULL)         /* NLOC(NNUM)=P       */
              && line_has(21340, 0260740015315ULL)         /* kill list          */
              && line_has(20700, 0312440451157ULL)         /* IF INUM=91 (axe)   */
              && line_has(22160, 0307440000003ULL)         /* ON A GOTO, 3 ways  */
              && line_addr(25500) > 0 && line_addr(13680) > 0
              && M[line_addr(4700) - 1] == XWD(0265040, 0426151);   /* GOSUB list objects */
        if (!ok) { fprintf(stderr, "[fixes: this VENTUR is not the one they were made for; left off]\n");
                   prog = PROG_OTHER; fix_hook_pc = -1; return; }
    } else {
        int ok = line_has(36400, 0260740016261ULL) && line_has(36400, 0260740016441ULL)
              && line_has(32130, 0260740015401ULL) && line_has(32120, 0260740016161ULL)
              && line_has(36480, 0260740017101ULL) && line_has(36500, 0260740017201ULL)
              && line_has(32030, 0254020017574ULL) && line_addr(31380) > 0;
        if (!ok) { fprintf(stderr, "[debug: this dungeon image is not the one the addresses came from; left off]\n");
                   prog = PROG_OTHER; fix_hook_pc = -1; return; }
    }
    fixes_on = opt_fix && prog == PROG_VENTUR;
    debug_on = opt_debug;
    if (!fixes_on && !debug_on) { fix_hook_pc = -1; return; }
    memset(&vx, 0, sizeof vx);
    srand((unsigned)time(NULL));
}

/* ------------------------------------------------------------------ */
/* commands typed at a prompt that never reach the program              */
/* ------------------------------------------------------------------ */
static void upper_trim(const char *in, char *out, size_t n)
{
    size_t i = 0;
    while (*in == ' ' || *in == '\t') in++;
    while (*in && i + 1 < n) out[i++] = (char)toupper((unsigned char)*in++);
    while (i > 0 && (out[i - 1] == ' ' || out[i - 1] == '\t')) i--;
    out[i] = 0;
}

static void debug_help(void)
{
    monitor_print("[DEBUG] #GOD    invulnerability on/off\n"
                  "[DEBUG] #STATS  maximum stats\n"
                  "[DEBUG] #GOLD   9,999,999 gold, the most that prints and saves whole\n");
}

static void debug_command(const char *cmd)
{
    int i, n = 0;
    if (prog == PROG_OTHER || !debug_on) {
        monitor_print("[DEBUG] NOTHING TO DO IN THIS PROGRAM.\n");
        return;
    }
    if (!strcmp(cmd, "#HELP") || !strcmp(cmd, "#")) { debug_help(); return; }
    if (!strcmp(cmd, "#GOD")) {
        god = !god;
        if (prog == PROG_VENTUR) god_str = getv(V_STR);
        for (i = 1; i <= 6; i++) { god_level[i] = 0; god_s[i] = 0; }
        printf_game("[DEBUG] INVULNERABILITY %s.\n", god ? "ON" : "OFF");
        return;
    }
    if (prog == PROG_VENTUR) {
        if (!strcmp(cmd, "#STATS")) {
            setv(V_STR, MAX_SAFE);
            if (god) god_str = MAX_SAFE;
            monitor_print("[DEBUG] STRENGTH 9999999.\n");
            return;
        }
        if (!strcmp(cmd, "#GOLD")) {
            setv(V_GOLD, MAX_SAFE);
            monitor_print("[DEBUG] 9999999 GOLD PIECES.\n");
            return;
        }
    } else {
        int s = !strcmp(cmd, "#STATS"), g = !strcmp(cmd, "#GOLD");
        if (s || g) {
            for (i = 1; i <= 6; i++) {
                if (!d_in_play(i)) continue;
                n++;
                if (s) {
                    static const int ab[] = { DT_S, DT_D, DT_C, DT_II, DT_W, DT_CH };
                    int k;
                    for (k = 0; k < 6; k++) sa(ab[k], i, ABILITY_MAX);
                    god_s[i] = ABILITY_MAX;
                    sa(DT_NHITS, i, ga(DT_HPTOT, i));
                    sa(DT_HP, i, ga(DT_HPTOT, i));
                    if (ga(DT_MANATOT, i) > 0) sa(DT_MANA, i, ga(DT_MANATOT, i));
                }
                if (g) sa(DT_GOLD, i, MAX_SAFE);
            }
            if (!n) monitor_print("[DEBUG] NO CHARACTERS ARE IN PLAY YET.\n");
            else if (s) printf_game("[DEBUG] ALL ABILITIES 18, HIT POINTS AND MANA FULL (%d CHARACTER%s).\n", n, n > 1 ? "S" : "");
            else printf_game("[DEBUG] 9999999 GOLD PIECES (%d CHARACTER%s).\n", n, n > 1 ? "S" : "");
            return;
        }
    }
    monitor_print("[DEBUG] UNKNOWN COMMAND. TYPE #HELP.\n");
}

/* A line of terminal input, before the program sees it.  Returns 0 to pass
 * it on, 1 if it was handled here and another line should be read, 2 if
 * the program should receive an empty line (a restore is about to replace
 * everything anyway). */
int fixes_input_line(const char *line)
{
    char cmd[80];
    if (!opt_fix && !opt_debug) return 0;
    upper_trim(line, cmd, sizeof cmd);
    if (opt_debug && cmd[0] == '#') {
        if (opt_echo_input) { monitor_print(line); monitor_print("\n"); }
        debug_command(cmd);
        monitor_reprint_line();
        return 1;
    }
    if (fixes_on && prog == PROG_VENTUR && !strcmp(cmd, "RESTORE") && last_line == 4860) {
        int r;
        if (opt_echo_input) { monitor_print(line); monitor_print("\n"); }
        r = v_restore();
        if (r > 0) { monitor_print("GAME RESTORED.\n"); return 2; }
        monitor_print(r < 0 ? "THAT SAVED GAME IS DAMAGED.\n" : "THERE IS NO SAVED GAME.\n");
        monitor_reprint_line();
        return 1;
    }
    return 0;
}
