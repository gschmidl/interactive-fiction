/*
**      PYTHAG -- This routine generates sqrt(a*a+b*b)
*/

double
pythag(a, b)
double  a, b;
{
	double p, q, r;

	if (a < 0)
	    a = -a;
	if (b < 0)
	    b = -b;
	if (a > b) {
	    p = a;
	    q = b;
	} else {
	    p = b;
	    q = a;
	}
	r = q / p;
	r *= r;
	r /= 4. + r;
	p += 2. * p * r;
	q *= r;
	r = q / p;
	r *= r;
	r /= 4. + r;
	p += 2. * p * r;
	q *= r;
	r = q / p;
	r *= r;
	r /= 4. + r;
	return(p + 2. * p * r);
}
