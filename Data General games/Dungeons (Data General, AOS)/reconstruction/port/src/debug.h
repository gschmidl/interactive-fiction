/* debug.h -- the -g debug verbs.
 *
 * A console line beginning with '#' is answered here and never reaches the
 * game's parser, so the original binary stays untouched: buggy or unfinished
 * rooms and objects are reached by writing the game's own variables, not by
 * patching its code.
 *
 * The two variables that matter are found by watching memory change across a
 * turn, and are recorded in ROOM_VAR / OBJ_TAB.  #set retargets them if a
 * different revision ever turns up.
 *
 * Included by cpu.c, which supplies M, AC, C, PC, SP, FP, word and AMASK.
 */
#ifndef THISSALA_DEBUG_H
#define THISSALA_DEBUG_H

/* 01D5 is the only persistent word that tracks the player's room (0x2D1 and
 * 0x1D6 shadow it), found by diffing memory across a turn.
 *
 * The object arrays are the ones ASSIST:6 prints as OBJECT[o,1..3], one word
 * per object indexed by the object number itself:
 *
 *     27EC + o   the room the object is in (0 = carried, -1 = not in play)
 *     29EC + o   flags, with the object's state in the top three bits
 *
 * Objects run 1..211.  Take the numbers from ASSIST:6, not from arithmetic on
 * THISSALA.DB6 string numbers -- DB6's stale record tails make that come out
 * 17 too high (the square dowel is object 152, not 169). */
/* The game has its own debug suite -- ASSIST (a 29-item menu), EXPRESS,
 * MTBL -- gated on the word at 01C7.  Node 0 overlay 0 sets that flag during
 * startup and then clears it again unless the AOS/VS user name is $$DAVE,
 * $PAUL or $PETER, i.e. unless you are David Auerbach, Paul Chiasson or Peter
 * Macaulay.  When the flag is zero the parser answers those verbs with
 * "I don't understand the word", which is why they look absent.
 *
 * -g re-asserts the flag before every console read: the game's init has long
 * since run by then, and writing it again each turn costs nothing and
 * survives anything that clears it.  Nothing is patched -- this is the value
 * the authors' own logins produced. */
#define DEBUG_FLAG 0x01C7u

static int  debug;
static word room_var  = 0x01D5;   /* current room number                   */
static word obj_tab   = 0x27EC;   /* object -> room, indexed by object no. */
static word obj_first = 1;        /* number of the first object in the tab */
static word obj_count = 211;      /* 27ED..28BF                            */

/* OBJECT[o,2] -- the word assist:6 prints -- lives at OBJ_STATE + o, and the
 * object's state is its top three bits.  Found by opening the post office box
 * (object 84): 2A40 went 1280 -> 22B0, and every state-1 object in the
 * authors' saved game has 2000 set while every state-0 one does not.
 *
 * This is what makes ENHOOK possible.  The game's own HOOK verb (225) cannot
 * be typed, so -g offers ENHOOK, which sets the curtains (object 32) to
 * state 1 exactly as the real verb would.  The game then prints its own
 * state-1 text and the passage north opens; nothing is patched, and the
 * game's UNHOOK still undoes it. */
#define OBJ_STATE  0x29ECu
#define OBJ_CURTAINS 32

static unsigned obj_state_get(unsigned o)
{
    return (M[(OBJ_STATE + o) & AMASK] >> 13) & 7;
}

static void obj_state_set(unsigned o, unsigned st)
{
    word *w = &M[(OBJ_STATE + o) & AMASK];
    *w = (word)((*w & 0x1FFFu) | ((st & 7u) << 13));
}

/* Decimal unless prefixed 0x -- room and object numbers are the common
 * case and the game numbers them in decimal. */
static unsigned dbg_num(const char **pp)
{
    const char *p = *pp;
    unsigned v = 0;
    unsigned base = 10;
    while (*p == ' ') p++;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
    for (;;) {
        unsigned d;
        if (*p >= '0' && *p <= '9')                  d = (unsigned)(*p - '0');
        else if (base == 16 && (*p | 32) >= 'a' && (*p | 32) <= 'f')
                                                     d = (unsigned)((*p | 32) - 'a' + 10);
        else break;
        v = v * base + d;
        p++;
    }
    *pp = p;
    return v;
}

