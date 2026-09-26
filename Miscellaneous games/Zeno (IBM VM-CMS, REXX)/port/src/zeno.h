/* ------------------------------------------------------------------
 * ZENO - native port of Dave Mitchell's 1983 VM/CMS REXX adventure.
 *
 * The game itself (ZENO.EXEC) is the author's original source, run
 * unmodified by Regina REXX.  What this program supplies is the
 * mainframe underneath it: the IOS3270 full-screen panel handler and
 * the handful of CMS commands the exec issues.
 * ------------------------------------------------------------------ */
#ifndef ZENO_H
#define ZENO_H

#include <stddef.h>

#define SROWS 24
#define SCOLS 80
#define SCELLS (SROWS * SCOLS)

/* per-cell display flags */
#define C_ATTR    0x01   /* cell holds a 3270 attribute byte (shows blank) */
#define C_INTENS  0x02
#define C_INPUT   0x04
#define C_NONDISP 0x08

/* field flags */
#define F_INPUT   0x01
#define F_INTENS  0x02
#define F_NONDISP 0x04
#define F_SKIP    0x08   /* autoskip */
#define F_PEN     0x10   /* light-pen detectable */

#define MAXFIELDS 128
#define MAXVAR    72

typedef struct {
    int      attrpos;         /* linear position of the attribute byte */
    int      len;             /* data length, in columns */
    unsigned flags;
    char     var[MAXVAR];     /* REXX variable read back from this field */
    int      mdt;             /* modified-data tag */
} Field;

typedef struct {
    char          ch[SCELLS];
    unsigned char fl[SCELLS];
    short         fid[SCELLS];        /* field index + 1, 0 = none */
    Field         f[MAXFIELDS];
    int           nf;

    char          pf[25][32];         /* PF key values, 1..24 */
    int           pass_on_pf;         /* .y  - return input on PF keys too */
    int           no_edit;            /* .n  - do not fold/strip input data */
    int           alarm;              /* .a  */
    int           cursor_row;         /* 0-based; -1 = first input field */
    int           cursor_col;
    int           cursor_field;       /* 1-based nth input field, 0 = unset */
} Screen;

/* ---- panel engine (panel.c) ---------------------------------------- */
int  panel_render(Screen *s, const char *file, const char *label);
void panel_set_libdir(const char *dir);

/* ---- variable pool, supplied by cms.c ------------------------------- */
int  rx_fetch(const char *name, char *buf, int bufsize);   /* -> length, -1 unset */
int  rx_set(const char *name, const char *val, int len);

/* ---- console (screen.c) --------------------------------------------- */
#define K_ENTER   1
#define K_PF      2      /* aux = 1..24 */
#define K_PA      3      /* aux = 1..3  */
#define K_CLEAR   4
#define K_QUIT    5      /* window closed / unrecoverable */

int  con_open(void);
extern int screen_batch;
void con_close(void);
void con_show(Screen *s);
int  con_read(Screen *s, int *aux);     /* paints, edits fields, returns K_* */
void con_message(const char *msg);      /* line-mode output, e.g. SAY */

/* ---- misc ------------------------------------------------------------ */
extern int zeno_debug;
void zlog(const char *fmt, ...);

#endif
