/* =====================================================================
 *
 *      ####  R E C O N S T R U C T E D   C O N T E N T  ####
 *
 *  Everything in this file is NEW WRITING.  None of it is from the tapes.
 *
 *  Two surviving lines tell us these tables existed and how big they were:
 *
 *      ROOMS, room 1:  "You must answer a riddle to leave (the way you
 *                       came in) ....there are 36 different riddles."
 *      ROOMS, room 10: "....*The bartender asks you a question.
 *                       (There are 36 different questions)"
 *
 *  and AMAZON.TXT gives the machinery that indexed them:
 *
 *      (READ   (DISPLAY  RIDTXT(RIDDLE)))
 *      (ANSWER (EQUAL (RIDCOD(RIDDLE) INTEXT) ...))
 *
 *  RIDTXT and RIDCOD themselves were never written to any file that
 *  survives -- see docs/RECONSTRUCTION.md.  The 72 items below were
 *  written for this port so that the two rooms are playable.  They are
 *  drawn ONLY from AMAZON's own material: every answer is a room, object
 *  or creature named in ROOMS or OBJECT.TYP.  Nothing is borrowed from
 *  any other game.
 *
 *  If the original tables are ever recovered, replace the two arrays and
 *  nothing else has to change.  data/riddles.txt and data/barexam.txt
 *  carry the same text in plain form for that purpose.
 *
 * ===================================================================== */
#include "amazon.h"

