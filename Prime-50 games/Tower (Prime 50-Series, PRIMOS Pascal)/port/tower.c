/*
 * tower.c -- a C port of PROGRAM CHIP1, the castle-and-caverns adventure
 * signed inside the game itself as "This dungeon courtesy of R. L. Tice."
 *
 * The original is PRIMOS Pascal; GETCOMMAND is Dave Montuori's, with the
 * source's own note that RLTICE modified it continuously.  This file follows
 * the Pascal procedure for procedure and keeps the original's behaviour even
 * where that behaviour is a mistake, notably:
 *
 *   - MOVE_IT lists nouns 11..52 and 54 but skips 53, so "move purple" falls
 *     through to the catch-all while "move amulet" does not.
 *   - GAD_ABOUT pads "bird's nest" to 17 columns where every other object
 *     name is padded to 18.
 *
 * Pascal's 1-based arrays are kept 1-based here, so index 0 is unused.
 *
 * The two data files the Pascal opens at run time, LEX2 and ROAD2, were
 * recovered from the PRIMOS disk image and are compiled in from tower_data.h.
 * ROAD2 is read until a value greater than 150 appears, which is why 41
 * entries of OBJECT are loaded although only 21 are ever used.
 *
 * Deliberate deviations from the original:
 *
 *   1. The game is made winnable.  The nine treasures are worth 10+Y each,
 *      which totals exactly 135, and BONUS is always zero -- but the original
 *      tests "IF SCORE_TOT > 135 THEN WINNER := TRUE", so WINNER could never
 *      be set and CHIP1 ended only on death or QUIT.  The test here is
 *      ">= 135", so selling all nine treasures wins.
 *
 *      Note what winning does, because the surrounding machinery is the
 *      original's: WINNER is only ever set inside SCORE, and the main loop
 *      tests it after the turn finishes.  So the win triggers when the player
 *      types SCORE holding a full 135, that score line prints as usual, and
 *      the program then ends -- "IF DEAD OR QUIT THEN SCORE" is false, so
 *      nothing further is written.  There is no victory text in CHIP1 to
 *      print; adding one would be invention, not restoration.
 *
 *   2. End of input exits instead of raising a Pascal run-time error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char Word[4];                       /* PACKED ARRAY[1..3] OF CHAR   */
typedef struct { const char (*book)[4]; int b_l; } Glossary;

#include "tower_data.h"

/* ------------------------------------------------------------------ VAR -- */

/* The Pascal VAR block also declares C, DONE, DUDLEY, HISS, BATTERY,
 * DICTIONARY, NOVEL, TREASURES and MOVERS, none of which CHIP1 ever uses;
 * they are dropped here rather than declared dead. */
static int  WINNER, FIN, BLIP, OFFER, DRACULA, QUIT, DEAD, FLUENT;
static int  NOTHING, SEE;
static char ATONIC[16];                     /* PACKED ARRAY[1..15] OF CHAR  */
static int  H, Y, I, W, X, V, QQ, JJ, II;
static int  WEIGHT, LOCATION, TURN, BONUS, DOLLOP, SCORE_TOT;
static Word USAGE, COGNATE, TERM, PRED;
static int  ROUTES[101][11];                /* [1..100][1..10]              */
static int  OBJECT[51];                     /* [1..50]                      */

/* ------------------------------------------------------------------ I/O -- */

#define ATONIC_SET(s) (memcpy(ATONIC, (s), 15))
static void writeln_atonic(void) { fwrite(ATONIC, 1, 15, stdout); putchar('\n'); }

static int read_line(char *inl, int max)
{
    int c, n = 0;

    memset(inl, ' ', 128);
    c = getchar();
    if (c == EOF) exit(0);
    while (c != EOF && c != '\n') {
        if (c != '\r' && n < max) inl[++n] = (char)c;
        c = getchar();
    }
    return n;
}

static void read_word(Word w)
{
    char inl[128];

    read_line(inl, 80);
    w[0] = inl[1]; w[1] = inl[2]; w[2] = inl[3]; w[3] = '\0';
}

/* bare READLN, which HELP_HIM uses to wait for RETURN */
static void read_skip(void)
{
    char inl[128];
    read_line(inl, 80);
}

static int w_eq(const char *a, const char *b) { return memcmp(a, b, 3) == 0; }

static void upshift(Word w)
{
    int i;
    for (i = 0; i < 3; i++)
        if (w[i] >= 'a' && w[i] <= 'z') w[i] = (char)(w[i] - 32);
}

/* ------------------------------------------------------------ GETCOMMAND -- */
/* written by Dave Montuori at the creator's request           */
/* SLIGHT MODIFICATIONS BY RLTICE CONTINUOUSLY                 */

