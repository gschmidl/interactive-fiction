/*  world.c -- the Amazon River Valley.
 *
 *  Room names and descriptions are transcribed verbatim from ROOMS
 *  (22-Sep-77) and, for the Riddle Room, from the RIDLRM item in
 *  AMAZON.TXT (09-Sep-79).  Original spelling is preserved, including
 *  "senetnced", "There ais something", "in a tiny Closet with a which",
 *  "Diamond Tiera", "Phanthom", "Crockadile" and "WIld Honeysuckle field".
 *
 *  Object notes are the verbatim OBJECT.TYP (29-Jul-79) comment for that
 *  object; the game prints them for EXAMINE.  Room-to-room connections are
 *  NOT in any surviving file -- see docs/RECONSTRUCTION.md for the
 *  justification of each one.
 */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "amazon.h"

/* CMDLIB.SAI DIRECTION class, in its own numeric order. */
const char *dirname_full[NDIRS] = {
    "NORTH", "NORTHEAST", "EAST", "SOUTHEAST", "SOUTH",
    "SOUTHWEST", "WEST", "NORTHWEST", "UP", "DOWN"
};
const char *dirname_abbr[NDIRS] = {
    "N", "NE", "E", "SE", "S", "SW", "W", "NW", "U", "D"
};

#define X R_NONE
/*                      N   NE  E   SE  S   SW  W   NW  U   D  */
Room rooms[NROOMS] = {
{0,0,0,{X,X,X,X,X,X,X,X,X,X}},

/* 1 */
{"Riddle Room",
 "You are in a place with unusual markings on the walls,\n"
 "possibly writings from an ancient language, apparently unknown\n"
 "to you.  Some of the writing may be legible after closer examination\n"
 "but I would not count on it.  You cannot see any viable exits from\n"
 "this chamber.",
 0,                    {X,  X,  X,  X,  X,  X,  X,  X,  X,  X}},

/* 2 */
{"Amazon Room",
 "There is a scale model of a small cave-like room here.",
 0,                    {X,  X,  X,  X,  X,  X,  20, X,  X,  X}},

/* 3 */
{"Equipment Shop",
 "There are all kinds of camping tools & things here in the\n"
 "shop, each item is worth some number of points.",
 RF_LIT|RF_SURFACE,    {10, X,  X,  X,  X,  X,  18, X,  X,  X}},

/* 4 */
{"Top of Cliff",
 "There is a large catapult-like contraption for launching\n"
 "some type of aircraft.  It seems to be loaded.",
 RF_LIT|RF_SURFACE,    {X,  X,  X,  X,  12, X,  X,  X,  X,  19}},

/* 5 */
{"Jail",
 "You may enter the Jail as a spectator or a prisoner.",
 RF_LIT,                    {6,  X,  X,  X,  X,  X,  10, X,  X,  X}},

/* 6 */
{"Court Room",
 "A large chamber with a Bench, Jury box, a Desk and a Stand.",
 RF_LIT,                    {X,  X,  X,  X,  5,  X,  X,  X,  X,  7}},

/* 7 */
{"Execution Chamber",
 "You have been found GUILTY by the court & were senetnced to die\n"
 "in the execution chamber.   Neither you nor your cohorts are allowed\n"
 "back into the amazon valley until Tomorrow... You have just been Executed.",
 RF_DEATH,             {X,  X,  X,  X,  X,  X,  X,  X,  X,  X}},

/* 8 */
{"The Shipyard",
 "Home of Captain Hook & his gang... \"beware the hook\".",
 RF_LIT|RF_SURFACE,    {9,  X,  X,  X,  19, X,  X,  X,  X,  X}},

/* 9 */
{"Wendy's Room",
 "You are in a Warm & Cozy little room with a large fluffy bed against\n"
 "one wall.  The bedspread is embroidered in large script letters the\n"
 "word \"WENDY\".  Against another wall there is a desk, and a chest of\n"
 "drawers.",
 RF_LIT,                    {X,  X,  X,  X,  8,  X,  X,  X,  X,  X}},

/* 10 */
{"Bar Room",
 "You are in a small dimly lit tavern.  There is a sign on one wall\n"
 "which reads \"Examiners & Collectors\" with a short list of names.",
 RF_LIT,                    {21, X,  5,  X,  3,  X,  X,  X,  11, X}},

/* 11 */
{"Broom Closet",
 "You are in a tiny Closet with a which seems to have been swept clean recently.",
 RF_LIT,                    {X,  X,  X,  X,  X,  X,  17, X,  X,  10}},

/* 12 */
{"Elm Grove",
 "You are in a large open field surrounded by Elm trees, there is a large wooden\n"
 "sign in the West corner of the field.",
 RF_LIT|RF_SURFACE,    {15, X,  18, X,  19, X,  13, X,  X,  X}},

/* 13 */
{"Entrance to Skull Cave",
 "You are at the entrance to a large cave which looks very much like\n"
 "the head of a human being.  The entrance leads through the Mouth of the Skull.",
 RF_LIT|RF_SURFACE,    {16, X,  12, X,  X,  X,  X,  X,  X,  X}},

/* 14 */
{"Swamp",
 "You are in the swamp, surrounded by trees on all sides.",
 RF_LIT|RF_SURFACE,    {19, X,  18, X,  15, X,  X,  X,  X,  X}},

/* 15 */
{"Marsh",
 "You are in the Marshes, surrounded by more marshland.",
 RF_LIT|RF_SURFACE,    {14, X,  17, X,  12, X,  X,  X,  X,  X}},

/* 16 */
{"Skull cave",
 "You are in the Skull Cave, there are Skulls hanging all over the walls.",
 0,                    {X,  X,  X,  X,  13, X,  X,  X,  X,  20}},

/* 17 */
{"Witch's Castle",
 "You are in a large Castle with many towers and corridors leading in all directions.",
 RF_LIT,                    {21, X,  11, X,  X,  X,  15, X,  X,  10}},

/* 18 */
{"WIld Honeysuckle field",
 "You are passing through a large field of Yellow and White Honeysuckle.",
 RF_LIT|RF_SURFACE,    {12, X,  3,  X,  14, X,  19, X,  X,  X}},

/* 19 */
{"Amazon River Bank",
 "You are standing on the bank of the Amazon River.  Below you is a raging torrent\n"
 "with a few boulders lying here and there.  To the west is a large wooden sign with\n"
 "writing on it.",
 RF_LIT|RF_SURFACE,    {12, X,  18, X,  14, X,  X,  X,  4,  X}},

/* 20 */
{"Shadow Room",
 "You have just entered the \"SHADOW ROOM\" guarded by the shadow of evil.  If you\n"
 "are caught, you must make it to the surface before nightfall, else you will die.\n"
 "If you are caught more than once by the Shadow, you will surely die!",
 0,                    {X,  X,  2,  X,  X,  X,  1,  X,  16, X}},

/* 21 */
{"Padded Cell",
 "You seem to have wandered through the halls of an Asylum, You are now in\n"
 "a small cell-like room which has padding on the floor, walls and ceiling.",
 RF_LIT,                    {X,  X,  X,  X,  10, X,  17, X,  X,  X}},
};
#undef X

