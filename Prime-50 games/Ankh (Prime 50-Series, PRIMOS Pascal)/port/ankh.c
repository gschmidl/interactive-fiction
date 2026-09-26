/*
 * ankh.c -- a C port of PROGRAM ANKH1.
 *
 * The original was written in PRIMOS Pascal by Randall Tice at the College of
 * William & Mary in January 1984 (minor modifications December 1984); the
 * source banner also thanks Dave Montouri, Mike Dullaghan and Chip Roberson,
 * and the GETCOMMAND procedure is Dave Montuori's.
 *
 * This file follows the Pascal structure procedure for procedure and keeps the
 * original's quirks, including its typos ("I'n not sure", "puramid"), its
 * unreachable cases and its one-sided doors.  Pascal's 1-based arrays are kept
 * 1-based here, so index 0 of ROUTES and OBJECT is deliberately unused.
 *
 * The two data files the Pascal opens at run time, LEXICON and ROADS, were
 * recovered from the PRIMOS disk image and are compiled in from ankh_data.h.
 *
 * Deliberate deviations from the original, and the only ones:
 *   - An empty input line crashes the real ANKH1: with LINELENGTH = 0
 *     GETCOMMAND indexes INLINE[0], PRIMOS raises ILLEGAL_SEGNO$ and the
 *     program dies.  Here the input buffer is blank filled, so an empty line
 *     yields two blank words and simply re-prompts, which is what the very
 *     same code does on a machine where that read happens to be harmless.
 *   - End of input exits instead of raising a Pascal run-time error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char Word[4];                       /* PACKED ARRAY[1..3] OF CHAR   */
typedef struct { const char (*book)[4]; int b_l; } Glossary;

#include "ankh_data.h"

/* ------------------------------------------------------------------ VAR -- */

static int  GAME, WINNER, FIN, BLIP, HISS, QUIT, DEAD, NOTHING, SEE;
static char ATONIC[16];                     /* PACKED ARRAY[1..15] OF CHAR  */
static int  II, V, X, W, H, QQ, JJ, I, Y;
static int  LOCATION, TURN, BONUS, DOLLOP, SCORE_TOT, BATTERY;
static Word USAGE, COGNATE, TERM, PRED;
static int  ROUTES[25][11];                 /* [1..24][1..10]               */
static int  OBJECT[18];                     /* [1..17]                      */

/* ------------------------------------------------------------------ I/O -- */

/* WRITELN(ATONIC) writes all fifteen characters, trailing blanks included. */
#define ATONIC_SET(s) (memcpy(ATONIC, (s), 15))
static void writeln_atonic(void) { fwrite(ATONIC, 1, 15, stdout); putchar('\n'); }

/*
 * READLN into a blank-filled 1-based buffer.  Returns the number of characters
 * read, which is what PRIMOS Pascal's READLN(v:n) leaves in n.
 */
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

/* READLN(W) where W is a three-character packed array. */
static void read_word(Word w)
{
    char inl[128];

    read_line(inl, 80);
    w[0] = inl[1]; w[1] = inl[2]; w[2] = inl[3]; w[3] = '\0';
}

static int w_eq(const char *a, const char *b) { return memcmp(a, b, 3) == 0; }

static void upshift(Word w)
{
    int i;
    for (i = 0; i < 3; i++)
        if (w[i] >= 'a' && w[i] <= 'z') w[i] = (char)(w[i] - 32);
}

/* ------------------------------------------------------------ GETCOMMAND -- */
/* written by Dave Montuori at the creator's request */

static void getcommand(Word word1, Word word2)
{
    char inl[128];
    int j, k, i, linelength;

    memcpy(word1, "   ", 4);
    memcpy(word2, "   ", 4);
    linelength = 0;
    linelength = read_line(inl, 80);

    for (i = 1; i <= linelength; i++)
        if (inl[i] >= 'a' && inl[i] <= 'z') inl[i] = (char)(inl[i] - 32);
    i = 1;
    while (inl[i] == ' ' && i <= linelength) i = i + 1;
    if (i > linelength) i = linelength;
    j = i;

    while (inl[j] != ' ' && j <= linelength) j = j + 1;
    if (j > i + 2) j = i + 2;
    if (j > linelength) j = linelength;

    for (k = i; k <= j; k++) word1[k - i] = inl[k];

    while (inl[i] != ' ' && i <= linelength) i = i + 1;
    while (inl[i] == ' ' && i <= linelength) i = i + 1;
    if (i > linelength) i = linelength;
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
    W = 0;
    do {
        W = W + 1;
        if (w_eq(phrase, comic->book[W - 1])) BLIP = 1;
    } while (!(BLIP || W == comic->b_l));
    return W;
}