static void getcommand(Word word1, Word word2)
{
    char inl[128];
    int j, k, i, linelength;

    memcpy(word1, "   ", 4);
    memcpy(word2, "   ", 4);
    linelength = 1;
    linelength = read_line(inl, 80);

    for (i = 1; i <= linelength; i++)
        if (inl[i] >= 'a' && inl[i] <= 'z') inl[i] = (char)(inl[i] - 32);
    i = 1;
    while (inl[i] == ' ' && i <= linelength) i = i + 1;
    if (i > linelength && linelength > 0) i = linelength;
    j = i;
    while (inl[j] != ' ' && j <= linelength) j = j + 1;
    if (j > i + 2) j = i + 2;
    if (j > linelength) j = linelength;
    for (k = i; k <= j; k++) word1[k - i] = inl[k];

    while (inl[i] != ' ' && i <= linelength) i = i + 1;
    while (inl[i] == ' ' && i <= linelength) i = i + 1;
    if (i > linelength && linelength > 0) i = linelength;
    j = i;
    while (inl[j] != ' ' && j <= linelength) j = j + 1;
    if (j > i + 2) j = i + 2;
    if (j > linelength) j = linelength;
    for (k = i; k <= j; k++) word2[k - i] = inl[k];
}

/* -------------------------------------------------------------- lookups -- */

static int IN_BOOK(const Word lode, const Glossary *tome)
{
    X = 1;
    FIN = 0;
    do {
        if (w_eq(tome->book[X - 1], lode)) FIN = 1;
        else                               X = X + 1;
    } while (!(FIN || X > tome->b_l));
    return FIN;
}

static int DEFINITION(const Word phrase, const Glossary *comic)
{
    BLIP = 0;
    W = 1;
    do {
        if (w_eq(phrase, comic->book[W - 1])) BLIP = 1;
        else                                  W = W + 1;
    } while (!(BLIP || W > comic->b_l));
    if (W > comic->b_l) W = comic->b_l;
    return W;
}

/* ---------------------------------------------------------------- SCORE -- */

static void SCORE(void)
{
    Y = 1;
    SCORE_TOT = 0;
    do {
        if (OBJECT[Y] == 64) SCORE_TOT = SCORE_TOT + 10 + Y;
        Y = Y + 1;
    } while (!(Y > 9));
    if (SCORE_TOT >= 135) WINNER = 1;   /* original: > 135, see file header */
    SCORE_TOT = SCORE_TOT + BONUS;
    printf("Your score is %8d of 135, and you used %8d turns.\n", SCORE_TOT, TURN);
    printf("You lose points for dying, and there are only 9 treasures.\n");
}

/* ------------------------------------------------------------ GAD_ABOUT -- */

static void GAD_ABOUT(int G)
{
    int L;

    L = 1;
    printf("\n");
    do {
        if (OBJECT[L] == G)
            switch (L) {
            case  1: printf("silver bells      \n"); break;
            case  2: printf("ruby eggs         \n"); break;
            case  3: printf("lead crystals     \n"); break;
            case  4: printf("bronze torc       \n"); break;
            case  5: printf("emerald gizzard   \n"); break;
            case  6: printf("rough diamonds    \n"); break;
            case  7: printf("china teapot      \n"); break;
            case  8: printf("platinum coins    \n"); break;
            case  9: printf("gold fillings     \n"); break;
            case 10: printf("iron key          \n"); break;
            case 11: printf("glass beads       \n"); break;
            case 12: printf("strange skeleton  \n"); break;
            case 13: printf("copper cross      \n"); break;
            case 14: printf("scissor tongs     \n"); break;
            case 15: printf("ice cube          \n"); break;
            case 16: printf("dead rat          \n"); break;
            case 17: printf("old branch        \n"); break;
            case 18: printf("parchment scroll  \n"); break;
            case 19: printf("paper note        \n"); break;
            case 20: printf("bird's nest      \n");  break;
            case 21: printf("purple amulet     \n"); break;
            default: printf("%8d IS AN UNKNOWN OBJECT THAT IS HERE.\n", L); break;
            }
        L = L + 1;
    } while (!(L > 21));
}

/* ------------------------------------------------------------ GET_INPUT -- */

static void GET_INPUT(Word spoken, const Glossary *buch)
{
    do {
        writeln_atonic();
        read_word(spoken);
        upshift(spoken);
    } while (!IN_BOOK(spoken, buch));
}

/* ------------------------------------------------------------ INVENTORY -- */

