/*
 * GoogleAdventure - plain C port of the .NET (C#) single-file console game
 * originally shipped as GoogleAdventure.exe (a ~73MB self-contained .NET 6
 * apphost bundling the whole CoreCLR runtime around a ~48KB game assembly
 * internally named "DoogleAdventure").
 *
 * Ported by decompiling DoogleAdventure.dll with ilspycmd and transcribing
 * the game/data logic (Game.cs, Item.cs, Obstacle.cs, Room.cs, Program.cs)
 * into freestanding C99. No external dependencies beyond the C standard
 * library - compiles to a single small native executable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define EOL "\r\n"
#else
#include <unistd.h>
#define EOL "\n"
#endif

/*
 * The original .NET build only ever emits "\r\n" as the terminator that
 * Console.WriteLine appends after each call; embedded "\n" bytes inside a
 * string literal (e.g. the blank line before "An exit appears...") are
 * passed straight through unmodified. Windows' C runtime, on the other
 * hand, silently rewrites *every* '\n' it sees to "\r\n" when stdout is in
 * its default text mode - so to reproduce the original byte-for-byte we
 * put stdout in binary mode and spell out "\r\n" (via EOL) ourselves only
 * at the actual end of each printed line.
 */

/* ------------------------------------------------------------------ */
/* ANSI colour layer                                                   */
/* ------------------------------------------------------------------ */
/*
 * The browser original draws the five letter friends in the Google brand
 * colours wherever their names appear in the prose ("...your friends red o,
 * yellow o, blue g, green l, and the always quirky red e?"). Plain console
 * output has no markup, so instead of tagging every string individually we
 * run each formatted line through a colouriser that recognises the friends'
 * names as whole words and wraps them in SGR sequences.
 *
 * The letter half of a name must be lower case to match, which is what keeps
 * the player's own "big blue G" in the intro uncoloured, exactly as the
 * browser version leaves it. (Set g_matchCaseOfLetter to 0 below if you
 * would rather have that one painted blue too.)
 *
 * Colour is on when stdout is a console that can do VT sequences; NO_COLOR
 * or --no-color turns it off, --color forces it on, --basic-color drops to
 * the 16-colour palette for terminals without truecolor.
 */

#define COLOR_OFF   0
#define COLOR_TRUE  1
#define COLOR_BASIC 2

static int g_color = COLOR_OFF;
static int g_colorWanted = -1;   /* -1 = auto-detect, else a COLOR_* value */
static const int g_matchCaseOfLetter = 1;

#define SGR_RESET "\033[0m"

typedef struct {
    const char *phrase;
    size_t      len;
    const char *trueSeq;
    const char *basicSeq;
    const char *seq;     /* whichever of the two is in force */
} ColorPhrase;

/* Google brand colours: blue #4285F4, red #EA4335, yellow #FBBC05,
 * green #34A853. */
#define TC_BLUE   "\033[38;2;66;133;244m"
#define TC_RED    "\033[38;2;234;67;53m"
#define TC_YELLOW "\033[38;2;251;188;5m"
#define TC_GREEN  "\033[38;2;52;168;83m"

#define B16_BLUE   "\033[1;94m"
#define B16_RED    "\033[1;91m"
#define B16_YELLOW "\033[1;93m"
#define B16_GREEN  "\033[1;92m"

static ColorPhrase g_phrases[] = {
    { "yellow o", 8, TC_YELLOW, B16_YELLOW, NULL },
    { "green l",  7, TC_GREEN,  B16_GREEN,  NULL },
    { "blue g",   6, TC_BLUE,   B16_BLUE,   NULL },
    { "red o",    5, TC_RED,    B16_RED,    NULL },
    { "red e",    5, TC_RED,    B16_RED,    NULL },
};
#define PHRASE_COUNT (sizeof(g_phrases) / sizeof(g_phrases[0]))

static const char *g_playerSeq = NULL; /* colour of the 'G' on the map */

static void selectColorSequences(void)
{
    for (size_t i = 0; i < PHRASE_COUNT; i++)
        g_phrases[i].seq = (g_color == COLOR_BASIC) ? g_phrases[i].basicSeq
                                                    : g_phrases[i].trueSeq;
    g_playerSeq = (g_color == COLOR_BASIC) ? B16_BLUE : TC_BLUE;
}

/*
 * want: -1 = auto-detect, COLOR_OFF / COLOR_TRUE / COLOR_BASIC = forced.
 */
static void initColor(int want)
{
    if (want == COLOR_OFF) { g_color = COLOR_OFF; return; }

    int forced = (want > 0);
    if (!forced) {
        const char *nc = getenv("NO_COLOR");
        if (nc && nc[0]) { g_color = COLOR_OFF; return; }
    }

#ifdef _WIN32
    if (!forced && !_isatty(_fileno(stdout))) { g_color = COLOR_OFF; return; }
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        int vt = 0;
        if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
            if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) vt = 1;
            else if (SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) vt = 1;
        }
        /* A redirected/piped handle has no console mode; that is fine when
         * the user asked for colour explicitly. */
        if (!vt && !forced) { g_color = COLOR_OFF; return; }
    }
    g_color = (want == COLOR_BASIC) ? COLOR_BASIC : COLOR_TRUE;
#else
    if (!forced && !isatty(fileno(stdout))) { g_color = COLOR_OFF; return; }
    if (want > 0) {
        g_color = want;
    } else {
        const char *ct = getenv("COLORTERM");
        int truecolor = ct && (strstr(ct, "truecolor") || strstr(ct, "24bit"));
        g_color = truecolor ? COLOR_TRUE : COLOR_BASIC;
    }
#endif

    selectColorSequences();
}

static int isWordChar(char c)
{
    return isalnum((unsigned char)c) || c == '\'';
}

/*
 * Copy `in` to `out`, wrapping every whole-word occurrence of a friend's
 * name in its colour. Anything that would not fit is copied plain.
 */
