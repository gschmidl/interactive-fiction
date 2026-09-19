#include "sd_def.h"
/*
**      TERMCAPVIZ -- Device-dependent visual module of Stardrek
**      This version for systems with /etc/termcap & /usr/lib/libtcap.a
**      The following globals are initialized from termcap entries:
**
**      tc_dl -- differentiates between CRTs with/without the ability to
**          delete a line and scroll up following lines.  If NULL,
**          the message area will be static and contain a pointer to the
**          most recent message.
**
**      tc_so, tc_se -- make important messages "stand out".
**          If NULL, no special action is taken.
**
**      tc_cl -- used to clear the CRT screen.  If NULL, no screen clear
**          will be done.
**
**      tc_rs -- a character sequence that will cause the repetition
**          of the following character-producing string.
**      tc_re -- a character sequence that will cause the repetition
**          of the preceding char-producing string.
**          These sequences will be used if rptthresh is non-zero and
**          there are more than rptthresh repeats of the same character.
**      These are my additions to termcap, specifically:
**          "rp" is a boolean, "do repeats exist?"
**          "rs" is the start repeat, (char follows)
**          "re" is the end repeat, (char precedes)
**      No padding is allowed on these; see rptguts() for more info.
**
**      tc_is -- initialize terminal
**      tc_ks -- initialize terminal for keypad use
**      tc_ke -- deinitialize terminal from keypad use
**      tc_ti -- initialize terminal for cursor addressing.
**      tc_te -- deinitialize terminal from cursor addressing.
**
**      crtbox -- draw a box around the bridge monitor or whatever other
**          "window dressing" seems appropriate.
**      (not from termcap; a box using "-", "|", and "+" is drawn)
**
**      funkey -- struct of function key definitions.  Used to map
**          function keys to maneuvers.  They must all have the same
**          first character which is that contained in func_char.
**      termcap "k0", "k1",..., "k9" are used to map into the last 10 entries
**      of this table, the first four are the cursor motion keys, "kl", "kd",
**      "ku", and "kr".  Note that if any of these are single character
**      strings they are put directly into com_map instead.  Also note that
**      the character chosen for func_char is the first character of the first
**      multicharacter sequence found in this order: kl, kd, ku, kr, k0,...,k9
**
** Copyright (c) 1981 by Peter S. Langston - New  York,  N.Y.
*/

char    *whatviz = "@(#)tcapviz.c	1.11 4/8/84 -- (c) psl 1976";

#define M_WIDTH     99     /* max width of bridge monitor in columns (odd) */
#define M_HEIGHT    49      /* max height of bridge monitor in lines (odd) */
#define M_TOP       1             /* bridge monitor location (line offset) */

#define D_LWIDTH    16            /* space needed by left-hand instruments */
#define D_RWIDTH    19           /* space needed by right-hand instruments */
#define M_LEFT      (D_LWIDTH+1)   /* bridge monitor location (col offset) */
#define MSG_MAX     16                      /* max lines of message buffer */
#define MSG_MIN     5                       /* min lines of message buffer */


#define VERT_LINE   1
#define HORIZ_LINE  2

#define BODY_FILL   1                             /* used in old[] & new[] */
#define FAR_AWAY    2                             /* used in old[] & new[] */

struct	dialstr {
	char    labx, laby;                         /* char, line of label */
	char    *label;                                       /* the label */
	char    valx, valy;                     /* char, line of the value */
	char    *valfmt;                           /* format for the value */
};

struct  shapestr {  /* note, these strings must each produce a single char */
	char    sh_near[4];                       /* as seen from close up */
	char    sh_fill1[4];    /* to fill out the body close up (usually) */
	char    sh_fill2[4];  /* to fill out the body close up (sometimes) */
	char    sh_far[4];                            /* as seen from afar */
};

/* only needed by terminals without "dl" capability */
int     linlen[MSG_MAX];                         /* displayed line lengths */
int     curlin;                                /* most recent display line */

int     o[MAX_O];                               /* used in dials & moninit */
char    old[M_WIDTH][M_HEIGHT], new[M_WIDTH][M_HEIGHT], dislen;

/* codes in old[] & new[] are:
	   0      blank
	   1      vert line of cross hairs
	   2      hor line of cross hairs
      3,6,9,...   object close up       object # * 3
     4,7,10,...   object body-fill      object # * 3 + BODY_FILL
     5,8,11,...   object at a distance  object # * 3 + FAR_AWAY
*/

    /* Terminal specific stuff set from termcap entry */