static void INVENTORY(void)
{
    NOTHING = 1;
    printf("You are carrying:\n");
    V = 1;
    do {
        if (OBJECT[V] == 0) NOTHING = 0;
        V = V + 1;
    } while (!(V > 21));
    if (NOTHING)
        printf("Nothing.\n");
    else {
        V = 0;
        GAD_ABOUT(V);
    }
}

/* ------------------------------------------------------------------ WTN -- */

static int WTN(const Word chum)
{
    H = DEFINITION(chum, &nouns);
    switch (H) {
    case  1: QQ =  1; break;
    case  2: QQ =  2; break;
    case  3: QQ =  3; break;
    case  4: QQ =  4; break;
    case  5: QQ =  5; break;
    case  6: QQ =  6; break;
    case  7: QQ =  7; break;
    case  8: QQ =  8; break;
    case  9: QQ =  9; break;
    case 10: QQ = 10; break;
    case 11: case 12: QQ =  1; break;
    case 13: case 14: QQ =  2; break;
    case 15: case 16: QQ =  3; break;
    case 17: case 18: QQ =  4; break;
    case 19: case 20: QQ =  5; break;
    case 21: case 22: QQ =  6; break;
    case 23: case 24: QQ =  7; break;
    case 25: case 26: QQ =  8; break;
    case 27: case 28: QQ =  9; break;
    case 29: case 30: QQ = 10; break;
    case 31: case 32: QQ = 11; break;
    case 33: case 34: case 35: case 36: QQ = 12; break;
    case 37: case 38: QQ = 13; break;
    case 39: case 40: QQ = 14; break;
    case 41: case 42: QQ = 15; break;
    case 43: case 44: QQ = 16; break;
    case 45: case 46: QQ = 17; break;
    case 47: case 48: QQ = 18; break;
    case 49: case 50: QQ = 19; break;
    case 51: case 52: QQ = 20; break;
    case 53: case 54: QQ = 21; break;
    default: QQ = LOCATION; break;
    }
    return QQ;
}

/* ------------------------------------------------------------- GO_ALONG -- */

static void GO_ALONG(void)
{
    ATONIC_SET("What direction?");
    memcpy(COGNATE, PRED, 4);
    if (w_eq(COGNATE, "   ") || !IN_BOOK(COGNATE, &inanimate))
        GET_INPUT(COGNATE, &inanimate);
    if (IN_BOOK(COGNATE, &directions)) {
        if (ROUTES[LOCATION][WTN(COGNATE)] < 1) {
            printf("You can't GO that way.\n");
            if (ROUTES[LOCATION][WTN(COGNATE)] < 0)
                printf("The way is blocked.\n");
        } else
            LOCATION = ROUTES[LOCATION][WTN(COGNATE)];
    } else if (IN_BOOK(COGNATE, &places))
        printf("What DIRECTION is that?\n");
    else
        printf("Where is it?\n");
}

/* ------------------------------------------------------------- PASSWORD -- */

static void PASSWORD(void)
{
    if (!w_eq(PRED, "   "))
        memcpy(TERM, PRED, 4);
    else {
        printf("What should I say? \n");
        read_word(TERM);
        upshift(TERM);
    }
    NOTHING = 1;
    switch (LOCATION) {
    case 1:
        if (w_eq(TERM, "SUN")) {
            ROUTES[1][4] = 2;
            printf("The sun shines BRIGHTLY through the east wall!\n");
            NOTHING = 0;
        }
        break;
    case 21:
        if (w_eq(TERM, "YES")) {
            LOCATION = 44;
            printf("The cavern spins wildly!\n");
            NOTHING = 0;
        }
        break;
    case 42:
        if (w_eq(TERM, "TOM")) {
            LOCATION = 21;
            printf("The cavern wildly spins!\n");
            NOTHING = 0;
        }
        break;
    case 56:
        if (w_eq(TERM, "T42")) {
            OBJECT[7] = LOCATION;
            printf("An expensive teapot appears!\n");
            NOTHING = 0;
        }
        break;
    case 58:
        if (w_eq(TERM, "FAL")) {
            printf("The floor opens beneath your feet, and you fall down!\n");
            printf("\n");
            printf("Down........\n");
            printf("\n");
            printf("....Down....\n");
            printf("\n");
            printf("........THUD!\n");
            LOCATION = 24;
            NOTHING = 0;
        } else
            printf("That's not it.....\n");
        break;
    default:
        NOTHING = 1;
        break;
    }
    if (NOTHING)
        printf("Mumble, mumble . . . nothing happened.\n");
}

/* ------------------------------------------------------------ INIT_GAME -- */

