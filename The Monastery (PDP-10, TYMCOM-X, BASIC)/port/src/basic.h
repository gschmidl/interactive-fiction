/* basic.h -- the small slice of Tymshare TYMCOM-X BASIC that SAVE.TBA uses.
 *
 * The port is a transliteration, not a rewrite, so the runtime has to behave
 * the way the 1987 interpreter did in the places the program can notice:
 *
 *   PRINT a:b:c      ':' is the concatenating separator (what most BASICs
 *                    spell ';').  A trailing ':' suppresses the newline.
 *   numbers          print with a leading blank (or '-') and a trailing
 *                    blank, the usual DEC convention.
 *   TAB(n)           pads to column n, does nothing if already past it.
 *   INPUT IN FORM "X"  reads exactly one character; the program drives the
 *                    line editor itself, one char at a time, until CR.
 *   CIB              clears the type-ahead buffer.
 *   END "message"    prints the message and stops.
 */
#ifndef BASIC_H
#define BASIC_H

#include <stddef.h>

/* ---- output ---------------------------------------------------------- */

void Ps(const char *s);          /* PRINT s:            (no newline)      */
void Pn(int v);                  /* PRINT v:            (no newline)      */
void Ptab(int col);              /* TAB(col)                              */
void NL(void);                   /* PRINT               (bare newline)    */
void PL(const char *s);          /* PRINT s             (newline)         */

/* ---- input ----------------------------------------------------------- */

void bas_cib(void);              /* CIB                                   */
int  bas_getch(void);            /* INPUT IN FORM "X" -> one char, CR=13  */
void bas_input_line(char *dst, size_t dstsz);  /* INPUT IN FORM "R"       */

/* ---- misc ------------------------------------------------------------ */

double bas_rnd(void);            /* RND(x): uniform [0,1)                 */
void   bas_seed(unsigned s);
void   bas_end(const char *msg); /* END "msg"                             */

#endif /* BASIC_H */