/* ================================================================ *
 *  OBJECTS.  "note" is the verbatim OBJECT.TYP entry.
 * ================================================================ */
#define TK OF_TAKEABLE
#define TR (OF_TAKEABLE|OF_TREASURE)
#define CR OF_CREATURE
#define CN (OF_TAKEABLE|OF_CONTAINER)
#define FX OF_FIXED
#define HD OF_HIDDEN
#define SH (OF_TAKEABLE|OF_SHOPITEM)
#define LT OF_LIGHT

ObjDef objdef[NOBJ] = {
{0},

/* ---- the Equipment Bag and the contents OBJECT.TYP lists ------- */
{"equipment bag","an Equipment Bag",{"BAG","EQUIPMENT",0},
 "What every adventurer needs...",
 "There is an Equipment Bag here.", CN|OF_SHOPITEM, R_SHOP,0, 25,10,0,0,0},
{"brass knife","a brass knife",{"KNIFE","BRASS",0},
 "Equipment Bag: Brass knife","", SH, R_SHOP,O_BAG,  8, 2,0,0,0},
{"tin skillet","a tin skillet",{"SKILLET","TIN","PAN",0},
 "Equipment Bag: tin skillet","", SH, R_SHOP,O_BAG,  5, 3,0,0,0},
{"matches","a box of matches",{"MATCHES","MATCH",0},
 "Equipment Bag: matches","", SH, R_SHOP,O_BAG,  3, 1,0,0,0},
{"food","some food",{"FOOD","RATIONS",0},
 "Equipment Bag: food","", SH, R_SHOP,O_BAG,  6, 3,0,0,0},
{"batteries","a set of batteries",{"BATTERIES","BATTERY",0},
 "Equipment Bag: batteries","", SH, R_SHOP,O_BAG,  9, 2,0,0,0},
{"flashlight","a flashlight",{"FLASHLIGHT","TORCH","LIGHT",0},
 "Equipment Bag: Flashlight","", SH|LT, R_SHOP,O_BAG, 15, 2,0,0,0},
{"oil lamp","an oil lamp",{"LAMP","OIL",0},
 "Equipment Bag: oil lamp","", SH|LT, R_SHOP,O_BAG, 12, 4,0,0,0},
{"jug of water","a jug of water",{"JUG","WATER",0},
 "Equipment Bag: jug of water","", SH, R_SHOP,O_BAG,  7, 6,0,0,0},
{"pan of oil","a pan of oil",{"PANOFOIL","OILPAN",0},
 "Equipment Bag: pan of oil","", SH, R_SHOP,O_BAG,  7, 5,0,0,0},
{"radio communicator","a radio communicator",{"RADIO","COMMUNICATOR",0},
 "Equipment Bag: A radio communicator","", SH, R_SHOP,O_BAG, 30, 3,0,0,0},
{"gun","a gun",{"GUN","PISTOL",0},
 "Equipment Bag: a gun","", SH, R_SHOP,O_BAG, 25, 4,0,0,0},
{"bullets","2 bullets (ordinary)",{"BULLETS","BULLET",0},
 "Equipment Bag: 2 bullets (ordinary)","", SH, R_SHOP,O_BAG, 10, 1,0,0,0},

/* ---- treasures, reward items and props ------------------------- */
{"hand mirror","a jewelled Hand Mirror",{"MIRROR","HAND",0},
 "Studded with jewels, belongs to Eleanor of south fairyland\n"
 "It has magical powers, REWARD!",
 "There is a Hand Mirror studded with jewels here.",
 TR, R_CASTLE,0, 90,3,0,C_ELEANOR,150},
{"fairy crown","the Fairy Crown",{"CROWN","FAIRY",0},
 "Owned by Prince of fairies, REWARD from King of fairies\n"
 "Return to Fairyland, protection from fairy rings.",
 "There is a delicate Fairy Crown lying here.",
 TR, R_CASTLE,0, 80,2,0,C_KINGFAIRY,150},
{"diamond tiera","the Diamond Tiera",{"TIERA","TIARA","DIAMOND",0},
 "Owned by Eleanor, Queen of south fairyland, REWARD!",
 "A Diamond Tiera glitters here.",
 TR, R_CASTLE,0,100,2,0,C_ELEANOR,150},
{"magic wand","a Magic Wand",{"WAND","MAGIC",0},
 "Belongs to Fairy Princess, needs it to stay alive after\n"
 "drinking poison meant for Peter Pan (Tinkerbell)",
 "There is a Magic Wand here.",
 TR, R_WENDY,O_DRAWERS, 75,1,0,C_FAIRYPRINCESS,150},
{"shadow","a Shadow",{"SHADOW",0},
 "Belongs to Peter Pan, hidden in wendy's room\n"
 "Collect from Peter Pan or Captain Hook",
 "A Shadow lies crumpled on the floor.",
 TR|HD, R_WENDY,O_PANDORA, 60,0,0,C_PETERPAN,200},
{"Pandora's box","Pandora's Box",{"PANDORA","PANDORAS",0},
 "Hidden in wendy's room, contains Peter Pan's shadow",
 "Pandora's Box is here.",
 CN|HD, R_WENDY,O_DESK, 40,4,0,0,0},
{"Wendy's box","Wendy's box",{"WENDYS","WENDYSBOX",0},
 "Hidden in wendy's room, contains \"the shadow\"",
 "Wendy's box is here.",
 CN|HD, R_WENDY,O_DRAWERS, 40,4,0,0,0},
{"silver bullet","a silver bullet",{"SILVER","SILVERBULLET",0},
 "Lone Ranger - Drops silver bullets",
 "A silver bullet lies here.",
 TK, R_MARSH,0, 35,1,0,0,0},
{"invitation","an invitation to the card party",{"INVITATION","CARD",0},
 "Rabbit,White - Gives CARD parties, an invitation is worth a lot",
 "There is an engraved invitation here.",
 TR, R_HONEY,0, 70,1,0,0,0},
{"diploma","the Bar Exam Diploma",{"DIPLOMA","EXAM",0},
 "Qualifies person as lawyer, must answer questions to recieve",
 "", TK|HD, 0,0, 50,1,0,0,0},
{"law book","the Law Book",{"LAWBOOK","LAW",0},
 "Belongs to Lawyer & Police, necessary to get out of Jail",
 "The Law Book is lying open on the bar.",
 TK, R_BAR,0, 20,5,0,0,0},
{"black robe","a long dark Black Robe",{"ROBE","BLACK",0},
 "Belongs to Judge, found in courtroom",
 "A long dark robe hangs on a peg.",
 TK, R_COURT,0, 30,4,0,C_JUDGE,40},
{"padding","a bale of Padding",{"PADDING",0},
 "For PADDED CELL, Reward offered, return to PADDED CELL",
 "There is a bale of Padding here.",
 TK, R_SHOP,0, 15,8,0,0,60},
{"wingless monkey","a wingless Monkey",{"MONKEY","WINGLESS",0},
 "Monkey, Wingless- Belongs to Tarzan, reward offered",
 "A frightened wingless Monkey is caged here.",
 TK|CR, R_CASTLE,0, 45,9,P_ANIM,C_TARZAN,100},
{"white pony","a White Pony",{"PONY","WHITEPONY",0},
 "White Pony - Belongs to Tonto, reward offered, collect from lone ranger",
 "A White Pony is tethered here.",
 TK|CR|OF_VEHICLE, R_MARSH,0, 55,40,P_ANIM,C_TONTO,100},
{"white stallion","a huge White Stallion",{"STALLION","HORSE",0},
 "White Stallion - Belongs to Phanthom, sometimes others are allowed to ride",
 "....*There is a huge White Stallion tied up here.",
 TK|CR|OF_VEHICLE, R_SKULL,0, 65,60,P_ANIM,0,0},
{"newborn baby","a Newborn Baby",{"BABY","MOSES","BASKET",0},
 "Baby, Newborn - Found floating in basket, \"MOSES\", belongs to Jane",
 "A basket is drifting against the bank.  There is a baby in it.",
 TK, R_BANK,0, 0,12,P_HUMA,C_JANE,120},
{"white mice","some White Mice",{"MICE","MOUSE",0},
 "White Mice - Belong in laboratory of Dr. Livingston",
 "Some White Mice scurry about here.",
 TK|CR, R_CELL,0, 20,1,P_ANIM,C_LIVINGSTON,80},
{"male rabbit","a male Rabbit",{"MALERABBIT","BUCK",0},
 "Rabbit, Male - Belongs in Rabbit Farm, return to BUGS-BUNNY",
 "A male Rabbit is nibbling here.",
 TK|CR, R_HONEY,0, 25,6,P_ANIM,C_BUGS,60},
{"female rabbit","a female Rabbit",{"FEMALERABBIT","DOE",0},
 "Rabbit, Female - Belongs in Rabbit Farm, return to BUGS-BUNNY",
 "A female Rabbit is nibbling here.",
 TK|CR, R_SWAMP,0, 25,6,P_ANIM,C_BUGS,60},
{"carrots","a crate of Carrots",{"CARROTS","CARROT",0},
 "Lost in transit to rabbit farm, must find rabbits first,\n"
 "Requires a truck to deliver them.",
 "A crate of Carrots has fallen off a truck here.",
 TK, R_BANK,0, 30,50,0,C_BUGS,90},
{"delivery truck","a Delivery Truck",{"TRUCK","DELIVERY",0},
 "Truck, Delivery - Belongs to rental agency, stolen by bank robbers, reward!\n"
 "If caught with truck, may be arrested & jailed.",
 "A Delivery Truck is parked here with its doors hanging open.",
 TK|OF_VEHICLE, R_BANK,0, 60,200,0,C_POLICEMAN,110},
{"red apple","a RED Apple",{"REDAPPLE","RED",0},
 "Apple, RED - Belongs to NICE old lady, sleeping poison",
 "A RED Apple sits on top of the apple cart.", TK, R_CASTLE,0, 5,1,0,0,0},
{"green apple","a GREEN Apple",{"GREENAPPLE","GREEN","APPLE",0},
 "Apple, GREEN - Belongs to Orchard, return to Johnny appleseed",
 "A GREEN Apple has fallen here.",
 TK, R_ELM,0, 5,1,0,C_JOHNNY,50},
{"honey","a pot of Honey",{"HONEY",0},
 "Bees - Beehive (Honey)",
 "There is a pot of Honey here.",
 TK, R_ELM,0, 20,4,0,0,0},
{"cake","a cake marked EAT-ME",{"CAKE","EAT-ME","EATME",0},
 "Cake, EAT-ME - Guess!",
 "There is a cake here.  It is marked EAT-ME.",
 TK, R_BAR,0, 10,1,0,0,0},
{"urn","an urn marked DRINK-ME",{"URN","DRINK-ME","DRINKME",0},
 "Urn, DRINK-ME - Guess!",
 "There is an urn here.  It is marked DRINK-ME.",
 TK, R_BAR,0, 10,3,0,0,0},
{"water pipe","a Water Pipe",{"PIPE","HOOKAH",0},
 "Pipe, Water - Belongs to caterpillar, return to Alice",
 "A Water Pipe is bubbling quietly in the corner.",
 TK, R_BAR,0, 25,5,0,0,0},
{"poster","a Horror Chamber poster",{"POSTER",0},
 "Horror Chamber - Poster, belongs on entrance to cave",
 "A rolled-up poster leans against the wall.",
 TK, R_SHOP,0, 15,1,0,0,40},
{"paper airplane","a Paper Airplane",{"AIRPLANE","PLANE","PAPER",0},
 "Airplane,Paper - Necessary to fly across the AMAZON river\n"
 "Nice to take a parachute, life-raft in case you miss",
 "A large Paper Airplane sits in the catapult.",
 TK, R_CLIFF,0, 20,8,0,0,0},
{"parachute","a parachute",{"PARACHUTE","CHUTE",0},
 "Airplane,Paper - Nice to take a parachute",
 "There is a parachute here.",
 TK, R_CLIFF,0, 20,10,0,0,0},
{"life-raft","a life-raft",{"RAFT","LIFERAFT","LIFE-RAFT",0},
 "Airplane,Paper - life-raft in case you miss",
 "There is a life-raft here.",
 TK, R_CLIFF,0, 20,12,0,0,0},
{"broom","an old Broom",{"BROOM",0},
 "Broom - Belongs to witch, bed for black cat",
 "....*There is an old Broom with strange writing on the handle leaning against the wall.",
 TK, R_BROOM,0, 30,6,0,C_WITCH,0},
{"black hat","an old Black Hat",{"HAT","POINTED",0},
 "Witches hat - Belongs to witch, water-proof",
 "....*There is an old Black Hat lying here.",
 TK, R_BROOM,0, 25,2,0,C_WITCH,0},
{"spider web","a sticky Spider Web",{"WEB","SPIDERWEB",0},
 "Spider web - Good to trap witch in, sticky",
 "A sticky Spider Web is stretched across the corner.",
 TK, R_MARSH,0, 10,2,0,0,0},
{"keys SET-A","the keys marked SET-A",{"SET-A","SETA","KEYSA",0},
 "Keys,SET-A - Front office, will do SET-B and SET-C also",
 "A ring of keys marked SET-A hangs behind the counter.",
 TR, R_SHOP,0, 40,1,0,0,0},
{"keys SET-B","the keys marked SET-B",{"SET-B","SETB","KEYSB",0},
 "Keys,SET-B - Front door, listings, teletypes, user offices",
 "A ring of keys marked SET-B lies on the bar.",
 TR, R_BAR,0, 30,1,0,0,0},
{"keys SET-C","the keys marked SET-C",{"SET-C","SETC","KEYSC",0},
 "Keys,SET-C - Back door, User area, Computer room, storage rooms",
 "A ring of keys marked SET-C hangs on a nail.",
 TR, R_BROOM,0, 30,1,0,0,0},
{"keys FA-33","the key marked FA-33",{"FA-33","FA33",0},
 "Keys,FA-33 - Computer Room",
 "A single key marked FA-33 is taped to the wall.",
 TR, R_CELL,0, 45,1,0,0,0},
{"building keys","the Building keys",{"BUILDING","BLDG",0},
 "Keys,Building - Allows access to administration building",
 "The Building keys are hanging from the Bench.",
 TR, R_COURT,0, 35,1,0,0,0},
{"book","a Book",{"BOOK",0},
 "....*There is a book here titled \"Yellav Nozama Eht Sdraziw Dnarg\".",
 "....*There is a book here titled \"Yellav Nozama Eht Sdraziw Dnarg\".",
 TK, R_CELL,0, 55,6,0,0,0},
{"sign","a large wooden sign",{"SIGN",0},
 "", "", FX, 0,0, 0,0,0,0,0},
{"desk","a desk",{"DESK",0},
 "", "", FX|OF_CONTAINER, R_WENDY,0, 0,0,0,0,0},
{"chest of drawers","a chest of drawers",{"DRAWERS","CHEST",0},
 "", "", FX|OF_CONTAINER, R_WENDY,0, 0,0,0,0,0},
{"scale model","a scale model of a small cave-like room",{"MODEL","SCALE",0},
 "There is a scale model of a small cave-like room here",
 "There is a scale model of a small cave-like room here.",
 FX|OF_CONTAINER, R_AMAZONRM,0, 0,0,0,0,0},
{"catapult","a large catapult-like contraption",{"CATAPULT","CONTRAPTION",0},
 "There is a large catapult-like contraption for launching\n"
 "some type of aircraft.  It seems to be loaded.",
 "", FX, R_CLIFF,0, 0,0,0,0,0},
{"fairy ring","a Fairy Ring",{"RING","FAIRYRING",0},
 "Fairy ring - found near fairy land, or fairy princess\n"
 "wizards are powerless within this circle",
 "There is a Fairy Ring of toadstools in the grass here.",
 FX, R_ELM,0, 0,0,0,0,0},
{"GOOD-MARK","the Phantom's GOOD-MARK",{"GOOD-MARK","GOODMARK","MARK",0},
 "Phanthom - Gives GOOD-MARK for helping him",
 "", TK|HD, 0,0, 50,0,0,0,0},
{"yellow bow","a yellow bow",{"BOW","YELLOW",0},
 "Frog,Green spots- Enchanted princess if wearing a yellow bow",
 "A yellow bow has been dropped here.",
 TK, R_WENDY,0, 10,1,0,0,0},
{"rat poison","a tin of rat poison",{"POISON","RATPOISON",0},
 "Blue Rat - Poisonous, can be killed with rat poison",
 "There is a tin of rat poison here.",
 TK, R_SHOP,0, 8,2,0,0,0},
{"alarm clock","a loudly ticking Clock",{"CLOCK","ALARM",0},
 "Captain Hook - One-armed pirate, afraid of clocks and alligators",
 "A Clock is ticking loudly somewhere nearby.",
 TK, R_SWAMP,0, 20,4,0,0,0},
{"gavel","a gavel",{"GAVEL",0},
 "", "", FX, R_COURT,0, 0,0,0,0,0},

/* ---- creatures ------------------------------------------------- */
{"frog","a Frog",{"FROG",0},
 "Frog - If kissed by prince/princess becomes princess/prince\n"
 "Frog - Person > Frog > Person\t* Spell\n"
 "Frog - Has answers to riddles, gives hints",
 "A small Frog is sitting on one of the markings, watching you.",
 CR|TK, R_RIDDLE,0, 15,2,P_ANIM,0,0},
{"bullfrog","a Bullfrog",{"BULLFROG",0},
 "Bullfrog - Enchanted, Don't fall asleep here",
 "A Bullfrog is croaking here.",
 CR, R_SWAMP,0, 0,4,P_ANIM,0,0},
{"grey frog","a Grey Frog",{"GREYFROG","GREY",0},
 "Frog,Grey - Enchanted fellow traveler, must be taken to sauna bath",
 "A Grey Frog looks up at you hopefully.",
 CR|TK, R_SWAMP,0, 20,2,P_ANIM,0,0},
{"purple butterfly","a Purple Butterfly",{"BUTTERFLY","PURPLE",0},
 "Purple butterfly- Becomes Pink Princess, or\n"
 "One-eyed, One-horned, Flying, Purple, People-eater",
 "A Purple Butterfly drifts among the flowers.",
 CR|TK, R_HONEY,0, 30,1,P_FOWL,0,0},
{"dragonfly","a Dragonfly",{"DRAGONFLY",0},
 "Dragonfly - Dragon",
 "A Dragonfly hovers over the water.",
 CR, R_MARSH,0, 0,1,P_FOWL,0,0},
{"elves","the Elves",{"ELVES","ELF",0},
 "Elves - Good, Friendly, Honest\t* Hard worker",
 "Some Elves are working among the trees.",
 CR, R_ELM,0, 0,30,P_ELF,0,0},
{"dwarf","a Dwarf",{"DWARF",0},
 "Dwarf - Good, Friendly, Discreet\t* Lazy\n"
 "Bad, Unfriendly, Liar\t\t* Thief",
 "A Dwarf is loitering here, watching your pockets.",
 CR, R_SKULL,0, 0,40,P_DWAR,0,0},
{"fairy","a Fairy",{"FAIRY2",0},
 "Fairy - Good, Playful, Funny\t* Changes Environment/You * fun\n"
 "Bad, Trikster, Unpleasant\t* Turns you into things",
 "A Fairy is dancing in the ring.",
 CR, R_ELM,0, 0,1,P_FAIR,0,0},
{"grue","a Grue",{"GRUE",0},
 "Grue - Bad, Hungry\t\t* Eats things in the dark\n"
 "* Afraid of light",
 "", CR|HD, R_SHADOW,0, 0,30,P_GRUE,0,0},
{"troll","a burly looking Troll",{"TROLL","BARTENDER",0},
 "Troll - Good, Strong, Silent\t* Stands guard\n"
 "Bad, Strong, Vocal\t\t* Fierce enemy",
 "",
 CR, R_BAR,0, 0,80,P_TROL,0,0},
{"wicked witch","a Wicked Witch",{"WITCH","HAG","WICKED",0},
 "Wicked Witch - Captures Dogs, little boys and girls\n"
 "Has winged monkey, black cat\n"
 "Allergic to water, may be tied up with silken spider webs\n"
 "Witch - Good, Generous, Helpful\t* Gives hints\n"
 "Bad, Playful, Trikster\t\t* Turns you into things",
 "....*There is an old hag wearing a pointed cap and a long black cape nearby.",
 CR, R_CASTLE,0, 0,50,P_WITC,0,0},
{"wizard","the Grand Wizard",{"WIZARD","OLDMAN2",0},
 "Wizard - Good, Makes Policy, Helpful\t* Knows all Sees all\n"
 "Bad, ???",
 "....*There is an old man sitting here wearing a pointed hat.",
 CR, R_CELL,0, 0,40,P_MAGI,0,0},
{"bird","a Bird",{"BIRD",0},
 "Bird - Birdcage, Attack things, Afraid of cats",
 "A Bird is fluttering about here.",
 CR|TK, R_ELM,0, 15,1,P_FOWL,0,0},
{"bees","a swarm of Bees",{"BEES","BEE","BEEHIVE","HIVE",0},
 "Bees - Beehive (Honey)",
 "A Beehive hangs from a branch.  Bees are coming and going.",
 CR, R_ELM,0, 0,5,P_ANIM,0,0},
{"white cat","a White Cat",{"WHITECAT",0},
 "Cat,White - Follows you, Becomes violent if sees food, must be killed",
 "A White Cat is watching you.",
 CR, R_SHOP,0, 0,8,P_ANIM,0,0},
{"black cat","a Black cat",{"BLACKCAT","CAT",0},
 "Cat,Black - Looking for Witch, Broom, Pumpkin, or Ghost",
 "....*There is a Black cat sitting in the corner.",
 CR|TK, R_CASTLE,0, 20,8,P_ANIM,0,0},
{"cheshire cat","a Cat",{"CHESHIRE",0},
 "Cat,Plain - Possibly enchanted, Cheshire",
 "A Cat is here, or most of one.",
 CR, R_BAR,0, 0,8,P_ANIM,0,0},
{"dog","a Dog",{"DOG",0},
 "Dog - Looking for Cat, Friend, Man or Phanthom",
 "A Dog is here, casting about for a scent.",
 CR|TK, R_MARSH,0, 20,25,P_ANIM,C_PHANTOM,80},
{"wolf","a Wolf",{"WOLF",0},
 "Wolf - Growls a lot, Guard, Must have GOOD-MARK to pass",
 "A Wolf stands in the passage, growling.",
 CR, R_SKULL,0, 0,45,P_ANIM,0,0},
{"Phantom","an old man in a purple costume",{"PHANTOM","PHANTHOM",0},
 "Phanthom - Owns DOG and Wolf and White Stallion\n"
 "Gives GOOD-MARK for helping him",
 "....*There is an old man in a purple costume sitting nearby.",
 CR, R_SKULL,0, 0,70,P_HUMA|P_MALE,0,0},
{"Lone Ranger","the Lone Ranger",{"RANGER","LONE",0},
 "Lone Ranger - Drops silver bullets, Gives hints, Looking for white stallion",
 "The Lone Ranger is here, looking for a white stallion.",
 CR, R_MARSH,0, 0,70,P_HUMA|P_MALE,0,0},
{"Tonto","Tonto",{"TONTO",0},
 "Tonto - Court Jester, Friend of Lone Ranger",
 "Tonto, the Court Jester, is here.",
 CR, R_COURT,0, 0,70,P_HUMA|P_MALE,0,0},
{"Tarzan","Tarzan",{"TARZAN",0},
 "Tarzan - Ruler of Jungle animals, If kill animal you better eat it",
 "Tarzan, Ruler of the Jungle animals, is watching from the trees.",
 CR, R_SWAMP,0, 0,90,P_HUMA|P_MALE,0,0},
{"Jane","Jane",{"JANE",0},
 "Jane - Wife of Tarzan, Pregnant, Lives in tree-house",
 "Jane is here, up in the tree-house.",
 CR, R_SWAMP,0, 0,60,P_HUMA|P_GIRL,0,0},
{"crockadile","a Crockadile",{"CROCKADILE","CROCODILE","ALLIGATOR","GATOR",0},
 "Crockadile - Really an Alligator from the everglades, Boastful",
 "A Crockadile is sunning itself here, and boasting about it.",
 CR, R_SWAMP,0, 0,120,P_SERP,0,0},
{"green dragon","a Green Dragon",{"GREENDRAGON",0},
 "Dragon, Green - Breathes fire, lights torches, eats frogs and birds",
 "A Green Dragon lies coiled in the dark.",
 CR, R_SHADOW,0, 0,200,P_DRAG,0,0},
{"yellow dragon","a Yellow Dragon",{"YELLOWDRAGON",0},
 "Dragon, Yellow - Wanders through caverns looking for Jackie-Paper",
 "A Yellow Dragon wanders past, looking for someone.",
 CR, R_SKULL,0, 0,200,P_DRAG,0,0},
{"spider","a Spider",{"SPIDER",0},
 "Spider - Spins a web, Scares miss tuffet",
 "A Spider is spinning here.",
 CR, R_MARSH,0, 0,2,P_ANIM,0,0},
{"winged monkey","a winged Monkey",{"WINGED","WINGEDMONKEY",0},
 "Monkey, Wings - Belongs to witch, catches dogs and little girls",
 "A winged Monkey perches on the battlement.",
 CR, R_CASTLE,0, 0,12,P_ANIM|P_FOWL,0,0},
{"Captain Hook","Captain Hook",{"HOOK","CAPTAIN","PIRATE",0},
 "Captain Hook - One-armed pirate, afraid of clocks and alligators\n"
 "Captures children and makes them slaves",
 "Captain Hook is here.  Beware the hook.",
 CR, R_SHIPYARD,0, 0,90,P_HUMA|P_MALE,0,0},
{"Peter Pan","Peter Pan",{"PETER","PAN","PETERPAN",0},
 "Peter Pan - Rescues people from pirate ship, lives in NEVER-NEVER land",
 "Peter Pan is hovering just out of reach.",
 CR, R_SHIPYARD,0, 0,40,P_HUMA|P_MALE,0,0},
{"Wendy","Wendy",{"WENDY",0},
 "Wendy's Room -- the bedspread is embroidered \"WENDY\"",
 "Wendy is sitting on the bed.",
 CR, R_WENDY,0, 0,50,P_HUMA|P_GIRL,0,0},
{"lawyer","your Lawyer",{"LAWYER",0},
 "Lawyer - Needed to get out of jail",
 "A Lawyer is seated behind the Desk, waiting to be retained.",
 CR, R_COURT,0, 0,70,P_HUMA,0,0},
{"judge","the Judge",{"JUDGE",0},
 "Judge - Lawyer must find one to get you out of jail",
 "",
 CR, R_COURT,0, 0,70,P_HUMA,0,0},
{"policeman","a Policeman",{"POLICEMAN","POLICE","COP",0},
 "Policeman - Can arrest people, request fine etc.",
 "A Policeman is standing by the door.",
 CR, R_JAIL,0, 0,80,P_HUMA,0,0},
{"old lady","an old lady with an apple cart",{"LADY","OLDLADY","CART",0},
 "Apple, RED - Belongs to NICE old lady, sleeping poison",
 "....There is an old lady with an apple cart off to one side.",
 CR, R_CASTLE,0, 0,50,P_HUMA|P_GIRL,0,0},
{"vampire","a Vampire",{"VAMPIRE",0},
 "Vampire - Can be killed by lone ranger, or with silver bullets",
 "A Vampire rises from the shadows.",
 CR, R_SHADOW,0, 0,60,P_SPIR,0,0},
{"blue rat","a Blue Rat",{"RAT","BLUERAT",0},
 "Blue Rat - Poisonous, can be killed with rat poison",
 "A Blue Rat scuttles along the wall.",
 CR, R_CELL,0, 0,3,P_ANIM,0,0},
{"white rabbit","the White Rabbit",{"WHITERABBIT",0},
 "Rabbit,White - \"I'm late, I'm late!\" fame, owns a line of gardens\n"
 "Gives CARD parties, an invitation is worth a lot",
 "The White Rabbit hurries past.  \"I'm late, I'm late!\"",
 CR, R_HONEY,0, 0,8,P_ANIM,0,0},
{"Fairy Princess","the Fairy Princess",{"PRINCESS","FAIRYPRINCESS",0},
 "Fairy Princess -",
 "The Fairy Princess is here, very pale.",
 CR, R_ELM,0, 0,20,P_FAIR|P_PRSS,0,0},
{"Eleanor","Eleanor, Queen of south fairyland",{"ELEANOR","QUEEN",0},
 "Diamond Tiera - Owned by Eleanor, Queen of south fairyland, REWARD!",
 "Eleanor, Queen of south fairyland, holds court here.",
 CR, R_HONEY,0, 0,40,P_FAIR|P_GODD,0,0},
{"shadow of evil","the shadow of evil",{"EVIL","SHADOWOFEVIL",0},
 "Shadow - Knows all, sees all, lurks in dark passages, you must\n"
 "sleep in sunlight if caught to remove the curse.",
 "",
 CR, R_SHADOW,0, 0,0,P_SPIR,0,0},
{"old man","an old man in a purple costume",{"OLDMAN","SKELETON","COSTUME",0},
 "....*There is a skeleton covered with a purple costume lying in the corner.",
 "....*There is a skeleton covered with a purple costume lying in the corner.",
 FX, R_SKULL,0, 0,0,0,0,0},
{"man-like figure","a man-like figure",{"FIGURE","MAN",0},
 "....there is a man-like figure walking around in this room.\n"
 "(if you pick him up, you better put him down)",
 "....there is a man-like figure walking around in this room.",
 CR|TK, R_AMAZONRM,0, 0,1,P_HUMA|P_MALE,0,0},
{"Johnny Appleseed","Johnny Appleseed",{"JOHNNY","APPLESEED",0},
 "Apple, GREEN - Belongs to Orchard, return to Johnny appleseed",
 "Johnny Appleseed is planting here.",
 CR, R_ELM,0, 0,60,P_HUMA|P_MALE,0,0},
{"Bugs Bunny","BUGS-BUNNY",{"BUGS","BUNNY","BUGS-BUNNY",0},
 "Rabbit, Male - Belongs in Rabbit Farm, return to BUGS-BUNNY",
 "BUGS-BUNNY is leaning on the Rabbit Farm gate.",
 CR, R_HONEY,0, 0,10,P_ANIM,0,0},
{"Dr. Livingston","Dr. Livingston",{"LIVINGSTON","DOCTOR","DR",0},
 "White Mice - Belong in laboratory of Dr. Livingston",
 "Dr. Livingston, I presume, is here with his laboratory.",
 CR, R_BANK,0, 0,70,P_HUMA|P_MALE,0,0},
{"King of fairies","the King of fairies",{"KING","FAIRYKING",0},
 "Fairy Crown - Owned by Prince of fairies, REWARD from King of fairies",
 "The King of fairies is holding court in the ring.",
 CR, R_ELM,0, 0,25,P_FAIR|P_GOD,0,0},
};