short   tc_co;                     /* number of columns in the display co# */
short   tc_li;                       /* number of lines in the display li# */
char    *tc_cm;                                /* cursor motion format cm= */
char    *tc_dl;                          /* close a line and scroll up dl= */
char    *tc_so;                                     /* start stand-out so= */
char    *tc_se;                                      /* stop stand-out se= */
char    *tc_cl;                                        /* clear screen cl= */
char    rpt_exist;                                /* do repeats exist? rp  */
char    rptthresh;                   /* length of repeat specification     */
char    *tc_rs;                                  /* rpt following char rs= */
char    *tc_re;                                  /* rpt preceding char re= */
char	*tc_is;					/* initialize terminal is= */
char	*tc_ks;			 /* initialize terminal for keypad use ks= */
char	*tc_ke;		      /* deinitialize terminal from keypad use ke= */
char	*tc_ti;		  /* initialize terminal for cursor addressing ti= */
char	*tc_te;	       /* deinitialize terminal from cursor addressing te= */
char    tcapbuf[1024];         /* space to keep the actual termcap strings */

short	ospeed;					  /* terminal output speed */
short   m_width;                           /* width of bridge monitor, odd */
double  m_hwidth;                       /* half of width of bridge monitor */
short   m_midw;                          /* col of top & bottom tick marks */
short   m_height;                         /* height of bridge monitor, odd */
double  m_hheight;                     /* half of height of bridge monitor */
short   m_midh;                         /* line of left & right tick marks */
short   msg_top;                      /* top line of message scroll buffer */
short   msg_bottom;                /* bottom line of message scroll buffer */
short   msg_lines;             /* number of lines of message scroll buffer */

double  xyfact;                  /* set to m_height / m_width in moninit() */

char    hor_line[8] = { '-' };                   /* to produce cross hairs */
char    ver_line[8] = { '|' };                                    /* ditto */

/* input chars */
char    bs_char     = '\b';            /* used to cancel a partial command */
char    cr_char     = '\r';                       /* used to end line mode */
char    fname_char  = '<';                       /* used to start filename */
char    fexec_char  = '>';                    /* used to end/execute files */
char    line_char   = ';';                         /* used to start a line */
char    list_char   = 'l';                          /* used to list macros */
char    mdef_char   = '=';                        /* used to define macros */
char    mexec_altc  = ',';                        /* used to invoke macros */
char    mexec_char  = 'm';                        /* used to invoke macros */
char    wait_char   = ':';                    /* wait until next iteration */
char    func_char   = '\0';              /* function &/| arrow key lead-in */

	/* mapping function keys into single or multiple characters */
	/* note that these values are taken from com_map[] */
struct  funkstr funkey[]    = {
	"",     cMYROT, "-13y",                          /* kl becomes cMYROT */
	"",     cMXROT, "-13x",                          /* kd becomes cMXROT */
	"",     cPXROT, "13x",                           /* ku becomes cPXROT */
	"",     cPYROT, "13y",                           /* kr becomes cPYROT */
	"",     0,      "0m",                        /* F0 becomes maneuver 0 */
	"",     0,      "1m",                        /* F1 becomes maneuver 1 */
	"",     0,      "2m",                                         /* etc. */
	"",     0,      "3m",
	"",     0,      "4m",
	"",     0,      "5m",
	"",     0,      "6m",
	"",     0,      "7m",
	"",     0,      "8m",
	"",     0,      "9m",
	"",     0,      "",
};

	/* mapping single characters into commands */
