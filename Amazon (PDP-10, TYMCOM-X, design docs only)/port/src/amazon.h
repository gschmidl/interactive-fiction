/*  AMAZON -- a Windows port of Carl Baltrunas' unfinished Tymshare game.
 *
 *  Data model transcribed literally from AMAZON.OFF (21-Oct-78), the
 *  MACRO-10 offset definitions for the item control blocks.  Every field
 *  macro below carries the PDP-10 definition it came from.
 *
 *  PDP-10 bit numbering: bit 0 is the MOST significant bit of a 36-bit word.
 *  "nnnBk" in MACRO-10 means "mask nnn positioned so its rightmost bit is
 *  bit k".  W36FLD(w,mask,rightbit) implements exactly that.
 */
#ifndef AMAZON_H
#define AMAZON_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t w36;                    /* a PDP-10 word, 36 bits used */

#define W36MASK  0777777777777ULL
#define HALFMASK 0777777ULL

/*  MACRO-10 "maskBrightbit" field access on a 36-bit word.
 *
 *  Each AT_xxx / CT_xxx / CON_xxx name below expands to the two tokens
 *  "mask, rightbit", so these have to be functions: a macro would count
 *  its arguments before expanding them and see one argument too few.  */
static __inline int w36fld(w36 word, unsigned mask, int rb)
{
    return (int)((word >> (35 - rb)) & (w36)mask);
}
static __inline void w36put(w36 *word, unsigned mask, int rb, int v)
{
    *word &= ~(((w36)mask) << (35 - rb));
    *word |=  ((((w36)v) & (w36)mask) << (35 - rb));
}
#define W36FLD(w,F)     w36fld((w), F)
#define W36PUT(wp,F,v)  w36put(&(wp), F, (v))

#define LH(w) (((w) >> 18) & HALFMASK)
#define RH(w) ((w) & HALFMASK)
#define XWD(l,r) (((((w36)(l)) & HALFMASK) << 18) | (((w36)(r)) & HALFMASK))

/* ---------------------------------------------------------------- *
 *  ITEM CONTROL BLOCK OFFSETS                        (AMAZON.OFF)
 * ---------------------------------------------------------------- */
#define IT_TYP 0        /* IT$TYP==0   ITEM TYPE WORD         */
#define IT_CTL 1        /* IT$CTL==1   CONTROL ITEM           */
#define IT_IDP 2        /* IT$IDP==2   IDENTIFICATION POINTER */
#define IT_LOC 3        /* IT$LOC==3   LOCATION POINTER       */

/* Item types */
#define TY_OBJ 0        /* TY$OBJ==0   TYPE = OBJECT / CONTAINER */
#define TY_CMD 1        /* TY$CMD==1   TYPE = COMMAND            */

/* Control bits: CT$BIT==776B7, four 2-bit fields S-G-C-O.
   CT$SLF==3B1  CT$GRP==3B3  CT$CMP==3B5  CT$OTH==3B7            */
#define CT_SLF 3, 1
#define CT_GRP 3, 3
#define CT_CMP 3, 5
#define CT_OTH 3, 7
#define CT_ADR 0777777, 35   /* CT$ADR==777777 ADDRESS OF CONTROL LOGIC */

/* Identification pointers */
#define ID_STA 0777777, 17   /* ID$STA==777777B17 STATISTICS BLOCK */
#define ID_ADR 0777777, 35   /* ID$ADR==777777B35 USER ID BLOCK    */

/* Keyed pointer access keys (BK$KEY==777777B17) */
#define KY_STR (1u<<0)  /* KY$STR==1B0  SET IF STRENGTH OF N > REQUIRED */
#define KY_INT (1u<<1)  /* KY$INT==1B1  INTELLIGENCE                    */
#define KY_WIS (1u<<2)  /* KY$WIS==1B2  WISDOM                          */
#define KY_CON (1u<<3)  /* KY$CON==1B3  CONSTITUTION                    */
#define KY_SIZ (1u<<4)  /* KY$SIZ==1B4  SIZE                            */
#define KY_AGI (1u<<5)  /* KY$AGI==1B5  AGILITY                         */
#define KY_DEX (1u<<6)  /* KY$DEX==1B6  DEXTERITY                       */
#define KY_CHA (1u<<7)  /* KY$CHA==1B7  CHARISMA                        */
/* KY$LVL==37B12 level required, KY$AMT==37B17 amount required          */