/* ---------------------------------------------------------------- SCORE -- */

static void SCORE(void)
{
    Y = 1;
    SCORE_TOT = 0;
    do {
        if (OBJECT[Y] == 3) SCORE_TOT = SCORE_TOT + 10 + Y;
        Y = Y + 1;
    } while (!(Y > 6));
    if (OBJECT[3]  == 3) SCORE_TOT = SCORE_TOT - 13;
    if (OBJECT[17] == 3) SCORE_TOT = SCORE_TOT + 13;
    if (SCORE_TOT > 80) WINNER = 1;
    SCORE_TOT = SCORE_TOT + BONUS;
    printf("Your score is %8d%%, and you used %8d turns.\n", SCORE_TOT, TURN);
    printf("You lose percentage for Dying and Helps, and there are only 6 treasures.\n");
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
            case  1: printf("Silver Ankh       \n"); break;
            case  2: printf("Jade Barrel       \n"); break;
            case  3: printf("Net               \n"); break;
            case  4: printf("Onyx Bracelet     \n"); break;
            case  5: printf("Gold Oxen         \n"); break;
            case  6: printf("Platinum Elephant \n"); break;
            case  7: printf("Apple             \n"); break;
            case  8: printf("Myna Bird         \n"); break;
            case  9: printf("Daisy             \n"); break;
            case 10: printf("Tuna Fish         \n"); break;
            case 11: printf("Clover            \n"); break;
            case 12: printf("Flashlight        \n"); break;
            case 13: printf("Igneous Rock      \n"); break;
            case 14: printf("Rubber Raft       \n"); break;
            case 15: printf("Log               \n"); break;
            case 16: printf("Shovel            \n"); break;
            case 17: printf("Jeweled Sceptre   \n"); break;
            default: printf("%8d IS AN UNKNOWN OBJECT THAT IS HERE.\n", L); break;
            }
        L = L + 1;
    } while (!(L > 17));
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
    V = 0;
    GAD_ABOUT(V);
    V = 1;
    do {
        if (OBJECT[V] == 0) NOTHING = 0;
        V = V + 1;
    } while (!(V > 17));
    if (NOTHING)
        printf("Nothing.\n");
}

/* ------------------------------------------------------------------ WTN -- */

