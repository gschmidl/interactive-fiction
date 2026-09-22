/*
 * six.c -- a native build of six.py 1.0, the interpreter for the SIX
 * language of the SIX/FANT system (University of Alberta, late 1970s).
 *
 * A translation of six.py, statement for statement, so that it behaves the
 * same way: the same output byte for byte, the same warnings and error
 * messages, and the same random numbers for '?' with --seed (Python's
 * Mersenne Twister and randrange(10000) are reproduced exactly).  Strings
 * are bytes; the worlds are ASCII.
 *
 *     gcc -O2 -s -o six.exe six.c -Wl,--stack,268435456
 *
 * The stack is large for the same reason six.py runs in a thread with a
 * 256 MB stack: SIX procedures may nest 20000 deep (--max-depth).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <process.h>
#define ISATTY _isatty
#define FILENO _fileno
#define GETPID _getpid
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#define GETPID getpid
#endif

#define VERSION "1.0"
#define INT_MIN24 (-8388608L)
#define INT_MAX24 8388607L

/* ====================================================================
 *  Memory and byte buffers (nothing is ever freed: a game is short)
 * ==================================================================== */

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) {
        fputs("six: out of memory\n", stderr);
        exit(1);
    }
    return p;
}

static void *xrealloc(void *p, size_t n)
{
    p = realloc(p, n ? n : 1);
    if (!p) {
        fputs("six: out of memory\n", stderr);
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

static const char *buf_str(const Buf *b)
{
    return b->s ? b->s : "";
}

/* ====================================================================
 *  Values
 *
 *  int -> int32 (24-bit), string -> Str (bytes), nil, absent, list
 *  (singly linked), table (hash table; things and verbs too), prop
 *  (identity only), proc
 * ==================================================================== */

typedef enum { T_NIL, T_ABSENT, T_INT, T_STR, T_LIST, T_TABLE, T_PROC,
               T_PROP } Type;

typedef struct Str {
    size_t len;
    char s[1];
} Str;

typedef struct List List;
typedef struct Table Table;
typedef struct Proc Proc;
typedef struct Prop Prop;
typedef struct Ast Ast;

typedef struct {
    Type t;
    union {
        long i;
        Str *s;
        List *l;
        Table *tb;
        Proc *p;
        Prop *pr;
    } u;
} Val;

typedef struct LNode {
    Val v;
    struct LNode *next;
    int dead;
} LNode;

struct List {
    LNode *head, *tail;
};

typedef struct Ent {
    Val k, v;
    struct Ent *next;
} Ent;

struct Table {
    Ent **b;
    size_t nb, n;
    const char *name;
};

struct Prop {
    const char *name;
};

struct Proc {
    const char *name;
    int nparams, nslots, is_func;
    Ast *body;
};

static Val v_nil(void)
{
    Val v;
    v.t = T_NIL;
    v.u.i = 0;
    return v;
}

static Val v_absent(void)
{
    Val v;
    v.t = T_ABSENT;
    v.u.i = 0;
    return v;
}

static Val v_int(long i)
{
    Val v;
    v.t = T_INT;
    v.u.i = i;
    return v;
}

static Str *str_new(const char *s, size_t n)
{
    Str *p = xmalloc(sizeof(Str) + n);
    p->len = n;
    if (n)
        memcpy(p->s, s, n);
    p->s[n] = 0;
    return p;
}

static Val v_str(Str *s)
{
    Val v;
    v.t = T_STR;
    v.u.s = s;
    return v;
}

static Val v_strn(const char *s, size_t n)
{
    return v_str(str_new(s, n));
}

static Val v_list(List *l)
{
    Val v;
    v.t = T_LIST;
    v.u.l = l;
    return v;
}

static Val v_table(Table *t)
{
    Val v;
    v.t = T_TABLE;
    v.u.tb = t;
    return v;
}

static Val v_proc(Proc *p)
{
    Val v;
    v.t = T_PROC;
    v.u.p = p;
    return v;
}

static Val v_prop(Prop *p)
{
    Val v;
    v.t = T_PROP;
    v.u.pr = p;
    return v;
}

static List *list_new(void)
{
    List *l = xmalloc(sizeof(List));
    l->head = l->tail = NULL;
    return l;
}

static Table *table_new(const char *name)
{
    Table *t = xmalloc(sizeof(Table));
    t->nb = 8;
    t->n = 0;
    t->b = xmalloc(t->nb * sizeof(Ent *));
    memset(t->b, 0, t->nb * sizeof(Ent *));
    t->name = name;
    return t;
}

static Prop *prop_new(const char *name)
{
    Prop *p = xmalloc(sizeof(Prop));
    p->name = xstrdup(name);
    return p;
}

static Proc *proc_new(const char *name)
{
    Proc *p = xmalloc(sizeof(Proc));
    p->name = xstrdup(name);
    p->nparams = p->nslots = p->is_func = 0;
    p->body = NULL;
    return p;
}

static const char *type_name(Val v)
{
    switch (v.t) {
    case T_NIL: return "nil";
    case T_INT: return "int";
    case T_STR: return "string";
    case T_ABSENT: return "absent";
    case T_LIST: return "list";
    case T_TABLE: return "table";
    case T_PROC: return "proc";
    case T_PROP: return "prop";
    }
    return "?";
}

/* "A true value is any nonempty value of that type." */
static int truth(Val v)
{
    switch (v.t) {
    case T_NIL:
    case T_ABSENT: return 0;
    case T_INT: return v.u.i != 0;
    case T_STR: return v.u.s->len > 0;
    case T_LIST: return v.u.l->head != NULL;
    case T_TABLE: return v.u.tb->n > 0;
    default: return 1;
    }
}

/* Ints and strings compare by value; everything else by identity. */
static int equal(Val a, Val b)
{
    if (a.t == T_INT)
        return b.t == T_INT && a.u.i == b.u.i;
    if (a.t == T_STR)
        return b.t == T_STR && a.u.s->len == b.u.s->len &&
               memcmp(a.u.s->s, b.u.s->s, a.u.s->len) == 0;
    if (a.t != b.t)
        return 0;
    switch (a.t) {
    case T_NIL:
    case T_ABSENT: return 1;
    case T_LIST: return a.u.l == b.u.l;
    case T_TABLE: return a.u.tb == b.u.tb;
    case T_PROC: return a.u.p == b.u.p;
    case T_PROP: return a.u.pr == b.u.pr;
    default: return 0;
    }
}

static long wrap24(long long n)
{
    if (n >= INT_MIN24 && n <= INT_MAX24)
        return (long)n;
    return (long)(((n + 8388608LL) & 0xFFFFFFLL) - 8388608LL);
}

/* Python's repr() of a str (the worlds are ASCII) */
static void py_repr(Buf *b, const char *s, size_t n)
{
    char q = '\'';
    size_t i;
    if (memchr(s, '\'', n) && !memchr(s, '"', n))
        q = '"';
    buf_putc(b, q);
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == (unsigned char)q || c == '\\') {
            buf_putc(b, '\\');
            buf_putc(b, c);
        } else if (c == '\t') {
            buf_puts(b, "\\t");
        } else if (c == '\n') {
            buf_puts(b, "\\n");
        } else if (c == '\r') {
            buf_puts(b, "\\r");
        } else if (c < 0x20 || c == 0x7f) {
            buf_printf(b, "\\x%02x", c);
        } else {
            buf_putc(b, c);
        }
    }
    buf_putc(b, q);
}

/* Short description of a value for error messages. */
static char *show(Val v)
{
    Buf b = {0};
    if (v.t == T_INT) {
        buf_printf(&b, "int %ld", v.u.i);
    } else if (v.t == T_STR) {
        buf_puts(&b, "string ");
        py_repr(&b, v.u.s->s, v.u.s->len);
    } else {
        buf_puts(&b, type_name(v));
    }
    return b.s;
}

/* ---- lists ---- */

static void list_append(List *l, Val v)
{
    LNode *n = xmalloc(sizeof(LNode));
    n->v = v;
    n->next = NULL;
    n->dead = 0;
    if (!l->tail) {
        l->head = l->tail = n;
    } else {
        l->tail->next = n;
        l->tail = n;
    }
}

static void list_prepend(List *l, Val v)
{
    LNode *n = xmalloc(sizeof(LNode));
    n->v = v;
    n->next = l->head;
    n->dead = 0;
    l->head = n;
    if (!l->tail)
        l->tail = n;
}

/* Delete the first occurrence of v (if any).  Like the original machine,
   a deleted node keeps its 'next' pointer, so a loop over the list simply
   carries on. */
static void list_remove(List *l, Val v)
{
    LNode *prev = NULL, *n = l->head;
    while (n) {
        if (equal(n->v, v)) {
            if (!prev)
                l->head = n->next;
            else
                prev->next = n->next;
            if (n == l->tail)
                l->tail = prev;
            n->dead = 1;
            return;
        }
        prev = n;
        n = n->next;
    }
}

static int list_contains(List *l, Val v)
{
    LNode *n;
    for (n = l->head; n; n = n->next)
        if (equal(n->v, v))
            return 1;
    return 0;
}

/* ---- tables: keys compare as equal() does ---- */

static size_t val_hash(Val k)
{
    size_t h;
    size_t i;
    switch (k.t) {
    case T_INT:
        return (size_t)((uint32_t)k.u.i * 2654435761u);
    case T_STR:
        h = 2166136261u;
        for (i = 0; i < k.u.s->len; i++)
            h = (h ^ (unsigned char)k.u.s->s[i]) * 16777619u;
        return h;
    case T_NIL:
        return 17;
    case T_ABSENT:
        return 31;
    default:
        return (size_t)(((uintptr_t)k.u.l >> 4) * 2654435761u);
    }
}

static Ent *table_find(Table *t, Val k)
{
    Ent *e = t->b[val_hash(k) & (t->nb - 1)];
    for (; e; e = e->next)
        if (equal(e->k, k))
            return e;
    return NULL;
}

static void table_set(Table *t, Val k, Val v)
{
    Ent *e = table_find(t, k);
    size_t h;
    if (e) {
        e->v = v;
        return;
    }
    if (t->n + 1 > t->nb * 3 / 4) {
        size_t nb = t->nb * 2, i;
        Ent **nbk = xmalloc(nb * sizeof(Ent *));
        memset(nbk, 0, nb * sizeof(Ent *));
        for (i = 0; i < t->nb; i++) {
            Ent *x = t->b[i], *nx;
            for (; x; x = nx) {
                size_t j = val_hash(x->k) & (nb - 1);
                nx = x->next;
                x->next = nbk[j];
                nbk[j] = x;
            }
        }
        free(t->b);
        t->b = nbk;
        t->nb = nb;
    }
    e = xmalloc(sizeof(Ent));
    e->k = k;
    e->v = v;
    h = val_hash(k) & (t->nb - 1);
    e->next = t->b[h];
    t->b[h] = e;
    t->n++;
}

static void table_del(Table *t, Val k)
{
    Ent **pp = &t->b[val_hash(k) & (t->nb - 1)];
    for (; *pp; pp = &(*pp)->next) {
        if (equal((*pp)->k, k)) {
            *pp = (*pp)->next;
            t->n--;
            return;
        }
    }
}

