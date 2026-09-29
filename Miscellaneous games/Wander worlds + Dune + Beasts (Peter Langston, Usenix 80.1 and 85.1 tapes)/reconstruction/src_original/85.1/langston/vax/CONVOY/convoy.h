#include	"../lock.h"
/*
**      CONVOY.H -- Defines for Convoy ship game   psl@DPW 2/81
** (c) P. Langston
*/

#define H_SCCS  "@(#)convoy.h	1.9 10/5/84 -- PSL games"

#define MAXPLYR     10
#define MAXFRTR     10

#define PI          3.1415926535
#define DTOR(a)     (a * PI / 180.)
#define RTOD(a)     (a * 180. / PI)

/* ways to leave the game (die()) */
#define SINK        0
#define QUIT_HON    1
#define QUIT_ANY    2
#define RECALL      3

/* p_st bits */
#define SUB         0
#define DEST        1

/* p_flags bits */
#define ALIVE       1
#define SUBMERGE    2
#define PERISCOPE   4
#define XCHECK      8

#define NOPLAYER    ((struct plyrstr *) 0)   /* used in calls to tellall() */

#define CBUFSIZE    64
#define NAMSIZE     24

#define	SCtorp		0	/* can fire torps */
#define	SCperi		1	/* can use periscope */
#define	SCradar		2	/* can use radar/sonar */
#define	SCr_map		3	/* will appear on maps if using radar/sonar */
#define	SCr_rad		4	/* will appear on radar/sonar if using it */
#define	SCa_map		5	/* will appear on maps in any case */
#define	SCa_rad		6	/* will appear on radar/sonar in any case */
#define	SCspeed		7	/* relative speed * 100 */

#define	SCsurf		0	/* surfaced */
#define	SCsubm		1	/* submerged */

struct  recstr  {
	int	r_uid;		/* user's i.d. */
	struct  srecstr {	/* records as sub & dest captain */
	    int     r_pnts;	/* how many points earned */
	    int     r_miss;	/* number of missions */
	    int     r_sunk;	/* number of times sunk */
	    int     r_fsafe;	/* freighters that made it */
	    int     r_fsnk;	/* freighters sunk */
	    int     r_ssnk[2];	/* ships sunk[SUB|DEST] */
	    long    r_time;	/* total seconds played */
	} r_st[2];
};

struct  pvstr   {
	float   pv_px, pv_py;	/* position vector */
	float   pv_vx, pv_vy;	/* velocity vector */
};

struct  plyrstr {
	char    *p_cp;              /* pointer past end of p_com */
	char    p_com[CBUFSIZE];    /* command(s) being typed */
	char    p_name[NAMSIZE];    /* player's name */
	char    p_st;               /* ship type */
	char    p_flags;            /* misc info */
	short   p_ifh;              /* input file handle for tty */
	short   p_ofh;              /* output file handle for tty */
	short   p_type;             /* crt type */
	short	p_ospeed;	    /* for tputsfh() */
	char    p_sh;               /* screen height */
	char    p_sw;               /* screen width */
	short   p_pid;              /* pid of paused process */
	short   p_arms;             /* armaments on board */
	short   p_uid;              /* user's real uid */
	float   p_eff;              /* efficiency of vessel */
	float   p_head;             /* heading in radians */
	float   p_speed;            /* speed */
	struct  pvstr   p_s;        /* ship position & velocity */
	struct  pvstr   p_p;        /* probe position & velocity */
	struct  smstr   *p_msp;     /* map screen pointer */
	struct  smstr   *p_rsp;     /* radar or periscope screen pointer */
	struct  smstr   *p_ssp;     /* status screen pointer */
	struct  srecstr p_rec;      /* new addition to stored record */
};

struct  frstr   {
	float   f_eff;              /* freighter efficiency */
	float   f_x, f_y;           /* freighter location */
};

struct  torpstr {
	short   t_fuel;             /* how much further it can go */
	struct  plyrstr *t_pp;      /* who launched it */
	struct  pvstr   t_pv;       /* position & velocity of torp */
};

extern	struct	lockstr	clck, dlck;
extern  char    *cpidfil;
extern  char    *metoofil;
extern  char    *logfil;
extern  char    *ologfil;
extern  char    *upfile;
extern  char    *infofil;
extern  char    *recfil;
extern  char    *slisfil;
extern  char    *erlogfil;

extern  short   maxtorp;
extern  short   maxfspd;
extern  short   maxsspd;
extern  short   maxdspd;
extern  short   maxtspd;
extern  short   maxpspd;
extern  double  velfact;
extern  double  torpdsq;
extern  double  depthdsq;
extern  double  probedsq;
extern  double  r_rad;
extern  short   torpfuel;
extern  short   torpold;

extern  short   hellosig;
extern  short   byesig;
extern  short   xmax;
extern  short   ymax;
extern  short   pdiam;
extern  short   pxdiam;
extern  short   prad;
extern  short   pxrad;
extern  short   pradsq;
extern  short   cmdy;
extern  double  perang2;
extern  short   cadjust;
extern	short	subchars[][2];
extern  short   ttopenmode;

extern  struct  plyrstr p[];
extern  struct  frstr   f[];
extern  struct  torpstr t[];
