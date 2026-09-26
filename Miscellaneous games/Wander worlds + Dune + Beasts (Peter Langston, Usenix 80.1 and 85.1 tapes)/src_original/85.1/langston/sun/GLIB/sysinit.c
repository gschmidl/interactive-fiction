#include    <signal.h>		/* needed by 4.1BSD systems */
#include    <stdio.h>
/*
**	SYSINIT -- Do site-dependent initializations including imposing
**	    repressive measures, logging game usage, suppressing mindless
**	    buffering, hiding game names, etc.
*/

#include <sys/ioctl.h>	       /* needed by 4.1BSD systems */

static	long now;
static	int ottydisc;			       /* needed by 4.? BSD systems */

extern	char	*equal();

sysinit(game, argc, argv)
char	*game, *argv[];
{
	struct timeparts {
	    short   t_sec, t_min, t_hours, t_mday, t_month;
	    short   t_year, t_wday, t_yday, t_dst;
	} *tp, *localtime();
/***	int nttydisc = OTTYDISC;	       /* needed by 4.? BSD systems */

	setbuf(stdout, 0);        /* get rid of stdout buffering in 4.? BSD */
	signal(SIGTSTP, SIG_IGN);	       /* ignore ^Z sig 4.? BSD */
	signal(SIGCHLD, SIG_IGN);	       /* ignore weird sigs 4.? BSD */
	signal(SIGTTIN, SIG_IGN);	       /* ignore weird sigs 4.? BSD */
	signal(SIGTTOU, SIG_IGN);	       /* ignore weird sigs 4.? BSD */
	signal(SIGXCPU, SIG_IGN);	       /* ignore weird sigs 4.? BSD */
	signal(SIGXFSZ, SIG_IGN);	       /* ignore weird sigs 4.? BSD */
/***	ioctl(0, TIOCGETD, &ottydisc);	         /* save tty driver 4.? BSD */
/***	ioctl(0, TIOCSETD, &nttydisc);	      /* set old tty driver 4.? BSD */
	umask(0);				  /* needed if umask exists */
	time(&now);
	/* Do misc initializations, checking for too much load, etc. */
	if (equal(game, "Global Thermonuclear War")
	 || equal(game, "bigotry")) {	/* our only two repressions */
	    tp = localtime(&now);
	    if (tp->t_wday >= 1 && tp->t_wday <= 5
	     && tp->t_hours >= 9 && tp->t_hours <= 17) {
		printf("Sorry, this game is down from 9 to 6 on weekdays.\n");
		exit(1);
	    }
	} else if (equal(game, "empdis"))	/* the only one we hide */
	    hide(argc, argv);
}

sysfini(game)
char	*game;
{
/***	ioctl(0, TIOCSETD, &ottydisc);	        /* reset tty driver 4.? BSD */
}

char	*disguises[]	= {			 /* last one must be short */
	"readnews", "nroff", "emacs", "mail", "csh", "vi", "ed", "sh", "e",
	0,
};

/* BEWARE: The argv[]s (other than [0]) haven't been processed yet */
hide(argc, argv)			       /* hide identity of program */
char	*argv[];
{
	register char *cp;
	register int len, i;

	for (cp = argv[0]; *cp; *cp++ = ' ');	     /* wipe out real name */
	len = cp - argv[0];
	i = now % ((sizeof disguises / sizeof (char *)) - 2);
	for (; (cp = disguises[i]); i++) {    /* look for one short enough */
	    while (*cp)
		cp++;
	    if (len >= cp - disguises[i]) {
		copy(disguises[i], argv[0]);
		break;
	    }
	}
}