/* ====================================================================
 *  Output formatter (the 'output' statement)
 *
 *  "Output operates somewhat like a text formatter in that sequences of
 *  nonblanks will not be broken up over a line boundary; instead, the line
 *  will be output short and the word will appear on the next line.
 *  Sequences of 2 or more blanks are treated as a unit."
 * ==================================================================== */

typedef struct {
    FILE *f;
    long width;             /* 0 = never wrap */
    int tty;                /* the console: show every line at once */
    Buf line;               /* the current output line so far */
    Buf blanks;             /* blank run seen after the last word */
    Buf word;               /* nonblank run still being collected */
} Output;

static void check_stdout(void);

static void out_emit(Output *o, const char *t, size_t n)
{
    while (n > 0 && t[n - 1] == ' ')
        n--;
    fwrite(t, 1, n, o->f);
    fputc('\n', o->f);
    if (o->tty)
        fflush(o->f);
    check_stdout();
}

static void out_end_word(Output *o)
{
    if (!o->word.n)
        return;
    if (o->line.n == 0) {
        buf_putn(&o->line, o->blanks.s, o->blanks.n);    /* leading blanks kept */
        buf_putn(&o->line, o->word.s, o->word.n);
    } else if (o->width &&
               (long)(o->line.n + o->blanks.n + o->word.n) > o->width) {
        out_emit(o, o->line.s, o->line.n);              /* break: blanks dropped */
        buf_clear(&o->line);
        buf_putn(&o->line, o->word.s, o->word.n);
    } else {
        buf_putn(&o->line, o->blanks.s, o->blanks.n);
        buf_putn(&o->line, o->word.s, o->word.n);
    }
    buf_clear(&o->word);
    buf_clear(&o->blanks);
}

static void out_end_line(Output *o)
{
    out_end_word(o);
    out_emit(o, buf_str(&o->line), o->line.n);
    buf_clear(&o->line);
    buf_clear(&o->blanks);
}

static void out_write(Output *o, const char *s, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        char c = s[i];
        if (c == '\n') {
            out_end_line(o);
        } else if (c == ' ') {
            out_end_word(o);
            buf_putc(&o->blanks, ' ');
        } else {
            buf_putc(&o->word, c);
        }
    }
}

/* Show a pending partial line (e.g. a prompt) before reading input. */
static void out_flush_partial(Output *o)
{
    out_end_word(o);
    if (o->line.n)
        fwrite(o->line.s, 1, o->line.n, o->f);
    if (o->blanks.n)
        fwrite(o->blanks.s, 1, o->blanks.n, o->f);
    buf_clear(&o->line);
    buf_clear(&o->blanks);
    fflush(o->f);
    check_stdout();
}

static void out_finish(Output *o)
{
    out_end_word(o);
    if (o->line.n) {
        out_emit(o, o->line.s, o->line.n);
        buf_clear(&o->line);
    }
    buf_clear(&o->blanks);
    fflush(o->f);
    check_stdout();
}

/* the reader of our output went away (e.g. "| head"): exit quietly */
static void check_stdout(void)
{
    if (ferror(stdout))
        _Exit(0);
}

/* ====================================================================
 *  Python's random: MT19937, seed(int) and randrange(n)
 * ==================================================================== */

#define MT_N 624
#define MT_M 397
static uint32_t mt[MT_N];
static int mti = MT_N + 1;

static void init_genrand(uint32_t s)
{
    mt[0] = s;
    for (mti = 1; mti < MT_N; mti++)
        mt[mti] = 1812433253u * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + (uint32_t)mti;
}

static void init_by_array(const uint32_t *key, size_t len)
{
    size_t i = 1, j = 0, k;
    init_genrand(19650218u);
    for (k = (MT_N > len ? MT_N : len); k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525u)) +
                key[j] + (uint32_t)j;
        i++;
        j++;
        if (i >= MT_N) {
            mt[0] = mt[MT_N - 1];
            i = 1;
        }
        if (j >= len)
            j = 0;
    }
    for (k = MT_N - 1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941u)) -
                (uint32_t)i;
        i++;
        if (i >= MT_N) {
            mt[0] = mt[MT_N - 1];
            i = 1;
        }
    }
    mt[0] = 0x80000000u;
}

static uint32_t genrand_uint32(void)
{
    static const uint32_t mag01[2] = {0x0u, 0x9908b0dfu};
    uint32_t y;
    if (mti >= MT_N) {
        int kk;
        for (kk = 0; kk < MT_N - MT_M; kk++) {
            y = (mt[kk] & 0x80000000u) | (mt[kk + 1] & 0x7fffffffu);
            mt[kk] = mt[kk + MT_M] ^ (y >> 1) ^ mag01[y & 1u];
        }
        for (; kk < MT_N - 1; kk++) {
            y = (mt[kk] & 0x80000000u) | (mt[kk + 1] & 0x7fffffffu);
            mt[kk] = mt[kk + (MT_M - MT_N)] ^ (y >> 1) ^ mag01[y & 1u];
        }
        y = (mt[MT_N - 1] & 0x80000000u) | (mt[0] & 0x7fffffffu);
        mt[MT_N - 1] = mt[MT_M - 1] ^ (y >> 1) ^ mag01[y & 1u];
        mti = 0;
    }
    y = mt[mti++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    y ^= (y >> 18);
    return y;
}

/* random.seed(n) for an int n: |n| in 32-bit words, low word first */
static void py_seed_decimal(const char *digits)
{
    uint32_t *w = xmalloc(sizeof(uint32_t) * (strlen(digits) / 9 + 2));
    size_t nw = 0, i;
    for (; *digits; digits++) {
        uint64_t carry = (uint64_t)(*digits - '0');
        for (i = 0; i < nw; i++) {
            uint64_t x = (uint64_t)w[i] * 10u + carry;
            w[i] = (uint32_t)x;
            carry = x >> 32;
        }
        if (carry)
            w[nw++] = (uint32_t)carry;
    }
    if (nw == 0)
        w[nw++] = 0;
    init_by_array(w, nw);
}

static void py_seed_random(void)
{
    uint32_t key[4];
    key[0] = (uint32_t)time(NULL);
    key[1] = (uint32_t)clock();
    key[2] = (uint32_t)GETPID();
    key[3] = (uint32_t)(uintptr_t)&key;
    init_by_array(key, 4);
}

/* randrange(n) = _randbelow(n): getrandbits(n.bit_length()) until < n */
static long py_randbelow(long n)
{
    int k = 0;
    uint32_t r;
    while ((n >> k) != 0)
        k++;
    do {
        r = genrand_uint32() >> (32 - k);
    } while ((long)r >= n);
    return (long)r;
}

/* ====================================================================
 *  Runtime state and errors
 * ==================================================================== */

static Val *G;                  /* global variable values */
static int nG, capG;
static Output OUT;
static int have_out;
static FILE *IN;
static int opt_echo;
static const char *opt_prompt = "";
static long rt_depth, max_depth = 20000;
static int rt_strict, rt_warn;
static char **soft_seen;
static int n_soft;

/* A runtime error jumps to run_world's handler.  Its line is that of the
   innermost statement being run, and its trace the procedures being run,
   innermost first - what six.py's except clauses collect.
   MinGW-w64's longjmp unwinds the stack through SEH, which can crash in
   optimised code; GCC's builtin pair only restores the registers. */
typedef void *JmpBuf[5];
#define SETJMP(b) __builtin_setjmp(b)
#define LONGJMP(b) __builtin_longjmp(b, 1)
static void **top_jmp;
enum { JMP_ERROR = 1, JMP_STOP = 2 };
static int jmp_code;
static char *err_msg;
static int err_line = -1;
static const char **err_trace;
static int n_err_trace;

static const char **pstack;     /* procedures being run */
static int psp, pcap;
static int *lstack;             /* lines of the statements being run */
static int lsp, lcap;

static void rt_error(const char *fmt, ...)
{
    Buf b = {0};
    va_list ap;
    int i;
    va_start(ap, fmt);
    buf_vprintf(&b, fmt, ap);
    va_end(ap);
    err_msg = b.s ? b.s : xstrdup("");
    err_line = lsp ? lstack[lsp - 1] : -1;
    err_trace = xmalloc(sizeof(char *) * (size_t)(psp + 1));
    n_err_trace = 0;
    for (i = psp - 1; i >= 0; i--)
        err_trace[n_err_trace++] = pstack[i];
    jmp_code = JMP_ERROR;
    LONGJMP(top_jmp);
}

/* A questionable operation.  An error in strict mode; otherwise carry on
   (optionally noting it on stderr, once per message). */
static void soft(const char *fmt, ...)
{
    Buf b = {0};
    va_list ap;
    int i;
    va_start(ap, fmt);
    buf_vprintf(&b, fmt, ap);
    va_end(ap);
    if (rt_strict)
        rt_error("%s", b.s);
    if (rt_warn) {
        for (i = 0; i < n_soft; i++)
            if (strcmp(soft_seen[i], b.s) == 0)
                return;
        soft_seen = xrealloc(soft_seen, sizeof(char *) * (size_t)(n_soft + 1));
        soft_seen[n_soft++] = b.s;
        if (have_out)
            out_flush_partial(&OUT);
        fprintf(stderr, "[six: warning: %s]\n", b.s);
    }
}

/* ====================================================================
 *  AST.  ev(node, frame) -> value; a frame holds a procedure's
 *  parameters followed by its local variables.
 * ==================================================================== */

enum {
    N_CONST, N_NEWLIST, N_NEWTABLE, N_LOCAL, N_GLOBAL, N_INDEX, N_CALL,
    N_SUBSTR, N_OR, N_AND, N_NOT, N_CMP, N_IS, N_IN, N_CAT, N_ARITH, N_NEG,
    N_DEC, N_LEN, N_RAND, N_SYSVAL, N_INPUT, N_BLOCK, N_IF, N_ASSIGN,
    N_LISTOP, N_TABDEL, N_FOR, N_WHILE, N_OUTPUT, N_MTS, N_STOP
};

enum { C_EQ, C_NE, C_LT, C_LE, C_GT, C_GE };
enum { L_APPEND, L_PREPEND, L_REMOVE };
enum { SV_CSID, SV_PROJECT, SV_TIME, SV_DATE };

struct Ast {
    int k;
    int op;                     /* CMP, ARITH ('+'...), LISTOP, SYSVAL, IS neg */
    int i;                      /* LOCAL/GLOBAL slot; INDEX: '..' */
    const char *s;              /* operator text / type name for messages */
    Val v;                      /* CONST */
    Ast *a, *b, *c;
    Ast **xs;                   /* BLOCK items, CALL args, OUTPUT exprs, IF arms */
    int *lines;                 /* BLOCK: the line of each item */
    int nx;
};

static Ast *mk(int k)
{
    Ast *n = xmalloc(sizeof(Ast));
    memset(n, 0, sizeof(Ast));
    n->k = k;
    n->v = v_nil();
    return n;
}

static void push_x(Ast *n, Ast *x)
{
    n->xs = xrealloc(n->xs, sizeof(Ast *) * (size_t)(n->nx + 1));
    n->xs[n->nx++] = x;
}

static Val ev(Ast *n, Val *fr);

static Val call_proc(Proc *p, Val *args, int nargs)
{
    Val r = v_nil();
    if (nargs != p->nparams)
        rt_error("procedure %s takes %d argument(s) but was given %d",
                 p->name, p->nparams, nargs);
    rt_depth++;
    if (rt_depth > max_depth) {
        rt_depth = 0;
        rt_error("interpreter stack overflow (recursion too deep)");
    }
    if (psp == pcap) {
        pcap = pcap ? pcap * 2 : 256;
        pstack = xrealloc(pstack, sizeof(char *) * (size_t)pcap);
    }
    pstack[psp++] = p->name;
    if (p->body) {
        int ns = p->nslots > nargs ? p->nslots : nargs, i;
        Val small[8];
        Val *fr = ns <= 8 ? small : xmalloc(sizeof(Val) * (size_t)ns);
        for (i = 0; i < nargs; i++)
            fr[i] = args[i];
        for (; i < ns; i++)
            fr[i] = v_nil();
        r = ev(p->body, fr);
        if (fr != small)
            free(fr);
        if (!p->is_func)
            r = v_nil();
    }
    psp--;
    rt_depth--;
    return r;
}

static void set_target(Ast *tg, Val *fr, Val v)
{
    if (tg->k == N_LOCAL)
        fr[tg->i] = v;
    else
        G[tg->i] = v;
}

static const char *os_user(void)
{
    static const char *names[] = {"LOGNAME", "USER", "LNAME", "USERNAME"};
    int i;
    for (i = 0; i < 4; i++) {
        const char *u = getenv(names[i]);
        if (u && *u)
            return u;
    }
    return NULL;
}

/* one line of input: universal newlines, as Python's text files read */
static Str *read_line(FILE *f)
{
    Buf b = {0};
    int c = EOF, any = 0;
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
        buf_putc(&b, c);
    }
    if (!any)
        return NULL;
    return str_new(buf_str(&b), b.n);
}

