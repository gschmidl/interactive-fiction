#define	GAMESPATH(z)	"/usr/psl/.../z"
#define	PRVUID		257
#define	PRVLOG		"psl"
#define	PRVNAM		"Peter Langston (201) 829-4332"

/* system definitions */

/* select one of these three for empty.c & notify.c */
/*#define	SELECTV8			/* for Research V8 */
#define	SELECTBSD			/* for 4.2 BSD */
/*#define	FIONRE				/* for 4.1 BSD and others */

/* select one of these two for notify.c */
/*#define V6UTMP          /* if you have a V6 /etc/utmp */
#define V7UTMP          /* if you have a V7 /etc/utmp */
