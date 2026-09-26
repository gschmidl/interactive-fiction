/*
 * mars.c -- Martian Adventure, recovered.
 *
 * Waterloo, Honeywell 6000, 1978-79.  Brad Templeton (bstempleton),
 * with the adventure definition language by Mark Niemiec.
 *
 * This is a transliteration of jmc/mars/game.v2.txt -- an unfinished B
 * program -- into C, driving the archive's own data files, plus a small
 * interpreter for the adventure definition language that jmc/mars/math.v2.txt
 * is written in.
 *
 * Everything printed as game text comes out of the archive.  Anything this
 * program says on its own account is prefixed with "--" so you can always
 * tell the 1978 file apart from the 2026 scaffolding.  Commands beginning
 * with '*' are likewise mine, not the original's.
 *
 * Build:  gcc -O2 -o mars mars.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"

/* ------------------------------------------------------------------ misc */

static int opt_fix = 0;      /* repair the two author typos in math.v2 */
static int opt_light = 0;    /* start with the lamp on (see NOTE-LIGHT)   */

static void note(const char *s) { printf("-- %s\n", s); }

static char line[512];
static char *lp;

static int getline_(void)
{
    if (!fgets(line, sizeof line, stdin)) return 0;
    lp = line;
    return 1;
}

/* geta() in the B source returned one word packed into a 36-bit machine
   word, i.e. at most four 9-bit characters.  That is why the source tests
   case'nort' and case'flas' -- words are matched on their first four
   characters, and why there is a case's' but no case'sout'. */
static char *geta(void)
{
    static char w[64];
    int n = 0;
    while (*lp && !isalnum((unsigned char)*lp) && *lp != '#' && *lp != '*') lp++;
    if (!*lp) return NULL;
    while (*lp && !isspace((unsigned char)*lp) && n < 63)
        w[n++] = (char)tolower((unsigned char)*lp++);
    w[n] = 0;
    return w;
}

static char *geta4(void)                  /* the four-character form */
{
    char *w = geta();
    if (w && strlen(w) > 4) w[4] = 0;
    return w;
}

/* The B source loops back to labels try:, try1: and aga: when geta() comes
   back empty, i.e. geta() keeps reading until it has a word.  This is that. */
static char *geta4_more(void)
{
    char *w = geta4();
    while (!w) {
        if (!getline_()) return NULL;
        w = geta4();
    }
    return w;
}

static int eq(const char *a, const char *b) { return a && !strcmp(a, b); }

static void prompt(const char *p)
{
    fputs(p, stdout);
    fflush(stdout);
}

static void wrap(const char *s)           /* the archive is already wrapped */
{
    fputs(s, stdout);
    putchar('\n');
}

/* ================================================================== MARS */
/*
 * jmc/mars/game.v2.txt, jmc/mars/goto, jmc/mars/snames, jmc/mars/lnames.
 *
 * The B program read two compiled binaries, jmc/mars/desc and jmc/mars/rooms.
 * Neither is in the archive; gendata.py rebuilds their contents from the text
 * sources they were made from.
 *
 * The treasure table is gone entirely -- there is no jmc/mars/expl, no
 * vocabulary file and no per-room object file in the archive -- so the
 * externs treasures[], mtno, foodnum, magn, dict and hints are all zero,
 * exactly as an undeclared extern would be in B.  The verbs that use them
 * still run, and still print what they printed in 1978.
 */

static int r, orr, po, light, robot, alien, charge, dict_, hintcount;

static const char *dirname_[11] = { "", "north", "northeast", "east",
    "southeast", "south", "southwest", "west", "northwest", "up", "down" };

static void prlon(void)
{
    if (r <= MARS_MAXROOM && mars_long[r]) {
        wrap(mars_long[r]);
    } else if (r <= MARS_MAXROOM && mars_short[r]) {
        printf("You are %s\n", mars_short[r]);
        note("no long description for this room was ever written; that is the"
             " short one from jmc/mars/snames.");
    } else {
        printf("You are in room %d.\n", r);
        note("nothing about this room survives -- it has no entry in snames,"
             " lnames or goto.  The map points here anyway.");
    }
}