char    com_map[]   = {
     0,     0,     0,     0,     0,     0,     0,     0,      /* NUL - BEL */
cIGNOR,     0,cMXROT,     0,     0,     0,     0,     0,      /*  BS - SI  */
     0, cNOOP,     0, cNOOP,     0,     0,     0,     0,      /* DLE - ETB */
     0,     0,     0,     0,cPXROT,     0,cPYROT,cMYROT,      /* CAN - US  */
     0,cSHELL,     0,     0,cSCORE,     0,     0,     0,      /*  SP - '   */
     0,     0, cTORP,     0,     0,     0,     0,     0,      /*   ( - /   */
     0,     0,     0,     0,     0,     0,     0,     0,      /*   0 - 7   */
     0,     0,     0,     0,     0,     0,     0,     0,      /*   8 - ?   */
     0, cAUTO,     0,  cCCC, cDOCK,     0,cTHRST,     0,      /*   @ - G   */
     0,cISTOP,     0,     0,     0,     0,     0,     0,      /*   H - O   */
 cPHAZ, cSALV,  cDAM, cSHLD, cTRAC,     0,     0, cWARP,      /*   P - W   */
 cXROT, cYROT, cZROT,     0,     0,     0,cRFRSH,     0,      /*   X - _   */
     0, cAUTO,     0,  cCCC, cDOCK,     0,cTHRST,     0,      /*   ` - g   */
     0,cISTOP,     0,     0,     0,     0,     0,     0,      /*   h - o   */
 cPHAZ, cSALV,  cDAM, cSHLD, cTRAC,     0,     0, cWARP,      /*   p - w   */
 cXROT, cYROT, cZROT,     0,     0,     0,     0,  cSOS,      /*   x - DEL */

};

struct  shapestr shape[] = {
/*      near    fill1   fill2   far     */
	{{'?'}, {'?'},  {'?'},  {'?'}},         /* Human */
	{{'N'}, {'+'},  {'+'},  {'?'}},         /* Nubian */
	{{'K'}, {':'},  {':'},  {'?'}},         /* Klingon */
	{{'R'}, {'#'},  {'#'},  {'?'}},         /* Romulan */
	{{'V'}, {'X'},  {'X'},  {'?'}},         /* Vallician */
	{{'='}, {'@'},  {'@'},  {'?'}},         /* Rigellian */
	{{'*'}, {'*'},  {'*'},  {'*'}},         /* torpedo */
	{{'O'}, {'$'},  {'$'},  {'O'}},         /* Starbase */
	{{'.'}, {'.'},  {'.'},  {'.'}},         /* star */
	{{'o'}, {'O'},  {'0'},  {'o'}},         /* planet */
};

/* NOTE: dialinfo must agree in order with defines, (*_DIAL), in sd_def.h */
/* neg x is dist to the right of the monitor */
struct  dialstr dialinfo[] = {
/*      labx,y  label               valx,y  valfmt  */
	0,0,    "",                 0,0,    "",
	0,0,    "Mission Vector",   0,1,    "%4d,%4d,",
	0,0,    "",                 9,1,   ",%4d ",
	-1,0,   "Energy",           -1,1,   "%4d ",
	-11,0,  "E. Time",          -11,1,  "%5d ",
	0,3,    "Starbase Vector",  0,4,    "%4d,%4d,",
	0,3,    "",                 9,4,   ",%4d ",
	-1,3,   "Vis Range",        -1,4,   "%4d ",
	-11,3,  "Sensors",          -11,4,  "%5d ",
	0,6,    "Velocity Vector",  0,7,    "%4d,%4d,",
	0,6,    "",                 9,7,   ",%5.1f ",
	-1,6,   "Speed",            -1,7,   "%5.1f ",
	-11,6,  "Condition",        -11,7,  "%s%s%s ",
	0,9,    "Rotation Vector",  0,10,   "%4d,%4d,%4d ",
	/* the following are not permanently labelled */
	/* the label info is used for erasing the dial */
	-1,9,   "                ", -1,9,   "%2d A.T.U. remain ",
	-1,10,  "            ",     -1,10,  "C.C.C. @%3.0f%% ",
	-1,11,  "             ",    -1,11,  "Shields @%3.0f%% ",
};

char    *cursor(), *dorpt(), *rptguts();
extern  char    *tcgstr(), *getenv();

