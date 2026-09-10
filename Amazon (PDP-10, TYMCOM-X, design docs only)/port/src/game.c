/*  game.c -- the AMAZON game loop.
 *
 *  The shape of the loop, the prompt and the stock replies are AMAZON.SAI
 *  (18-Dec-77) verbatim:
 *
 *      do begin "MAIN COMMAND LOOP"
 *          if any!more = 0 then outstr(CR&LF&">");
 *          ignoble!command_ -1;
 *          GETCMD(COMMAND,ANY!MORE,1,30);
 *          if equ(command,"GET") then ACQUIRE;
 *          ...
 *          if ignoble!command then OUTSTR(CR&LF&"What?"&CR&LF);
 *      END "MAIN COMMAND LOOP" until equ(command,"done");
 *
 *  and ACQUIRE / RELINQUISH print "<word> WHAT?", "<word> Taken." and
 *  "<word> DROPPED." exactly as that file does.
 *
 *  Room behaviour follows the "...." and "....*" lines in ROOMS, which are
 *  the conditional messages for each room; each one is printed here under
 *  the condition its wording implies.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "amazon.h"

static World  *W;
static int     ME = -1;
static int     quitting = 0;
static CmdLine CL;

#define P (&W->pl[ME])

/* AMAZON.SAI ends every line with CR LF; on Windows "\n" already does. */
#define CRLF "\n"

/* ---------------------------------------------------------------- */
static unsigned long rnd_next(void)
{
    W->seed = W->seed * 1664525u + 1013904223u;
    return W->seed >> 8;
}
static int rnd(int n) { return n > 0 ? (int)(rnd_next() % (unsigned long)n) : 0; }

static int a_get(w36 word, unsigned mask, int rb)
{
    return (int)((word >> (35 - rb)) & mask);
}
#define ATTR(p,W,F) a_get((p)->at[W], F)

static void a_put(w36 *word, unsigned mask, int rb, int v)
{
    *word &= ~(((w36)mask) << (35 - rb));
    *word |= (((w36)v & mask) << (35 - rb));
}

static int hpleft(Player *p)  { return ATTR(p, AT_HTS, AT_HPL); }
static void sethp(Player *p, int v)
{
    if (v < 0) v = 0;
    if (v > 511) v = 511;
    a_put(&p->at[AT_HTS], AT_HPL, v);
}

/* ---------------------------------------------------------------- */
static void say(const char *s) { fputs(s, stdout); }
static void sayl(const char *s) { fputs(s, stdout); fputs(CRLF, stdout); }

static void broadcast(const char *text, int except)
{
    int i = W->msghead;
    W->msgseq++;
    W->msgid[i] = W->msgseq;
    W->msgto[i] = except;               /* "everyone except this player" */
    strncpy(W->msgtext[i], text, MSGLEN - 1);
    W->msgtext[i][MSGLEN - 1] = 0;
    W->msghead = (W->msghead + 1) % MSGRING;
}

static void drain_messages(void)
{
    int i, k;
    long best = P->lastmsg;
    for (k = 0; k < MSGRING; k++) {
        i = (W->msghead + k) % MSGRING;
        if (W->msgid[i] > P->lastmsg && W->msgto[i] != ME && W->msgtext[i][0]) {
            printf("%s" CRLF, W->msgtext[i]);
            if (W->msgid[i] > best) best = W->msgid[i];
        } else if (W->msgid[i] > best) best = W->msgid[i];
    }
    P->lastmsg = best;
}

/* ---------------------------------------------------------------- */
static int carrying(int o)  { return W->it[o].carrier == ME; }

static int obj_here(int o)
{
    Item *t = &W->it[o];
    if ((objdef[o].flags & OF_HIDDEN) && !(t->state & IS_REVEALED)) return 0;
    if (t->carrier == ME) return 1;
    if (t->carrier >= 0) return 0;
    if (t->in) {
        Item *c = &W->it[t->in];
        if (!(c->state & IS_OPEN)) return 0;
        return (c->carrier == ME) || (c->carrier < 0 && c->loc == P->loc);
    }
    return t->loc == P->loc;
}

/*  A vehicle is driven or ridden, so its own weight is not on your back --
 *  and the Delivery Truck carries for you, which is why OBJECT.TYP can say
 *  the carrots "Require a truck to deliver them".                        */
static int carried_weight(void)
{
    int o, sum = 0;
    for (o = 1; o < NOBJ; o++)
        if (W->it[o].carrier == ME && !(objdef[o].flags & OF_VEHICLE))
            sum += objdef[o].weight;
    return sum;
}

/* AMAZON.OFF gives each person a STRENGTH; that is the carrying limit. */
static int carry_limit(void)
{
    int lim = 20 + 6 * ATTR(P, AT_PRM, AT_STR);
    if (carrying(O_TRUCK)) lim += 400;
    return lim;
}

static void put_in_room(int o, int room)
{
    W->it[o].carrier = -1;
    W->it[o].in = 0;
    W->it[o].loc = (short)room;
    a_put(&W->it[o].w[IT_LOC], 0777777, 35, room);
}

static void give_to_me(int o)
{
    W->it[o].carrier = (short)ME;
    W->it[o].in = 0;
    W->it[o].loc = 0;
}

/* ---------------------------------------------------------------- */
static int is_lit(void)
{
    int o;
    if (rooms[P->loc].flags & RF_LIT) return 1;
    for (o = 1; o < NOBJ; o++)
        if ((objdef[o].flags & OF_LIGHT) && (W->it[o].state & IS_LIT)
            && (W->it[o].carrier == ME
                || (W->it[o].carrier < 0 && W->it[o].loc == P->loc)))
            return 1;
    return 0;
}

/* ================================================================ *
 *  Room description
 * ================================================================ */
static void list_players_here(void)
{
    int i, n = 0;
    for (i = 0; i < MAXPLAYERS; i++) {
        if (i == ME || W->pl[i].state != PS_ACTIVE) continue;
        if (W->pl[i].loc != P->loc) continue;
        printf("%s is here.", W->pl[i].name);
        if (W->pl[i].flags & PF_PRISONER) printf("  (a prisoner)");
        if (W->pl[i].flags & PF_LAWYER)   printf("  (an Examiner)");
        say(CRLF);
        n++;
    }
    (void)n;
}

static void describe_room(int brief)
{
    Room *r = &rooms[P->loc];
    int o;

    say(CRLF);
    printf("%s" CRLF, r->name);

    if (!is_lit()) {
        sayl("It is pitch dark.  A Grue is hungry, and afraid of light.");
        list_players_here();
        return;
    }

    if (!brief || !(P->seen & (1UL << P->loc))) {
        printf("%s" CRLF, r->desc);
        P->seen |= 1UL << P->loc;
    }

    /* the "...." conditional lines belonging to this room */
    switch (P->loc) {
    case R_RIDDLE:
        if (P->riddle >= 0 && !P->ridans)
            sayl("....There are 36 different riddles.  READ the writing.");
        break;
    case R_JAIL:
        if (P->flags & PF_PRISONER) {
            sayl("....Prisoners cannot leave except when taken to court.");
            sayl("(PLEAD, and the Policeman will take you there.)");
        }
        break;
    case R_COURT:
        sayl("....Behind the bench there is a figure clad in a long dark robe");
        sayl("with a gavel in his hand.");
        if (P->flags & PF_ONTRIAL) {
            sayl("....*The policeman asks if the man behind the desk is your Lawyer,");
            if (P->flags & PF_LAWYER)
                sayl("....*Your Lawyer motions you to come near & be seated behind the desk.");
        }
        break;
    case R_WENDY:
        sayl((W->it[O_DESK].state & IS_LOCKED)
             ? "....*The desk is Locked."
             : "....*The desk is Unlocked.");
        break;
    case R_BAR:
        sayl(W->it[C_TROLL].state & IS_DEAD
             ? "....*The bartender is nowhere to be seen."
             : "....*There is a burly looking Troll behind the bar serving drinks.");
        if (W->nexaminers) {
            int i;
            say("....The list of names reads:");
            for (i = 0; i < W->nexaminers; i++)
                printf(" %s", W->examiners[i]);
            say(CRLF);
        }
        break;
    case R_BROOM:
        if (W->it[C_WITCH].loc == R_BROOM)
            sayl("....*There is a *Wicked Witch outside in the hallway.");
        else
            sayl("....*There is an old *Lady Waiting outside in the hallway.");
        break;
    case R_ELM:
        sayl("....*The sign says \"This way to the Skull Cave,  Beware!\"");
        break;
    case R_HONEY:
        sayl("....*There are many tiny arms which are reaching toward you.");
        break;
    case R_SHADOW:
        if (!(W->it[C_SHADOWEVIL].state & IS_DEAD))
            sayl("....There ais something moving toward you from the right... flee quickly.");
        break;
    case R_CELL:
        if (W->it[O_BOOK].loc == R_CELL && W->it[O_BOOK].carrier < 0) {
            sayl(W->it[O_BOOK].state & IS_LOCKED
                 ? "....*The book seems to be fastened shut."
                 : "....*The book opens and seems to be readable.");
        }
        break;
    default: break;
    }

    for (o = 1; o < NOBJ; o++) {
        Item *t = &W->it[o];
        if (t->carrier >= 0 || t->in) continue;
        if (t->loc != P->loc) continue;
        if ((objdef[o].flags & OF_HIDDEN) && !(t->state & IS_REVEALED)) continue;
        if (t->state & IS_DEAD) continue;
        if (objdef[o].here && objdef[o].here[0])
            printf("%s" CRLF, objdef[o].here);
    }
    list_players_here();
}