static void mars_look(void)
{
    if (r <= MARS_MAXROOM && mars_brief[r] && mars_short[r])
        printf("You are %s\n", mars_short[r]);
    else
        prlon();
}

static void show_exits(void)
{
    int i, n = 0;
    printf("--   exits:");
    for (i = 1; i <= 10; i++)
        if (r <= MARS_MAXROOM && mars_exits[r][i]) {
            printf(" %s->%d", dirname_[i], mars_exits[r][i]);
            n++;
        }
    if (!n) fputs(" none in jmc/mars/goto", stdout);
    putchar('\n');
}

static void mars_notes(void)
{
    puts("-- Mars surface and city, from jmc/mars/goto + snames + lnames.");
    puts("--   79 rooms have an exit list; 80 have a short name; 15 have a");
    puts("--   long description.  Rooms 21-40 are referenced by the map (46");
    puts("--   goes Down to 40) but nothing about them is in the archive.");
    puts("--   Rooms 51 and 52 have names but no exits: they are one-way.");
    puts("--   Rooms 10-13, the white highway running south to the Dome, can");
    puts("--   be reached from nowhere -- no room has an exit into them.");
    puts("--   There are no objects because the treasure table was not");
    puts("--   backed up, so take/drop/inventory have nothing to work on.");
    puts("--");
    puts("-- The route Brad describes, all of it in the archive:");
    puts("--   lamp / d      out of the ship onto the desert");
    puts("--   w w w         to the Viking I lander (room 4)");
    puts("--   w             the boulder, with Bottomos's four-inch hole");
    puts("--   w             the far side: WELCOME sign and airlock hatch");
    puts("--   d d d d       down the airlock shaft");
    puts("--   d             the bubble above the domed city (room 44)");
    puts("--   d d           down the Tower to the plaza, then n/ne/e/...");
    puts("-- Side trip: from the ship, e e e reaches the diamond well;");
    puts("--            n n n reaches the rim of the Valles Marineris.");
    puts("--");
    puts("-- My additions, all starting with '*':");
    puts("--   *exits  *map  *goto N  *notes  *back  *quit");
    puts("-- Bugs in the 1978 source, kept faithfully unless you pass -fix:");
    puts("--   LIGHT  light is an undeclared extern, so it is 0 and the game");
    puts("--          opens in the dark on a sunlit desert.  Type 'lamp'.");
    puts("--   LAMP   an unconditional 'if(light)...else light=1' in the");
    puts("--          middle of the verb means the lamp always ends up on,");
    puts("--          and the format string prints 'Your light is nowon'.");
    puts("--   SOUTH  there is a case's' but no case'sout', so 'south' does");
    puts("--          nothing while 'north' works.  Commands match on four");
    puts("--          characters -- geta() returned one 36-bit word.");
    puts("--   READ   case'read' has no goto at its end, so reading anything");
    puts("--          falls straight through into the teleport verb.");
    printf("-- -fix is currently %s.\n", opt_fix ? "ON -- all four repaired"
                                                 : "off -- all four in force");
    puts("-- Not repaired by -fix, because they are absence rather than bugs:");
    puts("--   any word the truncated source never handled is still silent,");
    puts("--   and there are still no objects to take.");
}

static void mars_map(void)
{
    int i, j;
    for (i = 0; i <= MARS_MAXROOM; i++) {
        if (!mars_known[i]) continue;
        printf("-- %3d %-6s %-46s", i,
               mars_brief[i] ? "brief" : (mars_long[i] ? "long" : "-"),
               mars_short[i] ? mars_short[i] : "(no name)");
        for (j = 1; j <= 10; j++)
            if (mars_exits[i][j]) printf(" %s%d", dirname_[j], mars_exits[i][j]);
        putchar('\n');
    }
}