static void dbg_word(const char **pp, char *out, size_t n)
{
    const char *p = *pp;
    size_t i = 0;
    while (*p == ' ') p++;
    while (*p && *p != ' ' && i < n - 1) out[i++] = *p++;
    out[i] = 0;
    *pp = p;
}

static void debug_help(void)
{
    fprintf(stderr,
      "  #room                 show the current room number\n"
      "  #goto <n>             put the player in room n\n"
      "  #where <obj>          show where object obj is\n"
      "  #move <obj> <n>       put object obj in room n\n"
      "  #bring <obj>          put object obj in the current room\n"
      "  #peek <addr> [words]  dump memory (addresses are hex: 0x1D5)\n"
      "  #poke <addr> <value>  store one word\n"
      "  #regs                 accumulators, carry, PC, stack\n"
      "  #dump <file>          write the 32K memory image out\n"
      "  #state <obj> [n]      show or set an object's state\n"
      "  #set room|obj <addr>  retarget the two tables\n"
      "  #help                 this list\n"
      "Numbers are decimal; 0x for hex.  Every other line goes to the game.\n"
      "\n"
      "ENHOOK is also available: the game's own HOOK verb cannot be typed, so\n"
      "this supplies it.  Stand in the Library (EXPRESS 2/13) and type ENHOOK.\n");
}

static int dbg_obj_slot(unsigned o, word *slot)
{
    if (!obj_tab) {
        fprintf(stderr, "  no object table configured -- #set obj <addr>\n");
        return 0;
    }
    if (o < obj_first || (obj_count && o >= (unsigned)(obj_first + obj_count))) {
        fprintf(stderr, "  object %u is outside %u..%u\n",
                o, obj_first, obj_first + (obj_count ? obj_count : 1) - 1);
        return 0;
    }
    *slot = (word)((obj_tab + o) & AMASK);
    return 1;
}

static void debug_arm(void)
{
    if (debug) M[DEBUG_FLAG] = 1;
}

