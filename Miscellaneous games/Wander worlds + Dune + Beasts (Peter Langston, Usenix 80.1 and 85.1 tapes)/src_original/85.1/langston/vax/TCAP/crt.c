#include    <crt.h>
/*
**      CRT -- Crt capabilities routine (uses /etc/termcap)
*/

/*#define DEBUG         /* define this for debugging */
struct  crtstr  crts[MAXCRT];

extern  char    PC;

extern  char    *tgoto(), *tcgstr(), *copy();

crtinit(tname)             /* return type # or -1 if tname not in termcap, */
char    *tname;             /* -2 if no termcap file, -3 if too many types */
{
        register char i;
        char buf[1024], sname[16], *cp, *bp, **cpp;

        for (i = 0; crts[i].crt_name[0]; i++)        /* already know type? */
            if (strcmp(tname, crts[i].crt_name) == 0)
                return(i);
        if ((i = tcgent(buf, tname)) <= 0)
            return(i - 1);
        for (cp = buf; (i = *cp) && i != '|' && i != ':'; cp++);
        *cp = '\0';
        strcpy(sname, buf);                    /* first name is "std" name */
        *cp = i;
        for (i = 0; crts[i].crt_name[0]; i++)        /* already know type? */
            if (strcmp(sname, crts[i].crt_name) == 0)
                return(i);
        if (i >= MAXCRT)
            return(-3);
        strcpy(crts[i].crt_name, sname);
        cp = crts[i].crt_buf;
	if (tcgstr("pc", &cp, buf))    /* pad character */
	    PC = *cp;
        cp = crts[i].crt_buf;
#ifdef DEBUG
        *cp++ = '?';
#endif
        *cp++ = '\0';
        crts[i].crt_al = tcgstr("al", &cp, buf);    /* insert line */
        crts[i].crt_ce = tcgstr("ce", &cp, buf);    /* clear to e-o-l */
        crts[i].crt_cl = tcgstr("cl", &cp, buf);    /* clear screen */
        crts[i].crt_cm = tcgstr("cm", &cp, buf);    /* cursor movement */
        crts[i].crt_sw = tcgnum("co", buf);         /* screen width */
        crts[i].crt_dl = tcgstr("dl", &cp, buf);    /* delete line */
        crts[i].crt_sh = tcgnum("li", buf);         /* screen height */
        crts[i].crt_mb = tcgstr("mb", &cp, buf);    /* mode blink */
        crts[i].crt_me = tcgstr("me", &cp, buf);    /* end blink, bold, etc. */
        crts[i].crt_rp = tcgflg("rp", buf);         /* does repeat exist? */
        crts[i].crt_re = tcgstr("re", &cp, buf);    /* repeat end */
        crts[i].crt_rs = tcgstr("rs", &cp, buf);    /* repeat start */
        crts[i].crt_se = tcgstr("se", &cp, buf);    /* standout (reverse) end */
        crts[i].crt_so = tcgstr("so", &cp, buf);    /* standout (reverse) */
        crts[i].crt_te = tcgstr("te", &cp, buf);    /* cursor motion de-init */
        crts[i].crt_ti = tcgstr("ti", &cp, buf);    /* cursor motion init */
        crts[i].crt_ue = tcgstr("ue", &cp, buf);    /* underline end */
        crts[i].crt_us = tcgstr("us", &cp, buf);    /* underline start */
        bp = cp = tgoto(crts[i].crt_cm, 40, 12);/* calc cmlen */
        while (*cp)
            cp++;
        crts[i].crt_cmlen = cp - bp;
        for (cpp = &crts[i].CRT_FSCAP; cpp <= &crts[i].CRT_LSCAP; cpp++)
            if (*cpp == 0)
                *cpp = crts[i].crt_buf;
        return(i);
}

/*VARARGS2*/
char    *
crt(type, x, y, z)
{
        register char *cp, *sp;
        register struct crtstr *tp;
        static char cursbuf[4][256], next;
        extern char *rptguts();

        tp = &crts[type];
        next &= 3;
        cp = cursbuf[next];
        if (x < 0) {
            if (x == C_CLEAR) {
                return(tp->crt_cl);
            } else if (x == C_LINE_INS) {
                return(tp->crt_al);
            } else if (x == C_LINE_DEL) {
                return(tp->crt_dl);
            } else if (x == C_LINE_ERA) {
                return(tp->crt_ce);
            } else if (x == C_INIT) {
                return(tp->crt_ti);
            } else if (x == C_FINI) {
                return(tp->crt_te);
            } else if (x == C_REPEAT) {
                if (tp->crt_rp == 0) {
                    while (--z >= 0)
                        *cp++ = y;
                } else {
                    cp = copy(rptguts(tp->crt_rs, z), cp);
                    *cp++ = y;
                    cp = copy(rptguts(tp->crt_re, z), cp);
                }
            } else if (x == C_FIELD) {
                if (y == F_NORMAL) {
                    cp = copy(tp->crt_ue, cp);
                    cp = copy(tp->crt_me, cp);
                    cp = copy(tp->crt_se, cp);
                } else {
                    if (y & F_UNDER)
                        cp = copy(tp->crt_us, cp);
                    if (y & F_BLINK)
                        cp = copy(tp->crt_mb, cp);
                    if (y & F_STANDOUT)
                        cp = copy(tp->crt_so, cp);
                }
            } else if (x == C_GRAPHICS) {
		*cp++ = "-|+++++++++"[y];
	    }
	    *cp = '\0';
            return(cursbuf[next++]);
        } else {
            for (sp = tgoto(tp->crt_cm, x, y); *cp++ = *sp++; );
            return(cursbuf[next++]);
        }
	/*NOTREACHED*/
}

/*
** Routine to do repeats
** "rptstr" contains printf-like escapes to allow insertion of the "count".
**      %d      as in printf
**      %2      like %2d
**      %3      like %3d
**      %.      gives %c
** The codes below affect the count but don't generate characters:
**      %+x     adds x to count (i.e. %+^A adds one to count)
**      %-x     subtracts x from count
**      %Q      where Q is anything else gives Q, (e.g. %% gives %)
** Example: the Ann Arbor Ambassador (ANSI std) uses :rp:rs=:re=%-^A\E[%db:
** which means 25 X's comes out as:  X<ESC>[24b  (which does work)
*/
char *
rptguts(rptstr, count)
char    *rptstr;
register int count;
{
        register char *bp, *cp = rptstr;
        register int c;
        static char buf[16];

        bp = buf;
        while (c = *cp++) {
            if (c != '%') {
                *bp++ = c;
                continue;
            }
            switch (c = *cp++) {
            case 'd':
                if (count < 10)
                    goto one;
                if (count < 100)
                    goto two;
            case '3':
                *bp++ = (count / 100) | '0';
                count %= 100;
            case '2':
two:            *bp++ = count / 10 | '0';
one:            *bp++ = count % 10 | '0';
                break;
            case '.':
                *bp++ = count;
                break;
            case '+':
                count += *cp++;
                break;
            case '-':
                count -= *cp++;
                break;
            default:
                *bp++ = c;
                break;
            }
        }
        *bp = '\0';
        return (buf);
}
