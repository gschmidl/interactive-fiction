/*
**	ANONMAIL is a sleazy hack.
**	It tries to subvert an important aspect of any system that
**	involves cooperation of independent individuals: personal
**	responsibility for one's actions.  Unfortunately there are
**	situations in games where revealing the sender of a piece
**	of mail really spoils the fun!
**	So, once again, with innocent hopes for good will (and so forth)
**	we put the loaded guns to our heads...
**		"We have met the enemy and he is us!"
**						-- Walt Kelly
*/

#define	RMAIL

#ifdef	RMAIL
/*
** This version of anonmail() uses a "uucp"-like address to disguise itself.
** This will not work unless the rmail program is dumb enough to believe
** a "remote from" line.
*/

anonmail(from, to, text)
char	*from, *to, *text;
{
	register char *cp, *ep;
	register int count;
	char *mpsave, *masave, *envsave, *bangback;
	char buf[128];
	int pfh[2];
	long now;
	extern char *mailprog, *mailarg;	/* from sendmsg.c */
	extern char **environ;			/* provided by crt0.o */
	extern char *copy();

	time(&now);
	envsave = *environ;
	*environ = 0;			/* to avoid cuteness with NAME */
	mpsave = mailprog;
	masave = mailarg;
	mailprog = "/bin/rmail";
	pipe(pfh);		/* this fails for very long messages */
	for (cp = ep = from; *cp; cp++)
	    if (*cp == '!')
		ep = cp;
	if (*ep == '!') {	/* it's already a uucp address */
	    bangback = ep;	/* save pointer to put the bang back later */
	    *ep++ = '\0';
	    cp = copy("From ", buf);
	    cp = copy(ep, cp);
	    ep = from;
	} else {		/* make a uucp-like address */
	    bangback = 0;
	    cp = copy("From \b\b", buf);
	    cp = copy(from, cp);
	    ep = "x";
	}
	*cp++ = ' ';
	cp = copy(ctime(&now), cp) - 1;
	cp = copy(" remote from ", cp);
	cp = copy(ep, cp);
	*cp++ = '\n';
	write(pfh[1], buf, cp - buf);
	count = 4095 - (cp - buf);
	if (bangback)		/* replace the ! that was nulled earlier */
	    *bangback = '!';
	for (cp = text; *cp && --count > 0; cp++);
	if (cp[-1] != '\n' && count > 0)
	    *cp++ = '\n';
	write(pfh[1], text, cp - text);
	close(pfh[1]);
	sendmsg(to, pfh[0]);
	close(pfh[0]);
	mailprog = mpsave;	/* better safe than sorry */
	mailarg = masave;
	*environ = envsave;
}
#endif	RMAIL

#ifdef	DELIVERMAIL
/*
** This version of anonmail() uses a "uucp" address to disguise itself.
** This will not work unless the mail program is dumb enough to believe
** a "-fuucp!address" argument.
*/

anonmail(from, to, text)
char	*from, *to, *text;
{
	char *mpsave, *masave, *envsave;
	char argbuf[128];
	extern char *mailprog, *mailarg;	/* from sendmsg.c */
	extern char **environ;			/* provided by crt0.o */

	envsave = *environ;
	*environ = 0;			/* to avoid cuteness with NAME */
	mpsave = mailprog;
	masave = mailarg;
	mailprog = "/etc/delivermail";
	copy(from, copy("-f!\b", mailarg = argbuf));
	sendtext(to, text);
	mailprog = mpsave;	/* better safe than sorry */
	mailarg = masave;
	*environ = envsave;
}
#endif	DELIVERMAIL

#ifdef	SM
/*
** This version of anonmail() uses a local mailer (sm) which can be fooled
** into changing the from name with an interface program.
*/
#include	"../gamesdef.h"

anonmail(from, to, text)
char	*from, *to, *text;
{
	char *mpsave, *masave, *envsave;
	char argbuf[128];
	extern char *mailprog, *mailarg;	/* from sendmsg.c */
	extern char **environ;			/* provided by crt0.o */

	envsave = *environ;
	*environ = 0;			/* to avoid cuteness with NAME */
	mpsave = mailprog;
	masave = mailarg;
	mailprog = GAMESPATH(hidemail);
	copy(from, copy("-f", mailarg = argbuf));
	sendtext(to, text);
	mailprog = mpsave;	/* better safe than sorry */
	mailarg = masave;
	*environ = envsave;
}
#endif	SM

#ifdef	VanillaMail
/*
** This version of anonmail() uses the setuid function to disguise itself.
** This will not work unless the mail program is dumb enough to believe
** in "real" uids.
*/
#define	ORACLE_UID	1	/* pick some acct like "daemon" or "Oracle" */

anonmail(from, to, text)
char	*from, *to, *text;
{
	int st;

	if (fork() == 0) {
	    setuid(ORACLE_UID);		/* disguise myself */
	    sendtext(to, text);
	    exit(0);
	}
	wait(&st);
}
#endif	VanillaMail

#ifdef	PWB
/*
** This version of anonmail() uses the logpost() routine to disguise itself.
** This will not work unless it is SETUID ROOT.
*/

struct user {				/* this is for logpost() */
	char logname[8];
	char logdir[22];
	char tty;
	char acct;
} user;

anonmail(from, to, text)
char	*from, *to, *text;
{
	copy(from, user.logname);
	copy("/tmp", user.logdir);
	logpost(&user);			/* disguise myself */
	sendtext(to, text);
}
#endif	PWB
