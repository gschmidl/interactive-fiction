/*
**      GRAMDEF -- Anagram exerciser defines
** (c) P. Langston
*/

#define H_SCCS  "@(#)gramdef.h	1.2 last mod 4/18/82 -- (c) psl 1979"

#define BUFSIZE     128
#define MAXWDS      2048              /* max number of words to be indexed */

struct  recstr {
	char    r_lname[9];                           /* holder's log name */
	char    r_name[17];                               /* holder's name */
	int     r_points;                           /* highest point score */
	long    r_date;                                 /* date record set */
};

extern  char    *dictionary;                  /* the computer's vocabulary */
extern  char    *infofil;                                  /* instructions */
extern  char    *recfil;                                        /* records */

extern  int     def_len;                            /* default word length */
extern  int     max_len;                            /* maximum word length */
extern  int     nwpr;                                     /* human's score */
