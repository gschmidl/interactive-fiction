#include	"../lock.h"
#include	"../gamesdef.h"
/*
**      FFDEF -- Global definitions for Fast Food
*/
#define	DEF_SCCS "@(#)ffdef.h	1.1 10/20/84 -- (c) psl 1977"

        /* commonly modified stuff is in "ffglb.c" */

#define LIM_X_SIZE      26      /* maximum horiz board size */
#define LIM_Y_SIZE      10      /* maximum vert board size */
#define LIM_PLAYERS     16      /* maximum number of tycoons in game */
#define LIM_CHAINS      26      /* maximum concurrent chains */
#define LIM_PLOTS       10      /* maximum plots each player holds */
#define LIM_UPDATES     64      /* maximum # of update time slots */
#define CNAME_LENGTH    24      /* how long chain names are */
#define PNAME_LENGTH    24      /* how long player names are */
#define NEWSLINES       4       /* how long an n_msg news story is */

#define SINGLE_STORE    -1      /* "chain number" for independents */
#define NO_PLOT_LEFT    -1      /* the null plot */

#define BUY     0               /* codes for the trade command */
#define SELL    1

#define FIRST_LOCK	0       /* codes for the lock routine */
#define UPDATE_LOCK	0
#define PLAYER_LOCK	1       /* (must agree with order in lck[] */
#define BOARD_LOCK	2
#define CHAIN_LOCK	3
#define OFFER_LOCK	4
#define MISC_LOCK	5
#define LAST_LOCK	MISC_LOCK

#define SAFE    0               /* := get requires a lock */
#define SORRY   1               /* := get doesn't require lock */

#define END_OF_GAME 1		/* nextup value at end of game */

struct  misc {
        long    m_nextup;	/* time of next update */
        struct  times {
            char    t_hr;	/* 0-23 */
            char    t_min;	/* 0-59 */
        } m_time[LIM_UPDATES];	/* hr min of updates */
        char    m_satsun;	/* 0 => updates not allowed on sat & sun */
        char    m_x_size;	/* horiz board size <= LIM_X_SIZE (26) */
        char    m_y_size;	/* vert board size <= LIM_Y_SIZE (10) */
        char    m_num_plots;	/* how many each player holds <= LIM_PLOTS */
        char    m_num_chains;	/* maximum concurrent chains <= LIM_CHAINS */
    /*** note: m_secflg should be moved to follow m_satsun ***/
        char    m_secflg;	/* 0 => no limit on price fluctuation */
        short   m_lstplt;	/* last plot given out */
        short   m_newsbits;	/* bit map of used news items */
        short   m_safe_size;	/* unmergable size for chains */
        short   m_plot_cost;	/* cost to develop a plot */
        short   m_new_bonus;	/* # of free shares for creating a chain */
        short   m_mrg1_bonus;	/* bonus to maj holder of merged out chain */
        short   m_mrg2_bonus;	/* bonus to min holder of merged out chain */
        long    m_initial_cash;	/* starting moola */
        long    m_start_time;	/* earliest time someone can log in */
        double  m_bank_int;	/* interest on cash-on-hand */
        double  m_stock_div;	/* stock dividend rate */
};

struct  player {
        char    p_name[PNAME_LENGTH];
        struct  plot {
            char    p_x;
            char    p_y;
        } p_plot[LIM_PLOTS];
        long    p_time;
        long    p_money;
        long    p_div;
        int     p_uid;
        char    p_shares[LIM_CHAINS + 1];
};

struct  chain {
        char    c_name[CNAME_LENGTH];
        short	c_size;		/* current size in plots */
        short	c_volume;	/* number of shares that have changed hands */
        short	c_trend;	/* historical effects on price */
	short	c_div;		/* last dividend amount */
        long	c_price;	/* average price during this period */
        long	c_open;		/* average price during previous period */
};

struct	offer	{
	char	o_bos;		/* holds "BUY" or "SELL" defined above */
	char	o_by;		/* who's offering */
	char	o_cnum;		/* what stock */
	char	o_shares;	/* how many shares */
	short	o_price;	/* how much per share */
};

struct  hist	{
        long    h_date;             /* time of this snapshot */
        struct  htycstr {
            long    ht_money;       /* cash $ */
            long    ht_stock;       /* stock $ */
        } h_tyc[LIM_PLAYERS];
        struct  hchnstr {
            int     hc_size;        /* size of chain */
            int     hc_shares;      /* # of shares available */
            int     hc_price;       /* price per share */
        } h_chn[LIM_CHAINS + 1];
};

struct  holder  {
        char    h_pnum;         /* player number */
        char    h_shares;       /* number of shares */
        char    h_maj;          /* share of major bonus */
        char    h_min;          /* share of minor bonus */
};

struct  newstr   {
        int     n_eff;                  /* maximum effect */
        char    *n_msg[NEWSLINES];      /* news story */
};

extern  char    *infols, *nroffil;
extern  char    *bozo_msg, *prvlog, *prvnam, *mailname;
extern  char    *upd_prog, *nroffhd, *infodir;
extern  char    *data_dir;
extern  char    *plyrfil, *boardfil, *miscfil, *chainfil, *offerfil;
extern  char    *newsfil, *onwsfil, *histfil, *jrnlfil, *ojrnfil, *upfil;
extern  char    board[LIM_X_SIZE][LIM_Y_SIZE], textbuf[1024];
extern  int     pnum, mfh, bfh, pfh, cfh, ofh;
extern  int     maxnews;
extern  long    period, fperiod, price();
extern  struct  player  plyr;
extern  struct  chain   chain;
extern  struct  misc    misc;
extern  struct  holder  hldr[];
extern  struct  newstr  news[];
extern  struct  lockstr	lck[];

extern  char    *getchrs();
