/*
**      EMPGLB -- Global definitions for Empire
**
** Copyright (c) 1976 by Peter S. Langston - Camb., Ma. 02140
*/

#define D_NATSTR
#define D_SCTSTR
#define D_SECTDES
#define D_SHPSTR
#define D_SHIPTYP
#define D_COMSTR
#define D_DCHRSTR
#define D_ICHRSTR
#define D_NWSSTR
#define D_RPTSTR
#define D_MCHRSTR
#define D_TELSTR
#define D_TRTSTR
#define D_TCHRSTR
#define D_LONSTR
#define D_NSCSTR
#define D_TRTYCLAUSE
#define D_NEWSVERBS
#define	D_NUKSTR
#define D_VARSTR
#define	D_PCHRSTR
#define	D_VCHRSTR
#define D_VTYPES
#include "empdef.h"

#include	"../gamesdef.h"
#define	EMPPATH(xxx)	GAMESPATH(EMP/xxx)


char    *emprog[]   = {                             /* empire module names */
	"/u1/games/empire",                      /* main entry into empire */
	EMPPATH(BIN/emp1),
	EMPPATH(BIN/emp2),
	EMPPATH(BIN/emp3),
	EMPPATH(BIN/emp4),
	EMPPATH(BIN/emp5),
	EMPPATH(BIN/emp6),
	EMPPATH(BIN/emp7),
};
char    *empfix     = EMPPATH(BIN/empfix);                   /* file fixer */
char    *empsrc     = EMPPATH(.);                 /* location of "sources" */
char    *infodir    = EMPPATH(INFO);                    /* info topics dir */
char    *datadir    = EMPPATH(DATA);          /* where the data files live */

		/* data file names */
char    *upfil      = EMPPATH(DATA/empup);		   /* enables play */
char    *downfil    = EMPPATH(DATA/empdown);		 /* prohibits play */
char    *holfil     = EMPPATH(DATA/emphol);		     /* "holidays" */
char    *sectfil    = EMPPATH(DATA/empsect);		    /* sector data */
char    *natfil     = EMPPATH(DATA/empnat);		    /* nation data */
char    *newsfil    = EMPPATH(DATA/empnews);	       /* recent news data */
char    *loanfil    = EMPPATH(DATA/emploan);		      /* loan data */
char    *shipfil    = EMPPATH(DATA/empship);		      /* ship data */
char    *telfil     = EMPPATH(DATA/emptel);   /* prefix for tel file names */
char    *powfil     = EMPPATH(DATA/emppow);	 /* last power report data */
char    *treatfil   = EMPPATH(DATA/emptreat);		    /* treaty data */
char	*nukfil	    = EMPPATH(DATA/empnuke);		      /* nuke data */
		/* global file handle variables */
int     sectf, natf, newsf, loanf, infof, shipf, telf, powf, trtf, nukf;


/* some people prefer to pre-nroff the info files and specify a pager here */
#define	MAXNRARG	5			    /* max value of nnrarg */
char    *nroffil    = "/usr/bin/nroff";    /* where nroff lives (for info) */
int	nnrarg	    = 3;	    /* how many nroff args we provide here */
char	*nrarg[MAXNRARG + 1]    = {		 /* args to nroff or pager */
	    "-s",				   /* wait after each page */
	    EMPPATH(INFO/CRT.MAC),			     /* crt macros */
	    EMPPATH(INFO/INFO.MAC),			    /* misc macros */
			      /* nrarg[nnrarg] will contain info file name */
};

	/* NOTE: to disallow subshells set *shllpath = 0; */
char    *shllpath   = "/bin/sh";      /* path to shell for "shell" command */
char    *shllnam    = "Empshell";      /* the zeroth argument to the shell */
char    *shllrg1    = "-";            /* first optional argument for shell */
char    *shllrg2    = 0;             /* second optional argument for shell */
char    *shllrg3    = 0;              /* third optional argument for shell */

		/* parametric goodies */
	/* NOTE: PRVNAM, PRVLOG, & PRVUID come from ../gamesdef.h */
char    *privname   = PRVNAM;			      /* name of priv user */
char    *privlog    = PRVLOG;			   /* logname of priv user */
char    l_cvt       = 'l';		     /* 'l' => printf("%ld", long) */
					      /* '-' => printf("%D", long) */
