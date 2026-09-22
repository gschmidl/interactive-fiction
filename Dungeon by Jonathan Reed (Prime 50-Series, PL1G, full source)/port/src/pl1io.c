/* ======================================================================
 *  pl1io.c - see pl1io.h.  Every rule here was measured against the
 *  original running under PRIMOS 23.4 on p50em, not guessed.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "pl1io.h"

#define FIELD 7                        /* field width, stops at 1, 8, 15 ... */

static int col = 1;                    /* next column to write (1 based)     */
static int line_open = 0;

static void out(const char *s, size_t n)
{
    fwrite(s, 1, n, stdout);
    col += (int)n;
    line_open = 1;
}

static void pad_to_field(void)
{
    /* the item starts at the first field boundary at or after col */
    int stop = 1;
    while (stop < col)
        stop += FIELD;
    while (col < stop) {
        fputc(' ', stdout);
        col++;
    }
}

void put_skip(void)
{
    fputc('\n', stdout);
    col = 1;
    line_open = 0;
}

void put_str(const char *s)
{
    pad_to_field();
    out(s, strlen(s));
    out(" ", 1);                       /* the blank that follows every item  */
}

void put_pic(const char *s)
{
    put_str(s);
}

void put_num(long v, int prec)
{
    char buf[32];
    int width = prec + 2;
    int n = snprintf(buf, sizeof buf, "%*ld", width, v);
    pad_to_field();
    out(buf, (size_t)n);
    out(" ", 1);
}

void put_end_line(void)
{
    if (line_open) {
        fputc('\n', stdout);
        col = 1;
        line_open = 0;
    }
}

/* ---------------------------------------------------------------------- */
/*  Input                                                                 */
/* ---------------------------------------------------------------------- */

static int echo = -1;                  /* echo input when it is not a tty   */

static int echoing(void)
{
    if (echo < 0) {
#ifdef _WIN32
        echo = !_isatty(_fileno(stdin));
#else
        echo = !isatty(fileno(stdin));
#endif
    }
    return echo;
}

/* read one line; returns 0 at end of input */
static int read_line(char *buf, int size)
{
    fflush(stdout);
    if (!fgets(buf, size, stdin)) {
        put_end_line();
        exit(0);
    }
    {
        size_t n = strlen(buf);
        while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
            buf[--n] = 0;
        if (echoing()) {
            /* a terminal echoes the typed line and its carriage return, so
               the PUT SKIP that follows leaves a blank line, as on PRIMOS */
            fputs(buf, stdout);
            fputc('\n', stdout);
            col = 1;
            line_open = 0;
        }
    }
    return 1;
}

/* GET SKIP LIST; with returns > 0 it also gives up after that many blank
   lines (the port's fix for "press <return> twice", see dungeon.c) */
static int scan_token(char *dst, int size, int pad, int returns)
{
    char line[512];
    int blanks = 0;
    for (;;) {
        char *p, *q;
        int n = 0;
        read_line(line, sizeof line);
        p = line;
        while (*p == ' ' || *p == '\t' || *p == ',')
            p++;
        if (!*p) {
            if (returns > 0 && ++blanks >= returns) {
                *dst = 0;
                return 0;
            }
            continue;                  /* blank line: keep looking          */
        }
        q = dst;
        while (*p && *p != ' ' && *p != '\t' && *p != ',' && n < size - 1) {
            *q++ = *p++;
            n++;
        }
        *q = 0;
        if (pad)
            while (n < size - 1)
                dst[n++] = ' ', dst[n] = 0;
        return n;
    }
}

int get_token(char *dst, int size, int pad)
{
    return scan_token(dst, size, pad, 0);
}

int get_token_or_returns(char *dst, int size, int returns)
{
    return scan_token(dst, size, 0, returns);
}

/* The game's own GETNUM: read a token, accept digits only, else complain.
   Its NUM is CHARACTER(4) VARYING, so only the first four characters of the
   token are seen - "12345" is read as 1234. */
long get_number(void)
{
    char tok[5];
    for (;;) {
        long v = 0;
        int i, len = get_token(tok, (int)sizeof tok, 0);
        int ok = len > 0;
        for (i = 0; i < len; i++) {
            if (tok[i] < '0' || tok[i] > '9') {
                ok = 0;
                break;
            }
            v = v * 10 + (tok[i] - '0');
        }
        if (ok)
            return v;
        put_skip();
        put_str("Invalid entry, try again:");
    }
}