static int py_space(int c)
{
    return c == ' ' || (c >= '\t' && c <= '\r') || (c >= 0x1c && c <= 0x1f);
}

static Val ev(Ast *n, Val *fr)
{
    switch (n->k) {
    case N_CONST:
        return n->v;
    case N_NEWLIST:
        return v_list(list_new());
    case N_NEWTABLE:
        return v_table(table_new(NULL));
    case N_LOCAL:
        return fr[n->i];
    case N_GLOBAL:
        return G[n->i];
    case N_INDEX: {             /* value '.' value  |  value '..' value */
        Val t = ev(n->a, fr), k = ev(n->b, fr);
        Ent *e;
        if (t.t != T_TABLE)
            rt_error("table lookup on a non-table (%s)", show(t));
        e = table_find(t.u.tb, k);
        if (n->i) {
            if (e)
                return e->v;
            table_set(t.u.tb, k, v_nil());
            return v_nil();
        }
        return e ? e->v : v_absent();
    }
    case N_CALL: {
        Val f = ev(n->a, fr);
        Val small[8];
        Val *args = n->nx <= 8 ? small : xmalloc(sizeof(Val) * (size_t)n->nx);
        Val r;
        int i;
        for (i = 0; i < n->nx; i++)
            args[i] = ev(n->xs[i], fr);
        if (f.t != T_PROC)
            rt_error("call of a non-procedure (%s)", show(f));
        r = call_proc(f.u.p, args, n->nx);
        if (args != small)
            free(args);
        return r;
    }
    case N_SUBSTR: {
        Val s = ev(n->a, fr), a = ev(n->b, fr), b = ev(n->c, fr);
        long ai, bi;
        size_t end;
        if (s.t != T_STR || a.t != T_INT || b.t != T_INT)
            rt_error("substring needs (string(int:int)), got (%s(%s:%s))",
                     show(s), show(a), show(b));
        ai = a.u.i;
        bi = b.u.i;
        if (ai < 0) {
            bi += ai;
            ai = 0;
        }
        if (bi <= 0 || (size_t)ai >= s.u.s->len)
            return v_strn("", 0);
        end = (size_t)ai + (size_t)bi;
        if (end > s.u.s->len)
            end = s.u.s->len;
        return v_strn(s.u.s->s + ai, end - (size_t)ai);
    }
    case N_OR:
        if (truth(ev(n->a, fr)))
            return v_int(1);
        return v_int(truth(ev(n->b, fr)) ? 1 : 0);
    case N_AND:
        if (!truth(ev(n->a, fr)))
            return v_int(0);
        return v_int(truth(ev(n->b, fr)) ? 1 : 0);
    case N_NOT:
        return v_int(truth(ev(n->a, fr)) ? 0 : 1);
    case N_CMP: {
        Val a = ev(n->a, fr), b = ev(n->b, fr);
        int r = 0;
        if (n->op == C_EQ)
            return v_int(equal(a, b) ? 1 : 0);
        if (n->op == C_NE)
            return v_int(equal(a, b) ? 0 : 1);
        if (a.t == T_INT && b.t == T_INT) {
            long x = a.u.i, y = b.u.i;
            r = n->op == C_LT ? x < y : n->op == C_LE ? x <= y :
                n->op == C_GT ? x > y : x >= y;
        } else if (a.t == T_STR && b.t == T_STR) {
            size_t la = a.u.s->len, lb = b.u.s->len;
            int c = memcmp(a.u.s->s, b.u.s->s, la < lb ? la : lb);
            if (c == 0)
                c = la < lb ? -1 : la > lb ? 1 : 0;
            r = n->op == C_LT ? c < 0 : n->op == C_LE ? c <= 0 :
                n->op == C_GT ? c > 0 : c >= 0;
        } else {
            rt_error("cannot compare %s %s %s", show(a), n->s, show(b));
        }
        return v_int(r);
    }
    case N_IS: {
        int r = strcmp(type_name(ev(n->a, fr)), n->s) == 0;
        return v_int(r != n->op ? 1 : 0);
    }
    case N_IN: {
        Val a = ev(n->a, fr), b = ev(n->b, fr);
        if (b.t != T_LIST) {
            soft("right operand of 'in' is not a list (%s); treated as "
                 "'not a member'", show(b));
            return v_int(0);
        }
        return v_int(list_contains(b.u.l, a) ? 1 : 0);
    }
    case N_CAT: {
        Val a = ev(n->a, fr), b = ev(n->b, fr);
        Str *s;
        if (a.t != T_STR || b.t != T_STR)
            rt_error("'$' needs two strings, got %s and %s", show(a), show(b));
        s = xmalloc(sizeof(Str) + a.u.s->len + b.u.s->len);
        s->len = a.u.s->len + b.u.s->len;
        memcpy(s->s, a.u.s->s, a.u.s->len);
        memcpy(s->s + a.u.s->len, b.u.s->s, b.u.s->len);
        s->s[s->len] = 0;
        return v_str(s);
    }
    case N_ARITH: {
        Val a = ev(n->a, fr), b = ev(n->b, fr);
        long long x, y, r;
        if (a.t != T_INT || b.t != T_INT)
            rt_error("arithmetic '%s' needs integers, got %s and %s",
                     n->s, show(a), show(b));
        x = a.u.i;
        y = b.u.i;
        switch (n->op) {
        case '+': r = x + y; break;
        case '-': r = x - y; break;
        case '*': r = x * y; break;
        default: {
            long long q;
            if (y == 0)
                rt_error("division by zero");
            q = (x < 0 ? -x : x) / (y < 0 ? -y : y);
            if ((x < 0) != (y < 0))
                q = -q;
            r = n->op == '/' ? q : x - q * y;
        }
        }
        return v_int(wrap24(r));
    }
    case N_NEG: {
        Val a = ev(n->a, fr);
        if (a.t != T_INT)
            rt_error("unary '-' needs an integer, got %s", show(a));
        return v_int(wrap24(-(long long)a.u.i));
    }
    case N_DEC: {               /* '#' string -> integer (or nil) */
        Val s = ev(n->a, fr);
        const char *p, *e;
        long long v = 0;
        int neg = 0;
        if (s.t != T_STR)
            rt_error("'#' needs a string, got %s", show(s));
        p = s.u.s->s;
        e = p + s.u.s->len;
        if (p < e && (*p == '+' || *p == '-'))
            neg = *p++ == '-';
        if (p == e)
            return v_nil();
        for (; p < e; p++) {
            if (*p < '0' || *p > '9')
                return v_nil();
            v = v * 10 + (*p - '0');
            if (v > 100000000LL)
                v = 100000000LL;        /* out of range either way */
        }
        if (neg)
            v = -v;
        if (v < INT_MIN24 || v > INT_MAX24)
            return v_nil();
        return v_int((long)v);
    }
    case N_LEN: {
        Val s = ev(n->a, fr);
        if (s.t != T_STR)
            rt_error("'length' needs a string, got %s", show(s));
        return v_int((long)s.u.s->len);
    }
    case N_RAND:
        return v_int(py_randbelow(10000));
    case N_SYSVAL: {
        char buf[64];
        time_t t;
        struct tm *tm;
        if (n->op == SV_CSID) {
            const char *u = os_user();
            size_t i, len;
            char *up;
            if (!u)
                return v_strn("USER", 4);
            len = strlen(u);
            up = xstrndup(u, len);
            for (i = 0; i < len; i++)
                if (up[i] >= 'a' && up[i] <= 'z')
                    up[i] = (char)(up[i] - 32);
            return v_strn(up, len);
        }
        if (n->op == SV_PROJECT)
            return v_strn("SIX", 3);
        t = time(NULL);
        tm = localtime(&t);
        strftime(buf, sizeof buf, n->op == SV_TIME ? "%H:%M:%S" : "%m/%d/%y", tm);
        return v_strn(buf, strlen(buf));
    }
    case N_INPUT: {             /* next line of input as a list of words */
        Str *line;
        List *l;
        size_t i, j;
        if (have_out)
            out_flush_partial(&OUT);
        if (*opt_prompt) {
            fputs(opt_prompt, stdout);
            fflush(stdout);
            check_stdout();
        }
        line = read_line(IN);
        if (!line)
            return v_absent();
        if (opt_echo) {
            fwrite(line->s, 1, line->len, OUT.f);
            fputc('\n', OUT.f);
        }
        l = list_new();
        for (i = 0; i < line->len;) {
            while (i < line->len && py_space((unsigned char)line->s[i]))
                i++;
            if (i >= line->len)
                break;
            j = i;
            while (j < line->len && !py_space((unsigned char)line->s[j]))
                j++;
            list_append(l, v_strn(line->s + i, j - i));
            i = j;
        }
        return v_list(l);
    }
    case N_BLOCK: {
        /* statements separated by ';': the value is the last item's */
        Val v = v_nil();
        int i;
        for (i = 0; i < n->nx; i++) {
            if (lsp == lcap) {
                lcap = lcap ? lcap * 2 : 1024;
                lstack = xrealloc(lstack, sizeof(int) * (size_t)lcap);
            }
            lstack[lsp++] = n->lines[i];
            v = ev(n->xs[i], fr);
            lsp--;
        }
        return v;
    }
    case N_IF: {
        int i;
        for (i = 0; i + 1 < n->nx; i += 2)
            if (truth(ev(n->xs[i], fr)))
                return ev(n->xs[i + 1], fr);
        if (n->a)
            return ev(n->a, fr);
        return v_nil();
    }
    case N_ASSIGN: {
        Ast *tg = n->a;
        if (tg->k == N_INDEX) {
            /* the left side is evaluated first, then the right side */
            Val t = ev(tg->a, fr), k = ev(tg->b, fr), v = ev(n->b, fr);
            if (t.t != T_TABLE)
                rt_error("assignment into a non-table (%s)", show(t));
            table_set(t.u.tb, k, v);
        } else {
            set_target(tg, fr, ev(n->b, fr));
        }
        return v_nil();
    }
    case N_LISTOP: {
        Val l = ev(n->a, fr), v = ev(n->b, fr);
        if (l.t != T_LIST)
            rt_error("'%s' needs a list on the left, got %s", n->s, show(l));
        if (n->op == L_APPEND)
            list_append(l.u.l, v);
        else if (n->op == L_PREPEND)
            list_prepend(l.u.l, v);
        else
            list_remove(l.u.l, v);
        return v_nil();
    }
    case N_TABDEL: {
        Val t = ev(n->a, fr), k = ev(n->b, fr);
        if (t.t != T_TABLE)
            rt_error("'--' needs a table on the left, got %s", show(t));
        table_del(t.u.tb, k);
        return v_nil();
    }
    case N_FOR: {
        Val l = ev(n->b, fr);
        LNode *node;
        if (l.t != T_LIST)
            rt_error("'for' needs a list, got %s", show(l));
        for (node = l.u.l->head; node; node = node->next) {
            if (!node->dead) {
                set_target(n->a, fr, node->v);
                ev(n->c, fr);
            }
        }
        return v_nil();
    }
    case N_WHILE:
        while (truth(ev(n->a, fr)))
            ev(n->b, fr);
        return v_nil();
    case N_OUTPUT: {
        int i;
        for (i = 0; i < n->nx; i++) {
            Val v = ev(n->xs[i], fr);
            if (v.t == T_STR) {
                out_write(&OUT, v.u.s->s, v.u.s->len);
            } else if (v.t == T_INT) {
                char buf[32];
                int k = snprintf(buf, sizeof buf, "%ld", v.u.i);
                out_write(&OUT, buf, (size_t)k);
            } else {
                soft("output can only print strings and integers, not %s "
                     "(nothing printed)", show(v));
            }
        }
        return v_nil();
    }
    case N_MTS: {
        /* 'mts' passes its output record to MTS as a command.  There is
           no MTS here, so the command is reported on stderr and ignored. */
        Buf b = {0};
        size_t i;
        int k;
        for (k = 0; k < n->nx; k++) {
            Val v = ev(n->xs[k], fr);
            if (v.t == T_INT)
                buf_printf(&b, "%ld", v.u.i);
            else if (v.t == T_STR)
                buf_putn(&b, v.u.s->s, v.u.s->len);
            else
                rt_error("mts can only use strings and integers, not %s",
                         show(v));
        }
        for (i = 0; i < b.n; i++)
            if (b.s[i] == '\n')
                b.s[i] = ' ';
        out_flush_partial(&OUT);
        fputs("[six: 'mts' command ignored (no MTS on this system): ", stderr);
        fwrite(buf_str(&b), 1, b.n, stderr);
        fputs("]\n", stderr);
        return v_nil();
    }
    case N_STOP:
        jmp_code = JMP_STOP;
        LONGJMP(top_jmp);
    }
    return v_nil();
}

