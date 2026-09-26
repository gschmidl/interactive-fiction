/* tower_data.h -- generated from the recovered PRIMOS files LEX2 and ROAD2.
 * LEX2: INIT_GAME skips one leading line, then reads DIRECTIONS, TREASURES,
 * MOVERS, PROPS, PLACES, ANIMATE, INANIMATE, NOUNS, VERBS (each ends "XXX").
 * ROAD2: FILE OF INTEGER (32-bit, big-endian) holding ROUTES[1..59][1..10]
 * then objects, read until a value > 150 is seen (21 objects, pad, then 200).
 */

static const char directions_w[10][4] = {
    "NOR", "UP ", "N-E", "EAS", "S-E", "DOW", "SOU", "S-W",
    "WES", "N-W",
};
static const Glossary directions = { directions_w, 10 };

static const char treasures_w[18][4] = {
    "SIL", "BEL", "RUB", "EGG", "LEA", "CRY", "BRO", "TOR",
    "EME", "GIZ", "ROU", "DIA", "CHI", "TEA", "PLA", "COI",
    "GOL", "FIL",
};
static const Glossary treasures = { treasures_w, 18 };

static const char movers_w[26][4] = {
    "IRO", "KEY", "GLA", "BEA", "STR", "SKE", "FOS", "BON",
    "COP", "CRO", "SCI", "TON", "ICE", "CUB", "DEA", "RAT",
    "OLD", "BRA", "PAR", "SCR", "PAP", "NOT", "BIR", "NES",
    "PUR", "AMU",
};
static const Glossary movers = { movers_w, 26 };

static const char props_w[25][4] = {
    "TRE", "TAB", "CHA", "DUS", "WAT", "GRA", "GRA", "TEN",
    "JUN", "GLO", "WAL", "WIN", "BOO", "SHE", "DOO", "RUS",
    "TRO", "KIN", "TYR", "REX", "WYS", "BUS", "WRI", "VAM",
    "BAT",
};
static const Glossary props = { props_w, 25 };

static const char places_w[24][4] = {
    "PIL", "STA", "CEL", "COL", "LAW", "FOR", "CLE", "GRA",
    "TRE", "MOA", "CAS", "PAR", "TOR", "CHA", "CAV", "PAS",
    "STA", "LIM", "PIT", "DEE", "OFF", "WAL", "PAR", "BAS",
};
static const Glossary places = { places_w, 24 };

static const char animate_w[44][4] = {
    "SIL", "BEL", "RUB", "EGG", "LEA", "CRY", "BRO", "TOR",
    "EME", "GIZ", "ROU", "DIA", "CHI", "TEA", "PLA", "COI",
    "GOL", "FIL", "IRO", "KEY", "GLA", "BEA", "STR", "SKE",
    "FOS", "BON", "COP", "CRO", "SCI", "TON", "ICE", "CUB",
    "DEA", "RAT", "OLD", "BRA", "PAR", "SCR", "PAP", "NOT",
    "BIR", "NES", "PUR", "AMU",
};
static const Glossary animate = { animate_w, 44 };

static const char inanimate_w[59][4] = {
    "TRE", "TAB", "CHA", "DUS", "WAT", "GRA", "GRA", "TEN",
    "JUN", "GLO", "WAL", "WIN", "BOO", "SHE", "DOO", "RUS",
    "TRO", "KIN", "TYR", "REX", "WYS", "BUS", "WRI", "VAM",
    "BAT", "PIL", "STA", "CEL", "COL", "LAW", "FOR", "CLE",
    "GRA", "TRE", "MOA", "CAS", "PAR", "TOR", "CHA", "CAV",
    "PAS", "STA", "LIM", "PIT", "DEE", "OFF", "WAL", "PAR",
    "BAS", "NOR", "UP ", "N-E", "EAS", "S-E", "DOW", "SOU",
    "S-W", "WES", "N-W",
};
static const Glossary inanimate = { inanimate_w, 59 };

