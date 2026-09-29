#include    <sm_def.h>
#include    <crt.h>
/*
**      SM -- Screen Manager version for termcap, crtinit(), and crt()
**
**          All usages return the screen pointer or 0 on error.
**
**      sp = sm_init(type, fh, x0, y0, width, height)
**          Create a new buffer for the specified terminal type's screen.
**          Top left corner is at x0, y0; if width or height is zero the
**          maximum for that terminal type is used.
**
**      sp = sm_spcl(sp, SM_CLEAR)
**          Clear the screen associated with the specified screen handle.
**      sp = sm_spcl(sp, SM_ERASE)
**          Erase the window associated with the specified screen handle.
**      sp = sm_spcl(sp, SM_REFRESH)
**          Rewrite the window, assuming it's full of garbage.
**      sp = sm_spcl(sp, SM_FLUSH)
**          Flush buffered characters.
**      sp = sm_spcl(sp, SM_CTLMAP, map)
**          Use mappings in `map' for control characters
**      sp = sm_spcl(sp, SM_REDRAW)
**          Rewrite the window, assuming it has been cleared to blanks.
**      sp = sm_spcl(sp, SM_FINI)
**          Free the screen handle & send the clean-up string to the terminal.
**      sp = sm_spcl(sp, SM_REUSE, type, fh)
**          Reuse the screen handle and buffers on a new type & file handle,
**          (doesn't erase the window but does send the clean-up string).
**
**      sp = sm_char(sp, x, y, c)
**          Set loc x,y to be character c for the terminal associated
**          with screen handle pointer sp.  All screen changes are
**          buffered and are output when sm_spcl(sp, SM_FLUSH) is called.
**
**      c = sm_peek(sp, x, y);
**          Return the current char at x, y;
**
**      sp = sm_str(sp, x, y, str)
**          Put character string str in the buffer for the terminal
**          associated with screen handle pointer sp.  The first character
**          will be put at location x,y and following characters at x+1,y
**          x+2,y etc.  Characters that overflow the edge will be discarded.
**          All buffered screen changes are output when sm_spcl(sp, SM_FLUSH)
**          is called.
**
*/

static  char    *sccsid = "@(#)sm.c	1.4+ 10/17/83 -- PSL misc";

static  char    *bflsh();

extern  char    *crt(), *copy();
extern  struct  crtstr   crts[];

static	struct	smstr	*fsp	= 0;	/* most recently freed one, if any */

struct  smstr   *
sm_init(type, fh, x0, y0, width, height)
{
	register int sw, sh;
	struct smstr *sp;

	sw = crts[type].crt_sw;
	if (width == 0 || x0 + width > sw)
	    width = sw - x0;
	sh = crts[type].crt_sh;
	if (height == 0 || y0 + height > sh)
	    height = sh - y0;
	if (width <= 0 || height <= 0)
	    return(0);
	if (fsp != 0 && fsp->sm_sh * fsp->sm_sw == height * width) {
	    sp = fsp;		/* reuse the freed one */
	    fsp = 0;
	} else {
	    sp = (struct smstr *) sbrk(sizeof *sp + 2 * height * width);
	    if ((int) sp == -1) {	/* no space left? */
		if (fsp != 0 && fsp->sm_sh * fsp->sm_sw >= height * width) {
		    sp = fsp;		/* reuse the freed one */
		    fsp = 0;
		} else
		    return(0);
	    }
	}
	sp->sm_type = type;
	sp->sm_fh = fh;
	sp->sm_x0 = x0;
	sp->sm_y0 = y0;
	sp->sm_sw = width;
	sp->sm_sh = height;
	sp->sm_cntab = 0;
	sp->sm_lox = sp->sm_hix = sp->sm_loy = sp->sm_hiy = 0;
	sp->sm_oscrn = (char *) sp + sizeof *sp;
	sp->sm_nscrn = sp->sm_oscrn + height * width;
	sp->sm_ospeed = getspd(fh);
	tputsfh(crt(type, C_INIT), height, fh, sp->sm_ospeed);
	return(sp);
}

