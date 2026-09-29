/*      ST_DEF -- External definitions for star-drek
**     (c) P.Langston 1976
*/

#include        <stdio.h>

/* compile with  -DNOEMPTY for systems with no empty(fh) systems call */

#define H_SCCS  "@(#)sd_def.h	2.10 4/8/84 -- (c) psl 1976"

				/* signals to helm process if needed */
#define IGN_SIG         13      /* broken pipe seemed the most appropriate */
#define ALARM_SIG       14      /* as did alarm */

#define MAXUSERS        10      /* sets maxusers in sd_glb.c */

#define MAX_O           18      /* number of elements in o[] (see dials()) */

#define	R_HUMAN		0
#define	R_NUBIAN	1
#define	R_KLINGON	2
#define	R_ROMULAN	3
#define	R_VALLICIAN	4
#define R_RIGELLIAN     5
#define R_TORPEDO       6
#define R_STARBASE      7
#define R_STAR          8
#define R_PLANET        9
		 /* these must agree with rac[] in sd_glb.c */
#define NUM_SHIPS       21                        /* not counting our ship */
#define NUM_TORPS       20
#define NUM_STARS       19
#define NUM_PLANETS     1                  /* much depends on this being 1 */
#define NUM_OBJECTS     (1 + NUM_SHIPS + NUM_TORPS + 1 + NUM_STARS + NUM_PLANETS)
#define FIRST_SHIP      1
#define LAST_SHIP       (FIRST_SHIP + NUM_SHIPS)     /* all "LAST"s are +1 */
#define FIRST_TORP      LAST_SHIP
#define LAST_TORP       (FIRST_TORP + NUM_TORPS)
#define S_BASE_NUM      LAST_TORP
#define FIRST_STAR      (S_BASE_NUM + 1)
#define LAST_STAR       (FIRST_STAR + NUM_STARS)
#define PLANET_NUM      LAST_STAR
#define LAST_OBJECT     (PLANET_NUM + 1)

#define NUM_SYSTEMS     12           /* number of systems in Scotty's care */

#define	PHASER_ANGLE	.1
#define	TRACTOR_ANGLE	.2

#define ANGLE_INCREMENT 13          /* used for cursor-motion rotations */

/* flags codes */
#define	USR_MALE	000000
#define	USR_FEMALE	000001
#define	USR_RANK	000030
#define	USR_RECRUIT	000000			  /* not in StarFleet, yet */
#define	USR_CADET	000010
#define	USR_CAPTAIN	000020
#define	USR_ADMIRAL	000030
#define USR_MARRIED     000100
#define USR_AT_SB       000200             /* on planet if not at Starbase */

#define	RANK(x)	(ranks[(x & USR_RANK) >> 3])

/* The instruments, (must agree with def of dialinfo in *viz.c) */
#define ALL_DIALS       0
#define MISS_ANG_DIAL   1
#define MISS_DST_DIAL   2
#define ENERGY_DIAL     3
#define ETIME_DIAL      4
#define SB_ANG_DIAL     5
#define SB_DST_DIAL     6
#define RANGE_DIAL      7
#define SENSORS_DIAL    8
#define VEL_ANG_DIAL    9
#define VEL_DST_DIAL    10
#define SPEED_DIAL      11
#define CONDITION_DIAL  12
#define ROTATION_DIAL   13
#define MAX_LABELLED_DIAL       13
#define AUTOPILOT_DIAL  14
#define CCC_DIAL        15
#define SHIELDS_DIAL    16
#define MAX_DIALS       16

	/* special character definitions, used by sd_com & helm */
#define CHAR_MODE       0
#define FILE_MODE       1
#define LINE_MODE       2
#define FUNC_MODE       3

	/* the commands */
#define cSOS            1
#define cDAM            2
#define cTHRST          3
#define cCCC            4
#define cTORP           5
#define cPHAZ           7
#define cXROT           8
#define cPXROT          9
#define cMXROT          10
#define cYROT           11
#define cPYROT          12
#define cMYROT          13
#define cZROT           14
#define cAUTO           15
#define cDOCK           16
#define cWARP           17
#define cTRAC           18
#define cSHLD           19
#define cISTOP          20
#define cSALV           21
#define cSCORE          22
#define cSHELL          23
#define cRFRSH          24
#define cIGNOR          25
#define cNOOP		26

struct	score {
	int	miss;
	int	nube;
	int	kling;
	int	rom;
	int	vall;
	int     rigel;
	int	salv;
	float	points;
	float	etim;
};

struct	record {
	int	usr_uid;
	long	usr_last;
	int	usr_ships;
	int	usr_flags;
	char    usr_fname[16];
	char    usr_lname[24];
	char    usr_bmonth;
	char    usr_bday;
	char    usr_byear;
	char    usr_weight;                                       /* in kg */
	char    usr_eyes[8];
	char    usr_hair[8];
	char    usr_shoe[8];
	char    usr_color[12];
	int     usr_height;                                       /* in cm */
	int     usr_iq;
	struct	score	usr_score;
	long	shp_ready;
	char	shp_name[16];
	struct	score	shp_score;
};