static void play_mars(void)
{
    char *com, *arg;
    int q, prev = 0, darkwarned = 0;

    r = 256; orr = 0; robot = 0; alien = 0; charge = 0;
    /* QUIRK-LIGHT: light is an undeclared extern, so it is 0 and the game
       opens in the dark on a sunlit desert.  -fix starts it lit. */
    light = opt_light || opt_fix; dict_ = 0; hintcount = 0;

    puts("Welcome to the game of Mars.");
    puts("Instructions are in jmc/mars/expl");
    puts("To stop type  'quit'");
    note("jmc/mars/expl was not backed up.  Type *notes for what is missing.");
    putchar('\n');

topofloop:
    if (r != orr) { /* groom(r) -- load the room */ }

    if (!light) {
        puts("It is now quite dark, should you proceed, you may die");
        if (!darkwarned && !opt_fix) {
            darkwarned = 1;
            note("light is an uninitialised extern in the B source, so it"
                 " starts at 0 and the game opens in the dark on a sunlit"
                 " desert.  Type 'lamp', or run with -fix.  (Said once; the"
                 " original says the line above every turn, and so does this.)");
        }
        goto midloop;
    }
    mars_look();
    if (robot) puts("There is a robot here.");
    if (alien) puts("Beelzebub is with you.");

midloop:
    prompt("> ");
    if (!getline_()) return;
    com = geta4();
    if (!com) goto midloop;
    po = 0;
    if (r > 490) { note("spmove: the special-movement routine was never"
                        " written into game.v2.txt."); goto midloop; }
    prev = orr;
    orr = r;

sw:
    /* the direction chain, with the same fall-through arithmetic as the B */
    if      (eq(com,"d")||eq(com,"down"))  po = 10;
    else if (eq(com,"up")||eq(com,"u"))    po = 9;
    else if (eq(com,"nw"))                 po = 8;
    else if (eq(com,"west")||eq(com,"w"))  po = 7;
    else if (eq(com,"sw"))                 po = 6;
    /* QUIRK-SOUTH: the source has case's' but no case'sout', so "south"
       does nothing while "north" works.  -fix adds the missing case. */
    else if (eq(com,"s") || (opt_fix && eq(com,"sout"))) po = 5;
    else if (eq(com,"se"))                 po = 4;
    else if (eq(com,"east")||eq(com,"e"))  po = 3;
    else if (eq(com,"ne"))                 po = 2;
    else if (eq(com,"nort")||eq(com,"n"))  po = 1;

    if (po) {
        q = (r <= MARS_MAXROOM) ? mars_exits[r][po] : 0;
        if (q) {
            r = q;
            /* if(!light&&!rand()&07) -- parses as (!light)&&((!rand())&07),
               so it can only fire when rand() returns exactly 0.  Kept. */
            if (!light && ((!rand()) & 07)) {
                puts("You have died.");
                return;
            }
        } else {
            puts("There is no way to go that direction");
        }
        goto topofloop;
    }

    if (eq(com,"look")) { prlon(); goto topofloop; }

    if (eq(com,"inve")) {
        puts("You are currently holding the following:");
        note("the inventory string is empty: no object table survives.");
        goto topofloop;
    }

    if (eq(com,"take") || eq(com,"get")) {
        /* trehere is empty, so the length(trehere)!=1 test holds and an
           empty argument sends the original back to try: for another word */
        arg = geta4();
        if (!arg) { prompt("Get what? "); arg = geta4_more(); }
        if (!arg) return;
        puts("I see none of that here.");     /* any(scant(arg), "") == 0 */
        goto midloop;
    }

    if (eq(com,"drop") || eq(com,"rele")) {
        arg = geta4();
        if (!arg) { prompt("Drop what? "); arg = geta4_more(); }
        if (!arg) return;
        puts("You are not holding it.");
        goto midloop;
    }

    if (eq(com,"ligh") || eq(com,"lamp") || eq(com,"flas")) {
        arg = geta4();
        if (opt_fix) {
            /* QUIRK-LAMP repaired: what the four statements below were
               plainly meant to do. */
            if (!arg) light = !light;
            else if (eq(arg,"on")) {
                if (light) puts("It already was"); else light = 1;
            } else if (eq(arg,"off")) {
                if (!light) puts("It already was"); else light = 0;
            }
            printf("Your light is now %s\n", light ? "on" : "off");
        } else {
            /* QUIRK-LAMP: transliterated exactly, dangling elses and all.
               The unconditional "if(light)...else light=1" in the middle
               means the lamp always ends up on, and the format string is
               missing the space before on/off. */
            if (!arg) { if (light) light = 0; else light = 1; }
            else if (eq(arg,"on")) { /* empty statement in the original */ }
            if (light) puts("It already was");
            else light = 1;
            if (eq(arg,"off")) { if (light) light = 0; else puts("It already was"); }
            fputs("Your light is now", stdout);
            if (light) puts("on"); else puts("off");
        }
        goto midloop;
    }

    if (eq(com,"read")) {
        arg = geta4();
        if (!arg) { prompt("Read what? "); arg = geta4_more(); }
        if (!arg) return;
        /* cn = scant(arg): with no vocabulary table this can only come back
           0, and no.things is 0 too, so "cn > no.things" is false and the
           "I don't understand that" branch is never taken. */
        if (eq(arg,"dict")) {
            fputs("You aren't carrying it", stdout);   /* no *n in the original */
            goto topofloop;
        }
        /* dict is 0, so: */
        puts("I can't read that, it's all Old High Martian to me!");
        /* QUIRK-READ: case'read' has no goto or break at its end, so it
           falls through into case'mt' below. */
        if (opt_fix) goto topofloop;
        goto case_mt;
    }

    if (eq(com,"mt") || eq(com,"tele") || eq(com,"zap")) {
case_mt:
        puts("You are not carrying the transmitter");
        goto topofloop;
    }

    if (eq(com,"swim")) { puts("I don't know how!"); goto topofloop; }
    if (eq(com,"lear")) { puts("I am incapable of learning"); goto topofloop; }

    if (eq(com,"say")) {
        fputs("OK, ", stdout);
        while (*lp && *lp != '\n') putchar(*lp++);
        putchar('\n');
        goto topofloop;
    }

    if (eq(com,"eat")) {
        puts("You don't have any food.  Whew!!");
        goto topofloop;
    }

    if (eq(com,"walk")) { com = geta4(); if (com) goto sw; goto midloop; }

    if (eq(com,"drin")) {
        puts("I wouldn't advise drinking the Martian water");
        goto topofloop;
    }

    if (eq(com,"kill") || eq(com,"atta") || eq(com,"dest")) {
        arg = geta4();
        if (eq(arg,"alie")) {
            puts("He is not here");
            goto topofloop;
        }
        note("game.v2.txt ends here, in the middle of case'kill'.  Nothing"
             " after 'kill alien' was ever written.");
        goto midloop;
    }

    /* -------- everything below this line is mine, not the archive's ----- */
    if (eq(com,"quit") || eq(com,"*qui")) return;
    if (eq(com,"*exi")) { show_exits(); goto midloop; }
    if (eq(com,"*map")) { mars_map(); goto midloop; }
    if (eq(com,"*not")) { mars_notes(); goto midloop; }
    if (eq(com,"*bac")) { if (prev) { r = prev; goto topofloop; } goto midloop; }
    if (eq(com,"*got")) {
        arg = geta();
        if (arg) { r = atoi(arg); orr = -1; goto topofloop; }
        goto midloop;
    }

    note("the truncated source has no case for that word, so the original"
         " would have silently ignored it.");
    goto midloop;
}

