/*
 * amigamud.c -- a native build of amigamud.py, the interpreter for the
 * Amiga MUD scripting language (Chris Gray, 1991) that loads and plays the
 * "uncle" demo world (go, code.m, rooms.m, books.m, spells.m, words.m).
 *
 * A translation of amigamud.py, statement for statement, so that it
 * behaves the same way: the same output byte for byte (textwrap's line
 * filling included), the same warnings, errors and lint messages.  Python's
 * bool is kept apart from int as the original is careful to do.  Strings
 * are bytes; the world files are ASCII.
 *
 *     gcc -O2 -s -o amigamud.exe amigamud.c -Wl,--stack,33554432
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

/* ====================================================================
 *  Memory and byte buffers (nothing is ever freed: a session is short)
 * ==================================================================== */

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) {
        fputs("amigamud: out of memory\n", stderr);
        exit(1);
    }
    return p;
}

static void *xrealloc(void *p, size_t n)
{
    p = realloc(p, n ? n : 1);
    if (!p) {
        fputs("amigamud: out of memory\n", stderr);
        exit(1);
    }
    return p;
}

static char *xstrndup(const char *s, size_t n)
{
    char *p = xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static char *xstrdup(const char *s)
{
    return xstrndup(s, strlen(s));
}

typedef struct {
    char *s;
    size_t n, cap;
} Buf;

static void buf_need(Buf *b, size_t k)
{
    if (b->n + k + 1 > b->cap) {
        size_t c = b->cap ? b->cap : 64;
        while (c < b->n + k + 1)
            c *= 2;
        b->s = xrealloc(b->s, c);
        b->cap = c;
    }
}

static void buf_putn(Buf *b, const char *s, size_t k)
{
    buf_need(b, k);
    if (k)
        memcpy(b->s + b->n, s, k);
    b->n += k;
    b->s[b->n] = 0;
}

static void buf_puts(Buf *b, const char *s)
{
    buf_putn(b, s, strlen(s));
}

static void buf_putc(Buf *b, int c)
{
    char ch = (char)c;
    buf_putn(b, &ch, 1);
}

static void buf_vprintf(Buf *b, const char *fmt, va_list ap)
{
    va_list ap2;
    int k;
    va_copy(ap2, ap);
    k = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (k < 0)
        return;
    buf_need(b, (size_t)k);
    vsnprintf(b->s + b->n, (size_t)k + 1, fmt, ap);
    b->n += (size_t)k;
}

static void buf_printf(Buf *b, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    buf_vprintf(b, fmt, ap);
    va_end(ap);
}

static void buf_clear(Buf *b)
{
    b->n = 0;
    if (b->s)
        b->s[0] = 0;
}

static char *fmt(const char *f, ...)
{
    Buf b = {0};
    va_list ap;
    va_start(ap, f);
    buf_vprintf(&b, f, ap);
    va_end(ap);
    return b.s ? b.s : xstrdup("");
}

/* A Python string may hold NUL (the escape \0, or a typed one); here a
   string is C text, so a NUL is kept as the pair C0 80, counted as one
   character, and written out as NUL. */
static int nul_at(const char *s)
{
    return (unsigned char)s[0] == 0xC0 && (unsigned char)s[1] == 0x80;
}

static size_t vlen_n(const char *s, size_t n)       /* the length Python sees */
{
    size_t i, k = 0;
    for (i = 0; i < n; i++, k++)
        if (i + 1 < n && nul_at(s + i))
            i++;
    return k;
}

static size_t vlen(const char *s)
{
    return vlen_n(s, strlen(s));
}

static void put_text(FILE *f, const char *s, size_t n)
{
    size_t i = 0, j;
    while (i < n) {
        for (j = i; j < n && !(j + 1 < n && nul_at(s + j)); j++)
            ;
        fwrite(s + i, 1, j - i, f);
        if (j < n) {
            fputc(0, f);
            j += 2;
        }
        i = j;
    }
}

static void put_str(FILE *f, const char *s)
{
    put_text(f, s, strlen(s));
}

/* ---- Python's str methods on ASCII ---- */

static int py_space(int c)
{
    return c == ' ' || (c >= '\t' && c <= '\r') || (c >= 0x1c && c <= 0x1f);
}

static char *lower(const char *s)
{
    char *p = xstrdup(s), *q;
    for (q = p; *q; q++)
        if (*q >= 'A' && *q <= 'Z')
            *q = (char)(*q + 32);
    return p;
}

/* s.strip(chars) - chars NULL: whitespace */
static char *strip_chars(const char *s, const char *chars, int left, int right)
{
    size_t a = 0, b = strlen(s);
#define IN_SET(c) (chars ? ((c) && strchr(chars, (c)) != NULL) : py_space((unsigned char)(c)))
    if (left)
        while (a < b && IN_SET(s[a]))
            a++;
    if (right)
        while (b > a && IN_SET(s[b - 1]))
            b--;
#undef IN_SET
    return xstrndup(s + a, b - a);
}

static char *strip(const char *s)
{
    return strip_chars(s, NULL, 1, 1);
}

/* s.split() on whitespace, or s.split(sep) on a character */
static char **split_ws(const char *s, int *n)
{
    char **out = NULL;
    int k = 0;
    size_t i = 0, len = strlen(s), j;
    while (i < len) {
        while (i < len && py_space((unsigned char)s[i]))
            i++;
        if (i >= len)
            break;
        j = i;
        while (j < len && !py_space((unsigned char)s[j]))
            j++;
        out = xrealloc(out, sizeof(char *) * (size_t)(k + 1));
        out[k++] = xstrndup(s + i, j - i);
        i = j;
    }
    *n = k;
    return out;
}

static char **split_ch(const char *s, char sep, int *n)
{
    char **out = NULL;
    int k = 0;
    const char *p = s, *q;
    for (;;) {
        q = strchr(p, sep);
        out = xrealloc(out, sizeof(char *) * (size_t)(k + 1));
        if (!q) {
            out[k++] = xstrdup(p);
            break;
        }
        out[k++] = xstrndup(p, (size_t)(q - p));
        p = q + 1;
    }
    *n = k;
    return out;
}

static int ends_with(const char *s, const char *suf)
{
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && strcmp(s + a - b, suf) == 0;
}

/* ====================================================================
 *  Values
 * ==================================================================== */

typedef enum { V_NIL, V_BOOL, V_INT, V_STR, V_STATUS, V_THING, V_LIST, V_PROP,
               V_GRAMMAR, V_PROC, V_BUILTIN } VT;

enum { ST_SUCCEED, ST_FAIL, ST_CONTINUE };
static const char *STATUS_NAME[] = {"succeed", "fail", "continue"};

typedef struct Thing Thing;
typedef struct TList TList;
typedef struct Prop Prop;
typedef struct Grammar Grammar;
typedef struct Proc Proc;
typedef struct Builtin Builtin;
typedef struct Node Node;

typedef struct {
    VT t;
    union {
        long long i;            /* BOOL, INT, STATUS */
        char *s;
        Thing *th;
        TList *l;
        Prop *p;
        Grammar *g;
        Proc *pr;
        Builtin *b;
    } u;
} Val;

enum { K_STRING, K_INT, K_BOOL, K_THING, K_ACTION, K_LIST };
static const char *KIND_NAME[] = {"string", "int", "bool", "thing", "action",
                                  "list"};

struct Prop {
    int kind;
    char *name;
};

typedef struct {
    Prop *p;
    Val v;
} PEnt;

struct Thing {
    Thing *parent;
    PEnt *props;
    int np, cap;
    char *name;
};

struct TList {
    Val *items;
    int n, cap;
};

/* string -> pointer map (globals, builtins, grammar words and verbs) */
typedef struct MEnt {
    char *k;
    void *v;
    struct MEnt *next;
} MEnt;

typedef struct {
    MEnt **b;
    size_t nb, n;
} Map;

typedef struct {
    int kind;
    char *name;
    char *prep;                 /* NULL: None */
    Val routine;
} Verb;

struct Grammar {
    Map verbs;                  /* word -> Verb* */
    Map words;                  /* a set */
    char *name;
};

typedef struct {
    char *type;
    char *name;
} Param;

typedef struct {
    const char *file;           /* NULL: no position (None) */
    int line;
} Pos;

struct Proc {
    char *name;
    Param *params;
    int nparams;
    char *rtype;
    Param *decls;
    int ndecls;
    Node *body;
    Pos pos;
};

struct Builtin {
    const char *name;
    int nargs;
    Val (*fn)(Val *a);
};

static Val vnil(void)
{
    Val v;
    v.t = V_NIL;
    v.u.i = 0;
    return v;
}

static Val vbool(int b)
{
    Val v;
    v.t = V_BOOL;
    v.u.i = b != 0;
    return v;
}

static Val vint(long long i)
{
    Val v;
    v.t = V_INT;
    v.u.i = i;
    return v;
}

static Val vstr(const char *s)
{
    Val v;
    v.t = V_STR;
    v.u.s = xstrdup(s);
    return v;
}

static Val vstr_own(char *s)
{
    Val v;
    v.t = V_STR;
    v.u.s = s;
    return v;
}

static Val vstatus(int s)
{
    Val v;
    v.t = V_STATUS;
    v.u.i = s;
    return v;
}

static Val vthing(Thing *t)
{
    Val v;
    v.t = V_THING;
    v.u.th = t;
    return v;
}

static Val vlist(TList *l)
{
    Val v;
    v.t = V_LIST;
    v.u.l = l;
    return v;
}

static Val vprop(Prop *p)
{
    Val v;
    v.t = V_PROP;
    v.u.p = p;
    return v;
}

static Val vproc(Proc *p)
{
    Val v;
    v.t = V_PROC;
    v.u.pr = p;
    return v;
}

static const char *type_name(Val v)
{
    switch (v.t) {
    case V_NIL: return "nil";
    case V_BOOL: return "bool";
    case V_INT: return "int";
    case V_STR: return "string";
    case V_STATUS: return "status";
    case V_THING: return "thing";
    case V_LIST: return "thinglist";
    case V_PROP: return "prop";
    case V_GRAMMAR: return "grammar";
    case V_PROC: return "proc";
    case V_BUILTIN: return "builtin";
    }
    return "?";
}

static const char *class_name(Val v)
{
    switch (v.t) {
    case V_LIST: return "ThingList";
    case V_PROP: return "Prop";
    case V_GRAMMAR: return "Grammar";
    case V_PROC: return "Proc";
    case V_BUILTIN: return "Builtin";
    default: return "object";
    }
}

/* str(v), as '%s' formats it */
static char *py_str(Val v)
{
    switch (v.t) {
    case V_NIL: return xstrdup("None");
    case V_BOOL: return xstrdup(v.u.i ? "True" : "False");
    case V_INT: return fmt("%lld", v.u.i);
    case V_STR: return xstrdup(v.u.s);
    case V_STATUS: return xstrdup(STATUS_NAME[v.u.i]);
    case V_THING:
        if (v.u.th->name && *v.u.th->name)
            return fmt("<thing %s>", v.u.th->name);
        return fmt("<thing 0x%llx>", (unsigned long long)(uintptr_t)v.u.th);
    default:
        return fmt("<__main__.%s object at 0x%016llX>", class_name(v),
                   (unsigned long long)(uintptr_t)v.u.th);
    }
}

/* repr(v) */
static char *py_repr(Val v)
{
    if (v.t == V_STR) {
        Buf b = {0};
        const char *s = v.u.s;
        char q = '\'';
        if (strchr(s, '\'') && !strchr(s, '"'))
            q = '"';
        buf_putc(&b, q);
        for (; *s; s++) {
            unsigned char c = (unsigned char)*s;
            if (nul_at(s)) {
                buf_puts(&b, "\\x00");
                s++;
            } else if (c == (unsigned char)q || c == '\\') {
                buf_putc(&b, '\\');
                buf_putc(&b, c);
            } else if (c == '\t') {
                buf_puts(&b, "\\t");
            } else if (c == '\n') {
                buf_puts(&b, "\\n");
            } else if (c == '\r') {
                buf_puts(&b, "\\r");
            } else if (c < 0x20 || c == 0x7f) {
                buf_printf(&b, "\\x%02x", c);
            } else {
                buf_putc(&b, c);
            }
        }
        buf_putc(&b, q);
        return b.s;
    }
    return py_str(v);
}

static int is_int(Val v)                /* isinstance(v, int), bool too */
{
    return v.t == V_INT || v.t == V_BOOL;
}

/* 'is': the same object */
static int same(Val a, Val b)
{
    if (a.t != b.t)
        return 0;
    switch (a.t) {
    case V_NIL: return 1;
    case V_BOOL:
    case V_INT:
    case V_STATUS: return a.u.i == b.u.i;
    case V_STR: return a.u.s == b.u.s || strcmp(a.u.s, b.u.s) == 0;
    default: return a.u.th == b.u.th;
    }
}

static int values_equal(Val a, Val b)
{
    if ((a.t == V_BOOL) != (b.t == V_BOOL))
        return 0;
    if (a.t != b.t)
        return 0;
    switch (a.t) {
    case V_NIL: return 1;
    case V_BOOL:
    case V_INT:
    case V_STATUS: return a.u.i == b.u.i;
    case V_STR: return strcmp(a.u.s, b.u.s) == 0;
    default: return a.u.th == b.u.th;
    }
}

/* ---- maps ---- */

static size_t str_hash(const char *s)
{
    size_t h = 2166136261u;
    for (; *s; s++)
        h = (h ^ (unsigned char)*s) * 16777619u;
    return h;
}

static MEnt *map_find(Map *m, const char *k)
{
    MEnt *e;
    if (!m->nb)
        return NULL;
    for (e = m->b[str_hash(k) & (m->nb - 1)]; e; e = e->next)
        if (strcmp(e->k, k) == 0)
            return e;
    return NULL;
}

static void *map_get(Map *m, const char *k)
{
    MEnt *e = map_find(m, k);
    return e ? e->v : NULL;
}

static int map_has(Map *m, const char *k)
{
    return map_find(m, k) != NULL;
}

static void map_set(Map *m, const char *k, void *v)
{
    MEnt *e = map_find(m, k);
    size_t h;
    if (e) {
        e->v = v;
        return;
    }
    if (m->n + 1 > m->nb * 3 / 4) {
        size_t nb = m->nb ? m->nb * 2 : 16, i;
        MEnt **nbk = xmalloc(nb * sizeof(MEnt *));
        memset(nbk, 0, nb * sizeof(MEnt *));
        for (i = 0; i < m->nb; i++) {
            MEnt *x = m->b[i], *nx;
            for (; x; x = nx) {
                size_t j = str_hash(x->k) & (nb - 1);
                nx = x->next;
                x->next = nbk[j];
                nbk[j] = x;
            }
        }
        free(m->b);
        m->b = nbk;
        m->nb = nb;
    }
    e = xmalloc(sizeof(MEnt));
    e->k = xstrdup(k);
    e->v = v;
    h = str_hash(k) & (m->nb - 1);
    e->next = m->b[h];
    m->b[h] = e;
    m->n++;
}

/* ====================================================================
 *  Errors.  A MudError jumps to the innermost handler (safe_call, an
 *  item of load_file, main).  Its position and trace are what
 *  amigamud.py's except clauses collect on the way out: the position of
 *  the innermost call or assignment that has one, the innermost six
 *  procedures.
 * ==================================================================== */

/* MinGW-w64's longjmp unwinds the stack through SEH, which can crash in
   optimised code; GCC's builtin pair only restores the registers. */
typedef void *JmpBuf[5];
#define SETJMP(b) __builtin_setjmp(b)
#define LONGJMP(b) __builtin_longjmp(b, 1)

typedef struct Handler {
    JmpBuf jb;
    struct Handler *prev;
    int psp, csp;
    int depth;
} Handler;

static Handler *H;
static int depth;

static Pos *posstack;           /* the calls and assignments being run */
static int psp, pcap;
static Proc **callstack;        /* the procedures being run */
static int csp, ccap;

static char *E_msg;
static Pos E_pos;
static char *E_trace[6];
static int E_ntrace;
static int E_parse;             /* a ParseError */

static JmpBuf abort_jmp;        /* LoadAbort */

/* never from a function that sets up a buffer itself (GCC's rule) */
static void load_abort(void) __attribute__((noinline, noreturn));
static void load_abort(void)
{
    LONGJMP(abort_jmp);
}

static void push_handler(Handler *h)
{
    h->prev = H;
    h->psp = psp;
    h->csp = csp;
    h->depth = depth;
    H = h;
}

static void pop_handler(Handler *h)
{
    H = h->prev;
}

static void caught(Handler *h)
{
    H = h->prev;
    psp = h->psp;
    csp = h->csp;
    depth = h->depth;
}

static void push_pos(Pos p)
{
    if (psp == pcap) {
        pcap = pcap ? pcap * 2 : 256;
        posstack = xrealloc(posstack, sizeof(Pos) * (size_t)pcap);
    }
    posstack[psp++] = p;
}

static void throw_error(Pos pos, int parse, char *msg)
{
    int i;
    E_msg = msg;
    E_parse = parse;
    E_pos = pos;
    if (!E_pos.file) {
        for (i = psp - 1; i >= 0; i--) {
            if (posstack[i].file) {
                E_pos = posstack[i];
                break;
            }
        }
    }
    E_ntrace = 0;
    for (i = csp - 1; i >= 0 && E_ntrace < 6; i--) {
        Proc *p = callstack[i];
        E_trace[E_ntrace++] = fmt("in %s (%s:%d)", p->name, p->pos.file,
                                  p->pos.line);
    }
    LONGJMP(H->jb);
}

static const Pos NOPOS = {NULL, 0};

static void mud_error(Pos pos, const char *f, ...)
{
    Buf b = {0};
    va_list ap;
    va_start(ap, f);
    buf_vprintf(&b, f, ap);
    va_end(ap);
    throw_error(pos, 0, b.s ? b.s : xstrdup(""));
}

static char *error_text(void)
{
    Buf b = {0};
    int i;
    if (E_pos.file)
        buf_printf(&b, "%s:%d: ", E_pos.file, E_pos.line);
    buf_puts(&b, E_msg);
    for (i = 0; i < E_ntrace; i++)
        buf_printf(&b, "\n    %s", E_trace[i]);
    return b.s;
}

/* ====================================================================
 *  Output: collected, then written at the next flush wrapped by
 *  textwrap.fill (break_long_words=False, break_on_hyphens=False)
 * ==================================================================== */

static Buf OUTB;
static long width = 78;

static void check_stdout(void)
{
    if (ferror(stdout))
        _Exit(0);
}

static char *expandtabs(const char *s)
{
    Buf b = {0};
    int col = 0;
    buf_puts(&b, "");
    for (; *s; s++) {
        if (*s == '\t') {
            int k = 8 - col % 8;
            while (k--) {
                buf_putc(&b, ' ');
                col++;
            }
        } else if (nul_at(s)) {
            buf_putn(&b, s, 2);
            col++;
            s++;
        } else {
            buf_putc(&b, *s);
            col++;
            if (*s == '\n' || *s == '\r')
                col = 0;
        }
    }
    return b.s;
}

typedef struct {
    const char *s;
    size_t len;                 /* bytes */
    long vl;                    /* characters */
    int ws;
} Chunk;

/* textwrap.fill(text, w, initial_indent=ind, subsequent_indent=ind,
   break_long_words=False, break_on_hyphens=False) */
static void textwrap_fill(Buf *out, const char *text, long w, const char *ind)
{
    char *t = xstrdup(text);
    char *p;
    Chunk *ch = NULL;
    int nch = 0, ci = 0, nlines = 0;
    size_t i, len;
    long lw;
    size_t indlen = strlen(ind);
    for (p = t; *p; p++)                /* replace_whitespace */
        if (*p == '\t' || *p == '\n' || *p == '\v' || *p == '\f' || *p == '\r')
            *p = ' ';
    len = strlen(t);
    for (i = 0; i < len;) {             /* split into words and blank runs */
        size_t j = i;
        int ws = t[i] == ' ';
        while (j < len && (t[j] == ' ') == ws)
            j++;
        ch = xrealloc(ch, sizeof(Chunk) * (size_t)(nch + 1));
        ch[nch].s = t + i;
        ch[nch].len = j - i;
        ch[nch].vl = (long)vlen_n(t + i, j - i);
        ch[nch].ws = ws;
        nch++;
        i = j;
    }
    lw = w - (long)indlen;
    while (ci < nch) {
        int first = ci, last;
        long cur = 0;
        if (ch[ci].ws && nlines)        /* drop a line's leading blanks */
            ci++;
        first = ci;
        while (ci < nch && cur + ch[ci].vl <= lw) {
            cur += ch[ci].vl;
            ci++;
        }
        if (ci < nch && ch[ci].vl > lw) {
            if (ci == first)            /* a long word goes on a line alone */
                ci++;
        }
        last = ci;
        if (last > first && ch[last - 1].ws)    /* drop trailing blanks */
            last--;
        if (last > first) {
            int k;
            if (nlines)
                buf_putc(out, '\n');
            buf_puts(out, ind);
            for (k = first; k < last; k++)
                buf_putn(out, ch[k].s, ch[k].len);
            nlines++;
        }
    }
}

static void flush_out(void)
{
    Buf text = OUTB;
    memset(&OUTB, 0, sizeof OUTB);
    if (!text.n)
        return;
    if (width > 0) {
        Buf res = {0};
        int nl, k;
        char **lines = split_ch(text.s, '\n', &nl);
        buf_puts(&res, "");
        for (k = 0; k < nl; k++) {
            char *line = expandtabs(lines[k]);
            if (k)
                buf_putc(&res, '\n');
            if ((long)vlen(line) <= width) {
                buf_puts(&res, line);
            } else {
                size_t ni = 0;
                char *ind, *body;
                while (line[ni] == ' ')
                    ni++;
                ind = xstrndup(line, ni);
                body = strip_chars(line, " ", 1, 1);
                textwrap_fill(&res, body, width, ind);
            }
        }
        text = res;
    }
    put_text(stdout, text.s, text.n);
    fflush(stdout);
    check_stdout();
}

static void out_write(const char *s)
{
    buf_puts(&OUTB, s);
}

static void warn(const char *msg, Pos pos)
{
    flush_out();
    if (pos.file)
        put_str(stderr, fmt("warning: %s:%d: %s\n", pos.file, pos.line, msg));
    else
        put_str(stderr, fmt("warning: %s\n", msg));
}

static int nerrors;

static void report(void)
{
    flush_out();
    nerrors++;
    put_str(stderr, fmt("error: %s\n", error_text()));
}

/* ====================================================================
 *  Things and properties
 * ==================================================================== */

static Thing *thing_new(Thing *parent)
{
    Thing *t = xmalloc(sizeof(Thing));
    memset(t, 0, sizeof(Thing));
    t->parent = parent;
    return t;
}

static TList *tlist_new(void)
{
    TList *l = xmalloc(sizeof(TList));
    memset(l, 0, sizeof(TList));
    return l;
}

static void tlist_insert(TList *l, int at, Val v)
{
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->items = xrealloc(l->items, sizeof(Val) * (size_t)l->cap);
    }
    memmove(l->items + at + 1, l->items + at, sizeof(Val) * (size_t)(l->n - at));
    l->items[at] = v;
    l->n++;
}