/* ---------------------------------------------------------------- *
 *  PERSONAL ATTRIBUTES BLOCK                         (AMAZON.OFF)
 *  AT$PRM  STR INT WIS CHA FLT   (five 5-bit fields, 0..31)
 *  AT$PHY  CON SIZ AGI DEX
 *  AT$LNG  languages understood, 1 bit each (0-35)
 *  AT$HTS  AT$DIE AT$HIT AT$HPL  (three 9-bit fields, 0..511)
 *  AT$EXP  total experience points
 * ---------------------------------------------------------------- */
enum { AT_PRM = 0, AT_PHY = 1, AT_LNG = 2, AT_HTS = 3, AT_EXP = 4, AT_NWORDS = 5 };

#define AT_STR 037,  4   /* AT$STR==37B4  STRENGTH                    */
#define AT_INT 037,  9   /* AT$INT==37B9  INTELLEGENCE  [sic, as in the source] */
#define AT_WIS 037, 14   /* AT$WIS==37B14 WISDOM                      */
#define AT_CHA 037, 19   /* AT$CHA==37B19 CHARISMA                    */
#define AT_FLT 037, 24   /* AT$FLT==37B24 FLOATING ABILITY (BOUYANCY) */

#define AT_CON 037,  4   /* AT$CON==37B4  CONSTITUTION */
#define AT_SIZ 037,  9   /* AT$SIZ==37B9  SIZE         */
#define AT_AGI 037, 14   /* AT$AGI==37B14 AGILITY      */
#define AT_DEX 037, 19   /* AT$DEX==37B19 DEXTERITY    */

#define AT_DIE 0777, 17  /* AT$DIE==777B17 NUMBER OF HIT DIE */
#define AT_HIT 0777, 26  /* AT$HIT==777B26 HIT POINTS        */
#define AT_HPL 0777, 35  /* AT$HPL==777B35 HIT POINTS LEFT   */

/* Contents descriptor block */
#define CON_MX 0777777, 17  /* CON$MX (LH) MAXIMUM CAPACITY, 0 IF NONE */
#define CON_VL 0777777, 35  /* CON$VL (RH) $PRICE OR $VALUE            */
#define CON_AT 0777777, 17  /* CON$AT (LH) OBJECT ATTRIBUTES           */
#define CON_WT 0777777, 35  /* CON$WT (RH) WEIGHT                      */

/* ---------------------------------------------------------------- *
 *  PLAYER TYPES.  AMAZON.OFF marks this page "OUTMODED -- HERE FOR
 *  HISTORICAL NOW", so it is carried over verbatim, gap at bit 9 and
 *  all, and used for the creature taxonomy.
 * ---------------------------------------------------------------- */
#define P_MAGI (1ULL<<0)  /* MAGIC USER   */
#define P_CLER (1ULL<<1)  /* CLERIC       */
#define P_FIGH (1ULL<<2)  /* FIGHTING MAN */
#define P_THIE (1ULL<<3)  /* THIEF        */
#define P_DWAR (1ULL<<4)  /* DWARF        */
#define P_ELF  (1ULL<<5)  /* ELF          */
#define P_HOBB (1ULL<<6)  /* HOBBIT       */
#define P_PALA (1ULL<<7)  /* PALADIN      */
#define P_PLAN (1ULL<<8)  /* PLANT        */
#define P_GNOM (1ULL<<10) /* GNOME        */
#define P_KOBO (1ULL<<11) /* KOBOLDS      */
#define P_GOBL (1ULL<<12) /* GOBLIN       */
#define P_ORC  (1ULL<<13) /* ORC          */
#define P_HOBG (1ULL<<14) /* HOBGOBLIN    */
#define P_GNOL (1ULL<<15) /* GNOLL        */
#define P_CYCL (1ULL<<16) /* CYCLOPS      */
#define P_PHAN (1ULL<<17) /* PHANTOM      */
#define P_DRAG (1ULL<<18) /* DRAGON       */
#define P_DINO (1ULL<<19) /* DINOSAUR     */
#define P_TROL (1ULL<<20) /* TROLL        */
#define P_FAIR (1ULL<<21) /* FAIRY        */
#define P_GRUE (1ULL<<22) /* GRUE         */
#define P_WITC (1ULL<<23) /* WITCH        */
#define P_SPIR (1ULL<<24) /* SPIRIT       */
#define P_HUMA (1ULL<<25) /* HUMAN        */
#define P_PRIN (1ULL<<26) /* PRINCE       */
#define P_PRSS (1ULL<<27) /* PRINCESS     */
#define P_GOD  (1ULL<<28) /* GOD          */
#define P_GODD (1ULL<<29) /* GODDESS      */
#define P_ANIM (1ULL<<30) /* ANIMAL       */
#define P_FOWL (1ULL<<31) /* FLYING BEAST */
#define P_SERP (1ULL<<32) /* SERPENT      */
#define P_FISH (1ULL<<33) /* FISH         */
#define P_MALE (1ULL<<34) /* MALE, BOY, MAN      */
#define P_GIRL (1ULL<<35) /* GIRL, WOMAN, FEMALE */

