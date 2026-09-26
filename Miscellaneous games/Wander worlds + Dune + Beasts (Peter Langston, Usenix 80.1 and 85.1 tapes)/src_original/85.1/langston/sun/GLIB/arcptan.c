/*
**	ARCPTAN(x, y) -- returns the angle whose ptan is x / y in oldPSLs.
**		error <= 5.e-3 radians or 0.815 oldPSLs
**	ARCPITAN(x, y) -- Same thing with x & y being integers.
** 	(512 oldPSLs == pi radians, BANG10)
**	Approx. from Abramowitz & Segun, "Handbook of Mathematical Functions"
**
**	Copyright (c) 1977 P. Langston, N.Y.C., N.Y.
*/

#define	PI	0x0200
#define	PI_2	0x0100
#define	PI_4	0x0080
#define	MOD2PI(psls)	((psls) & 0x03FF)
#define	RTP(radians)	((double) PI * (radians) / 3.1415926535)

double	iaptc[] = {
	0.9998660,
	-.3302995,
	0.1801410,
	-.0851330,
	0.0208351,
};

int
arcptan(x, y)
double	x, y;
{
	register int a;
	double q;

	if (y == 0.) {
	    if (x > 0.)
		return(PI_2);
	    else if (x < 0.)
		return(-PI_2);
	    else
		return(0);
	}
	q = x / y;
	if (-1. <= q) {
	    if (q <= 1.)
		a = RTP(q / (1. + .28 * q * q)) + .5;
	    else
		a = arcptan(q - 1, 1 + q) + PI_4;
	} else
	    a = arcptan(q + 1, 1 - q) - PI_4;
	if (y < 0.)
	    a += PI;
	return(MOD2PI(a));
}

int
arcpitan(x, y)
int	x, y;
{
	return(arcptan((double) x, (double) y));
}