static PEnt *own_prop(Thing *t, Prop *p)
{
    int i;
    for (i = 0; i < t->np; i++)
        if (t->props[i].p == p)
            return &t->props[i];
    return NULL;
}

static void put_prop(Thing *t, Prop *p, Val v)
{
    PEnt *e = own_prop(t, p);
    if (e) {
        e->v = v;
        return;
    }
    if (t->np == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 8;
        t->props = xrealloc(t->props, sizeof(PEnt) * (size_t)t->cap);
    }
    t->props[t->np].p = p;
    t->props[t->np].v = v;
    t->np++;
}

static Val thing_get(Thing *self, Prop *p)
{
    Thing *t;
    for (t = self; t; t = t->parent) {
        PEnt *e = own_prop(t, p);
        if (e)
            return e->v;
    }
    switch (p->kind) {
    case K_STRING: return vstr("");
    case K_INT: return vint(0);
    case K_BOOL: return vbool(0);
    case K_LIST: {                      /* auto-create empty lists */
        Val l = vlist(tlist_new());
        put_prop(self, p, l);
        return l;
    }
    }
    return vnil();
}

static void thing_set(Thing *t, Prop *p, Val v)
{
    int k = p->kind, ok;
    ok = (k == K_STRING && v.t == V_STR) ||
         (k == K_INT && v.t == V_INT) ||
         (k == K_BOOL && v.t == V_BOOL) ||
         (k == K_THING && (v.t == V_NIL || v.t == V_THING)) ||
         (k == K_ACTION && (v.t == V_NIL || v.t == V_PROC || v.t == V_BUILTIN)) ||
         (k == K_LIST && v.t == V_LIST);
    if (!ok)
        mud_error(NOPOS, "cannot store %s in %s property %s", type_name(v),
                  KIND_NAME[k], p->name ? p->name : "?");
    put_prop(t, p, v);
}