viz()
{
        register int i, j, k;
	int center, r, jj, kk, l, lsq, m, dksq, afar;
	double dists[85], d, zfact, *pi, q;

	i = m_midw - M_LEFT;
	j = m_midh - M_TOP;
	new[i][j - 2] = new[i][j - 1] = VERT_LINE;
	new[i][j + 1] = new[i][j + 2] = VERT_LINE;
	new[i - 6][j] = new[i - 4][j] = new[i - 2][j] = HORIZ_LINE;
	new[i + 2][j] = new[i + 4][j] = new[i + 6][j] = HORIZ_LINE;
	dists[0] = 99999.;    /* this also goes for VERT_LINE & HORIZ_LINE */
	range = center = 9999;
		  /* go through all objects & place visible ones on screen */
        for (i = 1; i < NUM_OBJECTS; i++) {
            r = rac[i];
            pi = pos[i];
	    if (active[i] <= 0 || pi[2] <= 0.)
		continue;                          /* if dead or behind us */
	    if (pi[2] > sc[r].sc_visd || (d = size(pi)) > sc[r].sc_visd)
		continue;                               /* if too far away */
	    zfact = m_hwidth / pi[2];
	    j = m_hwidth + pi[0] * zfact;
	    k = m_hheight - pi[1] * zfact * xyfact;
	    afar = d > sc[r].sc_visd * .4? FAR_AWAY : 0;
	    if (j >= 0 && j < m_width && k >= 0 && k < m_height
	     && dists[(new[j][k] & 0377) / 3] > d)
		center = vizadd(3 * i + afar, j, k, center, d);
	    dists[i] = d;
	    if (afar)
		continue;
	    q = sc[r].sc_rad * .1 * zfact;
	    l = q + .5;                     /* size of obj in char columns */
	    m = q * xyfact + .5;              /* size of obj in char lines */
	    lsq = l * l;
	    for (kk = max(0, k - m); kk <= min(m_height - 1, k + m); kk++) {
		dksq = (kk - k) / xyfact;
		dksq *= dksq;
		for (jj = max(0, j - l); jj <= min(m_width - 1, j + l); jj++)
		    if ((jj - j) * (jj - j) + dksq <= lsq         /* round */
		     && dists[(new[jj][kk] & 0377) / 3] > d)
			center = vizadd(3 * i + BODY_FILL, jj, kk, center, d);
	    }
	}
	/* now display the changes */
	for (j = 0; j < m_height; j++) {
	    for (i = 0; i < m_width; i++) {
		if (k = new[i][j])
		    new[i][j] = 0;
		if (k == (kk = old[i][j]))
                    continue;
		old[i][j] = k;
		if (k == 0)
		    monit(" ", i, j);
		else if (k == VERT_LINE)
		    monit(ver_line, i, j);
		else if (k == HORIZ_LINE)
		    monit(hor_line, i, j);
		else {
		    k &= 0377;
		    r = rac[k / 3];
		    kk = k % 3;
		    if (kk == FAR_AWAY)
			monit(shape[r].sh_far, i, j);
		    else if (kk != BODY_FILL)
			monit(shape[r].sh_near, i, j);
		    else if (rand() & 0120)
			monit(shape[r].sh_fill1, i, j); /* usual body fill */
		    else
			monit(shape[r].sh_fill2, i, j);   /* alt body fill */
		}
                continue;
            }
        }
	if (rpt_exist)
	    monit(NULL, 0, 0);                      /* flush saved characters */
}

vizadd(i, j, k, center, d)
double d;
{
	register int l, dx, dy;

	new[j][k] = i;
	dx = j - m_hwidth;
	dy = (m_hheight - k) / xyfact;
	l = dx * dx + dy * dy;
	if (l <= center) {
	    center = l;
	    range = d;
	}
	return(center);
}

monit(str, x, y)
char    *str;
{
	static char *lmstr;
	static int lmx, lmy, count;

	if (rpt_exist) {
	    if (x == lmx && y == lmy && equal(str, lmstr)) {
		count++;
		lmx++;
		return;
	    }
	    if (count > 0) {
		printf("%s", cursor(lmx + M_LEFT - count, lmy + M_TOP));
		if (count > rptthresh)
		    fputs(dorpt(tc_rs, lmstr, tc_re, count), stdout);
		else
		    while (--count >= 0)
			printf("%s", lmstr);
	    }
	    if (str == NULL) {
		count = 0;
		lmstr = NULL;
		lmx = lmy = -1;
	    } else {
		lmx = x + 1;
		lmy = y;
		lmstr = str;
		count = 1;
	    }
	} else {
	    if (x == lmx && y == lmy)
		printf("%s", str);
	    else
		printf("%s%s", cursor(x + M_LEFT, y + M_TOP), str);
	    lmx = x + 1;
	    lmy = y;
	}
}