/* ================================================================ *
 *  Movement
 * ================================================================ */
static void enter_room(int room);

static void do_move(int dir)
{
    int dest;

    if (P->flags & PF_PRISONER) {
        sayl("Prisoners cannot leave except when taken to court.");
        return;
    }
    if (P->loc == R_RIDDLE && !P->ridans) {
        sayl("You cannot see any viable exits from this chamber.");
        sayl("You must answer a riddle to leave (the way you came in).");
        return;
    }
    if (P->loc == R_RIDDLE && P->ridans) {
        /*  AMAZON.TXT: (ANSWER (EQUAL (RIDCOD(RIDDLE) INTEXT)
         *                       (INCREMENT LOCATION(CRETIN))))
         *  ROOMS says you leave "the way you came in", so the exit that
         *  opens is the one back to wherever that was.                */
        enter_room(P->prevloc ? P->prevloc : R_SHADOW);
        return;
    }

    dest = rooms[P->loc].exits[dir];
    if (!dest) {
        printf("There is no way %s from here." CRLF, dirname_full[dir]);
        return;
    }

    /* "Wolf - Growls a lot, Guard, Must have GOOD-MARK to pass" */
    if (P->loc == R_SKULL && dest == R_SHADOW
        && W->it[C_WOLF].loc == R_SKULL && !(W->it[C_WOLF].state & IS_DEAD)
        && !(P->flags & PF_GOODMARK)) {
        sayl("The Wolf growls and blocks the passage.  You must have the");
        sayl("GOOD-MARK to pass.");
        return;
    }
    /* the Amazon river: the Shipyard is on the far bank */
    if (P->loc == R_SHIPYARD && dest == R_BANK) {
        if (!carrying(O_RAFT) && ATTR(P, AT_PRM, AT_FLT) < 12) {
            sayl("The Amazon is a raging torrent here and you have nothing");
            sayl("to float on.  \"Airplane,Paper - Necessary to fly across");
            sayl("the AMAZON river.\"");
            return;
        }
        sayl("You paddle back across the raging torrent.");
    }
    if (P->loc == R_COURT && dest == R_EXEC && !(P->flags & PF_ONTRIAL)) {
        sayl("The way down to the Execution Chamber is barred.  It is only");
        sayl("used for one thing, and the court has not finished with you.");
        return;
    }
    enter_room(dest);
}

static void room_entry(int room);

static void enter_room(int room)
{
    P->prevloc = P->loc;
    P->loc = (short)room;
    room_entry(room);
    if (P->state == PS_ACTIVE) describe_room(0);
}

/* AMAZON.TXT's (Action (Prop (ENTRY ...))) for each room. */
static void room_entry(int room)
{
    switch (room) {
    case R_RIDDLE:
        /*  (ENTRY (SET (RIDDLE RAND(RIDLEN))) (SET (RIDANS FALSE))) */
        P->riddle = rnd(36);
        P->ridans = 0;
        break;

    case R_EXEC:
        /*  ROOMS room 7, verbatim consequence. */
        P->flags |= PF_BANNED;
        P->banned_until = (int)(W->clock + 200);
        P->deaths++;
        P->state = PS_DEAD;
        break;

    case R_SHADOW:
        if (!(W->it[C_SHADOWEVIL].state & IS_DEAD) && rnd(100) < 30) {
            if (P->flags & PF_SHADOWED) {
                sayl("");
                sayl("The Shadow has caught you a second time.");
                sayl("\"If you are caught more than once by the Shadow, you");
                sayl("will surely die!\"");
                P->flags |= PF_CURSED;
                sethp(P, 0);
            } else {
                sayl("");
                sayl("The shadow of evil has caught you!");
                sayl("You must make it to the surface before nightfall, else");
                sayl("you will die.");
                sayl("(\"you must sleep in sunlight if caught to remove the");
                sayl(" curse\" -- and a second catch is fatal.)");
                P->flags |= PF_SHADOWED;
                P->nightfall = 25;
            }
        }
        break;

    case R_HONEY:
        /*  ROOMS room 18's four conditional lines. */
        if (carrying(O_HONEY)) {
            sayl("....*The arms have grabbed your Honey and are quickly drawing away.");
            put_in_room(O_HONEY, R_HONEY);
            W->it[O_HONEY].state |= IS_DEAD;      /* gone into the field */
        } else if (rnd(100) < 30) {
            sayl("....*The arms have grabbed you and are trying to pull you apart.");
            sethp(P, hpleft(P) - 2 - rnd(4));
            if (hpleft(P) > 0)
                sayl("....*The arms have released you and are quickly drawing away.");
        }
        break;

    case R_SHIPYARD:
        if (W->it[C_HOOK].loc == R_SHIPYARD && !(W->it[C_HOOK].state & IS_DEAD)
            && !carrying(O_CLOCK)) {
            sayl("Captain Hook \"Captures children and makes them slaves\".");
            sayl("He looks you over and decides you will do.");
            if (rnd(100) < 40) {
                sayl("Peter Pan \"Rescues people from pirate ship\" -- and does.");
            } else {
                sethp(P, hpleft(P) - 3);
            }
        }
        break;

    default: break;
    }

    if (P->loc && (rooms[P->loc].flags & RF_SURFACE) && (P->flags & PF_SHADOWED)
        && !(P->flags & PF_CURSED)) {
        P->nightfall = 0;
        sayl("You have reached the surface.  You are still shadowed --");
        sayl("\"you must sleep in sunlight if caught to remove the curse.\"");
    }
}

/* ================================================================ *
 *  ACQUIRE / RELINQUISH  (CMDLIB.SAI classes, AMAZON.SAI wording)
 * ================================================================ */
static void arrest(const char *why);

