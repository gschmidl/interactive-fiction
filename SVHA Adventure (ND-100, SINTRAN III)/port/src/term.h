/* term.h - the user's terminal (SINTRAN logical device 1) on the host */
#ifndef TERM_H
#define TERM_H

/* charset for translating the 7-bit ND text */
enum { CS_NORWEGIAN, CS_ASCII };

void term_init(int raw, int charset);
void term_hold(void);        /* wait for a key if this is the only process on the console */
void term_restore(void);
int  term_is_console(void);

/* next typed character as a 7-bit ND code; -1 at end of input */
int  term_getc(void);
/* one character from the program (parity is stripped here) */
void term_putc(int ch);
void term_flush(void);
/* host text (UTF-8 in the source) for the port's own messages */
void term_puts(const char *s);
/* read a line of host text for the port's own prompts (no ND translation) */
int  term_gets(char *buf, int n);

#endif
