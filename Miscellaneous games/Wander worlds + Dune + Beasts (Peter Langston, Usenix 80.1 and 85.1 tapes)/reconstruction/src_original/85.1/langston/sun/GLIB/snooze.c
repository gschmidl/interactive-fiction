/*
**	SNOOZE -- A short sleep routine, argument in milliseconds.
*/

/*#define	BSD	/**/

#ifdef BSD	/* This works using interval timers */

#include <sys/time.h>
#include <signal.h>

#define	ALRMASK	(1<<((SIGALRM)-1))
/*#define mask(s) (1<<((s)-1))*/
#define setvec(vec, a) { vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0; }

static int ringring;

static
snort()
{
        ringring = 1;
}

snooze(msec)
unsigned msec;
{
        register struct itimerval *itp;
        int omask;
        struct itimerval itv, oitv;
        struct sigvec vec, ovec;

        if (msec == 0)
	    return;
        itp = &itv;
        timerclear(&itp->it_interval);
        timerclear(&itp->it_value);
        if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
	    return;
        setvec(ovec, SIG_DFL);
/*        omask = sigblock(mask(SIGALRM));
*/        omask = sigblock(ALRMASK);
        itp->it_value.tv_sec = msec / 1000;
	itp->it_value.tv_usec = (msec % 1000) * 1000;
        if (timerisset(&oitv.it_value)) {
	    if (oitv.it_value.tv_sec >= itp->it_value.tv_sec) {
		if (oitv.it_value.tv_sec == itp->it_value.tv_sec
		 && oitv.it_value.tv_usec > itp->it_value.tv_usec)
		    oitv.it_value.tv_usec -= itp->it_value.tv_usec;
		oitv.it_value.tv_sec -= itp->it_value.tv_sec;
	    } else {
		/* An uncomfortable situation at best */
		itp->it_value = oitv.it_value;
		oitv.it_value.tv_sec = 1;
		oitv.it_value.tv_usec = 0;
	    }
        }
        setvec(vec, snort);
        ringring = 0;
        (void) sigvec(SIGALRM, &vec, &ovec);
        (void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
        while (!ringring)
/*	    sigpause(omask &~ mask(SIGALRM));
*/	    sigpause(omask &~ ALRMASK);
        (void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
        (void) setitimer(ITIMER_REAL, &oitv, (struct itimerval *)0);
}

#else

snooze(msec)
{
	register int sec;
	static int mscum;

	mscum += msec;
	sec = (msec + 500) / 1000;
	mscum = msec - 1000 * sec;
	if (sec > 0)
	    sleep(sec);
}

#endif