static void colorize(const char *in, char *out, size_t outsize)
{
    size_t oi = 0;
    size_t i = 0;

    while (in[i]) {
        int matched = 0;

        if (i == 0 || !isWordChar(in[i - 1])) {
            for (size_t p = 0; p < PHRASE_COUNT; p++) {
                const ColorPhrase *cp = &g_phrases[p];
                size_t k;
                for (k = 0; k < cp->len; k++) {
                    if (!in[i + k]) break;
                    if ((char)tolower((unsigned char)in[i + k]) != cp->phrase[k]) break;
                }
                if (k != cp->len) continue;
                if (isWordChar(in[i + cp->len])) continue;
                /* The letter itself is lower case in the game's own text. */
                if (g_matchCaseOfLetter && in[i + cp->len - 1] != cp->phrase[cp->len - 1])
                    continue;

                size_t seqLen = strlen(cp->seq), rstLen = strlen(SGR_RESET);
                if (oi + seqLen + cp->len + rstLen >= outsize) break;
                memcpy(out + oi, cp->seq, seqLen);      oi += seqLen;
                memcpy(out + oi, in + i, cp->len);      oi += cp->len;
                memcpy(out + oi, SGR_RESET, rstLen);    oi += rstLen;
                i += cp->len;
                matched = 1;
                break;
            }
        }

        if (!matched) {
            if (oi + 1 >= outsize) break;
            out[oi++] = in[i++];
        }
    }
    out[oi] = 0;
}

/*
 * Every line of game output goes through here; it is printf() plus the
 * colouriser.
 */
static void gprintf(const char *fmt, ...)
{
    char buf[4096];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (g_color == COLOR_OFF) {
        fputs(buf, stdout);
        return;
    }

    char cbuf[8192];
    colorize(buf, cbuf, sizeof(cbuf));
    fputs(cbuf, stdout);
}


/* ------------------------------------------------------------------ */
/* Small portable helpers                                             */
/* ------------------------------------------------------------------ */

static int strcasecmp_port(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static char *trimStr(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = 0;
    return s;
}

static void firstToken(const char *s, char *out, size_t outsize)
{
    size_t i = 0;
    while (s[i] && s[i] != ' ' && i < outsize - 1) { out[i] = s[i]; i++; }
    out[i] = 0;
}

static void toLowerStr(char *dst, const char *src, size_t dstsize)
{
    size_t i = 0;
    for (; src[i] && i < dstsize - 1; i++) dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = 0;
}

/* ------------------------------------------------------------------ */
/* Data model                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *key, *article, *name, *hint, *find, *useText;
    int reusable;
    int count; /* mutable: inventory quantity */
} Item;

typedef struct {
    const char *key, *article, *name;
    const char *requiredItem;
    const char *resultText;
    const char *find;
    const char *status;
    const char *giveItems[3];
    int giveItemCount;
    const char *leaves[3];
    int leaveCount;
    int hasOpensExit; /* mutable: consumed once used */
    const char *opensExitDir;
    const char *opensExitTarget;
} Obstacle;

typedef struct {
    const char *obstacleKey;
    const char *dir;    /* may be NULL */
    const char *target; /* may be NULL */
} RoomObstacleEntry;

#define MAX_ROOMS          100
#define MAX_EXITS          8
#define MAX_ROOM_ITEMS     2
#define MAX_ROOM_OBSTACLES 8
#define MAX_ROOM_LETTERS   2

typedef struct {
    char id[8];
    const char *type;        /* map glyph fallback, e.g. "." */
    const char *ch;          /* explicit map glyph, or NULL */
    const char *description;

    const char *exitDirs[MAX_EXITS];
    const char *exitTargets[MAX_EXITS];
    int exitCount;

    const char *items[MAX_ROOM_ITEMS];
    int itemCount;

    RoomObstacleEntry obstacles[MAX_ROOM_OBSTACLES];
    int obstacleCount;

    const char *letters[MAX_ROOM_LETTERS];
    int letterCount;

    int visited;
} Room;

typedef struct { const char *name; int found; } LetterEntry;

typedef struct { const char *alias; const char *command; } Alias;

/* ------------------------------------------------------------------ */
/* Static game data (mirrors Game.BuildItems / BuildObstacles)         */
/* ------------------------------------------------------------------ */

static Item g_items[] = {
    { "sword", "the", "I'm Feeling Lucky sword",
      "I might need something sharp and to the point.",
      "A ray of light reflects off a shiny metal object. You realize you have found the I'm Feeling Lucky sword",
      "You strike Precision with the I'm Feeling Lucky sword.", 0, 0 },
    { "shield", "the", "Shield of Android",
      "I need something that will cover all of recall.",
      "The Shield of Android hangs on a wall",
      "You bash recall with the Shield of Android.", 0, 0 },
    { "banana peel", "a", "banana peel",
      "The floor seems to have a lot of traction.",
      "You find a banana peel next to the compost bin. Clearly someone needs to work on their basketball skills",
      "You strategically drop the banana peel on the robot dog's path.", 0, 0 },
    { "flyswatter", "a", "fly swatter",
      "Find something used for eliminating bugs",
      "You notice a fly swatter on someone's desk",
      "You smack the monitor with the fly swatter.", 0, 0 },
    { "costume", "a", "costume",
      "A rhyme about alligators leaving.",
      "Laying in the middle of the path is a crocodile costume",
      "You put on the crocodile costume and say, \"See you later alligator.\"", 0, 0 },
    { "stickers", "an", "assortment of doodle stickers",
      "You need something to trade.",
      "You find a bowl full of doodle stickers with a warning to take just one or two",
      "You ask to trade for your doodle stickers.", 0, 0 },
    { "apple", "a", "red apple", "", NULL,
      "You ask to trade for your apple.", 0, 0 },
    { "banana", "a", "banana", "", NULL,
      "You ask to trade for your banana.", 0, 0 },
    { "latte", "a", "latte", "", NULL,
      "You ask to trade for your latte.", 0, 0 },
    { "diet soda", "a", "can of diet soda", "", NULL,
      "You ask to trade for your diet soda.", 0, 0 },
    { "quinoa", "a", "cup of quinoa", "", NULL, NULL, 0, 0 },
    { "sticky note", "a", "sticky note with some clues on it",
      "Who around here might know the password?", NULL, NULL, 1, 0 },
    { "map", "a", "map",
      "They will need something to help them get around.",
      "You see a map on a bench. Type \"grab\" to pick it up",
      "You give the map to the horde of nooglers.", 0, 0 },
};
#define ITEM_COUNT (sizeof(g_items) / sizeof(g_items[0]))