/* ====================================================================
 *  Lexer
 * ==================================================================== */

enum { K_WORD, K_INT, K_STR, K_OP, K_EOF };

typedef struct {
    int k;
    char *v;                    /* word, operator, string; INT: its digits */
    size_t vlen;                /* STR */
    uint32_t m24;               /* INT: the value mod 2**24 */
    int line;
} Tok;

static const char *world_path;

static void syntax_exit(int line, const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fputs("six: ", stderr);
    if (line >= 0)
        fprintf(stderr, "%s:%d: ", world_path, line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(2);
}

static Tok *toks;
static int ntoks, captoks;

static void add_tok(int k, const char *v, size_t vlen, uint32_t m24, int line)
{
    Tok *t;
    if (ntoks == captoks) {
        captoks = captoks ? captoks * 2 : 1024;
        toks = xrealloc(toks, sizeof(Tok) * (size_t)captoks);
    }
    t = &toks[ntoks++];
    t->k = k;
    t->v = v ? xstrndup(v, vlen) : NULL;
    t->vlen = vlen;
    t->m24 = m24;
    t->line = line;
}

static int is_alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int is_digit(int c)
{
    return c >= '0' && c <= '9';
}

static void lex(const char *src, size_t n)
{
    static const char *ops2[] = {":=", "<=", ">=", "~=", "<+", "<-", "--", ".."};
    static const char ops1[] = "<>=+-*/%$#?(),;:.";
    size_t i = 0;
    int line = 1, k;
    Tok *merged;
    int nm;
    while (i < n) {
        unsigned char c = (unsigned char)src[i];
        if (c == '\n') {
            line++;
            i++;
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') {
            i++;
        } else if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            int depth = 1, start = line;         /* comments nest */
            i += 2;
            while (i < n && depth) {
                if (i + 1 < n && src[i] == '/' && src[i + 1] == '*') {
                    depth++;
                    i += 2;
                } else if (i + 1 < n && src[i] == '*' && src[i + 1] == '/') {
                    depth--;
                    i += 2;
                } else {
                    if (src[i] == '\n')
                        line++;
                    i++;
                }
            }
            if (depth)
                syntax_exit(start, "unterminated comment");
        } else if (c == '"') {
            Buf b = {0};
            i++;
            for (;;) {
                char ch;
                if (i >= n || src[i] == '\n')
                    syntax_exit(line, "unterminated string");
                ch = src[i];
                if (ch == '"') {
                    i++;
                    break;
                }
                if (ch == '%') {
                    char nx = i + 1 < n ? src[i + 1] : 0;
                    if (nx == 'n') {
                        buf_putc(&b, '\n');
                        i += 2;
                    } else if (nx == '"') {
                        buf_putc(&b, '"');
                        i += 2;
                    } else if (nx == '%') {
                        buf_putc(&b, '%');
                        i += 2;
                    } else {                /* unknown escape: keep as is */
                        buf_putc(&b, '%');
                        i++;
                    }
                } else {
                    buf_putc(&b, ch);
                    i++;
                }
            }
            add_tok(K_STR, buf_str(&b), b.n, 0, line);
        } else if (is_alpha(c) || c == '_') {
            size_t j = i + 1;
            while (j < n && (is_alpha((unsigned char)src[j]) ||
                             is_digit((unsigned char)src[j]) || src[j] == '_'))
                j++;
            add_tok(K_WORD, src + i, j - i, 0, line);
            i = j;
        } else if (is_digit(c)) {
            size_t j = i, z;
            uint32_t m = 0;
            while (j < n && is_digit((unsigned char)src[j])) {
                m = (m * 10u + (uint32_t)(src[j] - '0')) & 0xFFFFFFu;
                j++;
            }
            for (z = i; z + 1 < j && src[z] == '0'; z++)
                ;
            add_tok(K_INT, src + z, j - z, m, line);
            i = j;
        } else {
            if (i + 3 <= n && memcmp(src + i, "<++", 3) == 0) {
                add_tok(K_OP, "<++", 3, 0, line);
                i += 3;
                continue;
            }
            for (k = 0; k < 8; k++) {
                if (i + 2 <= n && memcmp(src + i, ops2[k], 2) == 0)
                    break;
            }
            if (k < 8) {
                add_tok(K_OP, ops2[k], 2, 0, line);
                i += 2;
            } else if (c && strchr(ops1, c)) {
                add_tok(K_OP, src + i, 1, 0, line);
                i++;
            } else {
                Buf b = {0};
                py_repr(&b, src + i, 1);
                syntax_exit(line, "illegal character %s", b.s);
            }
        }
    }
    add_tok(K_EOF, NULL, 0, 0, line);

    /* The "string break" rule: if the last item on a line is a string
       constant and the first item on the next non-empty line is also a
       string constant, the two are concatenated at compile time. */
    merged = xmalloc(sizeof(Tok) * (size_t)ntoks);
    nm = 0;
    for (k = 0; k < ntoks; k++) {
        Tok *t = &toks[k];
        if (t->k == K_STR && nm && merged[nm - 1].k == K_STR &&
            merged[nm - 1].line < t->line) {
            Tok *p = &merged[nm - 1];
            char *s = xmalloc(p->vlen + t->vlen + 1);
            memcpy(s, p->v, p->vlen);
            memcpy(s + p->vlen, t->v, t->vlen);
            s[p->vlen + t->vlen] = 0;
            p->v = s;
            p->vlen += t->vlen;
            p->line = t->line;          /* so a third line keeps chaining */
        } else {
            merged[nm++] = *t;
        }
    }
    toks = merged;
    ntoks = nm;
}

/* ====================================================================
 *  Parser (builds the world and the AST in one pass, like the SIX
 *  compiler)
 * ==================================================================== */

static const char *RESERVED[] = {
    "cons", "var", "thing", "proc", "verb", "noun", "start", "corp",
    "result", "for", "in", "do", "od", "while", "if", "then", "elif",
    "else", "fi", "output", "mts", "stop", "and", "or", "not", "is", "isnt",
    "nil", "absent", "emptylist", "emptytable", "input", "csid", "project",
    "time", "date", "length", NULL
};
static const char *ENTITY_KW[] = {"cons", "var", "thing", "proc", "verb",
                                  "start", NULL};
static const char *ENTITY_END[] = {"cons", "var", "thing", "proc", "verb",
                                   "start", "noun", NULL};
static const char *TYPES[] = {"int", "string", "nil", "absent", "prop",
                              "list", "proc", "table", NULL};

static int in_set(const char *w, const char **set)
{
    for (; *set; set++)
        if (strcmp(w, *set) == 0)
            return 1;
    return 0;
}

enum { S_VAR, S_CONST, S_TABLE, S_PROC, S_PROP };

typedef struct Sym {
    char *name;
    int kind;
    Val val;
    int gidx;                   /* S_VAR */
    struct Sym *next;
} Sym;

#define NSYMB 8192
static Sym *symtab[NSYMB];

static size_t str_hash(const char *s)
{
    size_t h = 2166136261u;
    for (; *s; s++)
        h = (h ^ (unsigned char)*s) * 16777619u;
    return h;
}

static Sym *sym_get(const char *name)
{
    Sym *s = symtab[str_hash(name) & (NSYMB - 1)];
    for (; s; s = s->next)
        if (strcmp(s->name, name) == 0)
            return s;
    return NULL;
}

static Sym *define(const char *name, int kind, Val val)
{
    Sym *s = sym_get(name);
    if (!s) {
        size_t h = str_hash(name) & (NSYMB - 1);
        s = xmalloc(sizeof(Sym));
        s->name = xstrdup(name);
        s->next = symtab[h];
        symtab[h] = s;
    }
    s->kind = kind;
    s->val = val;
    s->gidx = 0;
    return s;
}