int	privuid     = PRVUID;			 /* uid of owner of Empire */
int	w_xsize     = 128;		  /* world x size, (width), <= 128 */
int	l2w_xsize   = 7;		     /* log base 2 of world x size */
int	w_ysize     = 32;		 /* world y size, (height), <= 128 */
#define MAXNOC      16		  /* must not exceed MAX_MAXNOC (empdef.h) */
int	m_m_p_d     = 120;	 /* max mins of play per day (per country) */

int	maxnoc      = MAXNOC;			   /* max num of countries */
int	maxcno      = MAXNOC-1;			    /* largest country num */
int	sct_maxno   = SCT_MAXDEF;  /* the default number if no sects added */
int	shp_maxno   = SHP_MAXDEF;  /* the default number if no ships added */

/* Note: this constant is still built into many info files  */
/* set it to 10 for "Blitz Empire" */
long    s_p_etu   = 1800;                  /* seconds per Empire time unit */

/* Note: these constants are built into the "food" info file               */
/* "(dt * fert * wrk)" means the rate is multiplied by Empire time units,  */
/* fertility units and number of "workers" (civil + milit/5)               */
float   fgrate      = 0.002;               /* food growth rate (dt * fert) */
float   fcrate      = 0.002;       /* food cultivate rate (dt * workforce) */
float   eatrate     = 0.0005;            /* food eating rate (dt * people) */
float   babyeat     = 0.3;        /* food to mature 1 baby into a civilian */

float   ubrate      = 0.01;               /* urban birth rate (dt * civil) */
float   obrate      = 0.005;                   /* other sectors birth rate */
      /* values greater than 0.25 for either birth rate will give overflow */
float   bankint     = 0.125;             /* bank interest rate (dt * bars) */


/* D_P_ENLI enlist parameters */
double	enli_max	= 499.;		      /* max enlistment per sector */
double	enli_rate	= 0.5;	/* max enlistment rate, ignoring happiness */
double	enli_hfact	= 10.;	       /* happiness at which effect is 1/2 */
/* happiness effect is:   (happiness + 1.) / (happiness + enli_hfact + 1.) */
double	enli_btu	= 0.02;		 /* btu cost per civilian enlisted */
double	enli_happy	= 0.02;	   /* happiness cost per civilian enlisted */

/* D_P_POWE power report parameters */
double	powe_cost	= 10.;	    /* btu cost to do a "new" power report */

/* D_P_BUIL build parameters */
double	buil_bt		= 100.;	  /* tech level required to build a bridge */
int	buil_bh		= 200;		 /* hcm required to build a bridge */
double	buil_bc		= 3000.;	/* cash required to build a bridge */
double	buil_nt		= 300.;	    /* tech level required to build a nuke */
int	buil_nh		= 100;		   /* hcm required to build a nuke */
int	buil_nl		= 200;		   /* lcm required to build a nuke */
int	buil_nd		= 100;	     /* gold dust required to build a nuke */
int	buil_no		= 100;		   /* oil required to build a nuke */
double	buil_nc		= 10000.;	  /* cash required to build a nuke */


char    dirch[] = {                   /* agrees in order with DIR_ defines */
	'h',                                                 /* stop (end) */
	'u', 'j', 'n',                      /* up-right, right, down-right */
	'b', 'g', 'y',                         /* down-left, left, up-left */
	'v', '*',                                            /* view, bomb */
};



char	junk[80], combuf[80], *argp[16], *condarg;
int	wthr_xh, wthr_yh, wthr_xl, wthr_yl;
int	sx, sy, lx, hx, ly, hy, ix, iy;
int	capx, capy, nbrx, nbry;
int	capxof[MAXNOC], capyof[MAXNOC];
int	cnum, nbtu, nstat, ncomstat, nminused, btused;
int	god, broke;
int	owner, proto, redirin;
int	ttymod[3];
int	diroff[][2]	= {                     /* must agree with dirch[] */
	0,0, 1,-1, 2,0, 1,1, -1,1, -2,0, -1,-1, 0,0, 0,0,
};
int	defoff[]    = {                                 /* defense offsets */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	-16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
};
long    lasttime, curup;                   /* used in calc of minutes used */
long    wthr_date;

double  dolcost;
double  wthr_hi, wthr_lo;

extern	struct	comstr	coms[];

