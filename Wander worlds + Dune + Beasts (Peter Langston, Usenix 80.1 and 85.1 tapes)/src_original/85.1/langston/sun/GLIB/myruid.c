/*
**      MYRUID & MYEUID -- System dependent uid routines
*/

int myruid()        /* return "real" user id */
{
	return(getuid());		/* V7, 3.0, etc. */
/*      return(getuid() & 0377);	/* V6 */
}

int myeuid()        /* return "effective" user id */
{
	return(geteuid());		/* V7, 3.0, etc. */
/*      return(getuid() >> 8 & 0377);   /* V6 */
}