/* ---------------------------------------------------------------- */
int obj_by_word(const char *word)
{
    int o, k;
    for (o = 1; o < NOBJ; o++)
        for (k = 0; k < 6 && objdef[o].words[k]; k++)
            if (!strcmp(objdef[o].words[k], word)) return o;
    return 0;
}

const char *obj_name(int o)
{
    return (o > 0 && o < NOBJ) ? objdef[o].longname : "nothing";
}

/* Build the starting attribute words for one item, in the AMAZON.OFF
   layout, so the live data really is the documented data. */
static void set_attrs(w36 *at, int str, int inte, int wis, int cha, int flt,
                      int con, int siz, int agi, int dex,
                      int die, int hit, w36 lang)
{
    at[AT_PRM] = at[AT_PHY] = at[AT_LNG] = at[AT_HTS] = at[AT_EXP] = 0;
    W36PUT(at[AT_PRM], AT_STR, str);
    W36PUT(at[AT_PRM], AT_INT, inte);
    W36PUT(at[AT_PRM], AT_WIS, wis);
    W36PUT(at[AT_PRM], AT_CHA, cha);
    W36PUT(at[AT_PRM], AT_FLT, flt);
    W36PUT(at[AT_PHY], AT_CON, con);
    W36PUT(at[AT_PHY], AT_SIZ, siz);
    W36PUT(at[AT_PHY], AT_AGI, agi);
    W36PUT(at[AT_PHY], AT_DEX, dex);
    W36PUT(at[AT_HTS], AT_DIE, die);
    W36PUT(at[AT_HTS], AT_HIT, hit);
    W36PUT(at[AT_HTS], AT_HPL, hit);
    at[AT_LNG] = lang;
}