static int WTN(const Word chum)
{
    H = DEFINITION(chum, &nouns);
    switch (H) {
    case 76: QQ = 1;  break;
    case 77: QQ = 2;  break;
    case 78: QQ = 3;  break;
    case 79: QQ = 4;  break;
    case 80: QQ = 5;  break;
    case 81: QQ = 6;  break;
    case 82: QQ = 7;  break;
    case 83: QQ = 8;  break;
    case 84: QQ = 9;  break;
    case 85: QQ = 10; break;
    case  1: case  2: QQ = 1;  break;
    case  3: case  4: QQ = 2;  break;
    case  5: case  6: QQ = 17; break;
    case  7: case  8: QQ = 4;  break;
    case  9: case 10: QQ = 5;  break;
    case 11: case 12: QQ = 6;  break;
    case 13:          QQ = 7;  break;
    case 14: case 15: QQ = 8;  break;
    case 16:          QQ = 9;  break;
    case 17: case 18: QQ = 10; break;
    case 19:          QQ = 11; break;
    case 20: case 21: QQ = 12; break;
    case 22: case 23: QQ = 13; break;
    case 24: case 25: QQ = 14; break;
    case 26: case 27: QQ = 15; break;
    case 28:          QQ = 16; break;
    case 29:          QQ = 3;  break;
    case 30: case 31: QQ = 20; break;
    case 32:          QQ = 6;  break;
    case 33:
        if (LOCATION == 6)       QQ = 6;
        else if (LOCATION == 20) QQ = 20;
        else if (LOCATION == 5)  QQ = 5;
        else                     QQ = 0;
        break;
    case 34: QQ = 20; break;
    case 35: if (ROUTES[1][9] < 1) QQ = 1; else QQ = 2; break;
    case 36: QQ = 10; break;
    case 37: QQ = 1;  break;
    case 38: QQ = 19; break;
    case 39: QQ = 22; break;
    case 40: QQ = 7;  break;
    case 41: case 42: QQ = 10; break;
    case 43: QQ = 23; break;
    case 44: QQ = 1;  break;
    case 45: QQ = 13; break;
    case 46: if (LOCATION == 8) QQ = 8; else QQ = 18; break;
    case 47: QQ = 19; break;
    case 48: QQ = 3;  break;
    case 49: QQ = 7;  break;
    case 50: case 51: case 52: QQ = 12; break;
    case 53: case 54: QQ = 14; break;
    case 55: QQ = 23; break;
    case 56: case 57: QQ = 21; break;
    case 58: QQ = 6;  break;
    case 59: QQ = 9;  break;
    case 60: case 61: case 62: QQ = 17; break;
    case 63: QQ = LOCATION - 1; break;
    case 64: case 65: QQ = 11; break;
    case 66: QQ = 20; break;
    case 67: QQ = 10; break;
    case 68: QQ = 5;  break;
    case 69: QQ = 15; break;
    case 70: case 71: QQ = 18; break;
    case 72: QQ = 2;  break;
    case 73: QQ = LOCATION + 1; break;
    case 74: QQ = LOCATION - 1; break;
    case 75: QQ = 24; break;
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
            printf("You can't go that way.\n");
            if (ROUTES[LOCATION][WTN(COGNATE)] < 0)
                printf("The way is blocked.\n");
        } else
            LOCATION = ROUTES[LOCATION][WTN(COGNATE)];
    } else if (IN_BOOK(COGNATE, &places))
        printf("What direction is that?\n");
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

    if (w_eq(TERM, "HEL")) {
        printf("If you want Help just type : Help .\n");
        NOTHING = 0;
    }
    if (w_eq(TERM, "JAN") && LOCATION == 5) {
        NOTHING = 0;
        printf("KERFOOOOOOOOOM!!!!!!!\n");
        ROUTES[4][7] = ROUTES[4][7] * (-1);
        ROUTES[5][9] = ROUTES[5][9] * (-1);
    }
    if (w_eq(TERM, "FOO") && LOCATION == 4) {
        printf("KERFLAAAAAM!\n");
        ROUTES[4][7] = ROUTES[4][7] * (-1);
        ROUTES[5][9] = ROUTES[5][9] * (-1);
        NOTHING = 0;
    }
    if (LOCATION == 23 && w_eq(TERM, "MAN")) {
        printf("The Sphinx falls over and crumbles to dust!\n");
        ROUTES[23][9] = 24;
        NOTHING = 0;
    }
    if (NOTHING)
        printf("Mumble, Mumble, nothing happens.\n");
}

/* ------------------------------------------------------------ INIT_GAME -- */

static void INIT_GAME(void)
{
    int i, j;

    WINNER = 0;
    DEAD = 0;
    QUIT = 0;
    BATTERY = 100;
    LOCATION = 1;
    SEE = 0;
    BONUS = 19;
    TURN = 0;
    HISS = 0;               /* the Pascal leaves this undefined until EXTRA */

    for (i = 1; i <= 24; i++)
        for (j = 1; j <= 10; j++)
            ROUTES[i][j] = init_routes[i - 1][j - 1];
    for (i = 1; i <= 17; i++)
        OBJECT[i] = init_object[i - 1];
}

/* -------------------------------------------------------------- PICK_UP -- */

static void PICK_UP(void)
{
    ATONIC_SET(" Take what?    ");
    memcpy(COGNATE, PRED, 4);
    if (w_eq(COGNATE, "   ") || !IN_BOOK(COGNATE, &nouns))
        GET_INPUT(COGNATE, &nouns);
    if (IN_BOOK(COGNATE, &animate)) {
        if (OBJECT[WTN(COGNATE)] == LOCATION)
            OBJECT[WTN(COGNATE)] = 0;
        else
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
        if (OBJECT[WTN(COGNATE)] == 0)
            OBJECT[WTN(COGNATE)] = LOCATION;
        else
            printf("I am not holding it.\n");
    } else
        printf("I couldn't have held that at any time.\n");
}

/* -------------------------------------------------------------- GREMLIN -- */