/* ================================================================ DEIMOS */
/*
 * jmc/mars/indat.v2.txt: 63 long room descriptions for the second region --
 * the Martian underground and the interior of Deimos.  There is no goto file
 * for these rooms in the archive, so they cannot be walked.  Inventing a
 * connection graph from the prose would be writing the game, not recovering
 * it, so this is a reader, not a map.
 */
static void play_deimos(void)
{
    int n = 1;
    char *w;

    puts("-- jmc/mars/indat.v2.txt, 63 rooms.  These have descriptions but no");
    puts("-- connection table -- the goto file for this region was not backed");
    puts("-- up, so the rooms cannot be joined without making things up.");
    puts("-- Commands: a number, 'n' next, 'p' previous, 'q' back to the menu.");
    putchar('\n');

    for (;;) {
        if (n >= 1 && n <= DEI_MAXROOM && dei_long[n]) {
            printf("-- room %d\n", n);
            wrap(dei_long[n]);
            if (dei_restored[n])
                note("one line of this room was missing from indat.v2 and has"
                     " been restored from indat.v1; see RECOVERY.md.");
        } else {
            printf("-- there is no room %d in indat.v2.txt\n", n);
        }
        prompt("\nindat> ");
        if (!getline_()) return;
        w = geta();
        if (!w) { n++; continue; }
        if (*w == 'q') return;
        if (*w == 'n') { n++; continue; }
        if (*w == 'p') { n--; continue; }
        if (isdigit((unsigned char)*w)) n = atoi(w);
        if (n < 1) n = 1;
        if (n > DEI_MAXROOM) n = DEI_MAXROOM;
    }
}

