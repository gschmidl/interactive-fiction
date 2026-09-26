#define D_FILES
#define D_NATSTR
#define D_NATSTAT
#define D_SECTDES
#define D_SCTSTR
#define D_DCHRSTR
#define D_POWSTR
#define D_VTYPES
#include "../empdef.h"
/*
**      CREDEF -- defines for the creation
** Copyright (c) by Peter S. Langston - New York City
*/

#define	MAXV	64
#define	MAXM	64
#define	MAXG	64
#define MAXO    64
#define	NUMELS	1024
#define	EOL	-1

#define	icnv(x)	((w_xsize + x) % w_xsize)
#define	jcnv(y)	((w_ysize + y) % w_ysize)

struct  elevstr {
	short   e_type;     /* sector type */
	float   e_pcnt;     /* fraction of this type */
};
extern	struct	elevstr	elev[];

struct  coord {
	short   x;
	short   y;
};
extern	struct	coord	volc[MAXV];
extern	struct	coord	bmet[MAXM];
extern	struct	coord	lmet[MAXM];
extern	struct	coord	gold[MAXG];
extern	struct	coord	oil[MAXO];

extern	int	testflag;
extern	int	traceflag;
extern	int	creamflag;
extern	int	locflag;
extern	int	verbose;
extern	short	elcnts[];
extern	int	numsects, nvolc, nbmet, nlmet, ngold, nriver, noil;
