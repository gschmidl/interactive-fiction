#include    <kbd.h>
/* Compile: cc -O -q -c kbdmtch.c
**      KBDMTCH -- Keyboard string matcher
**      Usage: i = kbdmtch(type, str)
**          inputs to kbdmtch():
**              int  type       term type
**              char *str       characters to be matched
**          output from kbdmtch():
**              int  i          token <0 if a match is found
**                              0 if no match is possible
**                              1 if not enough characters to tell yet
** (c) P. Langston 1981
*/

static  char    *sccsid = "%W% %G% -- PSL misc";

kbdmtch(type, str)
char    *str;
{
	register char *sp, *kp;
	register int i;

	for (i = KBD_TOKS; --i >= 0; ) {
	    for (sp = str, kp = kbds[type].kbd_str[i]; *sp++ == *kp; )
		if (*kp++ == '\0')
		    return(-1 - i);
	    if (*--sp == '\0')
		return(sp > str? 1 : 0);
	}
	return(0);
}