static int P;                   /* the parser's token index */
static char **loc_names;        /* the current proc's locals: a dict of */
static int *loc_idx;            /* name -> slot, as six.py keeps them */
static int nloc;
static int in_proc;
static int opt_strict_parse, opt_fix_typos;
static Table *DICT;
static Proc *start_proc;
static char **warnings;
static int nwarnings;
static int undeclared_count;
static int cnt_cons, cnt_var, cnt_thing, cnt_verb, cnt_proc, cnt_noun;

static Tok *peek(void)
{
    return &toks[P];
}

static Tok *next_tok(void)
{
    return &toks[P++];
}

static int at_op(const char *o)
{
    Tok *t = &toks[P];
    return t->k == K_OP && strcmp(t->v, o) == 0;
}

static int at_word(const char *w)
{
    Tok *t = &toks[P];
    return t->k == K_WORD && strcmp(t->v, w) == 0;
}

static char *describe(Tok *t)
{
    Buf b = {0};
    if (t->k == K_EOF)
        return xstrdup("end of file");
    if (t->k == K_STR) {
        buf_puts(&b, "string ");
        py_repr(&b, t->v, t->vlen);
        return b.s;
    }
    if (t->k == K_INT)
        return xstrdup(t->v);
    py_repr(&b, t->v, strlen(t->v));
    return b.s;
}

static void perr_at(Tok *tok, const char *fmt, ...)
{
    Buf b = {0};
    va_list ap;
    va_start(ap, fmt);
    buf_vprintf(&b, fmt, ap);
    va_end(ap);
    syntax_exit((tok ? tok : peek())->line, "%s", b.s);
}

static void syntax(const char *expected)
{
    Tok *t = peek();
    perr_at(t, "syntax error: expected %s but found %s", expected, describe(t));
}

static void expect_op(const char *o)
{
    Buf b = {0};
    if (!at_op(o)) {
        buf_printf(&b, "'%s'", o);
        syntax(b.s);
    }
    P++;
}

static void expect_word(const char *w)
{
    Buf b = {0};
    if (!at_word(w)) {
        buf_printf(&b, "'%s'", w);
        syntax(b.s);
    }
    P++;
}

static const char *expect_ident(void)
{
    Tok *t = peek();
    if (t->k != K_WORD || in_set(t->v, RESERVED))
        syntax("an identifier");
    P++;
    return t->v;
}

static void warn_at(Tok *tok, const char *fmt, ...)
{
    Buf b = {0};
    va_list ap;
    buf_printf(&b, "line %d: ", (tok ? tok : peek())->line);
    va_start(ap, fmt);
    buf_vprintf(&b, fmt, ap);
    va_end(ap);
    warnings = xrealloc(warnings, sizeof(char *) * (size_t)(nwarnings + 1));
    warnings[nwarnings++] = b.s;
}

static int is_term(const char **terms)
{
    Tok *t = peek();
    if (t->k == K_EOF)
        return 1;
    return t->k == K_WORD && in_set(t->v, terms);
}

static void new_global(const char *name)
{
    Sym *s;
    if (nG == capG) {
        capG = capG ? capG * 2 : 256;
        G = xrealloc(G, sizeof(Val) * (size_t)capG);
    }
    G[nG++] = v_nil();
    s = define(name, S_VAR, v_nil());
    s->gidx = nG - 1;
}

static int str_cmp(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* Declared names that an undeclared word is probably a typo of: names one
   edit away (insert / delete / replace / swap), plus the word with a
   leading '_' added or removed.  Sorted. */
static char **suggest(const char *name, int *count)
{
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
    size_t L = strlen(name), i, a;
    char **out = NULL;
    int n = 0, k, m;
    char *buf = xmalloc(L + 3);

#define CAND(str) do { \
        const char *cs_ = (str); \
        Sym *sy_ = sym_get(cs_); \
        if (sy_ && strcmp(cs_, name) != 0 && !in_set(cs_, RESERVED)) { \
            out = xrealloc(out, sizeof(char *) * (size_t)(n + 1)); \
            out[n++] = xstrdup(cs_); \
        } \
    } while (0)

    if (name[0] == '_')
        CAND(name + 1);
    buf[0] = '_';
    memcpy(buf + 1, name, L + 1);
    CAND(buf);
    for (i = 0; i <= L; i++) {
        const char *right = name + i;
        size_t rl = L - i;
        if (rl) {                               /* delete */
            memcpy(buf, name, i);
            memcpy(buf + i, right + 1, rl - 1);
            buf[L - 1] = 0;
            CAND(buf);
        }
        if (rl > 1) {                           /* swap */
            memcpy(buf, name, i);
            buf[i] = right[1];
            buf[i + 1] = right[0];
            memcpy(buf + i + 2, right + 2, rl - 2);
            buf[L] = 0;
            CAND(buf);
        }
        for (a = 0; alphabet[a]; a++) {
            memcpy(buf, name, i);               /* insert */
            buf[i] = alphabet[a];
            memcpy(buf + i + 1, right, rl);
            buf[L + 1] = 0;
            CAND(buf);
            if (rl) {                           /* replace */
                memcpy(buf, name, i);
                buf[i] = alphabet[a];
                memcpy(buf + i + 1, right + 1, rl - 1);
                buf[L] = 0;
                CAND(buf);
            }
        }
    }
#undef CAND
    qsort(out, (size_t)n, sizeof(char *), str_cmp);
    for (k = 0, m = 0; k < n; k++)             /* a set: no duplicates */
        if (m == 0 || strcmp(out[m - 1], out[k]) != 0)
            out[m++] = out[k];
    *count = m;
    return out;
}

/* A word that was never declared.  kinds: a mask of 1 << S_... */
static Sym *undeclared(const char *name, Tok *tok, int kinds)
{
    int nc, i, n = 0;
    char **all = suggest(name, &nc);
    char **cands = xmalloc(sizeof(char *) * (size_t)(nc + 1));
    Buf hint = {0};
    for (i = 0; i < nc; i++)
        if (kinds & (1 << sym_get(all[i])->kind))
            cands[n++] = all[i];
    if (opt_fix_typos && n == 1) {
        warn_at(tok, "undeclared word '%s' taken to mean '%s'", name, cands[0]);
        return sym_get(cands[0]);
    }
    if (n) {
        buf_puts(&hint, " (did you mean ");
        for (i = 0; i < n && i < 3; i++)
            buf_printf(&hint, "%s'%s'", i ? " or " : "", cands[i]);
        buf_puts(&hint, "? try --fix-typos)");
    }
    warn_at(tok, "undeclared word '%s' treated as a new property%s", name,
            buf_str(&hint));
    undeclared_count++;
    return define(name, S_PROP, v_prop(prop_new(name)));
}

/* Value of a word used as a table index / noun / list element. */
static Val word_value(const char *name, int create)
{
    Sym *s = sym_get(name);
    if (!s) {
        if (!create)
            perr_at(NULL, "undefined identifier '%s'", name);
        return define(name, S_PROP, v_prop(prop_new(name)))->val;
    }
    if (s->kind == S_VAR)
        perr_at(NULL, "'%s' is a variable, not a constant/thing/proc/prop", name);
    return s->val;
}

static long parse_signed_int(void)
{
    int neg = 0;
    Tok *t;
    if (at_op("-")) {
        neg = 1;
        P++;
    } else if (at_op("+")) {
        P++;
    }
    t = peek();
    if (t->k != K_INT)
        syntax("an integer");
    P++;
    return wrap24(neg ? -(long long)t->m24 : (long long)t->m24);
}

static Val parse_cval(const char *name)
{
    Tok *t = peek();
    Sym *s;
    if (t->k == K_STR) {
        P++;
        return v_strn(t->v, t->vlen);
    }
    if (t->k == K_INT || (t->k == K_OP && (strcmp(t->v, "+") == 0 ||
                                           strcmp(t->v, "-") == 0)))
        return v_int(parse_signed_int());
    if (t->k == K_WORD && strcmp(t->v, "prop") == 0) {
        P++;
        return v_prop(prop_new(name));
    }
    if (t->k == K_WORD && (s = sym_get(t->v)) != NULL && s->kind == S_CONST) {
        P++;
        return s->val;
    }
    syntax("an integer, a string or 'prop'");
    return v_nil();
}

static void entity_cons(void)
{
    P++;
    for (;;) {
        const char *name = expect_ident();
        Val v;
        expect_op("=");
        v = parse_cval(name);
        define(name, S_CONST, v);
        cnt_cons++;
        if (at_op(",")) {
            P++;
            continue;
        }
        break;
    }
}

static void entity_var(void)
{
    P++;
    for (;;) {
        const char *name = expect_ident();
        Sym *s = sym_get(name);
        if (!s || s->kind != S_VAR) {
            if (s)
                warn_at(NULL, "'%s' redeclared as a variable", name);
            new_global(name);
            cnt_var++;
        }
        if (at_op(",")) {
            P++;
            continue;
        }
        break;
    }
}

/* word lists: word | string | '(' (word|string) {',' (word|string)} ')' */
typedef struct {
    int word;                   /* 1 word, 0 string */
    char *text;
    size_t len;
} Name;

static Name parse_name(void)
{
    Tok *t = next_tok();
    Name nm;
    if (t->k == K_WORD) {
        nm.word = 1;
        nm.text = t->v;
        nm.len = strlen(t->v);
        return nm;
    }
    if (t->k == K_STR) {
        nm.word = 0;
        nm.text = t->v;
        nm.len = t->vlen;
        return nm;
    }
    P--;
    syntax("a word or string");
    return nm;
}

static Name *parse_wordlist(int *count)
{
    Name *names = xmalloc(sizeof(Name));
    int n = 0;
    if (at_op("(")) {
        P++;
        names[n++] = parse_name();
        while (at_op(",")) {
            P++;
            names = xrealloc(names, sizeof(Name) * (size_t)(n + 1));
            names[n++] = parse_name();
        }
        expect_op(")");
    } else {
        names[n++] = parse_name();
    }
    *count = n;
    return names;
}

/* Create (or find, if pre-declared) the table for a thing/verb and enter
   every synonym in the main dictionary. */
static Table *declare_table(Name *names, int n, const char *what)
{
    Table *tbl = NULL;
    int i;
    for (i = 0; i < n; i++) {
        if (names[i].word) {
            Sym *s = sym_get(names[i].text);
            if (s) {
                if (s->kind != S_TABLE)
                    perr_at(NULL, "'%s' is already declared and cannot be "
                            "used as a %s name", names[i].text, what);
                if (!tbl)
                    tbl = s->val.u.tb;
                else if (s->val.u.tb != tbl)
                    warn_at(NULL, "'%s' already names a different table; it "
                            "now names this %s", names[i].text, what);
            }
        }
    }
    if (!tbl)
        tbl = table_new(xstrndup(names[0].text, names[0].len));
    for (i = 0; i < n; i++) {
        table_set(DICT, v_strn(names[i].text, names[i].len), v_table(tbl));
        if (names[i].word && !in_set(names[i].text, RESERVED))
            define(names[i].text, S_TABLE, v_table(tbl));
    }
    return tbl;
}

