/* term.h - the user's terminal (SINTRAN logical device 1) on the host */
#ifndef TERM_H
#define TERM_H

/* charset for translating the 7-bit ND text */
enum { CS_NORWEGIAN, CS_ASCII, CS_SWEDISH };

void term_init(int raw, int charset);
/* the program drives a Facit screen terminal: translate its escape codes to
   VT sequences, and send the arrow and Home keys as Facit's ESC A-D and ESC H */
void term_set_facit(int on);
/* whether Esc breaks the program now; while it does, the arrow and Home
   keys are dropped, as their ESC could only break it */
void term_set_escape(int on);
/* whether the arrow and Home keys mean anything to the program at all */
void term_set_arrows(int on);
void term_hold(void);       /* wait for a key if this is the only process on the console */
void term_restore(void);
int  term_is_console(void);

/* next typed character as a 7-bit ND code; -1 at end of input */
int  term_getc(void);
/* how many typed characters are waiting (never blocks; a pipe reports 0) */
int  term_pending(void);
/* forget what has been typed ahead on the console (a pipe keeps its bytes) */
void term_clear_input(void);
/* Esc (if esc_counts) or Ctrl-C struck on the console while the program runs */
int  term_break_pending(int esc_counts);
/* one character from the program (parity is stripped here) */
void term_putc(int ch);
void term_flush(void);
/* host text (UTF-8 in the source) for the port's own messages */
void term_puts(const char *s);
/* read a line of host text for the port's own prompts (no ND translation) */
int  term_gets(char *buf, int n);

#endif