pow(dx, dy, dst, siz)
double  dx, dy, dst, siz;
{
        register int i, j;
	int  x, y, xspread, yspread;

	xspread = dst > 50? 1 : 2;
	 yspread = dst > 50? 0 : 1;
	x = m_hwidth + m_hwidth * dx;
	x = min(m_width - xspread - 1, x);
	x = max(xspread, x);
	y = m_hheight - m_hheight * dy;
	y = min(m_height - yspread - 1, y);
	y = max(yspread, y);
	for (i = x - xspread; i <= x + xspread; i++)
	    for (j = y - yspread; j <= y + yspread; j++)
                old[i][j] = new[i][j] = 0;
	x = x + M_LEFT;
	y = y + M_TOP;
	if (dst > 50.) {
	    printf("%s+O=X=)0(*#*)0(*#*",
	     cursor(x, y));
	    while ((siz *= .5) > 35.)
		printf("(*)) (>+<");
	    printf(" )*(* *> <O- -   ");
	} else {
	    printf("%s+O>X<=)0(=*#*", cursor(x, y));
	    printf("%s\\ /", cursor(x - 1, y - 1));
	    printf("%s/ \\", cursor(x - 1, y + 1));
	    printf(cursor(x, y));
	    while ((siz *= .5) > 35.)
		printf(" (*))) (( + ");
	    printf("%s\\` '/", cursor(x - 2, y - 1));
	    printf("%s/' `\\", cursor(x - 2, y + 1));
	    printf("%s>#0#<)> <(>   <     ",
	     cursor(x - 2, y));
	    printf("%s     ", cursor(x - 2, y - 1));
	    printf("%s     ", cursor(x - 2, y + 1));
	}
}

phaz_effect(dx, dy, f)
double  dx, dy, f;
{
        register  int  i, x, y;

	x = m_hwidth + m_hwidth * dx;
	y = m_hheight + m_hheight * dy;
	new[x][y] = old[x][y] = new[x - 1][y] = old[x - 1][y] = 0;
	printf("%s| / - \\ | /",
	 cursor(x + M_LEFT, y + M_TOP));
	x = f + 1.;
	for (i = 0; i <= x; i += 3)
	    printf(" O%c%c %c%c%c %c%c%c  %c%c",
	     8,8, "-\\|/"[i & 3], 8,8, "-\\|/"[i & 3], 8,8, 8,8);
}

scrnclr()
{
	tputsfh(tc_cl, 60, 1, ospeed);	/* better safe than sorry */
}

terminit()                        /* fetch all necessary info from termcap */
{
	register char *cp;
	register int i;
	char buf[1024], id[3], *bp;

	if ((bp = getenv("TERM")) == NULL) {
	    printf("getenv(\"TERM\") fails\n");
	    exit(3);
	}
	ospeed = getspd(1);      /* for tputsfh() */
	if ((i = tcgent(buf, bp)) <= 0) {
	    printf("tcgent(buf, \"%s\") returns %d\n", bp, i);
	    exit(3);
	}
	bp = tcapbuf;
	tc_co = tcgnum("co", buf);
	tc_li = tcgnum("li", buf);
	if ((tc_cm = tcgstr("cm", &bp, buf)) == NULL) {
	    printf("Can't find a `cm' termcap entry\n");
	    exit(1);
	}
	tc_dl = tcgstr("dl", &bp, buf);
	if ((tc_so = tcgstr("so", &bp, buf)) == NULL
	 || (tc_se = tcgstr("se", &bp, buf)) == NULL)
	    tc_so = tc_se = "";
	if ((tc_cl = tcgstr("cl", &bp, buf)) == NULL) {
	    printf("Can't find a `cl' termcap entry\n");
	    exit(1);
	}
	if (rpt_exist = tcgflg("rp", buf)) {
	    tc_rs = tcgstr("rs", &bp, buf);
	    tc_re = tcgstr("re", &bp, buf);
	    cp = dorpt(tc_rs, "X", tc_re, 10);
	    rptthresh = copy(cp, cp) - cp;
	}
	if ((tc_is = tcgstr("is", &bp, buf)) == NULL)
	    tc_is = "";
	if ((tc_ks = tcgstr("ks", &bp, buf)) == NULL)
	    tc_ks = "";
	if ((tc_ke = tcgstr("ke", &bp, buf)) == NULL)
	    tc_ke = "";
	if ((tc_ti = tcgstr("ti", &bp, buf)) == NULL)
	    tc_ti = "";
	if ((tc_te = tcgstr("te", &bp, buf)) == NULL)
	    tc_te = "";
	id[0] = 'k';
	id[2] = '\0';
	bp = (char *) junk;
	for (i = 0; i < 14; i++) {             /* do arrow & function keys */
	    id[1] = "ldur0123456789"[i];
	    cp = tcgstr(id, &bp, buf);
	    if (cp != NULL && cp[1] != '\0') {
		if (func_char == 0)
		    func_char = *cp;
		copy(++cp, funkey[i].f_in);
	    } else if (cp != NULL) {
		com_map[*cp & 0177] = funkey[i].f_token;
		*funkey[i].f_in = 0252;              /* unlikely character */
	    }
	}
	m_width = (tc_co - D_LWIDTH - D_RWIDTH - 2) | 1;    /* must be odd */
	m_height = (tc_li - MSG_MIN - 2) | 1;          /* must be odd also */
	if (m_width < m_height + m_height)                  /* make it 2:1 */
	    m_height = (m_width / 2 - 1) | 1;
	else
	    m_width = (m_height + m_height) | 1;
	m_hwidth = (double) m_width / 2.;    /* typecasting is for clarity */
	m_midw = M_LEFT + (int) m_hwidth;
	m_hheight = (double) m_height / 2.;
	m_midh = M_TOP + (int) m_hheight;
	msg_top = M_TOP + m_height + 1;
	msg_bottom = min(tc_li - 1, msg_top + MSG_MAX - 1);
	msg_lines = msg_bottom + 1 - msg_top;
	for (i = 0; i <= MAX_DIALS; i++) {
	    if (dialinfo[i].labx < 0)
		dialinfo[i].labx = M_LEFT + m_width + 1 - dialinfo[i].labx;
	    if (dialinfo[i].valx < 0)
		dialinfo[i].valx = M_LEFT + m_width + 1 - dialinfo[i].valx;
	}
	sleep(1);		/* for terminals with lousy termcap entries */
	tputsfh(tc_is, 60, 1, ospeed);
	sleep(1);		/* ditto */
	tputsfh(tc_ks, 60, 1, ospeed);
	sleep(1);		/* ditto */
	tputsfh(tc_ti, 60, 1, ospeed);
}