/* ---------------------------------------------------------------- *
 *  DIRECTION CODES                                    (CMDLIB.SAI)
 *  The "strength" numbers in CMDLIB's DIRECTION class are the
 *  direction ordinals: N=0 NE=1 E=2 SE=3 S=4 SW=5 W=6 NW=7 U=8 D=9.
 * ---------------------------------------------------------------- */
enum {
    DIR_N = 0, DIR_NE = 1, DIR_E = 2, DIR_SE = 3,
    DIR_S = 4, DIR_SW = 5, DIR_W = 6, DIR_NW = 7,
    DIR_U = 8, DIR_D = 9, NDIRS = 10
};
extern const char *dirname_full[NDIRS];
extern const char *dirname_abbr[NDIRS];

/* ---------------------------------------------------------------- *
 *  ROOMS                                        (ROOMS, AMAZON.TXT)
 * ---------------------------------------------------------------- */
enum {
    R_NONE = 0,
    R_RIDDLE = 1, R_AMAZONRM = 2, R_SHOP = 3, R_CLIFF = 4, R_JAIL = 5,
    R_COURT = 6, R_EXEC = 7, R_SHIPYARD = 8, R_WENDY = 9, R_BAR = 10,
    R_BROOM = 11, R_ELM = 12, R_SKULLENT = 13, R_SWAMP = 14, R_MARSH = 15,
    R_SKULL = 16, R_CASTLE = 17, R_HONEY = 18, R_BANK = 19, R_SHADOW = 20,
    R_CELL = 21,
    NROOMS = 22                          /* 1..21 used; ROOMS ends at "22 []" */
};

#define RF_LIT     0001u   /* daylight -- where the Shadow's curse lifts */
#define RF_SURFACE 0002u   /* on the surface, not down in the caves      */
#define RF_DEATH   0004u   /* Execution Chamber                          */

typedef struct {
    const char *name;                    /* verbatim from ROOMS col. 2 */
    const char *desc;                    /* verbatim from ROOMS col. 3 */
    unsigned    flags;
    short       exits[NDIRS];            /* room number, or R_NONE */
} Room;
extern Room rooms[NROOMS];

/* ---------------------------------------------------------------- *
 *  OBJECTS AND CREATURES                              (OBJECT.TYP)
 * ---------------------------------------------------------------- */