struct  boundstr    nrealm[MAXNOR];
struct  sctstr      sect;
struct  natstr      nat;
struct  nwsstr      nws;
struct  shpstr      ship, as, vs;
struct  telstr      tgm;
struct  trtstr      trty;
struct  lonstr      loan;

char	nulls[64];

/*	efficiency adverbs -- vaguely describe efficiency */
char    *effadv[]   = {
	"minimally", "partially", "moderately", "completely"
};

char    *numnames[NUMNUMNAMES]  = {
	"zero", "one", "two", "three", "four", "five", "six",
	"seven", "eight", "nine", "ten", "eleven", "twelve",
	"thirteen", "fourteen", "fifteen", "sixteen",
	"seventeen", "eighteen", "nineteen",
};

char    *tennames[NUMTENNAMES]  = {
	"", "", "twenty", "thirty", "fourty", "fifty",
	"sixty", "seventy", "eighty", "ninety",
};

/*    designation characteristics -- one entry per designation type
*/
struct  dchrstr dchr[]  = {
/*      mnem prd     mcst  flg    pkg ostr dstr value name    */
	'.', 0,        0, NAVOK,  NPKG,  0,   0,   0, "sea",
	'^', 0,      255, 0,      NPKG,  2,  16,  10, "mountain",
	's', 0,        0, 0,      NPKG,  0,  99, 127, "sanctuary",
	'\\',0,        0, 0,      NPKG,  0,   0,   0, "wasteland",
	'-', 0,        3, 0,      NPKG,  2,   2,   5, "wilderness",
	'c', 0,        2, 0,      NPKG,  2,   4, 127, "capital",
	'u', 0,        2, 0,      UPKG,  2,   2,  10, "urban area",
	'p', P_HLEV,   2, 0,      NPKG,  2,   2,  10, "park",
	'd', P_GUN,    2, 0,      NPKG,  2,   2,  10, "defense plant",
	'i', P_SHELL,  2, 0,      NPKG,  2,   2,  10, "shell industry",
	'm', P_IRON,   2, 0,      NPKG,  2,   2,  20, "mine",
	'g', P_DUST,   2, 0,      NPKG,  2,   2,  20, "gold mine",
	'h', 0,        2, NAV_02, NPKG,  2,   2,  20, "harbor",
	'w', 0,        2, 0,      WPKG,  2,   2,  10, "warehouse",
	'*', P_PLANE,  2, 0,      NPKG,  2,   2,  10, "airfield",
	'a', P_FOOD,   2, 0,      NPKG,  2,   2,  10, "agribusiness",
	'o', P_OIL,    2, 0,      NPKG,  2,   2,  20, "oil field",
	'j', P_LCM,    2, 0,      NPKG,  2,   2,  10, "light manufacturing",
	'k', P_HCM,    2, 0,      NPKG,  2,   2,  10, "heavy manufacturing",
	'f', 0,        2, 0,      NPKG,  4,   8,  20, "fortress",
	't', P_TLEV,   2, 0,      NPKG,  2,   2,   0, "technical center",
	'r', P_RLEV,   2, 0,      NPKG,  2,   2,  10, "research lab",
	'n', 0,        2, 0,      NPKG,  2,   2,  10, "nuclear plant",
	'l', P_ELEV,   2, 0,      NPKG,  2,   2,  10, "library/school",
	'+', 0,        1, 0,      NPKG,  2,   2,  10, "highway",
	')', 0,        2, 0,      NPKG,  2,   2,  20, "radar installation",
	'!', 0,        2, 0,      NPKG,  2,   2,  10, "weather station",
	'#', 0,        2, 0,      NPKG,  2,   2,  15, "bridge head",
	'=', 0,        1, NAV_60, NPKG,  1,   1,  15, "bridge span",
	'b', P_BAR,    2, 0,      NPKG,  2,   2,  10, "bank",
	0,   0,        2, 0,      NPKG,  2,   2,  10, "extra sector type",
	0,   0,        0, 0,      0,     0,   0,   0, 0,
};	/* if you add sects be sure to increase the definition of sct_maxno */

/*      item characteristics -- one entry per movable item
*/

