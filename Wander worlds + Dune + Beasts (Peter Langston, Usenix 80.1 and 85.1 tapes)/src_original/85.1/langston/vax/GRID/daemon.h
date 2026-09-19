/*
**	DAEMON.H -- Header file for daemon for labyrinth game
**		P. Langston, 6/82
*/

#define	INGRID(x,y)	((x) >= 0 && (x) < MW && (y) >= 0 && (y) < MH)

#define	MW	21		/* map width */
#define	MH	21		/* map height */
#define	NSBYT	56		/* 56 = ceil(MW*MH/8) */
#define	MAXVW	199		/* maximum view width */
#define	MAXVH	99		/* maximum view height */
#define	MAXP	8		/* players */
#define	MAXL	32		/* max # of levels */
#define	MAXO	10		/* max # of "others" on each level */
#define	MAXG	8		/* max # of gold piles on each level */
#define	MAXT	32		/* max # of talismans at one time */

#define	MAXDIST	12		/* how far players can see */
#define	SHTDIST	8		/* how far players can shoot */

struct	xystr	{
	char	X;
	char	Y;
};

struct	othstr	{	/* describes location & direction of "others" */
	char	o_x;			/* x location */
	char	o_y;			/* y location */
	char	o_d;			/* direction */
	char	o_h;			/* health */
};

#define	NO_OTH	((struct othstr *) 0)

struct	goldstr	{	/* describes location and size of gold piles */
	char	g_x;			/* x location */
	char	g_y;			/* y location */
	int	g_a;			/* amount */
};

#define	NSTAIRS	8
struct	levstr	{	/* contains all level-specific information */
	char	l_vw[12];		/* 12 = ceil(((MW-3)/2*(MH-1)/2)/8) */
	char	l_hw[12];		/* 12 = ceil(((MH-3)/2*(MW-1)/2)/8) */
	struct	xystr	l_us[NSTAIRS];	/* up stairs */
	struct	xystr	l_ds[NSTAIRS];	/* down stairs */
	struct	othstr	l_o[MAXO];	/* others on that level */
	struct	goldstr	l_g[MAXG];	/* gold chests on that level */
};

/* values for p_flg */
#define	MAPS	0x00000001	/* show all stairs */
#define	MAPW	0x00000002	/* show all walls */
#define	MAPG	0x00000004	/* show all gold */
#define	MAPO	0x00000008	/* show all others */
#define	MAPP	0x00000010	/* show all players */
#define	MAPALL	0x0000001F	/* all the map priveleges */
#define	SNEAK	0x00000100	/* player is sneaky */
#define	INVIS	0x00000200	/* player is invisible */
#define	ALI	0x00000400	/* align toward attacker */
#define	SHIELD	0x00000800	/* player is shielded */
#define	TWALL	0x00001000	/* player can walk through walls */
#define	HEAL	0x00002000	/* player heals quickly */
#define	XHEAL	0x00004000	/* player heals very quickly */
#define	LEVI	0x00008000	/* player is floating */
#define	HASTE	0x00010000	/* player is moving double speed */
#define	TN_VIEW	0x00100000	/* view area is being used temporarily */
#define	T_OF_R	0x00200000	/* has found IT */
#define	YELL	0x00800000	/* entering text - wait for <CR> or <LF> */
#define	WIZARD	0x01000000	/* has become wizard (power but no records) */

/* flags that only act for one level */
#define	L_FLGS	(MAPALL | SNEAK | INVIS | ALI | SHIELD | TWALL | HEAL | XHEAL | LEVI | HASTE )

/* values for p_needs */
#define	N_MAP	1		/* something may have changed map */
#define	N_VIEW	2		/* something may have changed view */
#define	N_VFLSH	4		/* view has been recomputed, but not flushed */
#define	N_REPOS	8		/* cursor has moved from scroll region */

struct	plyrstr	{	/* contains all player-specific information */
	char	p_name[16];		/* player's name */
	char	p_x, p_y, p_d;		/* player location & direction */
	char	p_vx, p_vy, p_vd;	/* view location & direction */
	char	p_l;			/* current level */
	char	p_maxl;			/* deepest player has gone */
	char	p_health;		/* how strong you are, 100 max */
	char	p_needs;		/* used for graphics */
	short	p_pid;			/* pid of paused process */
	short	p_fh;			/* output file handle */
	short	p_ospeed;		/* output speed */
	short	p_crtyp;		/* crt type */
	short	p_vw;			/* view width */
	short	p_vh;			/* view height */
	short	p_vmx;			/* view mid x */
	short	p_vmy;			/* view mid y */
	short	p_arg;			/* numeric argument */
	long	p_flg;			/* misc info */
	long	p_gold;			/* amount of gold */
	long	p_score;		/* # points earned */
	char	p_seen[MAXL][NSBYT];	/* a bit per wall on each level */
	struct	smstr *p_vsmp;		/* view screen manager pointer */
	struct	smstr *p_msmp;		/* map screen manager pointer */
	struct	talstr	*p_tal;		/* pointer to linklist of talismans */
};

#define	NO_PLYR	((struct plyrstr *) 0)

struct	tchrstr	{	/* talisman characteristics */
	int	tc_bprice;	/* avg base price */
	int	tc_cprice;	/* avg price per charge */
	int	tc_drain;	/* avg health eaten by using it (percent) */
	char	tc_name[24];	/* what it's called */
};

struct	talstr	{	/* talisman instances */
	char	t_type;		/* type of talisman, 0 => unused */
	char	t_charge;	/* how many more times it will work */
	char	t_drain;	/* how much health using it eats (percent) */
	char	t_x, t_y, t_l;	/* location iff t_x > 0 */
	struct	talstr	*t_next;/* link to next talisman */
};

#define	NO_TAL		((struct talstr *) 0)

extern	char	sb[128];	/* scroll buffer used for sprintfs */

extern	int	maxp;		/* max # of players */
extern	int	maxl;		/* max # of levels */
extern	int	maxo;		/* max # of others per level */
extern	int	maxg;		/* max # of gold piles per level */
extern	int	maxt;		/* max # of talismans at one time */
extern	int	livelim;	/* minimum health to be alive */
extern	int	seelim;		/* minimum health to be able to see */
extern	int	minpdam;	/* min damage done by player's shots */
extern	int	minodam;	/* min damage done by other's shots */
extern	int	minfdam;	/* min damage done by fire talisman */

extern	int	dx[4];
extern	int	dy[4];

extern	int	maxpnum;	/* highest player number in use + 1 */
extern	int	dlocked;	/* we own dmnlock if != -1 */
extern	int	glocked;	/* we own gridlock if != -1 */
extern	int	nalive;		/* num of players still living */
extern	int	numnew;		/* num that have entered the game */
extern	int	npfh;		/* file handle for newplayer file */

extern	struct	levstr	lev[];

extern	struct	plyrstr	p[];

extern	struct	tchrstr	tc[];

extern	struct	talstr	t[];

extern	char	*copy();
