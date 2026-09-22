/* dx10.h - the part of DX10 3.7 (and of SCI) the Adventure task sees */
#ifndef DX10_H
#define DX10_H

#include <stdint.h>
#include <time.h>

struct dx10_config {
    int trace;                  /* --trace: each supervisor call on stderr */
    int fixes;                  /* 0 with --no-fixes */
    int unlimited;              /* -u: no prime time, no wait before a resume */
    const char *save_path;      /* host file behind UNIT5, or NULL (DUMY) */
    int virtual_clock;          /* --clock: a clock that ticks once a call */
    time_t clock;               /* its current reading */
};
extern struct dx10_config dx10;

/* the terminal (ME), as a 733-style teleprinter on the system console:
 * output is DX10's byte stream (records start with CR LF); input is upper-
 * cased as the reference terminals do. */
void term_write(const char *s, int n);
void term_puts(const char *s);
int term_gets(char *buf, int max);      /* -1 at the end of the input */
int term_gets_raw(char *buf, int max);  /* the same, case kept */
void term_end(void);

void dx10_now(struct tm *t);            /* the clock as the task sees it */
int dx10_svc(uint16_t blk);             /* XOP 15 */

/* why dx10_svc stopped the task */
#define DX10_END_TASK    1
#define DX10_END_PROGRAM 2
#define DX10_HANGUP      3              /* the input ran out */
#define DX10_UNKNOWN     4              /* a call this layer does not know */

#endif