/* ================================================== THE MATH BUILDING DSL */
/*
 * jmc/mars/math.v2.txt is the one surviving program written in the adventure
 * definition language Mark Niemiec built for the Honeywell.  gendata.py plays
 * the part of his compiler; this is the part his compiler emitted.
 *
 * Two typos in the source make the MIDJET puzzle unwinnable as written:
 * m2ndfoyer's west exit names "widjet" but the room is called "midjet", and
 * logging on sets onmidjet while crash tests onwidjet.  Both are left alone
 * unless you pass -fix.
 */
enum { OP_END, OP_PRINT, OP_PROMPT, OP_MOVE, OP_SET, OP_GETSTR,
       OP_JNEVAR, OP_JNEARG, OP_JEQARG, OP_JMP };

static int mvar[MATH_NVAR];
static char marg[128];

static int math_findroom(const char *name)
{
    int i;
    for (i = 0; i < MATH_NROOM; i++)
        if (!strcmp(math_room[i].name, name)) return i;
    return -1;
}

static int math_var(int v)
{
    if (opt_fix && !strcmp(math_varname[v], "onwidjet")) {
        int i;
        for (i = 0; i < MATH_NVAR; i++)
            if (!strcmp(math_varname[i], "onmidjet")) return i;
    }
    return v;
}

/* returns the room to move to, or -1 to stay put */
static int math_run(int pc)
{
    for (;;) {
        switch (math_code[pc]) {
        case OP_END:
            return -1;
        case OP_PRINT:
            wrap(math_str[math_code[pc+1]]);
            pc += 2; break;
        case OP_PROMPT:
            prompt(math_str[math_code[pc+1]]);
            pc += 2; break;
        case OP_SET:
            mvar[math_var(math_code[pc+1])] = math_code[pc+2];
            pc += 3; break;
        case OP_GETSTR:
            if (!getline_()) exit(0);
            { char *w = geta();
              marg[0] = 0;
              if (w) { strncpy(marg, w, sizeof marg - 1); marg[sizeof marg - 1] = 0; } }
            pc += 1; break;
        case OP_JNEVAR:
            if (mvar[math_var(math_code[pc+1])] != math_code[pc+2])
                pc = math_code[pc+3];
            else pc += 4;
            break;
        case OP_JEQARG:
            if (!strcmp(marg, math_str[math_code[pc+1]])) pc = math_code[pc+2];
            else pc += 3;
            break;
        case OP_JNEARG:
            if (strcmp(marg, math_str[math_code[pc+1]])) pc = math_code[pc+2];
            else pc += 3;
            break;
        case OP_JMP:
            pc = math_code[pc+1]; break;
        case OP_MOVE: {
            int ref = math_code[pc+1];
            int dst = math_ref[ref];
            if (dst < 0 && opt_fix && !strcmp(math_refname[ref], "widjet"))
                dst = math_findroom("midjet");
            if (dst < 0) {
                printf("-- that way is %s, which is not in the surviving"
                       " fragment.\n", math_refname[ref]);
                return -1;
            }
            return dst;
        }
        default:
            note("bad opcode"); return -1;
        }
    }
}

