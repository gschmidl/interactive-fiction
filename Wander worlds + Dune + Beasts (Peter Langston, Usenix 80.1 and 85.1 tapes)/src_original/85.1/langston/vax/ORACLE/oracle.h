/*
**      ORACLE.H -- definitions for oracle
*/
#include	"../lock.h"

#define H_SCCS  "@(#)oracle.h	1.4 9/16/84 -- (c) psl 1981"

#define TEXTLEN 113         /* max size for questions & answers */

extern  char    *indexfile;
extern  char    *q_afile;
extern  char    *tmpfile;
extern  char    *hmm[];
extern  char    *privname;
extern  short   privuid;
extern	struct	lockstr lck;

#define NO_ONE  0

struct  questr {
	short   q_askuid;           /* who asked */
	short	q_ansuid;           /* who answered */
	char    q_asknam[9];        /* logname of asker */
	char    q_ansnam[9];        /* logname of answerer*/
	long    q_askdat;           /* when asked */
	long    q_ansdat;           /* when answered */
	char    q_quest[TEXTLEN];   /* question */
	char    q_answer[TEXTLEN];  /* answer */
};