static int try_take(const char *word, int strength)
{
    int o = obj_by_word(word);
    Item *t;

    if (!o) { printf("I see no %s here." CRLF, word); return 0; }
    t = &W->it[o];

    if (t->carrier == ME) { printf("You already have %s." CRLF, word); return 0; }

    /* STEAL / ROB -- CMDLIB strength 2 -- work on other players too. */
    if (t->carrier >= 0) {
        Player *victim = &W->pl[t->carrier];
        if (strength < 2) {
            printf("%s has it." CRLF, victim->name);
            return 0;
        }
        if (victim->loc != P->loc) {
            printf("%s is not here." CRLF, victim->name);
            return 0;
        }
        if (ATTR(P, AT_PHY, AT_DEX) + rnd(12)
            > ATTR(victim, AT_PHY, AT_AGI) + 4) {
            char m[MSGLEN];
            give_to_me(o);
            sprintf(m, "%s has just robbed %s of %.40s!", P->name,
                    victim->name, objdef[o].shortname);
            broadcast(m, ME);
            printf("%s Taken." CRLF, word);
            /* "Policeman - Can arrest people, request fine etc." */
            if (W->it[C_POLICEMAN].loc == P->loc) arrest("robbery");
            return 1;
        }
        printf("%s catches your hand." CRLF, victim->name);
        return 0;
    }

    if (!obj_here(o)) { printf("I see no %s here." CRLF, word); return 0; }
    if (!(objdef[o].flags & OF_TAKEABLE)) {
        printf("You cannot take %s." CRLF, objdef[o].shortname);
        return 0;
    }
    if (t->state & IS_TIED) {
        printf("%s is tied up here." CRLF, objdef[o].shortname);
        return 0;
    }
    if (!(objdef[o].flags & OF_VEHICLE)
        && carried_weight() + objdef[o].weight > carry_limit()) {
        sayl("You are carrying too much already.");
        return 0;
    }
    /* the Equipment Shop sells, it does not give */
    if (P->loc == R_SHOP && (objdef[o].flags & OF_SHOPITEM)
        && !(t->state & IS_PAID)) {
        printf("Each item is worth some number of points.  BUY %s"
               " -- it costs %d." CRLF, word, objdef[o].value);
        return 0;
    }

    give_to_me(o);
    printf("%s Taken." CRLF, word);
    if (objdef[o].flags & OF_VEHICLE)
        printf("(You are %s it, not carrying it.)" CRLF,
               o == O_TRUCK ? "driving" : "leading");

    if (o == O_TRUCK && W->it[C_POLICEMAN].loc == P->loc)
        arrest("being caught with the truck");
    if (o == C_FIGURE) {
        P->figureturns = 0;
        sayl("(if you pick him up, you better put him down)");
    }
    return 1;
}

static int try_drop(const char *word, int strength)
{
    int o = obj_by_word(word);
    if (!o || !carrying(o)) {
        printf("You are not carrying %s." CRLF, word);
        return 0;
    }
    put_in_room(o, P->loc);
    printf("%s DROPPED." CRLF, word);

    if (strength >= 2) {                      /* THROW / TOSS */
        sayl("You throw it hard.");
        /* "Vampire - Can be killed ... with silver bullets" */
        if (o == O_SILVERBULLET && W->it[C_VAMPIRE].loc == P->loc) {
            W->it[C_VAMPIRE].state |= IS_DEAD;
            sayl("The silver bullet strikes the Vampire.  It crumbles away.");
            P->score += 40;
        }
        /* "Wicked Witch - Allergic to water" */
        if (o == O_JUG && W->it[C_WITCH].loc == P->loc
            && !(W->it[C_WITCH].state & IS_DEAD)) {
            W->it[C_WITCH].state |= IS_DEAD;
            sayl("The water strikes the Wicked Witch.  She is allergic to it,");
            sayl("and there is not much of her left.");
            P->score += 60;
        }
    }
    if (o == C_FIGURE) P->figureturns = 0;
    return 1;
}

/* ================================================================ *
 *  The Riddle Room, the Bar Exam and the law
 * ================================================================ */
static void do_read(const char *word)
{
    int o;

    if (P->loc == R_RIDDLE && (!word[0] || !strcmp(word, "WRITING")
                               || !strcmp(word, "MARKINGS")
                               || !strcmp(word, "WALLS"))) {
        /*  AMAZON.TXT: (READ (DISPLAY RIDTXT(RIDDLE))) */
        if (P->riddle < 0) P->riddle = rnd(36);
        say(CRLF);
        printf("%s" CRLF, riddle_text[P->riddle]);
        sayl("");
        sayl("(ANSWER <word> when you have it.)");
        return;
    }
    if (!word[0]) { sayl("Read what?"); return; }

    /*  The signs are part of the room text in ROOMS, not separate objects,
     *  so READ SIGN is answered by the room.                            */
    if (!strcmp(word, "SIGN")) {
        switch (P->loc) {
        case R_BANK:
            sayl("....*Welcome to the Amazon River Valley, be pleased to discover a new world");
            sayl("within the caves and jungle villages to be found nearby.  Untold treasures");
            sayl("await you deep inside the Amazon, but beware the suspicious... There are Witches,");
            sayl("Demons, and wild animals running loose which are as deadly to you as they");
            sayl("appear wonderful.  Many a soul has died within, leaving all the treasures of");
            sayl("Lost generations, Dead Adventurers and Fairy Kingdoms to those skillful enough");
            sayl("to obtain them.");
            return;
        case R_ELM:
            sayl("....*The sign says \"This way to the Skull Cave,  Beware!\"");
            return;
        case R_BAR: {
            int i;
            sayl("The sign reads \"Examiners & Collectors\".");
            if (!W->nexaminers) sayl("The list of names below it is empty.");
            else {
                say("The list of names reads:");
                for (i = 0; i < W->nexaminers; i++) printf(" %s", W->examiners[i]);
                say(CRLF);
            }
            return;
        }
        default:
            sayl("There is no sign here.");
            return;
        }
    }

    o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }

    if (o == O_BOOK) {
        if (W->it[O_BOOK].state & IS_LOCKED) {
            sayl("....*The book seems to be fastened shut.");
            return;
        }
        sayl("....*The book opens and seems to be readable.");
        sayl("");
        sayl("       Yellav Nozama Eht Sdraziw Dnarg");
        sayl("");
        sayl("....* Nozama :drow cigam,  Xnihpfs Cxx -- Enasni Eht Fo Yraid.");
        sayl("");
        sayl("(It is all written in a mirror.  Hold it up to one, or just");
        sayl(" read every word backwards.)");
        return;
    }
    if (o == O_SIGN || (P->loc == R_BANK && !strcmp(word, "SIGN"))) {
        sayl("....*Welcome to the Amazon River Valley, be pleased to discover a new world");
        sayl("within the caves and jungle villages to be found nearby.  Untold treasures");
        sayl("await you deep inside the Amazon, but beware the suspicious... There are Witches,");
        sayl("Demons, and wild animals running loose which are as deadly to you as they");
        sayl("appear wonderful.  Many a soul has died within, leaving all the treasures of");
        sayl("Lost generations, Dead Adventurers and Fairy Kingdoms to those skillful enough");
        sayl("to obtain them.");
        return;
    }
    if (o == O_LAWBOOK) {
        sayl("The Law Book of the Amazon River Valley.  It \"Belongs to Lawyer");
        sayl("& Police\" and is \"necessary to get out of Jail\".  It covers the");
        sayl("Policeman, the Lawyer, the Judge, the Court Room and the");
        sayl("Execution Chamber, and it is what the Bar Exam is set from.");
        sayl("STUDY it before you take the exam.");
        return;
    }
    if (objdef[o].note && objdef[o].note[0]) {
        printf("%s" CRLF, objdef[o].note);
        return;
    }
    printf("There is nothing written on %s." CRLF, word);
}

static void do_answer(const char *word)
{
    if (P->loc == R_RIDDLE) {
        if (P->riddle < 0) { sayl("You have not READ the writing yet."); return; }
        /*  (ANSWER (EQUAL (RIDCOD(RIDDLE) INTEXT)
         *                 (INCREMENT LOCATION(CRETIN)))) */
        if (!strcmp(word, riddle_answer[P->riddle])) {
            P->ridans = 1;
            P->score += 15;
            sayl("");
            sayl("The markings on the walls rearrange themselves, and the way");
            sayl("you came in is a way once more.");
        } else {
            sayl("That is not the answer.  The markings do not move.");
        }
        return;
    }
    if (P->loc == R_BAR && P->barasked) {
        int q = P->barq;
        if (!strcmp(word, bar_answer[q])) {
            P->barright++;
            P->barasked = 0;
            printf("The bartender nods.  (%d of 5 correct.)" CRLF, P->barright);
            if (P->barright >= 5) {
                char m[MSGLEN];
                P->flags |= PF_LAWYER;
                P->score += 100;
                sayl("....*The bartender exclaims \"Congratulations, You have just passed your Bar Exam\"");
                sayl("and he walks over to the sign and adds your name to the list of Examiners.");
                if (W->nexaminers < MAXPLAYERS) {
                    strncpy(W->examiners[W->nexaminers], P->name, NAMELEN - 1);
                    W->examiners[W->nexaminers][NAMELEN - 1] = 0;
                    W->nexaminers++;
                }
                W->it[O_DIPLOMA].carrier = (short)ME;
                W->it[O_DIPLOMA].loc = 0;
                sprintf(m, "%s has passed the Bar Exam.", P->name);
                broadcast(m, ME);
            }
        } else {
            P->barasked = 0;
            P->barright = 0;
            sayl("....*The bartender grabs you by the collar & throws you out into the night.");
            enter_room(R_SHOP);
        }
        return;
    }
    sayl("There is no question before you.");
}

