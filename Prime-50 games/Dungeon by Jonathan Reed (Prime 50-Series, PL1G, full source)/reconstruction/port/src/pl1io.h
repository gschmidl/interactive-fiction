/* ======================================================================
 *  pl1io.h - the little of Prime PL/I subset G that DUNGEON.PL1G needs:
 *  list-directed output and input, exactly as measured on PRIMOS 23.4
 *  running the original DUNGEON.SEG (see ..\README.md).
 *
 *  Output.  Items go into 7-column fields beginning at columns 1, 8, 15,
 *  22, ...  Each item is placed at the first field boundary at or after
 *  the current column; a numeric item is right justified in the six
 *  columns of its field (precision 4 + 2), a character item is written as
 *  it stands (CHARACTER(n) blank padded to n, VARYING at its own length);
 *  one blank follows every item.  PUT SKIP starts a new line first, PUT
 *  without SKIP continues on the current one.
 *
 *  Input.  GET SKIP LIST(x) discards whatever is left of the line the
 *  previous item came from, then skips blank lines until it finds a token,
 *  takes that one token and discards the rest of its line.
 * ====================================================================== */
#ifndef PL1IO_H
#define PL1IO_H

void put_skip(void);                  /* start a new line                 */
void put_str(const char *s);          /* character item                   */
void put_num(long v, int prec);       /* arithmetic item, FIXED(prec)      */
void put_pic(const char *s);          /* PICTURE item: written as it is   */
void put_end_line(void);              /* flush a partial line (at exit)   */

/* GET SKIP LIST: token into buf (blank padded to len-1 as CHARACTER(n)
   would be if pad is nonzero), returns the token length */
int  get_token(char *buf, int size, int pad);
/* the same, but giving up (an empty token) after `returns` blank lines -
   not PRIMOS's rule: the port's fix 1, see dungeon.c */
int  get_token_or_returns(char *buf, int size, int returns);
long get_number(void);                /* the game's own getnum, digits only */

#endif
