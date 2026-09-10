/*  parse.c -- the AMAZON command scanner.
 *
 *  AMAZOT.SAI, AMAZON.SAI and PRTEST.SAI all open with the same line:
 *
 *      SETBREAK(1,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*-",null,"KXNS");
 *
 *  and then read one word at a time with
 *
 *      GETCMD(COMMAND, ANY!MORE, 1, 30);
 *
 *  In SAIL, mode "N" complements the break set, so the break characters are
 *  everything that is NOT in "A-Z 0-9 * -"; "S" skips those separators
 *  instead of returning them; "K" keeps the break character in the source;
 *  "X" suppresses the no-break-found error.  The net effect is a scanner
 *  that hands back successive runs of [A-Z0-9*-] and nothing else -- which
 *  is what this file reproduces.  Input is upper-cased first, exactly as a
 *  1977 TTY would have delivered it.
 *
 *  ANY!MORE (here "more") is GETCMD's more-to-come flag: > 0 while further
 *  words remain on the line, 0 on the last word, and AMAZON.SAI treats a
 *  value < 1 as "nothing followed the verb", which is what produces the
 *  "<verb> WHAT?" response.
 */
#include <string.h>
#include <ctype.h>
#include "amazon.h"

static int isbreakchar(int c)
{
    /* the set given to SETBREAK, un-complemented */
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || c == '*' || c == '-';
}

void cmd_reset(CmdLine *c, const char *line)
{
    int i = 0, n = 0;
    memset(c, 0, sizeof *c);
    while (line[i] && i < (int)sizeof c->line - 1) {
        c->line[i] = (char)toupper((unsigned char)line[i]);
        i++;
    }
    c->line[i] = 0;

    for (i = 0; c->line[i] && n < 24; ) {
        int k = 0;
        while (c->line[i] && !isbreakchar((unsigned char)c->line[i])) i++;
        if (!c->line[i]) break;
        while (c->line[i] && isbreakchar((unsigned char)c->line[i])
               && k < (int)sizeof c->tok[0] - 1)
            c->tok[n][k++] = c->line[i++];
        c->tok[n][k] = 0;
        /* a word longer than the token buffer: swallow the rest of it */
        while (c->line[i] && isbreakchar((unsigned char)c->line[i])) i++;
        n++;
    }
    c->ntok = n;
    c->cur  = 0;
}

/*  GETCMD: return the next word, set *more as SAIL's MORE!TO!COME.
 *  Returns 0 when the line is exhausted.                             */
int getcmd(CmdLine *c, char *out, int *more)
{
    if (c->cur >= c->ntok) {
        out[0] = 0;
        *more = -1;                     /* PRTEST.SAI uses < 0 for "none" */
        return 0;
    }
    strcpy(out, c->tok[c->cur]);
    c->cur++;
    *more = c->ntok - c->cur;
    return 1;
}

/* ================================================================ *
 *  CMDLIB.SAI (11-Sep-77) -- "Sail Library of Commands".
 *  MAKCMD sets up three associative classes.  The third column is
 *  CMDLIB's "strength": 1 = gentle, 2 = forceful for the two verb
 *  classes, and the direction ordinal for the DIRECTION class.
 * ================================================================ */
typedef struct { const char *word; int cls, strength; } CmdEnt;