static Obstacle g_obstacles[] = {
    { .key = "robot", .article = "a", .name = "robot dog", .requiredItem = "banana peel",
      .resultText = "The robot dog walks unknowingly towards the banana peel. Its foot slips and its circuitry shuts down in response. It lays on its side, happily wagging its robot tail.",
      .find = "A cute robot dog paces back and forth, clearly guarding a door" },
    { .key = "closed door", .article = "an", .name = "alligator", .requiredItem = "costume",
      .resultText = "The alligator looks up at you and replies, \"after a while crocodile\" and walks away.",
      .find = "An alligator blocks the building to the north" },
    { .key = "bug", .article = "a", .name = "giant bug", .requiredItem = "flyswatter",
      .status = "clearly frustrating a group of engineers. No one will help you until they've resolved this issue.",
      .resultText = "The screen shows static for a second and reverts back. The distraction allows an engineer to see something they didn't before. They fix the bug and answer your question about your missing buddy.",
      .find = "You find a frustrated group of engineers trying to solve a bug" },
    { .key = "precision", .article = "the", .name = "ferocious twin ghouls of Precision and Recall", .requiredItem = "sword",
      .resultText = "The sword strikes Precision and the magical incantation for luck bursts like a grenade of light through the ghoul. Its twin laughs at your precise but harmless cuts. Clearly you'll need another way of going after Recall.",
      .find = "You are greeted by the ferocious twin ghouls of Precision and Recall",
      .leaves = { "recall" }, .leaveCount = 1 },
    { .key = "recall", .article = "the", .name = "grieving twin spirit of Recall", .requiredItem = "shield",
      .resultText = "The broad side of the shield manages to hit most of the ghoul. Its screeches in pain and vanishes before your eyes.",
      .find = "A sad, grieving spirit of Recall still blocks your way as it mourns its twin",
      .hasOpensExit = 1, .opensExitDir = "west", .opensExitTarget = "5037-4" },
    { .key = "excited intern", .article = "an", .name = "excited intern", .requiredItem = "stickers",
      .resultText = "The intern is so excited about the stickers she immediately gives you her banana. You wonder what you can trade for this?",
      .giveItems = { "banana" }, .giveItemCount = 1, .status = "holding a banana" },
    { .key = "happy noogler", .article = "a", .name = "happy noogler", .requiredItem = "banana",
      .resultText = "The noogler admits he doesn't even know what quinoa is and gratefully accepts your banana.",
      .giveItems = { "quinoa" }, .giveItemCount = 1, .status = "amazed that the micro-kitchen even offers quinoa" },
    { .key = "grumpy engineer", .article = "a", .name = "grumpy engineer", .requiredItem = "diet soda",
      .resultText = "\"You found the last one!\", the grumpy engineer no longer looks so frustrated and puts down the latte with a beautiful flower made out of foam for the diet soda.",
      .giveItems = { "latte" }, .giveItemCount = 1, .status = "holding a latte" },
    { .key = "twitchy googler", .article = "a", .name = "over caffeinated googler", .requiredItem = "latte",
      .resultText = "The over caffeinated googler gladly accepts your latte. \"Thank you for the latte, I hear you are looking for your friend. I'll only tell you this once.\"\n\n1. I am the beginning of everything, the end of everywhere. I'm the beginning of eternity, the end of time and space. What am I?\n2. I am the loneliest number.\n3. Check the clue number.\n4. Take a number. Double it. Add 21. Subtract 15. Divide by 2. Subtract your original number.\n5. To make this number even, you take the s out.",
      .giveItems = { "sticky note" }, .giveItemCount = 1, .status = "with a glass of water" },
    { .key = "VP", .article = "the", .name = "vice president of some product you don't recognize", .requiredItem = "quinoa",
      .resultText = "You suggest water and quinoa to the VP. Her eyes clearly show you've taken away a moment of respite, but the truth of your statement convinces her to give up the diet soda.",
      .giveItems = { "diet soda" }, .giveItemCount = 1, .status = "with a diet coke in her hands" },
    { .key = "lockbox", .article = "a", .name = "giant lockbox with an alphanumeric keypad", .requiredItem = "sticky note",
      .resultText = "You read your sticky note with the clues:\n1. I am the beginning of everything, the end of everywhere. I'm the beginning of eternity, the end of time & space. What am I?\n2. I am the loneliest number.\n3. Check the clue number.\n4. Take a number. Double it. Add 21. Subtract 15. Divide by 2. Subtract your original number.\n5. To make this number even, you take the s out.",
      .status = "which could easily fit a missing letter" },
    { .key = "horde", .article = "a", .name = "horde of lost nooglers (new googlers)", .requiredItem = "map",
      .resultText = "They gratefully accept the map. Now that they know where to go they immediately leave. You, on the other hand, feel a bit lost.",
      .find = "A horde of lost nooglers (new googlers) surrounds you and asks how to get around. Type \"use\" to give them your map to help them out" },
};
#define OBSTACLE_COUNT (sizeof(g_obstacles) / sizeof(g_obstacles[0]))

static LetterEntry g_letters[] = {
    { "red o", 0 }, { "yellow o", 0 }, { "blue g", 0 }, { "green l", 0 }, { "red e", 0 },
};
#define LETTER_COUNT (sizeof(g_letters) / sizeof(g_letters[0]))

static const char *g_why[] = {
    "why? why not?",
    "why? because it's there",
    "why as much wood as a woodchuck would chuck if a woodchuck could chuck wood.",
    "why? look to the map to find out where you've been and where you might go",
    "why? one is the loneliest number",
};
#define WHY_COUNT (sizeof(g_why) / sizeof(g_why[0]))

static const Alias g_aliases[] = {
    { "ask", "use" }, { "attack", "use" }, { "drop", "use" }, { "give", "use" }, { "hit", "use" },
    { "open", "use" }, { "place", "use" }, { "put", "use" }, { "show", "use" }, { "speak", "use" },
    { "say", "use" }, { "swat", "use" }, { "swing", "use" }, { "talk", "use" }, { "throw", "use" },
    { "climb", "up" }, { "u", "up" }, { "up", "up" },
    { "d", "down" }, { "down", "down" }, { "slide", "down" },
    { "e", "east" }, { "east", "east" },
    { "w", "west" }, { "west", "west" },
    { "n", "north" }, { "north", "north" },
    { "s", "south" }, { "south", "south" },
    { "e1337", "e1337" }, { "E1337", "e1337" },
    { "e7331", "wrong" }, { "e3333", "wrong" }, { "e1177", "wrong" }, { "e3773", "wrong" }, { "e1308", "wrong" },
    { "exit", "exits" }, { "exits", "exits" },
    { "friend", "friends" }, { "friends", "friends" }, { "letter", "friends" }, { "letters", "friends" },
    { "g", "grab" }, { "get", "grab" }, { "grab", "grab" }, { "pick", "grab" }, { "pickup", "grab" }, { "take", "grab" },
    { "hello", "hello" }, { "hi", "hello" },
    { "help", "help" },
    { "inventory", "inventory" }, { "inv", "inventory" },
    { "look", "map" }, { "ls", "map" }, { "m", "map" }, { "map", "map" }, { "maps", "map" },
    { "no", "no" },
    { "quack", "quack" },
    { "wait", "wait" },
    { "why", "why" },
    { "yes", "yes" },
};
#define ALIAS_COUNT (sizeof(g_aliases) / sizeof(g_aliases[0]))

