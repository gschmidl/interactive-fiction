/*
**      Defines for CRT -- Termcap routine
**      psl 7/81
*/

#define CRT_H   "@(#)crt.h	1.7 2/20/84 -- PSL misc"

#define MAXCRT  16              /* number of simultaneous crt types */

/* x meta values for crt() */
#define C_INIT      -65         /* initialize terminal for cursor motion */
#define C_FINI      -66         /* return terminal to pre-init state */
#define C_CLEAR     -67         /* screen clear */
#define C_LINE_INS  -68         /* open line */
#define C_LINE_DEL  -69         /* close line */
#define C_LINE_ERA  -70         /* erase rest of line */
#define C_CHAR_INS  -71         /* insert character */
#define C_CHAR_DEL  -72         /* delete character */
#define C_CHAR_ERA  -73         /* erase current char */
#define C_REPEAT    -74         /* repeat spec char string */
#define C_FIELD     -76         /* attribute setting */
#define     F_NORMAL    0000    /*      normal */
#define     F_UNDER     0001    /*      underscored */
#define     F_BLINK     0002    /*      blink */
#define     F_STANDOUT  0004    /*      reverse video */
#define C_GRAPHICS  -77         /* alt. graphics char set */
        /* syms are: horiz line, vert line, cross, bottom T, top T,    */
        /* BL corner, TL corner, TR corner, BR corner, left T, right T */
struct crtstr {
	char    crt_name[16];   /* terminal type name */
        short   crt_sw;         /* screen width */
        char    crt_sh;         /* screen height */
        char    crt_rp;         /* does repeat exist? (NEW) */
#define CRT_FSCAP   crt_cm      /* first string capability */
        char    *crt_cm;        /* cursor addressing */
        char    *crt_al;        /* insert line */
        char    *crt_ce;        /* clear to e-o-l */
        char    *crt_cl;        /* clear screen */
        char    *crt_dl;        /* delete (close) line */
        char    *crt_mb;        /* mode blink */
        char    *crt_me;        /* mode end blink, bold, etc. */
        char    *crt_rs;        /* repeat start (NEW) */
        char    *crt_re;        /* repeat end (NEW) */
        char    *crt_se;        /* standout end */
        char    *crt_so;        /* standout (reverse) */
        char    *crt_te;        /* end programs using cursor motion */
        char    *crt_ti;        /* initialize in programs using cursor motion */
        char    *crt_ue;        /* underscore end */
        char    *crt_us;        /* underscore start */
#define CRT_LSCAP   crt_us      /* last string capability */
        short   crt_cmlen;      /* size of cursor addressing string */
        char    crt_buf[128];   /* where termcap info goes */
};
