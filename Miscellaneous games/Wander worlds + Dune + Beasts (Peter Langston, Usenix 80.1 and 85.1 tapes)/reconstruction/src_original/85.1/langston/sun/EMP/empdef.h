#
/*
**              Basic Empire definitions
** Copyright (c) 1976 by Peter S. Langston - Camb., Ma. 02140
*/

	/* miscellany */

#define	MAX_MAXNOC	64             /* max value for MAXNOC (in empglb) */
#define	MAX_W_SIZE	128         /* w_xsize & w_ysize can't exceed this */
#define	MAXTELSIZE	512                      /* max telegram text size */
#define	MAXNOR		8                    /* maximum number of "realms" */
#define	LAND		0      /* returned by landorsea() to indicate land */
#define	SEA		1       /* returned by landorsea() to indicate sea */
#define	NUMNUMNAMES	20     /* number of names in numnames[] (empglb.c) */
#define	NUMTENNAMES	10              /* number of multiple of ten names */

#define	WXMOD(x)    (x & (w_xsize - 1))    /* w_xsize must be a power of 2 */
#define	WYMOD(y)    (y & (w_ysize - 1))    /* w_ysize must be a power of 2 */

#define	OFFSET(stype, oset) ((int)(&(((struct stype *)0)->oset)))
#define	SETOFF(sinst, oset) ((char *) &sinst + oset)

#define	OK_RET(x)	{pr(x);return(R_OK);}
#define	SYN_RET(x)	{pr(x);return(R_SYN);}
#define	FAIL_RET(x)	{pr(x);return(R_FAIL);}

	/* return codes from command routines */
#define	R_OK		0   /* command completed sucessfully */
#define	R_FAIL		1   /* command completed unsucessfully [?] */
#define	R_SYN		2   /* syntax error in command */
#define	SYS_RETURN	3   /* system error (missing file, etc) */

	/* direction indices */
#define	DIR_STOP	0
#define	DIR_UR		1
#define	DIR_R		2
#define	DIR_DR		3
#define	DIR_DL		4
#define	DIR_L		5
#define	DIR_UL		6
#define	DIR_VIEW	7
#define	DIR_BOMB	8
#define	SELL_NOT_DELIV	7                   /* in "_use" field => contract */
#define	FIRST_DIR	1
#define	LAST_DIR	6

#ifdef D_UPDATE
	/* selective update flags used by getsect() & getship() */
#define	UP_NONE		0       /* no update */
#define	UP_OWN		1       /* only when ???_own == cnum */
#define	UP_ALL		2       /* ignore ???_own */
#define	UP_TIME		4       /* even if little work done */
#define	UP_GOD		8       /* even if cnum == 0 */
#define	UP_VERBOSE	16	/* mention delivery backlogs, etc */
#define	UP_QUIET	32	/* don't mention anything */
#endif D_UPDATE


#ifdef D_NATSTAT
#define	MONEY		001
#define	CAP		002
#define	VIS		004
#define	NORM		010
#define	GOD		020
	/* nation status types */
#define	STAT_FREE	000
#define	STAT_VIS	004
#define	STAT_NEW	005
#define	STAT_NORM	014	/* CAP & MONEY are added at login */
#define	STAT_GOD	037
#endif D_NATSTAT

	/* nation relation codes */
#define	NEUTRAL	0
#define	ALLIED	1
#define	HOSTILE	2
#define	AT_WAR	3


#ifdef D_SECTDES
	/* sector types (must agree with order in dchr, empglb.c) */
#define	SCT_WATER	0       /* basics */
#define	SCT_MOUNT	1
#define	SCT_SANCT	2
#define	SCT_WASTE	3
#define	SCT_RURAL	4
#define	SCT_CAPIT	5
#define	SCT_URBAN	6
#define	SCT_PARK	7
#define	SCT_ARMSF	8       /* industries */
#define	SCT_AMMOF	9
#define	SCT_MINE	10
#define	SCT_GMINE	11
#define	SCT_HARBR	12
#define	SCT_WAREH	13
#define	SCT_AIRPT	14
#define	SCT_AGRI	15
#define	SCT_OIL		16
#define	SCT_LIGHT	17
#define	SCT_HEAVY	18
#define	SCT_FORTR	19      /* military/scientific */
#define	SCT_TECH	20
#define	SCT_RSRCH	21
#define	SCT_NUKE	22
#define	SCT_LIBR	23
#define	SCT_HIWAY	24      /* communications */
#define	SCT_RADAR	25
#define	SCT_WETHR	26
#define	SCT_BHEAD	27
#define	SCT_BSPAN	28
#define	SCT_BANK	29      /* financial */
#define	SCT_XTRA	30	/* a spare */