static void INIT_GAME(void)
{
    int i, j;

    WEIGHT = 0;
    DRACULA = 1;
    FLUENT = 0;
    WINNER = 0;
    DEAD = 0;
    QUIT = 0;
    LOCATION = 1;
    SEE = 0;
    BONUS = 0;
    TURN = 0;

    for (i = 1; i <= 100; i++)
        for (j = 1; j <= 10; j++)
            ROUTES[i][j] = (i <= 59) ? init_routes[i - 1][j - 1] : 0;
    for (i = 1; i <= 50; i++)
        OBJECT[i] = (i <= 41) ? init_object[i - 1] : 0;
}

/* ---------------------------------------------------------------- CLIMB -- */

static void CLIMB(void)
{
    if (ROUTES[LOCATION][2] > 0)
        LOCATION = ROUTES[LOCATION][2];
    else {
        if (ROUTES[LOCATION][6] > 0)
            LOCATION = ROUTES[LOCATION][6];
        else
            printf("You try to climb up yourself, and you take a fall!\n");
    }
}

/* -------------------------------------------------------------- PICK_UP -- */

static void PICK_UP(void)
{
    ATONIC_SET(" Take what?    ");
    memcpy(COGNATE, PRED, 4);
    if (w_eq(COGNATE, "   ") || !IN_BOOK(COGNATE, &nouns))
        GET_INPUT(COGNATE, &nouns);
    if (IN_BOOK(COGNATE, &animate)) {
        if (OBJECT[WTN(COGNATE)] == LOCATION) {
            if (WEIGHT < 11) {
                WEIGHT = WEIGHT + 1;
                OBJECT[WTN(COGNATE)] = 0;
            } else
                printf("I can't carry all of this and that too!\n");
        } else
            printf("I don't see it here.\n");
    } else
        printf("It is too difficult to hold that.\n");
}

/* ------------------------------------------------------------- PUT_DOWN -- */

static void PUT_DOWN(void)
{
    ATONIC_SET(" Drop what?    ");
    memcpy(COGNATE, PRED, 4);
    if (w_eq(COGNATE, "   ") || !IN_BOOK(COGNATE, &nouns))
        GET_INPUT(COGNATE, &nouns);
    if (IN_BOOK(COGNATE, &animate)) {
        if (OBJECT[WTN(COGNATE)] == 0) {
            WEIGHT = WEIGHT - 1;
            OBJECT[WTN(COGNATE)] = LOCATION;
        } else
            printf("I am not holding it.\n");
    } else
        printf("I couldn't have held that at any time.\n");
}

/* ---------------------------------------------------------------- GYPSY -- */

static void GYPSY(void)
{
    if (LOCATION != 9) {
        if ((TURN % 30) == 0) {
            NOTHING = 0;
            JJ = 1;
            do {
                if (OBJECT[JJ] == 0) {
                    WEIGHT = WEIGHT - 1;
                    OBJECT[JJ] = 59;
                    printf("You feel a tug at your pocket and as you turn you glimpse\n");
                    printf("a gypsy running away with one of your possessions! \n");
                    NOTHING = 1;
                }
                JJ = JJ + 1;
            } while (!(JJ > 9 || NOTHING));
        }
    } else {
        JJ = 1;
        do {
            if (OBJECT[JJ] == LOCATION) {
                if (OFFER) {
                    if (JJ < 10) {
                        WEIGHT = WEIGHT - 1;
                        OBJECT[JJ] = 64;
                        printf("The gypsy smiles and says,\"This will increase you're credit!\n");
                        printf("He takes the treasure and writes something on a scroll.\n");
                    } else {
                        WEIGHT = WEIGHT - 1;
                        printf("The gypsy takes it and throws it into his tent.\n");
                        printf("He says,\"I do collect worthless junk. Thank you!\"\n");
                        OBJECT[JJ] = 59;
                    }
                } else {
                    WEIGHT = WEIGHT - 1;
                    printf("The gypsy takes it and throws it into his tent.\n");
                    printf("He says, \"If you don't want it, I'll take it!\"\n");
                    OBJECT[JJ] = 59;
                }
            }
            JJ = JJ + 1;
        } while (!(JJ > 21));
    }
}

/* -------------------------------------------------------------- MOVE_IT -- */

