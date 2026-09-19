/*
**      FSQRTVAX -- This "fast" square root is simply a call to "sqrt"
**          When I understand the VAX assembler I'll rewrite fsqrt11.s
*/

double
fsqrt(arg)
double  arg;
{
	extern double sqrt();

	return(sqrt(arg));
}
