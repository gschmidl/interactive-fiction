#include  <stdio.h>
/*
**      Routines keyed to uid
**   getunam(uid)   return log name
**   getugid(uid)   return gid
**   getudir(uid)   return login dir
**   getupw(uid)    return encrypted password
**   getuprg(uid)   return initial program to run
**      Routines keyed to log name
**   getnuid(nam)   return uid
**   getngid(nam)   return gid
**   getndir(nam)   return login dir
**   getnpw(nam)    return encrypted password
**   getnprg(uid)   return initial program to run
**
**      Compile: cc -O -c -q pswdsub.c; ar r ../glib.a pswdsub.o
*/
#define	BSD

#ifdef	BSD
#include	<pwd.h>
extern	struct	passwd	*getpwuid(), *getpwnam();
#else
struct    pswdstr {
	int     p_nuid;
	char    *p_name;
	char    *p_pswd;
	char    *p_uid;
	char    *p_gid;
	char    *p_pnum;
	char    *p_ldir;
	char    *p_prog;
};
#endif

#define ERROR   -1

char *
getunam(uid)             /* return logname of uid */
{
#ifdef	BSD
	return(getpwuid(uid)->pw_name);
#else
	struct pswdstr pswd;

	if (puidinit(uid, &pswd) == ERROR)
	    return("");
	return(pswd.p_name);
#endif
}

getugid(uid)             /* return gid of uid */
{
#ifdef	BSD
	return(getpwuid(uid)->pw_gid);
#else
	struct pswdstr pswd;

	if (puidinit(uid, &pswd) == ERROR)
	    return(ERROR);
	return(atoi(pswd.p_gid));
#endif
}

char *
getudir(uid)            /* return login directory */
{
#ifdef	BSD
	return(getpwuid(uid)->pw_dir);
#else
	struct pswdstr pswd;

	if (puidinit(uid, &pswd) == ERROR)
	    return("");
	return(pswd.p_ldir);
#endif
}

char *
getupw(uid)             /* return encrypted password */
{
#ifdef	BSD
	return(getpwuid(uid)->pw_passwd);
#else
	struct pswdstr pswd;

	if (puidinit(uid, &pswd) == ERROR)
	    return("");
	return(pswd.p_pswd);
#endif
}

char *
getuprg(uid)            /* return initial program to run */
{
#ifdef	BSD
	return(getpwuid(uid)->pw_shell);
#else
	struct pswdstr pswd;

	if (puidinit(uid, &pswd) == ERROR)
	    return("");
	return(pswd.p_prog);
#endif
}

getnuid(name)            /* return uid for logname "name" */
char *name;
{
#ifdef	BSD
	return(getpwnam(name)->pw_uid);
#else
	struct pswdstr pswd;

	if (pnaminit(name, &pswd) == ERROR)
	    return(ERROR);
	return(pswd.p_nuid);
#endif
}

getngid(name)            /* return gid for logname "name" */
char *name;
{
#ifdef	BSD
	return(getpwnam(name)->pw_gid);
#else
	struct pswdstr pswd;

	if (pnaminit(name, &pswd) == ERROR)
	    return(ERROR);
	return(atoi(pswd.p_gid));
#endif
}

char *
getndir(name)           /* return login dir for logname "name" */
char *name;
{
#ifdef	BSD
	return(getpwnam(name)->pw_dir);
#else
	struct pswdstr pswd;

	if (pnaminit(name, &pswd) == ERROR)
	    return("");
	return(pswd.p_ldir);
#endif
}

char *
getnpw(name)            /* return encrypted password for logname "name" */
char *name;
{
#ifdef	BSD
	return(getpwnam(name)->pw_passwd);
#else
	struct pswdstr pswd;

	if (pnaminit(name, &pswd) == ERROR)
	    return("");
	return(pswd.p_pswd);
#endif
}

char *
getnprg(name)          /* return initial program to run for logname "name" */
char *name;
{
#ifdef	BSD
	return(getpwnam(name)->pw_shell);
#else
	struct pswdstr pswd;

	if (pnaminit(name, &pswd) == ERROR)
	    return("");
	return(pswd.p_prog);
#endif
}

#ifndef	BSD

puidinit(uid, pp)        /* initialize pswdstr struct pp for uid */
struct    pswdstr   *pp;
{
	static char buf[128] 0;

	if (*buf != '\0' && pp->p_nuid == uid && uid != 0)
	    return(pp->p_nuid);
	if (getpw(uid, buf) == 1)
	    return(ERROR);
	return(pparse(buf, pp));
}

pnaminit(name, pp)       /* initialize pswdstr struct pp for name */
char *name;
struct    pswdstr   *pp;
{
	register char *cp, *np;
	static char buf[128];
	FILE *p;

	p = fopen("/etc/passwd", "r");
	if (p == NULL) {
	    perror("/etc/passwd");
	    return(ERROR);
	}
	while (fgets(buf, 128, p) != NULL) {
	    cp = buf;
	    np = name;
	    for (;;) {
		if (*cp != *np) {
		    if (*cp == ':' && *np == '\0') {
			fclose(p);
			return(pparse(buf, pp));
		    }
		    break;
		}
		cp++;
		np++;
	    }
	}
	fclose(p);
	return(ERROR);
}

pparse(buf, pp)
char *buf;
struct    pswdstr   *pp;
{
	register char *cp, **b;

	cp = buf;
	for (b = &pp->p_name; b <= &pp->p_prog; b++) {
	    *b = cp;
	    while (*cp != ':' && *cp != '\0' && *cp != '\n')
		cp++;
	    if (*cp != '\0')
		*cp++ = '\0';
	}
	pp->p_nuid = atoi(pp->p_uid);
	return(pp->p_nuid);
}
#endif