static const CmdEnt cmdtab[] = {
    /*   class          command        strength */
    { "ACQUIRE",    VC_ACQUIRE,    1 },
    { "GET",        VC_ACQUIRE,    1 },
    { "TAKE",       VC_ACQUIRE,    1 },
    { "CAPTURE",    VC_ACQUIRE,    1 },
    { "KEEP",       VC_ACQUIRE,    1 },
    { "STEAL",      VC_ACQUIRE,    2 },
    { "ROB",        VC_ACQUIRE,    2 },

    { "RELINQUISH", VC_RELINQUISH, 1 },
    { "DROP",       VC_RELINQUISH, 1 },
    { "RELEASE",    VC_RELINQUISH, 1 },
    { "THROW",      VC_RELINQUISH, 2 },
    { "TOSS",       VC_RELINQUISH, 2 },

    { "N",          VC_DIRECTION,  DIR_N  },
    { "S",          VC_DIRECTION,  DIR_S  },
    { "E",          VC_DIRECTION,  DIR_E  },
    { "W",          VC_DIRECTION,  DIR_W  },
    { "U",          VC_DIRECTION,  DIR_U  },
    { "D",          VC_DIRECTION,  DIR_D  },
    { "NE",         VC_DIRECTION,  DIR_NE },
    { "NW",         VC_DIRECTION,  DIR_NW },
    { "SE",         VC_DIRECTION,  DIR_SE },
    { "SW",         VC_DIRECTION,  DIR_SW },
    { "UP",         VC_DIRECTION,  DIR_U  },
    { "DOWN",       VC_DIRECTION,  DIR_D  },
    { "NORTH",      VC_DIRECTION,  DIR_N  },
    { "SOUTH",      VC_DIRECTION,  DIR_S  },
    { "EAST",       VC_DIRECTION,  DIR_E  },
    { "WEST",       VC_DIRECTION,  DIR_W  },
    { "NORTHEAST",  VC_DIRECTION,  DIR_NE },
    { "NORTHWEST",  VC_DIRECTION,  DIR_NW },
    { "SOUTHEAST",  VC_DIRECTION,  DIR_SE },
    { "SOUTHWEST",  VC_DIRECTION,  DIR_SW },

    /* ---------------------------------------------------------------
     * Below this line the vocabulary is NOT from CMDLIB.SAI.  CMDLIB
     * only ever defined the three classes above; these verbs are the
     * ones the ROOMS and OBJECT.TYP notes require in order for the
     * described situations to be playable.  See docs/RECONSTRUCTION.md.
     * --------------------------------------------------------------- */
    { "LOOK",       VC_OTHER, V_LOOK },      { "L",      VC_OTHER, V_LOOK },
    { "INVENTORY",  VC_OTHER, V_INVENTORY }, { "I",      VC_OTHER, V_INVENTORY },
    { "GO",         VC_OTHER, V_GO },        { "WALK",   VC_OTHER, V_GO },
    { "READ",       VC_OTHER, V_READ },
    { "ANSWER",     VC_OTHER, V_ANSWER },    { "SAY",    VC_OTHER, V_SAY },
    { "OPEN",       VC_OTHER, V_OPEN },      { "CLOSE",  VC_OTHER, V_CLOSE },
    { "EXAMINE",    VC_OTHER, V_EXAMINE },   { "X",      VC_OTHER, V_EXAMINE },
    { "ATTACK",     VC_OTHER, V_ATTACK },    { "FIGHT",  VC_OTHER, V_ATTACK },
    { "KILL",       VC_OTHER, V_KILL },
    { "KISS",       VC_OTHER, V_KISS },
    { "EAT",        VC_OTHER, V_EAT },       { "DRINK",  VC_OTHER, V_DRINK },
    { "RIDE",       VC_OTHER, V_RIDE },      { "FLY",    VC_OTHER, V_FLY },
    { "SLEEP",      VC_OTHER, V_SLEEP },
    { "WEAR",       VC_OTHER, V_WEAR },
    { "UNLOCK",     VC_OTHER, V_UNLOCK },    { "LOCK",   VC_OTHER, V_LOCK },
    { "GIVE",       VC_OTHER, V_GIVE },      { "DELIVER",VC_OTHER, V_DELIVER },
    { "RETURN",     VC_OTHER, V_DELIVER },
    { "HELP",       VC_OTHER, V_HELP },      { "H",      VC_OTHER, V_HELP },
    { "SCORE",      VC_OTHER, V_SCORE },
    { "STATS",      VC_OTHER, V_STATS },
    { "WHO",        VC_OTHER, V_WHO },       { "TELL",   VC_OTHER, V_TELL },
    { "QUIT",       VC_OTHER, V_QUIT },      { "DONE",   VC_OTHER, V_QUIT },
    { "FINISH",     VC_OTHER, V_QUIT },
    { "BUY",        VC_OTHER, V_BUY },
    { "ORDER",      VC_OTHER, V_ORDER },
    { "WAIT",       VC_OTHER, V_WAIT },      { "Z",      VC_OTHER, V_WAIT },
    { "SOURCE",     VC_OTHER, V_SOURCE },
    { "PUT",        VC_OTHER, V_PUT },
    { "LAUNCH",     VC_OTHER, V_LAUNCH },
    { "SWEEP",      VC_OTHER, V_SWEEP },
    { "CLIMB",      VC_OTHER, V_CLIMB },
    { "SWIM",       VC_OTHER, V_SWIM },
    { "HINT",       VC_OTHER, V_HINT },
    { "PLEAD",      VC_OTHER, V_PLEAD },
    { "STUDY",      VC_OTHER, V_STUDY },
    { "TIE",        VC_OTHER, V_TIE },
    { "POUR",       VC_OTHER, V_POUR },
    { "LIGHT",      VC_OTHER, V_LIGHT },
    { "EXTINGUISH", VC_OTHER, V_TURNOFF },
    { 0, 0, 0 }
};

int lookup_command(const char *w, int *cls, int *strength)
{
    int i;
    for (i = 0; cmdtab[i].word; i++)
        if (!strcmp(cmdtab[i].word, w)) {
            *cls = cmdtab[i].cls;
            *strength = cmdtab[i].strength;
            return 1;
        }
    *cls = VC_NONE;
    *strength = 0;
    return 0;
}
