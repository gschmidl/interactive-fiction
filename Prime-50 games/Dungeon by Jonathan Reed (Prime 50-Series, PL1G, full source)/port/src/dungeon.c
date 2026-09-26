/* ======================================================================
 *  DUNGEON - Jonathan H. Reed, 6/83, Prime PL/I subset G
 *
 *  A transliteration of ..\..\src_original\PL1G_GAMES\DUNGEON.PL1G to C,
 *  statement for statement: same procedure names, same order, the same
 *  labels and GO TOs, the same global variables (and the same locals that
 *  shadow them in GETNUM, TAKE, WAND, POTION, ATTACK, MOVE, ...).
 *  List-directed input and output live in pl1io.c and follow what the
 *  original does on PRIMOS, measured against it.
 *
 *  PL/I details reproduced here:
 *    - CHARACTER comparison pads the shorter side with blanks (ceq).
 *    - INDEX(haystack, needle) is 1 based, 0 when absent: the game uses
 *      INDEX('north', item) = 1 to accept abbreviations.
 *    - CHARACTER(n) VARYING assignment truncates: COMMAND is CHARACTER(8)
 *      VARYING, which is why "checktraps" is stored as "checktra", and
 *      GETNUM's NUM is CHARACTER(4) VARYING.
 *    - MOD(x, y) = x - y * FLOOR(x / y) on FLOAT.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include "pl1io.h"

/* ---- declarations, in the order of the original --------------------- */
static long hidpoint, trap, lim, trapflag, ckfl;
static long hidden_[101][5];                    /* hidden (100,4)        */
static char mem_[11][11];                       /* mem (10) char (10)    */
static long memw[11][4];                        /* memw (10,3)           */
static long okesp, drag;
static long mempoint, pos_sub, anger, dead, newdrag, hp;
static long old_[4], position[4];               /* old, position (3)     */
static char possess[51][11];                    /* possess (50) char (10)*/
static char item[11];                           /* char (10) varying     */
static long gold, flg1, rate, added;
static char command[9];                         /* char (8) varying      */
static long i_, u_, v_, x_, y_, z_;             /* i, u, v, x, y, z      */
static long m_[11][11][11], t_[11][11][11];     /* m, t (10,10,10)       */

/* PORT: is a remembered place (memw) inside the dungeon?  REMEMBER takes
   any three numbers; see DTURN's dragon, the one place they are used as
   subscripts. */
static int memw_inside(const long *w)
{
    return w[1] >= 1 && w[1] <= 10 && w[2] >= 1 && w[2] <= 10 && w[3] >= 1 && w[3] <= 10;
}
static float seed, rnd;        /* PL/I FLOAT with no precision is single */
static long number;

static int fixes = 1;          /* the port's fixes; --no-fixes clears it */

/* ---- PL/I helpers --------------------------------------------------- */

/* CHARACTER comparison: the shorter operand is blank padded */
static int ceq(const char *a, const char *b)
{
    size_t i, la = strlen(a), lb = strlen(b), n = la > lb ? la : lb;
    for (i = 0; i < n; i++) {
        char ca = i < la ? a[i] : ' ';
        char cb = i < lb ? b[i] : ' ';
        if (ca != cb)
            return 0;
    }
    return 1;
}

/* INDEX(haystack, needle), 1 based, 0 if not found */
static int pl1_index(const char *hay, const char *needle)
{
    const char *p;
    if (!*needle)
        return 1;
    p = strstr(hay, needle);
    return p ? (int)(p - hay) + 1 : 0;
}

/* assignment to a CHARACTER(n) VARYING: truncate to n */
static void assign_varying(char *dst, int maxlen, const char *src)
{
    int n = 0;
    while (src[n] && n < maxlen)
        n++;
    memcpy(dst, src, (size_t)n);
    dst[n] = 0;
}

/* assignment to a fixed CHARACTER(n): blank padded, truncated */
static void assign_fixed(char *dst, int n, const char *src)
{
    int k = 0;
    while (src[k] && k < n) {
        dst[k] = src[k];
        k++;
    }
    while (k < n)
        dst[k++] = ' ';
    dst[n] = 0;
}

static float pl1_mod(float a, float b)
{
    return a - b * floorf(a / b);
}

/* ---- forward declarations (the internal procedures) ----------------- */
static void getnum(void);
static void score_(void);
static void abbrev(void);
static void status_(void);
static void rand_(void);
static void replace_(void);
static void init_(void);
static void chesp(void);
static void thcord(void);
static void where_(void);
static void dragon(void);
static void search_(void);
static void untrap(void);
static void abbrev2(void);
static void move_(void);
static void take_(void);
static void help_(void);
static void items_(void);
static void lev(void);
static void portal(void);
static void wand(void);
static void potion(void);
static void dturn(void);
static void attack(void);
static void remember(void);
static void memory_(void);
static void forget(void);
static void hide_(void);
static void retrieve(void);
static void falltrap(void);
static void checktrap(void);
static void direct(void);

/* ====================================================================== */
/*  main program                                                          */
/* ====================================================================== */

static void usage(void)
{
    puts("usage: dungeon [--no-fixes]\n"
         "\n"
         "DUNGEON, Jonathan H. Reed's dragon crawl (Prime PL/I, June 1983).\n"
         "\n"
         "  --no-fixes   the program as it ran on the Prime: at \"press <return>\n"
         "               twice\" the Returns alone do not go on\n"
         "  -h, --help   this");
}

int main(int argc, char **argv)
{
    int a;
    for (a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "--no-fixes"))
            fixes = 0;
        else if (!strcmp(argv[a], "-h") || !strcmp(argv[a], "--help")) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "dungeon: unknown option %s (dungeon --help "
                    "lists them)\n", argv[a]);
            return 2;
        }
    }

    /* The 50-Series rounds FLOAT toward zero, and the dungeon depends on it:
       RND = MOD(SEED,100)*.01 and FLOOR(RND*20) land on different integers
       under round-to-nearest, so the whole cave comes out different.  Checked
       against the original on PRIMOS: with truncation seed 42 starts the
       player at 5 7 10 and the ESP map matches cell for cell. */
    fesetround(FE_TOWARDZERO);

    put_skip(); put_str("******** DUNGEON ********");
    put_skip(); put_str("  ");
m1:
    put_skip(); put_str("Do you want instructions?");
    get_token(item, sizeof item, 0);
    if (pl1_index("yes", item) == 1) direct();
    else if (pl1_index("YES", item) == 1) direct();
    else if (pl1_index("no", item) == 1) ;
    else if (pl1_index("NO", item) == 1) ;
    else goto m1;

    put_skip(); put_str("Please input a wierd positive number.");
    getnum();
    seed = (float)number;
    seed = floorf(seed * 1397.0f) + 192.0f;

