/*
 * newadv -- a walkable map of Mark Niemiec's "New Adventure" (Waterloo, 1979).
 *
 * This is NOT the game.  The game's engine (games/f/include/main), its object
 * behaviour (games/f/newadv/obj.f) and Niemiec's F compiler are all lost; what
 * survives on the archive tape is the vocabulary, the creature events, and the
 * complete set of 162 locations with their descriptions and travel tables.
 *
 * So this walks the map and nothing else.  Every description, every refusal
 * message and every exit below comes out of loc.f verbatim -- see data.h and
 * gendata.py.  Where a room's behaviour depends on game state the walker does
 * not model (a lit lamp, an opened grate, the state of the acorn), it says so
 * and shows you the original F source rather than guessing at it.
 *
 * Build:  sh build.sh          Run:  ./newadv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"

#define WRAP 76

static int here;
static int brief_mode = 0;
static char visited[NROOMS];
static const Exit *pending;          /* last guarded exit we refused */

/* ------------------------------------------------------------------ */

static void wrap(const char *s, const char *indent)
{
    int col = 0;
    printf("%s", indent);
    while (*s) {
        const char *w = s;
        int len = 0;
        if (*s == '\n') {
            printf("\n%s", indent);
            col = 0;
            s++;
            continue;
        }
        while (s[len] && s[len] != ' ' && s[len] != '\n')
            len++;
        if (col && col + 1 + len > WRAP) {
            printf("\n%s", indent);
            col = 0;
        } else if (col) {
            putchar(' ');
            col++;
        }
        fwrite(w, 1, len, stdout);
        col += len;
        s += len;
        while (*s == ' ')
            s++;
    }
    if (col)
        putchar('\n');
}

static void show_source(const char *src)
{
    const char *p = src;
    while (*p) {
        const char *e = strchr(p, '\n');
        int n = e ? (int)(e - p) : (int)strlen(p);
        printf("    | %.*s\n", n, p);
        if (!e)
            break;
        p = e + 1;
    }
}