static void MOVE_IT(void)
{
    ATONIC_SET(" Move what?    ");
    memcpy(COGNATE, PRED, 4);
    if (w_eq(COGNATE, "   ") || !IN_BOOK(COGNATE, &nouns))
        GET_INPUT(COGNATE, &nouns);
    II = DEFINITION(COGNATE, &nouns);
    switch (II) {
    case  1: case  2: case  3: case  4: case  5:
    case  6: case  7: case  8: case  9: case 10:
        printf("If you want to go in that DIRECTION, type: GO .\n");
        break;
    case 11: case 12: case 13: case 14: case 15: case 16: case 17:
    case 18: case 19: case 20: case 21: case 22: case 23: case 24:
    case 25: case 26: case 27: case 28: case 29: case 30: case 31:
    case 32: case 33: case 34: case 35: case 36: case 37: case 38:
    case 39: case 40: case 41: case 42: case 43: case 44: case 45:
    case 46: case 47: case 48: case 49: case 50: case 51: case 52:
    case 54:
        if (OBJECT[WTN(COGNATE)] == LOCATION)
            printf("It moves easily.\n");
        else
            printf("It isn't here.\n");
        break;
    case 56: case 57: case 58:
    case 59: case 63: case 64:
    case 68: case 70:
        printf("Moving that changes nothing.\n");
        break;
    case 65:
        if (LOCATION == 49) {
            printf("The west wall appears loose, and\n");
            printf("it swings on hidden hinges!\n");
            ROUTES[LOCATION][9] = ROUTES[LOCATION][9] * (-1);
        } else
            printf("All walls here are too heavy to move.\n");
        break;
    case 69:
        printf("It won't move.\n");
        break;
    case 75: case 76:
        if (LOCATION == 3) {
            printf(" They move easily around, revealing a hole ");
            printf("in the ground!\n");
            ROUTES[LOCATION][6] = 33;
        } else
            printf("Doing that here changes nothing.\n");
        break;
    default:
        printf(" That's a pretty impossible thing to do.\n");
        break;
    }
}

/* ------------------------------------------------------------- HELP_HIM -- */

static void HELP_HIM(void)
{
    printf("You can do many things on this adventure. but the\n");
    printf("only clues you have are these:                   \n");
    printf("You GO directions, but you only MOVE other objects.\n");
    printf("\n");
    printf("To score, you must sell your treasures to a gypsy.\n");
    printf("But watch out! the gypsy is a thief, and will follow\n");
    printf("you to steal what you find.  No fighting is allowed\n");
    printf("in this dungeon, you must survive by your wits. To\n");
    printf("interact with this adventure, type in the ACTION\n");
    printf("you wish to make and then the DIRECTION, or THING,\n");
    printf("you want to act with; e.g. WALK WEST >return<.\n");
    printf("\n");
    printf(" You can abbreviate most words up to 3 letters (WEST\n");
    printf("becomes WES ), but the DIRECTIONS NorthWest, SouthEast,\n");
    printf("SouthWest, NorthEast, abbreviate like this:\n");
    printf("S-W, N-W, N-E, S-E\n");
    printf("\n");
    printf("All objects will be recognized by the way they are described.\n");
    printf("\n");
    printf("if you are stuck or lost and want to start over, type:  STUCK.\n");
    printf(" It usually works.\n");
    printf("And since you can't swim, find some flotsam before entering WATER!\n");
    printf("Please press <RETURN>\n");
    read_skip();
}

/* ------------------------------------------------------------- DEAD_MAN -- */

static void DEAD_MAN(void)
{
    printf("You can see and hear nothing, You are in LIMBO.\n");
    printf("You are dead. Sigh. Do you want to try REINCARNATION?\n");
    read_word(TERM);
    upshift(TERM);
    if (w_eq(TERM, "YES")) {
        printf("Sputter, CRACKLE, blip, blop, bloop!\n");
        printf("BANG!\n");
        printf("Bang, BANG!\n");
        printf("Bang, BANG, bang!\n");
        printf("BANG, bang, BANG!\n");
        printf("      BANG, bang!\n");
        printf("            BANG!\n");
        if (OBJECT[8] == 0) {
            printf("             POW!!!!!!!!!!!!!!!!!\n");
            printf("I had to borrow something from you to bring you back.\n");
            OBJECT[8] = 7;
        } else {
            printf("            fizzle.\n");
            printf(" Darn, I didn't have a magic amulet to bring you back with.\n");
            DEAD = 1;
        }
    }
}

/* ----------------------------------------------------------------- DOOR -- */

static void DOOR(void)
{
    if (w_eq(PRED, "DOO")) {
        if (OBJECT[10] == 0) {
            if (LOCATION == 4 || (LOCATION == 58 && ROUTES[4][1] == -58)) {
                ROUTES[58][7] = 4;
                ROUTES[4][1] = 58;
                printf("The door crumbles to dust when you try to unlock it!\n");
            } else
                printf("There aren't any locks here!\n");
        } else
            printf("You don't have a key!\n");
    } else
        printf("That's a silly thing to try!\n");
}