static int at_entry_end(void)
{
    Tok *t = peek();
    if (t->k == K_EOF)
        return 1;
    if (t->k == K_OP && (strcmp(t->v, ",") == 0 || strcmp(t->v, ";") == 0))
        return 1;
    return t->k == K_WORD && in_set(t->v, ENTITY_KW);
}

static Val parse_tindex(void)
{
    Tok *t = peek();
    if (t->k == K_WORD) {
        P++;
        return word_value(t->v, 1);
    }
    if (t->k == K_STR) {
        P++;
        return v_strn(t->v, t->vlen);
    }
    if (t->k == K_INT || (t->k == K_OP && (strcmp(t->v, "+") == 0 ||
                                           strcmp(t->v, "-") == 0)))
        return v_int(parse_signed_int());
    syntax("a table index (word, integer or string)");
    return v_nil();
}

static Val parse_tentry(int in_list)
{
    Tok *t = peek();
    if (t->k == K_STR) {
        P++;
        return v_strn(t->v, t->vlen);
    }
    if (t->k == K_INT || (t->k == K_OP && (strcmp(t->v, "+") == 0 ||
                                           strcmp(t->v, "-") == 0)))
        return v_int(parse_signed_int());
    if (t->k == K_OP && strcmp(t->v, "(") == 0 && !in_list) {
        List *l = list_new();
        P++;
        if (!at_op(")")) {
            for (;;) {
                Val v = parse_tentry(1);
                if (!list_contains(l, v))       /* duplicates are ignored */
                    list_append(l, v);
                if (at_op(",")) {
                    P++;
                    continue;
                }
                break;
            }
        }
        expect_op(")");
        return v_list(l);
    }
    if (t->k == K_WORD) {
        P++;
        if (strcmp(t->v, "nil") == 0)
            return v_nil();
        if (strcmp(t->v, "emptylist") == 0)
            return v_list(list_new());
        if (strcmp(t->v, "emptytable") == 0)
            return v_table(table_new(NULL));
        if (strcmp(t->v, "absent") == 0)
            return v_absent();
        if (!sym_get(t->v) && !opt_strict_parse)
            return undeclared(t->v, t, (1 << S_TABLE) | (1 << S_PROC) |
                              (1 << S_CONST) | (1 << S_PROP))->val;
        return word_value(t->v, 0);
    }
    syntax("a table entry");
    return v_nil();
}

typedef struct {
    int isint, isstr;
    long i;
    char *s;
    size_t len;
} Seen;

static void entity_thing(void)
{
    int n, ns = 0, i;
    Name *names;
    Table *tbl;
    Seen *seen = NULL;
    P++;
    names = parse_wordlist(&n);
    expect_op(":");
    tbl = declare_table(names, n, "thing");
    cnt_thing++;
    if (at_entry_end() && !at_op(","))
        return;                                 /* pre-declaration */
    if (at_op("*")) {
        P++;
        return;
    }
    for (;;) {
        Val key = parse_tindex(), val;
        int dup = 0;
        if (at_entry_end())
            val = v_nil();
        else
            val = parse_tentry(0);
        if (key.t == T_INT || key.t == T_STR) {
            for (i = 0; i < ns; i++) {
                if (key.t == T_INT && seen[i].isint && seen[i].i == key.u.i)
                    dup = 1;
                if (key.t == T_STR && seen[i].isstr &&
                    seen[i].len == key.u.s->len &&
                    memcmp(seen[i].s, key.u.s->s, seen[i].len) == 0)
                    dup = 1;
            }
            if (dup) {
                Buf b = {0};
                if (key.t == T_INT)
                    buf_printf(&b, "%ld", key.u.i);
                else
                    py_repr(&b, key.u.s->s, key.u.s->len);
                warn_at(NULL, "duplicate index %s in thing '%s'", b.s,
                        names[0].text);
            } else {
                seen = xrealloc(seen, sizeof(Seen) * (size_t)(ns + 1));
                seen[ns].isint = key.t == T_INT;
                seen[ns].isstr = key.t == T_STR;
                seen[ns].i = key.t == T_INT ? key.u.i : 0;
                seen[ns].s = key.t == T_STR ? key.u.s->s : NULL;
                seen[ns].len = key.t == T_STR ? key.u.s->len : 0;
                ns++;
            }
        }
        table_set(tbl, key, val);
        if (at_op(",")) {
            P++;
            continue;
        }
        break;
    }
}

static Ast *parse_block(const char **terms);

static void entity_verb(void)
{
    int n, i;
    Name *names;
    Table *tbl;
    const char *vname;
    P++;
    names = parse_wordlist(&n);
    expect_op(":");
    tbl = declare_table(names, n, "verb");
    cnt_verb++;
    vname = names[0].text;
    if (!at_word("noun"))
        syntax("'noun'");
    while (at_word("noun")) {
        Val *idxs;
        int nidx = 0;
        const char *label;
        Buf pname = {0};
        Proc *proc;
        P++;
        cnt_noun++;
        if (at_op("*")) {
            P++;
            idxs = xmalloc(sizeof(Val));
            idxs[nidx++] = v_int(1);
            label = "*";
        } else if (at_op(":")) {
            idxs = xmalloc(sizeof(Val));
            idxs[nidx++] = v_nil();
            label = "";
        } else {
            int m;
            Name *lst = parse_wordlist(&m);
            idxs = xmalloc(sizeof(Val) * (size_t)m);
            for (i = 0; i < m; i++) {
                if (!lst[i].word)
                    idxs[nidx++] = v_strn(lst[i].text, lst[i].len);
                else
                    idxs[nidx++] = word_value(lst[i].text, 1);
            }
            label = lst[0].text;
        }
        expect_op(":");
        buf_printf(&pname, "%s/%s", vname, *label ? label : "(none)");
        proc = proc_new(buf_str(&pname));
        loc_names = NULL;
        nloc = 0;
        in_proc = 0;
        proc->body = parse_block(ENTITY_END);
        for (i = 0; i < nidx; i++)
            table_set(tbl, idxs[i], v_proc(proc));
    }
}

static void entity_start(void)
{
    Tok *tok = next_tok();
    Proc *proc;
    expect_op(":");
    if (start_proc)
        perr_at(tok, "more than one 'start' entity");
    proc = proc_new("start");
    loc_names = NULL;
    nloc = 0;
    in_proc = 0;
    proc->body = parse_block(ENTITY_END);
    start_proc = proc;
}

static int local_find(const char *name)
{
    int i;
    for (i = 0; i < nloc; i++)
        if (strcmp(loc_names[i], name) == 0)
            return i;
    return -1;
}

static int local_index(const char *name)
{
    int k;
    if (!in_proc)
        return -1;
    k = local_find(name);
    return k < 0 ? -1 : loc_idx[k];
}

/* locals[name] = len(locals): a name seen again gets the next slot */
static void set_local(const char *name)
{
    int k = local_find(name);
    if (k >= 0) {
        loc_idx[k] = nloc;
        return;
    }
    loc_names = xrealloc(loc_names, sizeof(char *) * (size_t)(nloc + 1));
    loc_idx = xrealloc(loc_idx, sizeof(int) * (size_t)(nloc + 1));
    loc_names[nloc] = (char *)name;
    loc_idx[nloc] = nloc;
    nloc++;
}

static void entity_proc(void)
{
    static const char *corp_terms[] = {"corp", NULL};
    const char *name;
    const char **params = NULL;
    int nparams = 0, is_func = 0, i;
    Sym *s;
    Proc *proc;
    Ast *body;
    P++;
    name = expect_ident();
    expect_op("(");
    if (!at_op(")")) {
        for (;;) {
            params = xrealloc(params, sizeof(char *) * (size_t)(nparams + 1));
            params[nparams++] = expect_ident();
            if (at_op(",")) {
                P++;
                continue;
            }
            break;
        }
    }
    expect_op(")");
    if (at_word("result")) {
        P++;
        is_func = 1;
    }
    expect_op(":");
    s = sym_get(name);
    if (!s) {
        proc = proc_new(name);
        define(name, S_PROC, v_proc(proc));
    } else if (s->kind == S_PROC) {
        proc = s->val.u.p;                      /* fill in a pre-declared proc */
    } else {
        perr_at(NULL, "'%s' is already declared and cannot be a procedure", name);
        return;
    }
    cnt_proc++;

    loc_names = NULL;
    loc_idx = NULL;
    nloc = 0;
    in_proc = 1;
    for (i = 0; i < nparams; i++)
        set_local(params[i]);
    if (at_word("var")) {
        P++;
        for (;;) {
            const char *v = expect_ident();
            if (local_find(v) < 0)
                set_local(v);
            if (at_op(",")) {
                P++;
                continue;
            }
            break;
        }
        expect_op(";");
    }
    body = parse_block(corp_terms);
    expect_word("corp");
    proc->nparams = nparams;
    proc->nslots = nloc;
    proc->is_func = is_func;
    proc->body = body->nx ? body : NULL;
    loc_names = NULL;
    nloc = 0;
    in_proc = 0;
}

/* -- statements -- */

/* Both sample worlds contain an occasional unmatched 'fi' (e.g. "fi;
   corp;").  The original compiler let these through, so an unmatched 'fi'
   or 'od' is ignored (with a warning). */
static void skip_stray(const char **terms)
{
    Tok *t = peek();
    while (t->k == K_WORD && (strcmp(t->v, "fi") == 0 || strcmp(t->v, "od") == 0)
           && !in_set(t->v, terms)) {
        warn_at(t, "unmatched '%s' ignored", t->v);
        P++;
        t = peek();
    }
}

static Ast *parse_item(void);

static Ast *parse_block(const char **terms)
{
    Ast *blk = mk(N_BLOCK);
    for (;;) {
        Tok *t;
        skip_stray(terms);
        t = peek();
        if (!(is_term(terms) || (t->k == K_OP && strcmp(t->v, ";") == 0))) {
            int line = t->line;
            Ast *node = parse_item();
            push_x(blk, node);
            blk->lines = xrealloc(blk->lines, sizeof(int) * (size_t)blk->nx);
            blk->lines[blk->nx - 1] = line;
            skip_stray(terms);
        }
        if (at_op(";")) {
            P++;
            continue;
        }
        break;
    }
    if (!is_term(terms))
        syntax("';' or the end of the statement list");
    return blk;
}

static int at_stmt_end(void)
{
    static const char *ends[] = {"fi", "od", "else", "elif", "corp", "do",
                                 "then", NULL};
    Tok *t = peek();
    if (t->k == K_EOF)
        return 1;
    if (t->k == K_OP)
        return strcmp(t->v, ";") == 0;
    return t->k == K_WORD && (in_set(t->v, ENTITY_END) || in_set(t->v, ends));
}

static Ast *parse_expr(void);

static void parse_exprlist(Ast *n)
{
    if (at_stmt_end())
        return;
    push_x(n, parse_expr());
    while (at_op(",")) {
        P++;
        while (at_op(",")) {            /* tolerate ", ," (empty slot) */
            warn_at(NULL, "empty expression in list ignored");
            P++;
        }
        push_x(n, parse_expr());
    }
}

