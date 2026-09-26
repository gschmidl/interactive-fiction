/*
**      KBD.H -- Definitions for termcap keyboard lexer, kbd()
**      psl 7/81
** @(#)kbd.h	1.2  last mod 7/26/81 -- DPW misc
*/

#define MAXKBD  16              /* number of simultaneous kbd types */

/* key tokens */
#define K_F0        -1              /* func key */
#define K_F1        -2              /* func key */
#define K_F2        -3              /* func key */
#define K_F3        -4              /* func key */
#define K_F4        -5              /* func key */
#define K_F5        -6              /* func key */
#define K_F6        -7              /* func key */
#define K_F7        -8              /* func key */
#define K_F8        -9              /* func key */
#define K_F9        -10             /* func key */
#define K_BS        -11             /* backspace */
#define K_DARROW    -12             /* down arrow */
#define K_HOME      -13             /* home */
#define K_LARROW    -14             /* left arrow */
#define K_RARROW    -15             /* right arrow */
#define K_UARROW    -16             /* up arrow */

#define KBD_TOKS    16              /* number of tokens */
#define K_BUF       (32 + 4 * KBD_TOKS)  /* space for strings */

struct  kbdstr {
	char    kbd_name[16];       /* terminal type name */
	char	*kbd_ke;	    /* end keypad mode */
	char	*kbd_ks;	    /* start keypad mode */
	char    *kbd_str[KBD_TOKS]; /* pointers to defining strings */
	char    kbd_buf[K_BUF];     /* where termcap info goes */
};

extern  struct  kbdstr  kbds[];