g1:
    init_();

l1:
    if (dead > 0) {
        put_skip(); put_str(" ");
        put_skip(); put_str("Command?");
        get_token(command, sizeof command, 0);
        abbrev();
        if (trap > 0) {
            if (ceq(command, "search")) ;
            else if (ceq(command, "checktra")) ;
            else if (ceq(command, "help")) ;
            else if (ceq(command, "untrap")) ;
            else {
                falltrap();
                if (trapflag == 1) {
                    trapflag = 0;
                    if (anger == 0) goto l1;
                    assign_varying(command, 8, "d");
                }
            }
        }
        if (ceq(command, "d")) ;
        else if (ceq(command, "move")) move_();
        else if (ceq(command, "attack")) attack();
        else if (ceq(command, "items")) items_();
        else if (ceq(command, "help")) help_();
        else if (ceq(command, "checktra")) checktrap();
        else if (ceq(command, "where")) where_();
        else if (ceq(command, "untrap")) untrap();
        else if (ceq(command, "search")) search_();
        else if (ceq(command, "take")) take_();
        else if (ceq(command, "hide")) hide_();
        else if (ceq(command, "retrieve")) retrieve();
        else if (ceq(command, "esp")) lev();
        else if (ceq(command, "portal")) portal();
        else if (ceq(command, "status")) status_();
        else if (ceq(command, "wand")) wand();
        else if (ceq(command, "potion")) potion();
        else if (ceq(command, "quit")) ;
        else if (ceq(command, "remember")) remember();
        else if (ceq(command, "memory")) memory_();
        else if (ceq(command, "forget")) forget();
        else {
            put_skip(); put_str("I don't understand.");
            goto l1;
        }
    }
    if (ceq(command, "help")) ;
    else if (ceq(command, "quit")) ;
    else {
        okesp = okesp - 1;
        dead = dead - 1;
        if (anger > 0) {
            if (newdrag == 0) dturn();
            else newdrag = 0;
        }
    }
    if (ceq(command, "quit")) {
        score_();
        put_skip(); put_str("Your final score is:"); put_num(rate, 4);
        put_skip(); put_str("                 ");
    } else if (dead <= 0) {
        put_skip(); put_str("                 ");
        put_skip(); put_str("Unfortunately, this kills you!");
        put_skip(); put_str("                 ");
        put_skip(); put_str("Your"); put_num(gold, 4); put_str("gold pieces will make a");
        put_skip(); put_str("nice addition to some dragon's hoard!");
        put_skip(); put_str("                 ");
        score_();
        put_skip(); put_str("You died with a score of:"); put_num(rate, 4);
        put_skip(); put_str("                ");
    } else
        goto l1;

a1:
    put_skip(); put_str("Play again?");
    get_token(item, sizeof item, 0);
    if (pl1_index("yes", item) == 1) goto g1;
    else if (pl1_index("no", item) == 1) ;
    else goto a1;

    put_end_line();
    return 0;
}

/* ====================================================================== */
/*  procedures                                                            */
/* ====================================================================== */

static void getnum(void)
{
    number = get_number();             /* GETNUM's own loop, see pl1io.c   */
}

static void score_(void)
{
    rate = 3 * gold + pos_sub + 5 * drag + added;
}

static void abbrev(void)
{
    char c[9];
    assign_varying(c, 8, command);
    if (pl1_index("esp", c) == 1) assign_varying(c, 8, "esp");
    else if (pl1_index("help", c) == 1) assign_varying(c, 8, "help");
    else if (pl1_index("move", c) == 1) assign_varying(c, 8, "move");
    else if (pl1_index("take", c) == 1) assign_varying(c, 8, "take");
    else if (pl1_index("attack", c) == 1) assign_varying(c, 8, "attack");
    else if (pl1_index("wand", c) == 1) assign_varying(c, 8, "wand");
    else if (pl1_index("items", c) == 1) assign_varying(c, 8, "items");
    else if (pl1_index("search", c) == 1) assign_varying(c, 8, "search");
    else if (pl1_index("checktraps", c) == 1) assign_varying(c, 8, "checktra");
    else if (pl1_index("hide", c) == 1) assign_varying(c, 8, "hide");
    else if (pl1_index("remember", c) == 1) assign_varying(c, 8, "remember");
    else if (pl1_index("potion", c) == 1) assign_varying(c, 8, "potion");
    else if (pl1_index("portal", c) == 1) assign_varying(c, 8, "portal");
    else if (pl1_index("status", c) == 1) assign_varying(c, 8, "status");
    else if (pl1_index("where", c) == 1) assign_varying(c, 8, "where");
    else if (pl1_index("memory", c) == 1) assign_varying(c, 8, "memory");
    else if (pl1_index("forget", c) == 1) assign_varying(c, 8, "forget");
    else if (pl1_index("retrieve", c) == 1) assign_varying(c, 8, "retrieve");
    else if (pl1_index("untrap", c) == 1) assign_varying(c, 8, "untrap");
    else if (pl1_index("quit", c) == 1) assign_varying(c, 8, "quit");
    assign_varying(command, 8, c);
}

static void status_(void)
{
    put_skip(); put_str("Life energy level :"); put_num(dead, 4);
    if (okesp <= 0) { put_skip(); put_str("Esp : working"); }
    else { put_skip(); put_str("Esp : not working"); }
    score_();
    put_skip(); put_str("Current score :"); put_num(rate, 4);
}

static void rand_(void)
{
    float con = 12727.0f, joe;
    joe = floorf(seed / 100.0f);
    seed = pl1_mod(seed + joe + con, 100000.0f);
    rnd = pl1_mod(seed, 100.0f) * 0.01f;
}

static void replace_(void)
{
l1:
    rand_();
    rnd = floorf(rnd * 5) + 1;
    if (rnd == 1) goto l1;
    if (rnd == 5) rnd = 7;
    m_[old_[1]][old_[2]][old_[3]] = (long)rnd;
    thcord();
    if (m_[x_][y_][z_] == 0) m_[x_][y_][z_] = 1;
}