/* ------------------------------------------------------------------ */
/* Rooms - built imperatively, mirrors Game.BuildRooms                 */
/* ------------------------------------------------------------------ */

static Room g_rooms[MAX_ROOMS];
static int g_roomCount = 0;

static Room *addRoom(const char *id, const char *type, const char *ch, const char *desc)
{
    Room *r = &g_rooms[g_roomCount++];
    memset(r, 0, sizeof(*r));
    strncpy(r->id, id, sizeof(r->id) - 1);
    r->type = type;
    r->ch = ch;
    r->description = desc;
    return r;
}

static Room *cosmetic(const char *id, const char *type, const char *ch)
{
    return addRoom(id, type, ch, "");
}

static void ex(Room *r, ...)
{
    va_list ap;
    va_start(ap, r);
    const char *dir;
    while ((dir = va_arg(ap, const char *)) != NULL) {
        const char *target = va_arg(ap, const char *);
        r->exitDirs[r->exitCount] = dir;
        r->exitTargets[r->exitCount] = target;
        r->exitCount++;
    }
    va_end(ap);
}

static void roomAddItem(Room *r, const char *item) { r->items[r->itemCount++] = item; }
static void roomAddLetter(Room *r, const char *letter) { r->letters[r->letterCount++] = letter; }

static void roomAddObstacle(Room *r, const char *key, const char *dir, const char *target)
{
    r->obstacles[r->obstacleCount].obstacleKey = key;
    r->obstacles[r->obstacleCount].dir = dir;
    r->obstacles[r->obstacleCount].target = target;
    r->obstacleCount++;
}

