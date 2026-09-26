#include "sd_def.h"

/*
**      SD_GLB -- Global storage allocations for Stardrek
**              (c) Peter S. Langston 1976
*/

#ifdef NOEMPTY
static  char *glbwhat = "@(#)sd_glb.c	2.18  WITHOUT empty() 5/31/84 -- (c) psl 1976";
#else
static  char *glbwhat = "@(#)sd_glb.c	2.18  WITH empty() 5/31/84 -- (c) psl 1976";
#endif
static  char *glbh_sccs = H_SCCS;

char    *sd_mother  = "psl";        /* mailing name for new member notices */
				    /* set = 0 if no mail is to be sent */
int     privuid     = 257;          /* uid of priveledged user */
int     maxusers    = MAXUSERS;     /* maximum # of starfleet members */

double  repair_time = 300.;         /* seconds per % of damage */

int     uid;                        /* uid of current user */
int     active[NUM_OBJECTS], recnum, belsup;
int     xrots, yrots, zrots;
double	pos[NUM_OBJECTS][4], vel[NUM_OBJECTS][4], *p0, *v0, *dp;
double  energy, sensors, shields, range, etime, time0, ccomp;
double  uobl;
double	damage[12], perdam[12], comarg;
char    *systems[]   = {
	"damage control",	"di-lithium crystal",	"warp drive",
	"impulse drive",	"sensors",		"life support",
	"auto-pilot",		"bridge monitor",	"main phaser bank",
	"antimatter generator",	"tractor pressor",	"shield generators"
};
int     damcntl = 0,            reactor = 1,            warpd   = 2,
	impulsd = 3,            sensrs  = 4,            lifsup  = 5,
	autop   = 6,            vismon  = 7,            phaser  = 8,
	photorp = 9,            tpgen   = 10,           shieldg = 11;
int     enemy, kills, hcap, rebirth, lsup;
int     targ, badnubie, autopi, junk[40];
int     helm, recf, cumf, msgf;
char	*sorm, *title;
char    dispsbf[128];

char    *shellprog          = "/bin/sh";                             /*PATH*/

	/* this macro sets many paths */
#include	"../gamesdef.h"
#define SDPATH(x)	GAMESPATH(SD/x)                              /*PATH*/

#ifdef  NOEMPTY
char    *hlm                = "sd_tmp";     /* temp file used by helm proc */
char    *helm_prog          = SDPATH(helm);
int     helmp;                                         /* pid of helm proc */
#endif

char    *cum                = SDPATH(sd_cum);
char    *records            = SDPATH(sd_rec);
char    *thank_you_letter   = SDPATH(sd_10q);

char    *main_path          = SDPATH();     /* where `main's live */
char    *log_prog           = SDPATH(log);

char    *msgfil             = SDPATH(sd_message);
char    *infofil            = SDPATH(sd_txt);

    /* SterFleet ranks */
char	*ranks[]	= {
	"Recruit", "Cadet", "Captain", "Admiral",
};

    /* names of races */
char    *race[] = {
	"Human",
	"Nubian", "Klingon", "Romulan",
	"Vallician", "Rigellian",
	"torpedo", "Starbase", "star", "Blish",
};
    /* mapping between ship numbers and race */
char    rac[]   = { /* note: things like S_BASE_NUM in sd_def.h must agree */
	R_HUMAN,
	R_NUBIAN, R_NUBIAN, R_NUBIAN, R_NUBIAN, R_NUBIAN,
	R_KLINGON, R_KLINGON, R_KLINGON, R_KLINGON, R_KLINGON,
	R_ROMULAN, R_ROMULAN, R_ROMULAN, R_ROMULAN, R_ROMULAN,
	R_VALLICIAN, R_VALLICIAN, R_VALLICIAN, R_VALLICIAN, R_VALLICIAN,
	R_RIGELLIAN,
	R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO,
	R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO,
	R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO,
	R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO, R_TORPEDO,
	R_STARBASE,
	R_STAR, R_STAR, R_STAR, R_STAR, R_STAR,
	R_STAR, R_STAR, R_STAR, R_STAR, R_STAR,
	R_STAR, R_STAR, R_STAR, R_STAR, R_STAR,
	R_STAR, R_STAR, R_STAR, R_STAR,
	R_PLANET,
};

    /* ship characteristics for various `races' */
struct  scstr   sc[]    = {
/*phas  torp  trac shld pvul tvul acc wrp  vis  attd   mass radius */
  100,  100,  100,  100, 100, 100, 50,100,  140, 999,    100, 30,  /* human */
    0,    0,    0,   50, 100, 100, 10,  0,  200,   0,    200, 50,  /* nube */
  100,   10,  100,  100,  75, 150, 20,  0,  140,  70,    100, 30,  /* kling */
   10,  160,    0,   40, 150,  75, 10,100,  100,  55,     50, 15,  /* rom */
   80,   40,   60,  200,  80, 120, 15,  0,  160,  80,     75, 20,  /* vall */
   80,   33,   33,  500,  10, 300, 20,  0,  160,  50,    300, 50,  /* rigel */
    0,    0,    0,    0, 100, 100,  0,  0,   50,   0,     10,  1,  /* torp */
  100,    0,    0, 1000,  50,  50,  0,  0,  600,   0,  10000,100,  /* s.b. */
    0,    0,    0,    0,   0,   0,  0,  0,  999,   0,  32767,200,  /* stars */
    0,    0,    0,    0,  10,  10,  0,  0,  300,   0,  10000,150,  /* planet */
};

struct	record	rec;
struct	score	this;