enum {
    O_NONE = 0,
    /* the equipment bag and the contents OBJECT.TYP lists for it */
    O_BAG, O_KNIFE, O_SKILLET, O_MATCHES, O_FOOD, O_BATTERIES, O_FLASHLIGHT,
    O_LAMP, O_JUG, O_PAN, O_RADIO, O_GUN, O_BULLETS,
    /* treasures, reward items and props */
    O_MIRROR, O_CROWN, O_TIERA, O_WAND, O_SHADOW, O_PANDORA, O_WENDYBOX,
    O_SILVERBULLET, O_INVITATION, O_DIPLOMA, O_LAWBOOK, O_ROBE, O_PADDING,
    O_MONKEY, O_PONY, O_STALLION, O_BABY, O_MICE, O_RABBITM, O_RABBITF,
    O_CARROTS, O_TRUCK, O_APPLERED, O_APPLEGREEN, O_HONEY, O_CAKE, O_URN,
    O_PIPE, O_POSTER, O_AIRPLANE, O_PARACHUTE, O_RAFT, O_BROOM, O_HAT,
    O_WEB, O_KEYSA, O_KEYSB, O_KEYSC, O_KEYSFA33, O_KEYSBLDG, O_BOOK,
    O_SIGN, O_DESK, O_DRAWERS, O_MODEL, O_CATAPULT, O_FAIRYRING, O_GOODMARK,
    O_BOW, O_RATPOISON, O_CLOCK, O_GAVEL,
    /* creatures */
    C_FROG, C_BULLFROG, C_FROGGREY, C_BUTTERFLY, C_DRAGONFLY, C_ELF,
    C_DWARF, C_FAIRY, C_GRUE, C_TROLL, C_WITCH, C_WIZARD, C_BIRD, C_BEES,
    C_CATWHITE, C_CATBLACK, C_CATPLAIN, C_DOG, C_WOLF, C_PHANTOM,
    C_LONERANGER, C_TONTO, C_TARZAN, C_JANE, C_CROC, C_DRAGONGREEN,
    C_DRAGONYELLOW, C_SPIDER, C_MONKEYW, C_HOOK, C_PETERPAN, C_WENDY,
    C_LAWYER, C_JUDGE, C_POLICEMAN, C_OLDLADY, C_VAMPIRE, C_RAT,
    C_WHITERABBIT, C_FAIRYPRINCESS, C_ELEANOR, C_SHADOWEVIL, C_OLDMAN,
    C_FIGURE, C_JOHNNY, C_BUGS, C_LIVINGSTON, C_KINGFAIRY,
    NOBJ
};

#define OF_TAKEABLE  000001u
#define OF_TREASURE  000002u
#define OF_CREATURE  000004u
#define OF_CONTAINER 000010u
#define OF_LIGHT     000020u
#define OF_FIXED     000040u
#define OF_HIDDEN    000200u   /* not listed until revealed */
#define OF_SHOPITEM  000400u   /* stocked in the Equipment Shop */
#define OF_VEHICLE   001000u   /* driven or ridden, not carried */

/* live per-item state bits */
#define IS_OPEN      000001u
#define IS_LOCKED    000002u
#define IS_LIT       000004u
#define IS_DEAD      000010u
#define IS_REVEALED  000020u
#define IS_DELIVERED 000040u   /* returned to its owner for the reward */
#define IS_PAID      000400u   /* bought from the Equipment Shop        */
#define IS_TIED      000100u

typedef struct {
    const char *shortname;               /* NAM$SM */
    const char *longname;                /* NAM$LG */
    const char *words[6];                /* nouns the parser accepts    */
    const char *note;                    /* verbatim OBJECT.TYP comment */
    const char *here;                    /* line printed when in a room */
    unsigned    flags;
    short       start;                   /* starting room, or 0         */
    short       inside;                  /* containing object, or 0     */
    int         value;                   /* CON$VL price / value        */
    int         weight;                  /* CON$WT                      */
    w36         ptype;                   /* P$xxxx bits                 */
    short       owner;                   /* creature to return it to    */
    int         reward;                  /* points for returning it     */
} ObjDef;
extern ObjDef objdef[NOBJ];

/* ---------------------------------------------------------------- *
 *  ITEM  -- the live control block, laid out as AMAZON.OFF describes
 * ---------------------------------------------------------------- */
typedef struct {
    w36   w[4];                          /* IT$TYP IT$CTL IT$IDP IT$LOC */
    w36   at[AT_NWORDS];                 /* attributes block            */
    w36   cd[4];                         /* contents descriptor         */
    short loc;                           /* room number, or 0           */
    short in;                            /* containing item, or 0       */
    short carrier;                       /* player index carrying it, -1*/
    unsigned state;
} Item;

/* ---------------------------------------------------------------- *
 *  PLAYERS.  PRTEST.SAI sprouts one SAIL process per TTY and sizes
 *  every table [1:16], so sixteen is the house limit.
 * ---------------------------------------------------------------- */
#define MAXPLAYERS 16
#define NAMELEN 24
#define CODELEN 8

#define PS_FREE   0
#define PS_ACTIVE 1
#define PS_DEAD   2

#define PF_PRISONER 000001u
#define PF_LAWYER   000002u   /* passed the Bar Exam                  */
#define PF_SHADOWED 000004u   /* caught by the Shadow once            */
#define PF_CURSED   000010u   /* caught again -- you will surely die  */
#define PF_GOODMARK 000020u   /* the Phantom's GOOD-MARK              */
#define PF_BANNED   000040u   /* executed: barred until Tomorrow      */
#define PF_ARRESTED 000100u
#define PF_ONTRIAL  000200u

