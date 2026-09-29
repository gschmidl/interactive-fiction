/* ======================================================================
 *  portcompat.h - force-included (gcc -include) into every source file of
 *  Robert R. Hall's "Generic Adventure -- Version:7.0, July 1994".
 *
 *  Hall's C is already portable; the port layer only
 *    - finds the text files (advent1..4.dat) beside the .exe when they are
 *      not in the current directory (TEXTDIR was /usr/lib/advent/), and
 *    - with -DTESTRAND, swaps the C library's rand() for a fixed generator
 *      shared by the Windows and Linux test builds, so their transcripts
 *      can be compared byte for byte.  Never used in the release build.
 *  winport.c defines PORT_IMPLEMENTATION: declarations only, no macros.
 * ====================================================================== */
#ifndef PORTCOMPAT_H
#define PORTCOMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

FILE *port_fopen(const char *name, const char *mode);

/* the port's options (--no-fixes, --help), taken out of argv before the
   game looks at it; 0 in port_fixes with --no-fixes */
extern int port_fixes;
void port_options(int *argc, char ***argv);

#ifdef TESTRAND
int  port_rand(void);
void port_srand(unsigned seed);
#endif

#ifndef PORT_IMPLEMENTATION
#define fopen port_fopen
#ifdef TESTRAND
#define rand  port_rand
#define srand port_srand
#endif
#endif

#endif