static void bar_ask(void)
{
    if (W->it[C_TROLL].state & IS_DEAD) {
        sayl("....*The bartender is nowhere to be seen.");
        return;
    }
    if (P->flags & PF_LAWYER) {
        sayl("The bartender points at the sign.  Your name is already on it.");
        return;
    }
    P->barq = rnd(36);
    P->barasked = 1;
    sayl("....*The bartender brings you your drink.");
    sayl("....*The bartender asks you a question.  (There are 36 different questions)");
    sayl("");
    printf("%s" CRLF, bar_question[P->barq]);
    sayl("");
    sayl("(ANSWER <word>.  Five right in a row and you are an Examiner;");
    sayl(" one wrong and you are out in the night.)");
}

static void arrest(const char *why)
{
    char m[MSGLEN];
    printf("The Policeman arrests you for %s." CRLF, why);
    P->flags |= PF_PRISONER | PF_ARRESTED;
    sprintf(m, "%s has been arrested for %.60s.", P->name, why);
    broadcast(m, ME);
    P->prevloc = P->loc;
    P->loc = R_JAIL;
    describe_room(0);
}

static void do_plead(void)
{
    if (!(P->flags & PF_PRISONER)) { sayl("You are not charged with anything."); return; }
    if (P->loc == R_JAIL) {
        if (!carrying(O_LAWBOOK) && !(P->flags & PF_LAWYER)) {
            sayl("\"Law Book - Belongs to Lawyer & Police, necessary to get");
            sayl("out of Jail.\"  You have neither the book nor a lawyer's");
            sayl("name on the Bar Room sign.");
            if (P->score >= 50) {
                /* "Policeman - Can arrest people, request fine etc." */
                P->score -= 50;
                P->flags &= ~(PF_PRISONER | PF_ARRESTED);
                sayl("");
                sayl("The Policeman requests a fine instead.  It costs you 50");
                sayl("points, and you are free to go.");
                return;
            }
            sayl("");
            sayl("You have nothing to pay a fine with either, so you will have");
            sayl("to take your chances in front of the Judge.");
        }
        sayl("The Policeman takes you to court.");
        P->flags |= PF_ONTRIAL;
        enter_room(R_COURT);
        return;
    }
    if (P->loc == R_COURT) {
        int good = 0;
        if (P->flags & PF_LAWYER) good += 2;      /* you are your own lawyer */
        if (carrying(O_LAWBOOK))  good += 2;
        if (W->it[C_LAWYER].loc == R_COURT) good += 1;
        if (ATTR(P, AT_PRM, AT_CHA) > 14) good += 1;
        sayl("....*The policeman asks if the man behind the desk is your Lawyer,");
        if (good >= 3) {
            sayl("....*Your Lawyer motions you to come near & be seated behind the desk.");
            sayl("");
            sayl("The Judge raises the gavel, and lets it fall.  You are free to go.");
            P->flags &= ~(PF_PRISONER | PF_ARRESTED | PF_ONTRIAL);
            enter_room(R_JAIL);
        } else {
            sayl("");
            enter_room(R_EXEC);
            printf("%s" CRLF, rooms[R_EXEC].desc);
        }
        return;
    }
    sayl("Not here.");
}

/* ================================================================ *
 *  Everything else
 * ================================================================ */
static void do_inventory(void)
{
    int o, n = 0;
    for (o = 1; o < NOBJ; o++)
        if (W->it[o].carrier == ME) {
            if (!n++) sayl("You are carrying:");
            printf("     %s" CRLF, objdef[o].longname);
        }
    if (!n) sayl("You are empty-handed.");
    printf("Load %d of %d." CRLF, carried_weight(), carry_limit());
}

static int collectable(int o);

static void do_score(void)
{
    int o, treasure = 0, delivered = 0;
    for (o = 1; o < NOBJ; o++) {
        if (!collectable(o)) continue;
        treasure++;
        if (W->it[o].state & IS_DELIVERED) delivered++;
    }
    printf(CRLF "%s, you have scored %d point%s in %d turn%s." CRLF,
           P->name, P->score, P->score == 1 ? "" : "s",
           P->turns, P->turns == 1 ? "" : "s");
    printf("Treasures returned: %d of %d." CRLF, delivered, treasure);
    printf("Deaths: %d." CRLF, P->deaths);
    if (P->flags & PF_LAWYER)   sayl("You are on the list of Examiners.");
    if (P->flags & PF_GOODMARK) sayl("You carry the Phantom's GOOD-MARK.");
    if (P->flags & PF_SHADOWED) sayl("You are shadowed.");
}

static void do_stats(void)
{
    /*  Printed straight out of the AMAZON.OFF attribute words, in the
     *  order AMAZON.TXT lists them under (Ability (Prop ...)).        */
    printf(CRLF "%s   [CODE '%s']" CRLF, P->name, P->code);
    printf("     STRENGTH      %2d      CONSTITUTION  %2d" CRLF,
           ATTR(P, AT_PRM, AT_STR), ATTR(P, AT_PHY, AT_CON));
    printf("     INTELLIGENCE  %2d      SIZE          %2d" CRLF,
           ATTR(P, AT_PRM, AT_INT), ATTR(P, AT_PHY, AT_SIZ));
    printf("     WISDOM        %2d      AGILITY       %2d" CRLF,
           ATTR(P, AT_PRM, AT_WIS), ATTR(P, AT_PHY, AT_AGI));
    printf("     CHARISMA      %2d      DEXTERITY     %2d" CRLF,
           ATTR(P, AT_PRM, AT_CHA), ATTR(P, AT_PHY, AT_DEX));
    printf("     FLOATING      %2d" CRLF, ATTR(P, AT_PRM, AT_FLT));
    printf("     HIT DIE       %2d      HIT POINTS  %3d of %3d" CRLF,
           ATTR(P, AT_HTS, AT_DIE), hpleft(P), ATTR(P, AT_HTS, AT_HIT));
    printf("     EXPERIENCE  %5d      TURNS %5d   DEATHS %d" CRLF,
           (int)P->at[AT_EXP], P->turns, P->deaths);
    printf("     LANGUAGE (#%012llo)" CRLF, (unsigned long long)P->at[AT_LNG]);
}

static void do_who(void)
{
    int i, n = 0;
    say(CRLF);
    for (i = 0; i < MAXPLAYERS; i++) {
        Player *q = &W->pl[i];
        if (q->state == PS_FREE) continue;
        printf("User # %-2d is [%s]  in %-24s %s" CRLF, i + 1, q->name,
               q->state == PS_DEAD ? "(executed)" : rooms[q->loc].name,
               q->state == PS_DEAD ? "" :
               (q->flags & PF_PRISONER) ? "(a prisoner)" : "");
        n++;
    }
    if (!n) sayl("Nobody is around.");
}

static void do_tell(void)
{
    char msg[MSGLEN], w[32];
    int more;
    msg[0] = 0;
    while (getcmd(&CL, w, &more)) {
        if (strlen(msg) + strlen(w) + 2 >= sizeof msg) break;
        if (msg[0]) strcat(msg, " ");
        strcat(msg, w);
    }
    if (!msg[0]) { sayl("Tell them what: "); return; }
    {
        char line[MSGLEN];
        snprintf(line, sizeof line, "%s says: %s", P->name, msg);
        broadcast(line, ME);
        sayl("Told.");
    }
}