/* ---- Riddle Room (room 1).  36 riddles, one-word answers. --------- */
const char *riddle_text[36] = {
/*  1 */ "The witch rides upon me, the black cat sleeps upon me,\n"
         "and strange writing runs the length of my handle.  What am I?",
/*  2 */ "I belong to a boy who can fly.  I was folded up in a box\n"
         "in a girl's room, and I lie flat wherever the light falls.\n"
         "What am I?",
/*  3 */ "Kiss me and I am royalty.  Ignore me and I know every answer\n"
         "in this room and will not give you one.  What am I?",
/*  4 */ "I am strong.  When I am good I am silent and I stand guard;\n"
         "when I am bad I am loud, and I serve the drinks.  What am I?",
/*  5 */ "The bees make me, the field of flowers steals me, and the\n"
         "tiny arms will pull you apart to have me.  What am I?",
/*  6 */ "I am studded with jewels and I have magical powers, yet all\n"
         "I ever do is show you yourself.  A queen of fairyland wants me\n"
         "back.  What am I?",
/*  7 */ "I am hungry, I am afraid of light, and I eat whatever is left\n"
         "in the dark.  What am I?",
/*  8 */ "I am huge and white and tied up in a cave shaped like a head.\n"
         "A man in a purple costume owns me, and sometimes he lets others\n"
         "ride me.  What am I?",
/*  9 */ "I am spun, I am sticky, and I am the only thing that will hold\n"
         "a witch still.  What am I?",
/* 10 */ "The one-armed pirate cannot bear my voice, because I count out\n"
         "loud what is coming for him.  What am I?",
/* 11 */ "I am a captain, I am a pirate, I have one arm, and the shipyard\n"
         "is my home.  Beware me.  Who am I?",
/* 12 */ "Without me the Fairy Princess dies of the poison that was meant\n"
         "for Peter Pan.  What am I?",
/* 13 */ "From outside I look like the head of a human being, and you go\n"
         "in through my mouth.  What am I?",
/* 14 */ "I am a raging torrent with boulders in me, I am long and wide,\n"
         "I have many caves, and this whole valley is named for me.\n"
         "What am I?",
/* 15 */ "There are five sets of me.  One set opens the front office and\n"
         "will do for the other two as well.  What am I?",
/* 16 */ "A prince of fairies owned me, a king of fairies will pay for me,\n"
         "and while you wear me the fairy rings cannot touch you.\n"
         "What am I?",
/* 17 */ "I am fastened shut in a padded room, and my title is written\n"
         "backwards.  What am I?",
/* 18 */ "One of me follows you and turns violent at the sight of food.\n"
         "One of me is looking for a witch.  One of me is only a grin.\n"
         "What am I?",
/* 19 */ "I growl, I guard, and I will not let you by unless the Phantom\n"
         "has marked you as good.  What am I?",
/* 20 */ "The old lady with the cart will give me to you red, and there is\n"
         "sleeping poison in me.  Johnny will thank you for me green.\n"
         "What am I?",
/* 21 */ "I am folded from paper, I am loaded in a catapult on the cliff,\n"
         "and I am the only way across the water.  What am I?",
/* 22 */ "We hang in a hive, we come and go, and what we make is stolen by\n"
         "a field of flowers.  What are we?",
/* 23 */ "I spin the thing that traps the witch, and I frighten a girl off\n"
         "her tuffet.  What am I?",
/* 24 */ "Green, I breathe fire and light your torches and eat frogs and\n"
         "birds.  Yellow, I wander the caverns looking for Jackie-Paper.\n"
         "What am I?",
/* 25 */ "There are two of us to find, male and female, and Bugs wants us\n"
         "back at the farm before the crate arrives.  What are we?",
/* 26 */ "I fell off a truck on the way to the rabbit farm, and I cannot be\n"
         "delivered until the rabbits are found and the truck is recovered.\n"
         "What am I?",
/* 27 */ "I belong to a rental agency, bank robbers took me, and if the\n"
         "policeman catches you with me you will go to jail.  What am I?",
/* 28 */ "I am in the equipment bag.  I burn oil, and I need the pan that\n"
         "is next to me on the shelf.  What am I?",
/* 29 */ "I come in a box in the equipment bag.  Without me the lamp is\n"
         "only a lamp.  What am I?",
/* 30 */ "The shop sells me for a good many points, and two ordinary\n"
         "bullets come with me.  The silver one you must find yourself.\n"
         "What am I?",
/* 31 */ "I am on the floor, the walls and the ceiling of a small cell.\n"
         "A reward is offered to whoever brings me back there.\n"
         "What am I?",
/* 32 */ "You may enter me as a spectator or as a prisoner, and if you are\n"
         "the second you may not leave except to go to court.  What am I?",
/* 33 */ "I sit behind the bench in a long dark robe with a gavel in my\n"
         "hand, and your lawyer must find me before you go free.\n"
         "Who am I?",
/* 34 */ "Answer the burly bartender enough times and he hands me over,\n"
         "and writes your name on the sign.  What am I?",
/* 35 */ "I am surrounded by more of myself, and the swamp is my neighbour.\n"
         "What am I?",
/* 36 */ "I am a river, I am a valley, I am a room with a model in it, and\n"
         "in the wizard's diary I am the magic word.  What am I?"
};

const char *riddle_answer[36] = {
    "BROOM",     "SHADOW",   "FROG",      "TROLL",   "HONEY",    "MIRROR",
    "GRUE",      "STALLION", "WEB",       "CLOCK",   "HOOK",     "WAND",
    "SKULL",     "RIVER",    "KEYS",      "CROWN",   "BOOK",     "CAT",
    "WOLF",      "APPLE",    "AIRPLANE",  "BEES",    "SPIDER",   "DRAGON",
    "RABBITS",   "CARROTS",  "TRUCK",     "LAMP",    "MATCHES",  "GUN",
    "PADDING",   "JAIL",     "JUDGE",     "DIPLOMA", "MARSH",    "AMAZON"
};

/* ---- The Bar Exam (room 10).  36 questions, one-word answers. -----
 *  OBJECT.TYP: "Bar Exam Diploma - Qualifies person as lawyer, must
 *  answer questions to recieve" and "Bar and Grill - Place to take bar
 *  exam, etc.  Lawyers & policemen".  So the examination is a real one,
 *  on the law and the lore of the valley.                            */