#define	SCT_MAXDEF	30	/* highest sector type in header files */
#endif D_SECTDES

#ifdef D_SHIPTYP
	/* ship types */
#define	SHP_P		0	/* pt boat */
#define	SHP_F		1	/* fishing boat */
#define	SHP_M		2	/* minesweep */
#define	SHP_D		3	/* destroyer */
#define	SHP_S		4	/* submarine */
#define	SHP_T		5	/* tender */
#define	SHP_H		6	/* heavy cruiser */
#define	SHP_C		7	/* cargo ship */
#define	SHP_O		8	/* oil derrick */
#define	SHP_A		9	/* aircraft carrier */
#define	SHP_B		10	/* battleship */
#define	SHP_Y		11	/* yacht */
#define	SHP_MAXDEF	11	/* highest ship type in header files */
#endif D_SHIPTYP

#ifdef D_NEWSVERBS
	/* news verbs */
#define	N_WON_SECT	1
#define	N_SCT_LOSE	2
#define	N_SPY_SHOT	3
#define	N_SENT_TEL	4
#define	N_SIGN_TRE	5
#define	N_MAKE_LOAN	6
#define	N_REPAY_LOAN	7
#define	N_MAKE_SALE	8
#define	N_GRANT_SECT	9
#define	N_SCT_SHELL	10
#define	N_SHP_SHELL	11
#define	N_TOOK_UNOCC	12
#define	N_TORP_SHIP	13
#define	N_FIRE_BACK	14
#define	N_BROKE_SANCT	15
#define	N_SCT_BOMB	16
#define	N_SHP_BOMB	17
#define	N_BOARD_SHIP	18
#define	N_SHP_LOSE	19
#define	N_FLAK		20
#define	N_SEIZE_SECT	21
#define	N_HONOR_TRE	22
#define	N_VIOL_TRE	23
#define	N_DISS_GOV	24
#define	N_HIT_MINE	25
#define	N_DECL_ALLY	26
#define	N_DECL_NEUT	27
#define	N_DECL_WAR	28
#define	N_DIS_ALLY	29
#define	N_DIS_WAR	30
#define	N_SCT_STORM	31
#define	N_SHP_STORM	32
#define	N_OUT_PLAGUE	33
#define	N_DIE_PLAGUE	34
#define	N_NAME_CHNG	35
#define	N_DIE_FAMINE	36
#define	N_RIOT		37
#define	N_ANNEX		38
#define	N_NUKE		39
#define	N_MAX_VERB	39
#define	N_MAX_PAGE	 3             /* depends on values in rpt (empglb) */

#define	NEWS_PERIOD	302400.          /* max duration of news (seconds) */
#endif D_NEWSVERBS

#ifdef D_TRTYCLAUSE
	/* treaty clauses */
#define	SEAATT	0001
#define	SEAFIR	0002
#define	LANATT	0004
#define	LANFIR	0010
#define	NEWSHP	0020
#define	NEWNUK	0040
#define	TRTENL	0100
#endif D_TRTYCLAUSE

#ifdef D_PLGSTAGES
#define	PLG_HEALTHY	0
#define	PLG_DYING	1
#define	PLG_INFECT	2
#define	PLG_INCUBATE	3
#define	PLG_EXPOSED	4
#endif D_PLGSTAGES


	/*	STRUCT DEFINITIONS FOR EMPIRE	*/

#ifdef D_NATSTR
#define	NAT_TLEV	0
#define	NAT_RLEV	1
#define	NAT_ELEV	2
#define	NAT_HLEV	3

