/* harness.c - render panels without REXX, to eyeball the geometry.
 * Build: gcc -o harness harness.c panel.c
 * Usage: harness <gamedir> <;LABEL> [var=value ...]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "zeno.h"

int zeno_debug = 1;

static struct { char n[MAXVAR]; char v[512]; } vars[256];
static int nvars;

void zlog(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    fputs("  [log] ", stderr); vfprintf(stderr, fmt, ap); fputc('\n', stderr);
    va_end(ap);
}

int rx_fetch(const char *name, char *buf, int bufsize)
{
    int i, n;
    for (i = 0; i < nvars; i++)
        if (!strcmp(vars[i].n, name)) {
            n = (int)strlen(vars[i].v);
            if (n > bufsize) n = bufsize;
            memcpy(buf, vars[i].v, n);
            return n;
        }
    (void)buf; (void)bufsize;
    return -1;
}

int rx_set(const char *name, const char *val, int len)
{
    (void)name; (void)val; (void)len;
    return 0;
}

static void setvar(const char *nv)
{
    const char *eq = strchr(nv, '=');
    if (!eq || nvars >= 256) return;
    snprintf(vars[nvars].n, MAXVAR, "%.*s", (int)(eq - nv), nv);
    snprintf(vars[nvars].v, 512, "%s", eq + 1);
    nvars++;
}

int main(int argc, char **argv)
{
    Screen s;
    int r, c, i;
    if (argc < 3) { fprintf(stderr, "usage: harness <dir> <;LABEL> [v=x...]\n"); return 1; }
    for (i = 3; i < argc; i++) setvar(argv[i]);
    panel_set_libdir(argv[1]);
    if (!panel_render(&s, "ZENO.IOS3270", argv[2]))
        fprintf(stderr, "  [!] section %s not found\n", argv[2]);

    printf("     ....+....1....+....2....+....3....+....4"
           "....+....5....+....6....+....7....+....8\n");
    for (r = 0; r < SROWS; r++) {
        printf("%3d |", r + 1);
        for (c = 0; c < SCOLS; c++) {
            int p = r * SCOLS + c;
            unsigned char fl = s.fl[p];
            char ch = s.ch[p];
            if (fl & C_ATTR)  ch = (fl & C_INPUT) ? '[' : '<';
            printf("%c", ch);
        }
        printf("|\n");
    }
    printf("fields: %d\n", s.nf);
    for (i = 0; i < s.nf; i++)
        printf("  #%2d row %2d col %2d len %3d %s%s%s var=%s\n", i + 1,
               s.f[i].attrpos / SCOLS + 1, s.f[i].attrpos % SCOLS + 1, s.f[i].len,
               (s.f[i].flags & F_INPUT) ? "INPUT " : "",
               (s.f[i].flags & F_INTENS) ? "INTENS " : "",
               (s.f[i].flags & F_SKIP) ? "SKIP " : "", s.f[i].var);
    for (i = 1; i <= 12; i++)
        if (s.pf[i][0]) printf("  PF%-2d = %s\n", i, s.pf[i]);
    return 0;
}