static void thing_delete(Thing *t, Prop *p)
{
    int i;
    for (i = 0; i < t->np; i++) {
        if (t->props[i].p == p) {
            memmove(t->props + i, t->props + i + 1,
                    sizeof(PEnt) * (size_t)(t->np - i - 1));
            t->np--;
            return;
        }
    }
}

/* ====================================================================
 *  AST
 * ==================================================================== */

enum { X_CONST, X_VAR, X_LOGIC, X_NOT, X_NEG, X_BIN, X_PROPGET, X_INDEX,
       X_CALL, X_CALLOF, X_ANON, X_IF, X_WHILE, X_ASSIGN, X_DELPROP,
       X_IGNORE, X_SEQ };

struct Node {
    int k;
    const char *op;             /* LOGIC, BIN */
    Val v;                      /* CONST */
    char *name;                 /* VAR */
    Proc *proc;                 /* ANON */
    Node *a, *b;                /* operands, obj/prop, target/value ... */
    Node **xs;                  /* CALL args, SEQ stmts, IF conds and blocks */
    int nx;
    int value_last;             /* SEQ */
    Pos pos;
};

static Node *mk(int k, Pos pos)
{
    Node *n = xmalloc(sizeof(Node));
    memset(n, 0, sizeof(Node));
    n->k = k;
    n->v = vnil();
    n->pos = pos;
    return n;
}

static void push_x(Node *n, Node *x)
{
    n->xs = xrealloc(n->xs, sizeof(Node *) * (size_t)(n->nx + 1));
    n->xs[n->nx++] = x;
}

/* ====================================================================
 *  The interpreter's state
 * ==================================================================== */

typedef struct {
    char *name;
    Val v;
} VarEnt;

typedef struct {
    VarEnt *vars;
    int n;
} Frame;

static Map globals;             /* name -> Val* */
static Map builtins;            /* name -> Val* (a builtin) */
static Proc **all_procs;
static int nall_procs;
static int opt_strict, opt_verbose;
static int opt_echo = -1;       /* -1: None */
static Map assigns;             /* name -> char* dir */
static char *default_root;
static int continue_on_error;
static int single_user;
static int quit_flag;
static char *prompt;
static Val input_action, new_char_action;
static Thing *player;
static Val player_loc;
static Val find_result;

static Val *global_slot(const char *n)
{
    return map_get(&globals, n);
}

static void set_global(const char *n, Val v)
{
    Val *slot = global_slot(n);
    if (!slot) {
        slot = xmalloc(sizeof(Val));
        map_set(&globals, n, slot);
    }
    *slot = v;
}

static VarEnt *frame_var(Frame *fr, const char *n)
{
    int i;
    for (i = 0; i < fr->n; i++)
        if (strcmp(fr->vars[i].name, n) == 0)
            return &fr->vars[i];
    return NULL;
}

static Val ev(Node *n, Frame *fr);
static Val call(Val fn, Val *args, int nargs, Pos pos);

static int need_bool(Val v, const char *what, Pos pos)
{
    if (v.t != V_BOOL)
        mud_error(pos, "%s must be a bool, not %s", what, type_name(v));
    return (int)v.u.i;
}

static Val default_for(const char *ty)
{
    if (strcmp(ty, "string") == 0)
        return vstr("");
    if (strcmp(ty, "int") == 0)
        return vint(0);
    if (strcmp(ty, "bool") == 0)
        return vbool(0);
    return vnil();
}

static Val call_proc(Proc *p, Val *args, int nargs)
{
    Frame fr;
    int i;
    Val r;
    if (nargs != p->nparams)
        mud_error(NOPOS, "%s takes %d argument(s), got %d", p->name,
                  p->nparams, nargs);
    fr.vars = xmalloc(sizeof(VarEnt) * (size_t)(p->nparams + p->ndecls + 1));
    fr.n = 0;
    for (i = 0; i < p->nparams + p->ndecls; i++) {
        Param *pa = i < p->nparams ? &p->params[i] : &p->decls[i - p->nparams];
        Val v = i < p->nparams ? args[i] : default_for(pa->type);
        VarEnt *e = frame_var(&fr, pa->name);
        if (e) {
            e->v = v;
        } else {
            fr.vars[fr.n].name = pa->name;
            fr.vars[fr.n].v = v;
            fr.n++;
        }
    }
    depth++;
    if (csp == ccap) {
        ccap = ccap ? ccap * 2 : 64;
        callstack = xrealloc(callstack, sizeof(Proc *) * (size_t)ccap);
    }
    callstack[csp++] = p;
    if (depth > 300)
        mud_error(NOPOS, "procedure calls nested too deeply");
    r = ev(p->body, &fr);
    csp--;
    depth--;
    free(fr.vars);
    return r;
}

static Val call(Val fn, Val *args, int nargs, Pos pos)
{
    Val r;
    push_pos(pos);
    if (fn.t == V_PROC) {
        r = call_proc(fn.u.pr, args, nargs);
        psp--;
        return r;
    }
    if (fn.t == V_BUILTIN) {
        Builtin *b = fn.u.b;
        if (nargs != b->nargs)
            mud_error(NOPOS, "%s takes %d argument(s), got %d", b->name,
                      b->nargs, nargs);
        r = b->fn(args);
        psp--;
        return r;
    }
    psp--;
    mud_error(pos, "cannot call %s", type_name(fn));
    return vnil();
}

static Val lookup_var(Node *n, Frame *fr)
{
    VarEnt *e = frame_var(fr, n->name);
    Val *g;
    if (e)
        return e->v;
    if ((g = global_slot(n->name)) != NULL)
        return *g;
    if ((g = map_get(&builtins, n->name)) != NULL)
        return *g;
    mud_error(n->pos, "undefined symbol '%s'", n->name);
    return vnil();
}

static void propget_parts(Node *n, Frame *fr, Thing **o, Prop **p)
{
    Val ov = ev(n->a, fr), pv = ev(n->b, fr);
    if (ov.t == V_NIL)
        mud_error(n->pos, "property access on nil thing");
    if (ov.t != V_THING)
        mud_error(n->pos, "'@' needs a thing, not %s", type_name(ov));
    if (pv.t != V_PROP)
        mud_error(n->pos, "'@' needs a property, not %s", type_name(pv));
    *o = ov.u.th;
    *p = pv.u.p;
}

static int str_less(const char *a, const char *b)
{
    return strcmp(a, b) < 0;
}

static Val ev(Node *n, Frame *fr)
{
    switch (n->k) {
    case X_CONST:
        return n->v;
    case X_VAR:
        return lookup_var(n, fr);
    case X_LOGIC: {
        char what[32];
        int a;
        snprintf(what, sizeof what, "operand of '%s'", n->op);
        a = need_bool(ev(n->a, fr), what, n->pos);
        if (n->op[0] == 'a')
            return a ? vbool(need_bool(ev(n->b, fr), "operand of 'and'", n->pos))
                     : vbool(0);
        return a ? vbool(1) : vbool(need_bool(ev(n->b, fr), "operand of 'or'",
                                              n->pos));
    }
    case X_NOT:
        return vbool(!need_bool(ev(n->a, fr), "operand of 'not'", n->pos));
    case X_NEG: {
        Val v = ev(n->a, fr);
        if (v.t != V_INT)
            mud_error(n->pos, "cannot negate %s", type_name(v));
        return vint(-v.u.i);
    }
    case X_BIN: {
        const char *op = n->op;
        Val a = ev(n->a, fr), b = ev(n->b, fr);
        int num = a.t == V_INT && b.t == V_INT;
        int strs = a.t == V_STR && b.t == V_STR;
        if (strcmp(op, "=") == 0)
            return vbool(values_equal(a, b));
        if (strcmp(op, "~=") == 0)
            return vbool(!values_equal(a, b));
        if (strcmp(op, "==") == 0 || strcmp(op, "~==") == 0) {
            int r;
            if (strs)
                r = strcmp(lower(a.u.s), lower(b.u.s)) == 0;
            else
                r = values_equal(a, b);
            return vbool(op[0] == '=' ? r : !r);
        }
        if (strcmp(op, "+") == 0) {
            if (num)
                return vint(a.u.i + b.u.i);
            if (strs)
                return vstr_own(fmt("%s%s", a.u.s, b.u.s));
        } else if (num && (strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
                           strcmp(op, "/") == 0 || strcmp(op, "%") == 0)) {
            if (op[0] == '-')
                return vint(a.u.i - b.u.i);
            if (op[0] == '*')
                return vint(a.u.i * b.u.i);
            if (b.u.i == 0)
                mud_error(n->pos, "division by zero");
            if (op[0] == '/')
                return vint(a.u.i / b.u.i);
            return vint(a.u.i - b.u.i * (a.u.i / b.u.i));
        } else if ((num || strs) && (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                                     strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0)) {
            int lt, gt;
            if (num) {
                lt = a.u.i < b.u.i;
                gt = a.u.i > b.u.i;
            } else {
                lt = str_less(a.u.s, b.u.s);
                gt = str_less(b.u.s, a.u.s);
            }
            if (strcmp(op, "<") == 0)
                return vbool(lt);
            if (strcmp(op, ">") == 0)
                return vbool(gt);
            if (strcmp(op, "<=") == 0)
                return vbool(!gt);
            return vbool(!lt);
        }
        mud_error(n->pos, "cannot apply '%s' to %s and %s", op, type_name(a),
                  type_name(b));
        return vnil();
    }
    case X_PROPGET: {
        Thing *o;
        Prop *p;
        propget_parts(n, fr, &o, &p);
        return thing_get(o, p);
    }
    case X_INDEX: {
        Val l = ev(n->a, fr), i = ev(n->b, fr);
        if (l.t != V_LIST)
            mud_error(n->pos, "cannot index %s", type_name(l));
        if (!is_int(i) || i.u.i < 0 || i.u.i >= l.u.l->n)
            mud_error(n->pos, "list index %s out of range", py_repr(i));
        return l.u.l->items[i.u.i];
    }
    case X_CALL: {
        Val f = ev(n->a, fr);
        Val *args = xmalloc(sizeof(Val) * (size_t)(n->nx + 1));
        Val r;
        int i;
        for (i = 0; i < n->nx; i++)
            args[i] = ev(n->xs[i], fr);
        r = call(f, args, n->nx, n->pos);
        free(args);
        return r;
    }
    case X_CALLOF: {
        Val v = ev(n->a, fr);
        if (v.t == V_NIL)
            mud_error(n->pos, "call of a nil action");
        return v;
    }
    case X_ANON:
        return vproc(n->proc);
    case X_IF: {
        int i;
        for (i = 0; i + 1 < n->nx; i += 2)
            if (need_bool(ev(n->xs[i], fr), "'if' condition", n->pos))
                return ev(n->xs[i + 1], fr);
        return n->a ? ev(n->a, fr) : vnil();
    }
    case X_WHILE:
        while (need_bool(ev(n->a, fr), "'while' condition", n->pos))
            ev(n->b, fr);
        return vnil();
    case X_ASSIGN: {
        Val val = ev(n->b, fr);
        Node *t = n->a;
        push_pos(n->pos);
        if (t->k == X_VAR) {
            VarEnt *e = frame_var(fr, t->name);
            Val *g;
            if (e)
                e->v = val;
            else if ((g = global_slot(t->name)) != NULL)
                *g = val;
            else
                mud_error(NOPOS, "assignment to undeclared variable '%s'", t->name);
        } else if (t->k == X_PROPGET) {
            Thing *o;
            Prop *p;
            propget_parts(t, fr, &o, &p);
            thing_set(o, p, val);
        } else {
            Val l = ev(t->a, fr), i = ev(t->b, fr);
            if (l.t != V_LIST || !is_int(i) || i.u.i < 0 || i.u.i >= l.u.l->n)
                mud_error(NOPOS, "bad list assignment");
            l.u.l->items[i.u.i] = val;
        }
        psp--;
        return vnil();
    }
    case X_DELPROP: {
        Val o = ev(n->a, fr), p = ev(n->b, fr);
        if (o.t != V_THING || p.t != V_PROP)
            mud_error(n->pos, "'--' needs a thing and a property");
        thing_delete(o.u.th, p.u.p);
        return vnil();
    }
    case X_IGNORE:
        ev(n->a, fr);
        return vnil();
    case X_SEQ: {
        Val v = vnil();
        int i;
        for (i = 0; i < n->nx; i++)
            v = ev(n->xs[i], fr);
        return n->value_last ? v : vnil();
    }
    }
    return vnil();
}