/*VARARGS 2*/
struct  smstr   *
sm_spcl(sp, op, arg1, arg2)
struct  smstr   *sp;
{
	register char *cp;
	register int i, j;
	char buf[256], *bep, *oscrn, *nscrn, **map;
	int type, sw, sh, cmlen;
	int jj, k, lastk, fi, li, ci;

	if (sp == 0)
	    return(0);
	type = sp->sm_type;
	sw = sp->sm_sw;
	sh = sp->sm_sh;
	oscrn = sp->sm_oscrn;
	nscrn = sp->sm_nscrn;
	switch (op) {
	case SM_CLEAR:
	    tputsfh(crt(type, C_CLEAR), sh, sp->sm_fh, sp->sm_ospeed);
	    for (i = sw * sh; --i >= 0; )
		oscrn[i] = nscrn[i] = 040;
	    sp->sm_lox = sp->sm_hix = sp->sm_loy = sp->sm_hiy = 0;
	    break;
	case SM_ERASE:
	    for (i = sw * sh; --i >= 0; ) {
		nscrn[i] = 040;
		if (oscrn[i] != 040) {
		    j = i % sw;
		    if (j < sp->sm_lox)
			sp->sm_lox = j;
		    if (j >= sp->sm_hix)
			sp->sm_hix = j + 1;
		    j = i / sw;
		    if (j < sp->sm_loy)
			sp->sm_loy = j;
		    if (j >= sp->sm_hiy)
			sp->sm_hiy = j + 1;
		}
	    }
	    break;
	case SM_REFRESH:
	    for (i = sw * sh - 1; --i >= 0; )
		oscrn[i] = nscrn[i] + 1;   /* avoid last char on last line */
	    goto setlims;
	case SM_REDRAW:
	    for (i = sw * sh - 1; --i >= 0; )
		oscrn[i] = ' ';		   /* avoid last char on last line */
    setlims:
	    sp->sm_lox = sp->sm_loy = 0;
	    sp->sm_hix = sw;
	    sp->sm_hiy = sh;                    /* fall through into flush */
	case SM_FLUSH:
	    map = sp->sm_cntab;
	    bep = &buf[sizeof buf - 10];
	    cmlen = crts[type].crt_cmlen;
	    for (j = sp->sm_loy; j < sp->sm_hiy; j++) {
		jj = j * sw;
		lastk = 9999;
		fi = li = ci = 9999;
		cp = buf;
		for (i = sp->sm_lox; i <= sp->sm_hix; i++) {
		    /* when i == sp->sm_hix we just flush last cached char */
		    if (i == sp->sm_hix)
			k = 256;	     /* use impossible dummy value */
		    else
			k = nscrn[jj + i];
		    /* check if a repeat of cached character */
		    if ((k != lastk && lastk != 9999) || lastk < ' ') {
			/* move to location of (first) cached character */
			if (ci <= fi && fi < ci + cmlen)     /* if close by */
			    while (ci < fi)
				*cp++ = nscrn[jj + ci++];
			else {             /* not close by, requires crt() */
			    cp = bflsh(sp->sm_fh, buf, cp);
			    tputsfh(crt(type, sp->sm_x0 + fi, sp->sm_y0 + j),
			     1, sp->sm_fh, sp->sm_ospeed);
			}
			/* output potentially repeated character */
			/* note that cntl chars are never repeated */
			if (cp + li - fi >= bep)    /* potential buf oflo? */
			    cp = bflsh(sp->sm_fh, buf, cp);
			if (li++ > fi)
			    cp = copy(crt(type, C_REPEAT, lastk, li - fi), cp);
			else if (lastk < ' ' && map)
			    cp = copy(map[lastk], cp);   /* map cntrl char */
			else
			    *cp++ = lastk;
			ci = li;			/* cursor position */
			fi = li = 999;	  /* first & last in cached repeat */
			lastk = 9999;			    /* cached char */
		    }
		    if (k != 256		 /* if not dummy last char */
		     && k != oscrn[jj + i]) {		    /* and changed */
			li = i;
			if (lastk == 9999) {		 /* no caching yet */
			    fi = i;
			    lastk = k;
			}
			oscrn[jj + i] = k;
		    }
		}
		bflsh(sp->sm_fh, buf, cp);
	    }
	    sp->sm_lox = sp->sm_hix = sp->sm_loy = sp->sm_hiy = 0;
	    break;
	case SM_CTLMAP:
	    sp->sm_cntab = (char **) arg1;
	    break;
	case SM_REUSE:
	    tputsfh(crt(type, C_FINI), sh, sp->sm_fh, sp->sm_ospeed);
	    if (arg1 < 0 || arg2 < 0 || arg2 > 64)
		return(0);
	    sp->sm_type = arg1;
	    sp->sm_fh = arg2;
	    break;
	case SM_FINI:
	    tputsfh(crt(type, C_FINI), sh, sp->sm_fh, sp->sm_ospeed);
	    if (arg1 != 0 && (fsp == 0 || sh * sw > fsp->sm_sh * fsp->sm_sw))
		fsp = sp;
	    break;
	default:
	    return(0);
	}
	return(sp);
}