/* ------------------------------------------------------------ HINT_LINE -- */

static void HINT_LINE(void)
{
    if (w_eq(PRED, "PAR") || w_eq(PRED, "SCR")) {
        if (OBJECT[18] == 0) {
            FLUENT = 1;
            printf("This is an English/Trog dictionary!\n");
        } else
            printf("I am not holding a parchment scroll!\n");
    }
    if (w_eq(PRED, "PAP") || w_eq(PRED, "NOT")) {
        if (OBJECT[19] == 0)
            printf("This dungeon courtesy of R. L. Tice.\n");
        else
            printf("I'm not holding a paper note!\n");
    }

    if (!FLUENT && (w_eq(PRED, "WAL") || w_eq(PRED, "WRI")
                 || w_eq(PRED, "WEI") || w_eq(PRED, "CUR")))
        printf("It looks like unintelligible Troglodytic!\n");
    else if (w_eq(PRED, "WAL") || w_eq(PRED, "WRI")
          || w_eq(PRED, "WEI") || w_eq(PRED, "CUR"))
        switch (LOCATION) {
        case 27:
            printf("It says, \"Yesterday. Say it Here but not Now.\"\n");
            printf("         \"Tomorrow. Say it Before but not Then. \"\n");
            break;
        case 21:
            printf("It says, \"Here.\"\n");
            break;
        case 19:
            printf("It says, \"Now.\"\n");
            break;
        case 33:
            printf("It says, \"T42. Place your order in the parlor.\"\n");
            break;
        case 42:
            printf("It says, \"Before.\"\n");
            break;
        case 43:
            printf("It says, \"Then.\"\n");
            break;
        default:
            printf("There is nothing on the wall to read right now.   \n");
            break;
        }
}

/* ----------------------------------------------------------- CRUNCH_DATA -- */

static void CRUNCH_DATA(void)
{
    ATONIC_SET("What'll you do?");
    do {
        writeln_atonic();
        getcommand(USAGE, PRED);
    } while (!IN_BOOK(USAGE, &verbs));
    DOLLOP = DEFINITION(USAGE, &verbs);
    switch (DOLLOP) {
    case 1: case 2: case 3:
    case 4: case 5:
        PICK_UP();
        break;
    case 6:
        CLIMB();
        break;
    case 7: case 8: case 9:
    case 10: case 11: case 12:
    case 13:
        GO_ALONG();
        break;
    case 14:
        printf("I need a shovel to do that.\n");
        break;
    case 15: case 16: case 17:
    case 18:
        MOVE_IT();
        break;
    case 19: case 20: case 21:
        HINT_LINE();
        break;
    case 22: case 23: case 24:
    case 25:
        printf("Now, now, NO violence!\n");
        break;
    case 26:
        HELP_HIM();
        break;
    case 27:
        INVENTORY();
        break;
    case 28: case 29:
        printf("I don't need a light in this dungeon game.\n");
        break;
    case 30: case 31:
        DOOR();
        break;
    case 32: case 33:
        QUIT = 1;
        break;
    case 34: case 35: case 36:
    case 37: case 47:
        PASSWORD();
        break;
    case 38:
        SCORE();
        break;
    case 39: case 40: case 41:
    case 42:
        PUT_DOWN();
        break;
    case 43: case 44: case 45:
        OFFER = 1;
        PUT_DOWN();
        break;
    case 46:
        INIT_GAME();
        break;
    default:
        printf("I'm not sure about that.\n");
        break;
    }
    TURN = TURN + 1;
}

/* ---------------------------------------------------------- CHANGE_LINES -- */