struct	boundstr {
	char	b_xl, b_xh;		/* horizontal bounds */
	char	b_yl, b_yh;		/* vertical bounds */
};
extern	struct	boundstr	nrealm[];
struct	natstr {
	char	nat_cnam[20];		/* country name */
	char	nat_pnam[20];		/* representative */
	short	nat_btu;		/* bureaucratic time units */
	short	nat_nuid;		/* nation user-id */
	char	nat_tty[17];		/* name of current tty */
	char	nat_tgms;		/* # of telegrams to be announced */
	char	nat_xcap, nat_ycap;	/* location in abs coords */
	char	nat_stat;		/* visitor, nocap, etc */
	char	nat_dayno;		/* day of the year mod 128 */
	short	nat_minused;		/* number of minutes used today */
	short	nat_pid;		/* proc id of current login mod 2^15 */
	struct	boundstr nat_b[MAXNOR];	/* realm bounds */
	long	nat_date;		/* last logoff */
	long	nat_money;		/* moola */
	short	nat_relate[MAX_MAXNOC/8];/* two bits for each other country
		coded:	00 : NEUTRAL	01 : ALLIED
			10 : HOSTILE	11 : AT_WAR */
	float	nat_level[4];		/* technology, etc */
	long	nat_newstim;		/* date news last read */
};
extern	struct	natstr      nat;
extern	int	lgn_num;	/* number of current country in "nat" */
static	int	SN_num;		/* local copy of above for SAVNAT() */
#define	SAVNAT()	putnat(SN_num = lgn_num);
#define	RSTNAT()	getnat(SN_num);
#endif D_NATSTR

#ifdef D_VTYPES
#define	V_MINE		1	/* order must agree with vchr[] (empglb.c) */
#define	V_CHKPT		2
#define	V_SPARE
#define	V_PSTAGE	4
#define	V_PTIME		5
#define	V_CIVIL		6
#define	V_MILIT		7
#define	V_SHELL		8
#define	V_GUN		9
#define	V_PLANE		10
#define	V_IRON		11
#define	V_DUST		12
#define	V_BAR		13
#define	V_FOOD		14
#define	V_OIL		15
#define	V_LCM		16
#define	V_HCM		17
#define	V_D_FIRST	18
#define	V_CDEL		18
#define	V_MDEL		19
#define	V_SDEL		20
#define	V_GDEL		21
#define	V_PDEL		22
#define	V_IDEL		23
#define	V_DDEL		24
#define	V_BDEL		25
#define	V_FDEL		26
#define	V_ODEL		27
#define	V_LDEL		28
#define	V_HDEL		29
#define	V_D_LAST	29
#define	V_CSELL		30
#define	V_MSELL		31
#define	V_SSELL		32
#define	V_GSELL		33
#define	V_PSELL		34
#define	V_ISELL		35
#define	V_DSELL		36
#define	V_BSELL		37
#define	V_FSELL		38
#define	V_OSELL		39
#define	V_LSELL		40
#define	V_HSELL		41
#define	V_MAX		41
#endif D_VTYPES

struct  vstr    {
	unsigned short	v_type:6;	/* one of 64 possible types */
	unsigned short	v_amt:10;	/* a 10 bit value */
};

struct  avstr   {		/* alternate vstr */
	short	tv_amtype;	/* used for initializations */
};

#ifdef D_SCTSTR
#define	MAXSCTV 16

struct	sctstr {
	char	sct_x;		/* x coord of sector */
	char	sct_y;		/* y coord of sector */
	char	sct_own;	/* owner's country num */
	char	sct_type;	/* sector type */
	char	sct_effic;	/* 0% to 100% */
	char	sct_mobil;	/* mobility units */
	long	sct_lstup;	/* last update time */
	char	sct_min;	/* ease of mining ore */
	char	sct_gmin;	/* amount of gold ore */
	char	sct_fertil;	/* fertility of soil */
	char	sct_oil;	/* oil content */
	char	sct_fill;	/* to position sct_nv, very important */
	char	sct_nv;		/* current number of variables */
	struct	vstr sct_v[MAXSCTV];/* variable commodities in sector */
};
extern	struct	sctstr		sect;
static  int	SS_x, SS_y;	/* saved sector coordinates for SAVSCT() */

#define	SAVSCT()        putsect(SS_x = sect.sct_x, SS_y = sect.sct_y);
#define	RSTSCT()        getsect(SS_x, SS_y, UP_NONE);
#endif D_SCTSTR

#ifdef D_SHPSTR
#define	MAXSHPV 8