static void BuildRooms(void)
{
    Room *r;

    cosmetic("4839-3", "#", "/");
    r = addRoom("4840", ".", "<", "You enter the lobby of the Mars building and above you a slide slithers its way to your side.");
    ex(r, "up", "4840-2", "south", "5040", NULL);
    r = addRoom("4840-2", ".", "<", "You are on the 2nd floor. Exiting the elevator, you see the top of the slide. The temptation to ride it is very strong.");
    roomAddItem(r, "sword");
    ex(r, "up", "4840-3", "down", "4840", NULL);
    r = addRoom("4840-3", ".", "<", "You are on the 3rd floor of the Mars building. A graveyard of broken lava lamps sits in the middle of the floor. A skybridge connects you to the other buildings.");
    ex(r, "down", "4840-2", "south", "5240-3", "west", "5038-3", NULL);
    cosmetic("4841-3", "#", "|");
    r = addRoom("4845", ".", NULL, "You make your way into the Venus building.");
    ex(r, "south", "5045", "east", "4847", NULL);
    cosmetic("4846", "=", NULL);
    r = addRoom("4847", ".", NULL, "A bridge allows you to cross from building to building. You see a gym lined with treadmills and weights.");
    ex(r, "west", "4845", "east", "4849", NULL);
    addRoom("4848", ".", NULL, "");
    r = addRoom("4849", ".", NULL, "You've entered the Morning Star Cafe. A flurry of food choices inundates your senses and ability to make a choice.");
    ex(r, "west", "4847", NULL);
    roomAddLetter(r, "green l");
    cosmetic("4938-3", "=", "/");
    cosmetic("4939-3", "=", "/");
    cosmetic("4940", "+", NULL);
    cosmetic("4940-3", "=", "^");
    cosmetic("4942", "~", NULL);
    cosmetic("4943", "~", NULL);
    cosmetic("4944", "#", "-");
    cosmetic("4945", "+", NULL);
    r = addRoom("5035", ".", NULL, "A beach volleyball court with a small sand castle in the middle grabs your attention.");
    ex(r, "east", "5038", "south", "5236", NULL);
    addRoom("5036", ".", NULL, "");
    cosmetic("5037", "+", "+");
    r = addRoom("5036-4", ".", NULL, "This is what you call the end of the hallway.");
    ex(r, "east", "5037-4", NULL);
    roomAddLetter(r, "yellow o");
    r = addRoom("5037-4", ".", NULL, "You find a whiteboard filled with cartoonish drawings.");
    ex(r, "east", "5038-4", "west", "5036-4", NULL);
    roomAddItem(r, "stickers");
    r = addRoom("5038", ".", "<", "You find a strange hallway with clean rooms and signs with drawings of hazmat suits on them.");
    ex(r, "up", "5038-2", "east", "5040", "west", "5035", NULL);
    r = addRoom("5038-2", ".", "<", "The 2nd floor of the Saturn building.");
    ex(r, "up", "5038-3", "down", "5038", NULL);
    r = addRoom("5038-3", ".", "<", "You are on the 3rd floor of the Saturn building. You see a table with various jigsaw puzzles half finished. A skybridge connects you to a different building.");
    ex(r, "up", "5038-4", "down", "5038-2", "east", "4840-3", NULL);
    r = addRoom("5038-4", ".", ">", "You are on the 4th floor. A mural of a violent green wave in the ocean greets your entrance.");
    ex(r, "down", "5038-3", NULL);
    roomAddObstacle(r, "precision", NULL, NULL);
    cosmetic("5039", "+", NULL);
    r = addRoom("5040", ".", NULL, "Palm trees and corporate art occupy the concrete walkway between the 3 buildings.");
    ex(r, "west", "5038", "south", "5240", "north", "4840", "east", "5043", NULL);
    roomAddItem(r, "costume");
    cosmetic("5040-3", "=", "^");
    addRoom("5041", ".", NULL, "");
    cosmetic("5042", "=", NULL);
    r = addRoom("5043", "=", NULL, "A bridge hovers over a shallow creek. A cute duck quacks at you.");
    ex(r, "west", "5040", "east", "5045", NULL);
    addRoom("5044", ".", NULL, "");
    r = addRoom("5045", ".", NULL, "Two buildings hug the giant walkway and open up into a parking lot.");
    ex(r, "south", "5245", "west", "5043", "east", "5047", NULL);
    roomAddObstacle(r, "closed door", "north", "4845");
    addRoom("5046", ".", NULL, "");
    r = addRoom("5047", ".", NULL, "A T-rex skeleton towers over you and a beach volleyball court invites you to play.");
    ex(r, "west", "5045", "east", "5049", NULL);
    addRoom("5048", ".", NULL, "");
    r = addRoom("5049", ".", NULL, "The Google campus opens up before you. Colorful lawn chairs greet your passage, and a building on your left hosts an image of your likeness and all of your friends.");
    ex(r, "east", "5051", "south", "5249", "west", "5047", NULL);
    addRoom("5050", ".", NULL, "");
    r = addRoom("5051", ".", NULL, "A park opens up all around you with a small waterfall showing you a path west towards the campus.");
    ex(r, "west", "5049", "south", "5151", NULL);
    cosmetic("5052", "#", "|");
    cosmetic("5135", "#", "\\");
    cosmetic("5136", "#", "\\");
    cosmetic("5140", "+", NULL);
    cosmetic("5140-3", "=", "^");
    cosmetic("5142", "~", NULL);
    cosmetic("5143", "~", NULL);
    cosmetic("5144", "#", "-");
    cosmetic("5145", "+", NULL);
    cosmetic("5149", "+", NULL);
    r = addRoom("5236", ".", NULL, "You come upon a busy street full of self-driving cars zooming past you.");
    ex(r, "north", "5035", "south", "5436", NULL);
    r = addRoom("5240", ".", "<", "On your left you see a fake river comprised of rocks and on your right a piano. In fact, everything looks inviting except the elevators.");
    ex(r, "up", "5240-2", "north", "5040", NULL);
    r = addRoom("5240-2", ".", "<", "You are on the 2nd floor. Cubicles as far as the eye can see.");
    ex(r, "up", "5240-3", "down", "5240", NULL);
    r = addRoom("5240-3", ".", "<", "You are on the 3rd floor. You see a small micro-kitchen and a skybridge that will connect you to another building.");
    ex(r, "up", "5240-4", "down", "5240-2", "north", "4840-3", NULL);
    r = addRoom("5240-4", ".", ">", "You are on the 4th floor. An empty room with a giant whale, whose belly serves as a couch, greets you.");
    ex(r, "down", "5240-3", NULL);
    roomAddItem(r, "shield");
    cosmetic("5241-3", "#", "|");
    cosmetic("5243", "~", NULL);
    r = addRoom("5245", ".", NULL, "You find a small game room.");
    ex(r, "north", "5045", NULL);
    roomAddItem(r, "flyswatter");
    r = addRoom("5247", ".", NULL, "You enter the Messenger cafe, the smell of sandwiches and popcorn in the air.");
    ex(r, "east", "5248", NULL);
    roomAddLetter(r, "red e");
    r = addRoom("5248", ".", NULL, "You are greeted by a giant wall of jasmine.");
    ex(r, "east", "5249", NULL);
    roomAddObstacle(r, "robot", "west", "5247");
    r = addRoom("5249", ".", NULL, "You are inside the Mercury building. You spot a micro-kitchen and some open cubicles.");
    ex(r, "north", "5049", "west", "5248", NULL);
    roomAddItem(r, "banana peel");
    cosmetic("5336", "+", NULL);
    cosmetic("5346", "#", "|");
    cosmetic("5347", "#", "-");
    r = addRoom("5434", ".", NULL, "You find the massage room.");
    ex(r, "east", "5435", NULL);
    roomAddLetter(r, "blue g");
    r = addRoom("5435", ".", NULL, "You see a sea of cubicles.");
    ex(r, "east", "5436", NULL);
    roomAddObstacle(r, "bug", "west", "5434");
    r = addRoom("5436", ".", NULL, "You enter a building to find a yet another gym on your right.");
    ex(r, "north", "5236", "south", "5636", "west", "5435", NULL);
    r = addRoom("5151", ".", NULL, "You see a large field with googlers playing ultimate frisbee and dodgeball.");
    ex(r, "south", "5251", NULL);
    roomAddObstacle(r, "horde", "north", "5051");
    cosmetic("5152", "#", "|");
    cosmetic("5250", "#", "|");
    r = addRoom("5251", ".", NULL, "A sidewalk circles around a palm tree.");
    ex(r, "north", "5151", "south", "5450", NULL);
    roomAddItem(r, "map");
    cosmetic("5252", "#", "|");
    addRoom("5350", ".", "/", "");
    addRoom("5351", ".", "/", "");
    cosmetic("5449", "#", "|");
    r = addRoom("5450", ".", NULL, "You see a statue of a metal man peeking out of a building. A park is just across the street.");
    ex(r, "north", "5251", NULL);
    cosmetic("5451", "#", "|");
    cosmetic("5536", "+", NULL);
    cosmetic("5635", "#", "|");
    r = addRoom("5636", ".", NULL, "You enter a small cafeteria. People are standing around but no one is talking.");
    ex(r, "north", "5436", "south", "5736", NULL);
    roomAddObstacle(r, "twitchy googler", NULL, NULL);
    roomAddObstacle(r, "grumpy engineer", NULL, NULL);
    roomAddObstacle(r, "VP", NULL, NULL);
    roomAddObstacle(r, "happy noogler", NULL, NULL);
    roomAddObstacle(r, "excited intern", NULL, NULL);
    cosmetic("5637", "#", "|");
    r = addRoom("5736", ".", NULL, "A boring, somewhat empty room.");
    ex(r, "north", "5636", NULL);
    roomAddObstacle(r, "lockbox", NULL, NULL);
}

/* ------------------------------------------------------------------ */
/* Lookups                                                             */
/* ------------------------------------------------------------------ */

static Item *findItem(const char *key)
{
    for (size_t i = 0; i < ITEM_COUNT; i++)
        if (!strcasecmp_port(g_items[i].key, key)) return &g_items[i];
    return NULL;
}

static Obstacle *findObstacle(const char *key)
{
    for (size_t i = 0; i < OBSTACLE_COUNT; i++)
        if (!strcasecmp_port(g_obstacles[i].key, key)) return &g_obstacles[i];
    return NULL;
}

