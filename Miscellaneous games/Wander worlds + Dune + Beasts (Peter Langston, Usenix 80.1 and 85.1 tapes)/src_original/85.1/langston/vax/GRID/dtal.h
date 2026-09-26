/*
**	DTAL.H -- Header file for talisman operations in labyrinth game
**		P. Langston, 6/82
**	"daemon.h" must be included before this file
*/

/* values for t_type */
	/* NOTE: This list MUST match the definition of tchrstr[] in dtal.c */
#define T_TOUR		1	/* worthless tourist trinket */
#define T_MSTAIR	2	/* map both stairs */
#define T_ALIGN		3	/* turn to face attackers */
#define	T_HEAL		4	/* healing */
#define	T_LEVI		5	/* levitation */
#define T_TFLOOR	6	/* pass through floors (down) */
#define T_SNEAK		7	/* less likely to be noticed by others */
#define T_MGOLD		8	/* map gold locations */
#define T_INVIS		9	/* become invisible */
#define T_FIRE		10	/* shoot without bounce */
#define T_MPLYR		11	/* map player locations */
#define T_MWALL		12	/* map all walls */
#define T_TCEIL		13	/* pass through ceilings (up) */
#define T_TPORT		14	/* random teleport */
#define T_CANCEL	15	/* spells cancelled for anyone on same spot */
#define T_MOTHER	16	/* map locations of others */
#define T_TWALL		17	/* pass through walls */
#define T_DEST		18	/* destroy walls, gold, others, etc. */
#define T_SHIELD	19	/* shots bounce off, (inside & outside) */
#define T_HASTE		20	/* give commands twice as often */
#define T_BANISH	21	/* all within 4 are sent up one level */
#define T_SUCKER	22	/* worthless philanthropy token */
#define T_MAX		23	/* one past the last value used */