/* ====================================================================
 *  Lexer
 * ==================================================================== */

static const char *KEYWORDS[] = {
    "proc", "corp", "if", "then", "elif", "else", "fi", "while", "do", "od",
    "and", "or", "not", "ignore", "private", "public", "call",
    "nil", "true", "false", "succeed", "fail", "continue",
    "thing", "string", "int", "bool", "status", "action", "list",
    "property", "grammar", "void", NULL
};
static const char *TYPE_WORDS[] = {"thing", "string", "int", "bool", "status",
                                   "action", "list", "property", "grammar",
                                   "void", NULL};

static int in_set(const char *w, const char **set)
{
    for (; *set; set++)
        if (strcmp(w, *set) == 0)
            return 1;
    return 0;
}

enum { T_STR, T_INT, T_ID, T_OP, T_EOF };

typedef struct {
    int k;
    char *v;
    long long iv;
    int line;
    size_t s, e;
} Tok;

static int is_alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int is_digit(int c)
{
    return c >= '0' && c <= '9';
}

static void parse_error(const char *file, int line, const char *f, ...)
{
    Buf b = {0};
    va_list ap;
    Pos pos;
    va_start(ap, f);
    buf_vprintf(&b, f, ap);
    va_end(ap);
    pos.file = file;
    pos.line = line;
    throw_error(pos, 1, b.s);
}

static Tok *lex(const char *text, const char *fname, int *count)
{
    static const char *ops2[] = {":=", "~=", "==", "<=", ">=", "--", NULL};
    static const char ops1[] = "+-*/%()[],;:@$.<>=~";
    Tok *toks = NULL, *merged;
    int nt = 0, nm = 0, k;
    size_t i = 0, n = strlen(text);
    int line = 1;
#define ADD(K, V, IV, S, E) do { \
        toks = xrealloc(toks, sizeof(Tok) * (size_t)(nt + 1)); \
        toks[nt].k = (K); toks[nt].v = (V); toks[nt].iv = (IV); \
        toks[nt].line = line; toks[nt].s = (S); toks[nt].e = (E); nt++; \
    } while (0)
    while (i < n) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            line++;
            i++;
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\f') {
            i++;
        } else if (c == '/' && text[i + 1] == '*') {
            const char *j = strstr(text + i + 2, "*/");
            const char *q;
            if (!j)
                parse_error(fname, line, "unterminated comment");
            for (q = text + i; q < j + 2; q++)
                if (*q == '\n')
                    line++;
            i = (size_t)(j - text) + 2;
        } else if (c == '"') {
            size_t j = i + 1;
            Buf b = {0};
            buf_puts(&b, "");
            for (;;) {
                char ch;
                if (j >= n || text[j] == '\n')
                    parse_error(fname, line, "unterminated string");
                ch = text[j];
                if (ch == '"')
                    break;
                if (ch == '\\') {
                    char e;
                    j++;
                    if (j >= n)
                        parse_error(fname, line, "unterminated string");
                    e = text[j];
                    switch (e) {
                    case 'n': buf_putc(&b, '\n'); break;
                    case 't': buf_putc(&b, '\t'); break;
                    case 'r': buf_putc(&b, '\r'); break;
                    case 'e': buf_putc(&b, 0x1b); break;
                    case '0': buf_putc(&b, 0xC0); buf_putc(&b, 0x80); break;
                    default: buf_putc(&b, e); break;     /* " \ ' and the rest */
                    }
                } else {
                    buf_putc(&b, ch);
                }
                j++;
            }
            ADD(T_STR, b.s, 0, i, j + 1);
            i = j + 1;
        } else if (is_digit(c)) {
            size_t j = i;
            long long v = 0;
            while (j < n && is_digit((unsigned char)text[j])) {
                v = v * 10 + (text[j] - '0');
                j++;
            }
            ADD(T_INT, fmt("%lld", v), v, i, j);
            i = j;
        } else if (is_alpha(c) || c == '_') {
            size_t j = i;
            while (j < n && (is_alpha((unsigned char)text[j]) ||
                             is_digit((unsigned char)text[j]) || text[j] == '_'))
                j++;
            ADD(T_ID, xstrndup(text + i, j - i), 0, i, j);
            i = j;
        } else {
            if (strncmp(text + i, "~==", 3) == 0) {
                ADD(T_OP, xstrdup("~=="), 0, i, i + 3);
                i += 3;
                continue;
            }
            for (k = 0; ops2[k]; k++)
                if (strncmp(text + i, ops2[k], 2) == 0)
                    break;
            if (ops2[k]) {
                ADD(T_OP, xstrdup(ops2[k]), 0, i, i + 2);
                i += 2;
            } else if (c && strchr(ops1, c)) {
                ADD(T_OP, xstrndup(text + i, 1), 0, i, i + 1);
                i++;
            } else {
                Val cv;
                cv.t = V_STR;
                cv.u.s = xstrndup(text + i, 1);
                parse_error(fname, line, "unexpected character %s", py_repr(cv));
            }
        }
    }
    ADD(T_EOF, xstrdup(""), 0, n, n);
#undef ADD
    /* C-style concatenation of adjacent string literals */
    merged = xmalloc(sizeof(Tok) * (size_t)nt);
    for (k = 0; k < nt; k++) {
        if (toks[k].k == T_STR && nm && merged[nm - 1].k == T_STR) {
            merged[nm - 1].v = fmt("%s%s", merged[nm - 1].v, toks[k].v);
            merged[nm - 1].e = toks[k].e;
        } else {
            merged[nm++] = toks[k];
        }
    }
    *count = nm;
    return merged;
}

/* ====================================================================
 *  Parser
 * ==================================================================== */

typedef struct {
    Tok *toks;
    int ntoks;
    int i;
    const char *text;
    const char *fname;
    int start;
    int item_is_proc;
} Parser;

static Parser *PS;

static Tok *tok_at(int k)
{
    int j = PS->i + k;
    if (j > PS->ntoks - 1)
        j = PS->ntoks - 1;
    return &PS->toks[j];
}

static Tok *tok(void)
{
    return tok_at(0);
}

static Pos pos_of(Tok *t)
{
    Pos p;
    p.file = PS->fname;
    p.line = (t ? t : tok())->line;
    return p;
}

static char *desc(Tok *t)
{
    if (t->k == T_EOF)
        return xstrdup("end of file");
    if (t->k == T_STR)
        return xstrdup("string");
    return fmt("'%s'", t->v);
}

static void perr(const char *f, ...)
{
    Buf b = {0};
    va_list ap;
    va_start(ap, f);
    buf_vprintf(&b, f, ap);
    va_end(ap);
    throw_error(pos_of(NULL), 1, b.s);
}

static int isop(const char *v, int k)
{
    Tok *t = tok_at(k);
    return t->k == T_OP && strcmp(t->v, v) == 0;
}

static int iskw(const char *v, int k)
{
    Tok *t = tok_at(k);
    return t->k == T_ID && strcmp(t->v, v) == 0;
}

static int accept_op(const char *v)
{
    if (isop(v, 0)) {
        PS->i++;
        return 1;
    }
    return 0;
}

static int accept_kw(const char *v)
{
    if (iskw(v, 0)) {
        PS->i++;
        return 1;
    }
    return 0;
}

static void expect_op(const char *v)
{
    if (!accept_op(v))
        perr("expected '%s' but found %s", v, desc(tok()));
}

static void expect_kw(const char *v)
{
    if (!accept_kw(v))
        perr("expected '%s' but found %s", v, desc(tok()));
}

static char *ident(void)
{
    Tok *t = tok();
    if (t->k != T_ID || in_set(t->v, KEYWORDS))
        perr("expected a name but found %s", desc(t));
    PS->i++;
    return t->v;
}

static int at_type(void)
{
    Tok *t = tok();
    return t->k == T_ID && in_set(t->v, TYPE_WORDS);
}

static char *parse_type(void)
{
    Buf b = {0};
    Tok *t;
    buf_puts(&b, "");
    while (iskw("list", 0) || iskw("property", 0)) {
        buf_printf(&b, "%s%s", b.n ? " " : "", tok()->v);
        PS->i++;
    }
    t = tok();
    if (t->k == T_ID && in_set(t->v, TYPE_WORDS) && strcmp(t->v, "list") != 0 &&
        strcmp(t->v, "property") != 0) {
        buf_printf(&b, "%s%s", b.n ? " " : "", t->v);
        PS->i++;
    } else {
        perr("expected a type name but found %s", desc(t));
    }
    return b.s;
}

static Node *parse_expr(void);
static Node *parse_seq(const char **terms);
static Node *parse_postfix(void);

static Node *node2(int k, const char *op, Node *a, Node *b, Pos p)
{
    Node *n = mk(k, p);
    n->op = op;
    n->a = a;
    n->b = b;
    return n;
}

static Node *parse_and(void);
static Node *parse_not(void);
static Node *parse_cmp(void);
static Node *parse_add(void);
static Node *parse_mul(void);
static Node *parse_unary(void);
static Node *parse_primary(void);

static Node *parse_expr(void)
{
    Node *e = parse_and();
    while (iskw("or", 0)) {
        Pos p = pos_of(NULL);
        PS->i++;
        e = node2(X_LOGIC, "or", e, parse_and(), p);
    }
    return e;
}

static Node *parse_and(void)
{
    Node *e = parse_not();
    while (iskw("and", 0)) {
        Pos p = pos_of(NULL);
        PS->i++;
        e = node2(X_LOGIC, "and", e, parse_not(), p);
    }
    return e;
}

static Node *parse_not(void)
{
    if (iskw("not", 0)) {
        Pos p = pos_of(NULL);
        Node *n;
        PS->i++;
        n = mk(X_NOT, p);
        n->a = parse_not();
        return n;
    }
    return parse_cmp();
}

static Node *parse_cmp(void)
{
    static const char *ops[] = {"=", "~=", "==", "~==", "<", ">", "<=", ">=", NULL};
    Node *e = parse_add();
    Tok *t = tok();
    if (t->k == T_OP && in_set(t->v, ops)) {
        Pos p = pos_of(NULL);
        PS->i++;
        e = node2(X_BIN, t->v, e, parse_add(), p);
    }
    return e;
}

static Node *parse_add(void)
{
    Node *e = parse_mul();
    while (isop("+", 0) || isop("-", 0)) {
        Tok *t = tok();
        PS->i++;
        e = node2(X_BIN, t->v, e, parse_mul(), pos_of(t));
    }
    return e;
}

static Node *parse_mul(void)
{
    Node *e = parse_unary();
    while (isop("*", 0) || isop("/", 0) || isop("%", 0)) {
        Tok *t = tok();
        PS->i++;
        e = node2(X_BIN, t->v, e, parse_unary(), pos_of(t));
    }
    return e;
}

static Node *parse_unary(void)
{
    if (isop("-", 0)) {
        Pos p = pos_of(NULL);
        Node *n;
        PS->i++;
        n = mk(X_NEG, p);
        n->a = parse_unary();
        return n;
    }
    return parse_postfix();
}

static Node *parse_postfix(void)
{
    Node *e = parse_primary();
    for (;;) {
        Tok *t = tok();
        if (t->k != T_OP)
            break;
        if (strcmp(t->v, "@") == 0) {
            PS->i++;
            e = node2(X_PROPGET, NULL, e, parse_primary(), pos_of(t));
        } else if (strcmp(t->v, "(") == 0) {
            Node *c = mk(X_CALL, pos_of(t));
            PS->i++;
            c->a = e;
            if (!isop(")", 0)) {
                push_x(c, parse_expr());
                while (accept_op(","))
                    push_x(c, parse_expr());
            }
            expect_op(")");
            e = c;
        } else if (strcmp(t->v, "[") == 0) {
            Node *idx;
            PS->i++;
            idx = parse_expr();
            expect_op("]");
            e = node2(X_INDEX, NULL, e, idx, pos_of(t));
        } else {
            break;
        }
    }
    return e;
}

static Node *parse_if(void);
static Node *parse_while(void);
static Node *parse_anon_proc(void);