static LetterEntry *findLetter(const char *name)
{
    for (size_t i = 0; i < LETTER_COUNT; i++)
        if (!strcasecmp_port(g_letters[i].name, name)) return &g_letters[i];
    return NULL;
}

static Room *findRoom(const char *id)
{
    for (int i = 0; i < g_roomCount; i++)
        if (!strcasecmp_port(g_rooms[i].id, id)) return &g_rooms[i];
    return NULL;
}

static const char *resolveAlias(const char *token)
{
    for (size_t i = 0; i < ALIAS_COUNT; i++)
        if (!strcasecmp_port(g_aliases[i].alias, token)) return g_aliases[i].command;
    return NULL;
}

static const char *roomGetExit(Room *r, const char *dir)
{
    for (int i = 0; i < r->exitCount; i++)
        if (!strcasecmp_port(r->exitDirs[i], dir)) return r->exitTargets[i];
    return NULL;
}

static void roomSetExit(Room *r, const char *dir, const char *target)
{
    for (int i = 0; i < r->exitCount; i++) {
        if (!strcasecmp_port(r->exitDirs[i], dir)) { r->exitTargets[i] = target; return; }
    }
    if (r->exitCount < MAX_EXITS) {
        r->exitDirs[r->exitCount] = dir;
        r->exitTargets[r->exitCount] = target;
        r->exitCount++;
    }
}

/* ------------------------------------------------------------------ */
/* Game state                                                          */
/* ------------------------------------------------------------------ */

static char g_current[8] = "5450";
static char g_lastAction[64] = "";
static int g_actions = 0;
static int g_quacks = 0;
static int g_playing = 0;
static int g_won = 0;
static clock_t g_startClock;

static Room *R(void) { return findRoom(g_current); }

/* ------------------------------------------------------------------ */
/* Map / geometry helpers                                              */
/* ------------------------------------------------------------------ */

static int floorOf(const char *id)
{
    const char *dash = strchr(id, '-');
    if (!dash) return 1;
    return atoi(dash + 1);
}

static void coordOf(const char *id, int *x, int *y)
{
    char ybuf[3] = { id[0], id[1], 0 };
    char xbuf[3] = { id[2], id[3], 0 };
    *y = atoi(ybuf);
    *x = atoi(xbuf);
}

static void makeId(int x, int y, int floor, char *out, size_t outsize)
{
    if (floor > 1) snprintf(out, outsize, "%02d%02d-%d", y, x, floor);
    else snprintf(out, outsize, "%02d%02d", y, x);
}

/* ------------------------------------------------------------------ */
/* English list join, e.g. "a, b, and c"                               */
/* ------------------------------------------------------------------ */

static void joinEnglish(const char **items, int count, char *out, size_t outsize)
{
    if (count == 0) { out[0] = 0; return; }
    if (count == 1) { snprintf(out, outsize, "%s", items[0]); return; }
    if (count == 2) { snprintf(out, outsize, "%s and %s", items[0], items[1]); return; }
    out[0] = 0;
    for (int i = 0; i < count - 1; i++) {
        strncat(out, items[i], outsize - strlen(out) - 1);
        strncat(out, ", ", outsize - strlen(out) - 1);
    }
    strncat(out, "and ", outsize - strlen(out) - 1);
    strncat(out, items[count - 1], outsize - strlen(out) - 1);
}

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */

static void Help(void);
static void ShowInventory(void);
static void ShowFriends(void);
static void ShowExits(void);
static void ShowMap(void);
static void Grab(void);
static void Use(void);
static void EnterCode(void);
static void WrongCode(void);
static void Quack(void);
static void Move(const char *dir);
static void Describe(int movement);
static void FindLetters(void);
static void Win(void);
static const char *Verb(const char *dir);

/* ------------------------------------------------------------------ */
/* Game logic (mirrors Game.cs 1:1)                                    */
/* ------------------------------------------------------------------ */

static void Help(void)
{
    gprintf("Type single word commands, no need to describe the subject. For example, 'grab banana peel' should just be 'grab' or 'use banana peel' should just be 'use'." EOL);
    gprintf("" EOL);
    gprintf("Commands: north, south, east, west, up, down, grab, why, wait, inventory, use, help, exits, map, friends." EOL);
    gprintf("Also: quit exits the standalone version." EOL);
}

static void ShowInventory(void)
{
    gprintf("Inventory:" EOL);
    int any = 0;
    for (size_t i = 0; i < ITEM_COUNT; i++) {
        if (g_items[i].count > 0) {
            any = 1;
            gprintf("  %d %s" EOL, g_items[i].count, g_items[i].name);
        }
    }
    if (!any) gprintf("  (empty)" EOL);
}

static void ShowFriends(void)
{
    gprintf("Friends:" EOL);
    for (size_t i = 0; i < LETTER_COUNT; i++)
        gprintf("  %s %s" EOL, g_letters[i].name, g_letters[i].found ? "found" : "missing");
}

static void Grab(void)
{
    Room *r = R();
    for (int i = 0; i < r->itemCount; i++)
        findItem(r->items[i])->count++;
    r->itemCount = 0;
    ShowInventory();
}