static void do_buy(const char *word)
{
    int o = obj_by_word(word);
    if (P->loc != R_SHOP) { sayl("There is nothing for sale here."); return; }
    if (!o || !(objdef[o].flags & OF_SHOPITEM)) {
        printf("The shop does not stock %s." CRLF, word);
        return;
    }
    if (W->it[o].carrier >= 0) { sayl("Somebody has already bought that."); return; }
    if (P->score < objdef[o].value) {
        printf("%s costs %d and you have %d." CRLF,
               objdef[o].shortname, objdef[o].value, P->score);
        return;
    }
    P->score -= objdef[o].value;
    W->it[o].state |= IS_PAID;
    give_to_me(o);
    printf("%s Taken.  (%d points)" CRLF, word, objdef[o].value);
}

/*  Where a thing has to be taken for its reward.
 *
 *  OBJECT.TYP names an owner for most of them ("Belongs to Tarzan, reward
 *  offered", "return to Johnny appleseed"), but three are owed to a PLACE
 *  rather than a person -- "Padding - For PADDED CELL ... return to PADDED
 *  CELL" and "Horror Chamber - Poster, belongs on entrance to cave" -- and
 *  the jewellery and the five key sets are owed to nobody at all.  For
 *  those the Bar Room is the drop: its sign reads "Examiners & Collectors",
 *  and the Bar Exam accounts for the Examiners.  See RECONSTRUCTION.md.  */
static int deliver_room(int o)
{
    if (o == O_PADDING) return R_CELL;
    if (o == O_POSTER)  return R_SKULLENT;
    if (objdef[o].owner) return 0;                  /* judged by the owner */
    if (objdef[o].flags & OF_TREASURE) return R_BAR;
    return -1;                                      /* nothing is owed */
}

static int collectable(int o)
{
    return objdef[o].reward > 0 || deliver_room(o) > 0;
}

static void do_deliver(const char *word)
{
    int o = obj_by_word(word), owner, room;
    if (!o || !carrying(o)) { printf("You are not carrying %s." CRLF, word); return; }
    if (W->it[o].state & IS_DELIVERED) { sayl("That has been returned already."); return; }

    room  = deliver_room(o);
    owner = objdef[o].owner;

    if (room < 0) { sayl("Nobody has a claim on that."); return; }
    if (room > 0) {
        if (P->loc != room) {
            printf("That has to be taken to the %s." CRLF, rooms[room].name);
            return;
        }
        put_in_room(o, room);
        W->it[o].state |= IS_DELIVERED;
        P->score += objdef[o].reward ? objdef[o].reward : objdef[o].value;
        P->at[AT_EXP] += 10;
        printf("Delivered.  %d points." CRLF,
               objdef[o].reward ? objdef[o].reward : objdef[o].value);
        {
            char m[MSGLEN];
            snprintf(m, sizeof m, "%s has delivered %.40s.",
                     P->name, objdef[o].shortname);
            broadcast(m, ME);
        }
        return;
    }
    if (W->it[owner].loc != P->loc || (W->it[owner].state & IS_DEAD)) {
        printf("%s is not here." CRLF, objdef[owner].shortname);
        return;
    }
    if (o == O_CARROTS && !carrying(O_TRUCK)) {
        sayl("\"Requires a truck to deliver them.\"");
        return;
    }
    if (o == O_CARROTS
        && !((W->it[O_RABBITM].state & IS_DELIVERED)
             && (W->it[O_RABBITF].state & IS_DELIVERED))) {
        sayl("\"must find rabbits first\"");
        return;
    }
    put_in_room(o, 0);
    W->it[o].loc = 0;
    W->it[o].state |= IS_DELIVERED;
    P->score += objdef[o].reward;
    P->at[AT_EXP] += objdef[o].reward;
    printf("%s takes %s.  REWARD: %d points." CRLF,
           objdef[owner].shortname, objdef[o].shortname, objdef[o].reward);
    if (owner == C_PHANTOM && !(P->flags & PF_GOODMARK)) {
        P->flags |= PF_GOODMARK;
        sayl("\"Phanthom - Gives GOOD-MARK for helping him\".  You have it now.");
    }
    {
        char m[MSGLEN];
        sprintf(m, "%s has returned %.40s.", P->name, objdef[o].shortname);
        broadcast(m, ME);
    }
}

static void do_open(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (W->it[o].state & IS_LOCKED) {
        if (o == O_DESK) { sayl("....*The desk is Locked."); return; }
        if (o == O_BOOK) { sayl("....*The book seems to be fastened shut."); return; }
        sayl("It is locked.");
        return;
    }
    if (!(objdef[o].flags & OF_CONTAINER)) { sayl("You cannot open that."); return; }
    W->it[o].state |= IS_OPEN;
    printf("%s is open." CRLF, objdef[o].shortname);
    {   /* reveal what was hidden inside */
        int k, n = 0;
        for (k = 1; k < NOBJ; k++)
            if (W->it[k].in == o) {
                W->it[k].state |= IS_REVEALED;
                if (!n++) sayl("Inside:");
                printf("     %s" CRLF, objdef[k].longname);
            }
        if (!n) sayl("It is empty.");
    }
}

static void do_unlock(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (!(W->it[o].state & IS_LOCKED)) { sayl("It is not locked."); return; }

    if (o == O_DESK) {
        if (carrying(O_KEYSA) || carrying(O_KEYSB) || carrying(O_KEYSC)) {
            W->it[o].state &= ~IS_LOCKED;
            sayl("....*The desk is Unlocked.");
            return;
        }
        sayl("You have no key that fits.");
        return;
    }
    if (o == O_BOOK) {
        /* the wizard's diary: the magic word is in the diary itself */
        sayl("The book is fastened with a word, not a lock.  SAY it.");
        return;
    }
    sayl("You have no key that fits.");
}

static void do_say(const char *word)
{
    if (!strcmp(word, "AMAZON")) {
        if (obj_here(O_BOOK) && (W->it[O_BOOK].state & IS_LOCKED)) {
            W->it[O_BOOK].state &= ~IS_LOCKED;
            sayl("....*The book opens and seems to be readable.");
            P->score += 25;
            return;
        }
        sayl("Nothing happens.  It is the right word in the wrong place.");
        return;
    }
    printf("You say \"%s\".  Nothing happens." CRLF, word);
}

static void do_examine(const char *word)
{
    int o = obj_by_word(word);
    if (!o) { printf("I see no %s here." CRLF, word); return; }
    if (!obj_here(o) && W->it[o].loc != P->loc) {
        printf("I see no %s here." CRLF, word);
        return;
    }
    printf("%s" CRLF, objdef[o].longname);
    if (objdef[o].note && objdef[o].note[0])
        printf("%s" CRLF, objdef[o].note);
    if (objdef[o].value)
        printf("Worth %d points." CRLF, objdef[o].value);
    if (objdef[o].owner)
        printf("Belongs to %s." CRLF, objdef[objdef[o].owner].shortname);
}

static void do_kiss(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (o == C_FROG || o == C_BULLFROG || o == C_FROGGREY) {
        if (P->ptype & (P_PRIN | P_PRSS)) {
            sayl("\"Frog - If kissed by prince/princess becomes princess/prince\"");
            sayl("The frog straightens up, and is royalty.");
            P->score += 30;
        } else {
            sayl("The frog looks at you.  \"If kissed by prince/princess\" --");
            sayl("and you are neither.  Nothing happens.");
        }
        return;
    }
    sayl("That would not be welcome.");
}