static Node *parse_primary(void)
{
    Tok *t = tok();
    if (t->k == T_INT || t->k == T_STR) {
        Node *n = mk(X_CONST, NOPOS);
        PS->i++;
        n->v = t->k == T_INT ? vint(t->iv) : vstr(t->v);
        return n;
    }
    if (t->k == T_OP && strcmp(t->v, "(") == 0) {
        Node *e;
        PS->i++;
        e = parse_expr();
        expect_op(")");
        return e;
    }
    if (t->k == T_ID) {
        const char *v = t->v;
        Node *n;
        int c = -1;
        if (strcmp(v, "nil") == 0) c = 0;
        else if (strcmp(v, "true") == 0) c = 1;
        else if (strcmp(v, "false") == 0) c = 2;
        else if (strcmp(v, "succeed") == 0) c = 3;
        else if (strcmp(v, "fail") == 0) c = 4;
        else if (strcmp(v, "continue") == 0) c = 5;
        if (c >= 0) {
            n = mk(X_CONST, NOPOS);
            PS->i++;
            n->v = c == 0 ? vnil() : c == 1 ? vbool(1) : c == 2 ? vbool(0) :
                   vstatus(c - 3);
            return n;
        }
        if (strcmp(v, "if") == 0)
            return parse_if();
        if (strcmp(v, "while") == 0)
            return parse_while();
        if (strcmp(v, "proc") == 0)
            return parse_anon_proc();
        if (strcmp(v, "call") == 0) {
            Node *e;
            PS->i++;
            expect_op("(");
            e = parse_expr();
            expect_op(",");
            parse_type();
            expect_op(")");
            n = mk(X_CALLOF, pos_of(t));
            n->a = e;
            return n;
        }
        if (in_set(v, KEYWORDS))
            perr("unexpected '%s'", v);
        PS->i++;
        n = mk(X_VAR, pos_of(t));
        n->name = t->v;
        return n;
    }
    perr("unexpected %s in expression", desc(t));
    return NULL;
}

static Node *parse_if(void)
{
    static const char *arm[] = {"elif", "else", "fi", NULL};
    static const char *fi[] = {"fi", NULL};
    Pos p = pos_of(NULL);
    Node *n = mk(X_IF, p);
    PS->i++;
    push_x(n, parse_expr());
    expect_kw("then");
    push_x(n, parse_seq(arm));
    while (accept_kw("elif")) {
        push_x(n, parse_expr());
        expect_kw("then");
        push_x(n, parse_seq(arm));
    }
    if (accept_kw("else"))
        n->a = parse_seq(fi);
    expect_kw("fi");
    return n;
}

static Node *parse_while(void)
{
    static const char *od[] = {"od", NULL};
    Pos p = pos_of(NULL);
    Node *n = mk(X_WHILE, p);
    PS->i++;
    n->a = parse_expr();
    expect_kw("do");
    n->b = parse_seq(od);
    expect_kw("od");
    return n;
}

static Node *parse_stmt(void)
{
    Pos p;
    Node *e;
    if (iskw("ignore", 0)) {
        Node *n = mk(X_IGNORE, NOPOS);
        PS->i++;
        n->a = parse_expr();
        return n;
    }
    p = pos_of(NULL);
    e = parse_expr();
    if (isop(":=", 0)) {
        Node *n;
        if (e->k != X_VAR && e->k != X_PROPGET && e->k != X_INDEX)
            perr("left side of ':=' cannot be assigned to");
        PS->i++;
        n = mk(X_ASSIGN, p);
        n->a = e;
        n->b = parse_expr();
        return n;
    }
    if (isop("--", 0)) {
        /* `thing -- property` deletes the property from the thing */
        Node *n;
        PS->i++;
        n = mk(X_DELPROP, p);
        n->a = e;
        n->b = parse_postfix();
        return n;
    }
    return e;
}

static Node *parse_seq(const char **terms)
{
    Node *seq = mk(X_SEQ, NOPOS);
    int value_last = 0;
    for (;;) {
        Tok *t;
        while (accept_op(";"))
            ;
        t = tok();
        if (t->k == T_EOF)
            perr("unexpected end of file");
        if ((t->k == T_ID || t->k == T_OP) && in_set(t->v, terms))
            break;
        push_x(seq, parse_stmt());
        if (accept_op(";")) {
            value_last = 0;
            continue;
        }
        value_last = 1;
        break;
    }
    seq->value_last = value_last && seq->nx > 0;
    return seq;
}

static void add_param(Param **ps, int *n, char *ty, char *name)
{
    *ps = xrealloc(*ps, sizeof(Param) * (size_t)(*n + 1));
    (*ps)[*n].type = ty;
    (*ps)[*n].name = name;
    (*n)++;
}

static void parse_params(Param **ps, int *n)
{
    *ps = NULL;
    *n = 0;
    expect_op("(");
    if (!isop(")", 0)) {
        for (;;) {
            char *ty = parse_type();
            add_param(ps, n, ty, ident());
            while (accept_op(",")) {
                if (at_type())
                    ty = parse_type();
                add_param(ps, n, ty, ident());
            }
            if (!accept_op(";"))
                break;
        }
    }
    expect_op(")");
}

static void parse_body(Proc *p)
{
    static const char *corp[] = {"corp", NULL};
    p->decls = NULL;
    p->ndecls = 0;
    while (at_type()) {
        char *ty = parse_type();
        add_param(&p->decls, &p->ndecls, ty, ident());
        while (accept_op(","))
            add_param(&p->decls, &p->ndecls, ty, ident());
        expect_op(";");
    }
    p->body = parse_seq(corp);
    expect_kw("corp");
}

static void add_proc(Proc *p)
{
    all_procs = xrealloc(all_procs, sizeof(Proc *) * (size_t)(nall_procs + 1));
    all_procs[nall_procs++] = p;
}

static Node *parse_anon_proc(void)
{
    Pos p = pos_of(NULL);
    Proc *proc = xmalloc(sizeof(Proc));
    Node *n;
    memset(proc, 0, sizeof(Proc));
    PS->i++;
    proc->rtype = "void";
    if (isop("(", 0)) {
        parse_params(&proc->params, &proc->nparams);
        if (at_type())
            proc->rtype = parse_type();
        accept_op(":");
    }
    parse_body(proc);
    proc->name = fmt("<anonymous %s:%d>", p.file, p.line);
    proc->pos = p;
    add_proc(proc);
    n = mk(X_ANON, NOPOS);
    n->proc = proc;
    return n;
}

static Proc *parse_procdef(void)
{
    Pos p = pos_of(NULL);
    Proc *proc = xmalloc(sizeof(Proc));
    memset(proc, 0, sizeof(Proc));
    expect_kw("proc");
    proc->name = ident();
    parse_params(&proc->params, &proc->nparams);
    proc->rtype = at_type() ? parse_type() : "void";
    expect_op(":");
    parse_body(proc);
    if (!accept_op(";"))
        accept_op("$");
    proc->pos = p;
    add_proc(proc);
    return proc;
}

/* top-level items */
enum { I_NONE, I_SOURCE, I_PROC, I_DECL, I_STMT };

typedef struct {
    int kind;
    char *raw;                  /* SOURCE */
    Proc *proc;                 /* PROC */
    char *name;                 /* DECL */
    Node *e;                    /* DECL, STMT */
    int line;
} Item;

static Item next_item(void)
{
    static const char *dollar[] = {"$", NULL};
    Item it;
    Tok *t;
    memset(&it, 0, sizeof it);
    for (;;) {
        t = tok();
        if (t->k == T_EOF) {
            it.kind = I_NONE;
            return it;
        }
        if (t->k == T_OP && (strcmp(t->v, "$") == 0 || strcmp(t->v, ";") == 0)) {
            PS->i++;
            continue;
        }
        break;
    }
    PS->start = PS->i;
    PS->item_is_proc = iskw("proc", 0) ||
                       ((iskw("private", 0) || iskw("public", 0)) && iskw("proc", 1));
    t = tok();
    if (t->k == T_ID && strcmp(t->v, "source") == 0) {
        const char *eolp = strchr(PS->text + t->e, '\n');
        size_t eol = eolp ? (size_t)(eolp - PS->text) : strlen(PS->text);
        char *raw = xstrndup(PS->text + t->e, eol - t->e);
        raw = strip(raw);
        raw = strip_chars(raw, "$;", 0, 1);
        raw = strip(raw);
        PS->i++;
        while (tok()->k != T_EOF && tok()->s < eol)
            PS->i++;
        it.kind = I_SOURCE;
        it.raw = raw;
        it.line = t->line;
        return it;
    }
    if (iskw("private", 0) || iskw("public", 0)) {
        PS->i++;
        if (iskw("proc", 0)) {
            it.kind = I_PROC;
            it.proc = parse_procdef();
            return it;
        }
        it.name = ident();
        it.e = parse_expr();
        expect_op("$");
        it.kind = I_DECL;
        it.line = t->line;
        return it;
    }
    if (iskw("proc", 0)) {
        it.kind = I_PROC;
        it.proc = parse_procdef();
        return it;
    }
    it.e = parse_seq(dollar);
    if (accept_op("$")) {
        ;
    } else if (isop(".", 0) && !opt_strict) {
        warn("stray '.' where '$' was expected; treating it as '$'", pos_of(NULL));
        PS->i++;
    } else {
        perr("expected ';' or '$' but found %s", desc(tok()));
    }
    it.kind = I_STMT;
    it.line = t->line;
    return it;
}

/* Skip the broken top-level item so loading can carry on. */
static void recover(void)
{
    int j = PS->start, dep = 0, n = PS->ntoks;
    while (j < n) {
        Tok *t = &PS->toks[j];
        if (t->k == T_EOF)
            break;
        if (t->k == T_ID && strcmp(t->v, "proc") == 0) {
            dep++;
        } else if (t->k == T_ID && strcmp(t->v, "corp") == 0) {
            dep--;
            if (dep <= 0 && PS->item_is_proc) {
                j++;
                if (PS->toks[j].k == T_OP && (strcmp(PS->toks[j].v, ";") == 0 ||
                                              strcmp(PS->toks[j].v, "$") == 0))
                    j++;
                break;
            }
        } else if (t->k == T_OP && strcmp(t->v, "$") == 0 && dep <= 0 &&
                   !PS->item_is_proc) {
            j++;
            break;
        }
        j++;
    }
    PS->i = j > PS->start + 1 ? j : PS->start + 1;
}

/* ====================================================================
 *  Encoded object names
 *
 *    name   := group ('.' group)*          alternative ways to refer to it
 *    group  := nouns [';' adjectives]      e.g.  "overcoat,coat;nondescript"
 *
 *  A typed phrase matches a group if its last word is one of the nouns and
 *  every earlier word is one of the adjectives.  The display form is the
 *  group's adjectives followed by its first noun ("nondescript overcoat").
 * ==================================================================== */

typedef struct {
    char **nouns;
    int nn;
    char **adjs;
    int na;
} Group;

static void parse_list_part(const char *s, int low, char ***out, int *n)
{
    int k, i;
    char **parts = split_ch(s, ',', &k);
    *out = NULL;
    *n = 0;
    for (i = 0; i < k; i++) {
        char *x = strip(parts[i]);
        if (!*x)
            continue;
        *out = xrealloc(*out, sizeof(char *) * (size_t)(*n + 1));
        (*out)[(*n)++] = low ? lower(x) : x;
    }
}

static void partition(const char *s, char sep, char **head, char **tail)
{
    const char *q = strchr(s, sep);
    if (!q) {
        *head = xstrdup(s);
        *tail = xstrdup("");
    } else {
        *head = xstrndup(s, (size_t)(q - s));
        *tail = xstrdup(q + 1);
    }
}

static Map name_cache;

typedef struct {
    Group *g;
    int n;
} Groups;

static Groups *parse_name(const char *enc)
{
    Groups *gs = map_get(&name_cache, enc);
    int k, i;
    char **parts;
    if (gs)
        return gs;
    gs = xmalloc(sizeof(Groups));
    gs->g = NULL;
    gs->n = 0;
    parts = split_ch(enc, '.', &k);
    for (i = 0; i < k; i++) {
        char *n, *a, *st = strip(parts[i]);
        Group g;
        if (!*st)
            continue;
        partition(parts[i], ';', &n, &a);
        parse_list_part(n, 1, &g.nouns, &g.nn);
        parse_list_part(a, 1, &g.adjs, &g.na);
        if (g.nn) {
            gs->g = xrealloc(gs->g, sizeof(Group) * (size_t)(gs->n + 1));
            gs->g[gs->n++] = g;
        }
    }
    map_set(&name_cache, enc, gs);
    return gs;
}

