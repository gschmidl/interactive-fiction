#include    <stdio.h>
#include    <crt.h>
#include    "bolo.h"

/*
**	TCVIZ -- Termcap output routines for Bolo
** (c) P. Langston 1981
*/

#define SCRNX(x)        ((x + 256.) * scrnx / 512.)
#define SCRNY(y)        (scrny - (y + 256.) * scrny / 512.)

extern	struct	bolostr bolo[MAXBOLO];
extern	short	lobolo, hibolo, maxbolo;
extern	short	trace;
short		crtype;
short		ospeed;
short           scrnx;
short           scrny;
short		xl[MAXBOLO], yl[MAXBOLO];
short		dam[MAXBOLO], hits[MAXBOLO];
short		boomdelay[16]	= {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 8, 16, 1, 1,
};
extern	struct	crtstr	crts[];

vizinit()
{
	register short i;
	char buf[6];

	gtty(2, buf);
	ospeed = buf[1] & 017;
	crtype = crtinit(getenv("TERM"));
	ospeed = getspd(1);
	scrny = crts[crtype].crt_sh;
	if (crts[crtype].crt_sw >= 2 * scrny + 14)
	    scrnx = 2 * scrny;
	else
	    scrnx = scrny;
	tputsfh(crt(crtype, C_CLEAR), scrny, 1, ospeed);
	for (i = MAXBOLO; --i >= 0; )
	    dam[i] = -1;
}

disname(blp)
struct  bolostr *blp;
{
	tputsfh(crt(crtype, scrnx + 1, 2 * (blp - bolo)), 1, 1, ospeed);
	printf("%d %s", blp - bolo, blp->b_name);
}

message(string)
char	*string;
{
	tputsfh(crt(crtype, 0, scrny - 2), 1, 1, ospeed);
	printf("%s", string);
}

display(flag)
{
	short bx, by, d, x, y, i;

	for (i = lobolo; i <= hibolo; i++) {
	    d = bolo[i].b_rregs[R_DAMAGE];
	    bx = SCRNX(bolo[i].b_rregs[R_XLOC]);
	    by = SCRNY(bolo[i].b_rregs[R_YLOC]);
	    if (d != dam[i]) {
		x = scrnx + 5;
		y = i * 2 + 1;
		if (d > 90) {
		    tputsfh(crt(crtype, x, y), 1, 1, ospeed);
		    fputs("DEFUNCT", stdout);
		    tputsfh(crt(crtype, bx, by), 1, 1, ospeed);
		    fputs(" ", stdout);
		} else {
		    tputsfh(crt(crtype, x, y), 1, 1, ospeed);
		    printf("%2d%%", d);
		}
		dam[i] = d;
	    }
	    if (d <= 90) {
	        if (bx != xl[i] || by != yl[i]) {
		    tputsfh(crt(crtype, bx, by), 1, 1, ospeed);
		    printf("%d", i);
		    tputsfh(crt(crtype, xl[i], yl[i]), 1, 1, ospeed);
		    fputs(" ", stdout);
		    xl[i] = bx;
		    yl[i] = by;
		} else if (flag) {
		    tputsfh(crt(crtype, bx, by), 1, 1, ospeed);
		    printf("%d", i);
		}
	    }
	}
}

boom(n, x, y)
{
	register short i, bx, by;

	bx = SCRNX(bolo[n].b_rregs[R_XLOC]);
	by = SCRNY(bolo[n].b_rregs[R_YLOC]);
	tputsfh(crt(crtype, bx, by), 1, 1, ospeed);
	fputs("#", stdout);
	hits[n]++;
	tputsfh(crt(crtype, scrnx + 1, n * 2 + 1), 1, 1, ospeed);
	printf("%3d", hits[n]);
	x = SCRNX(x);
	y = SCRNY(y);
	for (i = boomdelay[ospeed]; --i >= 0; ) {
	    tputsfh(crt(crtype, x, y), 1, 1, ospeed);
	    tputsfh(crt(crtype, C_FIELD, F_STANDOUT), 1, 1, ospeed);
	    fputs(" ", stdout);
	    tputsfh(crt(crtype, C_FIELD, F_NORMAL), 1, 1, ospeed);
	}
	tputsfh(crt(crtype, x, y), 1, 1, ospeed);
	tputsfh(crt(crtype, C_FIELD, F_NORMAL), 1, 1, ospeed);
	fputs(" ", stdout);
	display(1);
}