static void GREMLIN(void)
{
    if (LOCATION > 4 && (TURN % 20) == 0) {
        NOTHING = 0;
        JJ = 1;
        do {
            if (OBJECT[JJ] == 0) {
                if (JJ > 3) OBJECT[JJ] = LOCATION - 1;
                else        OBJECT[JJ] = 6;
                printf("You feel a tug at your pocket and as you turn you see\n");
                printf("a Gremlin scampering away with one of your possessions!\n");
                NOTHING = 1;
            }
            JJ = JJ + 1;
        } while (!(JJ > 6 || NOTHING));
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
    case 47:
        printf("It gorges itself on you!\n");
        LOCATION = 100;
        DEAD = 1;
        break;
    case 46:
        printf("It moves right back.\n");
        break;
    case 48:
        DEAD = 1;
        LOCATION = 100;
        printf("Reality comes crashing in!\n");
        break;
    case 30: case 31:
        printf("It bites you!\n");
        DEAD = 1;
        LOCATION = 100;
        break;
    case 35:
        printf("It moves into the wall.\n");
        ROUTES[2][6] = ROUTES[2][6] * (-1);
        ROUTES[2][4] = ROUTES[2][4] * (-1);
        ROUTES[1][9] = ROUTES[1][9] * (-1);
        ROUTES[2][7] = ROUTES[2][7] * (-1);
        break;
    case 41: case 42:
        printf("The odor overcomes you and you die.\n");
        DEAD = 1;
        LOCATION = 100;
        break;
    case 43:
        printf("The Sphinx hits you over your head.\n");
        DEAD = 1;
        LOCATION = 100;
        break;
    case 45:
        printf("The throne moves to the side.\n");
        ROUTES[13][7] = ROUTES[13][7] * (-1);
        break;
    default:
        printf(" That's a futile thing to do right now!\n");
        break;
    }

    switch (II) {
    case 30: case 31: case 41:
    case 42: case 43: case 47: case 48:
        LOCATION = 100;
        break;
    default:
        LOCATION = LOCATION;
        break;
    }
}

/* ------------------------------------------------------------- HELP_HIM -- */

static void HELP_HIM(void)
{
    if (OBJECT[8] == 0) {
        switch (LOCATION) {
        case 1:
            printf("A beginner, eh?\n");
            break;
        case 2:
            printf("The Myna looks disgusted and says:\n");
            printf("\"You're such an untidy twit. You didn't put\n");
            printf("the bookcase back where you found it.\"\n");
            break;
        case 4:
            printf(" The Myna Bird says:\n");
            printf("\"Can't you read, Fool? It's as obvious as \n");
            printf("saying your own Name !\"\n");
            break;
        case 5:
            printf("The Myna looks perplexed but says:\n");
            printf("\"You know, I bet Janus wrote that note!\"\n");
            break;
        case 8:
            printf("The Myna bird squawks:\n");
            printf("\"So get out of the water! I'm not a lifeguard!\"\n");
            break;
        case 13:
            printf("Myna: \"Where did the King go! Where did he hide!\"\n");
            break;
        case 10:
            printf("Myna: \"Rawk! That skunk can sure smell!\"\n");
            break;
        case 17:
            printf("\"Don't go west! Don't go west!\",The bird says.\n");
            break;
        case 18:
            printf("\"You might raft across, Dodo!\", Myna says.\n");
            break;
        case 19:
            printf("\"Cats love seafood! But not birds!\" pleads the bird.\n");
            break;
        case 20:
            printf("\"You might kill with sex, drugs, Rock or roll,\"\n");
            printf("sings the Myna.\n");
            break;
        case 22:
            printf("Myna: \"Sex and drugs and Rock and roll, chirp!\"\n");
            break;
        case 23:
            printf("Myna: \"Man, that's tough!\" says the bird.\n");
            break;
        default:
            printf("The Myna looks bored. try going somewhere else.\n");
            break;
        }
    } else
        printf("Why don't you ask the bird for help?\n");

    if (LOCATION == 1) {
        printf("This is the only place where help is free.  You can\n");
        printf("go places, but you can only move other objects. To\n");
        printf("interact with this adventure, type in the action\n");
        printf("you wish to make and then the direction, or thing,\n");
        printf("you want to act with, e.g. WALK WEST <return>.\n");
        printf(" You can abbreviate most words up to 3 letters (WEST\n");
        printf("becomes WES), but the directions NORTHWEST, SOUTHEAST,\n");
        printf("SOUTHWEST, NORTHEAST, abbreviate like this:\n");
        printf("N-W, S-E, S-W, N-E.\n");
        printf("Do not go near the door to the south.  Reality comes\n");
        printf("crashing in and you die.  There is another way out of\n");
        printf("this room.\n");
        printf("\n");
    } else
        BONUS = BONUS - 1;
}