typedef struct {
    int   state;
    char  name[NAMELEN];
    char  code[CODELEN];                 /* CODE 'PHREAD' in AMAZON.TXT */
    w36   ppn;                           /* PPN  (#100000200)           */
    w36   at[AT_NWORDS];
    w36   ptype;
    short loc;
    short prevloc;
    unsigned flags;
    int   turns;                         /* TURNS  22792 */
    int   ptime;                         /* TIME   19582 */
    int   deaths;                        /* DEATHS 2     */
    int   score;
    int   riddle;                        /* RIDDLE index, -1 if none */
    int   ridans;                        /* RIDANS flag              */
    int   barq;                          /* current Bar Exam question */
    int   barright;                      /* questions answered right  */
    int   barasked;
    int   nightfall;                     /* turns left after the Shadow */
    int   banned_until;
    int   figureturns;                   /* "you better put him down"   */
    int   darkturns;                     /* turns spent in the dark     */
    long  lastmsg;                       /* last message ring id seen   */
    unsigned long seen;                  /* rooms visited bitmap */
} Player;

/* ---------------------------------------------------------------- *
 *  SHARED WORLD -- the whole valley in one file, so several copies of
 *  AMAZON.EXE can play in it at once, the way the TTYs did.
 * ---------------------------------------------------------------- */
#define WORLD_MAGIC 0x414D5A4EU          /* "AMZN" */
#define WORLD_VERS  5
#define MSGRING     40                   /* PRTEST.SAI: user!message[0:40] */
#define MSGLEN      140

typedef struct {
    unsigned magic, vers;
    long     clock;
    unsigned seed;
    Player   pl[MAXPLAYERS];
    Item     it[NOBJ];
    long     msgseq;
    int      msghead;
    long     msgid[MSGRING];
    int      msgto[MSGRING];             /* -1 = broadcast */
    char     msgtext[MSGRING][MSGLEN];
    char     examiners[MAXPLAYERS][NAMELEN];
    int      nexaminers;
} World;

/* ---------------------------------------------------------------- */
/* parse.c -- GETCMD over SETBREAK(1,"A-Z0-9*-","","KXNS")           */
typedef struct {
    char line[512];
    char tok[24][32];
    int  ntok, cur;
} CmdLine;
void cmd_reset(CmdLine *c, const char *line);
int  getcmd(CmdLine *c, char *out, int *more);

/* CMDLIB.SAI command classes */
enum { VC_NONE = 0, VC_ACQUIRE, VC_RELINQUISH, VC_DIRECTION, VC_OTHER };
int  lookup_command(const char *w, int *cls, int *strength);

/* verbs beyond CMDLIB's three classes */
enum {
    V_NONE = 0, V_LOOK, V_INVENTORY, V_GO, V_READ, V_ANSWER, V_OPEN, V_CLOSE,
    V_EXAMINE, V_ATTACK, V_KISS, V_EAT, V_DRINK, V_RIDE, V_FLY, V_SLEEP,
    V_WEAR, V_UNLOCK, V_LOCK, V_GIVE, V_SAY, V_HELP, V_SCORE, V_WHO, V_TELL,
    V_QUIT, V_BUY, V_ORDER, V_STATS, V_WAIT, V_SOURCE, V_LIST, V_PUT,
    V_THROWAT, V_KILL, V_LAUNCH, V_SWEEP, V_DIG, V_CLIMB, V_SWIM, V_HINT,
    V_DELIVER, V_ARREST, V_PLEAD, V_STUDY, V_TIE, V_POUR, V_LIGHT, V_TURNOFF
};

/* world.c */
void        world_reset(World *w);
int         obj_by_word(const char *word);
const char *obj_name(int o);
int         room_by_word(const char *word);

/* riddles.c -- see the banner in that file: reconstructed content */
extern const char *riddle_text[36];
extern const char *riddle_answer[36];
extern const char *bar_question[36];
extern const char *bar_answer[36];

/* share.c */
int    world_open(const char *path, int solo);
World *world_lock(void);
void   world_unlock(void);
void   world_close(void);
const char *world_path(void);

/* game.c */
int  game_join(World *w, const char *name, const char *code);
void game_loop(int me);

#endif /* AMAZON_H */