struct  ichrstr ichr[]  = {
/*
mnem    vtype    del     sell bid val lbs rg,wh,ur,???  name */
'c', V_CIVIL, V_CDEL, V_CSELL,  1, 1,  1, {1, 1,10, 1},"civilians",
'm', V_MILIT, V_MDEL, V_MSELL,  1, 1,  1, {1, 1, 1, 1},"military",
's', V_SHELL, V_SDEL, V_SSELL,  3, 2,  2, {1,10, 1, 1},"shells",
'g',   V_GUN, V_GDEL, V_GSELL,  5,10, 10, {1,10, 1, 1},"guns",
'p', V_PLANE, V_PDEL, V_PSELL, 15,25, 20, {1, 1, 1, 1},"planes",
'i',  V_IRON, V_IDEL, V_ISELL,  2, 1,  1, {1,10, 1, 1},"iron ore",
'd',  V_DUST, V_DDEL, V_DSELL,  5,10,  5, {1,10, 1, 1},"dust (gold)",
'b',   V_BAR, V_BDEL, V_BSELL, 33,99, 50, {1, 1, 1, 1},"bars of gold",
'f',  V_FOOD, V_FDEL, V_FSELL,  3, 1,  1, {1,10, 1, 1},"food",
'o',   V_OIL, V_ODEL, V_OSELL,  2, 1,  1, {1,10, 1, 1},"oil",
'l',   V_LCM, V_LDEL, V_LSELL,  3, 2,  1, {1,10, 1, 1},"light products",
'h',   V_HCM, V_HDEL, V_HSELL,  3, 2,  1, {1,10, 1, 1},"heavy products",
  0,       0,      0,       0,  0, 0,  0, {0, 0, 0, 0},0,
};

#define OS(x)	((char) (&((struct sctstr *) 0) -> x))
#define SO(x)	((char) (&((struct shpstr *) 0) -> x))
struct  castr   ca[]    = {
	NSC_OFF | OS(sct_x),	"xloc",
	NSC_OFF | OS(sct_y),	"yloc",
	NSC_OFF | OS(sct_own),	"owner",
	NSC_OFF | OS(sct_type),	"designation",
	NSC_OFF | OS(sct_effic),"efficiency",
	NSC_OFF | OS(sct_mobil),"mobility",
	NSC_OFF | OS(sct_min),	"minerals",
	NSC_OFF | OS(sct_gmin),	"gold mineral",
	NSC_OFF | OS(sct_fertil),"fertility",
	NSC_OFF | OS(sct_oil),	"petroleum content",
	NSC_OFF | SO(shp_fleet),"fleet",
	NSC_VAR | V_CIVIL,	"civilians",
	NSC_VAR | V_MILIT,	"military",
	NSC_VAR | V_SHELL,	"shells",
	NSC_VAR | V_GUN,	"guns",
	NSC_VAR | V_PLANE,	"planes",
	NSC_VAR | V_IRON,	"iron ore",
	NSC_VAR | V_DUST,	"dust (gold)",
	NSC_VAR | V_BAR,	"bars of gold",
	NSC_VAR | V_FOOD,	"food",
	NSC_VAR | V_OIL,	"oil",
	NSC_VAR | V_LCM,	"lcm light const matls",
	NSC_VAR | V_HCM,	"hcm heavy const matls",
	NSC_VAR | V_CHKPT,	"checkpoint",
	NSC_VAR | V_CDEL,	"c_delivery",
	NSC_VAR | V_MDEL,	"m_delivery",
	NSC_VAR | V_SDEL,	"s_delivery",
	NSC_VAR | V_GDEL,	"g_delivery",
	NSC_VAR | V_PDEL,	"p_delivery",
	NSC_VAR | V_IDEL,	"i_delivery",
	NSC_VAR | V_DDEL,	"d_delivery",
	NSC_VAR | V_BDEL,	"b_delivery",
	NSC_VAR | V_FDEL,	"f_delivery",
	NSC_VAR | V_ODEL,	"o_delivery",
	NSC_VAR | V_LDEL,	"l_delivery",
	NSC_VAR | V_HDEL,	"h_delivery",
	NSC_VAR | V_CSELL,	"c_price",
	NSC_VAR | V_MSELL,	"m_price",
	NSC_VAR | V_SSELL,	"s_price",
	NSC_VAR | V_GSELL,	"g_price",
	NSC_VAR | V_PSELL,	"p_price",
	NSC_VAR | V_ISELL,	"i_price",
	NSC_VAR | V_DSELL,	"d_price",
	NSC_VAR | V_BSELL,	"b_price",
	NSC_VAR | V_FSELL,	"f_price",
	NSC_VAR | V_OSELL,	"o_price",
	NSC_VAR | V_LSELL,	"l_price",
	NSC_VAR | V_HSELL,	"h_price",
	0,                      0,
};

	/* variable characteristics -- one entry per V_ define */
	/* NOTE: if V_ variables are added make sure empfix.c is up to date */