/* ------------------------------------------------------------- DEAD_MAN -- */

static void DEAD_MAN(void)
{
    printf("You can see and hear nothing. You are in Limbo.\n");
    printf("You are dead. Sigh. Do you want to try reincarnation?\n");
    read_word(TERM);
    upshift(TERM);
    if (w_eq(TERM, "YES")) {
        printf("SPUTTER, CRACKLE, BLIP,BLIP,BLIP!\n");
        printf("BANG!\n");
        printf("BANG, BANG!\n");
        printf("BANG, BANG, BANG!\n");
        printf("BANG, BANG, BANG!\n");
        printf("      BANG, BANG!\n");
        printf("            BANG!\n");
        if (OBJECT[2] == 0)
            OBJECT[2] = 20;
        if (OBJECT[11] == 0) {
            printf("             POW!!!!!!!!!!!!!!!!!\n");
            printf("I had to borrow something from you to bring you back.\n");
            OBJECT[11] = 9; LOCATION = 1; BONUS = BONUS - 4;
        } else {
            printf("            fizzle.\n");
            printf("Damn. I ran out of clovers. sorry, you're a corpse.\n");
            DEAD = 1;
        }
    }
}

/* ----------------------------------------------------------- SLOW_DEATH -- */

static void SLOW_DEATH(void)
{
    I = 1;
    do {
        ATONIC_SET("What\"ll you do?");
        getcommand(USAGE, PRED);
        if (w_eq(USAGE, "DRO"))
            PUT_DOWN();
        else
            printf("No time for that, you're sinking fast!\n");
        if (OBJECT[15] == 15)
            I = 16;
        I = I + 1;
    } while (!(I > 10));
    if (I < 15)
        LOCATION = 100;
}

/* ---------------------------------------------------------------- EXTRA -- */

static void EXTRA(void)
{
    if (SEE)
        BATTERY = BATTERY - 1;
    if (SEE && BATTERY < 1) {
        printf("The flashlight's batteries are dead!!!\n");
        SEE = 0;
    }
    if (SEE && BATTERY < 23)
        printf("The flashlight is growing dim...\n");
    TURN = TURN + 1;
    if (OBJECT[13] == 20) HISS = 0;
    else                  HISS = 1;
    if (HISS && OBJECT[2] == 0) {
        HISS = 0;
        LOCATION = 100;
        printf("The snake bites you, and you die!!!\n");
    }
    if (LOCATION == 100)
        DEAD_MAN();
    /* the Pascal has the SLOW_DEATH arm of this chain commented out */
    else
        GREMLIN();

    if (OBJECT[14] == 0) ROUTES[18][4] = 19;
    else                 ROUTES[18][4] = 8;

    if (OBJECT[9] == 10) {
        ROUTES[10][8] = 11;
        ROUTES[11][1] = 10;
    } else {
        ROUTES[10][8] = -11;
        ROUTES[11][1] = -10;
    }

    if (OBJECT[10] == 0 && OBJECT[3] != 0) {
        OBJECT[10] = LOCATION;
        printf("The fish slips out of your grasp!\n");
    }

    if (QUIT) {
        printf("Do you wish to quit? \n");
        read_word(TERM);
        upshift(TERM);
        if (!w_eq(TERM, "YES"))
            QUIT = 0;
    }
}

/* ----------------------------------------------------------- CRUNCH_DATA -- */

static void CRUNCH_DATA(void)
{
    ATONIC_SET("What\"ll you do?");
    do {
        writeln_atonic();
        getcommand(USAGE, PRED);
    } while (!IN_BOOK(USAGE, &verbs));
    DOLLOP = DEFINITION(USAGE, &verbs);
    switch (DOLLOP) {
    case 1: case 2: case 3: case 4: case 5:
        PICK_UP();
        break;
    case 6: case 7: case 8: case 9: case 10: case 11: case 12:
        GO_ALONG();
        break;
    case 13:
        printf("I need a shovel to do that.\n");
        break;
    case 14: case 15: case 16: case 17:
        MOVE_IT();
        break;
    case 18: case 19: case 20:
        printf("This is all you can see:\n");
        break;
    case 21: case 22: case 23: case 24:
        printf("Now, now, NO violence!\n");
        break;
    case 25:
        HELP_HIM();
        break;
    case 26:
        INVENTORY();
        break;
    case 27: case 28:
        if (OBJECT[12] == 0 || OBJECT[12] == LOCATION) SEE = 1;
        break;
    case 29: case 30:
        if (OBJECT[12] == 0 || OBJECT[12] == LOCATION) SEE = 0;
        break;
    case 31: case 32:
        QUIT = 1;
        break;
    case 33: case 34: case 35: case 36:
        PASSWORD();
        break;
    case 37:
        SCORE();
        break;
    case 38: case 39: case 40: case 41:
        PUT_DOWN();
        break;
    default:
        printf("I'n not sure how to do that.\n");
        break;
    }
}