termdone()
{
	if (tc_ke != NULL)
	    tputsfh(tc_ke, 60, 1, ospeed);
	if (tc_te != NULL)
	    tputsfh(tc_te, 60, 1, ospeed);
}
moninit()
{
        register int i, j;
	extern char dislen;

	for (i = 0; i < m_width; i++)
	    for (j = 0; j < m_height; j++)
                old[i][j] = new[i][j] = 0;
	scrnclr();
	crtbox();                        /* draw box around bridge monitor */
	for (i = 0; i < MAX_O; i++)
            o[i] = 9876;
	for (i = 1; i <= MAX_LABELLED_DIAL; i++)
	    init_dial(i);
	dislen = 0;
	if (tc_dl == NULL) {
	    for (i = msg_lines; --i >= 0; )
		linlen[i] = 0;
	    curlin = msg_lines - 1;
	}
	xyfact = (double) m_height / (double) m_width;
}

display(secs, string)
char *string;
{
        register char *cp;
	register int i;
	extern char dislen;

	for (cp = string; *cp++; );
	dislen = cp - string;
	if (tc_dl != NULL) {
	    fputs(cursor(0, msg_top), stdout);
	    tputsfh(tc_dl, 10, 1, ospeed);                   /* scroll up 1 */
	    fputs(cursor(0, msg_bottom), stdout);
	    fputs(string, stdout);                          /* display msg */
	} else {
	    fputs(cursor(0, msg_top + curlin), stdout);
	    fputs("  ", stdout);                          /* wipe old ">>" */
	    curlin = (curlin + 1) % msg_lines;
	    printf("%s>> %s", cursor(0, msg_top + curlin), string);
	    i = linlen[curlin] - dislen;
	    while (--i >= 0)
		fputs(" ", stdout);
	    linlen[curlin] = dislen + 2;
	}
        sleep(secs);
}

disapp(secs, string)
char *string;
{
	register char *cp, *ap;
	register int length;
	short line, oldislen;
	extern char dislen;

	if (tc_dl == NULL)
	    dislen = linlen[curlin];
	for (ap = cp = string; *cp++; );
	length = dislen + cp - ap;
	if (length >= tc_co) {     /* find a spot to break string */
	    for (cp = &string[tc_co - dislen]; --cp > ap && *cp > ' '; );
	    if (cp > ap) {
		*cp++ = '\0';
		length = dislen + cp - ap;
	    } else {
		cp = ap;                        /* stuff for next line */
		ap = "";                              /* appended part */
	    }
	} else
	    cp = "";
	if (*ap) {
	    oldislen = dislen;
	    if (tc_dl == NULL) {
		line = msg_top + curlin;
		linlen[curlin] = length + 2;
	    } else {
		line = msg_bottom;
		dislen = length;
	    }
	    printf("%s%s", cursor(oldislen, line), ap);
	}
	if (*cp)
	    display(0, cp);
        sleep(secs);
}