static void Use(void)
{
    Room *r = R();
    if (r->obstacleCount == 0) {
        gprintf("You are not sure that will do much right here." EOL);
        return;
    }

    RoomObstacleEntry newList[MAX_ROOM_OBSTACLES];
    int newCount = 0;
    const char *hint = NULL;

    for (int i = 0; i < r->obstacleCount; i++) {
        RoomObstacleEntry entry = r->obstacles[i];
        Obstacle *ob = findObstacle(entry.obstacleKey);
        Item *reqItem = ob->requiredItem ? findItem(ob->requiredItem) : NULL;

        if (reqItem && reqItem->count > 0) {
            if (!reqItem->reusable) reqItem->count--;
            if (reqItem->useText && reqItem->useText[0]) gprintf("%s" EOL, reqItem->useText);

            char text2[4096];
            snprintf(text2, sizeof(text2), "%s", ob->resultText ? ob->resultText : "That seems to work.");

            if (ob->hasOpensExit) {
                char extra[160];
                snprintf(extra, sizeof(extra), "\n\nAn exit appears towards the %s.", ob->opensExitDir);
                strncat(text2, extra, sizeof(text2) - strlen(text2) - 1);
                roomSetExit(r, ob->opensExitDir, ob->opensExitTarget);
                ob->hasOpensExit = 0;
            }
            if (entry.dir && entry.target) {
                char extra[160];
                snprintf(extra, sizeof(extra), "\n\nAn exit appears towards the %s.", entry.dir);
                strncat(text2, extra, sizeof(text2) - strlen(text2) - 1);
                roomSetExit(r, entry.dir, entry.target);
            }
            for (int g = 0; g < ob->giveItemCount; g++)
                findItem(ob->giveItems[g])->count++;

            gprintf("%s" EOL, text2);

            for (int lv = 0; lv < ob->leaveCount; lv++) {
                if (newCount < MAX_ROOM_OBSTACLES) {
                    newList[newCount].obstacleKey = ob->leaves[lv];
                    newList[newCount].dir = NULL;
                    newList[newCount].target = NULL;
                    newCount++;
                }
            }
        } else {
            if (newCount < MAX_ROOM_OBSTACLES) newList[newCount++] = entry;
            if (ob->requiredItem && findItem(ob->requiredItem)->hint[0] && !hint)
                hint = findItem(ob->requiredItem)->hint;
        }
    }

    for (int i = 0; i < newCount; i++) r->obstacles[i] = newList[i];
    r->obstacleCount = newCount;

    if (r->obstacleCount > 0) {
        if (hint) gprintf("Hint: %s" EOL, hint);
        for (int i = 0; i < r->obstacleCount; i++) {
            Obstacle *ob2 = findObstacle(r->obstacles[i].obstacleKey);
            if (ob2->find) gprintf("%s" EOL, ob2->find);
            else gprintf("You %s %s" EOL, ob2->article, ob2->name);
        }
    }
    ShowInventory();
}

static void EnterCode(void)
{
    Room *r = R();
    if (r->obstacleCount > 0 && !strcasecmp_port(r->obstacles[0].obstacleKey, "lockbox")) {
        r->obstacleCount = 0;
        r->letters[r->letterCount++] = "red o";
        gprintf("You finally google search all the clues and sure enough the lockbox opens." EOL);
        FindLetters();
    } else {
        gprintf("I get it, you're not a noob, but enough with the l33t speak." EOL);
    }
}

static void WrongCode(void)
{
    Room *r = R();
    if (r->obstacleCount > 0 && !strcasecmp_port(r->obstacles[0].obstacleKey, "lockbox")) {
        gprintf("The lockbox whirls and makes chunking noises, but nothing happens. You must have used the wrong code." EOL);
    } else {
        gprintf("That's a strange command. Are you sure you are in the correct spot?" EOL);
    }
}

static void Quack(void)
{
    g_quacks++;
    if (!strcmp(g_current, "5043")) gprintf("quack! quack! quack! quack!" EOL);
    else gprintf("That is a strange sound to make." EOL);
}

static void ShowExits(void)
{
    Room *r = R();
    if (r->exitCount != 0) {
        char buf[512];
        joinEnglish(r->exitDirs, r->exitCount, buf, sizeof(buf));
        gprintf("Possible exits: %s." EOL, buf);
    }
}

static void ShowMap(void)
{
    int floor = floorOf(g_current);
    int haveMap = findItem("map")->count > 0;

    int minX = 999, maxX = -999, minY = 999, maxY = -999;
    int any = 0;
    for (int i = 0; i < g_roomCount; i++) {
        Room *r = &g_rooms[i];
        if (floorOf(r->id) != floor) continue;
        if (!(haveMap || r->visited)) continue;
        int x, y;
        coordOf(r->id, &x, &y);
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        any = 1;
    }
    if (!any) return;

    gprintf("Map - floor %d:" EOL, floor);
    for (int y = minY; y <= maxY; y++) {
        char line[256];
        int pos = 0;
        for (int x = minX; x <= maxX; x++) {
            char id[16];
            makeId(x, y, floor, id, sizeof(id));
            char c;
            if (!strcasecmp_port(id, g_current)) {
                c = 'G';
            } else {
                Room *r = findRoom(id);
                if (!r || (!r->visited && !haveMap)) {
                    c = ' ';
                } else if (r->ch) {
                    c = r->ch[0];
                } else if (r->obstacleCount > 0 && r->visited) {
                    c = '@';
                } else {
                    c = r->type[0];
                }
            }
            if (pos < (int)sizeof(line) - 1) line[pos++] = c;
        }
        line[pos] = 0;
        if (g_color != COLOR_OFF && strchr(line, 'G')) {
            /* You are the big blue G. */
            for (int k = 0; line[k]; k++) {
                if (line[k] == 'G') gprintf("%s%c%s", g_playerSeq, line[k], SGR_RESET);
                else gprintf("%c", line[k]);
            }
            gprintf(EOL);
        } else {
            gprintf("%s" EOL, line);
        }
    }
}

static void Win(void)
{
    g_won = 1;
    clock_t endClock = clock();
    ShowFriends();
    gprintf("" EOL);
    gprintf("Congratulations on winning the game!" EOL);
    double elapsedSec = (double)(endClock - g_startClock) / CLOCKS_PER_SEC;
    int seconds = (int)(elapsedSec + 0.5);
    gprintf("It took %d actions and %d seconds." EOL, g_actions, seconds);
    gprintf("You quacked %d time%s." EOL, g_quacks, g_quacks == 1 ? "" : "s");
    gprintf("Now get back to work." EOL);
}

static void FindLetters(void)
{
    Room *r = R();
    if (r->letterCount == 0) return;
    for (int i = 0; i < r->letterCount; i++) {
        LetterEntry *le = findLetter(r->letters[i]);
        if (le && !le->found) {
            le->found = 1;
            gprintf("You found %s!" EOL, le->name);
        }
    }
    r->letterCount = 0;
    ShowFriends();
    int all = 1;
    for (size_t i = 0; i < LETTER_COUNT; i++) if (!g_letters[i].found) all = 0;
    if (all) Win();
}

static const char *Verb(const char *dir)
{
    if (!strcasecmp_port(dir, "north")) return "move north";
    if (!strcasecmp_port(dir, "south")) return "move south";
    if (!strcasecmp_port(dir, "east")) return "move east";
    if (!strcasecmp_port(dir, "west")) return "move west";
    if (!strcasecmp_port(dir, "up")) return "move up";
    if (!strcasecmp_port(dir, "down")) return "move down";
    return "stand still";
}