static void debug_command(const char *line)
{
    const char *p = line + 1;
    char verb[16];

    dbg_word(&p, verb, sizeof verb);

    if (!verb[0] || !strcmp(verb, "help")) { debug_help(); return; }

    if (!strcmp(verb, "room")) {
        if (!room_var) { fprintf(stderr, "  no room variable -- #set room <addr>\n"); return; }
        fprintf(stderr, "  room %u (%04X), held at %04X\n",
                M[room_var], M[room_var], room_var);
        return;
    }

    if (!strcmp(verb, "goto")) {
        unsigned n = dbg_num(&p);
        if (!room_var) { fprintf(stderr, "  no room variable -- #set room <addr>\n"); return; }
        M[room_var] = (word)n;
        fprintf(stderr, "  room := %u -- LOOK to see it\n", n);
        return;
    }

    if (!strcmp(verb, "where")) {
        unsigned o = dbg_num(&p);
        word slot;
        if (!dbg_obj_slot(o, &slot)) return;
        fprintf(stderr, "  object %u is at %u (slot %04X)\n", o, M[slot], slot);
        return;
    }

    if (!strcmp(verb, "move") || !strcmp(verb, "bring")) {
        unsigned o = dbg_num(&p), where;
        word slot;
        if (!dbg_obj_slot(o, &slot)) return;
        if (!strcmp(verb, "bring")) {
            if (!room_var) { fprintf(stderr, "  no room variable -- #set room <addr>\n"); return; }
            where = M[room_var];
        } else {
            where = dbg_num(&p);
        }
        M[slot] = (word)where;
        fprintf(stderr, "  object %u := room %u\n", o, where);
        return;
    }

    if (!strcmp(verb, "peek")) {
        unsigned a = dbg_num(&p), n = dbg_num(&p), j;
        if (!n) n = 8;
        for (j = 0; j < n; j++) {
            if (!(j & 7)) fprintf(stderr, "  %04X:", (a + j) & AMASK);
            fprintf(stderr, " %04X", M[(a + j) & AMASK]);
            if ((j & 7) == 7) fprintf(stderr, "\n");
        }
        if (n & 7) fprintf(stderr, "\n");
        return;
    }

    if (!strcmp(verb, "poke")) {
        unsigned a = dbg_num(&p), v = dbg_num(&p);
        M[a & AMASK] = (word)v;
        fprintf(stderr, "  %04X := %04X\n", a & AMASK, v);
        return;
    }

    if (!strcmp(verb, "regs")) {
        fprintf(stderr, "  AC %04X %04X %04X %04X  C%d   PC %04X  SP %04X FP %04X\n",
                AC[0], AC[1], AC[2], AC[3], C, PC, SP, FP);
        return;
    }

    if (!strcmp(verb, "dump")) {
        char name[256];
        FILE *o;
        unsigned a;
        dbg_word(&p, name, sizeof name);
        if (!name[0]) { fprintf(stderr, "  #dump <file>\n"); return; }
        o = fopen(name, "wb");
        if (!o) { fprintf(stderr, "  cannot write %s\n", name); return; }
        for (a = 0; a < MEMWORDS; a++) { fputc(M[a] >> 8, o); fputc(M[a] & 0xFF, o); }
        fclose(o);
        fprintf(stderr, "  wrote %s\n", name);
        return;
    }

    if (!strcmp(verb, "state")) {
        unsigned o = dbg_num(&p);
        const char *q = p;
        if (o < 1 || o > 500) { fprintf(stderr, "  #state <obj> [n]\n"); return; }
        while (*q == ' ') q++;
        if (*q) {
            unsigned n = dbg_num(&p);
            obj_state_set(o, n);
            fprintf(stderr, "  object %u state := %u\n", o, obj_state_get(o));
        } else {
            fprintf(stderr, "  object %u is in state %u (word %04X at %04X)\n",
                    o, obj_state_get(o), M[(OBJ_STATE + o) & AMASK],
                    (unsigned)((OBJ_STATE + o) & AMASK));
        }
        return;
    }

    if (!strcmp(verb, "set")) {
        char what[16];
        dbg_word(&p, what, sizeof what);
        if      (!strcmp(what, "room")) room_var = (word)dbg_num(&p);
        else if (!strcmp(what, "obj"))  obj_tab  = (word)dbg_num(&p);
        else { fprintf(stderr, "  #set room|obj <addr>\n"); return; }
        fprintf(stderr, "  room=%04X obj=%04X\n", room_var, obj_tab);
        return;
    }

    fprintf(stderr, "  no debug command '%s' -- try #help\n", verb);
}

/* ENHOOK -- the verb the shipped vocabulary cannot reach.  Returns 1 if the
 * line was consumed here and must not go to the game. */
static int debug_verb(const char *line)
{
    const char *p = line;
    char verb[16];
    unsigned i;

    while (*p == ' ') p++;
    for (i = 0; i < sizeof verb - 1 && p[i] && p[i] != ' '; i++)
        verb[i] = (char)((p[i] >= 'A' && p[i] <= 'Z') ? p[i] + 32 : p[i]);
    verb[i] = 0;

    if (strcmp(verb, "enhook")) return 0;

    if (!room_var || !obj_tab) {
        fprintf(stderr, "  enhook needs the room and object tables\n");
        return 1;
    }
    if (M[(obj_tab + OBJ_CURTAINS) & AMASK] != M[room_var]) {
        fprintf(stderr, "  there is nothing here to hook"
                        " (the curtains are in the Library, EXPRESS 2/13)\n");
        return 1;
    }
    if (obj_state_get(OBJ_CURTAINS) == 1) {
        fprintf(stderr, "  it is already hooked to the hook\n");
        return 1;
    }
    obj_state_set(OBJ_CURTAINS, 1);
    fprintf(stderr, "  the curtain is now hooked to the hook -- LOOK to see it\n");
    return 1;
}

#endif /* THISSALA_DEBUG_H */
