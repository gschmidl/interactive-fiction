/*
**      FMOD -- Floating modulus done right!
*/

double
fmod(x, y)
double  x, y;
{
	extern double floor();

	if (y == 0.)
	    return(0.);
	return(x - y * floor(x / y));
}
