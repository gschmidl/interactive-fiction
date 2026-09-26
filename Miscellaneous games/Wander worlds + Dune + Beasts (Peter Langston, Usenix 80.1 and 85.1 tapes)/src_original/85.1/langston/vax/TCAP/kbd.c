#include    <kbd.h>
/* Compile: cc -O -q -c kbd.c
**      KBD -- Keyboard lexer using termcap
**      Usage: count = kbd(kbdtype, rawbuf, &rbp, lexbuf)
**          inputs to kbd():
**              int  kbdtype        term type
**              char rawbuf[]       contains unlexed characters
**              char *rbp           points to next unused char in rawbuf
**              char lexbuf[]       gets output from lexer
**          outputs from kbd():
**              int count           number of chars left in lexbuf
**              char *rbp           reset for further raw collection
**              char lexbuf[]       filled with <NUL> terminated tokens
** (c) P. Langston 1981
*/

static  char    *sccsid = "%W% %G% -- PSL misc";

kbd(type, rawbuf, repp, lexbuf)
char    *rawbuf, **repp, *lexbuf;
{
	register char *rbp;                    /* next raw char to look at */
	register char *lbp;    /* next place to put a tentative lexed char */
	register int i;
	char *lep;                    /* 1 past last guaranteed lexed char */
	char *rep;                                 /* 1 past last raw char */
	char *rsp;                      /* beginning of current raw string */

	rep = *repp;
	lbp = lep = lexbuf;
	for (rbp = rsp = rawbuf; rbp < rep; ) {         /* go through raws */
	    *lbp++ = *rbp++;                  /* save as tentatively lexed */
	    *lbp = '\0';
	    i = kbdmtch(type, lep);
	    if (i < 0) {                                  /* found a token */
		*lep++ = i;
		lbp = lep;                /* read chars in after new token */
		rsp = rbp;                 /* forget interpreted raw chars */
	    } else if (i == 0) {            /* no possible token */
		*lep++ = *rsp++;            /* this char not part of a key */
		lbp = lep;
		rbp = rsp;     /* try for a match starting at new rbp, lbp */
	    }
	}
	for (rbp = rawbuf; rsp < rep; )                /* save unused raws */
	    *rbp++ = *rsp++;
	*repp = rbp;                         /* reset rawbuf input pointer */
	*lep = '\0';                              /* terminate lexed chars */
	return(lep - lexbuf);                              /* return count */
}
