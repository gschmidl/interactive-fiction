/* ankh_data.h -- generated from the recovered PRIMOS files LEXICON and ROADS.
 * LEXICON: read by INIT_GAME in the order TREASURES, MOVERS, PROPS, PLACES,
 * DIRECTIONS, ANIMATE, INANIMATE, NOUNS, VERBS (each list ends with "XXX").
 * ROADS: a PRIMOS Pascal FILE OF INTEGER (32-bit, big-endian) holding
 * ROUTES[1..24][1..10] followed by OBJECT[1..17].
 */

static const char treasures_w[12][4] = {
    "SIL", "ANK", "JAD", "BAR", "JEW", "SCE", "ONY", "BRA",
    "GOL", "OXE", "PLA", "ELE",
};
static const Glossary treasures = { treasures_w, 12 };

static const char movers_w[17][4] = {
    "APP", "MYN", "BIR", "DAI", "TUN", "FIS", "CLO", "LIG",
    "FLA", "IGN", "ROC", "RUB", "RAF", "POL", "LOG", "SHO",
    "NET",
};
static const Glossary movers = { movers_w, 17 };

static const char props_w[19][4] = {
    "SNA", "ASP", "EVE", "TRE", "PAL", "BOO", "GRA", "DUS",
    "MUD", "PIL", "SAN", "SME", "SKU", "SPH", "TAB", "THR",
    "WAT", "TIG", "DOO",
};
static const Glossary props = { props_w, 19 };

static const char places_w[28][4] = {
    "ATT", "BEA", "CAV", "HAL", "BUR", "KIN", "CHA", "DES",
    "PYR", "EGI", "FOR", "GAR", "TOP", "MOU", "HIL", "HOL",
    "TWI", "HUT", "JUN", "LAW", "ORC", "QUI", "VAL", "RIV",
    "STA", "IN ", "OUT", "CRY",
};
static const Glossary places = { places_w, 28 };

static const char directions_w[10][4] = {
    "NOR", "UP ", "N-E", "EAS", "S-E", "DOW", "SOU", "S-W",
    "WES", "N-W",
};
static const Glossary directions = { directions_w, 10 };

static const char animate_w[29][4] = {
    "SIL", "ANK", "JAD", "BAR", "JEW", "SCE", "ONY", "BRA",
    "GOL", "OXE", "PLA", "ELE", "APP", "MYN", "BIR", "DAI",
    "TUN", "FIS", "CLO", "LIG", "FLA", "IGN", "ROC", "RUB",
    "RAF", "POL", "LOG", "SHO", "NET",
};
static const Glossary animate = { animate_w, 29 };

static const char inanimate_w[56][4] = {
    "SNA", "ASP", "EVE", "TRE", "PAL", "BOO", "GRA", "DUS",
    "MUD", "PIL", "SAN", "SME", "SKU", "SPH", "TAB", "THR",
    "WAT", "TIG", "ATT", "BEA", "CAV", "HAL", "BUR", "KIN",
    "CHA", "DES", "PYR", "EGI", "FOR", "GAR", "TOP", "MOU",
    "HIL", "HOL", "TWI", "HUT", "JUN", "LAW", "ORC", "QUI",
    "VAL", "RIV", "STA", "IN ", "OUT", "CRY", "NOR", "UP ",
    "N-E", "EAS", "S-E", "DOW", "SOU", "S-W", "WES", "N-W",
};
static const Glossary inanimate = { inanimate_w, 56 };

static const char nouns_w[85][4] = {
    "SIL", "ANK", "JAD", "BAR", "JEW", "SCE", "ONY", "BRA",
    "GOL", "OXE", "PLA", "ELE", "APP", "MYN", "BIR", "DAI",
    "TUN", "FIS", "CLO", "LIG", "FLA", "IGN", "ROC", "RUB",
    "RAF", "POL", "LOG", "SHO", "NET", "SNA", "ASP", "EVE",
    "TRE", "PAL", "BOO", "GRA", "DUS", "MUD", "PIL", "SAN",
    "SME", "SKU", "SPH", "TAB", "THR", "WAT", "TIG", "ATT",
    "BEA", "CAV", "HAL", "BUR", "KIN", "CHA", "DES", "PYR",
    "EGI", "FOR", "GAR", "TOP", "MOU", "HIL", "HOL", "TWI",
    "HUT", "JUN", "LAW", "ORC", "QUI", "VAL", "RIV", "STA",
    "IN ", "OUT", "CRY", "NOR", "UP ", "N-E", "EAS", "S-E",
    "DOW", "SOU", "S-W", "WES", "N-W",
};
static const Glossary nouns = { nouns_w, 85 };

