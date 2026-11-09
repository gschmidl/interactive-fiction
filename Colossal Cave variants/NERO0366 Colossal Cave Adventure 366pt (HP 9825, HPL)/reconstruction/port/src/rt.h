/* Runtime for the HPL programs of NERO0366 translated to C by hpl2c.py.
   Numbers are doubles (HPL's are 12-digit decimal; the game only uses
   integers and halves).  Strings are byte strings of a fixed maximum
   length, as dimensioned; fti/itf pack 16-bit integers into them. */
#ifndef RT_H
#define RT_H

#include <stddef.h>

/* ---- data files (data.c) --------------------------------------------- */
typedef struct { unsigned char t; double n; const char *s; int len; } HItem;  /* t: 1 number, 2 string */
typedef struct { int nitems; const HItem *it; } HRec;
typedef struct { const char *name; int nrec; const HRec *rec; } HFile;
extern const HFile hfiles[];

/* ---- values ------------------------------------------------------------ */
typedef struct { int len; const unsigned char *d; } SV;      /* string value (in the arena) */

typedef struct {               /* string variable: n == 0 simple, else elements 1..n */
    int n, max;
    int *len;
    unsigned char *d;          /* element i at d + i*max */
} HSVar;

typedef struct { int n; double *d; } NArr;   /* numeric array, elements 1..n */

/* ---- options ------------------------------------------------------------ */
extern int opt_fixes;          /* the port's fixes (on unless --no-fixes) */
extern int rt_interactive;     /* stdin is a console: no echo of commands */
extern int opt_width;
extern double flg13;
int rt_echo(void);             /* print the player's command after reading it */
int rt_reflow(void);           /* join data-file lines and refill them to the width */

/* ---- arena for string temporaries ---------------------------------------- */
size_t rt_mark(void);
void rt_release(size_t m);

/* ---- strings -------------------------------------------------------------- */
SV rt_lit(const char *s, int len);
SV rt_sget(HSVar *v, int nsub, double a, double b, double c);
SV rt_sset(HSVar *v, int nsub, double a, double b, double c, SV val);
SV rt_cat(SV a, SV b);
SV rt_cap(SV a);
SV rt_fti(double x);
double rt_itf(SV a);
double rt_len(SV a);
double rt_pos(SV a, SV b);
int rt_scmp(SV a, SV b);
void rt_sdim(HSVar *v, double n, double max);   /* n < 0: simple string */
void rt_ndim(NArr *a, double n);

/* ---- numbers -------------------------------------------------------------- */
long rt_round(double x);
double rt_int(double x);
double rt_mod(double a, double b);
double rt_prnd(double x, double n);
double rt_rnd(double x);
double rt_bit(double n, double x);
double rt_max(double a, double b);
double rt_min(double a, double b);
double *rt_aptr(NArr *a, double i);

/* ---- output, input, buffer -------------------------------------------------- */
void rt_out(SV s);                       /* wrt S,s / prt s: one line */
void rt_out_blank(void);                 /* wrt S */
void rt_dsp(SV s);
void rt_ent_str(SV prompt, HSVar *v, int nsub, double a, double b, double c);
void rt_ent_num(SV prompt, double *x);
typedef struct { char kind; const char *s; } FmtItem;   /* 's' text, '/', 'f' number, 'c' string, 0 end */
void rt_fmt(int n, const FmtItem *items);
void rt_wrt_begin(int fmt);
void rt_wrt_num(double x);
void rt_wrt_str(SV s);
void rt_wrt_end(void);
void rt_red(HSVar *v);
double rt_rds(void);

/* ---- files --------------------------------------------------------------------- */
double rt_asgn(const char *name, double f);
void rt_open(const char *name, double nrec);
void rt_rread(double f, double rec);
void rt_rd_num(double f, double *x);
void rt_rd_narr(double f, NArr *a);
void rt_rd_str(double f, HSVar *v, int nsub, double a, double b, double c);
void rt_rd_sarr(double f, HSVar *v);
void rt_wr_num(double f, double x);
void rt_wr_str(double f, SV s);
void rt_wr_narr(double f, NArr *a);
void rt_wr_sarr(double f, HSVar *v);
void rt_wr_eof(double f);
void rt_wr_done(double f);
double rt_type(double f);

/* ---- control -------------------------------------------------------------------- */
void rt_chain(int prog, int line);
void rt_stop(void);
void rt_fail(const char *msg, int prog, int line);

#endif
