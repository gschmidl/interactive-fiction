#
/*
**	ARCPSLTAN(x, y) -- returns the angle whose psltan is x / y in PSLs.
**		error <= 5.e-3 radians or 52.152 PSLs
**	IARCPSLTAN(x, y) -- More accurate version
**		error <= 1.e-5 radians or 0.104 PSLs
** 	(32768 PSLs == pi radians)
**	Approx. from Abramowitz & Segun, "Handbook of Mathematical Functions"
**
**	Copyright (c) 1977 P. Langston, N.Y.C., N.Y.
*/

#define	PI	0100000
#define	PI_2	0040000
#define	PI_4	0020000
#define	RADTOPSL	(32768. / 3.1415926535)

double	iaptc[] = {
	0.9998660,
	-.3302995,
	0.1801410,
	-.0851330,
	0.0208351,
};

int
arcpsltan(x, y)
double x, y;
{
	register int a;
	double q;

	if (y == 0.) 
	    if (x > 0.)
		return(PI_2);
	    else if (x < 0.)
		return(-PI_2);
	    else
		return(0);
	q = x / y;
	if (-1 <= q) {
	    if (q <= 1)
		a = RADTOPSL * q / (1. + .28 * q * q) + .5;
	    else
		a = arcpsltan(q - 1, 1 + q) + PI_4;
	} else
	    a = arcpsltan(q + 1, 1 - q) - PI_4;
	return(y > 0.? a : a + PI);
}

int
iarcpsltan(x, y)
double x, y;
{
	register int i, a;
	double q, qsq, ang;

	if (y == 0.) 
	    if (x > 0.)
		return(PI_2);
	    else if (x < 0.)
		return(-PI_2);
	    else
		return(0);
	q = x / y;
	if (-1 <= q) {
	    if (q <= 1) {
		ang = 0.;
		qsq = q * q;
		for (i=0; i<5; i++) {
		    ang += iaptc[i] * q;
		    q *= qsq;
		}
		a = ang * RADTOPSL;
	    } else
		a = arcpsltan(q - 1, 1 + q) + PI_4;
	} else
	    a = arcpsltan(q + 1, 1 - q) - PI_4;
	return(y > 0.? a : a + PI);
}