static void CHANGE_LINES(void)
{
    switch (LOCATION) {
    case 3:
        if (ROUTES[LOCATION][6] > 0)
            printf("THere is a hole under the bushes.\n");
        break;
    case 4:
        if (ROUTES[LOCATION][1] > 0)
            printf("There is a pile of dust where the door was.\n");
        else
            printf("There is a stout looking door in the entrance.\n");
        if (OBJECT[17] != 4)
            printf("You sink swiftly into the moat waters!\n");
        else
            printf("You are floating on a branch in the moat.   \n");
        break;
    case 19: case 21:
    case 27: case 33:
        printf("There is strange writing on the wall here!\n");
        break;
    case 42: case 43:
        printf("There is curious writing on the wall here!\n");
        break;
    case 46:
        if (OBJECT[16] == LOCATION) {
            printf("The Tyrranosaurus quickly grabs the dead rat\n");
            printf("and wolfs it down.  It keels over from food \n");
            printf("poisoning, crumbles to dust, leaving only it's\n");
            printf("innards behind!\n");
            OBJECT[5] = LOCATION;
            OBJECT[16] = 63;
        } else
            printf("There is a hungry looking Tyrannosaurus here!\n");
        break;
    case 49:
        if (ROUTES[LOCATION][9] > 0)
            printf("There is a doorway in the western wall.\n");
        break;
    case 52:
        if (DRACULA) {
            if (OBJECT[13] != LOCATION)
                printf("There is a vampire bat here!\n");
            else {
                printf("A bat flies away, squeaking in fear!\n");
                DRACULA = 0;
            }
        }
        break;
    case 100:
        printf("Oblivion. . . . .\n");
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------- BASIC_LINES -- */

static void BASIC_LINES(void)
{
    printf("\n");
    switch (LOCATION) {
    case 1:
        printf("You are in a room that has a southern door marked\n");
        printf("\"REALITY\". A word is scrawled on the east wall,\"SUN\".\n");
        printf("There is a table in this room that has a message \n");
        printf("written on it, \"Type this word,'help',for more\n");
        printf("information.\"\n");
        break;
    case 2:
        printf("You are on a grassy lawn south of a moat-encircled\n");
        printf("castle.  To the south, east, and west is a forest,\n");
        printf("and there is an entrance across the moat from you.\n");
        break;
    case 3:
        printf("You are north of an old moat-encricled castle.\n");
        printf("There is forest to the north, east, and west.\n");
        printf("There are some wysteria bushes in neat rows here.\n");
        break;
    case 4:
        printf("You are in a moat full of swift moving water.\n");
        printf("To the north you see the front entrance of the");
        printf(" castle.\n");
        break;
    case 5: case 6: case 7:
        printf("You are lost in a forest of evergreen trees. Paths\n");
        printf("lead in all directions.\n");
        break;
    case 8:
        printf("You are at a gravesite in a forest clearing.\n");
        break;
    case 9:
        printf("You are standing in a clearing with a tent to the\n");
        printf("north. There is a gypsy standing in front of the\n");
        printf("tent with his arms crossed, blocking the entry.\n");
        break;
    case 10: case 11: case 12:
        printf("you are at the top of an evergreen tree, from which\n");
        printf("there is only one exit: down.\n");
        break;
    case 13: case 14: case 15: case 16: case 17:
    case 18: case 19: case 20: case 21: case 22:
    case 23: case 24: case 25: case 26: case 27:
    case 28: case 29: case 30: case 31: case 32:
    case 33: case 34: case 35: case 36:
    case 37: case 38:
        printf("You are in a rough hewn corridor, with exits in\n");
        printf("many directions. The walls glow with an eerie light.\n");
        break;
    case 39:
        printf("You are in a cold cellar, the walls are chilly to\n");
        printf("touch.  There are doorways to the north and west.\n");
        break;
    case 40:
        printf("You are in a natural cavern, deep beneath the Earth,\n");
        printf("from which there is only one exit: North.\n");
        break;
    case 41: case 42: case 43: case 44:
        printf("You are in a rough hewn corridor, with exits in\n");
        printf("many directions.  The walls glow here, also. \n");
        break;
    case 45:
        printf("You are in a natural cavern, deep beneath the Earth, \n");
        printf("and there is a hungry looking Troglodyte King\n");
        printf("here, wearing a bronze torc. There is one exit, a\n");
        printf("crack in the wall to the north.\n");
        break;
    case 46:
        printf("You are at the bottom of a deep pit. There is one\n");
        printf("way out, a hole leading down. The walls are too\n");
        printf("steep to climb.\n");
        break;
    case 47: case 48: case 50:
        printf("You are on a staircase with two obvious exits: up and down.\n");
        break;
    case 49:
        printf("You are on a staircase with two obvious exits: down and up.\n");
        break;
    case 51:
        printf("You are in a hidden library with a door to the\n");
        printf("east. On the shelves are moldering piles of what\n");
        printf("at one time could have been books.\n");
        break;
    case 52:
        printf("You are in a belfry. There is a door to the north\n");
        printf("and a hole in the floor with stairs leading down.\n");
        break;
    case 53:
        printf("You are on a parapet with a view of the surrounding\n");
        printf("territory. The forest seems to extend forever.\n");
        printf("There is a door to the south, and a hole in the \n");
        printf("floor where stones appear to have fallen out.\n");
        break;
    case 54:
        printf("You are in a torture chamber. There is dried blood\n");
        printf("on the floor, and several piles of rust that at\n");
        printf(" one time could have been almost anything.\n");
        printf("There is a door to the west and a break in the\n");
        printf("ceiling, through which you see the sky.\n");
        break;
    case 55:
        printf("You are in the main library of the castle. Shelves\n");
        printf("are full of piles of what at one time could have \n");
        printf("been books, but are too decomposed to read now.\n");
        printf("Light comes from tiny windows in the north wall.\n");
        printf("there is a large break in the floor. A door \n");
        printf("leads east.\n");
        break;
    case 56:
        printf("You are in the tea parlor of the castle. A pile\n");
        printf("of rubble on the floor is all that remains of \n");
        printf("the chandelier. There is a door to the north that\n");
        printf("is rusted shut, and a staircase to the south that\n");
        printf("leads down.\n");
        break;
    case 57:
        printf("You are in the basement.  Massive pillars hold up the\n");
        printf("ceiling.  Light comes from the stairwell to the south,\n");
        printf("and a hole can barely be seen in the east wall.\n");
        break;
    case 58:
        printf("You are in the entrance chamber of the castle. All the exits\n");
        printf("are rusted shut, except for the southern door.\n");
        printf("This is written on the east wall, \"Name a season of \n");
        printf("the year.\"\n");
        break;
    case 59:
        printf("You are in a tent, with a lot of worthless junk\n");
        printf("lying about. You see a flap to the south, and a\n");
        printf("hole in the floor to the north-east.\n");
        break;
    case 100:
        printf("Darkness descends.\n");
        break;
    case 200:
        printf("The door opens, and Reality comes leaping in on\n");
        printf("this fantasy world! Reality engulfs you!! You\n");
        printf("cannot get away!! The shock is so terrible, you ");
        printf("DIE!!!!\n");
        printf("\n");
        LOCATION = 100;
        break;
    default:
        LOCATION = LOCATION;
        break;
    }
}

/* ----------------------------------------------------------- SLOW_DEATH -- */

static void SLOW_DEATH(void)
{
    BASIC_LINES();
    CHANGE_LINES();
    I = 1;
    do {
        ATONIC_SET("What'll you do?");
        writeln_atonic();
        getcommand(USAGE, PRED);
        if (w_eq(USAGE, "DRO") && (w_eq(PRED, "BRA") || w_eq(PRED, "OLD")))
            PUT_DOWN();
        else
            printf("You are going under again!   >GLUB<!\n");
        if (OBJECT[17] == 4)
            I = 16;
        I = I + 1;
    } while (!(I > 10));
    if (I < 15)
        LOCATION = 100;
}

/* ---------------------------------------------------------------- EXTRA -- */

static void EXTRA(void)
{
    if (LOCATION == 8 && OBJECT[13] == 0 && ROUTES[LOCATION][6] < 1) {
        printf("The grave caves in with a rumble!\n");
        ROUTES[13][2] = 8;
        ROUTES[LOCATION][6] = 13;
    }

    if (OBJECT[15] == 0 && OBJECT[14] != 0) {
        OBJECT[15] = LOCATION;
        printf("Ice is too cold to carry in your pocket. Find something\n");
        printf("to carry it with!\n");
    }

    if (OBJECT[16] == 0 && OBJECT[15] != 0) {
        OBJECT[16] = LOCATION;
        printf("The rat stinks too much. Find some ice to keep it on.\n");
    }

    if (LOCATION == 4 && OBJECT[17] != 4)
        SLOW_DEATH();

    if (LOCATION == 52 && DRACULA && OBJECT[1] == 0) {
        printf("The vampire swoops on you, and kills you!\n");
        OBJECT[1] = LOCATION;
        LOCATION = 100;
    }
    if (LOCATION == 100)
        DEAD_MAN();
    else
        GYPSY();

    if (LOCATION == 45 && OFFER) {
        if (w_eq(PRED, "GLA") || w_eq(PRED, "BEA")) {
            if (OBJECT[11] == LOCATION) {
                OBJECT[11] = 63;
                printf(" The king takes your trinkets and throws ");
                printf("some rough diamonds down in trade!\n");
                OBJECT[6] = LOCATION;
            } else
                printf("You don't have any glass beads!\n");
        } else
            printf("The King ignores the offer.          \n");
    }

    OFFER = 0;
}

/* ----------------------------------------------------------------- MAIN -- */

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    INIT_GAME();
    do {
        BASIC_LINES();
        CHANGE_LINES();
        GAD_ABOUT(LOCATION);
        if (LOCATION != 100)
            CRUNCH_DATA();
        EXTRA();
    } while (!(DEAD || QUIT || WINNER));
    if (DEAD || QUIT)
        SCORE();
    return 0;
}