static int eqi(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static int contains(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    if (!hay || !n)
        return 0;
    for (; *hay; hay++) {
        size_t i;
        for (i = 0; i < n; i++)
            if (toupper((unsigned char)hay[i]) != toupper((unsigned char)needle[i]))
                break;
        if (i == n)
            return 1;
    }
    return 0;
}

/* Long names for the compass, so you can type "northeast" as well as "ne".
 * The original synonym table lived in the lost games/f/include/vocab; these
 * are a convenience of this walker, not recovered data. */
static const char *const longname[][2] = {
    {"NORTH", "N"}, {"SOUTH", "S"}, {"EAST", "E"}, {"WEST", "W"},
    {"NORTHEAST", "NE"}, {"NORTHWEST", "NW"},
    {"SOUTHEAST", "SE"}, {"SOUTHWEST", "SW"},
    {"UP", "U"}, {"DOWN", "D"},
    {"ENTER", "IN"}, {"EXIT", "OUT"}, {"LEAVE", "OUT"},
    {NULL, NULL}
};

static const char *canon(const char *w)
{
    int i;
    for (i = 0; longname[i][0]; i++)
        if (eqi(w, longname[i][0]))
            return longname[i][1];
    return w;
}

static int findroom(const char *name)
{
    int i;
    for (i = 0; i < NROOMS; i++)
        if (eqi(name, rooms[i].name))
            return i;
    return -1;
}

/* ------------------------------------------------------------------ */

static void describe(int full)
{
    const Room *r = &rooms[here];
    int i;

    printf("\n== %s ==%s\n", r->name, r->lit ? "" : "   (no LIGHT in Furnish)");
    if (!r->built) {
        /* loc.f has no block for this location, so the global default in
         * locexec() applies -- and it bounces you back to the well house. */
        wrap(under_construction, "");
        return;
    }
    if (!full && r->brief)
        wrap(r->brief, "");
    else if (r->look)
        wrap(r->look, "");
    else {
        /* the four rooms whose Look is a conditional */
        for (i = 0; i < r->nac; i++)
            if (!strcmp(r->ac[i].label, "Look")) {
                printf("  [Look depends on game state; loc.f says:]\n");
                show_source(r->ac[i].src);
            }
    }
    for (i = 0; i < r->nent; i++)
        printf("  [on entry: %s -- \"src %s\" to see it]\n",
               r->ent[i].label, r->ent[i].label);
}

static void list_exits(void)
{
    const Room *r = &rooms[here];
    char done[64];
    int i, j;

    memset(done, 0, sizeof done);
    if (!r->nex) {
        printf("  (loc.f gives this room no travel table)\n");
        return;
    }
    for (i = 0; i < r->nex && i < (int)sizeof done; i++) {
        const Exit *e = &r->ex[i];
        if (done[i])
            continue;
        printf("  ");
        for (j = i; j < r->nex && j < (int)sizeof done; j++) {
            const Exit *f = &r->ex[j];
            if (f->dest != e->dest || f->ovr != e->ovr || f->cond != e->cond)
                continue;
            done[j] = 1;
            printf(f->wneg ? "(%s) " : "%s ", f->word);
        }
        if (e->dest >= 0)
            printf("-> %s%s\n", e->dneg ? "-" : "", rooms[e->dest].name);
        else if (e->msg) {
            printf("-> refused:\n");
            wrap(e->msg, "        ");
        } else {
            printf("-> guarded");
            if (e->nalts) {
                printf(" (");
                for (j = 0; j < e->nalts; j++)
                    printf("%s%s", j ? " or " : "", rooms[e->alts[j]].name);
                printf(")");
            }
            printf("\n");
        }
    }
}

static void enter(int n)
{
    here = n;
    describe(!brief_mode || !visited[n]);
    visited[n] = 1;
    pending = NULL;
}

/* ------------------------------------------------------------------ */

static int prev[NROOMS];

static int bfs(int from, int use_guarded)
{
    int queue[NROOMS], head = 0, tail = 0, i, j;
    for (i = 0; i < NROOMS; i++)
        prev[i] = -2;
    prev[from] = -1;
    queue[tail++] = from;
    while (head < tail) {
        const Room *r = &rooms[queue[head++]];
        for (i = 0; i < r->nex; i++) {
            const Exit *e = &r->ex[i];
            if (e->dest >= 0) {
                if (prev[e->dest] == -2) {
                    prev[e->dest] = queue[head - 1];
                    queue[tail++] = e->dest;
                }
            } else if (use_guarded) {
                for (j = 0; j < e->nalts; j++)
                    if (prev[e->alts[j]] == -2) {
                        prev[e->alts[j]] = queue[head - 1];
                        queue[tail++] = e->alts[j];
                    }
            }
        }
    }
    return tail;
}

static void show_path(int to)
{
    int stack[NROOMS], n = 0, i;
    if (prev[to] == -2) {
        printf("  no route using the exits loc.f gives.\n");
        return;
    }
    for (i = to; i != -1; i = prev[i])
        stack[n++] = i;
    printf("  %d step%s:", n - 1, n == 2 ? "" : "s");
    while (n--)
        printf(" %s%s", rooms[stack[n]].name, n ? " ->" : "");
    printf("\n");
}

static void dump_dot(const char *fn)
{
    FILE *f = fopen(fn, "w");
    int i, j;
    if (!f) {
        printf("  cannot write %s\n", fn);
        return;
    }
    fprintf(f, "digraph newadv {\n  node [shape=box,fontsize=9];\n");
    for (i = 0; i < NROOMS; i++) {
        fprintf(f, "  %s [%s%s];\n", rooms[i].name,
                i >= FIRST_CAVE ? "style=filled,fillcolor=lightgrey"
                                : "style=filled,fillcolor=lightyellow",
                rooms[i].lit ? "" : ",color=red");
        /* one edge per destination, with every motion word that reaches it */
        for (j = 0; j < rooms[i].nex; j++) {
            const Exit *e = &rooms[i].ex[j];
            int to = e->dest >= 0 ? e->dest : (e->nalts ? e->alts[0] : -1);
            int k, first = 1;
            if (to < 0)
                continue;
            for (k = 0; k < j; k++) {
                const Exit *p = &rooms[i].ex[k];
                int pt = p->dest >= 0 ? p->dest : (p->nalts ? p->alts[0] : -1);
                if (pt == to && (p->dest >= 0) == (e->dest >= 0))
                    first = 0;
            }
            if (!first)
                continue;
            fprintf(f, "  %s -> %s [label=\"", rooms[i].name, rooms[to].name);
            for (k = j; k < rooms[i].nex; k++) {
                const Exit *p = &rooms[i].ex[k];
                int pt = p->dest >= 0 ? p->dest : (p->nalts ? p->alts[0] : -1);
                if (pt == to && (p->dest >= 0) == (e->dest >= 0))
                    fprintf(f, "%s%s", k == j ? "" : " ", p->word);
            }
            fprintf(f, "\"%s];\n", e->dest >= 0 ? "" : ",style=dashed");
        }
    }
    fprintf(f, "}\n");
    fclose(f);
    printf("  wrote %s\n", fn);
}

/* ------------------------------------------------------------------ */

static void help(void)
{
    puts("\n  Movement   type an exit word: n s e w ne nw se sw u d in out back,");
    puts("             climb, jump, or a noun the room's travel table names");
    puts("             (road, building, cavern, ...).  \"exits\" lists them.");
    puts("  exits x    the room's travel table, as loc.f gives it");
    puts("  look l     long description      brief / verbose   description mode");
    puts("  read       the room's Inread text (runes, signs, inscriptions)");
    puts("  acts       other verbs loc.f defines here");
    puts("  src LABEL  the original F source for one label in this room");
    puts("  furn       the room's Furnish list");
    puts("  force [n]  take a guarded exit anyway (state is not modelled)");
    puts("  go ROOM    jump straight to a room by name");
    puts("  path ROOM  shortest route from here using unguarded exits");
    puts("  rooms      list all 162 locations       find TEXT   search descriptions");
    puts("  reach      reachability report from the start");
    puts("  dot FILE   write a graphviz map");
    puts("  quit");
}

static void acts(void)
{
    const Room *r = &rooms[here];
    int i;
    if (!r->nac && !r->nent) {
        printf("  loc.f defines no other verbs here.\n");
        return;
    }
    for (i = 0; i < r->nac; i++)
        printf("  %-10s %s\n", r->ac[i].label,
               r->ac[i].msg ? "(message)" : "(code)");
    for (i = 0; i < r->nent; i++)
        printf("  %-10s (on entry)\n", r->ent[i].label);
}

static void source(const char *label)
{
    const Room *r = &rooms[here];
    int i, n = 0;
    for (i = 0; i < r->nac; i++)
        if (eqi(label, r->ac[i].label)) {
            show_source(r->ac[i].src);
            n++;
        }
    for (i = 0; i < r->nent; i++)
        if (eqi(label, r->ent[i].label)) {
            show_source(r->ent[i].src);
            n++;
        }
    for (i = 0; i < r->nex; i++)
        if (r->ex[i].cond && eqi(label, r->ex[i].word)) {
            show_source(r->ex[i].cond);
            n++;
        }
    if (!n)
        printf("  no label \"%s\" in %s -- try \"acts\".\n", label, r->name);
}

static void move_by(const char *word)
{
    const Room *r = &rooms[here];
    int i;
    for (i = 0; i < r->nex; i++) {
        const Exit *e = &r->ex[i];
        if (!eqi(word, e->word))
            continue;
        if (e->dest >= 0) {
            enter(e->dest);
            return;
        }
        if (e->msg) {
            wrap(e->msg, "");
            return;
        }
        printf("  [guarded: this exit depends on game state the walker does not\n"
               "   model.  loc.f says:]\n");
        show_source(e->cond);
        pending = e;
        if (e->nalts)
            printf("  [\"force\" to go anyway%s]\n",
                   e->nalts > 1 ? ", \"force 2\" for the other branch" : "");
        return;
    }
    printf("  loc.f gives %s no exit for \"%s\".  Try \"exits\".\n",
           r->name, word);
}

int main(int argc, char **argv)
{
    char line[256], *cmd, *arg;
    int n, echo = 0;

    for (n = 1; n < argc; n++)
        if (!strcmp(argv[n], "-e"))
            echo = 1;                 /* echo commands: for scripted demos */

    puts("New Adventure -- walkable map");
    puts("Mark D. Niemiec, University of Waterloo, 1979 (Copyright (C) 1979).");
    puts("");
    puts("The engine and the object code are lost; this walks the 162 recovered");
    puts("locations only.  Nothing here is invented -- \"src\" shows you the");
    puts("original F source behind anything the walker will not simulate.");
    puts("Type \"help\" for commands.");

    enter(START_LOC);

    for (;;) {
        printf("\n%s> ", rooms[here].name);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin))
            break;
        line[strcspn(line, "\r\n")] = 0;
        if (echo)
            printf("%s\n", line);
        cmd = line;
        while (*cmd == ' ')
            cmd++;
        arg = cmd;
        while (*arg && *arg != ' ')
            arg++;
        if (*arg) {
            *arg++ = 0;
            while (*arg == ' ')
                arg++;
        }
        if (!*cmd)
            continue;

        if (eqi(cmd, "quit") || eqi(cmd, "q"))
            break;
        else if (eqi(cmd, "help") || eqi(cmd, "?"))
            help();
        else if (eqi(cmd, "look") || eqi(cmd, "l"))
            describe(1);
        else if (eqi(cmd, "brief")) {
            brief_mode = 1;
            puts("  brief descriptions after the first visit.");
        } else if (eqi(cmd, "verbose")) {
            brief_mode = 0;
            puts("  long descriptions every time.");
        } else if (eqi(cmd, "exits") || eqi(cmd, "x"))
            list_exits();
        else if (eqi(cmd, "read") || eqi(cmd, "r")) {
            if (rooms[here].inread)
                wrap(rooms[here].inread, "");
            else
                puts("  loc.f gives this room no Inread text.");
        } else if (eqi(cmd, "acts"))
            acts();
        else if (eqi(cmd, "src")) {
            if (*arg)
                source(arg);
            else
                puts("  src WHAT?  \"acts\" lists the labels.");
        } else if (eqi(cmd, "furn")) {
            const Room *r = &rooms[here];
            int i;
            if (!r->nfurn)
                puts("  loc.f gives this room no Furnish list.");
            else {
                printf("  furnish(");
                for (i = 0; i < r->nfurn; i++)
                    printf("%s%s", i ? "," : "", r->furn[i]);
                printf(");\n");
            }
        } else if (eqi(cmd, "force")) {
            int k = *arg ? atoi(arg) - 1 : 0;
            if (!pending || !pending->nalts)
                puts("  nothing to force.");
            else if (k < 0 || k >= pending->nalts)
                printf("  that exit has %d branch%s.\n", pending->nalts,
                       pending->nalts == 1 ? "" : "es");
            else {
                puts("  [forced -- the original would have tested the condition above]");
                enter(pending->alts[k]);
            }
        } else if (eqi(cmd, "go")) {
            n = findroom(arg);
            if (n < 0)
                printf("  no location named \"%s\".\n", arg);
            else
                enter(n);
        } else if (eqi(cmd, "path")) {
            n = findroom(arg);
            if (n < 0)
                printf("  no location named \"%s\".\n", arg);
            else {
                bfs(here, 0);
                if (prev[n] == -2) {
                    bfs(here, 1);
                    if (prev[n] != -2)
                        puts("  (only by way of guarded exits)");
                }
                show_path(n);
            }
        } else if (eqi(cmd, "rooms")) {
            int i;
            for (i = 0; i < NROOMS; i++)
                printf("  %-14s%s", rooms[i].name, (i % 5) == 4 ? "\n" : "");
            if (NROOMS % 5)
                putchar('\n');
        } else if (eqi(cmd, "find")) {
            int i, n2 = 0;
            if (!*arg) {
                puts("  find WHAT?");
                continue;
            }
            for (i = 0; i < NROOMS; i++)
                if (contains(rooms[i].look, arg)
                    || contains(rooms[i].brief, arg)
                    || contains(rooms[i].inread, arg)) {
                    printf("  %s\n", rooms[i].name);
                    n2++;
                }
            printf("  %d room%s.\n", n2, n2 == 1 ? "" : "s");
        } else if (eqi(cmd, "reach")) {
            int i, a, b;
            a = bfs(START_LOC, 0);
            printf("  from %s using unguarded exits: %d of %d locations\n",
                   rooms[START_LOC].name, a, NROOMS);
            for (i = 0; i < NROOMS; i++)
                if (prev[i] == -2)
                    printf("    unreached: %s\n", rooms[i].name);
            b = bfs(START_LOC, 1);
            printf("  counting guarded exits too: %d of %d\n", b, NROOMS);
            for (i = 0; i < NROOMS; i++)
                if (prev[i] == -2)
                    printf("    still unreached: %s\n", rooms[i].name);
            puts("  (locexec() is not the only way in: ISLAND and SEA are entered");
            puts("   from vehexec()'s RAFT block in loc.f, and the REPOS_* rooms");
            puts("   from endgame() in adv.f.)");
        } else if (eqi(cmd, "dot"))
            dump_dot(*arg ? arg : "newadv.dot");
        else
            move_by(canon(cmd));
    }
    puts("");
    return 0;
}