static int math_match(const MClause *c, const char *w)
{
    char pat[64];
    size_t n = strlen(w);
    if (n > 60) return 0;
    pat[0] = '|';
    memcpy(pat + 1, w, n);
    pat[n+1] = '|'; pat[n+2] = 0;
    if (strstr(c->w, pat)) return 1;
    if (opt_fix) {                       /* the source mixes up/u and d/down */
        const char *alias = NULL;
        if (!strcmp(w, "up")) alias = "|u|";
        else if (!strcmp(w, "u")) alias = "|up|";
        else if (!strcmp(w, "down") || !strcmp(w, "dn")) alias = "|d|";
        else if (!strcmp(w, "d")) alias = "|down|";
        if (alias && strstr(c->w, alias)) return 1;
    }
    return 0;
}

static void math_notes(void)
{
    int i, j;
    puts("-- jmc/mars/math.v2.txt -- the Math building, written in Mark");
    puts("-- Niemiec's adventure definition language.  This is the only");
    puts("-- program in that language anywhere in the archive; the compiler");
    puts("-- itself was not backed up.");
    printf("-- %d rooms defined.  Rooms named but never written: ", MATH_NROOM);
    for (i = 0; i < MATH_NREF; i++)
        if (math_ref[i] < 0) printf("%s ", math_refname[i]);
    putchar('\n');
    puts("-- Two typos in the source make the MIDJET puzzle unwinnable:");
    puts("--   m2ndfoyer says  w : nw : widjet;  but the room is 'midjet'");
    puts("--   logon sets onmidjet, crash tests onwidjet");
    puts("-- Run with -fix to repair both (and to alias up/u and d/down,");
    puts("-- which the source also uses inconsistently).");
    printf("-- -fix is currently %s.\n", opt_fix ? "ON" : "off");
    puts("-- Verbs, per room, exactly as the source defines them:");
    for (i = 0; i < MATH_NROOM; i++) {
        printf("--   %-13s", math_room[i].name);
        for (j = math_room[i].c0; j < math_room[i].c0 + math_room[i].cn; j++) {
            char b[128];
            size_t k;
            strncpy(b, math_clause[j].w, sizeof b - 1);
            b[sizeof b - 1] = 0;
            for (k = 0; b[k]; k++) if (b[k] == '|') b[k] = ' ';
            printf("%s", b);
        }
        putchar('\n');
    }
}

static void play_math(void)
{
    int here = math_findroom("mentrance"), i, dst;
    char seen[MATH_NROOM];
    char *w;

    memset(seen, 0, sizeof seen);
    memset(mvar, 0, sizeof mvar);

    puts("-- The Math building, from jmc/mars/math.v2.txt.");
    puts("-- Type *notes for what the source does and does not contain,");
    puts("-- *verbs for the words this room understands, *quit to leave.");
    putchar('\n');

    for (;;) {
        const MRoom *R = &math_room[here];
        if (seen[here] && R->brf[0]) wrap(R->brf);
        else wrap(R->lng);
        seen[here] = 1;

        prompt("\n> ");
        if (!getline_()) return;
        w = geta();
        if (!w) continue;

        if (!strcmp(w, "*quit") || !strcmp(w, "quit")) return;
        if (!strcmp(w, "*notes")) { math_notes(); continue; }
        if (!strcmp(w, "look")) { wrap(R->lng); continue; }
        if (!strcmp(w, "*verbs")) {
            printf("-- ");
            for (i = R->c0; i < R->c0 + R->cn; i++) printf("%s ", math_clause[i].w);
            putchar('\n');
            continue;
        }

        dst = -1;
        for (i = R->c0; i < R->c0 + R->cn; i++) {
            if (math_match(&math_clause[i], w)) {
                dst = math_run(math_clause[i].pc);
                break;
            }
        }
        if (i == R->c0 + R->cn)
            note("this room defines no clause for that word.");
        if (dst >= 0) here = dst;
    }
}