static Ast *ident(const char *name, Tok *tok, int want_var);

static Ast *parse_for(void)
{
    static const char *od_terms[] = {"od", NULL};
    Ast *n = mk(N_FOR);
    Tok *t;
    const char *name;
    P++;
    t = peek();
    name = expect_ident();
    n->a = ident(name, t, 1);
    expect_word("in");
    n->b = parse_expr();
    expect_word("do");
    n->c = parse_block(od_terms);
    expect_word("od");
    return n;
}

static Ast *as_target(Ast *e, Tok *tok)
{
    if (e->k == N_LOCAL || e->k == N_GLOBAL || e->k == N_INDEX)
        return e;
    perr_at(tok, "the left side of ':=' must be a variable or a table entry");
    return e;
}

static Ast *parse_item(void)
{
    static const char *do_terms[] = {"do", NULL};
    static const char *od_terms[] = {"od", NULL};
    Tok *t = peek();
    Ast *e;
    if (t->k == K_WORD) {
        const char *w = t->v;
        if (strcmp(w, "for") == 0)
            return parse_for();
        if (strcmp(w, "while") == 0) {
            Ast *n = mk(N_WHILE);
            P++;
            n->a = parse_block(do_terms);
            if (!n->a->nx)
                perr_at(t, "'while' needs a condition");
            expect_word("do");
            n->b = parse_block(od_terms);
            expect_word("od");
            return n;
        }
        if (strcmp(w, "output") == 0) {
            Ast *n = mk(N_OUTPUT);
            P++;
            parse_exprlist(n);
            return n;
        }
        if (strcmp(w, "mts") == 0) {
            Ast *n = mk(N_MTS);
            P++;
            parse_exprlist(n);
            return n;
        }
        if (strcmp(w, "stop") == 0) {
            P++;
            return mk(N_STOP);
        }
    }
    e = parse_expr();
    t = peek();
    if (t->k == K_OP) {
        if (strcmp(t->v, ":=") == 0) {
            Ast *n = mk(N_ASSIGN);
            P++;
            n->b = parse_expr();
            n->a = as_target(e, t);
            return n;
        }
        if (strcmp(t->v, "<+") == 0 || strcmp(t->v, "<++") == 0 ||
            strcmp(t->v, "<-") == 0) {
            Ast *n = mk(N_LISTOP);
            P++;
            n->op = strcmp(t->v, "<+") == 0 ? L_APPEND :
                    strcmp(t->v, "<++") == 0 ? L_PREPEND : L_REMOVE;
            n->s = t->v;
            n->a = e;
            n->b = parse_expr();
            return n;
        }
        if (strcmp(t->v, "--") == 0) {
            Ast *n = mk(N_TABDEL);
            P++;
            n->a = e;
            n->b = parse_expr();
            return n;
        }
    }
    return e;
}

static Ast *parse_if(void)
{
    static const char *arm_terms[] = {"elif", "else", "fi", NULL};
    static const char *fi_terms[] = {"fi", NULL};
    Ast *n = mk(N_IF);
    P++;
    push_x(n, parse_expr());
    expect_word("then");
    push_x(n, parse_block(arm_terms));
    while (at_word("elif")) {
        P++;
        push_x(n, parse_expr());
        expect_word("then");
        push_x(n, parse_block(arm_terms));
    }
    if (at_word("else")) {
        P++;
        n->a = parse_block(fi_terms);
    }
    expect_word("fi");
    return n;
}

/* -- expressions -- */

static Ast *bin(int k, Ast *a, Ast *b)
{
    Ast *n = mk(k);
    n->a = a;
    n->b = b;
    return n;
}

static Ast *parse_and(void);
static Ast *parse_not(void);
static Ast *parse_bool(void);
static Ast *parse_cat(void);
static Ast *parse_add(void);
static Ast *parse_mul(void);
static Ast *parse_um(void);
static Ast *parse_source(void);
static Ast *parse_value(void);

static Ast *parse_expr(void)
{
    Ast *left = parse_and();
    while (at_word("or")) {
        P++;
        left = bin(N_OR, left, parse_and());
    }
    return left;
}

static Ast *parse_and(void)
{
    Ast *left = parse_not();
    while (at_word("and")) {
        P++;
        left = bin(N_AND, left, parse_not());
    }
    return left;
}

static Ast *parse_not(void)
{
    if (at_word("not")) {
        Ast *n = mk(N_NOT);
        P++;
        n->a = parse_not();
        return n;
    }
    return parse_bool();
}

static Ast *parse_bool(void)
{
    static const char *cmp[] = {"=", "~=", "<", "<=", ">", ">=", NULL};
    static const int cmpc[] = {C_EQ, C_NE, C_LT, C_LE, C_GT, C_GE};
    Ast *left = parse_cat();
    Tok *t = peek();
    int i;
    if (t->k == K_OP) {
        for (i = 0; cmp[i]; i++) {
            if (strcmp(t->v, cmp[i]) == 0) {
                Ast *n;
                P++;
                n = bin(N_CMP, left, parse_cat());
                n->op = cmpc[i];
                n->s = cmp[i];
                return n;
            }
        }
    }
    if (t->k == K_WORD) {
        if (strcmp(t->v, "is") == 0 || strcmp(t->v, "isnt") == 0) {
            Tok *ty;
            Ast *n = mk(N_IS);
            P++;
            ty = next_tok();
            if (ty->k != K_WORD || !in_set(ty->v, TYPES))
                perr_at(ty, "syntax error: a type (int, string, nil, absent, "
                        "prop, list, proc, table) must follow '%s'", t->v);
            n->a = left;
            n->s = ty->v;
            n->op = strcmp(t->v, "isnt") == 0;
            return n;
        }
        if (strcmp(t->v, "in") == 0) {
            P++;
            return bin(N_IN, left, parse_cat());
        }
    }
    return left;
}

static Ast *parse_cat(void)
{
    Ast *left = parse_add();
    while (at_op("$")) {
        P++;
        left = bin(N_CAT, left, parse_add());
    }
    return left;
}

static Ast *arith(const char *op, Ast *a, Ast *b)
{
    Ast *n = bin(N_ARITH, a, b);
    n->op = op[0];
    n->s = op;
    return n;
}

static Ast *parse_add(void)
{
    Ast *left = parse_mul();
    for (;;) {
        Tok *t = peek();
        if (t->k == K_OP && (strcmp(t->v, "+") == 0 || strcmp(t->v, "-") == 0)) {
            P++;
            left = arith(t->v, left, parse_mul());
        } else {
            return left;
        }
    }
}

static Ast *parse_mul(void)
{
    Ast *left = parse_um();
    for (;;) {
        Tok *t = peek();
        if (t->k == K_OP && (strcmp(t->v, "*") == 0 || strcmp(t->v, "/") == 0 ||
                             strcmp(t->v, "%") == 0)) {
            P++;
            left = arith(t->v, left, parse_um());
        } else {
            return left;
        }
    }
}

static Ast *negate(Ast *e)
{
    Ast *n;
    if (e->k == N_CONST && e->v.t == T_INT) {
        n = mk(N_CONST);
        n->v = v_int(wrap24(-(long long)e->v.u.i));
        return n;
    }
    n = mk(N_NEG);
    n->a = e;
    return n;
}

static Ast *parse_um(void)
{
    Tok *t = peek();
    if (t->k == K_OP) {
        if (strcmp(t->v, "-") == 0) {
            P++;
            return negate(parse_um());
        }
        if (strcmp(t->v, "+") == 0) {
            P++;
            return parse_um();
        }
        if (strcmp(t->v, "#") == 0) {
            Ast *n = mk(N_DEC);
            P++;
            n->a = parse_source();
            return n;
        }
    } else if (t->k == K_WORD && strcmp(t->v, "length") == 0) {
        Ast *n = mk(N_LEN);
        P++;
        n->a = parse_source();
        return n;
    }
    return parse_source();
}

static Ast *parse_source(void)
{
    Ast *e = parse_value();
    for (;;) {
        Tok *t = peek();
        if (t->k != K_OP)
            return e;
        if (strcmp(t->v, ".") == 0 || strcmp(t->v, "..") == 0) {
            Ast *n = mk(N_INDEX);
            P++;
            n->a = e;
            n->b = parse_value();
            n->i = t->v[1] == '.';
            e = n;
        } else if (strcmp(t->v, "(") == 0) {
            Ast *a;
            P++;
            if (at_op(")")) {
                Ast *n = mk(N_CALL);
                P++;
                n->a = e;
                e = n;
                continue;
            }
            a = parse_expr();
            if (at_op(":")) {
                Ast *n = mk(N_SUBSTR);
                P++;
                n->a = e;
                n->b = a;
                n->c = parse_expr();
                expect_op(")");
                e = n;
            } else {
                Ast *n = mk(N_CALL);
                n->a = e;
                push_x(n, a);
                while (at_op(",")) {
                    P++;
                    push_x(n, parse_expr());
                }
                expect_op(")");
                e = n;
            }
        } else {
            return e;
        }
    }
}

static Ast *cnst(Val v)
{
    Ast *n = mk(N_CONST);
    n->v = v;
    return n;
}

static Ast *parse_value(void)
{
    Tok *t = next_tok();
    if (t->k == K_INT)
        return cnst(v_int(wrap24((long long)t->m24)));
    if (t->k == K_STR)
        return cnst(v_strn(t->v, t->vlen));
    if (t->k == K_OP) {
        if (strcmp(t->v, "(") == 0) {
            Ast *e = parse_expr();
            expect_op(")");
            return e;
        }
        if (strcmp(t->v, "?") == 0)
            return mk(N_RAND);
        if (strcmp(t->v, "-") == 0)
            return negate(parse_value());
    } else if (t->k == K_WORD) {
        const char *w = t->v;
        if (strcmp(w, "nil") == 0)
            return cnst(v_nil());
        if (strcmp(w, "absent") == 0)
            return cnst(v_absent());
        if (strcmp(w, "emptylist") == 0)
            return mk(N_NEWLIST);
        if (strcmp(w, "emptytable") == 0)
            return mk(N_NEWTABLE);
        if (strcmp(w, "input") == 0)
            return mk(N_INPUT);
        if (strcmp(w, "csid") == 0 || strcmp(w, "project") == 0 ||
            strcmp(w, "time") == 0 || strcmp(w, "date") == 0) {
            Ast *n = mk(N_SYSVAL);
            n->op = w[0] == 'c' ? SV_CSID : w[0] == 'p' ? SV_PROJECT :
                    w[0] == 't' ? SV_TIME : SV_DATE;
            return n;
        }
        if (strcmp(w, "if") == 0) {
            P--;
            return parse_if();
        }
        if (in_set(w, RESERVED)) {
            P--;
            syntax("an expression");
        }
        return ident(w, t, 0);
    }
    P--;
    syntax("an expression");
    return NULL;
}