static void do_attack(const char *word, int lethal)
{
    int o = obj_by_word(word);
    int dam, hp;
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (!(objdef[o].flags & OF_CREATURE)) { sayl("Attacking that would achieve nothing."); return; }
    if (W->it[o].state & IS_DEAD) { sayl("It is already dead."); return; }

    if (o == C_VAMPIRE && !carrying(O_SILVERBULLET)) {
        sayl("\"Vampire - Can be killed by lone ranger, or with silver bullets\".");
        sayl("Your blows go through it.");
        sethp(P, hpleft(P) - 4);
        return;
    }
    if (o == C_WITCH && carrying(O_JUG)) {
        W->it[o].state |= IS_DEAD;
        sayl("You empty the jug over her.  \"Allergic to water\" was not an");
        sayl("exaggeration.");
        P->score += 60;
        return;
    }
    if (o == C_RAT && carrying(O_RATPOISON)) {
        W->it[o].state |= IS_DEAD;
        sayl("\"Blue Rat - Poisonous, can be killed with rat poison\".  It is.");
        P->score += 10;
        return;
    }

    dam = 1 + ATTR(P, AT_PRM, AT_STR) / 3 + rnd(6);
    if (carrying(O_KNIFE)) dam += 4;
    if (carrying(O_GUN) && carrying(O_BULLETS)) dam += 10;
    hp = a_get(W->it[o].at[AT_HTS], AT_HPL) - dam;
    if (hp < 0) hp = 0;
    a_put(&W->it[o].at[AT_HTS], AT_HPL, hp);

    if (hp == 0) {
        W->it[o].state |= IS_DEAD;
        printf("You have killed %s." CRLF, objdef[o].shortname);
        P->at[AT_EXP] += 10;
        /* "Tarzan - Ruler of Jungle animals, If kill animal you better eat it" */
        if (objdef[o].ptype & P_ANIM)
            sayl("Tarzan is Ruler of the Jungle animals.  If you kill an animal");
        if (objdef[o].ptype & P_ANIM)
            sayl("you had better EAT it.");
    } else {
        int back = 1 + rnd(1 + a_get(W->it[o].at[AT_HTS], AT_DIE) * 3);
        printf("You strike %s.  It strikes back." CRLF, objdef[o].shortname);
        sethp(P, hpleft(P) - back);
    }
    (void)lethal;
}

static void do_eat(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !carrying(o)) { printf("You are not carrying %s." CRLF, word); return; }
    if (o == O_CAKE) {
        sayl("\"Cake, EAT-ME - Guess!\"");
        sayl("You grow.  SIZE up by four.");
        a_put(&P->at[AT_PHY], AT_SIZ,
              ATTR(P, AT_PHY, AT_SIZ) + 4 > 31 ? 31 : ATTR(P, AT_PHY, AT_SIZ) + 4);
        put_in_room(o, 0); W->it[o].loc = 0;
        return;
    }
    if (o == O_APPLERED) {
        sayl("\"Apple, RED - Belongs to NICE old lady, sleeping poison\".");
        sayl("You fall asleep where you stand.");
        P->nightfall = 0;
        sethp(P, hpleft(P) - 6);
        put_in_room(o, 0); W->it[o].loc = 0;
        return;
    }
    if (o == O_FOOD) {
        sayl("You eat the rations.");
        sethp(P, hpleft(P) + 6);
        put_in_room(o, 0); W->it[o].loc = 0;
        return;
    }
    if (objdef[o].ptype & P_ANIM) {
        sayl("You eat it, as Tarzan requires of anyone who kills an animal.");
        W->it[o].state |= IS_DEAD;
        put_in_room(o, 0); W->it[o].loc = 0;
        return;
    }
    sayl("That is not edible.");
}

static void do_drink(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (o == O_URN) {
        sayl("\"Urn, DRINK-ME - Guess!\"");
        sayl("You shrink.  SIZE down by four.");
        a_put(&P->at[AT_PHY], AT_SIZ,
              ATTR(P, AT_PHY, AT_SIZ) > 4 ? ATTR(P, AT_PHY, AT_SIZ) - 4 : 1);
        put_in_room(o, 0); W->it[o].loc = 0;
        return;
    }
    if (o == O_JUG) { sayl("You drink some water."); sethp(P, hpleft(P) + 2); return; }
    sayl("You cannot drink that.");
}

static void do_sleep(void)
{
    if (!(rooms[P->loc].flags & RF_SURFACE)) {
        sayl("\"Bullfrog - Enchanted, Don't fall asleep here\".  You think");
        sayl("better of it.");
        return;
    }
    if (P->flags & PF_SHADOWED) {
        sayl("\"you must sleep in sunlight if caught to remove the curse.\"");
        sayl("You sleep in the sun.  The curse lifts.");
        P->flags &= ~PF_SHADOWED;
        P->nightfall = 0;
        P->score += 20;
        return;
    }
    sayl("You doze in the sun for a while.");
    sethp(P, hpleft(P) + 3);
}

static void do_light(const char *word, int on)
{
    int o = obj_by_word(word);
    if (!o || !carrying(o)) { printf("You are not carrying %s." CRLF, word); return; }
    if (!(objdef[o].flags & OF_LIGHT)) { sayl("That is not a light."); return; }
    if (on) {
        if (o == O_FLASHLIGHT && !carrying(O_BATTERIES)) {
            sayl("The flashlight needs its batteries.");
            return;
        }
        if (o == O_LAMP && !carrying(O_PAN) && !carrying(O_MATCHES)) {
            sayl("The oil lamp needs the pan of oil and the matches.");
            return;
        }
        W->it[o].state |= IS_LIT;
        printf("%s is lit." CRLF, objdef[o].shortname);
    } else {
        W->it[o].state &= ~IS_LIT;
        printf("%s is out." CRLF, objdef[o].shortname);
    }
}

static void do_launch(void)
{
    if (P->loc != R_CLIFF) { sayl("There is no catapult here."); return; }
    if (!carrying(O_AIRPLANE) && W->it[O_AIRPLANE].loc != R_CLIFF) {
        sayl("The catapult is empty.");
        return;
    }
    sayl("The catapult fires.  \"Airplane,Paper - Necessary to fly across");
    sayl("the AMAZON river.\"");
    if (carrying(O_PARACHUTE) || carrying(O_RAFT) || rnd(100) < 55) {
        sayl("You come down on the far bank, at the shipyard.");
        if (carrying(O_AIRPLANE)) put_in_room(O_AIRPLANE, R_SHIPYARD);
        enter_room(R_SHIPYARD);
    } else {
        sayl("You miss.  \"Nice to take a parachute, life-raft in case you miss\".");
        sethp(P, hpleft(P) - 12);
        enter_room(R_BANK);
    }
}

static void do_ride(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (o == O_STALLION) {
        if (W->it[o].state & IS_TIED) {
            if (P->flags & PF_GOODMARK) {
                W->it[o].state &= ~IS_TIED;
                sayl("The Phantom nods.  \"sometimes others are allowed to ride\".");
            } else {
                sayl("The White Stallion is tied up, and it is not yours.");
                return;
            }
        }
        sayl("You ride.  It is faster than walking, and it is not yours.");
        return;
    }
    sayl("You cannot ride that.");
}

static void do_hint(void)
{
    /*  "Frog - Has answers to riddles, gives hints"
     *  "Witch - Good, Generous, Helpful * Gives hints"
     *  "Lone Ranger - Gives hints"                                    */
    if (obj_here(C_FROG) && P->loc == R_RIDDLE && P->riddle >= 0) {
        printf("The Frog blinks and says: \"%c...\"" CRLF,
               riddle_answer[P->riddle][0]);
        return;
    }
    if (obj_here(C_LONERANGER)) {
        sayl("The Lone Ranger: \"I am looking for a white stallion.  It is");
        sayl("tied up in a cave shaped like a head.  Tonto wants his pony.\"");
        return;
    }
    if (obj_here(C_WITCH) && !(W->it[C_WITCH].state & IS_DEAD)) {
        sayl("The witch, in a generous mood: \"Water. Always water. And silken");
        sayl("spider webs, if you want me quiet instead of dead.\"");
        return;
    }
    sayl("There is nobody here who gives hints.");
}

static void do_tie(const char *word)
{
    int o = obj_by_word(word);
    if (!o || !obj_here(o)) { printf("I see no %s here." CRLF, word); return; }
    if (o == C_WITCH && carrying(O_WEB)) {
        sayl("\"Allergic to water, may be tied up with silken spider webs\".");
        sayl("The Wicked Witch is tied up and can do nothing.");
        W->it[o].state |= IS_TIED;
        P->score += 40;
        put_in_room(O_WEB, 0); W->it[O_WEB].loc = 0;
        return;
    }
    sayl("You have nothing to tie it with.");
}