/* ---------------------------------------------------------- CHANGE_LINES -- */

static void CHANGE_LINES(void)
{
    switch (LOCATION) {
    case 1:
        if (ROUTES[1][9] < 1)
            printf("There is a bookcase on the west wall.\n");
        else
            printf("There is a passage in the west wall.\n");
        break;
    case 2:
        if (ROUTES[1][9] < 1)
            printf("The stairs also go down and south.\n");
        else {
            printf("A passage leads east.\n");
            printf("The bookcase is to the south.\n");
        }
        break;
    case 4:
        if (ROUTES[4][7] < 1) {
            printf("There is a brick wall to the south with this\n");
            printf("message written on it: Speak, Fool, and you may");
            printf(" pass!\n");
        } else
            printf("South is a wall with a passage through it.\n");
        break;
    case 5:
        if (ROUTES[4][7] < 1)
            printf("West is a wall with a hole in it.\n");
        else {
            printf("West is a wall with this message: \n");
            printf("What is my two-faced name?\n");
        }
        break;
    case 10:
        if (ROUTES[10][8] < 1)
            printf("There is a skunk blocking the door.\n");
        break;
    case 11:
        if (ROUTES[10][8] < 1)
            printf("There is a skunk blocking the door.\n");
        if (OBJECT[14] != LOCATION) {
            ROUTES[11][6] = 16;
            ROUTES[16][2] = 11;
            printf("There is a hole in the floor.\n");
        } else {
            ROUTES[11][6] = -16;
            ROUTES[16][2] = -11;
        }
        break;
    case 13:
        if (!SEE)
            printf("It is too dark to see. Watch out for holes.\n");
        else {
            printf("You are in a room with a throne to the south.\n");
            printf("A passage leads west from here. \n");
            if (ROUTES[13][7] > 0)
                printf("There is a passage beside the throne.\n");
            printf("There is a hole in the ceiling.\n");
            if (OBJECT[17] == 0)
                printf("The sceptre will not fit through the hole.\n");
        }
        if (OBJECT[17] == 0) ROUTES[13][2] = -12;
        else                 ROUTES[13][2] = 12;
        break;
    case 14:
        if (SEE) {
            printf("You are in a small  King's chamber. Only one");
            printf(" exit is visible, a northeast door.\n");
        } else
            printf("It is too dark to see.\n");
        break;
    case 15:
        if (OBJECT[15] == 15)
            printf("You are standing on a log over the quicksand.\n");
        else {
            printf("You are sinking into the quicksand!\n");
            SLOW_DEATH();
        }
        break;
    case 16:
        if (OBJECT[14] == 11)
            printf("The ramp is blocked at the top.\n");
        break;
    case 19:
        if (OBJECT[10] != 19) {
            ROUTES[19][5] = -20;
            printf("There is a hungry tiger blocking the beach!\n");
        } else {
            ROUTES[19][5] = 20;
            printf("A tiger is eating a fish here.\n");
        }
        break;
    case 20:
        if (HISS && OBJECT[2] == 20)
            printf(" There is an asp here, hissing at you.\n");
        else
            printf(" There is a crushed asp here.\n");
        break;
    case 23:
        if (ROUTES[23][9] < 1) {
            printf(" A Sphinx in front of the pyramid asks:\n");
            printf("What walks on 4 legs in the morning, 2 in the\n");
            printf("afternoon, and 3 in the evening?\n");
        } else
            printf("There is a passage into the pyramid.\n");
        break;
    case 24:
        if (SEE) {
            printf("You are in a crypt with a northeast door. There");
            printf(" are hieroglyphics on the\n");
            printf("walls that say'Home of the Last Treasure'\n");
        } else {
            printf("You are inside a dark, gloomy puramid. you can");
            printf(" not see anything for the lack of light.\n");
        }
        break;
    case 100:
        printf("Oblivion\n");
        break;
    default:
        printf("\n");
        break;
    }
}