static int in_list(const char *w, char **l, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(l[i], w) == 0)
            return 1;
    return 0;
}

static int match_name(const char *enc, char **words, int nw)
{
    Groups *gs;
    int idx, i;
    if (!nw)
        return -1;
    gs = parse_name(enc);
    for (idx = 0; idx < gs->n; idx++) {
        Group *g = &gs->g[idx];
        int ok = in_list(words[nw - 1], g->nouns, g->nn);
        for (i = 0; ok && i < nw - 1; i++)
            if (!in_list(words[i], g->adjs, g->na))
                ok = 0;
        if (ok)
            return idx;
    }
    return -1;
}

static char *format_name(const char *enc)
{
    int k, i, nn, na;
    char **parts, **nouns, **adjs, *n, *a;
    char *first = NULL;
    Buf b = {0};
    if (!strpbrk(enc, ";,."))
        return xstrdup(enc);            /* already plain text */
    parts = split_ch(enc, '.', &k);
    for (i = 0; i < k; i++) {
        if (*strip(parts[i])) {
            first = parts[i];
            break;
        }
    }
    if (!first)
        return xstrdup(enc);
    partition(first, ';', &n, &a);
    parse_list_part(n, 0, &nouns, &nn);
    parse_list_part(a, 0, &adjs, &na);
    if (!nn)
        return xstrdup(enc);
    buf_puts(&b, "");
    for (i = 0; i < na; i++)
        buf_printf(&b, "%s ", adjs[i]);
    buf_puts(&b, nouns[0]);
    return b.s;
}

static int is_plural(const char *name)
{
    int n;
    char **w = split_ws(name, &n);
    char *last;
    if (!n)
        return 0;
    last = lower(w[n - 1]);
    return strlen(last) > 2 && ends_with(last, "s") && !ends_with(last, "ss") &&
           !ends_with(last, "us") && !ends_with(last, "is");
}

/* ====================================================================
 *  Builtins
 * ==================================================================== */

static const char *ARTICLES[] = {"a", "an", "the", "some", NULL};

static Val need(Val v, int ok, const char *what)
{
    if (!ok)
        mud_error(NOPOS, "expected %s, got %s", what, type_name(v));
    return v;
}

#define NEED_STR(v) need((v), (v).t == V_STR, "a string")
#define NEED_LIST(v) need((v), (v).t == V_LIST, "a thing list")
#define NEED_THING(v) need((v), (v).t == V_THING, "a thing")
#define NEED_GRAMMAR(v) need((v), (v).t == V_GRAMMAR, "a grammar")
#define NEED_BOOL(v) need((v), (v).t == V_BOOL, "a bool")

static Val b_print(Val *a)
{
    out_write(NEED_STR(a[0]).u.s);
    return vnil();
}

static Val b_inttostring(Val *a)
{
    need(a[0], is_int(a[0]), "an int");
    return vstr_own(py_str(a[0]));
}

static Val b_formatname(Val *a)
{
    return vstr_own(format_name(NEED_STR(a[0]).u.s));
}

static Val b_aan(Val *a)
{
    char *name = strip(NEED_STR(a[1]).u.s);
    const char *art;
    if (!*name)
        return a[0];
    if (is_plural(name))
        art = "some";
    else
        art = strchr("aeiou", lower(name)[0]) ? "an" : "a";
    return vstr_own(fmt("%s %s %s", py_str(a[0]), art, name));
}

static Val b_isare(Val *a)
{
    char *c = py_str(a[2]);
    return vstr_own(fmt("%s %s %s %s %s", py_str(a[0]), is_plural(c) ? "are" : "is",
                        py_str(a[1]), c, py_str(a[3])));
}

static Val b_matchname(Val *a)
{
    int n;
    char **w;
    NEED_STR(a[0]);
    w = split_ws(lower(NEED_STR(a[1]).u.s), &n);
    return vint(match_name(a[0].u.s, w, n));
}

static TList *FN_list;
static Prop *FN_prop;

static TList *fn_search(char **ws, int nw)
{
    TList *found = tlist_new();
    int i, j;
    for (i = 0; i < FN_list->n; i++) {
        Val t = FN_list->items[i];
        Val nm = t.t == V_THING ? thing_get(t.u.th, FN_prop) : vnil();
        int dup = 0;
        if (nm.t != V_STR || match_name(nm.u.s, ws, nw) < 0)
            continue;
        for (j = 0; j < found->n; j++)
            if (same(found->items[j], t))
                dup = 1;
        if (!dup)
            tlist_insert(found, found->n, t);
    }
    return found;
}

static Val b_findname(Val *a)
{
    int nw, k;
    char **words;
    TList *found;
    NEED_LIST(a[0]);
    need(a[1], a[1].t == V_PROP, "a property");
    words = split_ws(lower(NEED_STR(a[2]).u.s), &nw);
    FN_list = a[0].u.l;
    FN_prop = a[1].u.p;
    found = fn_search(words, nw);
    if (!found->n && nw) {                      /* forgive simple plurals */
        char *w = words[nw - 1];
        size_t L = strlen(w);
        char *alts[2];
        alts[0] = ends_with(w, "es") ? xstrndup(w, L - 2) : NULL;
        alts[1] = ends_with(w, "s") ? xstrndup(w, L - 1) : NULL;
        for (k = 0; k < 2; k++) {
            if (alts[k] && *alts[k]) {
                char **ws2 = xmalloc(sizeof(char *) * (size_t)nw);
                memcpy(ws2, words, sizeof(char *) * (size_t)(nw - 1));
                ws2[nw - 1] = alts[k];
                found = fn_search(ws2, nw);
                if (found->n)
                    break;
            }
        }
    }
    find_result = found->n == 1 ? found->items[0] : vnil();
    if (!found->n)
        return vstatus(ST_FAIL);
    return vstatus(found->n == 1 ? ST_SUCCEED : ST_CONTINUE);
}

static Val b_findresult(Val *a)
{
    (void)a;
    return find_result;
}

static Val new_prop(int kind)
{
    Prop *p = xmalloc(sizeof(Prop));
    p->kind = kind;
    p->name = NULL;
    return vprop(p);
}

static Val b_cstringprop(Val *a) { (void)a; return new_prop(K_STRING); }
static Val b_cintprop(Val *a) { (void)a; return new_prop(K_INT); }
static Val b_cboolprop(Val *a) { (void)a; return new_prop(K_BOOL); }
static Val b_cthingprop(Val *a) { (void)a; return new_prop(K_THING); }
static Val b_cactionprop(Val *a) { (void)a; return new_prop(K_ACTION); }
static Val b_clistprop(Val *a) { (void)a; return new_prop(K_LIST); }

static Val b_creatething(Val *a)
{
    if (a[0].t != V_NIL)
        need(a[0], a[0].t == V_THING, "a thing or nil");
    return vthing(thing_new(a[0].t == V_THING ? a[0].u.th : NULL));
}

static Val b_createthinglist(Val *a)
{
    (void)a;
    return vlist(tlist_new());
}

static Val b_count(Val *a)
{
    return vint(NEED_LIST(a[0]).u.l->n);
}

static Val b_addtail(Val *a)
{
    TList *l = NEED_LIST(a[0]).u.l;
    tlist_insert(l, l->n, NEED_THING(a[1]));
    return vnil();
}

static Val b_addhead(Val *a)
{
    TList *l = NEED_LIST(a[0]).u.l;
    tlist_insert(l, 0, NEED_THING(a[1]));
    return vnil();
}

static Val b_delelement(Val *a)
{
    TList *l = NEED_LIST(a[0]).u.l;
    int i;
    for (i = 0; i < l->n; i++) {
        if (same(l->items[i], a[1])) {
            memmove(l->items + i, l->items + i + 1, sizeof(Val) * (size_t)(l->n - i - 1));
            l->n--;
            break;
        }
    }
    return vnil();
}

static Val b_me(Val *a)
{
    (void)a;
    return player ? vthing(player) : vnil();
}

static Val b_here(Val *a)
{
    (void)a;
    return player_loc;
}

static Val b_setlocation(Val *a)
{
    player_loc = NEED_THING(a[0]);
    return vnil();
}

static Val b_maingrammar(Val *a)
{
    Grammar *g = xmalloc(sizeof(Grammar));
    Val v;
    (void)a;
    memset(g, 0, sizeof(Grammar));
    v.t = V_GRAMMAR;
    v.u.g = g;
    return v;
}

static Val b_word(Val *a)
{
    char *s;
    NEED_GRAMMAR(a[0]);
    s = lower(NEED_STR(a[1]).u.s);
    map_set(&a[0].u.g->words, s, (void *)1);
    return vstr_own(s);
}

static Val b_findword(Val *a)
{
    char *s;
    NEED_GRAMMAR(a[0]);
    s = lower(NEED_STR(a[1]).u.s);
    if (!map_has(&a[0].u.g->words, s))
        mud_error(NOPOS, "the word \"%s\" is not defined in the grammar", s);
    return vstr_own(s);
}

static Val make_verb(Val *a, int kind)
{
    Grammar *g;
    char *name;
    char *prep = NULL;
    Verb *v;
    NEED_GRAMMAR(a[0]);
    g = a[0].u.g;
    name = lower(NEED_STR(a[1]).u.s);
    if ((is_int(a[2]) && a[2].u.i == 0) || a[2].t == V_NIL)
        prep = NULL;
    else
        prep = NEED_STR(a[2]).u.s;
    need(a[3], a[3].t == V_PROC || a[3].t == V_BUILTIN, "an action");
    map_set(&g->words, name, (void *)1);
    v = xmalloc(sizeof(Verb));
    v->kind = kind;
    v->name = name;
    v->prep = prep;
    v->routine = a[3];
    map_set(&g->verbs, name, v);
    return vnil();
}

static Val b_verb0(Val *a) { return make_verb(a, 0); }
static Val b_verb1(Val *a) { return make_verb(a, 1); }
static Val b_verb2(Val *a) { return make_verb(a, 2); }

static Val b_synonym(Val *a)
{
    Grammar *g;
    char *name, *nw;
    Verb *v;
    NEED_GRAMMAR(a[0]);
    g = a[0].u.g;
    name = lower(NEED_STR(a[1]).u.s);
    nw = lower(NEED_STR(a[2]).u.s);
    v = map_get(&g->verbs, name);
    if (!v)
        mud_error(NOPOS, "cannot make a synonym: no verb \"%s\"", name);
    map_set(&g->words, nw, (void *)1);
    map_set(&g->verbs, nw, v);
    return vnil();
}

static char *phrase(char **ws, int n)
{
    Buf b = {0};
    int i;
    if (n > 1 && in_set(ws[0], ARTICLES)) {
        ws++;
        n--;
    }
    buf_puts(&b, "");
    for (i = 0; i < n; i++)
        buf_printf(&b, "%s%s", i ? " " : "", ws[i]);
    return b.s;
}

static Val parse_line(Grammar *g, const char *line)
{
    int nw0, nw = 0, i, nrest, nargs = 0;
    char **w0 = split_ws(lower(line), &nw0);
    char **words = xmalloc(sizeof(char *) * (size_t)(nw0 + 1));
    char **rest;
    Verb *v;
    Val args[2], res;
    for (i = 0; i < nw0; i++) {
        char *w = strip_chars(w0[i], ".,;!?", 1, 1);
        if (*w)
            words[nw++] = w;
    }
    if (!nw)
        return vint(0);
    v = map_get(&g->verbs, words[0]);
    if (!v) {
        out_write(fmt("I don't understand \"%s\".\n", words[0]));
        return vint(0);
    }
    rest = words + 1;
    nrest = nw - 1;
    if (v->kind == 0) {
        if (nrest) {
            Buf b = {0};
            for (i = 0; i < nrest; i++)
                buf_printf(&b, "%s%s", i ? " " : "", rest[i]);
            out_write(fmt("I don't understand what to do with \"%s\" after \"%s\".\n",
                          b.s, words[0]));
            return vint(0);
        }
    } else if (v->kind == 1) {
        if (v->prep && nrest && strcmp(rest[0], v->prep) == 0) {
            rest++;
            nrest--;
        } else if (v->prep && nrest > 1 && strcmp(rest[nrest - 1], v->prep) == 0) {
            nrest--;
        }
        args[nargs++] = vstr_own(phrase(rest, nrest));
    } else {
        int k = -1;
        if (v->prep)
            for (i = 0; i < nrest; i++)
                if (strcmp(rest[i], v->prep) == 0) {
                    k = i;
                    break;
                }
        if (k >= 0) {
            args[nargs++] = vstr_own(phrase(rest, k));
            args[nargs++] = vstr_own(phrase(rest + k + 1, nrest - k - 1));
        } else {
            args[nargs++] = vstr_own(phrase(rest, nrest));
            args[nargs++] = vstr("");
        }
    }
    res = call(v->routine, args, nargs, NOPOS);
    return vint(res.t == V_BOOL && res.u.i ? 1 : 0);
}