/*VARARGS 2*/
dial_int(n, val0, val1, val2)
int     n, val0, val1, val2;
{
        struct dialstr *dip;

        dip = &dialinfo[n];
	printf("%s", cursor(dip->valx, dip->valy));
        printf(dip->valfmt, val0, val1, val2);
}

/*VARARGS 2*/
dial_dbl(n, val0, val1, val2)
int     n;
double  val0, val1, val2;
{
        struct dialstr *dip;

        dip = &dialinfo[n];
        printf("%s", cursor(dip->valx, dip->valy));
        printf(dip->valfmt, val0, val1, val2);
}

init_dial(n)                    /* put dial #n in initial state */
int     n;
{
        struct dialstr *dip;

        dip = &dialinfo[n];
	printf("%s%s", cursor(dip->labx, dip->laby), dip->label);
}

dials(which)
{
        register int i, j, k;
	long now;

	switch (which) {
	case ALL_DIALS:
	case MISS_ANG_DIAL:
	case MISS_DST_DIAL:
	    dp = pos[targ];
	    i = arcpsltan(dp[1], dp[2]);
	    i += 32;
	    i >>= 6;
	    j = arcpsltan(dp[0], dp[2]);
	    j += 32;
	    j >>= 6;
	    k = size(dp);
	    if (o[0] != i || o[1] != j)
		dial_int(MISS_ANG_DIAL,  o[0]=i, o[1]=j);
	    if (o[2] != k)
		dial_int(MISS_DST_DIAL, o[2]=k);
	    if (which != ALL_DIALS)
		break;
	case ENERGY_DIAL:
	    if (o[3] != (i=energy))
		dial_int(ENERGY_DIAL, o[3]=i);
	    if (which != ALL_DIALS)
		break;
	case ETIME_DIAL:
	    time(&now);
	    etime = now / 60. - time0;
	    if (o[4] != (i=etime))
		dial_int(ETIME_DIAL, o[4]=i);
	    if (which != ALL_DIALS)
		break;
	case SB_ANG_DIAL:
	case SB_DST_DIAL:
	    dp = pos[S_BASE_NUM];
	    i = arcpsltan(dp[1], dp[2]);
	    i += 32;
	    i >>= 6;
	    j = arcpsltan(dp[0], dp[2]);
	    j += 32;
	    j >>= 6;
	    k = size(dp);
	    if (o[5] != i || o[6] != j)
		dial_int(SB_ANG_DIAL,  o[5]=i, o[6]=j);
	    if (o[7] != k)
		dial_int(SB_DST_DIAL, o[7]=k);
	    if (which != ALL_DIALS)
		break;
	case RANGE_DIAL:
	    if (o[8] != (i=range))
		dial_int(RANGE_DIAL, o[8]=i);
	    if (which != ALL_DIALS)
		break;
	case SENSORS_DIAL:
	    if (o[9] != (i=sensors))
		dial_int(SENSORS_DIAL, o[9]=i);
	    if (which != ALL_DIALS)
		break;
	case VEL_ANG_DIAL:
	case VEL_DST_DIAL:
	    dp = vel[0];
	    i = arcpsltan(dp[1], dp[2]);
	    i += 32;
	    i >>= 6;
	    j = arcpsltan(dp[0], dp[2]);
	    j += 32;
	    j >>= 6;
	    if (o[10] != i || o[11] != j)
		dial_int(VEL_ANG_DIAL,  o[10]=i, o[11]=j);
	    k = size(dp) * 10.;
	    if (o[12] != k)
		dial_dbl(VEL_DST_DIAL, (o[12]=k) / 10.);
	    if (which != ALL_DIALS)
		break;
	case SPEED_DIAL:
	    k = v0[2] * 10.;
	    if (o[13] != k)
		dial_dbl(SPEED_DIAL, (o[13]=k) / 10.);
	    if (which != ALL_DIALS)
		break;
	case CONDITION_DIAL:
	    if (o[14] != (i=sensors/128.)) {
		o[14] = i;
		j = (int) "";
		if (i <= 0)
		    dial_int(CONDITION_DIAL, j, (int) " GREEN ", j);
		else if (i < 4)
		    dial_int(CONDITION_DIAL, j, (int) "YELLOW ", j);
		else
		    dial_int(CONDITION_DIAL,
		     (int) tc_so, (int) " RED ", (int) tc_se);
	    }
	    if (which != ALL_DIALS)
		break;
	case ROTATION_DIAL:
	    if (o[15] != xrots || o[16] != yrots || o[17] != zrots)
		dial_int(ROTATION_DIAL, o[15]=xrots, o[16]=yrots, o[17]=zrots);
	}
}