static void do_help(void)
{
    /*  AMAZON.TXT, Item 'HELP', (Description "...") -- verbatim. */
    sayl("");
    sayl("This is the Game of Amazon");
    sayl("");
    sayl("You are wandering somewhere in the Amazon River Valley deep in the");
    sayl("heart of South America.  You may find Untold treasures in the caverns");
    sayl("below.  But, BEWARE!!!  There are traps, puzzles, and other adventurers");
    sayl("awaiting you in the caves.  Watch out for anything and everything,");
    sayl("for things may not be what they seem.");
    sayl("");
    sayl("Commands come in classes, as CMDLIB.SAI defined them:");
    sayl("   ACQUIRE  GET TAKE CAPTURE KEEP          (STEAL ROB -- by force)");
    sayl("   RELINQUISH DROP RELEASE                 (THROW TOSS -- by force)");
    sayl("   DIRECTION  N NE E SE S SW W NW U D  and their long forms");
    sayl("Also: LOOK INVENTORY EXAMINE READ ANSWER SAY OPEN UNLOCK PUT");
    sayl("      ATTACK KILL KISS EAT DRINK SLEEP RIDE LAUNCH TIE LIGHT");
    sayl("      BUY ORDER DELIVER PLEAD STUDY HINT WAIT");
    sayl("      SCORE STATS WHO TELL SOURCE HELP QUIT");
    sayl("");
}

static void do_source(void)
{
    sayl("");
    sayl("AMAZON, by Carl Baltrunas, Tymshare, 1977-1981.  Never finished.");
    sayl("");
    sayl("What survives on the tapes, and is used here verbatim:");
    sayl("   ROOMS       22-Sep-77  the 21 rooms, names and descriptions");
    sayl("   AMAZON.SAI  18-Dec-77  the command loop and its exact replies");
    sayl("   CMDLIB.SAI  11-Sep-77  the verb classes and direction codes");
    sayl("   PRTEST.SAI  03-Sep-77  the 16-player multi-process framework");
    sayl("   AMAZON.OFF  21-Oct-78  the item and attribute data structures");
    sayl("   OBJECT.TYP  29-Jul-79  the objects and creatures");
    sayl("   AMAZON.TXT  09-Sep-79  the HELP text and the Riddle Room");
    sayl("   AMAZOT.SAI  08-Jan-81  the parser test (and AMAZOT.SAV, its");
    sayl("                          compiled form -- NOT a compiled game)");
    sayl("");
    sayl("What does NOT survive, and was written for this port:");
    sayl("   the 36 riddles and the 36 Bar Exam questions (their COUNT is");
    sayl("      documented, their text never was);");
    sayl("   which room connects to which -- no surviving file says;");
    sayl("   every verb outside CMDLIB's three classes;");
    sayl("   all numeric values: prices, weights, rewards, hit points.");
    sayl("");
    sayl("See docs/RECONSTRUCTION.md for the line-by-line accounting.");
    sayl("");
}

/* ================================================================ *
 *  Per-turn daemons
 * ================================================================ */
static void arrest(const char *why);

static void tick(void)
{
    W->clock++;
    P->turns++;
    P->ptime++;

    /* "if you pick him up, you better put him down" */
    if (carrying(C_FIGURE)) {
        P->figureturns++;
        if (P->figureturns == 6)
            sayl("The man-like figure is struggling.  You were told to put him down.");
        if (P->figureturns > 12) {
            sayl("The figure goes limp in your hand.  Somewhere a long way off,");
            sayl("an adventurer stops walking.");
            W->it[C_FIGURE].state |= IS_DEAD;
            P->score -= 50;
            P->figureturns = 0;
        }
    }

    /* the Shadow's nightfall */
    if ((P->flags & PF_SHADOWED) && P->nightfall > 0) {
        P->nightfall--;
        if (P->nightfall == 5) sayl("The light is going.");
        if (P->nightfall == 0 && !(rooms[P->loc].flags & RF_SURFACE)) {
            sayl("Nightfall.  You did not make it to the surface.");
            sethp(P, 0);
        }
    }
    if (P->flags & PF_CURSED) sethp(P, hpleft(P) - 1);

    /* "Grue - Bad, Hungry * Eats things in the dark * Afraid of light" */
    if (!is_lit()) {
        P->darkturns++;
        if (P->darkturns == 1)
            sayl("Something is moving in the dark, and it is hungry.");
        else if (rnd(100) < 25) {
            sayl("Something with no name eats you in the dark.");
            sethp(P, 0);
        }
    } else P->darkturns = 0;

    /* "Truck, Delivery - ... If caught with truck, may be arrested & jailed." */
    if (carrying(O_TRUCK) && !(P->flags & PF_PRISONER) && rnd(100) < 4) {
        arrest("being caught with the truck");
        return;
    }

    /* the Wicked Witch "Captures Dogs, little boys and girls" */
    if (W->it[C_WITCH].loc == P->loc && !(W->it[C_WITCH].state & IS_DEAD)
        && !(W->it[C_WITCH].state & IS_TIED) && rnd(100) < 12) {
        sayl("The Wicked Witch \"Turns you into things\".  For a moment you");
        sayl("are a frog.  It wears off.  Your CHARISMA does not recover.");
        a_put(&P->at[AT_PRM], AT_CHA,
              ATTR(P, AT_PRM, AT_CHA) > 0 ? ATTR(P, AT_PRM, AT_CHA) - 1 : 0);
    }

    /* "Dwarf - Bad, Unfriendly, Liar * Thief" */
    if (W->it[C_DWARF].loc == P->loc && !(W->it[C_DWARF].state & IS_DEAD)
        && rnd(100) < 10) {
        int o;
        for (o = 1; o < NOBJ; o++)
            if (W->it[o].carrier == ME && (objdef[o].flags & OF_TREASURE)) {
                put_in_room(o, R_SKULL);
                printf("A Dwarf snatches %s and is gone." CRLF, objdef[o].shortname);
                break;
            }
    }

    if (hpleft(P) <= 0 && P->state == PS_ACTIVE) {
        char m[MSGLEN];
        sayl("");
        sayl("You have died.");
        P->deaths++;
        P->score -= 25;
        sprintf(m, "%s has died.", P->name);
        broadcast(m, ME);
        if (P->flags & PF_CURSED) {
            sayl("The Shadow had you twice.  There is no coming back from that.");
            P->state = PS_DEAD;
        } else {
            int o;
            for (o = 1; o < NOBJ; o++)
                if (W->it[o].carrier == ME) put_in_room(o, P->loc);
            P->flags &= ~(PF_SHADOWED | PF_PRISONER | PF_ONTRIAL);
            a_put(&P->at[AT_HTS], AT_HPL, ATTR(P, AT_HTS, AT_HIT));
            sayl("You wake up on the bank of the Amazon River, with nothing.");
            P->loc = R_BANK;
        }
    }
}

/* ================================================================ *
 *  Joining
 * ================================================================ */
int game_join(World *w, const char *name, const char *code)
{
    int i, slot = -1;
    for (i = 0; i < MAXPLAYERS; i++)
        if (w->pl[i].state != PS_FREE && !strcmp(w->pl[i].name, name))
            { slot = i; break; }
    if (slot < 0)
        for (i = 0; i < MAXPLAYERS; i++)
            if (w->pl[i].state == PS_FREE) { slot = i; break; }
    if (slot < 0) return -1;

    if (w->pl[slot].state == PS_FREE) {
        Player *p = &w->pl[slot];
        unsigned s = (unsigned)time(NULL) ^ (unsigned)(slot * 7919);
        memset(p, 0, sizeof *p);
        p->state = PS_ACTIVE;
        strncpy(p->name, name, NAMELEN - 1);
        strncpy(p->code, code, CODELEN - 1);
        p->ppn = XWD(0100000, 0200 + slot);       /* PPN (#100000200) */
        p->loc = R_BANK;
        p->prevloc = R_BANK;
        p->riddle = -1;
        p->lastmsg = w->msgseq;

        /* roll the AMAZON.OFF attribute words: 3d6-ish in 5-bit fields */
        {
            int k;
            int v[9];
            for (k = 0; k < 9; k++) {
                s = s * 1664525u + 1013904223u;
                v[k] = 8 + (int)((s >> 9) % 10);
            }
            a_put(&p->at[AT_PRM], AT_STR, v[0]);
            a_put(&p->at[AT_PRM], AT_INT, v[1]);
            a_put(&p->at[AT_PRM], AT_WIS, v[2]);
            a_put(&p->at[AT_PRM], AT_CHA, v[3]);
            a_put(&p->at[AT_PRM], AT_FLT, v[4]);
            a_put(&p->at[AT_PHY], AT_CON, v[5]);
            a_put(&p->at[AT_PHY], AT_SIZ, 14);
            a_put(&p->at[AT_PHY], AT_AGI, v[6]);
            a_put(&p->at[AT_PHY], AT_DEX, v[7]);
            a_put(&p->at[AT_HTS], AT_DIE, 3);
            a_put(&p->at[AT_HTS], AT_HIT, 24 + v[8]);
            a_put(&p->at[AT_HTS], AT_HPL, 24 + v[8]);
            p->at[AT_LNG] = 1;                 /* one language: your own */
        }
        p->ptype = P_HUMA;
    } else {
        w->pl[slot].state = PS_ACTIVE;
    }
    return slot;
}