void world_reset(World *w)
{
    int o, i;
    memset(w, 0, sizeof *w);
    w->magic = WORLD_MAGIC;
    w->vers  = WORLD_VERS;
    w->seed  = 0x414D5A4EU ^ (unsigned)time(NULL);
    if (!w->seed) w->seed = 0x414D5A4EU;

    for (o = 1; o < NOBJ; o++) {
        ObjDef *d = &objdef[o];
        Item   *t = &w->it[o];
        int weight = d->weight, size = d->weight > 31 ? 31 : d->weight;

        W36PUT(t->w[IT_TYP], CT_ADR, o);          /* control logic = itself */
        W36PUT(t->w[IT_TYP], CT_CMP, (d->flags & OF_CREATURE) ? 3 : 0);
        W36PUT(t->w[IT_TYP], CT_SLF, (d->flags & OF_CREATURE) ? 1 : 0);
        W36PUT(t->w[IT_IDP], ID_ADR, o);
        W36PUT(t->w[IT_LOC], ID_ADR, d->start);

        W36PUT(t->cd[0], CON_MX, (d->flags & OF_CONTAINER) ? 100 : 0);
        W36PUT(t->cd[0], CON_VL, d->value);
        W36PUT(t->cd[1], CON_AT, d->flags);
        W36PUT(t->cd[1], CON_WT, weight);

        if (d->flags & OF_CREATURE) {
            int hd = 1 + weight / 20;
            set_attrs(t->at, 6 + weight / 12, 8, 8, 10, 8,
                      8 + weight / 20, size, 12, 12, hd, hd * 6, 1);
        } else {
            set_attrs(t->at, 0,0,0,0,0, 0,size,0,0, 0,0, 0);
        }

        t->loc     = d->start;
        t->in      = d->inside;
        t->carrier = -1;
        t->state   = 0;
        if (d->flags & OF_CONTAINER) t->state |= IS_LOCKED;
    }

    /* The one lock ROOMS actually mentions: "The desk is Locked." */
    w->it[O_DESK].state     |= IS_LOCKED;
    w->it[O_DRAWERS].state  &= ~IS_LOCKED;
    w->it[O_BAG].state      &= ~IS_LOCKED;
    w->it[O_MODEL].state    &= ~IS_LOCKED;
    w->it[O_MODEL].state    |= IS_OPEN;
    /* "The book seems to be fastened shut." */
    w->it[O_BOOK].state     |= IS_LOCKED;
    /* "There is a huge White Stallion tied up here." */
    w->it[O_STALLION].state |= IS_TIED;

    for (i = 0; i < MSGRING; i++) w->msgto[i] = -1;
    for (i = 0; i < MAXPLAYERS; i++) w->pl[i].state = PS_FREE;
}
