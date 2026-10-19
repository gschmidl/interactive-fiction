/*
**		    Copyright (c) 1985	Ken Wellsch
**
**     Permission is hereby granted to all users to possess, use, copy,
**     distribute, and modify the programs and files in this package
**     provided it is not for direct commercial benefit and secondly,
**     that this notice and all modification information be kept and
**     maintained in the package.
**
*/

#include "adefs.h"

/* Windows port: -u starts the player as a wizard (ADMIN bit OLORIN), the
** game's own way past the 30-minute wait it imposes after SAVE.  ADMIN is
** the 17th VARIABLE in adv_db/vars.d, OLORIN bit 5 in adv_db/bits.d. */
#define ADMIN_VAR	16
#define OLORIN_BIT	5

main (argc,argv)
  int argc ;
  char *argv[] ;
{
	register int i, unlimited = NO ;

	for ( i = 1 ; i < argc ; i++ )
		if ( strcmp (argv[i],"-u") == 0 )
			unlimited = YES ;

	srandom (time(0)+getpid()) ;

	setup () ;
	if ( unlimited )
		varbit[ADMIN_VAR] |= (1 << OLORIN_BIT) ;
	opendb (ADV) ;
	webster () ;

#ifdef PSTAB
	pstab () ;
#endif PSTAB

	process (INITIAL) ;

	forever
		process (REPEAT) ;
}

int setup ()
{
	register int i ;

	for ( i = 0 ; i < OBJECTS ; i++ )
	{
		objval[i] = 0 ;
		objbit[i] = XOBJECT ;
		objloc[i] = 0 ;
	}
	
	for ( i = 0 ; i < PLACES ; i++ )
	{
		plcbit[i] = XPLACE ;
	}

	for ( i = 0 ; i < VARS ; i++ )
	{
		varval[i] = 0 ;
		varbit[i] = XVERB ;
	}

#ifdef CACHE
	ClrCache () ;
#endif CACHE

	return ;
}

int opendb(name)
  char *name ;
{
	dbunit = openk (name) ;

	if ( dbunit == ERROR )
		error ("Opendb","unable to open %s!",name) ;
	return ;
}