/* ================================================================ *
 *  The main command loop -- AMAZON.SAI's shape
 * ================================================================ */
static void dispatch(void);

void game_loop(int me)
{
    char line[512];

    ME = me;
    W = world_lock();
    if (!W) return;
    do_help();
    do_source();
    enter_room(W->pl[ME].loc);
    world_unlock();

    while (!quitting) {
        int more = 0;

        /* AMAZON.SAI: if any!more = 0 then outstr(CR&LF&">"); */
        say(CRLF ">");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;

        W = world_lock();
        if (!W) break;
        if (W->pl[ME].state == PS_DEAD) {
            if (W->pl[ME].flags & PF_BANNED) {
                sayl("You have been Executed.  Neither you nor your cohorts are");
                sayl("allowed back into the amazon valley until Tomorrow...");
            } else {
                sayl("You are dead, and the Amazon has closed over you.");
            }
            world_unlock();
            break;
        }
        drain_messages();
        cmd_reset(&CL, line);
        (void)more;
        dispatch();
        if (!quitting && W->pl[ME].state == PS_ACTIVE) tick();
        drain_messages();
        world_unlock();
    }

    W = world_lock();
    if (W) {
        if (W->pl[ME].state == PS_ACTIVE) {
            /*  Whatever you were carrying stays in the valley for the
             *  other adventurers -- otherwise it would leave with you and
             *  no one could ever finish the game.                      */
            int o;
            char m[MSGLEN];
            for (o = 1; o < NOBJ; o++)
                if (W->it[o].carrier == ME) put_in_room(o, W->pl[ME].loc);
            snprintf(m, sizeof m, "%s has left the valley.", W->pl[ME].name);
            broadcast(m, ME);
            W->pl[ME].state = PS_FREE;
        }
        world_unlock();
    }
}

static void dispatch(void)
{
    char word[32], noun[32];
    int  more, cls, strength, ignoble;

    if (!getcmd(&CL, word, &more)) return;

    do {
        ignoble = -1;                        /* AMAZON.SAI: ignoble!command */

        if (!lookup_command(word, &cls, &strength)) {
            /* a bare noun after a verb is handled inside the verbs */
            printf(CRLF "What?" CRLF);
            return;
        }

        switch (cls) {
        case VC_DIRECTION:
            ignoble = 0;
            do_move(strength);
            break;

        case VC_ACQUIRE:
            ignoble = 0;
            /* AMAZON.SAI ACQUIRE, verbatim structure */
            if (CL.cur >= CL.ntok) printf("%s WHAT?" CRLF, word);
            while (CL.cur < CL.ntok) {
                getcmd(&CL, noun, &more);
                if (noun[0]) try_take(noun, strength);
            }
            break;

        case VC_RELINQUISH:
            ignoble = 0;
            if (CL.cur >= CL.ntok) printf("%s WHAT?" CRLF, word);
            while (CL.cur < CL.ntok) {
                getcmd(&CL, noun, &more);
                if (noun[0]) try_drop(noun, strength);
            }
            break;

        case VC_OTHER:
            ignoble = 0;
            noun[0] = 0;
            switch (strength) {
            case V_LOOK:      describe_room(0); break;
            case V_INVENTORY: do_inventory(); break;
            case V_SCORE:     do_score(); break;
            case V_STATS:     do_stats(); break;
            case V_WHO:       do_who(); break;
            case V_TELL:      do_tell(); break;
            case V_HELP:      do_help(); break;
            case V_SOURCE:    do_source(); break;
            case V_QUIT:      quitting = 1; sayl("Goodbye."); break;
            case V_WAIT:      sayl("Time passes."); break;
            case V_SLEEP:     do_sleep(); break;
            case V_LAUNCH:    do_launch(); break;
            case V_HINT:      do_hint(); break;
            case V_PLEAD:     do_plead(); break;
            case V_ORDER:
                if (P->loc == R_BAR) bar_ask();
                else sayl("There is no bar here.");
                break;
            case V_STUDY:
                if (carrying(O_LAWBOOK)) {
                    sayl("You study the Law Book.  The examination is on the law");
                    sayl("and the lore of the valley: who arrests, who defends,");
                    sayl("who judges, and who owns what.");
                    a_put(&P->at[AT_PRM], AT_WIS,
                          ATTR(P, AT_PRM, AT_WIS) < 31
                          ? ATTR(P, AT_PRM, AT_WIS) + 1 : 31);
                } else sayl("You have nothing to study.");
                break;
            case V_GO:
                getcmd(&CL, noun, &more);
                if (!noun[0]) sayl("Go where?");
                else {
                    int c2, s2;
                    if (lookup_command(noun, &c2, &s2) && c2 == VC_DIRECTION)
                        do_move(s2);
                    else sayl("Go where?");
                }
                break;
            default:
                getcmd(&CL, noun, &more);
                switch (strength) {
                case V_READ:    do_read(noun); break;
                case V_ANSWER:  if (noun[0]) do_answer(noun); else sayl("Answer what?"); break;
                case V_SAY:     if (noun[0]) do_say(noun); else sayl("Say what?"); break;
                case V_OPEN:    if (noun[0]) do_open(noun); else sayl("Open what?"); break;
                case V_UNLOCK:  if (noun[0]) do_unlock(noun); else sayl("Unlock what?"); break;
                case V_EXAMINE: if (noun[0]) do_examine(noun); else describe_room(0); break;
                case V_ATTACK:
                case V_KILL:    if (noun[0]) do_attack(noun, 1); else sayl("Attack what?"); break;
                case V_KISS:    if (noun[0]) do_kiss(noun); else sayl("Kiss what?"); break;
                case V_EAT:     if (noun[0]) do_eat(noun); else sayl("Eat what?"); break;
                case V_DRINK:   if (noun[0]) do_drink(noun); else sayl("Drink what?"); break;
                case V_RIDE:    if (noun[0]) do_ride(noun); else sayl("Ride what?"); break;
                case V_BUY:     if (noun[0]) do_buy(noun); else sayl("Buy what?"); break;
                case V_DELIVER: if (noun[0]) do_deliver(noun); else sayl("Return what?"); break;
                case V_TIE:     if (noun[0]) do_tie(noun); else sayl("Tie what?"); break;
                case V_LIGHT:   if (noun[0]) do_light(noun, 1); else sayl("Light what?"); break;
                case V_TURNOFF: if (noun[0]) do_light(noun, 0); else sayl("Put out what?"); break;
                case V_CLOSE:
                    if (noun[0]) {
                        int o = obj_by_word(noun);
                        if (o && obj_here(o)) {
                            W->it[o].state &= ~IS_OPEN;
                            printf("%s is closed." CRLF, objdef[o].shortname);
                        } else printf("I see no %s here." CRLF, noun);
                    } else sayl("Close what?");
                    break;
                case V_PUT:
                    if (noun[0]) try_drop(noun, 1);
                    else sayl("Put what?");
                    break;
                case V_CLIMB: case V_SWIM: case V_FLY:
                    sayl("Not that way.");
                    break;
                default:
                    printf(CRLF "What?" CRLF);
                    break;
                }
                break;
            }
            break;

        default:
            break;
        }

        if (ignoble) printf(CRLF "What?" CRLF);

    } while (0);
}
