#include    <kbd.h>
/* Compile: cc -O -q -c kbdinit.c
**      KBDINIT -- Set up for kbd() & kbdtok()
**      Usage: kbdtype = kbdinit(name)
**          inputs to kbdinit():
**              char *name          term name
**          output from kbdinit():
**              int  kbdtype        term type
** (c) P. Langston 1981
*/

static  char    *sccsid = "%W% %G% -- PSL misc";

struct  kbdstr  kbds[MAXKBD];

extern  char    *tcgstr();

kbdinit(tname)             /* return type # or -1 if tname not in termcap, */
char    *tname;             /* -2 if no termcap file, -3 if too many types */
{
	register char i, kbdtype;
	char buf[1024], sname[16], *cp, *bp;

	for (kbdtype = 0; kbds[kbdtype].kbd_name[0]; kbdtype++)
	    if (strcmp(tname, kbds[kbdtype].kbd_name) == 0)
		return(kbdtype);                   /* we already know type */
	if ((i = tcgent(buf, tname)) <= 0)
	    return(i - 1);
	for (cp = buf; (i = *cp) && i != '|' && i != ':'; cp++);
	*cp = '\0';
	strcpy(sname, buf);                    /* first name is "std" name */
	*cp = i;
	for (kbdtype = 0; kbds[kbdtype].kbd_name[0]; kbdtype++)
	    if (strcmp(sname, kbds[kbdtype].kbd_name) == 0)
		return(kbdtype);                   /* we already know type */
	if (kbdtype >= MAXKBD)
	    return(-3);
	strcpy(kbds[kbdtype].kbd_name, sname);
	cp = kbds[kbdtype].kbd_buf;
	*cp++ = 0377;                       /* for missing termcap strings */
	*cp++ = '\0';
	kbds[kbdtype].kbd_ke = tcgstr("ke", &cp, buf);
	kbds[kbdtype].kbd_ks = tcgstr("ks", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F0] = tcgstr("k0", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F1] = tcgstr("k1", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F2] = tcgstr("k2", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F3] = tcgstr("k3", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F4] = tcgstr("k4", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F5] = tcgstr("k5", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F6] = tcgstr("k6", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F7] = tcgstr("k7", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F8] = tcgstr("k8", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_F9] = tcgstr("k9", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_BS] = tcgstr("kb", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_DARROW] = tcgstr("kd", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_HOME] = tcgstr("kh", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_LARROW] = tcgstr("kl", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_RARROW] = tcgstr("kr", &cp, buf);
	kbds[kbdtype].kbd_str[-1 - K_UARROW] = tcgstr("ku", &cp, buf);
	for (i = KBD_TOKS; --i >= 0; )
	    if (kbds[kbdtype].kbd_str[i] == 0)
		kbds[kbdtype].kbd_str[i] = kbds[kbdtype].kbd_buf;
	return(kbdtype);
}
