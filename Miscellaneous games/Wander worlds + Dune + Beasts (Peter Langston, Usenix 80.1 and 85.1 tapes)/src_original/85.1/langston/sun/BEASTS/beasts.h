#include	"../lock.h"
/*
**      Header file for them beasts
** (c) P. Langston 1979
*/

#define H_SCCS      "@(#)beasts.h	1.8  last mod 9/12/84 -- (c) psl 1979"

#define N_QUEST     256                 /* max num questions possible */
#define NQI         ((N_QUEST+15)/16)   /* space for bit map */

#define YES         1
#define NO          0

extern  short   bmap[];

extern  char    *beastfile;
extern  char    *questfile;
extern  char    *locknode;
extern  char    *lockfile;
extern  char    *oopsfile;
extern  char    *mnblog;
extern  char    *mnqlog;

#define	BNAMSIZ	30

struct  bstr {
	char    b_name[BNAMSIZ];	/* name of beast */
	short   b_uid;			/* who supplied the beast */
	short   b_ans[NQI];		/* bit map of question answers */
};

#define	QTXTSIZ	122

struct  qstr {
	char    q_text[QTXTSIZ];	/* text of question, w/o `?' */
	short   q_uid;			/* who supplied the question */
	long    q_date;
};

extern	struct	lockstr	lck;
