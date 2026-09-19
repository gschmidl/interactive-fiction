#include "credef.h"
/*
**      CREGLB -- Globals for the creation
** Copyright (c) by Peter S. Langston - New York City
*/

struct  elevstr	elev[]    = {
/* This data defines the stratification of the world -- play with it! */
/* (Ordered from highest elevation to lowest.) */
/*	SCT_RURAL,	0.04,	/* plateaus */
/*	SCT_MOUNT,	0.05,	/* mountain peaks */
/*	SCT_RURAL,	0.15,	/* plains */
/*	SCT_MOUNT,	0.02,	/* hills */
/*	SCT_RURAL,	0.25,	/* more plains */
/*	SCT_WATER,	0.49,	/* oceans */
	SCT_RURAL,	0.02,		/* for testing */
	SCT_MOUNT,	0.07,		/* for testing */
	SCT_RURAL,	0.42,		/* for testing */
	SCT_WATER,	0.49,		/* for testing */
	EOL,		0.00,	/* must terminate with this */
};

struct  coord	volc[MAXV], bmet[MAXM], lmet[MAXM], gold[MAXG], oil[MAXO];

int	testflag    = 0;
int	traceflag   = 0;
int	creamflag   = 0;
int	locflag     = 0;
int	verbose	    = 0;
short	elcnts[NUMELS];
int	numsects, nvolc, nbmet, nlmet, ngold, nriver, noil;

setnums()
{
	numsects = w_xsize * w_ysize / 2;
	printf("World size is %d x %d (%d sectors)\n",
	 w_xsize, w_ysize, numsects);
	nvolc = numsects / 500 + 2;
	if (nvolc > MAXV)
	    nvolc = MAXV;
	nbmet = numsects / 1400 + 2;
	if (nbmet > MAXM)
	    nbmet = MAXM;
	nlmet = numsects / 1000 + 2;
	if (nlmet > MAXM)
	    nlmet = MAXM;
	ngold = numsects / 600 + 2;
	if (ngold > MAXG)
	    ngold = MAXG;
	nriver = numsects / 400 + 2;
	noil = numsects / 500 + 2;
	if (noil > MAXO)
	    noil = MAXO;
}