struct  vchrstr vchr[]  = {
	0,          0,          /* unused */
	0,          0,          /* mine */
	0,          0,          /* chkpt */
	0,          0,          /* spare */
	0,          0,          /* pstage */
	0,          0,          /* ptime */
	I_CIVIL,    VCH_ITEM,   /* civil */
	I_MILIT,    VCH_ITEM,   /* milit */
	I_SHELL,    VCH_ITEM,   /* shell */
	I_GUN,      VCH_ITEM,   /* gun */
	I_PLANE,    VCH_ITEM,   /* plane */
	I_IRON,     VCH_ITEM,   /* iron */
	I_DUST,     VCH_ITEM,   /* dust */
	I_BAR,      VCH_ITEM,   /* bar */
	I_FOOD,     VCH_ITEM,   /* food */
	I_OIL,      VCH_ITEM,   /* oil */
	I_LCM,      VCH_ITEM,   /* lcm */
	I_HCM,      VCH_ITEM,   /* hcm */
	I_CIVIL,    VCH_DEL,    /* cdel */
	I_MILIT,    VCH_DEL,    /* mdel */
	I_SHELL,    VCH_DEL,    /* sdel */
	I_GUN,      VCH_DEL,    /* gdel */
	I_PLANE,    VCH_DEL,    /* pdel */
	I_IRON,     VCH_DEL,    /* idel */
	I_DUST,     VCH_DEL,    /* ddel */
	I_BAR,      VCH_DEL,    /* bdel */
	I_FOOD,     VCH_DEL,    /* fdel */
	I_OIL,      VCH_DEL,    /* odel */
	I_LCM,      VCH_DEL,    /* ldel */
	I_HCM,      VCH_DEL,    /* hdel */
	I_CIVIL,    VCH_PRICE,  /* csell */
	I_MILIT,    VCH_PRICE,  /* msell */
	I_SHELL,    VCH_PRICE,  /* ssell */
	I_GUN,      VCH_PRICE,  /* gsell */
	I_PLANE,    VCH_PRICE,  /* psell */
	I_IRON,     VCH_PRICE,  /* isell */
	I_DUST,     VCH_PRICE,  /* dsell */
	I_BAR,      VCH_PRICE,  /* bsell */
	I_FOOD,     VCH_PRICE,  /* fsell */
	I_OIL,      VCH_PRICE,  /* osell */
	I_LCM,      VCH_PRICE,  /* lsell */
	I_HCM,      VCH_PRICE,  /* hsell */
};


#define V(x,y)  {((y<<6)|x)}      /* for vstr initializations */

struct  pchrstr pchr[]  = {   /* order must agree with P_ defines */
	/* NOTE: V(V_xxx) entries must be in numerical order */
/*       level      cost    nrndx nrdep nlndx   nlmin nllag effic  name    */
0,       0,         0,      0,    0,    0,        0,    0,  0,     "unused",
    {0}, 0, {{0}},
V_SHELL, -1,        3,      0,    0,    NAT_TLEV, -10,  10, 100,   "shells",
    {0}, 2, { V(V_LCM,2), V(V_HCM,1), },
V_GUN,   -1,        15,     0,    0,    NAT_TLEV, -10,  10, 100,   "guns",
    {0}, 3, { V(V_OIL,1), V(V_LCM,5), V(V_HCM,10), },
V_PLANE, -1,        30,     0,    0,    NAT_TLEV, -10,  10, 100,   "planes",
    {0}, 3, { V(V_OIL,2), V(V_LCM,10), V(V_HCM,20), },
V_IRON,  -1,        1,OS(sct_min),0,    -1,       0,    0,  100,   "iron ore",
    {0}, 0, {{0}},
V_DUST,  -1,        1,OS(sct_gmin),50,  -1,       0,    0,  100,   "gold dust",
    {0}, 0, {{0}},
V_BAR,   -1,        5,      0,    0,    -1,       0,    0,  100,   "gold bars",
    {0}, 1, { V(V_DUST,5), },
V_FOOD,  -1,        1,OS(sct_fertil),0, NAT_ELEV, -10,  10, 1000,  "food",
    {0}, 0, {{0}},
V_OIL,   -1,        1,OS(sct_oil),20,   NAT_TLEV, -10,  10, 100,   "oil",
    {0}, 0, {{0}},
V_LCM,   -1,        1,      0,    0,    NAT_TLEV, -10,  10, 100,   "light construction materials",
    {0}, 1, { V(V_IRON,1), },
V_HCM,   -1,        2,      0,    0,    NAT_TLEV, -10,  10, 100,   "heavy construction materials",
    {0}, 1, { V(V_IRON,2), },
0,       NAT_TLEV,  25,     0,    0,    NAT_ELEV, 0,    50, 100,   "technologocial breakthroughs",
    {0}, 3, { V(V_DUST,1), V(V_OIL,5), V(V_LCM,10), },
0,       NAT_RLEV,  25,     0,    0,    NAT_ELEV, 0,    50, 100,   "medical discoveries",
    {0}, 3, { V(V_DUST,1), V(V_OIL,5), V(V_LCM,10), },
0,       NAT_ELEV,  10,     0,    0,    -1,       0,    0,  100,   "a class of graduates",
    {0}, 1, { V(V_LCM,1), },
0,       NAT_HLEV,  5,      0,    0,    -1,       0,    0,  100,   "happy strollers",
    {0}, 1, { V(V_LCM,1), },
};