struct	shpstr {
	char    shp_x;		/* x location in abs coords */
	char    shp_y;		/* y location in abs coords */
	char    shp_own;	/* owner's country num */
	char    shp_type;	/* ship type */
	char    shp_effic;	/* 0% to 100% */
	char    shp_mobil;	/* mobility units */
	long    shp_lstup;	/* last update time */
	char    shp_price;	/* sale price / ton */
	char    shp_fleet;	/* group membership */
	char    shp_fill[3];	/* to position shp_nv, very important */
	char    shp_nv;		/* current number of variables */
	struct  vstr shp_v[MAXSHPV];/* variable commodities on ship */
};
extern  struct  shpstr  ship;
#endif D_SHPSTR

#ifdef D_COMSTR
#define	CLIST   -1      /* spit out command list */
#define	EXEC    -2      /* execute commands from file */
#define	FIX     -3      /* call(FIX) execls empfix */
#define	QUIT    -4      /* call(QUIT) ends a session */
#define	SHELL   -5      /* call(SHELL) execls a shell */

struct	comstr {
	char	*c_form;	/* prototype of command */
	char	c_prog;		/* # of module that contains it */
	char	c_cost;		/* btu cost of command */
	int	(*c_addr)();	/* core addr of appropriate routine */
	int	c_permit;	/* who is allowed to "do" this command */
};
extern  struct  comstr  coms[];
#endif D_COMSTR

#ifdef D_POWSTR
struct powstr {
	float	p_sects;
	float	p_effic;
	float	p_civil;
	float	p_milit;
	float	p_shell;
	float	p_guns;
	float	p_plane;
	float	p_iron;
	float	p_dust;
	float	p_gold;
	float	p_food;
	float	p_oil;
	float	p_ships;
	float	p_money;
	float	p_power;
};
#endif D_POWSTR

#ifdef D_DCHRSTR

#define	NPKG	0		/* no special packaging */
#define	WPKG	1		/* "warehouse" packaging */
#define	UPKG	2		/* "urban" packaging */

/* for d_flg */
#define	NAVOK	1		/* ships can always navigate */
#define	NAV_02	2		/* requires 2% effic to navigate */
#define	NAV_60	3		/* requires 60% effic to navigate */

struct	dchrstr	{
	char	d_mnem;		/* map symbol */
	char	d_prd;		/* product vtype */
	char	d_mcst;		/* movement cost */
	char	d_flg;		/* movement cost */
	char	d_pkg;		/* type of packaging in these sects */
	char	d_ostr;		/* offensive strength */
	char	d_dstr;		/* defensive strength */
	char	d_value;	/* resale ("collect") value */
	char	*d_name;	/* full name of sector type */
};
extern	struct	dchrstr dchr[];
#endif D_DCHRSTR

#ifdef D_ICHRSTR
#define	NUMPKG	4		/* number of different kinds of packaging */
#define	I_CIVIL	0
#define	I_MILIT	1
#define	I_SHELL	2
#define	I_GUN	3
#define	I_PLANE	4
#define	I_IRON	5
#define	I_DUST	6
#define	I_BAR	7
#define	I_FOOD	8
#define	I_OIL	9
#define	I_LCM	10
#define	I_HCM	11

struct	ichrstr	{
	char	i_mnem;		/* usually the initial letter */
	char	i_vtype;	/* vtype of item */
	char	i_del;		/* vtype of item delivery variable */
	char	i_sell;		/* vtype of item price variable */
	char	i_bid;		/* average amount paid on contract */
	char	i_value;	/* mortgage value */
	char	i_lbs;		/* how hard to move */
	char	i_pkg[NUMPKG];	/* units for reg, ware, urb, bank */
	char	*i_name;	/* full name of item */
};
extern	struct	ichrstr ichr[];
#endif D_ICHRSTR

#ifdef D_VCHRSTR
#define	VCH_ITEM	1	/* v_type is a (movable) "thing" */
#define	VCH_DEL		2	/* v_type is delivery, vch_inum is "thing" */
#define	VCH_PRICE	3	/* v_type is price, vch_inum is "thing" */

struct  vchrstr {
	char	vch_inum;	/* index into ichr[] for "thing" */
	char	vch_flag;	/* indicates relationship to vch_inum */
};

extern  struct  vchrstr vchr[];
#endif D_VCHRSTR

#ifdef D_NWSSTR
struct	nwsstr {
	char	nws_ano;	/* "actor" country # */
	char	nws_vrb;	/* action (verb) */
	char	nws_vno;	/* "victim" country # */
	char	nws_ntm;	/* number of times */
	long	nws_when;	/* time of action */
};
extern	struct	nwsstr nws;
#endif D_NWSSTR

