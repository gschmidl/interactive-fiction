/* plisup.c -- PL/I language semantics.  See plisup.h. */
#include "plisup.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void pl_assign(char *dst, int dlen, const char *src, int slen)
{
    int n = slen < dlen ? slen : dlen;
    if (n > 0)
        memmove(dst, src, (size_t)n);
    if (n < dlen)
        memset(dst + n, ' ', (size_t)(dlen - n));
}

void pl_substr_assign(char *dst, int dlen, int pos, int len,
                      const char *src, int slen)
{
    if (pos < 1 || len <= 0)
        return;
    if (pos - 1 + len > dlen)
        len = dlen - (pos - 1);
    if (len <= 0)
        return;
    pl_assign(dst + pos - 1, len, src, slen);
}

int pl_cmp(const char *a, int alen, const char *b, int blen)
{
    int n = alen > blen ? alen : blen;
    int i;
    for (i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)(i < alen ? a[i] : ' ');
        unsigned char cb = (unsigned char)(i < blen ? b[i] : ' ');
        if (ca != cb)
            return ca < cb ? -1 : 1;
    }
    return 0;
}

int pl_eq(const char *a, int alen, const char *b, int blen)
{
    return pl_cmp(a, alen, b, blen) == 0;
}

int pl_index(const char *hay, int hlen, const char *ned, int nlen)
{
    int i;
    if (nlen <= 0 || nlen > hlen)
        return 0;
    for (i = 0; i + nlen <= hlen; i++)
        if (memcmp(hay + i, ned, (size_t)nlen) == 0)
            return i + 1;
    return 0;
}

int pl_verify(const char *s, int slen, const char *set, int setlen)
{
    int i, j;
    for (i = 0; i < slen; i++) {
        for (j = 0; j < setlen; j++)
            if (s[i] == set[j])
                break;
        if (j == setlen)
            return i + 1;
    }
    return 0;
}

void pl_translate(char *s, int slen,
                  const char *to, int tolen, const char *from, int fromlen)
{
    int i, j;
    for (i = 0; i < slen; i++)
        for (j = 0; j < fromlen; j++)
            if (s[i] == from[j]) {
                s[i] = j < tolen ? to[j] : ' ';
                break;
            }
}

void pl_blank(char *s, int slen)
{
    memset(s, ' ', (size_t)slen);
}

void pl_low(char *s, int slen)
{
    memset(s, '\0', (size_t)slen);
}

int pl_trimlen(const char *s, int slen)
{
    int i;
    for (i = slen; i > 0; i--)
        if (s[i - 1] != ' ')
            return i;
    return 0;
}

/* ---- VARYING -------------------------------------------------------- */

void pl_vassign(vchar133 *dst, const char *src, int slen)
{
    int max = (int)sizeof(dst->s);
    if (slen > max)
        slen = max;
    if (slen > 0)
        memmove(dst->s, src, (size_t)slen);
    dst->len = slen;
}

void pl_vcat_char(vchar133 *dst, char c)
{
    if (dst->len < (fixed31)sizeof(dst->s))
        dst->s[dst->len++] = c;
}

void pl_vsubstr_assign(vchar133 *dst, int pos, int len,
                       const char *src, int slen)
{
    if (pos < 1 || len <= 0)
        return;
    /* PL/I only lets SUBSTR reach within the current length of a
     * VARYING string; the engine always sets the string up first. */
    if (pos - 1 + len > dst->len)
        len = dst->len - (pos - 1);
    if (len <= 0)
        return;
    pl_assign(dst->s + pos - 1, len, src, slen);
}

/* ---- edit-directed conversion ---------------------------------------- */

void pl_edit_f(char *dst, int w, fixed31 value)
{
    char buf[32];
    int n = snprintf(buf, sizeof buf, "%*ld", w, (long)value);
    if (n > w)                       /* overflow: PL/I would raise ERROR */
        n = w;
    pl_assign(dst, w, buf, n);
}

void pl_edit_zn(char *dst, int w, fixed31 value)
{
    char buf[32];
    int n = snprintf(buf, sizeof buf, "%*ld", w, (long)value);
    if (n > w)
        n = w;
    pl_assign(dst, w, buf, n);
}

void pl_edit_a(char *dst, int w, const char *src, int slen)
{
    pl_assign(dst, w, src, slen);
}

fixed31 pl_get_f(const char *s, int slen, int pos, int w)
{
    char buf[64];
    int i, n = 0;
    if (pos < 1 || w <= 0 || pos - 1 + w > slen)
        return 0;
    for (i = 0; i < w && n < (int)sizeof buf - 1; i++) {
        char c = s[pos - 1 + i];
        if (c == ' ')                 /* PL/I F-format ignores blanks */
            continue;
        buf[n++] = c;
    }
    buf[n] = '\0';
    if (n == 0)
        return 0;
    return (fixed31)strtol(buf, NULL, 10);
}

fixed31 pl_pic_get(const char *s, int width)
{
    char buf[32];
    int i, n = 0;
    for (i = 0; i < width && n < (int)sizeof buf - 1; i++)
        buf[n++] = (s[i] == ' ') ? '0' : s[i];
    buf[n] = '\0';
    return (fixed31)strtol(buf, NULL, 10);
}

void pl_pic_put(char *s, int width, fixed31 value)
{
    char buf[32];
    long v = (long)value;
    int i;
    if (v < 0)
        v = -v;
    snprintf(buf, sizeof buf, "%0*ld", width, v);
    /* PICTURE truncates on the left when the value is too wide. */
    i = (int)strlen(buf) - width;
    if (i < 0)
        i = 0;
    memcpy(s, buf + i, (size_t)width);
}
