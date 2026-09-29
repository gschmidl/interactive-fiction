/*  vmsrt.h -- shared between the VMS runtime, the keyed file layer and
 *  the image dispatcher.  Fortran entry points follow gfortran's
 *  convention: lower case, one trailing underscore, hidden character
 *  lengths of type size_t appended in argument order.
 */
#ifndef VMSRT_H
#define VMSRT_H

#include <stddef.h>

/*  which image the dispatcher should run next  */
enum { IMG_QUEST = 0, IMG_QUEST1, IMG_QUEST2, IMG_QUEST3, IMG_DNDOP };

/*  set by -f / QUEST_FAST: skip the timed pauses  */
extern int no_delay;

/*  the console ran out of input -- leave the way an idle terminal did  */
void eof_seen(void);
void input_seen(void);

void tty_init(void);
void tty_put(const char *s, int n);

/*  frozen clock support, so a transcript can be reproduced  */
long vms_now(void);

/*  Fortran entry points implemented in C  */
void inchk_(int *n);
void getinput_noecho_(char *buf, size_t len);
void ttyget_(char *buf, size_t len);
void ttygetnum_(int *value, int *err);
void format_(int *nlf, const char *str, size_t len);
void ttyrec_(const int *cc, const char *buf, size_t len);
void ttyraw_(const char *buf, size_t len);
void clrscr_(void);
void setscroll_(const int *top, const int *bot);
float vaxran_(int *seed);
float vsecnd_(float *x);
void libday_(int *days);
void vmstim_(char *buf, size_t len);
void basslp_(const int *secs);
void getname_(short *uic, char *username, size_t ulen);
void putcommon_(const char *buf, size_t len);
void getcommon_(char *buf, size_t len);
void runprog_(const char *file, size_t len);
void forexit_(void);
void delprc_(void);
void ttynl_(const int *n);
void qpath_(const char *name, char *path, size_t nlen, size_t plen);

/*  the indexed CHARACTER.DTA layer (keyed.c)  */
void kopen_(void);
void kclose_(void);
void kwrite_(char *rec, int *ios, size_t len);
void kread_key_(char *rec, const char *key, int *keyid, int *ge, int *ios,
                size_t reclen, size_t keylen);
void kfind_key_(const char *key, int *keyid, int *ge, int *ios, size_t keylen);
void kread_next_(char *rec, int *ios, size_t len);
void krewrite_(const char *rec, int *ios, size_t len);
void kdelete_(int *ios);
void kseq_rewind_(void);

#endif