static	char	*
bflsh(fh, buf, cp)
char	*buf, *cp;
{
	*cp = '\0';
	write(fh, buf, cp - buf);
	return(buf);
}

struct  smstr   *
sm_char(sp, x, y, c)
struct  smstr   *sp;
{
	register int i, sw, sh;

	sw = sp->sm_sw;
	sh = sp->sm_sh;
	if (x < 0 || x >= sw || y < 0 || y >= sh)
	    return(0);
	i = y * sw + x;
	sp->sm_nscrn[i] = c;
	if (sp->sm_oscrn[i] != c) {
	    if (x < sp->sm_lox)
		sp->sm_lox = x;
	    if (x >= sp->sm_hix)
		sp->sm_hix = x + 1;
	    if (y < sp->sm_loy)
		sp->sm_loy = y;
	    if (y >= sp->sm_hiy)
		sp->sm_hiy = y + 1;
	}
	return(sp);
}

sm_peek(sp, x, y)
struct  smstr   *sp;
{
	register int sw, sh;

	sw = sp->sm_sw;
	sh = sp->sm_sh;
	if (x < 0 || x >= sw || y < 0 || y >= sh)
	    return(0);
	return(sp->sm_nscrn[y * sw + x]);
}

struct  smstr   *
sm_str(sp, x, y, str)
struct  smstr   *sp;
char    *str;
{
	register char *cp;
	register int i;
	int sw, sh, ilo, ihi;

	sw = sp->sm_sw;
	sh = sp->sm_sh;
	if (x < 0 || x >= sw || y < 0 || y >= sh)
	    return(0);
	i = y * sw + x;
	sh = (y + 1) * sw;
	ilo = -1;
	for (cp = str; *cp && i < sh; i++, cp++) {
	    sp->sm_nscrn[i] = *cp;
	    if (sp->sm_oscrn[i] != *cp) {
		if (ilo < 0)    /* first one */
		    ilo = ihi = i;
		else
		    ihi = i;
	    }
	}
	if (ilo >= 0) {                    /* if anything has been changed */
	    i = ilo - y * sw;
	    if (i < sp->sm_lox)
		sp->sm_lox = i;
	    i = ihi - y * sw;
	    if (i >= sp->sm_hix)
		sp->sm_hix = i + 1;
	    if (y < sp->sm_loy)
		sp->sm_loy = y;
	    if (y >= sp->sm_hiy)
		sp->sm_hiy = y + 1;
	}
	return(sp);
}