/* ================================================================== main */

static void banner(void)
{
puts("");
puts("        M A R T I A N   A D V E N T U R E   --   r e c o v e r e d");
puts("");
puts("  Waterloo, Honeywell 6000, 1978-79.  Brad Templeton, with the");
puts("  adventure definition language by Mark Niemiec.");
puts("");
puts("  The game was never finished and the archive holds only part of it.");
puts("  What is here is real: every word of game text below came out of the");
puts("  1978-79 files.  Lines beginning '--' and commands beginning '*' are");
puts("  this program talking, not the archive.");
puts("");
puts("  1  Mars        the desert, the airlock and the domed city.");
puts("                 76 reachable rooms, real map.  No objects survive.");
puts("  2  Deimos      63 room descriptions with no map.  A reader.");
puts("  3  Math        the Math building, in Mark's adventure language.");
puts("                 15 rooms, real puzzle logic, fully playable.");
puts("  4  Notes       what survived, what did not.");
puts("  q  quit");
puts("");
printf("  -fix is %s.  It repairs the authors' own bugs -- the dark start,\n",
       opt_fix ? "ON" : "off");
puts("  the lamp verb, the missing 'south', read falling into teleport, and");
puts("  the two typos that make the Math building's puzzle unwinnable.");
puts("  Without it you get all of them exactly as they were written.");
puts("");
}

static void overall_notes(void)
{
puts("-- The archive (jmc/mars, plus jmc/marsgame.b) contains:");
puts("--   game.v1/v2.txt  5106/5108 bytes of B, breaking off in mid-verb");
puts("--   exter           the extern list the B source includes");
puts("--   goto            exit table, 79 rooms");
puts("--   snames          short room names, 80 rooms");
puts("--   lnames          long descriptions, 15 rooms");
puts("--   indat.v1/v2     long descriptions for a second region, 36 / 63");
puts("--   math.v1/v2      the Math building in the adventure language");
puts("--   polar/          a duplicate of indat.v2 and lnames, byte for byte");
puts("--   marsgame.b      a printout of game.v2 with a 'bstempleton' header");
puts("-- It does not contain:");
puts("--   the compiled desc and rooms binaries the B program opens");
puts("--   jmc/mars/expl, the instructions the B program points you at");
puts("--   any object, treasure or vocabulary table");
puts("--   the connection table for the indat region");
puts("--   the second half of game.b, from case'kill' onward");
puts("--   the adventure language compiler itself");
puts("--   the well house, the keyboard, the rocket, the Viking landing");
puts("--     sequence, Bottomos's hole and the ray gun that Brad describes");
puts("-- The Viking lander, the boulder, the four-inch hole bored through it");
puts("-- and the welcome sign are all in lnames, rooms 4, 5 and 6.");
}

int main(int argc, char **argv)
{
    int i;
    char *w;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-fix")) opt_fix = 1;
        else if (!strcmp(argv[i], "-light")) opt_light = 1;
        else {
            fprintf(stderr,
                "usage: %s [-fix] [-light]\n"
                "  -fix    repair the authors' bugs rather than reproduce them:\n"
                "          Mars   -- the dark start, the lamp verb, missing\n"
                "                    case'sout', read falling into teleport\n"
                "          Math   -- widjet/midjet, onwidjet/onmidjet, up/u\n"
                "  -light  start the Mars lamp on, without the other repairs\n",
                argv[0]);
            return 1;
        }
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    banner();
    for (;;) {
        prompt("choose> ");
        if (!getline_()) return 0;
        w = geta();
        if (!w) continue;
        switch (*w) {
        case '1': play_mars();   break;
        case '2': play_deimos(); break;
        case '3': play_math();   break;
        case '4': overall_notes(); break;
        case 'q': return 0;
        default:  banner();      break;
        }
        putchar('\n');
    }
}
