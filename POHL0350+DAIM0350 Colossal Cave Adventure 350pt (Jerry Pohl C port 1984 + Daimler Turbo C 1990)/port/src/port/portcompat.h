/* ======================================================================
 *  portcompat.h - force-included (gcc -include) into every source file
 *  of both games, so that the 1984 (Pohl, CII C86) and 1990 (Daimler,
 *  Turbo C 2.0) sources build with a modern Windows gcc unchanged apart
 *  from a few edits marked PORT:.
 *
 *  Build with -DPORT_POHL or -DPORT_DAIMLER.  winport.c defines
 *  PORT_IMPLEMENTATION and gets the declarations but not the macros.
 * ====================================================================== */
#ifndef PORTCOMPAT_H
#define PORTCOMPAT_H

#include <stdio.h>
#ifdef PORT_DAIMLER
/* Daimler's files include these themselves; they must be seen before the
   macros below.  Pohl's files declare the library K&R style instead
   (extern long atoi(); extern unsigned strlen(); ...), which the ANSI
   headers would contradict, so for Pohl they stay out. */
#include <stdlib.h>
#include <string.h>
#endif

/* exit() is called with no argument in the 1984 code */
void port_exit(int code);

/* the text files are found beside the .exe; saved games stay in the
   current directory as before */
FILE *port_fopen(const char *name, const char *mode);

/* CII C86 ltoa(value, string) returned the length; Turbo C
   ltoa(value, string, radix) returned the string.  Both callers only test
   the result for zero, so one routine serves: it returns the length. */
int port_ltoa(long value, char *s, ...);

/* Turbo C putw/getw write and read a 16-bit int (the saved-game format) */
int port_putw(int w, FILE *f);
int port_getw(FILE *f);

/* BDS C / C86 setmem(address, count, byte) */
int setmem(void *p, unsigned n, int c);

/* the port's own options (--no-fixes, --restore, --help), taken out of
   argv before the authors' loop sees it; 0 in port_fixes with --no-fixes */
extern int port_fixes;
void port_options(int *argc, char ***argv);

#ifdef PORT_DAIMLER
/* Fix 3.  Daimler keeps message numbers in char variables ("char msg").
   Turbo C's char is signed, so on the PC every number above 127 went to
   rspeak() negative and printed text from a place picked out of the tables
   before idx4 (port_idx4 in database.c): READ LAMP said "e an object are
   attempting something beyond their ...".  With the fixes the number is
   the one Daimler wrote; with --no-fixes it is what Turbo C passed on. */
#define PORT_CHAR(c)  (port_fixes ? (int)(unsigned char)(c) : (int)(signed char)(c))
#endif

#ifndef PORT_IMPLEMENTATION
#define exit(...)   port_exit(__VA_ARGS__ + 0)
/* turn.c brings its own generator, rand() and srand(short); renamed so that
   it neither clashes with the C library's declarations nor is replaced by
   them.  rnum = rnum * 0x41C64E6D + 0x3039 on a long, built with -fwrapv. */
#define rand        adv_rand
#define srand       adv_srand
#define fopen       port_fopen
#define ltoa        port_ltoa
#define putw        port_putw
#define getw        port_getw
#ifdef PORT_POHL
/* Pohl names his countdown timer "clock", which is also a C library
   function; Daimler's copy already calls it clock1 */
#define clock       adv_clock
#endif
#endif

#endif
