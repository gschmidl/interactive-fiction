#include    "wardef.h"
#include    <stdio.h>

static  char    *warid = "@(#)war.c	1.9  last mod 5/3/82 -- (c) psl 1976";
static  char    *wardefid = DEF_SCCS;
static  char    comfil[26];
static  int     childproc;

#ifndef EMPTY
#ifndef NODELAY
#ifndef TWOPROC
GEEZ, LOUISE!  DON'T YOU GUYS EVER READ THE INSTRUCTIONS OR NOTHIN'?
#endif
#endif
#endif

/* Return the location of the capital */
cap_pos()
{
        register int n;
	char *cp, buf[128];

	printf("Where would you like your Capital? ");
	fgets(cp = buf, sizeof buf, stdin);
	return(get_coords(&cp));
}

/* Return the number of paratroops */
num_para(bucks)
{
	register int n;
	char buf[128];

	n = bucks / PARA_COST;
	printf("How many paratroops do you want to employ, (max %d)? ", n);
	fgets(buf, sizeof buf, stdin);
	return(atoi(buf));
}

/* Return the number of artillery shells */
num_shell(bucks)
{
	register int n;
	char buf[128];

	n = bucks / SHELL_COST;
	printf("How many shells do you want to buy, (max %d)? ", n);
	fgets(buf, sizeof buf, stdin);
	return(atoi(buf));
}

/* Start up the input process (if necessary) */
start_cmnd()
{
#ifdef	EMPTY
	/* IS/1 system & others with the empty() system call */
	cfh = 0;	/* commands arrive on std input */
#endif
#ifdef NODELAY
	/* Bell V3.0 system & others with NODELAY reads */
#include <fcntl.h>	/* V3.0 include file */
	cfh = open(ttyname(0), O_RDONLY | O_NDELAY);
#endif
#ifdef TWOPROC
	/* systems with neither empty() nor NODELAY open() */
        char buf[64];

        sprintf(comfil, "warmnd.%d", onum);
        if ((cfh = creat(comfil, 0600)) < 0) {
	    fprintf(stderr, "Can't creat command file '%s'\n", comfil);
            exit(3);
        }
        if ((childproc = fork()) == 0) {
            for (;;) {
                read(0, buf, 64);
                buf[15] = '\n';
                write(cfh, buf, 16);
            }
        }
        close(cfh);
        cfh = open(comfil, 0);
#endif
	setmodes(2, 'e');				/* turn off echo */
	display(DISP_ALL);  /* to clean screen after questions were asked */
}

/* read the next command into bp; return the length */
get_command(bp)
char	*bp;
{
        register char *cp;

#ifdef	EMPTY
	if (empty(cfh))
	    return(0);
#endif
        if (read(cfh, bp, 16) <= 0)
            return(0);
        for (cp = bp; *cp && *cp != '\n'; cp++);
        *cp = '\0';
        return(cp - bp);
}

/* Reset modes, etc. just before exit */
cleanup()
{
	resetmodes(2);
#ifndef	EMPTY
#ifndef NODELAY
	if (childproc)
	    kill(childproc, 2);
        unlink(comfil);
#endif
#endif
}