struct  cumstr  {
	int     cum_miss;           /* total missions flown */
	int     cum_kills;          /* total enemy kills */
	int     cum_lost;           /* total starfleet losses */
	long    cum_sbdate;         /* date of last Starbase destruction */
	long    cum_pldate;         /* date of last planet destruction */
	short   cum_sbuid;          /* person responsible for sb dest */
	short   cum_pluid;          /* person responsible for pl dest */
};

struct  scstr   {           /* ship characteristics */
	int     sc_phas;            /* phaser power */
	int     sc_torp;            /* torpedo size */
	int     sc_trac;            /* tractor power */
	int     sc_shld;            /* shield power */
	int     sc_pvul;            /* phaser vulnerability */
	int     sc_tvul;            /* torp vulnerability */
	int     sc_acc;             /* max acceleration * 10 per GTU */
	int     sc_warp;            /* max warp * 10 */
	int     sc_visd;            /* visual distance */
	int     sc_attd;            /* attack distance */
	int     sc_mass;            /* mass of ship */
	int     sc_rad;             /* radius of ship in .1 GDUs */
};

struct  funkstr {
	char    f_in[7];                    /* minus the common first char */
	char    f_token;      /* version to go in com_map[] if single char */
	char    f_out[8];   /* version to substitute in input if multichar */
};

	/* function definitions */

#define DISP0(f)            display(1,f)
#define DISP1(f,a)          display(1,dispsbf,sprintf(dispsbf,f,a))
#define DISP2(f,a,b)        display(1,dispsbf,sprintf(dispsbf,f,a,b))
#define DISP3(f,a,b,c)      display(1,dispsbf,sprintf(dispsbf,f,a,b,c))

#define DISQ0(f)            display(0,f)
#define DISQ1(f,a)          display(0,dispsbf,sprintf(dispsbf,f,a))
#define DISQ2(f,a,b)        display(0,dispsbf,sprintf(dispsbf,f,a,b))
#define DISQ3(f,a,b,c)      display(0,dispsbf,sprintf(dispsbf,f,a,b,c))

#define DISA0(f)            disapp(1,f)
#define DISA1(f,a)          disapp(1,dispsbf,sprintf(dispsbf,f,a))
#define DISA2(f,a,b)        disapp(1,dispsbf,sprintf(dispsbf,f,a,b))
#define DISA3(f,a,b,c)      disapp(1,dispsbf,sprintf(dispsbf,f,a,b,c))

#define DISB0(f)            disapp(0,f)
#define DISB1(f,a)          disapp(0,dispsbf,sprintf(dispsbf,f,a))
#define DISB2(f,a,b)        disapp(0,dispsbf,sprintf(dispsbf,f,a,b))
#define DISB3(f,a,b,c)      disapp(0,dispsbf,sprintf(dispsbf,f,a,b,c))
#define DISB4(f,a,b,c,d)    disapp(0,dispsbf,sprintf(dispsbf,f,a,b,c,d))

#define min(a,b)    ((a) < (b)? (a) : (b))
#define max(a,b)    ((a) > (b)? (a) : (b))
#define dmin(a,b)   ((a) < (b)? (a) : (b))
#define dmax(a,b)   ((a) > (b)? (a) : (b))
#define dabs(a)     ((a) < 0? -(a) : (a))

	/* external definitions for stuff in sd_glb.c and *viz.c */

extern  char    *cum, *records, *thank_you_letter, *msgfil, *infofil;
extern  char    *ranks[], *race[], rac[], *sorm, *title, *systems[];
extern  char    *main_path, *log_prog, *shellprog;
extern  char    *sd_mother;
extern  char    com_map[128], dispsbf[];
extern  char    bs_char, cr_char, fname_char, fexec_char, func_char, line_char;
extern  char    list_char, mdef_char, mexec_altc, mexec_char, wait_char;
extern  int     privuid, maxusers, uid;
extern	int	autop, vismon, phaser, photorp, tpgen, shieldg;
extern	int	damcntl, reactor, warpd, impulsd, sensrs, lifsup;
extern  int     active[NUM_OBJECTS], recnum, belsup;
extern  int     enemy, kills, hcap, rebirth, lsup;
extern  int     helm, recf, urecf, cumf, msgf;
extern  int     nearshp, neardst, targ, badnubie, autopi, junk[];
extern  int     xrots, yrots, zrots;
extern  double  damage[12], perdam[12], comarg, repair_time, uobl;
extern	double	pos[NUM_OBJECTS][4], vel[NUM_OBJECTS][4], *p0, *v0, *dp;
extern  double  energy, sensors, shields, range, etime, time0, ccomp;
extern	struct	record	rec;
extern	struct	score	this;
extern  struct  funkstr funkey[];
extern  struct  scstr   sc[];

extern  char    *copy();
extern  double  dist(), distsq(), dsq(), eff(), extrapolate();
extern  double  size(), sqrt(), zrnd();