static void init_(void)
{
    for (i_ = 1; i_ <= 10; i_++)
        for (u_ = 1; u_ <= 10; u_++)
            for (v_ = 1; v_ <= 10; v_++) {
                m_[i_][u_][v_] = 0;
                t_[i_][u_][v_] = 0;
            }
    for (i_ = 1; i_ <= 75; i_++) {         /* monsters, gold, and traps    */
        thcord();
        m_[x_][y_][z_] = 1;
        { long lim_ = (long)floorf(rnd * 20); for (v_ = 1; v_ <= lim_; v_++) rand_(); }
        thcord();
        m_[x_][y_][z_] = 2;
        { long lim_ = (long)floorf(rnd * 20); for (v_ = 1; v_ <= lim_; v_++) rand_(); }
        thcord();
        t_[x_][y_][z_] = 1;
        { long lim_ = (long)floorf(rnd * 20); for (v_ = 1; v_ <= lim_; v_++) rand_(); }
    }
    for (i_ = 1; i_ <= 50; i_++) {         /* more gold and traps          */
        thcord();
        { long lim_ = (long)floorf(rnd * 20); for (v_ = 1; v_ <= lim_; v_++) rand_(); }
        m_[x_][y_][z_] = 2;
        { long lim_ = (long)floorf(rnd * 20) + 1; for (v_ = 1; v_ <= lim_; v_++) rand_(); }
        thcord();
        t_[x_][y_][z_] = 1;
    }
    { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
    for (i_ = 1; i_ <= 12; i_++) {         /* wands, potions, and monsters */
        thcord();
        m_[x_][y_][z_] = 4;
        thcord();
        m_[x_][y_][z_] = 1;
        thcord();
        m_[x_][y_][z_] = 3;
        { long lim_ = (long)floorf(rnd * 20) + 1; for (u_ = 1; u_ <= lim_; u_++) rand_(); }
    }
    { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
    for (i_ = 1; i_ <= 12; i_++) {         /* swords and potions           */
        thcord();
        m_[x_][y_][z_] = 7;
        thcord();
        m_[x_][y_][z_] = 3;
        { long lim_ = (long)floorf(rnd * 20) + 1; for (u_ = 1; u_ <= lim_; u_++) rand_(); }
    }
l1:
    put_skip(); put_str("Short or long version?");
    get_token(item, sizeof item, 0);
    if (pl1_index("short", item) == 1) {
        lim = 500;
        dead = 500;
        v_ = 5;
    } else if (pl1_index("long", item) == 1) {
        lim = 1000;
        dead = 1000;
        v_ = 3;
    } else
        goto l1;
    for (u_ = 1; u_ <= v_; u_++) {
        { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
        thcord();                          /* fake portal                  */
        m_[x_][y_][z_] = 5;
        { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
        thcord();                          /* true portal                  */
        m_[x_][y_][z_] = 6;
    }
    thcord();
    position[1] = x_;
    position[2] = y_;
    position[3] = z_;
    if (t_[x_][y_][z_] == 1) trap = 1; else trap = 0;

    /*                1 = monster             */
    /*                2 = gold                */
    /*                3 = potion              */
    /*                4 = wand                */
    /*                5 = fake portal         */
    /*                6 = true portal         */
    /*                7 = sword               */
    ckfl = 0; flg1 = 0;
    added = 0;
    gold = 0;
    pos_sub = 1;
    where_();
    anger = 0;
    okesp = 0;
    drag = 0;
    mempoint = 1;
    hidpoint = 1;
    trapflag = 0;
}

static void chesp(void)
{
    rand_();
    if (okesp < 5) okesp = (long)(floorf(rnd * 15) + 5);
}

static void thcord(void)
{
    long j;
    rand_();
    x_ = (long)(floorf(rnd * 10) + 1);
    rand_(); { long lim_ = (long)floorf(rnd * 20); for (j = 1; j <= lim_; j++) rand_(); }
    y_ = (long)(floorf(rnd * 10) + 1);
    rand_(); { long lim_ = (long)floorf(rnd * 20); for (j = 1; j <= lim_; j++) rand_(); }
    z_ = (long)(floorf(rnd * 10) + 1);
}

static void where_(void)
{
    put_skip(); put_str("Currently at:");
    put_num(position[1], 4); put_num(position[2], 4); put_num(position[3], 4);
}

static void dragon(void)
{
    newdrag = 1;
    old_[1] = position[1]; old_[2] = position[2]; old_[3] = position[3];
    put_skip(); put_str("                  ");
    put_skip(); put_str("There is a dragon in this room!");
    rand_();
    anger = (long)(floorf(rnd * 10) + 1);
    rand_();
    hp = (long)(floorf(rnd * 5) + 18);
    if (anger < 4) { put_skip(); put_str("You are lucky...it is asleep."); }
    if (anger > 7) {
        put_skip(); put_str("It heard you coming and has sneak attacked!");
        rand_();
        if (floorf(rnd * 11) > 6) {
            put_skip(); put_str("You are badly bitten!");
            rand_(); dead = dead - (long)floorf(rnd * 10) - 21; chesp();
        } else if (floorf(rnd * 11) < 3) {
            put_skip(); put_str("It jumps at you but misses!");
        } else {
            rand_();
            gold = (long)floorf(gold * rnd);
            put_skip(); put_str("It swings at you with a claw, but");
            put_skip(); put_str("misses. Unfortunately, your gold purse");
            put_skip(); put_str("is cut open and you lose some gold!");
        }
    }
    if (anger < 8) if (anger > 3) {
        put_skip(); put_str("You have both surprised one another,");
        rand_();
        if (floorf(rnd * 11) > 5) {
            put_skip(); put_str("however, you gain the initiative!");
        } else {
            put_skip(); put_str("but it acts first!");
            rand_();
            if (floorf(rnd * 11) > 3) dturn();
            else {
                rand_();
                put_skip(); put_str("The dragon roars ferociously at you,");
                put_skip(); put_str("shattering some of your possessions!");
                rand_();
                if (pos_sub > 5) pos_sub = (long)floorf(pos_sub * rnd) + 1;
                else if (pos_sub > 1) pos_sub = pos_sub - 1;
            }
        }
    }
}

static void search_(void)
{
    long temp;                             /* PICTURE '999'                */
    flg1 = 1;
    temp = m_[position[1]][position[2]][position[3]];
    if (temp == 1) { put_skip(); put_str("You can't, while a dragon's present."); }
    if (temp == 2) { put_skip(); put_str("There is gold here!"); }
    if (temp == 3) { put_skip(); put_str("There is a potion here!"); }
    if (temp == 4) { put_skip(); put_str("There is a strange wand here!"); }
    if (temp == 5) { put_skip(); put_str("There is a portal here!"); }
    if (temp == 6) { put_skip(); put_str("There is a portal here!"); }
    if (temp == 7) { put_skip(); put_str("There is a sword here!"); }
    if (temp == 0) { put_skip(); put_str("There is nothing here."); }
}

static void untrap(void)
{
    if (ckfl == 0) {
        put_skip(); put_str("You have to find a trap first.");
    } else {
        if (t_[position[1]][position[2]][position[3]] == 0) {
            put_skip(); put_str("There is no trap here.");
        } else {
            rand_();
            if (floorf(rnd * 10) + 1 > 2) {
                put_skip(); put_str("You are successful.");
                added = added + 3;
            } else {
                put_skip(); put_str("You accidentally spring the ");
                put_skip(); put_str("trap on yourself and...");
                rand_();
                if (floorf(rnd * 10) < 3) {
                    put_skip(); put_str("are injured by falling rocks!");
                } else if (floorf(rnd * 10) < 6) {
                    put_skip(); put_str("are injured by flying rocks!");
                } else if (floorf(rnd * 10) < 8) {
                    put_skip(); put_str("are bitten by a snake in a pit!");
                } else {
                    put_skip(); put_str("are crushed as the floor caves");
                    put_skip(); put_str("in, but you do not fall through!");
                }
                dead = dead - (long)floorf(rnd * 4) - 1;
                added = added + 1;
            }
            trap = 0;
            t_[position[1]][position[2]][position[3]] = 0;
        }
    }
}

static void abbrev2(void)
{
    char c[11];
    assign_varying(c, 10, item);
    if (pl1_index("north", c) == 1) assign_varying(c, 10, "north");
    else if (pl1_index("south", c) == 1) assign_varying(c, 10, "south");
    else if (pl1_index("east", c) == 1) assign_varying(c, 10, "east");
    else if (pl1_index("west", c) == 1) assign_varying(c, 10, "west");
    else if (pl1_index("up", c) == 1) assign_varying(c, 10, "up");
    else if (pl1_index("down", c) == 1) assign_varying(c, 10, "down");
    assign_varying(item, 10, c);
}

static void move_(void)
{
    long flag = 0, flag1 = 0;
l1:
    put_skip(); put_str("What direction?");
    get_token(item, sizeof item, 0);
    abbrev2();
    if (ceq(item, "up")) {
        if (position[3] == 1) flag = 1;
        else { position[3] = position[3] - 1; flag1 = 1; }
    }
    if (ceq(item, "down")) {
        if (position[3] == 10) flag = 1;
        else { position[3] = position[3] + 1; flag1 = 1; }
    }
    if (ceq(item, "north")) {
        if (position[1] == 1) flag = 1;
        else { position[1] = position[1] - 1; flag1 = 1; }
    }
    if (ceq(item, "south")) {
        if (position[1] == 10) flag = 1;
        else { position[1] = position[1] + 1; flag1 = 1; }
    }
    if (ceq(item, "west")) {
        if (position[2] == 1) flag = 1;
        else { position[2] = position[2] - 1; flag1 = 1; }
    }
    if (ceq(item, "east")) {
        if (position[2] == 10) flag = 1;
        else { position[2] = position[2] + 1; flag1 = 1; }
    }
    if (flag == 1) { put_skip(); put_str("You can't go"); put_str(item); put_str("! "); }
    else if (flag1 == 0) goto l1;
    if (flag == 1) ;
    else {
        ckfl = 0; flg1 = 0;
        put_skip(); put_str("Ok.");
        if (t_[position[1]][position[2]][position[3]] == 1) trap = 1;
        else trap = 0;
    }
    if (m_[position[1]][position[2]][position[3]] == 1)
        if (flag1 == 1) dragon();
}

static void take_(void)
{
    long i;                                /* PICTURE '9'                  */
    long flag = 0;
    char pic[8];
    if (flg1 == 0) {
        put_skip(); put_str("Take what? You haven't searched yet.");
    } else {
        if (m_[position[1]][position[2]][position[3]] == 4) {
            flag = 1;
            m_[position[1]][position[2]][position[3]] = 0;
            assign_fixed(possess[pos_sub], 10, "wand");
            pos_sub = pos_sub + 1;
        }
        if (m_[position[1]][position[2]][position[3]] == 7) {
            flag = 1;
            m_[position[1]][position[2]][position[3]] = 0;
            assign_fixed(possess[pos_sub], 10, "sword");
            pos_sub = pos_sub + 1;
        }
        if (m_[position[1]][position[2]][position[3]] == 1) {
            put_skip(); put_str("Be serious!!");
            flag = 3;
        }
        if (m_[position[1]][position[2]][position[3]] == 3) {
            flag = 1;
            m_[position[1]][position[2]][position[3]] = 0;
            assign_fixed(possess[pos_sub], 10, "potion");
            pos_sub = pos_sub + 1;
        }
        if (m_[position[1]][position[2]][position[3]] == 2) {
            rand_(); i = (long)(floorf(rnd * 5) + 1);
            gold = gold + i;
            snprintf(pic, sizeof pic, "%ld", i % 10);   /* PICTURE '9' */
            put_skip(); put_pic(pic); put_str("gold pieces");
            put_skip(); put_str("        ");
            flag = 1;
            m_[position[1]][position[2]][position[3]] = 0;
        }
        if (m_[position[1]][position[2]][position[3]] == 5) {
            put_skip(); put_str("You can't possibly take a portal!");
            flag = 3;
        }
        if (m_[position[1]][position[2]][position[3]] == 6) {
            put_skip(); put_str("You can't possibly take a portal!");
            flag = 3;
        }
        if (flag == 0) { put_skip(); put_str("There is nothing here."); }
        else if (flag == 1) { put_skip(); put_str("Ok."); }
    }
}

static void help_(void)
{
    put_skip(); put_str("Valid commands are:");
    put_skip(); put_str("   attack     - attack dragon by hands or with sword");
    put_skip(); put_str("   checktraps - check cavern for a trap");
    put_skip(); put_str("   esp        - view current level");
    put_skip(); put_str("   forget     - erase an item from memory");
    put_skip(); put_str("   help       - list commands and their functions");
    put_skip(); put_str("   hide       - hide gold in cavern");
    put_skip(); put_str("   items      - take inventory of your possessions");
    put_skip(); put_str("   memory     - list contents of your memory");
    put_skip(); put_str("   move       - move to adjacent cavern");
    put_skip(); put_str("   portal     - enter portal");
    put_skip(); put_str("   potion     - drink a potion");
    put_skip(); put_str("   quit       - quit the game");
    put_skip(); put_str("   retrieve   - retrieve gold that you have hidden");
    put_skip(); put_str("   remember   - enter something into your memory");
    put_skip(); put_str("   search     - search cavern for items");
    put_skip(); put_str("   status     - check energy level, esp, and score ");
    put_skip(); put_str("   take       - take an item from cavern");
    put_skip(); put_str("   untrap     - unset a trap in cavern");
    put_skip(); put_str("   wand       - use a wand against dragon");
    put_skip(); put_str("   where      - list x,y,z coordin. of position in dungeon");
}

static void items_(void)
{
    for (x_ = 1; x_ <= pos_sub - 1; x_++) {
        put_skip(); put_num(x_, 4); put_str(possess[x_]);
    }
    put_skip(); put_str("                         ");
    put_skip(); put_num(gold, 4); put_str("gold pieces.");
}

static void lev(void)
{
    long flag;
    if (okesp <= 0) {
        put_skip(); put_str(" ");
        put_str("1      2      3      4      5      6");
        put_str("7      8      9      10");
        for (x_ = 1; x_ <= 10; x_++) {
            put_skip(); put_num(x_, 4);
            for (y_ = 1; y_ <= 10; y_++) {
                flag = 0;
                if (x_ == position[1]) if (y_ == position[2]) {
                    put_str("@"); flag = 1;
                }
                if (flag == 0) {
                    if (m_[x_][y_][position[3]] > 0) put_str("*");
                    else put_str(".");
                }
            }
        }
    } else {
        put_skip(); put_str("The last injury you suffered");
        put_skip(); put_str("has messed up your esp powers.");
    }
}

static void portal(void)
{
    if (flg1 == 0) {
        put_skip(); put_str("You haven't searched for a portal here yet.");
    } else if (m_[position[1]][position[2]][position[3]] == 6) {
        put_skip(); put_str("You enter the portal and there is a");
        put_skip(); put_str("blinding flash of light.");
        put_skip(); put_str("The next thing you know...");
        put_skip(); put_str("                ");
        put_skip(); put_str("You're outside in the sunlight in the");
        put_skip(); put_str("middle of a grassy meadow!");
        put_skip(); put_str("         ");
        put_skip(); put_str("You made it!!!");
        score_();
        added = added + 50;
        put_skip(); put_str("          ");
        put_skip(); put_str("You vanquished"); put_num(drag, 5); put_str("dragons, and");
        put_skip(); put_str("with your gold and other possessions...");
        assign_varying(command, 8, "quit");
    } else if (m_[position[1]][position[2]][position[3]] == 5) {
        put_skip(); put_str("You enter the portal and there is a");
        put_skip(); put_str("blinding flash of light!");
        put_skip(); put_str("The next thing you know...");
        put_skip(); put_str("           ");
        put_skip(); put_str("Nothing else happens!");
        put_skip(); put_str("              ");
        put_skip(); put_str("This is a fake portal!");
        m_[position[1]][position[2]][position[3]] = 0;
    } else {
        put_skip(); put_str("There is no portal here.");
    }
}

static void wand(void)
{
    long w, flag = 0, temp;
    if (m_[position[1]][position[2]][position[3]] == 1) {
        put_skip(); put_str("Which number wand will you use?");
        getnum(); w = number;
        if (w > 0) if (w < pos_sub) if (ceq(possess[w], "wand")) {
            flag = 1;
            put_skip(); put_str("You get out wand #"); put_num(w, 4); put_str("and wave");
            put_skip(); put_str("it high over your head.");
            put_skip(); put_str("        ");
            rand_();
            temp = (long)(floorf(rnd * 10) + 1);
            if (temp > 8) {
                put_skip(); put_str("The dragon suddenly turns red with anger!");
                anger = anger + 5;
            } else if (temp > 6) {
                put_skip(); put_str("The dragon suddenly disappears!");
                replace_();
                anger = 0;
                drag = drag + 1;
            } else if (temp > 4) {
                put_skip(); put_str("The dragon explodes, splattering its");
                put_skip(); put_str("guts all over the room...and you!");
                { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
                if (floorf(rnd * 10) + 1 > 5) {
                    rand_();
                    dead = dead - (long)floorf(rnd * 5) - 1;
                    put_skip(); put_str("Its blood seems to burn your skin");
                    put_skip(); put_str("a bit, but then it stops.");
                }
                anger = 0;
                replace_();
                drag = drag + 1;
            } else if (temp > 2) {
                put_skip(); put_str("The dragon suddenly falls to ");
                put_skip(); put_str("the ground! Then it disappears!");
                anger = 0;
                replace_();
                drag = drag + 1;
            } else {
                put_skip(); put_str("The dragon begins to smile!");
                rand_();
                temp = (long)(floorf(rnd * 5) + 1);
                put_skip(); put_str("It befriends you and gives you");
                put_skip(); put_num(temp, 4); put_str("gold pieces!");
                put_skip(); put_str("Then it flies away.");
                anger = 0;
                replace_();
                drag = drag + 1;
                gold = gold + temp;
            }
            put_skip(); put_str("The wand disappears from your hand.");
            pos_sub = pos_sub - 1;
            memcpy(possess[w], possess[pos_sub], 11);
        }
        if (flag == 0) { put_skip(); put_str("You have no wand #"); put_num(w, 4); put_str("."); }
    } else {
        put_skip(); put_str("Wands only work in the presence of dragons.");
    }
}

static void potion(void)
{
    long p, flag = 0, temp;
    put_skip(); put_str("Which number potion will you drink?");
    getnum(); p = number;
    if (p > 0) if (p < pos_sub) if (ceq(possess[p], "potion")) {
        flag = 1;
        put_skip(); put_str("You drink the potion and...");
        rand_();
        temp = (long)(floorf(rnd * 10) + 1);
        if (temp > 5) {
            put_skip(); put_str("you feel greatly refreshed!");
            if (dead < lim) {
                rand_();
                dead = dead + (long)floorf(rnd * 20) + 41;
            }
        } else if (temp > 3) {
            if (okesp > 0) {
                put_skip(); put_str("your esp is restored!");
                okesp = 0;
            } else {
                put_skip(); put_str("Nothing happens!");
            }
        } else if (temp > 1) {
            put_skip(); put_str("you suddenly disappear,");
            thcord();
            anger = 0; ckfl = 0; flg1 = 0;
            position[1] = x_; position[2] = y_; position[3] = z_;
            put_skip(); put_str("and reappear at:");
            put_num(x_, 4); put_num(y_, 4); put_num(z_, 4);
            if (t_[x_][y_][z_] == 1) trap = 1; else trap = 0;
            if (m_[x_][y_][z_] == 1) dragon();
        } else {
            put_skip(); put_str("you get great pains in your stomach!");
            put_skip(); put_str("It was poison!");
            rand_();
            dead = dead - (long)floorf(rnd * 15) - 6;
        }
        pos_sub = pos_sub - 1;
        memcpy(possess[p], possess[pos_sub], 11);
    }
    if (flag == 0) { put_skip(); put_str("You have no potion #"); put_num(p, 4); put_str("."); }
}

static void dturn(void)
{
    long flag = 0;
    put_skip(); put_str("            ");
    if (hp <= 0) {
        put_skip(); put_str("The dragon falls dead!");
        put_skip(); put_str("And then it disappears.");
        anger = 0;
        drag = drag + 1;
        replace_();
    } else {
        if (anger < 10) anger = anger + 1;
        if (m_[position[1]][position[2]][position[3]] == 1) {
            if (anger < 5) {
                rand_();
                if (floorf(rnd * 11) < 5) {
                    put_skip(); put_str("The dragon jumps to its feet.");
                    anger = anger + 4;
                } else if (floorf(rnd * 11) < 8) {
                    put_skip(); put_str("The dragon must have been fast");
                    put_skip(); put_str("asleep because it is only slowly");
                    put_skip(); put_str("rising.");
                    anger = anger + 1;
                } else {
                    put_skip(); put_str("The dragon wakes up and flies away!");
                    anger = 0;
                    replace_();
                    drag = drag + 1;
                }
            } else if (anger > 7) {
                rand_();
                if (floorf(rnd * 11) > 2) {
                    chesp();
                    put_skip(); put_str("The dragon attacks you!");
                    rand_();
                    if (floorf(rnd * 11) > 9) {
                        put_skip(); put_str("The dragon knocks you in the head");
                        put_skip(); put_str("and you lose some of your memory!");
                        chesp();
                        if (mempoint > 1) {
                            rand_();
                            i_ = (long)(floorf((mempoint - 1) * rnd) + 1);
t1:
                            thcord();
                            if (m_[x_][y_][z_] == 0) {
                                /* PORT: the PL/I had no subscript checking: a
                                   remembered place outside the dungeon read and
                                   cleared whatever lay there.  Here it holds
                                   nothing and nothing is cleared. */
                                if (memw_inside(memw[i_])) {
                                    m_[x_][y_][z_] = m_[memw[i_][1]][memw[i_][2]][memw[i_][3]];
                                    m_[memw[i_][1]][memw[i_][2]][memw[i_][3]] = 0;
                                }
                            } else
                                goto t1;
                            { long lim_ = hidpoint; for (x_ = 1; x_ <= lim_; x_++) {
                                if (hidden_[x_][2] == memw[i_][1])
                                if (hidden_[x_][3] == memw[i_][2])
                                if (hidden_[x_][4] == memw[i_][3]) {
                                    flag = 1;
                                    hidpoint = hidpoint - 1;
                                    hidden_[x_][1] = hidden_[hidpoint][1];
                                    hidden_[x_][2] = hidden_[hidpoint][2];
                                    hidden_[x_][3] = hidden_[hidpoint][3];
                                    hidden_[x_][4] = hidden_[hidpoint][4];
                                }
                            } }
                            if (flag == 0)
                                if (hidpoint > 1) hidpoint = hidpoint - 1;
                            mempoint = mempoint - 1;
                            memcpy(mem_[i_], mem_[mempoint], 11);
                            memw[i_][1] = memw[mempoint][1];
                            memw[i_][2] = memw[mempoint][2];
                            memw[i_][3] = memw[mempoint][3];
                        }
                    } else if (floorf(rnd * 11) > 6) {
                        put_skip(); put_str("It breathes fire and burns you!");
                        rand_();
                        dead = dead - (long)floorf(rnd * 10) - 21;
                    } else if (floorf(rnd * 11) > 3) {
                        put_skip(); put_str("It crushes you against the wall!");
                        rand_();
                        dead = dead - (long)floorf(rnd * 10) - 21;
                    } else if (floorf(rnd * 11) > 1) {
                        put_skip(); put_str("It misses you, but spills your gold!");
                        rand_();
                        gold = (long)floorf(gold * rnd);
                    } else {
                        put_skip(); put_str("It claws you, breaking some of your");
                        put_skip(); put_str("possessions.");
                        rand_();
                        if (pos_sub > 5) pos_sub = (long)floorf(pos_sub * rnd) + 1;
                        else if (pos_sub > 1) pos_sub = pos_sub - 1;
                        rand_();
                        dead = dead - (long)floorf(rnd * 10) - 1;
                    }
                } else {
                    put_skip(); put_str("The dragon does not attack you but");
                    put_skip(); put_str("flies in circles above your head!");
                    { long lim_ = (long)floorf(rnd * 20) + 1; for (i_ = 1; i_ <= lim_; i_++) rand_(); }
                    if (floorf(rnd * 10) + 1 > 6) {
                        chesp();
                        put_skip(); put_str("You get dizzy watching it and you");
                        put_skip(); put_str("fall...hurting yourself!");
                        rand_();
                        dead = dead - (long)floorf(rnd * 5) - 1;
                    }
                }
            } else {
                rand_();
                if (floorf(rnd * 11) < 2) {
                    put_skip(); put_str("The dragon turns and flies away!");
                    anger = 0;
                    replace_();
                    drag = drag + 1;
                } else if (floorf(rnd * 11) < 5) {
                    put_skip(); put_str("The dragon now pauses, studying you.");
                } else if (floorf(rnd * 11) < 9) {
                    chesp();
                    put_skip(); put_str("The dragon rips your flesh with its");
                    put_skip(); put_str("huge claws!");
                    rand_();
                    dead = dead - (long)floorf(rnd * 10) - 21;
                } else {
                    put_skip(); put_str("The dragon pounces on you, destroying");
                    put_skip(); put_str("every one of your possessions...and");
                    put_skip(); put_str("nearly destroying you!");
                    rand_();
                    dead = dead - (long)floorf(rnd * 10) - 21;
                    pos_sub = 1;
                    chesp();
                }
            }
        } else {
            rand_();
            anger = anger - 4;
            if (anger < 4) {
                put_skip(); put_str("You no longer see nor hear the dragon.");
                anger = 0;
            } else {
                put_skip(); put_str("The dragon has followed you!");
                m_[old_[1]][old_[2]][old_[3]] = 0;
                old_[1] = position[1]; old_[2] = position[2]; old_[3] = position[3];
                m_[position[1]][position[2]][position[3]] = 1;
                rand_();
                if (floorf(rnd * 11) < 3) {
                    anger = anger + 1;
                    dturn();
                } else if (floorf(rnd * 11) < 6) {
                    put_skip(); put_str("It bites you from behind!");
                    rand_(); chesp();
                    dead = dead - (long)floorf(rnd * 10) - 11;
                } else if (floorf(rnd * 11) < 8) {
                    put_skip(); put_str("It breathes fire at you, but misses!");
                } else {
                    put_skip(); put_str("But you are ready for it!");
                }
            }
        }
    }
}

static void attack(void)
{
    long temp, flag;
    char p[11];
    if (m_[position[1]][position[2]][position[3]] == 1) {
l1:
        put_skip(); put_str("With what?");
        get_token(p, sizeof p, 0);
        if (pl1_index("hands", p) == 1) assign_varying(p, 10, "hands");
        else if (pl1_index("sword", p) == 1) assign_varying(p, 10, "sword");
        if (ceq(p, "hands")) {
            put_skip(); put_str("You attempt to strike the dragon...");
            rand_();
            temp = (long)(floorf(rnd * 10) + 1);
            if (temp < 3) {
                put_skip(); put_str("but it easily dodges your blow!");
            } else if (temp < 6) {
                put_skip(); put_str("and you land a hard right to its snout!");
                rand_(); hp = hp - (long)floorf(rnd * 4) - 4;
            } else if (temp < 8) {
                put_skip(); put_str("and you really lay one on it!");
                rand_(); hp = hp - (long)floorf(rnd * 4) - 6;
            } else {
                put_skip(); put_str("you kick the beast in the stomach!");
                rand_(); hp = hp - (long)floorf(rnd * 4) - 4;
            }
        } else if (ceq(p, "sword")) {
            flag = 0;
            for (i_ = 1; i_ <= pos_sub - 1; i_++) {
                if (ceq(possess[i_], "sword")) { flag = 1; u_ = i_; }
            }
            if (flag == 0) {
                put_skip(); put_str("You have no sword.");
                put_skip(); put_str("           ");
                goto l1;
            } else {
                put_skip(); put_str("You whip out your mighty sword and");
                put_skip(); put_str("slash at the dragon...");
                rand_();
                temp = (long)(floorf(rnd * 10) + 1);
                if (temp < 3) {
                    put_skip(); put_str("deeply wounding it!");
                    rand_(); hp = hp - (long)floorf(rnd * 5) - 8;
                } else if (temp < 5) {
                    put_skip(); put_str("but you miss it!");
                } else if (temp < 8) {
                    put_skip(); put_str("and draw much blood!");
                    rand_(); hp = hp - (long)floorf(rnd * 5) - 8;
                } else if (temp == 8) {
                    put_skip(); put_str("your sword breaks!");
                    pos_sub = pos_sub - 1;
                    memcpy(possess[u_], possess[pos_sub], 11);
                } else {
                    put_skip(); put_str("hitting it, but not doing");
                    put_skip(); put_str("too much harm!");
                    rand_(); hp = hp - (long)floorf(rnd * 4) - 6;
                }
            }
        } else {
            put_skip(); put_str("Hands or sword only.");
            put_skip(); put_str("              ");
            goto l1;
        }
    } else {
        put_skip(); put_str("There is no dragon here.");
    }
}

static void remember(void)
{
    char p[11];
    if (mempoint == 11) {
        put_skip(); put_str("Your memory is full.");
    } else {
        put_skip(); put_str("What do you wish to remember?");
        get_token(p, sizeof p, 0);
        assign_fixed(mem_[mempoint], 10, p);
        put_skip(); put_str("At what coordinates?");
        put_skip(); put_str("   first  :");
        getnum(); memw[mempoint][1] = number;
        put_str("   second :");
        getnum(); memw[mempoint][2] = number;
        put_str("   third  :");
        getnum(); memw[mempoint][3] = number;
        mempoint = mempoint + 1;
        put_skip(); put_str("Ok.");
    }
}

static void memory_(void)
{
    for (x_ = 1; x_ <= mempoint - 1; x_++) {
        put_skip(); put_num(x_, 4); put_str(":"); put_str(mem_[x_]); put_str("at");
        put_num(memw[x_][1], 4); put_num(memw[x_][2], 4); put_num(memw[x_][3], 4);
    }
}

static void forget(void)
{
    if (mempoint == 1) {
        put_skip(); put_str("There's nothing in your memory.");
    } else {
        put_skip(); put_str("Which number memory item do you wish to forget?");
        getnum(); x_ = number;
        /* the ELSE belongs to "IF X<MEMPOINT", so X<=0 says nothing at all */
        if (x_ > 0) {
            if (x_ < mempoint) {
                mempoint = mempoint - 1;
                memcpy(mem_[x_], mem_[mempoint], 11);
                memw[x_][1] = memw[mempoint][1];
                memw[x_][2] = memw[mempoint][2];
                memw[x_][3] = memw[mempoint][3];
                put_skip(); put_str("Ok.");
            } else {
                put_skip(); put_str("You have no memory item #"); put_num(x_, 4); put_str(".");
            }
        }
    }
}

static void hide_(void)
{
    long p;
    if (hidpoint == 101) {
        put_skip(); put_str("You can't hide in more than 100 locations.");
    } else {
        put_skip(); put_str("How many gold pieces do you wish to hide?");
        getnum();
        p = number;
        if (p > gold) {
            put_skip(); put_str("You only have"); put_num(gold, 4); put_str("gold pieces.");
        } else if (p < 0) {
            put_skip(); put_str("Don't be cute.");
        } else {
            hidden_[hidpoint][1] = p;
            hidden_[hidpoint][2] = position[1];
            hidden_[hidpoint][3] = position[2];
            hidden_[hidpoint][4] = position[3];
            gold = gold - p;
            hidpoint = hidpoint + 1;
            put_skip(); put_str("Ok.");
        }
    }
}

static void retrieve(void)
{
    long p, flag = 0;
    p = 1;
l1:
    if (p < hidpoint) {
        if (position[1] == hidden_[p][2])
        if (position[2] == hidden_[p][3])
        if (position[3] == hidden_[p][4]) {
            flag = 1;
            gold = gold + hidden_[p][1];
            put_skip(); put_str("You've retrieved"); put_num(hidden_[p][1], 4); put_str("gold pieces.");
            hidpoint = hidpoint - 1;
            hidden_[p][1] = hidden_[hidpoint][1];
            hidden_[p][2] = hidden_[hidpoint][2];
            hidden_[p][3] = hidden_[hidpoint][3];
            hidden_[p][4] = hidden_[hidpoint][4];
        }
        p = p + 1;
        goto l1;
    }
    if (flag == 0) {
        put_skip(); put_str("There is nothing hidden here.");
    }
}

static void falltrap(void)
{
    long i;
    rand_();
    if (floorf(rnd * 10) + trap + 1 < 7) {
        trap = trap + 1;
    } else {
        trapflag = 1;
        trap = 0;
        t_[position[1]][position[2]][position[3]] = 0;
        put_skip(); put_str("Unfortunately, as you are ");
        put_skip(); put_str("beginning to"); put_str(command); put_str("...");
        rand_(); dead = dead - (long)floorf(rnd * 7) - 1;
g1:
        rand_(); i = (long)floorf(rnd * 11);
        if (i > 8) {
            put_skip(); put_str("a bunch of rocks fall from");
            put_skip(); put_str("the ceiling and injure you!");
        } else if (i > 6) {
            put_skip(); put_str("a spear comes flying out of");
            put_skip(); put_str("nowhere and pierces your side!");
        } else if (i > 4) {
            put_skip(); put_str("you trip over a small cord that was");
            put_skip(); put_str("stretched across the floor, and fall");
            put_skip(); put_str("into a shallow depression full of");
            put_skip(); put_str("poisonous snakes and are bitten!");
        } else if (i > 2) {
            put_skip(); put_str("you step on a rock which moves, and");
            put_skip(); put_str("all of a sudden you are wounded by");
            put_skip(); put_str("a bunch of flying darts!");
        } else if (position[3] < 10) {
            rand_();
            v_ = (long)(floorf(rnd * (10 - position[3])) + 1);
            put_skip(); put_str("the floor caves in and you");
            put_skip(); put_str("fall down"); put_num(v_, 4); put_str("levels!");
            position[3] = position[3] + v_; ckfl = 0; flg1 = 0;
            if (t_[position[1]][position[2]][position[3]] == 1) trap = 1;
            if (m_[position[1]][position[2]][position[3]] == 1) dragon();
        } else
            goto g1;
        rand_();
        if (floorf(rnd * 10) + 1 > 5) {
            put_skip(); put_str("You are badly hurt!");
            chesp();
            dead = dead - 3;
        }
    }
}

static void checktrap(void)
{
    if (anger > 0) {
        put_skip(); put_str("You can't while a dragon's present!");
    } else {
        if (t_[position[1]][position[2]][position[3]] == 0) {
            put_skip(); put_str("There is no trap here.");
        } else {
            put_skip(); put_str("Beware! There's a trap!");
        }
        ckfl = 1;
    }
}

/* PORT FIX 1.  The instructions say "press <return> twice", and read with
   GET SKIP LIST - which skips blank lines until it finds a token (measured
   on PRIMOS 23.4), so on the Prime the Returns alone never went on and
   something had to be typed.  Here two blank lines go on, as the text
   says, and a word still does.  --no-fixes: as on the Prime. */
static void press_return_twice(void)
{
    if (fixes)
        get_token_or_returns(item, sizeof item, 2);
    else
        get_token(item, sizeof item, 0);
}

static void direct(void)
{
    put_skip(); put_str("  "); put_skip(); put_str("  "); put_skip(); put_str("  ");
    put_skip(); put_str("   DUNGEON is played in lower case only.");
    put_skip(); put_str(" ");
    put_skip(); put_str("   You find yourself in a three-dimensional underground matrix of caverns.");
    put_skip(); put_str("Your ultimate object is to successfully escape from the caverns through");
    put_skip(); put_str("a magical portal (there are fake portals too!), but only after having");
    put_skip(); put_str("killed as many dragons, collected as much gold, and unset as many");
    put_skip(); put_str("traps as possible.  Of course, you only have so much energy with which");
    put_skip(); put_str("to do all of this!");
    put_skip(); put_str(" ");
    put_skip(); put_str("   You have 20 commands to work with.  Their names and their ");
    put_skip(); put_str("functions can be listed by the HELP command.  Note that they can be ");
    put_skip(); put_str("abbreviated as you like.  Also note that HELP is the only command that ");
    put_skip(); put_str("does not use up any of your energy or take up a turn.");
    put_skip(); put_str("  ");
    put_skip(); put_str("   Besides the few general playing tips listed below, you are on your");
    put_skip(); put_str("own.  Good luck!");
    put_skip(); put_str("  "); put_skip(); put_str("  "); put_skip(); put_str("  "); put_skip(); put_str("  ");
    put_skip(); put_str("*** press <return> twice to continue ***");
    press_return_twice();
    put_skip(); put_str("   1)  It is not a good idea to carry around too many items (wands, ");
    put_skip(); put_str("       potions, swords, gold) at once because they can be lost in ");
    put_skip(); put_str("       battle.  Therefore, REMEMBER the locations of some things for");
    put_skip(); put_str("       later TAKEing, and HIDE gold at intervals for later RETRIEVEal.");
    put_skip(); put_str(" ");
    put_skip(); put_str("   2)  Your ESP allows you to view the entire level that you are on.");
    put_skip(); put_str("       The asterisks indicate that something -who knows what- is in");
    put_skip(); put_str("       a particular cavern.  Note that traps do not show up on ESP.");
    put_skip(); put_str("  ");
    put_skip(); put_str("   3)  Upon entering a true PORTAL, the game is ended.  So you may");
    put_skip(); put_str("       want to delay such action if you happen to run across a portal");
    put_skip(); put_str("       early on in your adventure.");
    put_skip(); put_str(" ");
    put_skip(); put_str("   4)  Your overall score is calculated from the amount of gold you");
    put_skip(); put_str("       are carrying with you when the game ends, the number of dragons");
    put_skip(); put_str("       you have disposed of, and, of lesser importance, the number of");
    put_skip(); put_str("       traps you have UNTRAPed.  Note that items such as wands, potions,");
    put_skip(); put_str("       swords, etc., mean little to your score, but are very valuable");
    put_skip(); put_str("       in satisfying your main objectives.");
    put_skip(); put_str(" ");
    put_skip(); put_str("*** press <return> twice to continue ***");
    press_return_twice();
    put_skip(); put_str("   5)  As dragons move through the dungeon, they destroy everything");
    put_skip(); put_str("       in their path.  So be careful where you go if you decide to run");
    put_skip(); put_str("       away.");
    put_skip(); put_str("  ");
    put_skip(); put_str("*** press <return> twice to begin ***");
    press_return_twice();
}