static Val b_parse(Val *a)
{
    NEED_GRAMMAR(a[0]);
    return parse_line(a[0].u.g, NEED_STR(a[1]).u.s);
}

static Val b_setprompt(Val *a)
{
    char *old = prompt;
    prompt = NEED_STR(a[0]).u.s;
    return vstr(old);
}

static Val b_setinput(Val *a)
{
    Val old = input_action;
    input_action = a[0];
    return old;
}

static Val b_setnew(Val *a)
{
    Val old = new_char_action;
    new_char_action = a[0];
    return old;
}

static Val b_setcontinue(Val *a)
{
    continue_on_error = (int)NEED_BOOL(a[0]).u.i;
    return vnil();
}

static Val b_setsingle(Val *a)
{
    single_user = (int)NEED_BOOL(a[0]).u.i;
    return vnil();
}

static Val b_quit(Val *a)
{
    (void)a;
    quit_flag = 1;
    return vnil();
}

static void reg(const char *name, int nargs, Val (*fn)(Val *))
{
    Builtin *b = xmalloc(sizeof(Builtin));
    Val *v = xmalloc(sizeof(Val));
    b->name = name;
    b->nargs = nargs;
    b->fn = fn;
    v->t = V_BUILTIN;
    v->u.b = b;
    map_set(&builtins, name, v);
}

static void register_builtins(void)
{
    reg("Print", 1, b_print);
    reg("IntToString", 1, b_inttostring);
    reg("FormatName", 1, b_formatname);
    reg("AAn", 2, b_aan);
    reg("IsAre", 4, b_isare);
    reg("MatchName", 2, b_matchname);
    reg("FindName", 3, b_findname);
    reg("FindResult", 0, b_findresult);
    reg("CreateStringProp", 0, b_cstringprop);
    reg("CreateIntProp", 0, b_cintprop);
    reg("CreateBoolProp", 0, b_cboolprop);
    reg("CreateThingProp", 0, b_cthingprop);
    reg("CreateActionProp", 0, b_cactionprop);
    reg("CreateThingListProp", 0, b_clistprop);
    reg("CreateThing", 1, b_creatething);
    reg("CreateThingList", 0, b_createthinglist);
    reg("Count", 1, b_count);
    reg("AddTail", 2, b_addtail);
    reg("AddHead", 2, b_addhead);
    reg("DelElement", 2, b_delelement);
    reg("Me", 0, b_me);
    reg("Here", 0, b_here);
    reg("SetLocation", 1, b_setlocation);
    reg("MainGrammar", 0, b_maingrammar);
    reg("Word", 2, b_word);
    reg("FindWord", 2, b_findword);
    reg("Verb0", 4, b_verb0);
    reg("Verb1", 4, b_verb1);
    reg("Verb2", 4, b_verb2);
    reg("Synonym", 3, b_synonym);
    reg("Parse", 2, b_parse);
    reg("SetPrompt", 1, b_setprompt);
    reg("SetCharacterInputAction", 1, b_setinput);
    reg("SetNewCharacterAction", 1, b_setnew);
    reg("SetContinue", 1, b_setcontinue);
    reg("SetSingleUser", 1, b_setsingle);
    reg("Quit", 0, b_quit);
}

/* ====================================================================
 *  Loading
 * ==================================================================== */

static int is_sep(char c)
{
    return c == '/' || c == '\\';
}

static int is_abs(const char *p)
{
    return is_sep(p[0]) || (p[0] && p[1] == ':');
}

static char *path_join(const char *a, const char *b)
{
    size_t n = strlen(a);
    if (is_abs(b) || !n)
        return xstrdup(b);
    if (is_sep(a[n - 1]))
        return fmt("%s%s", a, b);
#ifdef _WIN32
    return fmt("%s\\%s", a, b);
#else
    return fmt("%s/%s", a, b);
#endif
}

static char *abspath(const char *p)
{
#ifdef _WIN32
    char buf[4096];
    if (_fullpath(buf, p, sizeof buf))
        return xstrdup(buf);
    return xstrdup(p);
#else
    char buf[4096];
    if (p[0] == '/')
        return xstrdup(p);
    if (getcwd(buf, sizeof buf))
        return path_join(buf, p);
    return xstrdup(p);
#endif
}

static char *dirname_of(const char *p)
{
    const char *s = p, *last = NULL;
    for (; *s; s++)
        if (is_sep(*s))
            last = s;
    if (!last)
        return xstrdup("");
    if (last == p || (last == p + 2 && p[1] == ':'))
        return xstrndup(p, (size_t)(last - p) + 1);
    return xstrndup(p, (size_t)(last - p));
}

static const char *basename_of(const char *p)
{
    const char *s, *b = p;
    for (s = p; *s; s++)
        if (is_sep(*s) || (*s == ':' && s == p + 1))
            b = s + 1;
    return b;
}

static int exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

static int is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

/* AmigaDOS is case-insensitive; be forgiving about it. */
static char *ci_path(const char *path)
{
    char *head, *tail, *lt;
    DIR *d;
    struct dirent *e;
    if (exists(path))
        return xstrdup(path);
    head = dirname_of(path);
    tail = xstrdup(basename_of(path));
    if (*head && !is_dir(head))
        head = ci_path(head);
    d = opendir(*head ? head : ".");
    if (d) {
        lt = lower(tail);
        while ((e = readdir(d)) != NULL) {
            if (strcmp(lower(e->d_name), lt) == 0) {
                char *r = path_join(head, e->d_name);
                closedir(d);
                return r;
            }
        }
        closedir(d);
    }
    return xstrdup(path);
}

static char *resolve(const char *name, const char *cur_dir)
{
    const char *colon = strchr(name, ':');
    char *path;
    if (colon) {
        char *assign = lower(xstrndup(name, (size_t)(colon - name)));
        char *base = map_get(&assigns, assign);
        path = path_join(base ? base : default_root, colon + 1);
    } else {
        path = path_join(cur_dir, name);
    }
    return ci_path(path);
}

static int only_word_chars(const char *s)
{
    if (!*s)
        return 0;
    for (; *s; s++)
        if (!(is_alpha((unsigned char)*s) || is_digit((unsigned char)*s) || *s == '_'))
            return 0;
    return 1;
}

/* `go` starts with two lines (user, password) typed at the login. */
static char *strip_login(char *text)
{
    int n, i, ok = 1;
    char **lines = split_ch(text, '\n', &n);
    Buf b = {0};
    if (n <= 2)
        return text;
    for (i = 0; i < 2; i++) {
        char *l = strip(lines[i]);
        if (!only_word_chars(l) || in_set(l, KEYWORDS) || strcmp(l, "source") == 0)
            ok = 0;
    }
    if (!ok)
        return text;
    if (opt_verbose)
        fprintf(stderr, "(logging in as %s)\n", strip(lines[0]));
    lines[0] = lines[1] = "";
    buf_puts(&b, "");
    for (i = 0; i < n; i++)
        buf_printf(&b, "%s%s", i ? "\n" : "", lines[i]);
    return b.s;
}

/* lex a file; a ParseError is reported, and aborts loading unless
   SetContinue(true) */
static Tok *lex_file(const char *text, const char *fname, int *ntoks)
{
    Handler h;
    Tok *toks;
    push_handler(&h);
    if (SETJMP(h.jb) != 0) {
        caught(&h);
        report();
        if (!continue_on_error)
            load_abort();
        return NULL;
    }
    toks = lex(text, fname, ntoks);
    pop_handler(&h);
    return toks;
}

static void load_file(const char *path, int ldepth)
{
    FILE *f;
    Buf raw = {0};
    char chunk[65536];
    size_t got, i;
    char *text;
    const char *fname;
    Tok *toks;
    int ntoks;
    Parser ps, *saved_ps = PS;
    char *cur_dir;
    Frame top;

    if (ldepth > 10)
        mud_error(NOPOS, "'source' nested too deeply");
    if (opt_verbose)
        fprintf(stderr, "loading %s\n", path);
    f = fopen(path, "rb");
    if (!f || is_dir(path)) {
        int e = f ? EACCES : errno;
        if (f)
            fclose(f);
        mud_error(NOPOS, "cannot read %s: %s", path, strerror(e));
    }
    buf_puts(&raw, "");
    while ((got = fread(chunk, 1, sizeof chunk, f)) > 0)
        buf_putn(&raw, chunk, got);
    fclose(f);
    /* text mode (universal newlines), then .replace('\r\n', '\n') */
    text = xmalloc(raw.n + 1);
    {
        size_t k = 0;
        for (i = 0; i < raw.n; i++) {
            if (raw.s[i] == '\r') {
                text[k++] = '\n';
                if (i + 1 < raw.n && raw.s[i + 1] == '\n')
                    i++;
            } else {
                text[k++] = raw.s[i];
            }
        }
        text[k] = 0;
    }
    text = strip_login(text);
    fname = xstrdup(basename_of(path));

    toks = lex_file(text, fname, &ntoks);
    if (!toks)
        return;
    ps.toks = toks;
    ps.ntoks = ntoks;
    ps.i = 0;
    ps.text = text;
    ps.fname = fname;
    ps.start = 0;
    ps.item_is_proc = 0;
    cur_dir = dirname_of(abspath(path));
    top.vars = NULL;
    top.n = 0;
    for (;;) {
        volatile Item item;
        Handler h;
        memset((void *)&item, 0, sizeof item);
        PS = &ps;
        push_handler(&h);
        if (SETJMP(h.jb) == 0) {
            item = next_item();
            pop_handler(&h);
        } else {
            caught(&h);
            report();
            recover();
            if (!continue_on_error)
                load_abort();
            continue;
        }
        PS = saved_ps;
        if (item.kind == I_NONE)
            break;
        push_handler(&h);
        if (SETJMP(h.jb) == 0) {
            if (item.kind == I_SOURCE) {
                load_file(resolve(item.raw, cur_dir), ldepth + 1);
            } else if (item.kind == I_PROC) {
                set_global(item.proc->name, vproc(item.proc));
            } else if (item.kind == I_DECL) {
                Val val = ev(item.e, &top);
                if (val.t == V_THING && !val.u.th->name)
                    val.u.th->name = item.name;
                else if (val.t == V_PROP && !val.u.p->name)
                    val.u.p->name = item.name;
                else if (val.t == V_GRAMMAR && !val.u.g->name)
                    val.u.g->name = item.name;
                set_global(item.name, val);
            } else {
                ev(item.e, &top);
            }
            pop_handler(&h);
        } else {
            caught(&h);
            if (!E_pos.file) {
                E_pos.file = fname;
                E_pos.line = item.kind == I_PROC ? 0 : item.line;
            }
            report();
            if (!continue_on_error)
                load_abort();
        }
    }
    PS = saved_ps;
}

/* ====================================================================
 *  Lint: symbols that are never defined
 * ==================================================================== */

static Buf lint_out;
static int nlint;

static void walk(Node *n, Proc *p)
{
    int i;
    if (!n)
        return;
    if (n->k == X_VAR) {
        int in_scope = 0;
        for (i = 0; i < p->nparams; i++)
            if (strcmp(p->params[i].name, n->name) == 0)
                in_scope = 1;
        for (i = 0; i < p->ndecls; i++)
            if (strcmp(p->decls[i].name, n->name) == 0)
                in_scope = 1;
        if (!in_scope && !map_has(&globals, n->name) && !map_has(&builtins, n->name)) {
            buf_printf(&lint_out, "lint: %s:%d: undefined symbol '%s' in %s\n",
                       n->pos.file, n->pos.line, n->name, p->name);
            nlint++;
        }
    }
    switch (n->k) {
    case X_LOGIC: case X_BIN: case X_PROPGET: case X_INDEX: case X_WHILE:
    case X_ASSIGN: case X_DELPROP:
        walk(n->a, p);
        walk(n->b, p);
        break;
    case X_NOT: case X_NEG: case X_CALLOF: case X_IGNORE:
        walk(n->a, p);
        break;
    case X_CALL:
        walk(n->a, p);
        for (i = 0; i < n->nx; i++)
            walk(n->xs[i], p);
        break;
    case X_IF:
        for (i = 0; i < n->nx; i++)
            walk(n->xs[i], p);
        if (n->a)
            walk(n->a, p);
        break;
    case X_SEQ:
        for (i = 0; i < n->nx; i++)
            walk(n->xs[i], p);
        break;
    }
}