/*	marine characteristics -- one entry per ship type */
struct  mchrstr mchr[]  = { /*
 lcm  hcm armor speed visib vrnge frnge glim name   */
 20,  10,  30,   50,   10,    4,    1,    1, "patrol boat",
    {0}, 4, { V(V_MILIT,10), V(V_SHELL,10), V(V_GUN,2), V(V_FOOD,30), },
 25,  15,  10,   15,   10,    2,    0,    0, "fishing boat",
    {0}, 2, { V(V_CIVIL,20), V(V_FOOD,100), },
 25,  25,  60,   20,   20,    3,    1,    1, "minesweep",
    {0}, 4, { V(V_MILIT,25), V(V_SHELL,60), V(V_GUN,2), V(V_FOOD,25), },
 25,  35,  80,   35,   15,    4,    3,    2, "destroyer",
    {0}, 4, { V(V_MILIT,80), V(V_SHELL,40), V(V_GUN,4), V(V_FOOD,80), },
 40,  30,  30,   25,    2,    3,    2,    2, "submarine",
    {0}, 4, { V(V_MILIT,25), V(V_SHELL,25), V(V_GUN,4), V(V_FOOD,25), },
 40,  40,  60,   30,   20,    3,    1,    2, "tender",
    {0}, 4, { V(V_MILIT,100), V(V_SHELL,200), V(V_GUN,30), V(V_FOOD,100), },
 40,  50, 100,   30,   20,    5,    6,    3, "heavy cruiser",
    {0}, 4, { V(V_MILIT,80), V(V_SHELL,60), V(V_GUN,6), V(V_FOOD,80), },
 60,  40,  50,   20,   20,    3,    0,    0, "cargo ship",
    {0}, 8, { V(V_CIVIL,100), V(V_SHELL,50), V(V_GUN,50), V(V_IRON,100),
	      V(V_DUST,100), V(V_BAR,50), V(V_FOOD,100), V(V_OIL,100), },
 50,  50,  20,   10,   25,    4,    0,    0, "oil derrick",
    {0}, 4, { V(V_CIVIL,80), V(V_MILIT,80), V(V_FOOD,200), V(V_OIL,200), },
 50,  60,  80,   25,   25,    4,    2,    2, "aircraft carrier",
    {0}, 5, { V(V_MILIT,60), V(V_SHELL,100), V(V_GUN,4), V(V_PLANE,40),
	      V(V_FOOD,80), },
 55,  65, 127,   25,   25,    6,    8,    4, "battleship",
    {0}, 4, { V(V_MILIT,200), V(V_SHELL,100), V(V_GUN,8), V(V_FOOD,200), },
 90,  40,  15,   40,   15,    2,    0,    0, "yacht",
    {0}, 4, { V(V_CIVIL,25), V(V_DUST,100), V(V_BAR,25), V(V_FOOD,50), },
  0,   0,   0,    0,    0,    0,    0,    0, 0,
    {0}, 0, {{0}},
};	/* if you add ships be sure to increase the definition of shp_maxno */
	/* NOTE: V(V_xxx) entries must be in numerical order */

