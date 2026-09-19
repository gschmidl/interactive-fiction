#include "sd_def.h"

/*
**      SD_SYS -- System dependent routines for Stardrek
**              (c) Peter S. Langston 1981
*/

static  char    *sccsid = "@(#)sd_sys.c	1.4  3/14/84 -- (c) psl 1981";

#ifdef NOEMPTY
extern  char    *hlm, *helm_prog;
extern  int     helmp;
#endif

helmcheck()               /* make sure everything's okay in 2 proc version */
{
#ifdef NOEMPTY
	if (stat(hlm, junk) == -1) {
	    alarum("The helm has disappeared!");
	    bye(0);
	}
	if (fstat(1, junk) == -1
	 || (junk[3] >> 8) & 0377 == 0) {
	    alarum("Aliens have taken over the bridge!");
	    bye(0);
	}
#endif
}

killhelm()      /* terminate spare process if 2 process version */
{
#ifdef NOEMPTY
	if (helmp != 0)
	    kill(helmp, 2);
        setuid(rec.usr_uid);
        unlink(hlm);
#endif
}

starthelm()     /* start other process in 2 process version */
{
#ifdef NOEMPTY
	char ubuf[2];
	int i;

	i = getpid();
        if ((helmp = fork()) == 0) {
	    ubuf[0] = uid;
	    ubuf[1] = '\0';
	    execl(helm_prog, "! Star-Drek", "!", ubuf, 0);
	    printf("Exec(%s) failed!\n", helm_prog);
	    kill(i, 9);
	    resetmodes(2);
            exit(3);
        }
        printf("Helm checkout ... ");
	for (i = 0; i < 10 && (helm = open(hlm, 0)) < 0; i++)
	    sleep(6);
	if (helm < 0) {
            printf("### FAILED ###\n");
	    resetmodes(2);
	    exit(3);
	}
#endif
}

getnxtchar()           /* return next command char from helm or -1 if none */
{
	char buf[1];

#ifdef NOEMPTY
	if (read(helm, buf, 1) <= 0)
	    return(-1);
#else
	if (empty(0))
	    return(-1);
	read(0, buf, 1);
#endif
	return(*buf & 0177);
}

snoozhelm()         /* if 2 proc version put helm to sleep */
{
#ifdef NOEMPTY
	kill(helmp, IGN_SIG);
	sleep(2);
#endif
}

wakehelm()                 /* wake it up if 2 proc version */
{
#ifdef NOEMPTY
	kill(helmp, ALARM_SIG);
	sleep(2);
#endif
}