const char *bar_question[36] = {
/*  1 */ "Counsellor.  Who is the only one here who can arrest a person\n"
         "and request a fine?",
/*  2 */ "What must a prisoner have before he can get out of the Jail?",
/*  3 */ "Whom must that lawyer find before the prisoner goes free?",
/*  4 */ "Name the book that belongs to the lawyers and the police and is\n"
         "necessary to get out of Jail.",
/*  5 */ "A prisoner may not leave the Jail except when taken where?",
/*  6 */ "What garment does the figure behind the bench wear?",
/*  7 */ "What is in that figure's hand?",
/*  8 */ "Besides the Bench and the Jury box, name one of the two other\n"
         "furnishings of the Court Room.",
/*  9 */ "Where does a man go who has been found GUILTY by the court?",
/* 10 */ "After an execution, when may the condemned and his cohorts come\n"
         "back into the valley?",
/* 11 */ "If the policeman catches you with the stolen rental vehicle,\n"
         "where do you go?",
/* 12 */ "Who took that vehicle from the rental agency?",
/* 13 */ "What does the sign in this tavern read, before the list of\n"
         "names?  Give the first word.",
/* 14 */ "What document qualifies a person as a lawyer?",
/* 15 */ "How many different questions are in this examination?",
/* 16 */ "Who serves the drinks in this establishment?",
/* 17 */ "What is the court jester's name?",
/* 18 */ "Who is that jester's friend, the one who drops silver bullets?",
/* 19 */ "What is the only thing that will kill a vampire, apart from that\n"
         "same gentleman?",
/* 20 */ "The Phantom gives out a token for helping him.  What is it\n"
         "called?  One word.",
/* 21 */ "Who must have that token before he will let you pass?",
/* 22 */ "What animal is the wicked witch allergic to being splashed with?",
/* 23 */ "What may a witch be tied up with?",
/* 24 */ "Name the ruler of the jungle animals.",
/* 25 */ "If you kill one of his animals, what must you then do with it?",
/* 26 */ "Who lives in the tree-house and is expecting a child?",
/* 27 */ "The baby found floating in a basket has a name.  What is it?",
/* 28 */ "Whose laboratory do the white mice belong in?",
/* 29 */ "Who owns the wingless monkey?",
/* 30 */ "Who owns the winged one?",
/* 31 */ "A boy rescues people from the pirate ship.  Where does he live?\n"
         "Answer with the name of the land.",
/* 32 */ "What two things is Captain Hook afraid of?  Name either one.",
/* 33 */ "Within what circle is a wizard powerless?  Two words, run\n"
         "together, or just the second.",
/* 34 */ "What is written on the cake?",
/* 35 */ "What is written on the urn?",
/* 36 */ "Last question, counsellor.  The wizard's diary gives a magic\n"
         "word.  What is it?"
};

const char *bar_answer[36] = {
/*  1 */ "POLICEMAN",
/*  2 */ "LAWYER",
/*  3 */ "JUDGE",
/*  4 */ "LAWBOOK",
/*  5 */ "COURT",
/*  6 */ "ROBE",
/*  7 */ "GAVEL",
/*  8 */ "DESK",
/*  9 */ "EXECUTION",
/* 10 */ "TOMORROW",
/* 11 */ "JAIL",
/* 12 */ "ROBBERS",
/* 13 */ "EXAMINERS",
/* 14 */ "DIPLOMA",
/* 15 */ "36",
/* 16 */ "TROLL",
/* 17 */ "TONTO",
/* 18 */ "RANGER",
/* 19 */ "SILVER",
/* 20 */ "GOOD-MARK",
/* 21 */ "WOLF",
/* 22 */ "WATER",
/* 23 */ "WEBS",
/* 24 */ "TARZAN",
/* 25 */ "EAT",
/* 26 */ "JANE",
/* 27 */ "MOSES",
/* 28 */ "LIVINGSTON",
/* 29 */ "TARZAN",
/* 30 */ "WITCH",
/* 31 */ "NEVER-NEVER",
/* 32 */ "CLOCKS",
/* 33 */ "RING",
/* 34 */ "EAT-ME",
/* 35 */ "DRINK-ME",
/* 36 */ "AMAZON"
};