static const char nouns_w[103][4] = {
    "NOR", "UP ", "N-E", "EAS", "S-E", "DOW", "SOU", "S-W",
    "WES", "N-W", "SIL", "BEL", "RUB", "EGG", "LEA", "CRY",
    "BRO", "TOR", "EME", "GIZ", "ROU", "DIA", "CHI", "TEA",
    "PLA", "COI", "GOL", "FIL", "IRO", "KEY", "GLA", "BEA",
    "STR", "SKE", "FOS", "BON", "COP", "CRO", "SCI", "TON",
    "ICE", "CUB", "DEA", "RAT", "OLD", "BRA", "PAR", "SCR",
    "PAP", "NOT", "BIR", "NES", "PUR", "AMU", "TRE", "TAB",
    "CHA", "DUS", "WAT", "GRA", "GRA", "TEN", "JUN", "GLO",
    "WAL", "WIN", "BOO", "SHE", "DOO", "RUS", "TRO", "KIN",
    "TYR", "REX", "WYS", "BUS", "WRI", "VAM", "BAT", "PIL",
    "STA", "CEL", "COL", "LAW", "FOR", "CLE", "GRA", "TRE",
    "MOA", "CAS", "PAR", "TOR", "CHA", "CAV", "PAS", "STA",
    "LIM", "PIT", "DEE", "OFF", "WAL", "PAR", "BAS",
};
static const Glossary nouns = { nouns_w, 103 };

static const char verbs_w[47][4] = {
    "CAR", "HOL", "NET", "TAK", "GET", "CLI", "JUM", "RUN",
    "ENT", "WAL", "TUR", "SWI", "GO ", "DIG", "MOV", "OPE",
    "PUS", "SHO", "EXA", "LOO", "REA", "ATT", "FIG", "HIT",
    "KIL", "HEL", "INV", "LIG", "ON ", "UNL", "KEY", "QUI",
    "STO", "ASK", "SAY", "SPE", "YEL", "SCO", "DRO", "THR",
    "TOS", "LEA", "GIV", "OFF", "SEL", "STU", "SIN",
};
static const Glossary verbs = { verbs_w, 47 };

