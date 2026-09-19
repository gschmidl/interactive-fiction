#include	<stdio.h>
#include	<signal.h>
#include	"convoy.h"
/*
**      CONGLB.C -- Globals & system specific misc for Convoy ship game
** (c) P. Langston 2/81
*/

static  char    *sccsid = "@(#)conglb.c	1.11 2/24/85 -- PSL games";
static  char    *h_sccs = H_SCCS;

/* define your system here */

	/* one of these three */
/*#define EMPTY           /* if the empty() sys call (or equivalent) exists */
/*#define NODELAY         /* if you have non-blocking read */
/*#define TWOPROC         /* if you don't have either of the other two */
	/* one of these two */
/*#define V6PRINTF        /* printf that uses "%D" for decimal of a long */
/*#define V7PRINTF        /* printf that uses "%ld" for decimal of a long */

#ifdef  DPW
#define EMPTY
#define V6PRINTF
#endif

#ifdef  BSD
#define EMPTY
#define V7PRINTF
#endif

#ifdef V3.0
#define NODELAY
#define V7PRINTF
#endif

#ifdef NODELAY
#include    <fcntl.h>
#endif

#ifdef	TWOPROC
char	*cmdfil;	    /* used to pass commands to convoy */
short	cfh	= -1;	    /* used to read cmdfil */
#endif

#include	"../gamesdef.h"
#define CONVOYPATH(x)	GAMESPATH(CONVOY/x)

/* NOTE: files marked "created by prog" must NOT exist initially */

#define CPIDFIL     "/tmp/convoy.pid"                   /* created by prog */
#define METOOFIL    "/tmp/convoy.me_too"                /* created by prog */
#define LOGFIL      "/tmp/convoy.log"                   /* created by prog */
#define OLOGFIL     "/tmp/convoy.olog"                  /* created by prog */
#define UPFILE	    CONVOYPATH(convoy.up)             /* must exist to run */
#define INFOFIL     CONVOYPATH(convoy.txt)                 /* playing info */
#define RECFIL      CONVOYPATH(convoy.rec)             /* if you want them */
#define SLISFIL     CONVOYPATH(sucker.list)                 /* sucker list */
#define ERLOGFIL    CONVOYPATH(oops)			 /* errors go here */

#define MAXTORP     64
#define MAXFSPD     10                     /* max freighter speed in knots */
#define MAXSSPD     30                     /* max submarine speed in knots */
#define MAXDSPD     40                     /* max destroyer speed in knots */
#define MAXTSPD     15                           /* torpedo speed in knots */
#define MAXPSPD     75                              /* probe speed in knots */
#define VELFACT     50.          /* screen-unit hours per naut-mi. seconds */
#define TORPDSQ     0.09               /* explosion radius of torp squared */
#define DEPTHDSQ    2.25       /* explosion radius of depth charge squared */
#define PROBEDSQ    0.5625     /* explosion radius of depth charge squared */
#define R_RAD       3.            /* radius of the radar area in map units */
#define TORPFUEL    12                                    /* range of torp */
#define TORPOLD     4    /* slot can be reused if less than this fuel left */

#define HELLOSIG    5                        /* used to signal new players */
#define BYESIG      6                      /* used to signal mystery death */
#define SCRNY       1          /* Y location of top of map/periscope/radar */
#define XMAX        40                                /* width of map area */
#define YMAX        19             /* height of map & periscope/radar area */
#define PDIAM       YMAX     /* periscope/radar Y diameter in screen units */
#define PXDIAM      (PDIAM * 2 + 1)      /* p/r X diameter in screen units */
#define PRAD        (PDIAM / 2)            /* p/r Y radius in screen units */
#define PXRAD       PDIAM                  /* p/r X radius in screen units */
#define PRADSQ      (PRAD * PRAD)     /* p/r Y rad squared in screen units */
#define CMDY        (YMAX + SCRNY)            /* top line of scroll buffer */
/*#define PERANG2     0.5236         /* half angle of periscope (30 degrees) */
#define PERANG2     0.7854         /* half angle of periscope (45 degrees) */
#define CADJUST     10			   /* effect of '[' & ']' commands */

struct	lockstr	clck = {
	CONVOYPATH(.cnode),		/* linking spot */
	CONVOYPATH(c.lock),		/* file to link */
	"Waiting on lock ... be patient!\n",
	15*60,				/* how long a lock can lie around */
	20,				/* how many tries before giving up */
	3,				/* time to sleep after each failure */
	SIG_IGN,			/* no routine to catch signals */
};

struct	lockstr	dlck = {
	CONVOYPATH(.dnode),		/* linking spot */
	CONVOYPATH(d.lock),		/* file to link */
	0,
	5*60,				/* how long a lock can lie around */
	1,				/* how many tries before giving up */
	0,				/* time to sleep after each failure */
	SIG_IGN,			/* no routine to catch signals */
};

