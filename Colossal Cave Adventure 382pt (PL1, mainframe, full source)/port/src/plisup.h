/* plisup.h -- PL/I language semantics for the Adventure transliteration.
 *
 * The C port of adventure.pli is meant to be read side by side with the
 * original PL/I, so it keeps PL/I's data model rather than C's:
 *
 *   - CHARACTER(n) is a fixed-length, blank-padded array of exactly n
 *     bytes.  It is NOT NUL-terminated.  Assignment truncates or
 *     blank-pads on the right; comparison blank-pads the shorter side.
 *   - CHARACTER(n) VARYING carries a current length from 0 to n.
 *   - SUBSTR is 1-based and is an lvalue as well as an rvalue.
 *   - INDEX and VERIFY return 1-based positions, 0 for "not found".
 *
 * Every routine here takes explicit lengths, and the PC()/PV() macros
 * below supply them from the declaration, so a CHARACTER(n) in the PL/I
 * becomes a plain "char x[n]" here with no hidden terminator.
 */
#ifndef PLISUP_H
#define PLISUP_H

#include <stddef.h>
#include <stdint.h>

/* ---- types ------------------------------------------------------- */

typedef int32_t fixed31;                 /* FIXED BINARY(31) */
typedef int16_t fixed15;                 /* FIXED BINARY(15) */
typedef unsigned char bit1;              /* BIT(1): 0 or 1     */

#define ON  1
#define OFF 0

/* CHARACTER(n) VARYING.  Declared with a matching max so that
 * sizeof-based length deduction still works. */
typedef struct { fixed31 len; char s[133]; } vchar133;
typedef struct { fixed31 len; char s[100]; } vchar100;

/* Pass a fixed CHARACTER(n) as (pointer, length). */
#define PC(x)  ((char *)(x)), ((int)sizeof(x))
/* Pass a character literal as (pointer, length) -- the trailing NUL of
 * the C literal is not part of the PL/I string. */
#define LIT(s) ((char *)(s)), ((int)sizeof(s) - 1)

/* ---- fixed-length CHARACTER ---------------------------------------- */

/* dst = src   (blank-pad or truncate on the right) */
void pl_assign(char *dst, int dlen, const char *src, int slen);

/* SUBSTR(dst,pos,len) = src   (pos is 1-based) */
void pl_substr_assign(char *dst, int dlen, int pos, int len,
                      const char *src, int slen);

/* Compare with blank padding of the shorter operand.  <0, 0, >0. */
int pl_cmp(const char *a, int alen, const char *b, int blen);
int pl_eq(const char *a, int alen, const char *b, int blen);

/* INDEX(hay,needle): 1-based position, 0 if absent. */
int pl_index(const char *hay, int hlen, const char *ned, int nlen);

/* VERIFY(s,set): 1-based position of the first character of s that is
 * not in set, 0 if every character is. */
int pl_verify(const char *s, int slen, const char *set, int setlen);

/* TRANSLATE(s,to,from) in place. */
void pl_translate(char *s, int slen,
                  const char *to, int tolen, const char *from, int fromlen);

/* Fill with blanks / with LOW(n) (X'00'). */
void pl_blank(char *s, int slen);
void pl_low(char *s, int slen);

/* Number of characters up to and including the last non-blank, i.e.
 * PL/I's  MAX(TRMLEN(s),1)  idiom used for trimming output. */
int pl_trimlen(const char *s, int slen);

/* ---- CHARACTER(n) VARYING ------------------------------------------ */

#define PV(v)     ((v).s), ((v).len)
#define PVMAX(v)  ((int)sizeof((v).s))

void pl_vassign(vchar133 *dst, const char *src, int slen);
void pl_vcat_char(vchar133 *dst, char c);          /* dst = dst || c */
/* SUBSTR(v,pos,len) = src, within the current length. */
void pl_vsubstr_assign(vchar133 *dst, int pos, int len,
                       const char *src, int slen);

/* ---- edit-directed conversion -------------------------------------- */

/* F(w): integer right-justified in w columns, blank-filled. */
void pl_edit_f(char *dst, int w, fixed31 value);
/* PICTURE 'Z9' style: leading zero suppressed, at least one digit. */
void pl_edit_zn(char *dst, int w, fixed31 value);
/* A(w) with a source of slen: left-justified, blank-padded. */
void pl_edit_a(char *dst, int w, const char *src, int slen);

/* GET STRING ... EDIT: read an F(w) field starting at 1-based pos. */
fixed31 pl_get_f(const char *s, int slen, int pos, int w);

/* Convert a fixed CHARACTER field of digits to a number (PICTURE read). */
fixed31 pl_pic_get(const char *s, int width);
/* Store a number back into a PICTURE'9..9' field of the given width. */
void pl_pic_put(char *s, int width, fixed31 value);

#endif /* PLISUP_H */
