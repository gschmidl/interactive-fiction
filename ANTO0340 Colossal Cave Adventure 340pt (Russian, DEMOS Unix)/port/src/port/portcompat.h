/* ====================================================================== *
 *  portcompat.h -- Windows port shim for the Russian 340-point           *
 *                  "ПРИКЛЮЧЕНИЕ" (DEMOS Unix, 1984-85).                  *
 *                                                                        *
 *  Force-included into every original source file with -include, so the  *
 *  1984 K&R C stays untouched.  It supplies:                             *
 *                                                                        *
 *    * real prototypes for the routines the old code called without      *
 *      declaring -- on a 64-bit target an implicit "int" return          *
 *      truncates a pointer and the program crashes,                      *
 *    * binary file modes and legal Windows file names,                   *
 *    * data files resolved next to the .exe rather than the cwd,         *
 *    * a KOI8-R <-> Unicode console bridge, replacing the "luit          *
 *      -encoding KOI8-R" wrapper the Linux build relies on.              *
 * ====================================================================== */

#ifndef PORTCOMPAT_H
#define PORTCOMPAT_H

/* --- system headers first, so the macros below never rewrite them ----- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>

/* --- declarations for the game's own routines ------------------------- *
 *  Deliberately old-style (empty parens): the definitions are K&R, so a   *
 *  modern prototype here would be a conflicting type.  What matters is    *
 *  the RETURN type -- an undeclared function is assumed to return int,    *
 *  which silently truncates a 64-bit pointer.                             */
char *conv();
int   advtolower();     /* объявлен как в оригинале: возвращает int */

int  vocab();   int  fatal();   int  act();     int  pct();     int  yes();
int  dark();    int  at();      int  here();    int  get();     int  iniget();
int  mes();     int  rspeak();  int  descr();   int  descr2();  int  indobj();
int  chnloc();  int  specia();  int  score();   int  mscore();  int  freeze();
int  loadfr();  int  savecm();  int  loadcm();  int  slcm();    int  events();
int  ini();     int  action();  int  motion();  int  ds();      int  screen();
int  getlin();  int  getwrd();  int  scan();    int  getobj();  int  putmes();
int  putcnd();  int  condit();  int  outd();    int  _outt();   int  stat();
int  ivocab();  int  iobjec();  int  iactio();  int  iclass();  int  icave();
int  imessa();  int  ievent();

/* --- the port layer (port/winport.c, port/koi8con.c) ------------------ */
int   port_open  (const char *name, int flags);
FILE *port_fopen (const char *name, const char *mode);
int   port_unlink(const char *name);
int   port_read  (int fd, void *buf, unsigned n);
int   port_write (int fd, const void *buf, unsigned n);
int   port_printf(const char *fmt, ...);
void  port_init  (void);

#ifndef PORT_IMPLEMENTATION      /* the port layer itself needs the real ones */

/* Our own tolower() would collide with the C library's; the game's takes
   a buffer + length and is nothing like it. */
#define tolower   advtolower

/* Every open/read/write in the game goes through the port layer: fd 0 and
   fd 1 become the KOI8-R console bridge, everything else is a real file
   forced into binary mode. */
#define open      port_open
#define fopen     port_fopen
#define unlink    port_unlink
#define read      port_read
#define write     port_write
#define printf    port_printf

#endif /* !PORT_IMPLEMENTATION */

#ifdef TESTRAND
/* Deterministic RNG shared by the Windows and Linux test builds, so their
   transcripts can be diffed byte for byte.  Never enabled in a release. */
int  port_rand(void);
void port_srand(unsigned s);
# ifndef PORT_IMPLEMENTATION
#  define rand     port_rand
#  define srand    port_srand
# endif
#endif

#endif /* PORTCOMPAT_H */
