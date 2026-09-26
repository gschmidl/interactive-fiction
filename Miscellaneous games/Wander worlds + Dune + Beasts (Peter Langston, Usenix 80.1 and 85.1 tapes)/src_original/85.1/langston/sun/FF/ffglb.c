#include        "ffdef.h"
/*
**      FFGLB -- Global storage allocations for Fast Food
*/
static  char    *sccsid = "@(#)ffglb.c	1.1 10/20/84 -- (c) psl 1977";
static  char    *defid  = DEF_SCCS;

char    *infols         = "ls %s";

char    *nroffil	= "/usr/bin/nroff";

char	*bozo_msg	=
	"Never heard of you.\nIf you'd like to play send mail to %s (%s).\n";
		/* This message output when someone fails to get into ff */
		/* The two %s are replaced by prvlog & prvnam */
char	*prvlog		= PRVLOG;		/* PRVLOG from ../gamesdef.h */
char	*prvnam		= PRVNAM;		/* PRVNAM from ../gamesdef.h */

char	*mailname	= "Fast!Food";		/* sender for "w" messages */

#define	FFPATH(x)	GAMESPATH(FF/x)		/* also from ../gamesdef.h */
char    *upd_prog	= FFPATH(update);
char    *nroffhd	= FFPATH(info.mac);
char    *infodir	= FFPATH(INFO);


char	*data_dir	= GAMESPATH(FF);	/* default data file dir. */

/* START OF FILES IN data_dir */
char    *plyrfil	= "plyr.ff";
char    *boardfil	= "board.ff";
char    *chainfil	= "chain.ff";
char    *miscfil	= "misc.ff";
char    *offerfil	= "offer.ff";
char    *newsfil	= "news.ff";
char    *onwsfil	= "onews.ff";
char    *histfil	= "hist.ff";
char    *jrnlfil	= "journal.ff";
char    *ojrnfil	= "ojournal.ff";
char    *upfil		= "upfile";

struct	lockstr	lck[]	= {
        { ".unode", "ulock.ff",	"waiting on update lock", 60, 99, 2, 0 },
        { ".pnode", "plock.ff",	"waiting on player lock", 60, 99, 2, 0 },
        { ".bnode", "block.ff",	"waiting on board lock",  60, 99, 2, 0 },
        { ".cnode", "clock.ff",	"waiting on chain lock",  60, 99, 2, 0 },
        { ".onode", "olock.ff",	"waiting on offer lock",  60, 99, 2, 0 },
        { ".mnode", "mlock.ff",	"waiting on misc lock",   60, 99, 2, 0 },
};

/* END OF FILES IN data_dir */

char    board[LIM_X_SIZE][LIM_Y_SIZE], textbuf[1024];
int     pnum, mfh, bfh, pfh, cfh, ofh;

struct  player  plyr;
struct  chain   chain;
struct  misc    misc;
struct  holder  hldr[LIM_PLAYERS+1];