/* ----------------------------------------------------------- BASIC_LINES -- */

static void BASIC_LINES(void)
{
    printf("\n");
    switch (LOCATION) {
    case 1:
        printf("You are in a room that has one door to the south.\n");
        printf("A table is in this room and it has a note     \n");
        printf("scratched on it's surface, 'Type this word,   \n");
        printf("HELP, for some game information.'\n");
        break;
    case 2:
        printf("You are at a landing of a staircase that goes ");
        printf("up and north.\n");
        break;
    case 3:
        printf("You are in the attic.  Stairs go down and south. This is\n");
        printf(" where all treasures must be taken to score points.\n");
        break;
    case 4:
        printf("You are at the bottom of the staircase. \n");
        break;
    case 5:
        printf("You are in an apple orchard.  All the trees are ");
        printf("too young to bear fruit.\n");
        break;
    case 6:
        printf("You are in a forest of young evergreens, too small to climb.\n");
        break;
    case 7:
        printf("You are on a sandy beach. South is the ocean.\n");
        printf("East is a high, impassable mountain. \n");
        break;
    case 8:
        printf("The current has swept you to calm waters south ");
        printf("of a beach.\n");
        break;
    case 9:
        printf("You are in a garden. South is a beach. North\n");
        printf("Is an ocean. East is a cave-riddled mountain.\n");
        break;
    case 10:
        printf("You are on a grassy lawn. West is a forest. A\n");
        printf("Twig hut is to the southwest. North is an ocean.\n");
        printf("East is a mountain with cave openings.\n");
        break;
    case 11:
        printf("You are in a twig hut. There is a door to the north.\n");
        break;
    case 12:
        printf("You are in a buried hall. Passages go down,\n");
        printf("south, and southwest. light comes from above.\n");
        break;
    case 15:
        printf("You are in a room with exits to the north and \n");
        printf("east. Light comes from above. The floor is \n");
        printf("composed of quicksand.\n");
        break;
    case 16:
        printf("You are on a ramp going up to the north and   \n");
        printf("down to the south.             \n");
        break;
    case 17:
        printf("You are on a mountain top. To the north is ocean,\n");
        printf("a passage goes south, and to the east is a valley.\n");
        break;
    case 18:
        printf("You are west of a river in a valley. To the west is a ");
        printf("mountain passage.\n");
        break;
    case 19:
        printf("You are on the east bank of a river. To the north,\n");
        printf("south, and east is what looks like an impassable jungle.\n");
        break;
    case 20:
        printf("You are in a dense jungle, with twisty paths \n");
        printf("leading in all directions. The palm trees here \n");
        printf("are too slippery to climb. To the southwest you\n");
        printf("can see a large structure.\n");
        break;
    case 21:
        printf("You are on top of an Egyptian pyramid. A path ");
        printf("goes down and north of here.\n");
        break;
    case 22:
        printf("You are at a rocky place in the dense jungle.\n");
        break;
    case 23:
        printf("You are in a desert with a pyramid to the west.\n");
        break;
    case 100:
        printf("Darkness descends.\n");
        break;
    case 200:
        printf("The door opens, and Reality comes leaping in on\n");
        printf("this fantasy world! Reality engulfs you!! You\n");
        printf("cannot get out!!! The shock is so terrible, you\n");
        printf("die!!!!\n");
        printf("\n");
        LOCATION = 100;
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------- MAIN -- */

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    GAME = 1;
    while (GAME) {
        INIT_GAME();
        do {
            BASIC_LINES();
            CHANGE_LINES();
            if (LOCATION < 25)
                if ((SEE && ROUTES[LOCATION][5] == -100) ||
                    ROUTES[LOCATION][5] != -100)
                    GAD_ABOUT(LOCATION);
            if (LOCATION != 100)
                CRUNCH_DATA();
            EXTRA();
        } while (!(DEAD || QUIT || WINNER));
        if (DEAD || QUIT)
            SCORE();
        GAME = 0;
        printf("Would you like to play again? yes or no \n");
        read_word(PRED);
        upshift(PRED);
        if (w_eq(PRED, "YES")) GAME = 1;
    }
    return 0;
}
