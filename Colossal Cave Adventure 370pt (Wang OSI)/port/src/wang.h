/* wang.h -- shared state between the workstation core and its front ends. */
#ifndef WANG_H
#define WANG_H

#include <stdint.h>

#define SCR_ATTR   0xC000      /* attribute plane, 24 rows 256 bytes apart */
#define SCR_CHAR   0xE000      /* character plane, 24 rows 256 bytes apart */
#define SCR_ROWS   24
#define SCR_COLS   80

extern uint8_t mem[0x10000];
extern int     running;        /* cleared to stop the machine */
extern int     debug;
extern int     cur_row, cur_col;

/* ASCII for one byte of the Wang display code page (00h is the space). */
char wang_to_ascii(uint8_t c);

/* Queue a Wang keyboard scan code; the front end translates from host keys. */
void    kq_push(int scancode);
int     kq_empty(void);
uint8_t key_scancode(unsigned char ascii);   /* 0 when the key has no code */

/* Front end, implemented by con_win.c (shipping) or con_test.c (scripted). */
void con_init(void);
void con_refresh(void);
void con_poll(void);
void con_idle(void);          /* machine has nothing to do and no key queued */
void con_shutdown(void);

#endif