/*   treaty clause characteristics -- one entry per possible treaty clause */
struct  tchrstr tchr[]  = {
	SEAATT,	"no attacks on ships",
	SEAFIR,	"no shelling ships",
	LANATT,	"no sector attacks",
	LANFIR,	"no shelling land",
	NEWSHP,	"no building ships",
	NEWNUK,	"no new nuclear weapons",
	TRTENL,	"no enlistment",
	0,      0,
};

/* report -- one entry per type of news item (order agrees with news verbs) */
struct  rptstr rpt[]    = {
/*  nice    page    text
*/  0,      3,      "does nothing in particular to %s",
		    "does nothing to %s",
    -3,     1,      "wins a sector from %s",
		    "captures one of %s's sectors",
    -3,     1,      "is repulsed by %s troops",
		    "fails in attacking %s",
    -1,     2,      "has a spy shot by %s",
		    "spy captured, tried, and shot by %s",
    1,      2,      "sends a telegram to %s",
		    "corresponds with %s",
    3,      3,      "signs a treaty with %s",
		    "agrees to treaty with %s",
    2,      3,      "makes a loan to %s",
		    "lends money to %s",
    1,      3,      "repays a loan from %s",
		    "makes last payment on loan from %s",
    0,      3,      "makes a sale to %s",
		    "sells merchandise to %s",
    2,      2,      "grants a sector to %s",
		    "sector granted to %s",
    -2,     1,      "bombards land occupied by %s",
		    "bombs %s sectors",
    -2,     1,      "shells a ship owned by %s",
		    "fires on %s ships",
    0,      2,      "takes over unoccupied land",
		    "foolishly attacks unowned land",
    0,      1,      "has a ship torpedoed",
		    "ship torpedoed by unknown aggressor",
    0,      1,      "fires on %s in self-defense",
		    "guns pound %s troops in heroic self-defense",
    0,      2,      "breaks sanctuary",
		    "no longer has a sanctuary",
    -2,     1,      "planes bomb one of %s's sectors",
		    "flies a bombing raid against %s",
    -2,     1,      "bombs a ship flying the flag of %s",
		    "planes bombard %s navy",
    -2,     1,      "seadogs board one of %s's ships",
		    "successfully boards one of %s's ships",
    -3,     1,      "is repelled by %s while attempting to board a ship",
		    "seadogs prove inept at boarding %s's ships",
    -1,     1,      "fires on %s aircraft",
		    "attempts to shoot down %s aircraft",
    -2,     3,      "seizes a sector from %s to collect on a loan",
		    "collects one of %s's sectors in repayment of a loan",
    -1,     2,      "considers an action which would violate a treaty with %s",
		    "decides not to violate treaty with %s (yet)",
    -4,     1,      "violates a treaty with %s",
		    "actions violate treaty with %s",
    0,      1,      "dissolves its government",
		    "throws in the towel",
    0,      1,      "ship hits a mine",
		    "ship damaged in mine field",
    5,      1,      "announces an alliance with %s",
		    "/ %s alliance declared",
    0,      1,      "declares their neutrality toward %s", /* no longer used */
		    "announces neutral relations with %s",
    -5,     1,      "declares WAR on %s",
		    "gets tough with %s and declares WAR",
    -5,     1,      "disavows former alliance with %s",
		    "is no longer allied with %s",
    5,      1,      "is no longer at war with %s",
		    "declares \"No more war with %s\"",
    0,      2,      "sector sustains storm damages",
		    "ravaged by tornados",
    0,      2,      "navy suffers hurricane damages",
		    "ravaged by hurricanes",
    0,      1,      "reports outbreak of plague",
		    "sector infected with plague",
    0,      2,      "citizens die from plague",
		    "sector reports plague deaths",
    0,      2,      "goes through a name change",
		    "adopts a new country name",
    0,      2,      "citizens starve in tragic famine",
		    "loses citizens to famine",
    0,      2,      "endures lawless rioting",
		    "suffers from outbreaks of rioting",
    -5,     1,      "annexes a sector from %s",
		    "annexes a sector from helpless %s",
    -10,    1,      "nuclear device devastates %s sector",
		    "explodes a nuclear device damaging %s territory",
    0,      0,      0
};

int   n_max_verb  = N_MAX_VERB;

struct	nscstr	ns_cond[8];
