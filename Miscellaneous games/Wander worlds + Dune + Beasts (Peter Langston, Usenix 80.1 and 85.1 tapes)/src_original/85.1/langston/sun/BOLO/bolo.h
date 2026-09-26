#define	O_NOP		(short) 0xC000
#define	O_GOTO		(short) 0xC001
#define	O_SET		(short) 0xC002
#define	O_IFEQ		(short) 0xC003
#define	O_IFNE		(short) 0xC004
#define	O_IFGT		(short) 0xC005
#define	O_IFLT		(short) 0xC006
#define	O_IFLE		(short) 0xC007
#define	O_IFGE		(short) 0xC008
#define	O_ENDIF		(short) 0xC009
#define	O_ADD		(short) 0xC00a
#define	O_SUB		(short) 0xC00b
#define	O_MUL		(short) 0xC00c
#define	O_DIV		(short) 0xC00d

#define	R_XLOC		0		/* our x position */
#define	R_YLOC		1		/* our y position */
#define	R_HEADING	2		/* motion direction */
#define	R_SPEED		3		/* our requested motion speed */
#define	R_RSPEED	4		/* our real motion speed */
#define	R_DAMAGE	5		/* 0 - 100% */
#define	R_AIM		6		/* gun direction */
#define	R_RANGE		7		/* gun range (fires gun) */
#define	R_SCAN		8		/* radar direction */
#define	R_DIST		9		/* radar target distance */
#define	R_TYPE		10		/* radar target type */
#define	R_RANDOM	11		/* a random number */
#define	R_NUMREGS	12

#define	T_WALL		-1		/* target type for a wall */

#define	MAXBOLO		8
#define	MAXSYMS		128
#define	MAXMEM		512
#define	MAXTREG		16
#define	CPS		32		/* mchine speed in cycles/second */
#define	MAXACCEL	64		/* bolo accel in meters/second^2 */
#define	MAXTURNSP	16		/* speed limit for turns in m/sec */
#define	SHOOTWAIT	10		/* wait cycles for shooting */
#define	RADARWAIT	4		/* wait cycles for radar scan */
#define	BOLO_RADIUS	10		/* bolo radius in meters */

#define	A_CONSTANT	(short) 0x0000
#define	A_MACH_REG	(short) 0x8000
#define	A_TEMP_REG	(short) 0x4000
#define	A_SYMBOL	(short) 0x2000

#define	PI		0x0200		/* assumes oldPSLs (BANG10) */
#define	PI_2		0x0100
#define	MOD2PI(psls)	((psls) & (PI + PI - 1))

struct	symstr	{
	char	sym_name[16];
	short	sym_val;
};

struct  bolostr {                                          /* one per bolo */
	char            b_name[16];                /* name of bolo program */
	struct  symstr  b_sym[MAXSYMS];                    /* symbol table */
	short           b_mem[MAXMEM];                      /* bolo memory */
	short           b_pc;          /* next instruction index in memory */
	short           b_rregs[R_NUMREGS];            /* "real" registers */
	short           b_tregs[MAXTREG];           /* temporary registers */
	short           b_waits;                      /* wait cycles to go */
	short		b_xfract, b_yfract; /* fractional part of position */
};