/* ROUTES[1..59][1..10] as loaded by INIT_GAME */
static const int init_routes[59][10] = {
    {     0,     0,     0,    -2,     0,     0,   200,     0,     0,     0 },  /*  1 */
    {     4,     0,     3,     5,     5,     0,     5,     6,     6,     3 },  /*  2 */
    {     6,     0,     5,     5,     2,   -33,     4,     2,     6,     6 },  /*  3 */
    {   -58,     0,     3,     2,     2,     0,     2,     2,     3,     3 },  /*  4 */
    {     5,    12,     5,     6,     8,     0,     7,     7,     2,     3 },  /*  5 */
    {     6,    11,     5,     3,     2,     0,     7,     9,     5,     6 },  /*  6 */
    {     5,    10,     5,     8,     5,     0,     7,     6,     9,     6 },  /*  7 */
    {     5,     0,     5,     8,     8,   -13,     5,     7,     7,     5 },  /*  8 */
    {   -59,     0,     6,     7,     7,     0,     6,     6,     9,     6 },  /*  9 */
    {     0,     0,     0,     0,     0,     7,     0,     0,     0,     0 },  /* 10 */
    {     0,     0,     0,     0,     0,     6,     0,     0,     0,     0 },  /* 11 */
    {     0,     0,     0,     0,     0,     5,     0,     0,     0,     0 },  /* 12 */
    {     0,    -8,     0,    14,     0,    13,     0,     0,    13,     0 },  /* 13 */
    {     0,    14,     0,    15,     0,     0,    17,     0,    13,     0 },  /* 14 */
    {     0,    47,     0,    15,     0,    23,     0,     0,    14,     0 },  /* 15 */
    {     0,     0,    16,    17,     0,     0,    19,     0,    16,     0 },  /* 16 */
    {    14,     0,     0,     0,     0,    17,    20,     0,    16,     0 },  /* 17 */
    {     0,     0,     0,     0,     0,    30,    21,     0,     0,     0 },  /* 18 */
    {    16,     0,     0,     0,    19,     0,    40,     0,     0,     0 },  /* 19 */
    {    17,    31,     0,     0,     0,    31,     0,    20,     0,     0 },  /* 20 */
    {     0,     0,     0,     0,    18,     0,     0,     0,     0,     0 },  /* 21 */
    {     0,     0,     0,    23,    26,     0,     0,     0,     0,     0 },  /* 22 */
    {     0,    15,     0,    24,     0,     0,     0,     0,    22,     0 },  /* 23 */
    {     0,     0,     0,    39,     0,     0,     0,     0,    23,     0 },  /* 24 */
    {     0,     0,     0,    26,     0,    32,     0,     0,     0,     0 },  /* 25 */
    {     0,     0,     0,    27,     0,     0,     0,     0,    25,    22 },  /* 26 */
    {     0,     0,     0,     0,     0,     0,    30,    29,    26,     0 },  /* 27 */
    {     0,    59,     0,    29,     0,    28,     0,     0,     0,     0 },  /* 28 */
    {     0,     0,    27,     0,     0,     0,     0,     0,    28,     0 },  /* 29 */
    {    27,     0,     0,    18,     0,     0,    30,     0,     0,     0 },  /* 30 */
    {     0,    20,     0,    20,     0,     0,     0,     0,    34,     0 },  /* 31 */
    {     0,     0,     0,    33,     0,    25,    35,     0,     0,     0 },  /* 32 */
    {     0,     3,     0,     0,     0,     0,    32,     0,     0,     0 },  /* 33 */
    {    31,     0,     0,    35,     0,     0,    37,     0,    34,     0 },  /* 34 */
    {    32,     0,     0,    36,     0,     0,    34,     0,    38,     0 },  /* 35 */
    {     0,     0,     0,     0,     0,     0,    39,     0,    35,     0 },  /* 36 */
    {    34,     0,     0,     0,     0,     0,     0,     0,     0,     0 },  /* 37 */
    {     0,    35,     0,     0,     0,     0,     0,     0,    57,     0 },  /* 38 */
    {    36,     0,     0,     0,     0,     0,     0,     0,    24,     0 },  /* 39 */
    {    19,     0,     0,     0,     0,     0,     0,     0,     0,     0 },  /* 40 */
    {     0,    43,     0,     0,    44,     0,    43,    42,     0,     0 },  /* 41 */
    {     0,    46,    41,    44,    43,    44,     0,     0,     0,     0 },  /* 42 */
    {    41,    45,    44,     0,     0,     0,    45,     0,     0,    42 },  /* 43 */
    {     0,    42,     0,     0,     0,    41,     0,    43,    42,    41 },  /* 44 */
    {    43,     0,     0,     0,     0,     0,     0,     0,     0,     0 },  /* 45 */
    {     0,     0,     0,     0,     0,    42,     0,     0,     0,     0 },  /* 46 */
    {     0,    48,     0,     0,     0,    15,     0,     0,     0,     0 },  /* 47 */
    {     0,    49,     0,     0,     0,    47,     0,     0,     0,     0 },  /* 48 */
    {     0,    50,     0,     0,     0,    48,     0,     0,   -51,     0 },  /* 49 */
    {     0,    52,     0,     0,     0,    49,     0,     0,     0,     0 },  /* 50 */
    {     0,     0,     0,    49,     0,     0,     0,     0,     0,     0 },  /* 51 */
    {    53,     0,     0,     0,     0,    50,     0,     0,     0,     0 },  /* 52 */
    {     0,     0,     0,     0,     0,    54,    52,     0,     0,     0 },  /* 53 */
    {     0,    53,     0,     0,     0,     0,     0,     0,    55,     0 },  /* 54 */
    {     0,     0,     0,    54,     0,    56,     0,     0,     0,     0 },  /* 55 */
    {     0,    55,     0,     0,     0,    57,    57,     0,     0,     0 },  /* 56 */
    {     0,    56,     0,    38,     0,     0,    56,     0,     0,     0 },  /* 57 */
    {     0,     0,     0,     0,     0,     0,    -4,     0,     0,     0 },  /* 58 */
    {     0,     0,    28,     0,     0,    28,     9,     0,     0,     0 },  /* 59 */
};

/* OBJECT[1..41] as loaded by INIT_GAME */
static const int init_object[41] = {
      52,   11,   56,   40,   63,   63,   63,   23,   54,   10,
      40,   40,    8,   54,   39,   27,   12,   51,   55,   11,
       7,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     200,
};