char    *cpidfil    = CPIDFIL;
char    *metoofil   = METOOFIL;
char    *logfil     = LOGFIL;
char    *ologfil    = OLOGFIL;
char    *upfile	    = UPFILE;
char    *infofil    = INFOFIL;
char    *recfil     = RECFIL;
char    *slisfil    = SLISFIL;
char    *erlogfil   = ERLOGFIL;		/* for erlogf() from glib.a */

short   maxtorp     = MAXTORP;
short   maxfspd     = MAXFSPD;
short   maxsspd     = MAXSSPD;
short   maxdspd     = MAXDSPD;
short   maxtspd     = MAXTSPD;
short   maxpspd     = MAXPSPD;
double  velfact     = VELFACT;
double  torpdsq     = TORPDSQ;
double  depthdsq    = DEPTHDSQ;
double  probedsq    = PROBEDSQ;
double  r_rad       = R_RAD;
short   torpfuel    = TORPFUEL;
short   torpold     = TORPOLD;

short   hellosig    = HELLOSIG;
short   byesig      = BYESIG;
short   xmax        = XMAX;
short   ymax        = YMAX;
short   pdiam       = PDIAM;
short   pxdiam      = PXDIAM;
short   prad        = PRAD;
short   pxrad       = PXRAD;
short   pradsq      = PRADSQ;
short   cmdy        = CMDY;
double  perang2     = PERANG2;
short   cadjust     = CADJUST;
short	subchars[][2]	= {	/* sub characteristics */
/* surfaced	submerged 	(see SC defines in convoy.h) */
	1,	1,	/* can fire torpedos */
	1,	1,	/* can use periscope */
	1,	1,	/* can use radar */
	1,	1,	/* appear on maps if using radar */
	1,	1,	/* appear in radar if using radar */
	1,	0,	/* appear on maps in any case */
	1,	1,	/* appear in radar in any case */
	100,	50,	/* relative speed * 100 */
};


#ifdef NODELAY
short   ttopenmode  = O_RDWR | O_NDELAY;
#else
short   ttopenmode  = 2;
#endif

struct  plyrstr p[MAXPLYR];
struct  frstr   f[MAXFRTR];
struct  torpstr t[MAXTORP];

/***************************************************************************/
/*****************  System dependent routines ******************************/
/***************************************************************************/

char	*
ininit()		/* return filename of source of commands */
{
#ifdef	TWOPROC
	register char *cp, *sp;
	static char buf[32];
	extern char *ttyname();

	cp = ttyname(2);
	for (sp = cp; *cp; )
	    if (*cp++ == '/')
		sp = cp;
	sprintf(buf, "/tmp/cnvy.%s", sp);
	cmdfil = buf;
	close(creat(buf, 0666));
	chmod(buf, 0666);
	return(buf);
#else
	extern char *ttyname();

	return(ttyname(2));
#endif
}

char	*
outinit()		/* return filename of destination of graphics */
{
	extern char *ttyname();

	return(ttyname(2));
}

killtime()		/* either pause() or put commands in temp file */
{
#ifdef	TWOPROC
	char buf[64];
	short i, cfh;

	if ((cfh = open(cmdfil, 1)) < 0) {
	    sprintf(buf, "Couldn't open %s for mode 1\n", cmdfil);
	    erlogf(buf);
	    return;
	}
	while ((i = read(0, buf, sizeof buf)) >= 0 && write(cfh, buf, i) == i);
#else
	pause();
#endif
}

#ifndef EMPTY
empty(fh)
{
	return(0);
}
#endif

long
timeoops(uid, stime, ntime)     /* called if play duration is unreasonable */
long    stime, ntime;
{
	char tbuf[16], fmtbuf[128];

#ifdef V6PRINTF
	sprintf(tbuf, "%D", stime);
#else
	sprintf(tbuf, "%ld", stime);
#endif
	sprintf(fmtbuf, "uid%d start=%s=%20.20s\n", uid, tbuf, ctime(&stime));
	erlogf(fmtbuf);
#ifdef V6PRINTF
	sprintf(tbuf, "%D", ntime);
#else
	sprintf(tbuf, "%ld", ntime);
#endif
	sprintf(fmtbuf, " quit=%s=%20.20s\n", tbuf, ctime(&ntime));
	erlogf(fmtbuf);
	return(ntime - 3600);
}

keyread(pp, buf, max)	/* return up to "max" chars -- very site dependent */
struct	plyrstr	*pp;	/* The place to do weird function keys, etc. */
char	*buf;
{
	return(read(pp->p_ifh, buf, max)); /* default; works for any type */
}