#ifdef D_RPTSTR
#define	NUM_RPTS	2	/* number of story alternates */
struct	rptstr {
	char	r_good_will;	/* how "nice" the action is */
	char	r_newspage;	/* which page this item belongs on */
	char	*r_newstory[NUM_RPTS];	/* texts for fmt( */
};
extern	struct	rptstr rpt[];
#endif D_RPTSTR

#ifdef D_PCHRSTR
#define	P_SHELL	1
#define	P_GUN	2
#define	P_PLANE	3
#define	P_IRON	4
#define	P_DUST	5
#define	P_BAR	6
#define	P_FOOD	7
#define	P_OIL	8
#define	P_LCM	9
#define	P_HCM	10
#define	P_TLEV	11
#define	P_RLEV	12
#define	P_ELEV	13
#define	P_HLEV	14

#define	MAXPRDV 4

struct	pchrstr {
	char	p_vtype;	/* vtype if product is a variable */
	char	p_level;	/* index (NAT_?LEV) if product is not a var */
	char	p_cost;		/* dollars / product unit */
	char	p_nrndx;	/* index into sect of natural resource */
	char	p_nrdep;	/* depletion as a % of resource used */
	char	p_nlndx;	/* index (NAT_?LEV) affecting production */
	short	p_nlmin;	/* minimum lvl required */
	short	p_nllag;	/* lag, mul by (lvl-nlmin)/(lvl-nlmin+nllag) */
	short	p_effic;	/* process efficiency, mult by p_effic/100 */
	char	*p_name;	/* name of product */
	char	p_fill[1];	/* to align p_nv like sctstr, very important */
	char	p_nv;		/* number of constituents */
	struct	avstr	p_v[MAXPRDV];	/* constituent costs */
};					/* (actually a vstr struct) */
extern  struct  pchrstr pchr[];
#endif D_PCHRSTR

#ifdef D_MCHRSTR
#define	MAXMCHV 8
struct	mchrstr {
	char	m_lcm;		/* units of lcm to build */
	char	m_hcm;		/* units of hcm to build */
	char	m_armor;	/* how well armored it is */
	char	m_speed;	/* how fast it can go */
	char	m_visib;	/* how well it can be seen */
	char	m_vrnge;	/* how well it can see */
	char	m_frnge;	/* how far it can fire */
	char	m_glim;		/* how many guns it can fire */
	char	*m_name;	/* full name of type of ship */
	char	m_fill[5];	/* align m_nv as in sctstr, very important */
	char	m_nv;		/* number of variables it can hold */
	struct	avstr	m_v[MAXMCHV];	/* variables it can hold */
};					/* (really a vstr struct) */
extern	struct	mchrstr	mchr[];
#endif D_MCHRSTR

#ifdef D_TELSTR
struct	telstr {
	char	tel_from;	/* sender */
	char	tel_spare;
	long	tel_date;	/* when sent */
	short	tel_length;	/* how long */
};
extern	struct	telstr tgm;
#endif D_TELSTR

#ifdef D_TRTSTR
struct	trtstr {
	char	trt_cna;	/* proposed by */
	char	trt_cnb;	/* accepted by (if >0, else pending) */
	char	trt_acond;	/* conditions for proposer */
	char	trt_bcond;	/* conditions for accepter */
	float	trt_bond;	/* amount of bond involved */
	long	trt_exp;	/* expiration date */
};
extern  struct  trtstr trty;
#endif D_TRTSTR

#ifdef D_TCHRSTR
struct	tchrstr {
	char	t_cond;		/* bit to indicate this clause */
	char	*t_name;	/* description of clause */
};
extern	struct	tchrstr	tchr[];
#endif D_TCHRSTR

#ifdef D_LONSTR
struct	lonstr {
	char	l_loner;	/* loan shark */
	char	l_lonee;	/* sucker */
	char	l_irate;	/* interest rate */
	char	l_ldur;		/* intended duration */
	short	l_amtpaid;	/* amount paid so far */
	short	l_amtdue;	/* amount still owed */
	long	l_lastpay;	/* date of most recent payment */
	long	l_duedate;	/* date after which interest doubles, etc */
};
extern	struct	lonstr loan;
#endif D_LONSTR

