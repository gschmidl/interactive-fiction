#include    <stdio.h>
#include    <crt.h>
/*
**      BOGSYS -- System specific module for BOG
*/

static  char    *whatsccs = "@(#)bogsys.c	1.7 5/31/84 -- (c) psl 1979";

	/* You must define one of these three */
/* #define NODELAY                 /* define this for V3.0 & others */
/* #define EMPTY		   /* define this if you have empty() */
/* #define TWOPROC                 /* define this if nothing else fits */

#ifndef NODELAY
#ifndef EMPTY
#ifndef TWOPROC
Geez!  Can't you read?  You MUST define one of them three!  Wake up, huh?
#endif
#endif
#endif

#ifdef	NODELAY
#include    <fcntl.h>
#endif

#include	"../gamesdef.h"
#define BOGPATH(x)	GAMESPATH(BOG/x)

char    *dictionary = BOGPATH(bogwds);    /* computer's default vocabulary */
char    *monfil     = BOGPATH(bog.mon);               /* new words learned */
char    *infofil    = BOGPATH(boginfo);                    /* instructions */
char    *ctmpfmt     = "/tmp/bogc%d";      /* format for name of temp file */
char    *htmpfmt     = "/tmp/bogh%d";      /* format for name of temp file */
#ifdef  TWOPROC
char    *tmpfmt     = "/tmp/bog%d";        /* format for name of temp file */
char    tmpfil[32];                                /* keystrokes kept here */
short   kidpid      = 0;                           /* pid of input process */
#endif

#define DEF_SECS    180
#define DEF_SIZE    4

short   def_secs    = DEF_SECS;                    /* default secs / board */
short   def_size    = DEF_SIZE;                      /* default board size */
short   tflag       = DEF_SECS;                            /* secs / board */
short   n_cubes     = DEF_SIZE * DEF_SIZE;      /* def num of letter cubes */

#ifdef	NODELAY
short   ndfh;                            /* file handle for no-delay reads */
#endif

#ifdef  TWOPROC
FILE    *tpfp;                                /* file pointer to temp file */
#endif

#define WAIT    0                     /* used by accwds(), `non-busy wait' */
#define NOWAIT  1                         /* used by accwds(), `busy wait' */

extern  struct  crtstr  crts[];                      /* from crt.h & crt() */

char        *ndfgets();
extern char *copy();

ndopen()    /* called to set up non-delay reads */
{
#ifdef	NODELAY
	if ((ndfh = open("/dev/tty", O_RDONLY | O_NDELAY)) < 0) {
	    perror("Failed to open `/dev/tty'");
	    done(3);
	}
#endif
#ifdef  TWOPROC
	char buf[64];

	sprintf(tmpfil, tmpfmt, getpid());
	if ((tpfp = fopen(tmpfil, "w")) == NULL) {
	    perror(tmpfil);
	    done(3);
	}
	switch (kidpid = fork()) {
	case -1:
	    perror("fork failed");
	    done(3);
	case 0:
	    setbuf(tpfp, NULL);
	    while (fgets(buf, sizeof buf, stdin) != NULL)
		fputs(buf, tpfp);
	    fputs("q\nq\n", tpfp);
	    exit(0);                        /* this shold NOT be "done(0)" */
	default:
	    fclose(tpfp);
	    if ((tpfp = fopen(tmpfil, "r")) == NULL) {
		perror(tmpfil);
		kill(kidpid, 15);
	    }
	}
#endif
}

wordget(buf, mode)
char    *buf;
{
#ifdef	EMPTY
	if (mode == NOWAIT && empty(stdin->_file))
	    return(0);
	if (fgets(buf, 64, stdin) == NULL) {
	    clearerr(stdin);
	    return(-2);
	}
#endif
#ifdef	NODELAY
	register int i;

	if (mode == NOWAIT) {
	    if ((i = ndfgets(buf, 64)) == NULL)
		return(-2);
	    else if (i == 0)
		return(0);
	} else
	    fgets(buf, 64, stdin);
#endif
#ifdef  TWOPROC
	while (fgets(buf, 64, tpfp) == NULL) {
	    clearerr(tpfp);
	    if (mode == NOWAIT)
		return(0);
	    if (*buf == '\04')
		return(-2);
	    sleep(1);                                  /* this is optional */
	}
#endif
	return(*buf == '\n'? -1 : 1);
}

#ifdef	NODELAY
char	*
ndfgets(buf, len)
char	*buf;
{
	register int i;
	static char rbuf[64], *rbp;

	if (rbp == 0)
	    rbp = rbuf;
	while ((i = read(ndfh, rbp, 1)) == 1) {
	    if (*rbp++ == '\n') {
		*rbp = '\0';
		copy(rbuf, buf);
		rbp = rbuf;
		return(rbp);
	    }
	}
	return(i == 0? 0 : NULL);
}
#endif

done(exstat)
{
#ifdef  TWOPROC
	if (kidpid)
	    kill(kidpid, 15);
	if (*tmpfil)
	    unlink(tmpfil);
#endif
	exit(exstat);
}