static void Describe(int movement)
{
    ShowMap();
    if (movement) gprintf("You %s." EOL, Verb(g_lastAction));

    Room *r = R();
    gprintf("%s" EOL, r->description);

    for (int i = 0; i < r->itemCount; i++) {
        Item *it = findItem(r->items[i]);
        if (it->find) gprintf("%s" EOL, it->find);
        else gprintf("%s %s" EOL, it->article, it->name);
    }

    for (int i = 0; i < r->obstacleCount; i++) {
        Obstacle *ob = findObstacle(r->obstacles[i].obstacleKey);
        if (ob->find) {
            gprintf("%s" EOL, ob->find);
            continue;
        }
        char text[1024];
        snprintf(text, sizeof(text), "%s %s", ob->article, ob->name);
        if (ob->status && ob->status[0]) {
            strncat(text, " ", sizeof(text) - strlen(text) - 1);
            strncat(text, ob->status, sizeof(text) - strlen(text) - 1);
        }
        if (r->obstacles[i].dir) {
            strncat(text, " is blocking the exit to the ", sizeof(text) - strlen(text) - 1);
            strncat(text, r->obstacles[i].dir, sizeof(text) - strlen(text) - 1);
        }
        gprintf("You %s." EOL, text);
    }

    ShowExits();
    FindLetters();
}

static void Move(const char *dir)
{
    Room *r = R();
    const char *target = roomGetExit(r, dir);
    if (target) {
        Room *nr = findRoom(target);
        if (!nr) {
            gprintf("It seems the map is incomplete... good luck with that." EOL);
            return;
        }
        strncpy(g_current, target, sizeof(g_current) - 1);
        g_current[sizeof(g_current) - 1] = 0;
        nr->visited = 1;
        Describe(1);
    } else {
        ShowMap();
        gprintf("You failed to %s" EOL, Verb(dir));
        ShowExits();
    }
}

static void Handle(const char *action)
{
    static const char *dirs[6] = { "north", "south", "east", "west", "up", "down" };
    int isDir = 0;
    for (int i = 0; i < 6; i++) if (!strcmp(action, dirs[i])) { isDir = 1; break; }

    if (isDir) {
        Move(action);
    } else if (!strcmp(action, "help")) {
        Help();
    } else if (!strcmp(action, "inventory")) {
        ShowInventory();
    } else if (!strcmp(action, "grab")) {
        Grab();
    } else if (!strcmp(action, "map")) {
        ShowMap();
    } else if (!strcmp(action, "use")) {
        Use();
    } else if (!strcmp(action, "exits")) {
        ShowExits();
    } else if (!strcmp(action, "why")) {
        gprintf("%s" EOL, g_why[rand() % WHY_COUNT]);
    } else if (!strcmp(action, "friends")) {
        ShowFriends();
    } else if (!strcmp(action, "e1337")) {
        EnterCode();
    } else if (!strcmp(action, "wrong")) {
        WrongCode();
    } else if (!strcmp(action, "quack")) {
        Quack();
    } else if (!strcmp(action, "hello")) {
        gprintf("No one responds to your greeting. You have a sinking feeling that you are not in control of your actions." EOL);
    } else if (!strcmp(action, "wait")) {
        gprintf("You wait. Surprisingly, this accomplishes exactly what waiting usually accomplishes." EOL);
    } else {
        gprintf("That's a strange command. Are you sure you are in the correct spot?" EOL);
    }
    g_actions++;
}

static void Start(void)
{
    g_playing = 1;
    g_startClock = clock();
    Help();
    gprintf("" EOL);
    gprintf("A strange tingle trickles across your skin. You feel lightheaded and sit down. Feeling better you stand up again and notice your reflection in a window. You are still the same big blue G you've always been and you can't help but smile." EOL);
    gprintf("" EOL);
    gprintf("But wait! Where are your friends red o, yellow o, blue g, green l, and the always quirky red e?" EOL);
    gprintf("" EOL);
    Describe(0);
}

static void Run(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    initColor(g_colorWanted);
    gprintf("Would you like to play a game? (yes/no)" EOL);

    char line[1024];
    while (!g_won) {
        gprintf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;

        char *trimmed = trimStr(line);
        if (strlen(trimmed) == 0) continue;

        char token[256];
        firstToken(trimmed, token, sizeof(token));

        if (!strcasecmp_port(token, "quit") || !strcasecmp_port(token, "q")) break;

        const char *resolved = resolveAlias(token);
        if (resolved) {
            strncpy(g_lastAction, resolved, sizeof(g_lastAction) - 1);
        } else {
            toLowerStr(g_lastAction, token, sizeof(g_lastAction));
        }
        g_lastAction[sizeof(g_lastAction) - 1] = 0;

        if (!g_playing) {
            if (!strcmp(g_lastAction, "yes")) {
                Start();
            } else if (!strcmp(g_lastAction, "no")) {
                gprintf("The only winning move is not to play." EOL);
                return;
            } else if (!strcmp(g_lastAction, "help")) {
                Help();
            } else if (!strcmp(g_lastAction, "why")) {
                gprintf("%s" EOL, g_why[rand() % WHY_COUNT]);
            } else {
                gprintf("Would you like to play a game? (yes/no)" EOL);
            }
        } else {
            Handle(g_lastAction);
        }
    }
}

static void Usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [--color|--basic-color|--no-color]\n", argv0);
    fprintf(stderr, "  --color        force ANSI colour on (default: on when stdout is a console)\n");
    fprintf(stderr, "  --basic-color  use the 16-colour palette instead of 24-bit colour\n");
    fprintf(stderr, "  --no-color     plain text (also honoured: the NO_COLOR environment variable)\n");
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (!strcasecmp_port(argv[i], "--no-color") || !strcasecmp_port(argv[i], "--no-colour")) {
            g_colorWanted = COLOR_OFF;
        } else if (!strcasecmp_port(argv[i], "--color") || !strcasecmp_port(argv[i], "--colour")) {
            g_colorWanted = COLOR_TRUE;
        } else if (!strcasecmp_port(argv[i], "--basic-color") || !strcasecmp_port(argv[i], "--basic-colour")) {
            g_colorWanted = COLOR_BASIC;
        } else {
            Usage(argv[0]);
            return 2;
        }
    }

    srand((unsigned)time(NULL));
    BuildRooms();
    Room *start = findRoom(g_current);
    if (start) start->visited = 1;
    Run();
    return 0;
}