#ifdef D_NUKSTR
struct	nukstr {
	char	nuk_x;		/* current loc of device */
	char	nuk_y;
	char	nuk_fsx;	/* fail-safe coords */
	char	nuk_fsy;
	short	nuk_fscode;	/* fail-safe code, armed if != 0 */
	short	nuk_size;	/* megatons */
};
#endif D_NUKSTR

#ifdef D_NSCSTR
struct	nscstr	{
	short	n_fld1;		/* first commodity or number */
	short	n_oper;		/* required relationship operator */
	short	n_fld2;		/* second commodity or number */
};
struct	nstr	{		/* for sectors */
	char	n_x, n_y;	/* current sector x & y */
	char	n_lx, n_hx;	/* x bounds */
	char	n_ly, n_hy;	/* y bounds */
	char	n_ix, n_iy;	/* x & y increments */
	short	n_ncond;	/* number of conditions */
	struct	nscstr	n_cond[8];	/* the conditions */
};
#define	NBLISTMAX   32
struct	nbstr	{		/* for ships (boats) */
	short	nb_sno;		/* current ship number */
	char	nb_cno;		/* country number, (0 => all countries) */
	char	nb_mode;	/* 0 => all ships, -1 => fleet, */
				/* -2 => area, >0 => list length */
	short	nb_nums[NBLISTMAX];	/* list of ships */
	char	nb_scnt;	/* current index into ship list (nb_nums) */
	char	nb_fleet;	/* fleet letter */
	char	nb_lx, nb_xs;	/* low x and x size (width) */
	char	nb_ly, nb_ys;	/* low y and y size (height) */
	short	nb_ncond;	/* number of conditions */
	struct	nscstr	nb_cond[8];	/* the conditions */
};

#define	NSC_VAR 0100000
#define	NSC_OFF 0010000

struct	castr	{
	short	ca_code;	/* encoded form */
	char	*ca_name;	/* name used for matches */
};

extern	struct	castr	ca[];

#endif D_NSCSTR

#ifdef D_TIMSTR
struct  tm  {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};
#endif D_TIMSTR

/*	External definitions
*/
extern	char	*emprog[], *empsrc, *empfix;

#ifdef D_FILES
extern	char	*upfil, *downfil, *holfil;
extern	char	*sectfil, *natfil, *newsfil, *loanfil, *infodir, *shipfil;
extern	char	*telfil, *powfil, *treatfil, *nukfil, *datadir;
extern	int	 sectf, natf, newsf, loanf, infof, shipf, telf, powf, trtf, nukf;
extern	int	nnrarg;
extern	char	*nroffil, *nrarg[];
#endif D_FILES

extern	char	*privname, *privlog, l_cvt;
extern	int	privuid, w_xsize, w_ysize;
extern	int	maxnoc, maxcno, m_m_p_d, n_max_verb, sct_maxno, shp_maxno;
extern	long	s_p_etu;
extern	float	fgrate, fcrate, eatrate, babyeat;
extern	float	ubrate, obrate, bankint;
extern	char	dirch[];

#ifdef D_WEATHER
extern	int	wthr_xh, wthr_yh, wthr_xl, wthr_yl;
extern	long	wthr_date;
extern	double	wthr_hi, wthr_lo;
#endif D_WEATHER

extern	char	*Version;			/* version ident */
extern	char	junk[], combuf[], *argp[], *condarg;
extern	char	nulls[], *effadv[], *numnames[], *tennames[];
extern	char	*shllpath, *shllnam, *shllrg1, *shllrg2, *shllrg3;
extern	int	sx, sy, lx, hx, ly, hy, ix, iy;
extern	int	capx, capy, nbrx, nbry;
extern	int	capxof[], capyof[];
extern	int	cnum, nbtu, nstat, ncomstat, nminused, btused;
extern	int	god, broke;
extern	int	owner, proto, redirin;
extern	int	ttymod[3], weirdmode;
extern	int	diroff[][2], defoff[32];
extern	long	lasttime, curup;
extern	double	dolcost;

#ifdef D_P_ENLI
extern	double	enli_max, enli_rate, enli_hfact, enli_btu, enli_happy;
#endif D_P_ENLI

#ifdef D_P_POWE
extern	double	powe_cost;
#endif D_P_POWE

#ifdef D_P_BUIL
extern	double	buil_bt, buil_bc;
extern	int	buil_bh;
extern	double	buil_nt, buil_nc;
extern	int	buil_nh, buil_nl, buil_nd, buil_no;
#endif D_P_BUIL
