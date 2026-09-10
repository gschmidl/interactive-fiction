/* basic.c -- runtime shims, see basic.h. */

#include "basic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ---- output ---------------------------------------------------------- */

static int column = 0;

static void putch(int c)
{
    fputc(c, stdout);
    if (c == '\n')
        column = 0;
    else
        column++;
}

void Ps(const char *s)
{
    while (*s)
        putch((unsigned char)*s++);
}

void Pn(int v)
{
    char buf[32];
    /* DEC BASIC prints a number as <sign-or-blank><digits><blank>. */
    sprintf(buf, "%s%d ", v < 0 ? "" : " ", v);
    Ps(buf);
}

void Ptab(int col)
{
    while (column < col)
        putch(' ');
}

void NL(void)
{
    putch('\n');
    fflush(stdout);
}

void PL(const char *s)
{
    Ps(s);
    NL();
}

/* ---- input ----------------------------------------------------------- */

/* The program reads one character at a time and does its own line
 * assembly.  We buffer a whole line from stdin and hand it back
 * character by character, terminated by the CR the program looks for. */

static char inbuf[1024];
static size_t inpos = 0;
static size_t inlen = 0;
static int need_line = 1;

void bas_cib(void)
{
    inpos = inlen = 0;
    need_line = 1;
}

static void fill_line(void)
{
    fflush(stdout);
    if (!fgets(inbuf, (int)sizeof inbuf, stdin)) {
        /* Terminal hung up.  The 1987 program had no way out but death;
         * for a redirected transcript the sane thing is to stop. */
        NL();
        exit(0);
    }
    inlen = strlen(inbuf);
    while (inlen > 0 && (inbuf[inlen - 1] == '\n' || inbuf[inlen - 1] == '\r'))
        inbuf[--inlen] = '\0';
    /* The Tymshare terminals this was written for sent upper case. */
    {
        size_t i;
        for (i = 0; i < inlen; i++)
            inbuf[i] = (char)toupper((unsigned char)inbuf[i]);
    }
    inpos = 0;
    need_line = 0;
    column = 0;                     /* the user's newline moved the cursor */
}

int bas_getch(void)
{
    if (need_line)
        fill_line();
    if (inpos < inlen)
        return (unsigned char)inbuf[inpos++];
    need_line = 1;
    return 13;                                            /* carriage return */
}

void bas_input_line(char *dst, size_t dstsz)
{
    size_t n;
    fill_line();
    n = inlen;
    if (n >= dstsz)
        n = dstsz - 1;
    memcpy(dst, inbuf, n);
    dst[n] = '\0';
    need_line = 1;
}

/* ---- misc ------------------------------------------------------------ */

void bas_seed(unsigned s)
{
    srand(s ? s : (unsigned)time(NULL));
}

double bas_rnd(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

void bas_end(const char *msg)
{
    NL();
    if (msg && *msg)
        PL(msg);
    fflush(stdout);
    exit(0);
}