static const char verbs_w[41][4] = {
    "CAR", "HOL", "NET", "TAK", "GET", "CLI", "JUM", "RUN",
    "ENT", "WAL", "TUR", "GO ", "DIG", "MOV", "OPE", "PUS",
    "SHO", "EXA", "LOO", "SME", "ATT", "FIG", "HIT", "KIL",
    "HEL", "INV", "LIG", "ON ", "UNL", "OFF", "QUI", "STO",
    "ASK", "SAY", "SPE", "YEL", "SCO", "DRO", "THR", "TOS",
    "LEA",
};
static const Glossary verbs = { verbs_w, 41 };

/* ROUTES[1..24][1..10] as loaded by INIT_GAME */
static const int init_routes[24][10] = {
    {     0,     0,     0,     0,     0,     0,   200,     0,    -2,     0 },  /*  1 */
    {     3,     3,     0,    -1,     0,     4,     4,     0,     0,     0 },  /*  2 */
    {     0,     0,     0,     0,     0,     2,     2,     0,     0,     0 },  /*  3 */
    {     2,     2,     0,     0,     0,     0,    -5,     0,     0,     0 },  /*  4 */
    {     6,     0,    10,     9,     9,     0,     7,     7,     4,     6 },  /*  5 */
    {     6,     0,     5,     6,     9,     0,     5,     5,     6,     6 },  /*  6 */
    {     5,     0,     9,     0,     8,     8,     8,     8,     0,     5 },  /*  7 */
    {     7,     7,     7,     8,     8,     8,     8,     8,     8,     7 },  /*  8 */
    {    10,     0,    10,    12,    12,     0,     7,     7,     5,     6 },  /*  9 */
    {     8,     0,     8,    12,    12,     8,     9,   -11,     6,     8 },  /* 10 */
    {   -10,     0,     0,     0,     0,   -16,     0,     0,     0,     0 },  /* 11 */
    {     0,    17,     0,     0,     0,    13,    15,     9,     0,    10 },  /* 12 */
    {     0,    12,     0,     0,  -100,     0,   -14,     0,    15,     0 },  /* 13 */
    {     0,     0,    13,     0,  -100,     0,     0,     0,     0,     0 },  /* 14 */
    {    16,     0,     0,    13,     0,     0,     0,     0,     0,     0 },  /* 15 */
    {    11,    11,     0,     0,     0,    15,    15,     0,     0,     0 },  /* 16 */
    {     8,     0,     8,    18,     0,    18,    12,     0,     0,     8 },  /* 17 */
    {     8,     0,     8,   -19,     8,     8,     8,     8,    17,     8 },  /* 18 */
    {     0,     0,     0,     0,    20,     0,     0,     0,    18,     0 },  /* 19 */
    {    20,     0,    20,    20,    23,     0,    22,    21,    19,    20 },  /* 20 */
    {    20,     0,     0,     0,     0,    20,     0,     0,     0,     0 },  /* 21 */
    {    20,     0,    20,    20,    20,     0,    20,    20,    20,    20 },  /* 22 */
    {    20,     0,    20,    20,     0,     0,     0,     0,   -24,    20 },  /* 23 */
    {     0,     0,    23,     0,  -100,     0,     0,     0,     0,     0 },  /* 24 */
};

/* OBJECT[1..17] as loaded by INIT_GAME */
static const int init_object[17] = {
      24,   20,    3,   16,   21,   17,   -1,    2,    9,    8,
       9,    5,   22,   11,    6,   -1,   14,
};