/* ====================================================================
 *  The game session
 * ==================================================================== */

static char *read_line(FILE *f, int *eof)
{
    Buf b = {0};
    int c, any = 0;
    buf_puts(&b, "");
    while ((c = getc(f)) != EOF) {
        any = 1;
        if (c == '\n')
            break;
        if (c == '\r') {
            int d = getc(f);
            if (d != '\n' && d != EOF)
                ungetc(d, f);
            break;
        }
        if (c == 0) {
            buf_putc(&b, 0xC0);
            buf_putc(&b, 0x80);
        } else {
            buf_putc(&b, c);
        }
    }
    *eof = !any;
    return b.s;
}

static void safe_call(Val fn, Val *args, int nargs)
{
    Handler h;
    push_handler(&h);
    if (SETJMP(h.jb) == 0) {
        call(fn, args, nargs, NOPOS);
        pop_handler(&h);
    } else {
        caught(&h);
        report();
    }
}

static void run_game(void)
{
    int echo = opt_echo;
    player = thing_new(NULL);
    player->name = "<player>";
    if (new_char_action.t != V_NIL)
        safe_call(new_char_action, NULL, 0);
    if (echo < 0)
        echo = !ISATTY(FILENO(stdin));
    while (!quit_flag) {
        int eof;
        char *line;
        flush_out();
        put_str(stdout, prompt);
        fflush(stdout);
        check_stdout();
        line = read_line(stdin, &eof);
        if (eof) {
            fputs("\n", stdout);
            break;
        }
        if (echo) {
            put_str(stdout, line);
            fputs("\n", stdout);
        }
        if (input_action.t == V_NIL) {
            out_write("(no character input action has been set)\n");
        } else {
            Val arg = vstr_own(line);
            safe_call(input_action, &arg, 1);
        }
    }
    flush_out();
}

/* ====================================================================
 *  Main (argparse's rules: --opt value, --opt=value, unique prefixes)
 * ==================================================================== */

static const char *PROG = "amigamud";

/* argparse's usage line, wrapped at 78 columns as it wraps it */
static char *usage_text(void)
{
    static const char *opts[] = {"[-h]", "[--assign NAME=DIR]", "[--width WIDTH]",
                                 "[--strict]", "[--lint]", "[--load-only]",
                                 "[--verbose]", "[--echo]", "[--no-echo]", NULL};
    const char *pos = "[files ...]";
    int text_width = 78, i;
    Buf all = {0}, out = {0};
    size_t plen = strlen("usage: "), proglen = strlen(PROG);
    buf_printf(&all, "%s", PROG);
    for (i = 0; opts[i]; i++)
        buf_printf(&all, " %s", opts[i]);
    buf_printf(&all, " %s", pos);
    if (plen + all.n <= (size_t)text_width)
        return fmt("usage: %s\n", all.s);
    buf_puts(&out, "usage: ");
    if (plen + proglen <= 0.75 * text_width) {
        size_t ind = plen + proglen + 1, line_len = plen - 1;
        Buf line = {0};
        const char *parts[16];
        int np = 0, first = 1;
        parts[np++] = PROG;
        for (i = 0; opts[i]; i++)
            parts[np++] = opts[i];
        buf_puts(&line, "");
        for (i = 0; i < np; i++) {
            if (line_len + 1 + strlen(parts[i]) > (size_t)text_width && line.n) {
                if (!first)
                    buf_printf(&out, "%*s", (int)ind, "");
                buf_printf(&out, "%s\n", line.s);
                first = 0;
                buf_clear(&line);
                line_len = ind - 1;
            }
            buf_printf(&line, "%s%s", line.n ? " " : "", parts[i]);
            line_len += strlen(parts[i]) + 1;
        }
        if (line.n) {
            if (!first)
                buf_printf(&out, "%*s", (int)ind, "");
            buf_printf(&out, "%s\n", line.s);
        }
        buf_printf(&out, "%*s%s\n", (int)ind, "", pos);
    } else {
        Buf line = {0};
        size_t ind = plen, line_len = ind - 1;
        buf_printf(&out, "%s\n", PROG);
        buf_puts(&line, "");
        for (i = 0; opts[i]; i++) {
            if (line_len + 1 + strlen(opts[i]) > (size_t)text_width && line.n) {
                buf_printf(&out, "%*s%s\n", (int)ind, "", line.s);
                buf_clear(&line);
                line_len = ind - 1;
            }
            buf_printf(&line, "%s%s", line.n ? " " : "", opts[i]);
            line_len += strlen(opts[i]) + 1;
        }
        if (line.n)
            buf_printf(&out, "%*s%s\n", (int)ind, "", line.s);
        buf_printf(&out, "%*s%s\n", (int)ind, "", pos);
    }
    return out.s;
}

static void usage_error(const char *f, ...)
{
    va_list ap;
    fputs(usage_text(), stderr);
    fprintf(stderr, "%s: error: ", PROG);
    va_start(ap, f);
    vfprintf(stderr, f, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(2);
}

static int looks_negative(const char *s)
{
    const char *d;
    if (*s != '-')
        return 0;
    s++;
    for (d = s; is_digit((unsigned char)*d); d++)
        ;
    if (d > s && !*d)
        return 1;
    if (*d != '.')
        return 0;
    s = ++d;
    for (; is_digit((unsigned char)*d); d++)
        ;
    return d > s && !*d;
}

static long int_arg(const char *opt, const char *s)
{
    const char *p = s;
    long long v = 0;
    int neg = 0, any = 0;
    Val sv;
    while (py_space((unsigned char)*p))
        p++;
    if (*p == '+' || *p == '-')
        neg = *p++ == '-';
    for (; is_digit((unsigned char)*p) || (*p == '_' && any && is_digit((unsigned char)p[1])); p++) {
        if (*p == '_')
            continue;
        any = 1;
        v = v * 10 + (*p - '0');
        if (v > 2000000000LL)
            v = 2000000000LL;
    }
    while (py_space((unsigned char)*p))
        p++;
    if (!any || *p) {
        sv.t = V_STR;
        sv.u.s = (char *)s;
        usage_error("argument %s: invalid int value: %s", opt, py_repr(sv));
    }
    return (long)(neg ? -v : v);
}

static void interrupted(int sig)
{
    (void)sig;
    fputs("\n", stdout);
    fflush(stdout);
    _Exit(0);
}

/* load the boot files: 0, or 2 when loading was aborted or failed */
static int load_all(const char **files, int nfiles)
{
    static int fi;
    Handler h;
    if (SETJMP(abort_jmp) != 0) {
        fputs("loading aborted (SetContinue(false) and an error occurred)\n", stderr);
        return 2;
    }
    push_handler(&h);
    if (SETJMP(h.jb) != 0) {
        caught(&h);
        report();
        return 2;
    }
    for (fi = 0; fi < nfiles; fi++)
        load_file(files[fi], 0);
    pop_handler(&h);
    return 0;
}

int main(int argc, char **argv)
{
    static const char *longopts[] = {"--help", "--assign", "--width", "--strict",
                                     "--lint", "--load-only", "--verbose",
                                     "--echo", "--no-echo", NULL};
    static const int takes_arg[] = {0, 1, 1, 0, 0, 0, 0, 0, 0};
    const char **files = NULL;
    int nfiles = 0, i, only_pos = 0, lint = 0, load_only = 0;
    Buf unrec = {0};
    char **assign_specs = NULL;
    int nassign = 0;

    setvbuf(stdout, NULL, _IOFBF, 1 << 16);
    if (argc > 0 && argv[0] && *argv[0])
        PROG = basename_of(argv[0]);
    prompt = "> ";
    input_action = new_char_action = player_loc = find_result = vnil();

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!only_pos && strcmp(a, "--") == 0) {
            only_pos = 1;
            continue;
        }
        if (!only_pos && strcmp(a, "-h") == 0) {
            printf("%s\nAmiga MUD language interpreter\n\n"
                   "positional arguments:\n"
                   "  files              boot script(s), default: ./go\n\n"
                   "options:\n"
                   "  -h, --help         show this help message and exit\n"
                   "  --assign NAME=DIR\n  --width WIDTH\n  --strict\n  --lint\n"
                   "  --load-only\n  --verbose\n  --echo\n  --no-echo\n", usage_text());
            return 0;
        }
        if (!only_pos && a[0] == '-' && a[1] == '-' && a[2]) {
            const char *eq = strchr(a, '=');
            size_t nlen = eq ? (size_t)(eq - a) : strlen(a);
            int match = -1, nmatch = 0, k;
            Buf amb = {0};
            const char *val = NULL;
            for (k = 0; longopts[k]; k++) {
                if (strlen(longopts[k]) == nlen && strncmp(longopts[k], a, nlen) == 0) {
                    match = k;
                    nmatch = 1;
                    break;
                }
            }
            if (match < 0) {
                for (k = 0; longopts[k]; k++) {
                    if (strncmp(longopts[k], a, nlen) == 0) {
                        buf_printf(&amb, "%s%s", nmatch ? ", " : "", longopts[k]);
                        match = k;
                        nmatch++;
                    }
                }
            }
            if (nmatch > 1)
                usage_error("ambiguous option: %s could match %s", a, amb.s);
            if (nmatch == 0) {
                buf_printf(&unrec, "%s%s", unrec.n ? " " : "", a);
                continue;
            }
            if (takes_arg[match]) {
                if (eq) {
                    val = eq + 1;
                } else {
                    if (i + 1 >= argc || (argv[i + 1][0] == '-' && argv[i + 1][1] &&
                                          !looks_negative(argv[i + 1])))
                        usage_error("argument %s: expected one argument",
                                    longopts[match]);
                    val = argv[++i];
                }
            } else if (eq) {
                Val sv;
                sv.t = V_STR;
                sv.u.s = (char *)(eq + 1);
                usage_error("argument %s: ignored explicit argument %s",
                            longopts[match], py_repr(sv));
            }
            switch (match) {
            case 0:
                argv[i] = "-h";
                i--;
                break;
            case 1:
                assign_specs = xrealloc(assign_specs, sizeof(char *) * (size_t)(nassign + 1));
                assign_specs[nassign++] = (char *)val;
                break;
            case 2: width = int_arg("--width", val); break;
            case 3: opt_strict = 1; break;
            case 4: lint = 1; break;
            case 5: load_only = 1; break;
            case 6: opt_verbose = 1; break;
            case 7: opt_echo = 1; break;
            case 8: opt_echo = 0; break;
            }
            continue;
        }
        if (!only_pos && a[0] == '-' && a[1] && !looks_negative(a)) {
            buf_printf(&unrec, "%s%s", unrec.n ? " " : "", a);
            continue;
        }
        files = xrealloc(files, sizeof(char *) * (size_t)(nfiles + 1));
        files[nfiles++] = a;
    }
    if (unrec.n)
        usage_error("unrecognized arguments: %s", unrec.s);

    if (!nfiles) {
        files = xmalloc(sizeof(char *));
        files[nfiles++] = "go";
    }
    register_builtins();
    default_root = dirname_of(abspath(files[0]));
    for (i = 0; i < nassign; i++) {
        char *name, *d;
        partition(assign_specs[i], '=', &name, &d);
        map_set(&assigns, strip_chars(lower(name), ":", 0, 1), d);
    }
    signal(SIGINT, interrupted);

    if (load_all(files, nfiles) != 0)
        return 2;
    if (lint) {
        for (i = 0; i < nall_procs; i++)
            walk(all_procs[i]->body, all_procs[i]);
        if (nlint)
            put_str(stderr, lint_out.s);
        else
            fputs("lint: no undefined symbols\n", stderr);
    }
    if (load_only) {
        fflush(stdout);
        return nerrors ? 1 : 0;
    }
#ifdef _WIN32
    if (!ISATTY(FILENO(stdin)))
        _setmode(FILENO(stdin), _O_BINARY);
#endif
    run_game();
    fflush(stdout);
    return 0;
}