char    *
cursor(x, y)
int    x, y;
{
	register char *cp;
	static char cursbuf[4][4], next;

	next = (next + 1) & 3;
	cp = cursbuf[next];
	copy(tgoto(tc_cm, x, y), cp);
	return(cp);
}

char *
dorpt(rpt1, chstr, rpt2, count)
char    *rpt1, *chstr, *rpt2;
{
	register char *cp;
	static char buf[16];

	cp = copy(rptguts(rpt1, count), buf);
	cp = copy(chstr, cp);
	cp = copy(rptguts(rpt2, count), cp);
	return(buf);
}

/*
** Routine to do repeats
** "rptstr" contains printf-like escapes to allow insertion of the "count".
**      %d      as in printf
**      %2      like %2d
**      %3      like %3d
**      %.      gives %c
** The codes below affect the count but don't generate characters:
**      %+x     adds x to count (i.e. %+^A adds one to count)
**      %-x     subtracts x from count
**      %Q      where Q is anything else gives Q, (e.g. %% gives %)
** Example: the Ann Arbor Ambassador (ANSI std) uses :rp:rs=:re=%-^A\E[%db:
** which means 25 X's comes out as:  X<ESC>[24b  (which does work)
*/
char *
rptguts(rptstr, count)
register int count;
char    *rptstr;
{
	register char *bp, *cp = rptstr;
	register int c;
	static char buf[16];

	bp = buf;
	while (c = *cp++) {
	    if (c != '%') {
		*bp++ = c;
		continue;
	    }
	    switch (c = *cp++) {
	    case 'd':
		if (count < 10)
		    goto one;
		if (count < 100)
		    goto two;
	    case '3':
		*bp++ = (count / 100) | '0';
		count %= 100;
	    case '2':
two:            *bp++ = count / 10 | '0';
one:            *bp++ = count % 10 | '0';
		break;
	    case '.':
		*bp++ = count;
		break;
	    case '+':
		count += *cp++;
		break;
	    case '-':
		count -= *cp++;
		break;
	    default:
		*bp++ = c;
		break;
	    }
	}
	*bp = '\0';
	return (buf);
}

crtbox()
{
	register char *cp;
	register int i;
	int l_edge;
	char *boxline();

	l_edge = M_LEFT - 1;
	cp = boxline("+", "+", "+", "-");
	fputs(cursor(l_edge, M_TOP - 1), stdout);
	fputs(cp, stdout);
	fputs(cursor(l_edge, M_TOP + m_height), stdout);
	fputs(cp, stdout);
	cp = boxline("|", " ", "|", " ");
	for (i = M_TOP + m_height; --i >= M_TOP; ) {
	    fputs(cursor(l_edge, i), stdout);
	    fputs(cp, stdout);
	}
	fputs(cursor(l_edge, m_midh), stdout);
	fputs(boxline("+", " ", "+", " "), stdout);
}

char    *
boxline(left, middle, right, fill)
char    *left, *middle, *right, *fill;
{
	register char *cp;
	register int i;
	static char buf[80];

	cp = copy(left, buf);
	i = m_midw - M_LEFT;
	if (rpt_exist && i > rptthresh)
	    cp = copy(dorpt(tc_rs, fill, tc_re, i), cp);
	else
	    while (--i >= 0)
		cp = copy(fill, cp);
	cp = copy(middle, cp);
	i = m_midw - M_LEFT;
	if (rpt_exist && i > rptthresh)
	    cp = copy(dorpt(tc_rs, fill, tc_re, i), cp);
	else
	    while (--i >= 0)
		cp = copy(fill, cp);
	cp = copy(right, cp);
	return(buf);
}