static Ast *ident(const char *name, Tok *tok, int want_var)
{
    int li = local_index(name);
    Sym *s;
    Ast *n;
    if (li >= 0) {
        n = mk(N_LOCAL);
        n->i = li;
        return n;
    }
    s = sym_get(name);
    if (!s) {
        if (opt_strict_parse || want_var)
            perr_at(tok, "undefined identifier '%s'", name);
        /* Like an unused table index, an undeclared word becomes a new,
           unique property (the original compiler evidently allowed it). */
        s = undeclared(name, tok, (1 << S_VAR) | (1 << S_TABLE) | (1 << S_PROC) |
                       (1 << S_CONST) | (1 << S_PROP));
    }
    if (s->kind == S_VAR) {
        n = mk(N_GLOBAL);
        n->i = s->gidx;
        return n;
    }
    if (want_var)
        perr_at(tok, "'%s' is not a variable", name);
    return cnst(s->val);
}

static void parse_world(void)
{
    DICT = table_new("dict");
    define("newline", S_CONST, v_strn("\n", 1));
    define("true", S_CONST, v_int(1));
    define("false", S_CONST, v_int(0));
    define("dict", S_TABLE, v_table(DICT));
    for (;;) {
        Tok *t;
        while (at_op(";"))
            P++;
        t = peek();
        if (t->k == K_EOF)
            break;
        if (t->k != K_WORD || !in_set(t->v, ENTITY_KW))
            perr_at(NULL, "syntax error: expected an entity (cons, var, thing, "
                    "proc, verb or start) but found %s", describe(t));
        if (strcmp(t->v, "cons") == 0)
            entity_cons();
        else if (strcmp(t->v, "var") == 0)
            entity_var();
        else if (strcmp(t->v, "thing") == 0)
            entity_thing();
        else if (strcmp(t->v, "proc") == 0)
            entity_proc();
        else if (strcmp(t->v, "verb") == 0)
            entity_verb();
        else
            entity_start();
    }
    if (!start_proc)
        perr_at(NULL, "no 'start' entity: a world must have one start point");
}

/* ====================================================================
 *  Options (argparse's rules: --opt value, --opt=value, unique prefixes)
 * ==================================================================== */

static const char USAGE[] =
    "usage: six [-h] [--width N] [--seed N] [--echo] [--prompt TEXT] [--input FILE]\n"
    "           [--check] [--strict] [--fix-typos] [--quiet] [--warn]\n"
    "           [--max-depth N] [--version]\n"
    "           world\n";

static const char HELP[] =
    "\n"
    "Run a SIX/FANT world (*.6 source file).\n"
    "\n"
    "positional arguments:\n"
    "  world          the SIX source file to run, e.g. adventure.6\n"
    "\n"
    "options:\n"
    "  -h, --help     show this help message and exit\n"
    "  --width N      wrap 'output' at N columns (default 79; 0 = never)\n"
    "  --seed N       seed the random number generator ('?')\n"
    "  --echo         echo each input line to the output (useful when input is\n"
    "                 piped in)\n"
    "  --prompt TEXT  text to print before each 'input' (default: none)\n"
    "  --input FILE   read 'input' lines from FILE instead of the keyboard\n"
    "  --check        only parse the world and report problems\n"
    "  --strict       be strict: undeclared words and questionable operations are\n"
    "                 errors instead of being tolerated\n"
    "  --fix-typos    resolve an undeclared word to the one declared name it is a\n"
    "                 near-miss of (e.g. 'weight' -> '_weight')\n"
    "  --quiet        suppress the startup note about tolerated problems\n"
    "  --warn         report tolerated problems on stderr\n"
    "  --max-depth N  maximum SIX procedure nesting (default 20000)\n"
    "  --version      show program's version number and exit\n";

static void usage_error(const char *fmt, ...)
{
    va_list ap;
    fputs(USAGE, stderr);
    fputs("six: error: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(2);
}

/* int() of an option value: blanks around, a sign, digits (with '_'
   between them); the digits are returned in *digits (for --seed) */
static int parse_int_arg(const char *s, int *neg, char **digits)
{
    Buf b = {0};
    const char *p = s;
    while (*p == ' ' || (*p >= '\t' && *p <= '\r'))
        p++;
    *neg = 0;
    if (*p == '+' || *p == '-')
        *neg = *p++ == '-';
    if (!is_digit((unsigned char)*p))
        return 0;
    while (is_digit((unsigned char)*p) || (*p == '_' && is_digit((unsigned char)p[1]))) {
        if (*p != '_')
            buf_putc(&b, *p);
        p++;
    }
    while (*p == ' ' || (*p >= '\t' && *p <= '\r'))
        p++;
    if (*p)
        return 0;
    *digits = b.s;
    return 1;
}

static long int_arg(const char *opt, const char *s, char **digits_out, int *neg_out)
{
    int neg;
    char *digits;
    long long v = 0;
    const char *d;
    Buf b = {0};
    if (!parse_int_arg(s, &neg, &digits)) {
        py_repr(&b, s, strlen(s));
        usage_error("argument %s: invalid int value: %s", opt, b.s);
    }
    for (d = digits; *d; d++) {
        v = v * 10 + (*d - '0');
        if (v > 1000000000000LL)
            v = 1000000000000LL;
    }
    if (digits_out)
        *digits_out = digits;
    if (neg_out)
        *neg_out = neg;
    if (neg)
        v = -v;
    if (v > 2000000000LL)
        v = 2000000000LL;
    if (v < -2000000000LL)
        v = -2000000000LL;
    return (long)v;
}

/* argparse: '^-\d+$|^-\d*\.\d+$' is a negative number, not an option */
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

static void interrupted(int sig)
{
    (void)sig;
    fflush(stdout);
    fputs("\nsix: interrupted\n", stderr);
    _Exit(130);
}

/* ====================================================================
 *  Driver
 * ==================================================================== */

static int run_world(void)
{
    static JmpBuf j;
    top_jmp = j;
    if (SETJMP(j) == 0) {
        call_proc(start_proc, NULL, 0);
        out_finish(&OUT);
        return 0;
    }
    if (jmp_code == JMP_STOP) {
        out_finish(&OUT);
        return 0;
    }
    out_finish(&OUT);
    {
        Buf b = {0};
        const char *base = world_path, *p;
        int i, shown;
        for (p = world_path; *p; p++)
            if (*p == '/' || *p == '\\')
                base = p + 1;
        buf_printf(&b, "six: runtime error: %s", err_msg);
        if (err_line >= 0)
            buf_printf(&b, " (line %d of %s)", err_line, base);
        if (n_err_trace) {
            shown = n_err_trace < 12 ? n_err_trace : 12;
            buf_puts(&b, "\n     in ");
            for (i = 0; i < shown; i++)
                buf_printf(&b, "%s%s", i ? " <- " : "", err_trace[i]);
            if (n_err_trace > shown)
                buf_printf(&b, " <- ... (%d more)", n_err_trace - shown);
        }
        fprintf(stderr, "%s\n", b.s);
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char *longopts[] = {
        "--help", "--width", "--seed", "--echo", "--prompt", "--input",
        "--check", "--strict", "--fix-typos", "--quiet", "--warn",
        "--max-depth", "--version", NULL
    };
    static const int takes_arg[] = {0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0};
    const char *world = NULL, *input_path = NULL;
    char *seed_digits = NULL;
    long width = 79;
    int check = 0, quiet = 0, only_pos = 0, i;
    Buf unrec = {0};
    FILE *f;
    Buf src = {0};
    char chunk[65536];
    size_t got;

    setvbuf(stdout, NULL, _IOFBF, 1 << 16);
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!only_pos && strcmp(a, "--") == 0) {
            only_pos = 1;
            continue;
        }
        if (!only_pos && (strcmp(a, "-h") == 0)) {
            fputs(USAGE, stdout);
            fputs(HELP, stdout);
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
                Buf b = {0};
                py_repr(&b, eq + 1, strlen(eq + 1));
                usage_error("argument %s: ignored explicit argument %s",
                            longopts[match], b.s);
            }
            switch (match) {
            case 0:
                fputs(USAGE, stdout);
                fputs(HELP, stdout);
                return 0;
            case 1: width = int_arg("--width", val, NULL, NULL); break;
            case 2: {
                int neg;
                int_arg("--seed", val, &seed_digits, &neg);
                (void)neg;
                break;
            }
            case 3: opt_echo = 1; break;
            case 4: opt_prompt = val; break;
            case 5: input_path = val; break;
            case 6: check = 1; break;
            case 7: rt_strict = opt_strict_parse = 1; break;
            case 8: opt_fix_typos = 1; break;
            case 9: quiet = 1; break;
            case 10: rt_warn = 1; break;
            case 11: max_depth = int_arg("--max-depth", val, NULL, NULL); break;
            case 12: puts("six " VERSION); return 0;
            }
            continue;
        }
        if (!only_pos && a[0] == '-' && a[1] && !looks_negative(a)) {
            buf_printf(&unrec, "%s%s", unrec.n ? " " : "", a);
            continue;
        }
        if (!world)
            world = a;
        else
            buf_printf(&unrec, "%s%s", unrec.n ? " " : "", a);
    }
    if (!world)
        usage_error("the following arguments are required: world");
    if (unrec.n)
        usage_error("unrecognized arguments: %s", unrec.s);

    world_path = world;
    signal(SIGINT, interrupted);

    f = fopen(world, "rb");
    if (!f) {
        Buf b = {0};
        int e = errno;
        py_repr(&b, world, strlen(world));
        fprintf(stderr, "six: cannot read %s: [Errno %d] %s: %s\n", world, e,
                strerror(e), b.s);
        return 2;
    }
    while ((got = fread(chunk, 1, sizeof chunk, f)) > 0)
        buf_putn(&src, chunk, got);
    fclose(f);

    lex(buf_str(&src), src.n);
    P = 0;
    parse_world();

    if (check) {
        printf("%s: OK  (%d things, %d verbs with %d nouns, %d procs, "
               "%d constants, %d global variables)\n", world, cnt_thing,
               cnt_verb, cnt_noun, cnt_proc, cnt_cons, cnt_var);
        for (i = 0; i < nwarnings; i++)
            printf("  warning: %s\n", warnings[i]);
        return 0;
    }
    if (undeclared_count && !quiet) {
        const char *base = world, *p;
        for (p = world; *p; p++)
            if (*p == '/' || *p == '\\')
                base = p + 1;
        fprintf(stderr, "six: note: %d undeclared word(s) in %s were treated "
                "as new properties (run with --check for details, or "
                "--fix-typos to correct near-misses)\n", undeclared_count, base);
    }
    if (seed_digits)
        py_seed_decimal(seed_digits);
    else
        py_seed_random();

    if (input_path) {
        IN = fopen(input_path, "rb");
        if (!IN) {
            Buf b = {0};
            int e = errno;
            py_repr(&b, input_path, strlen(input_path));
            fprintf(stderr, "six: cannot read %s: [Errno %d] %s: %s\n",
                    input_path, e, strerror(e), b.s);
            return 2;
        }
    } else {
        IN = stdin;
#ifdef _WIN32
        if (!ISATTY(FILENO(stdin)))
            _setmode(FILENO(stdin), _O_BINARY);
#endif
    }

    OUT.f = stdout;
    OUT.width = width;
    OUT.tty = ISATTY(FILENO(stdout));
    have_out = 1;
    return run_world();
}
