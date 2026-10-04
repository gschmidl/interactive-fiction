/*
 * dunnet.c - runs Ron Schnell's 1982 MacLisp "dungeon" (the predecessor of
 * Dunnet) as written.
 *
 * A small interpreter for the MacLisp the game uses, with the archived source
 * (foo.lsp) built in, plus a COMRED library in C.  COMRED was the game's
 * interface to the TOPS-20 COMND JSYS; the original library is lost, so this
 * one follows COMND's own conventions: keywords may be abbreviated, ESC
 * completes a keyword and shows the guide words, ? lists the choices,
 * deleting back into an earlier field reparses the command, and a bad field
 * prints the error text and prompts again.
 *
 * MacLisp traits kept because the game depends on them: numbers are read and
 * printed in octal unless they end in a point, every variable is dynamically
 * bound, symbols are upcased by the reader, "/" quotes the next character
 * (also inside strings), small fixnums are EQ, and CAR/CDR of a fixnum is NIL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "sources.h"   /* SRC_FOO, SRC_FIXES */

/* ------------------------------------------------------------------ objects */

typedef struct Obj Obj;
typedef struct Sym Sym;
typedef Obj *(*Subr)(Obj **a, int n);
typedef Obj *(*Fsubr)(Obj *args);

enum { T_FIX, T_SYM, T_STR, T_CONS };
enum { FN_NONE, FN_EXPR, FN_SUBR, FN_FSUBR };

struct Obj {
    int type;
    union {
        long fix;
        Sym *sym;
        char *str;
        struct { Obj *car, *cdr; } c;
    } u;
};

struct Sym {
    char *name;
    Obj *value;          /* NULL = unbound */
    int fnkind;
    Obj *expr;           /* (params . body) */
    Subr subr;
    Fsubr fsubr;
    int minargs, maxargs;
    Obj *plist;
    Obj *self;
    Sym *next;
};

static void *xalloc(size_t n)
{
    static char *cur;
    static size_t left;
    void *p;
    n = (n + 15) & ~(size_t)15;
    if (n > left) {
        size_t sz = n > (1 << 20) ? n : (1 << 20);
        cur = malloc(sz);
        if (!cur) { fputs("out of memory\n", stderr); exit(1); }
        left = sz;
    }
    p = cur; cur += n; left -= n;
    return p;
}

static char *xstrdup(const char *s)
{
    char *p = xalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

static Obj *NIL, *T;

static Obj *mkfix(long v) { Obj *o = xalloc(sizeof *o); o->type = T_FIX; o->u.fix = v; return o; }
static Obj *mkstr(const char *s) { Obj *o = xalloc(sizeof *o); o->type = T_STR; o->u.str = xstrdup(s); return o; }
static Obj *cons(Obj *a, Obj *d) { Obj *o = xalloc(sizeof *o); o->type = T_CONS; o->u.c.car = a; o->u.c.cdr = d; return o; }

#define CONSP(o) ((o)->type == T_CONS)
#define SYMP(o)  ((o)->type == T_SYM)
#define FIXP(o)  ((o)->type == T_FIX)
#define STRP(o)  ((o)->type == T_STR)

static Obj *car(Obj *o) { return CONSP(o) ? o->u.c.car : NIL; }
static Obj *cdr(Obj *o) { return CONSP(o) ? o->u.c.cdr : NIL; }

#define HSIZE 1021
static Sym *symtab[HSIZE];

static unsigned hash(const char *s) { unsigned h = 0; while (*s) h = h * 31 + (unsigned char)*s++; return h % HSIZE; }

static Sym *newsym(const char *name)
{
    Sym *s = xalloc(sizeof *s);
    memset(s, 0, sizeof *s);
    s->name = xstrdup(name);
    s->plist = NIL;
    s->self = xalloc(sizeof(Obj));
    s->self->type = T_SYM;
    s->self->u.sym = s;
    return s;
}

static Obj *intern(const char *name)
{
    unsigned h = hash(name);
    Sym *s;
    for (s = symtab[h]; s; s = s->next)
        if (!strcmp(s->name, name)) return s->self;
    s = newsym(name);
    s->next = symtab[h];
    symtab[h] = s;
    if (NIL) s->plist = NIL;
    return s->self;
}

static Obj *uninterned(const char *name) { Sym *s = newsym(name); s->plist = NIL; return s->self; }

static const char *pname(Obj *o) { return o->u.sym->name; }

/* --------------------------------------------------------------- output */

static int column;
static int debug;
static int fixes = 1;

static void out_char(int c)
{
    putchar(c);
    if (c == '\n' || c == '\r') column = 0;
    else if (c == '\b') { if (column) column--; }
    else column++;
}

static void out_str(const char *s) { while (*s) out_char((unsigned char)*s++); }

static void fresh_line(void) { if (column) out_char('\n'); }

static void flush_out(void) { fflush(stdout); }

/* MacLisp base 8 output, no trailing point. */
static void fix_to_str(long v, char *buf)
{
    char tmp[40];
    int i = 0, neg = v < 0;
    unsigned long u = neg ? -(unsigned long)v : (unsigned long)v;
    do { tmp[i++] = '0' + (int)(u & 7); u >>= 3; } while (u);
    if (neg) *buf++ = '-';
    while (i) *buf++ = tmp[--i];
    *buf = 0;
}

/* Does this print name need |...| to read back as the same symbol? */
static int needs_bars(const char *s)
{
    const char *p;
    int digits = 0, other = 0;
    if (!*s) return 1;
    for (p = s; *p; p++) {
        unsigned char c = *p;
        if (islower(c) || c <= ' ' || c >= 127 || strchr("()';\"|/.", c)) return 1;
        if (isdigit(c)) digits++; else if (c != '+' && c != '-') other++;
    }
    return digits && !other;   /* would read as a number */
}

typedef struct { char *p; size_t n, cap; } Buf;

static void buf_add(Buf *b, int c)
{
    if (b->n + 2 > b->cap) {
        size_t nc = b->cap ? b->cap * 2 : 64;
        char *np = malloc(nc);
        if (b->n) memcpy(np, b->p, b->n);
        free(b->p);
        b->p = np; b->cap = nc;
    }
    b->p[b->n++] = (char)c;
    b->p[b->n] = 0;
}

static void buf_str(Buf *b, const char *s) { while (*s) buf_add(b, (unsigned char)*s++); }

static void print_obj(Buf *b, Obj *o, int slash)
{
    char tmp[40];
    switch (o->type) {
    case T_FIX:
        fix_to_str(o->u.fix, tmp);
        buf_str(b, tmp);
        break;
    case T_STR:
        if (!slash) buf_str(b, o->u.str);
        else {
            const char *p;
            buf_add(b, '"');
            for (p = o->u.str; *p; p++) { if (*p == '"' || *p == '/') buf_add(b, '/'); buf_add(b, *p); }
            buf_add(b, '"');
        }
        break;
    case T_SYM:
        if (slash && needs_bars(pname(o))) {
            const char *p;
            buf_add(b, '|');
            for (p = pname(o); *p; p++) { if (*p == '|' || *p == '/') buf_add(b, '/'); buf_add(b, *p); }
            buf_add(b, '|');
        } else
            buf_str(b, pname(o));
        break;
    case T_CONS:
        buf_add(b, '(');
        for (;;) {
            print_obj(b, o->u.c.car, slash);
            o = o->u.c.cdr;
            if (o == NIL) break;
            if (!CONSP(o)) { buf_str(b, " . "); print_obj(b, o, slash); break; }
            buf_add(b, ' ');
        }
        buf_add(b, ')');
        break;
    }
}

static char *obj_string(Obj *o, int slash)
{
    Buf b = {0};
    buf_add(&b, ' ');
    b.n = 0; b.p[0] = 0;
    print_obj(&b, o, slash);
    return b.p;
}

static void out_obj(Obj *o, int slash) { char *s = obj_string(o, slash); out_str(s); free(s); }

/* ---------------------------------------------------- non-local exits */

enum { FR_BLOCK, FR_COMRED, FR_GUARD };
enum { JMP_RETURN = 1, JMP_REPARSE, JMP_RESTART, JMP_ERROR };

typedef struct Frame {
    jmp_buf jb;
    int kind;
    int bmark;
    int cmark;
    int depth;
    Obj *val;
} Frame;

#define MAXFRAMES 4096
static Frame *frames[MAXFRAMES];
static int nframes;

typedef struct { Sym *s; Obj *old; } Binding;
#define MAXBIND 65536
static Binding bstack[MAXBIND];
static int nbind;
static int eval_depth;

static void bind(Obj *sym, Obj *val);
static void unbind_to(int mark)
{
    while (nbind > mark) { nbind--; bstack[nbind].s->value = bstack[nbind].old; }
}

static int ncomnd;   /* COMND state stack depth, see below */

static void push_frame(Frame *f, int kind)
{
    if (nframes >= MAXFRAMES) { fputs("frame stack overflow\n", stderr); exit(1); }
    f->kind = kind;
    f->bmark = nbind;
    f->cmark = ncomnd;
    f->depth = eval_depth;
    f->val = NIL;
    frames[nframes++] = f;
}

static void pop_frame(Frame *f)
{
    while (nframes > 0 && frames[nframes - 1] != f) nframes--;
    if (nframes > 0) nframes--;
}

/* Unwind to frame index i and jump there. */
static void jump_to(int i, int code)
{
    Frame *f = frames[i];
    nframes = i + 1;
    unbind_to(f->bmark);
    ncomnd = f->cmark;
    eval_depth = f->depth;
    longjmp(f->jb, code);
}

static Obj *last_error_obj;

static void lisp_error(Obj *o, const char *msg)
{
    int i;
    fresh_line();
    out_char(';');
    if (o) { out_obj(o, 1); out_char(' '); }
    out_str(msg);
    out_char('\n');
    last_error_obj = o;
    for (i = nframes - 1; i >= 0; i--)
        if (frames[i]->kind == FR_GUARD) jump_to(i, JMP_ERROR);
    flush_out();
    exit(1);
}

/* ------------------------------------------------------------ reader */

typedef struct {
    const char *s;
    size_t pos, len;
    int line;
} Reader;

static Obj *DOT;   /* internal marker */

static int rd_peek(Reader *r) { return r->pos < r->len ? (unsigned char)r->s[r->pos] : -1; }
static int rd_get(Reader *r)
{
    int c = rd_peek(r);
    if (c >= 0) { r->pos++; if (c == '\n') r->line++; }
    return c;
}

static int is_ws(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
static int is_delim(int c) { return c < 0 || is_ws(c) || c == '(' || c == ')' || c == '\'' || c == ';' || c == '"'; }

static void skip_ws(Reader *r)
{
    for (;;) {
        int c = rd_peek(r);
        if (is_ws(c)) rd_get(r);
        else if (c == ';') { while ((c = rd_get(r)) >= 0 && c != '\n') ; }
        else break;
    }
}

static int read_err;

static Obj *read_form(Reader *r);

/* A token of digits (optional sign) is a fixnum: octal, or decimal with a
   trailing point. */
static int parse_number(const char *t, long *out)
{
    const char *p = t;
    int neg = 0, nd = 0, point = 0;
    long v8 = 0, v10 = 0;
    if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
    for (; isdigit((unsigned char)*p); p++, nd++) {
        v8 = v8 * 8 + (*p - '0');
        v10 = v10 * 10 + (*p - '0');
    }
    if (!nd) return 0;
    if (*p == '.') { point = 1; p++; }
    if (*p) return 0;
    *out = neg ? -(point ? v10 : v8) : (point ? v10 : v8);
    return 1;
}

static Obj *read_token(Reader *r)
{
    Buf b = {0};
    int escaped = 0;
    long v;
    Obj *o;
    for (;;) {
        int c = rd_peek(r);
        if (c == '/') {
            rd_get(r);
            c = rd_get(r);
            if (c < 0) break;
            buf_add(&b, c);
            escaped = 1;
        } else if (c == '|') {
            rd_get(r);
            while ((c = rd_get(r)) >= 0 && c != '|') {
                if (c == '/') { c = rd_get(r); if (c < 0) break; }
                buf_add(&b, c);
            }
            escaped = 1;
        } else if (is_delim(c))
            break;
        else if (c == '.' && b.n) {
            /* a point after digits makes a decimal number; otherwise the
               dot ends the token and is read as a dotted-pair dot */
            long dummy;
            int nx = r->pos + 1 < r->len ? (unsigned char)r->s[r->pos + 1] : -1;
            if (!escaped && parse_number(b.p, &dummy) && strchr(b.p, '.') == NULL && is_delim(nx)) {
                rd_get(r);
                buf_add(&b, '.');
            }
            break;
        } else if (c == '.') {
            rd_get(r);
            buf_add(&b, '.');
            break;
        } else {
            rd_get(r);
            buf_add(&b, toupper(c));
        }
    }
    if (!b.p) buf_add(&b, 0), b.n = 0, b.p[0] = 0;
    if (!escaped && !strcmp(b.p, ".")) o = DOT;
    else if (!escaped && parse_number(b.p, &v)) o = mkfix(v);
    else o = intern(b.p);
    free(b.p);
    return o;
}

static Obj *read_list(Reader *r)
{
    Obj *head = NIL, *tail = NIL;
    for (;;) {
        Obj *x;
        skip_ws(r);
        if (rd_peek(r) < 0) { read_err = 1; return head; }
        if (rd_peek(r) == ')') { rd_get(r); return head; }
        x = read_form(r);
        if (read_err) return head;
        if (x == DOT) {
            Obj *d;
            skip_ws(r);
            if (rd_peek(r) == ')') {   /* (LAMP .) */
                x = cons(intern("."), NIL);
                if (head == NIL) head = x; else tail->u.c.cdr = x;
                tail = x;
                continue;
            }
            if (head == NIL) { read_err = 1; return NIL; }
            d = read_form(r);
            if (read_err) return head;
            tail->u.c.cdr = d;
            skip_ws(r);
            if (rd_get(r) != ')') { read_err = 1; }
            return head;
        }
        x = cons(x, NIL);
        if (head == NIL) head = x; else tail->u.c.cdr = x;
        tail = x;
    }
}

static Obj *read_form(Reader *r)
{
    int c;
    skip_ws(r);
    c = rd_peek(r);
    if (c < 0) { read_err = 1; return NIL; }
    if (c == '(') { rd_get(r); return read_list(r); }
    if (c == ')') { rd_get(r); read_err = 1; return NIL; }
    if (c == '\'') {
        Obj *x;
        rd_get(r);
        x = read_form(r);
        return cons(intern("QUOTE"), cons(x, NIL));
    }
    if (c == '"') {
        Buf b = {0};
        Obj *o;
        rd_get(r);
        buf_add(&b, 0); b.n = 0; b.p[0] = 0;
        while ((c = rd_get(r)) >= 0 && c != '"') {
            if (c == '/') { c = rd_get(r); if (c < 0) break; }
            buf_add(&b, c);
        }
        o = mkstr(b.p);
        free(b.p);
        return o;
    }
    return read_token(r);
}

/* --------------------------------------------------------- terminal input */

static int is_console;
static int pending_lf;      /* READLINE left the LF of CR LF unread */
static Obj *ttyint_char;    /* interrupt character (SSTATUS TTYINT) */
static Obj *ttyint_fn;

#ifdef _WIN32
static HANDLE hin;
#endif

static void quit_program(void)
{
    fresh_line();
    flush_out();
    exit(0);
}

/* One raw character, no echo.  CR or LF both come back as '\r'.  -1 = EOF. */
static int tty_raw(void)
{
    int c;
    flush_out();
#ifdef _WIN32
    if (is_console) {
        for (;;) {
            c = _getch();
            if (c == 0 || c == 0xE0) { _getch(); continue; }   /* function/arrow keys */
            if (c == 26) return -1;                              /* ^Z */
            c &= 0x7F;
            if (c == '\n') c = '\r';
            return c;
        }
    }
#endif
    for (;;) {
        c = getchar();
        if (c == EOF) return -1;
        c &= 0x7F;
        if (c == '\r') continue;
        if (c == '\n') return '\r';
        return c;
    }
}

static Obj *char_sym(int c) { char s[2]; s[0] = (char)c; s[1] = 0; return intern(s); }

static Obj *apply(Obj *fn, Obj **args, int n);

static void erase_char(int c)
{
    (void)c;
    out_str("\b \b");
}

/* READLINE: a line edited with DEL/BS and ^U, returned as an uninterned
   symbol.  Typing the interrupt character runs its handler at once. */
static Obj *read_line(void)
{
    Buf b = {0};
    Obj *o;
    buf_add(&b, 0); b.n = 0; b.p[0] = 0;
    pending_lf = 0;
    for (;;) {
        int c = tty_raw();
        if (c < 0) {
            if (!b.n) quit_program();
            break;
        }
        if (c == '\r') { out_char('\n'); pending_lf = 1; break; }
        if (ttyint_fn && ttyint_char && c == pname(ttyint_char)[0]) {
            Frame f;
            Obj *args[2];
            args[0] = NIL; args[1] = char_sym(c);
            push_frame(&f, FR_GUARD);   /* an error ends only the handler */
            if (!setjmp(f.jb)) apply(ttyint_fn, args, 2);
            pop_frame(&f);
            continue;
        }
        if (c == 8 || c == 127) {
            if (b.n) { b.n--; b.p[b.n] = 0; erase_char(c); }
            continue;
        }
        if (c == 21) {   /* ^U */
            while (b.n) { b.n--; erase_char(0); }
            b.p[0] = 0;
            continue;
        }
        if (c < ' ') continue;
        buf_add(&b, c);
        out_char(c);
    }
    o = uninterned(b.p);
    free(b.p);
    return o;
}

static Obj *read_char(void)
{
    int c;
    if (pending_lf) { pending_lf = 0; return char_sym('\n'); }
    c = tty_raw();
    if (c < 0) quit_program();
    if (c == '\r') { out_char('\n'); return char_sym('\n'); }
    if (c >= ' ') out_char(c);
    return char_sym(c);
}

/* -------------------------------------------------------------- eval */

static Obj *eval(Obj *x);

static void bind(Obj *sym, Obj *val)
{
    Sym *s;
    if (!SYMP(sym) || sym == NIL || sym == T) lisp_error(sym, "CAN'T BE BOUND");
    if (nbind >= MAXBIND) lisp_error(NULL, "PDL OVERFLOW");
    s = sym->u.sym;
    bstack[nbind].s = s;
    bstack[nbind].old = s->value;
    nbind++;
    s->value = val;
}

static Obj *progn(Obj *body)
{
    Obj *v = NIL;
    for (; CONSP(body); body = body->u.c.cdr) v = eval(body->u.c.car);
    return v;
}

static int list_length(Obj *l) { int n = 0; while (CONSP(l)) { n++; l = l->u.c.cdr; } return n; }

/* User functions whose errors only abort themselves: the command loop of
   the game survives a Lisp error in a command (MacLisp would stop in a
   breakpoint instead). */
static Obj *guarded[8];
static int nguarded;

static Obj *apply_lambda(Obj *params, Obj *body, Obj **args, int n, Obj *name)
{
    int mark = nbind, i = 0;
    Obj *p, *v;
    for (p = params; CONSP(p); p = p->u.c.cdr, i++) {
        if (i >= n) lisp_error(name, "WRONG NUMBER OF ARGS");
        bind(p->u.c.car, args[i]);
    }
    if (i < n) lisp_error(name, "WRONG NUMBER OF ARGS");
    v = progn(body);
    unbind_to(mark);
    return v;
}

static Obj *apply_guarded(Obj *params, Obj *body, Obj **args, int n, Obj *name)
{
    Frame f;
    Obj *v;
    push_frame(&f, FR_GUARD);
    if (setjmp(f.jb)) {
        pop_frame(&f);
        return NIL;
    }
    v = apply_lambda(params, body, args, n, name);
    pop_frame(&f);
    return v;
}

#define MAXARGS 64

static Obj *apply(Obj *fn, Obj **args, int n)
{
    if (SYMP(fn) && fn != NIL) {
        Sym *s = fn->u.sym;
        int i;
        switch (s->fnkind) {
        case FN_EXPR:
            for (i = 0; i < nguarded; i++)
                if (guarded[i] == fn)
                    return apply_guarded(car(s->expr), cdr(s->expr), args, n, fn);
            return apply_lambda(car(s->expr), cdr(s->expr), args, n, fn);
        case FN_SUBR:
            if (n < s->minargs || (s->maxargs >= 0 && n > s->maxargs))
                lisp_error(fn, "WRONG NUMBER OF ARGS");
            return s->subr(args, n);
        default:
            lisp_error(fn, "UNDEFINED FUNCTION");
        }
    }
    if (CONSP(fn) && fn->u.c.car == intern("LAMBDA"))
        return apply_lambda(car(cdr(fn)), cdr(cdr(fn)), args, n, fn);
    lisp_error(fn, "UNDEFINED FUNCTION");
    return NIL;
}

static Obj *eval(Obj *x)
{
    Obj *head, *args[MAXARGS], *p, *v;
    int n = 0;
    switch (x->type) {
    case T_FIX: case T_STR:
        return x;
    case T_SYM:
        if (x == NIL || x == T) return x;
        if (!x->u.sym->value) lisp_error(x, "UNBOUND VARIABLE");
        return x->u.sym->value;
    }
    if (++eval_depth > 100000) { eval_depth = 0; lisp_error(NULL, "PDL OVERFLOW"); }
    head = x->u.c.car;
    if (SYMP(head) && head->u.sym->fnkind == FN_FSUBR) {
        v = head->u.sym->fsubr(x->u.c.cdr);
        eval_depth--;
        return v;
    }
    for (p = x->u.c.cdr; CONSP(p); p = p->u.c.cdr) {
        if (n >= MAXARGS) lisp_error(x, "TOO MANY ARGS");
        args[n++] = eval(p->u.c.car);
    }
    if (SYMP(head) && head->u.sym->fnkind == FN_NONE && head->u.sym->value &&
        head->u.sym->value != NIL)
        head = head->u.sym->value;   /* MacLisp: a variable holding a function */
    v = apply(head, args, n);
    eval_depth--;
    return v;
}

/* --------------------------------------------------------- special forms */

static Obj *sf_quote(Obj *a) { return car(a); }
static Obj *sf_function(Obj *a) { return car(a); }

static Obj *sf_setq(Obj *a)
{
    Obj *v = NIL;
    while (CONSP(a)) {
        Obj *sym = car(a);
        v = eval(car(cdr(a)));
        if (!SYMP(sym) || sym == NIL || sym == T) lisp_error(sym, "CAN'T BE SET");
        sym->u.sym->value = v;
        a = cdr(cdr(a));
    }
    return v;
}

static Obj *sf_cond(Obj *a)
{
    for (; CONSP(a); a = a->u.c.cdr) {
        Obj *clause = a->u.c.car;
        Obj *t = eval(car(clause));
        if (t != NIL) return CONSP(cdr(clause)) ? progn(cdr(clause)) : t;
    }
    return NIL;
}

static Obj *sf_and(Obj *a) { Obj *v = T; for (; CONSP(a); a = a->u.c.cdr) if ((v = eval(a->u.c.car)) == NIL) return NIL; return v; }
static Obj *sf_or(Obj *a) { Obj *v; for (; CONSP(a); a = a->u.c.cdr) if ((v = eval(a->u.c.car)) != NIL) return v; return NIL; }
static Obj *sf_progn(Obj *a) { return progn(a); }
static Obj *sf_prog1(Obj *a) { Obj *v = eval(car(a)); progn(cdr(a)); return v; }
static Obj *sf_prog2(Obj *a) { Obj *v; eval(car(a)); v = eval(car(cdr(a))); progn(cdr(cdr(a))); return v; }

static Obj *sf_defun(Obj *a)
{
    Obj *name = car(a);
    Sym *s;
    if (!SYMP(name)) lisp_error(name, "BAD FUNCTION NAME");
    s = name->u.sym;
    s->fnkind = FN_EXPR;
    s->expr = cdr(a);
    return name;
}

/* (let ((v init) v2 ...) body) - parallel binding, not a block. */
static Obj *sf_let(Obj *a)
{
    Obj *vars[MAXARGS], *vals[MAXARGS], *p, *v;
    int n = 0, i, mark = nbind;
    for (p = car(a); CONSP(p); p = p->u.c.cdr) {
        Obj *b = p->u.c.car;
        if (n >= MAXARGS) lisp_error(a, "TOO MANY LET VARIABLES");
        if (CONSP(b)) { vars[n] = car(b); vals[n] = eval(car(cdr(b))); }
        else { vars[n] = b; vals[n] = NIL; }
        n++;
    }
    for (i = 0; i < n; i++) bind(vars[i], vals[i]);
    v = progn(cdr(a));
    unbind_to(mark);
    return v;
}

static Obj *sf_return(Obj *a)
{
    Obj *v = eval(car(a));
    int i;
    for (i = nframes - 1; i >= 0; i--)
        if (frames[i]->kind == FR_BLOCK || frames[i]->kind == FR_COMRED) {
            frames[i]->val = v;
            jump_to(i, JMP_RETURN);
        }
    lisp_error(NULL, "RETURN OUTSIDE OF PROG OR DO");
    return NIL;
}

/* (do ((var init step) ...) (endtest result...) body...) and the old form
   (do var init step endtest body...).  A block for RETURN. */
static Obj *sf_do(Obj *a)
{
    Frame f;
    Obj *vars[MAXARGS], *steps[MAXARGS], *vals[MAXARGS], *endc, *body, *p, *v;
    int hasstep[MAXARGS];
    int n = 0, i, mark = nbind;
    volatile int oldform = !CONSP(car(a)) && car(a) != NIL;

    if (oldform) {
        vars[0] = car(a);
        vals[0] = eval(car(cdr(a)));
        steps[0] = car(cdr(cdr(a)));
        hasstep[0] = 1;
        n = 1;
        endc = cons(car(cdr(cdr(cdr(a)))), NIL);
        body = cdr(cdr(cdr(cdr(a))));
    } else {
        for (p = car(a); CONSP(p); p = p->u.c.cdr) {
            Obj *b = p->u.c.car;
            if (n >= MAXARGS) lisp_error(a, "TOO MANY DO VARIABLES");
            if (CONSP(b)) {
                vars[n] = car(b);
                vals[n] = eval(car(cdr(b)));
                hasstep[n] = CONSP(cdr(cdr(b)));
                steps[n] = car(cdr(cdr(b)));
            } else { vars[n] = b; vals[n] = NIL; hasstep[n] = 0; steps[n] = NIL; }
            n++;
        }
        endc = car(cdr(a));
        body = cdr(cdr(a));
    }
    for (i = 0; i < n; i++) bind(vars[i], vals[i]);
    push_frame(&f, FR_BLOCK);
    if (setjmp(f.jb)) {
        v = f.val;
        pop_frame(&f);
        unbind_to(mark);
        return v;
    }
    if (!oldform && !CONSP(endc)) {   /* (do ((...)) nil ...) runs once */
        progn(body);
        v = NIL;
    } else for (;;) {
        if (eval(car(endc)) != NIL) { v = progn(cdr(endc)); break; }
        progn(body);
        for (i = 0; i < n; i++) if (hasstep[i]) vals[i] = eval(steps[i]);
        for (i = 0; i < n; i++) if (hasstep[i]) vars[i]->u.sym->value = vals[i];
    }
    pop_frame(&f);
    unbind_to(mark);
    return v;
}

static Obj *sf_ignore(Obj *a) { (void)a; return NIL; }

/* (sstatus ttyint char fn) */
static Obj *sf_sstatus(Obj *a)
{
    if (car(a) == intern("TTYINT")) {
        ttyint_char = eval(car(cdr(a)));
        ttyint_fn = eval(car(cdr(cdr(a))));
        if (FIXP(ttyint_char)) ttyint_char = char_sym((int)ttyint_char->u.fix);
    }
    return T;
}

/* ------------------------------------------------------------ COMRED */

typedef struct {
    char prompt[80];
    char buf[1024];
    int len;        /* characters typed in this command */
    int pos;        /* parse position */
    int last_esc;   /* the field just parsed ended in ESC */
} Comnd;

#define MAXCOMND 32
static Comnd comnd[MAXCOMND];
static Obj *S_HELP, *S_ERROR, *S_ITEMS, *S_CONFIRM, *S_TEXT;

static Comnd *cm(void)
{
    if (!ncomnd) {   /* COMRED used outside LET-COMRED: an unnamed state */
        memset(&comnd[0], 0, sizeof comnd[0]);
        ncomnd = 1;
    }
    return &comnd[ncomnd - 1];
}

static void cm_prompt(void)
{
    Comnd *c = cm();
    fresh_line();
    out_str(c->prompt);
}

static void cm_retype(void)
{
    Comnd *c = cm();
    int i;
    cm_prompt();
    for (i = 0; i < c->len; i++) if (c->buf[i] != '\r') out_char(c->buf[i]);
}

static int comred_frame(void)
{
    int i;
    for (i = nframes - 1; i >= 0; i--) if (frames[i]->kind == FR_COMRED) return i;
    return -1;
}

static void cm_reparse(void)
{
    int i = comred_frame();
    if (i >= 0) jump_to(i, JMP_REPARSE);
}

static void cm_error(const char *text)
{
    int i;
    fresh_line();
    if (text && *text != '?') out_char('?');
    out_str(text ? text : "?");
    out_char('\n');
    i = comred_frame();
    if (i >= 0) jump_to(i, JMP_RESTART);
}

static const char *spec_text(Obj *spec, Obj *prop, const char *dflt)
{
    Obj *p;
    for (p = spec->u.sym->plist; CONSP(p) && CONSP(cdr(p)); p = cdr(cdr(p)))
        if (car(p) == prop) {
            Obj *v = car(cdr(p));
            if (STRP(v)) return v->u.str;
            if (SYMP(v)) return pname(v);
        }
    return dflt;
}

static Obj *get_prop(Obj *sym, Obj *prop)
{
    Obj *p;
    if (!SYMP(sym)) return NIL;
    for (p = sym->u.sym->plist; CONSP(p) && CONSP(cdr(p)); p = cdr(cdr(p)))
        if (car(p) == prop) return car(cdr(p));
    return NIL;
}

static void put_prop(Obj *sym, Obj *prop, Obj *val)
{
    Obj *p;
    for (p = sym->u.sym->plist; CONSP(p) && CONSP(cdr(p)); p = cdr(cdr(p)))
        if (car(p) == prop) { cdr(p)->u.c.car = val; return; }
    sym->u.sym->plist = cons(prop, cons(val, sym->u.sym->plist));
}

static void upcase_str(const char *s, char *out, size_t n)
{
    size_t i;
    for (i = 0; s[i] && i + 1 < n; i++) out[i] = (char)toupper((unsigned char)s[i]);
    out[i] = 0;
}

/* Keywords of a spec, sorted as a COMND table is. */
static int spec_keywords(Obj *spec, Obj **out, int max)
{
    Obj *p;
    int n = 0, i, j;
    for (p = get_prop(spec, S_ITEMS); CONSP(p) && n < max; p = p->u.c.cdr)
        out[n++] = p->u.c.car;
    for (i = 1; i < n; i++)
        for (j = i; j > 0; j--) {
            char a[256], b[256];
            Obj *t;
            upcase_str(obj_string(out[j - 1], 0), a, sizeof a);
            upcase_str(obj_string(out[j], 0), b, sizeof b);
            if (strcmp(a, b) <= 0) break;
            t = out[j]; out[j] = out[j - 1]; out[j - 1] = t;
        }
    return n;
}

static int prefix_of(const char *field, const char *kw)
{
    char a[256], b[256];
    upcase_str(field, a, sizeof a);
    upcase_str(kw, b, sizeof b);
    return !strncmp(a, b, strlen(a));
}

static int same_name(const char *x, const char *y)
{
    char a[256], b[256];
    upcase_str(x, a, sizeof a);
    upcase_str(y, b, sizeof b);
    return !strcmp(a, b);
}

enum { FLD_KEYWORD, FLD_TEXT, FLD_CONFIRM };

static void cm_help(int mode, Obj *spec, const char *field)
{
    Obj *kw[256];
    int n, i, shown = 0;
    out_str("? ");
    if (mode == FLD_CONFIRM) out_str("confirm with carriage return");
    else if (mode == FLD_TEXT) out_str(spec_text(spec, S_HELP, "text string"));
    else {
        out_str(spec_text(spec, S_HELP, "keyword"));
        n = spec_keywords(spec, kw, 256);
        out_str(", one of the following:");
        for (i = 0; i < n; i++) {
            char name[256];
            char *s = obj_string(kw[i], 0);
            if (!prefix_of(field, s)) { free(s); continue; }
            upcase_str(s, name, sizeof name);
            free(s);
            if (shown % 4 == 0) out_str("\n ");
            out_str(name);
            if (shown % 4 != 3) { int k = (int)strlen(name); while (k++ < 16) out_char(' '); }
            shown++;
        }
    }
    out_char('\n');
    cm_retype();
}

/* Delete the last typed character; reparse when it belonged to a field that
   has already been parsed. */
static void cm_delete(int field_start)
{
    Comnd *c = cm();
    if (!c->len) { out_char('\a'); return; }
    c->len--;
    if (c->buf[c->len] != '\r') erase_char(c->buf[c->len]);
    {   /* into a guide word: it goes as a whole */
        int k = c->len;
        while (k > 0 && c->buf[k - 1] != '(' && c->buf[k - 1] != ')') k--;
        if (k > 0 && c->buf[k - 1] == '(')
            while (c->len >= k) { c->len--; erase_char(c->buf[c->len]); }
    }
    if (c->len < field_start || c->len < c->pos) cm_reparse();
}

/* Read the next field.  Returns its text in out (empty if none); *term is
   ' ' or '\r'.  A completed keyword is stored with its trailing space. */
static void cm_field(int mode, Obj *spec, char *out, int outn, int *term)
{
    Comnd *c = cm();
    int start, i, esc = 0;
    for (;;) {
        while (c->pos < c->len && c->buf[c->pos] == ' ') c->pos++;
        start = c->pos;
        /* already in the buffer (reparse, or typed ahead)? */
        for (i = start; i < c->len; i++)
            if (c->buf[i] == ' ' || c->buf[i] == '\r') break;
        if (i < c->len) {
            int n = i - start;
            if (n >= outn) n = outn - 1;
            memcpy(out, c->buf + start, n);
            out[n] = 0;
            *term = c->buf[i];
            c->pos = c->buf[i] == ' ' ? i + 1 : i;
            if (out[0] == '(' && out[n - 1] == ')' && n > 1) continue;   /* guide word typed in */
            c->last_esc = esc;
            return;
        }
        /* read from the terminal */
        for (;;) {
            int ch = tty_raw();
            int flen = c->len - start;
            if (ch < 0) quit_program();
            if (ch == '?') {
                char f[256];
                int n = flen < 255 ? flen : 255;
                memcpy(f, c->buf + start, n); f[n] = 0;
                cm_help(mode, spec, f);
                continue;
            }
            if (ch == 27) {   /* ESC: recognition */
                Obj *kw[256];
                int nk, k, match = -1, nmatch = 0;
                char f[256];
                int n = flen < 255 ? flen : 255;
                if (mode != FLD_KEYWORD) { out_char('\a'); continue; }
                memcpy(f, c->buf + start, n); f[n] = 0;
                nk = spec_keywords(spec, kw, 256);
                for (k = 0; k < nk; k++) {
                    char *s = obj_string(kw[k], 0);
                    if (same_name(f, s)) { match = k; nmatch = 1; free(s); break; }
                    if (prefix_of(f, s)) { match = k; nmatch++; }
                    free(s);
                }
                if (nmatch == 1) {
                    char name[256];
                    char *s = obj_string(kw[match], 0);
                    const char *rest;
                    upcase_str(s, name, sizeof name);
                    free(s);
                    rest = name + n;
                    while (*rest && c->len < (int)sizeof c->buf - 2) { c->buf[c->len++] = *rest; out_char(*rest); rest++; }
                    c->buf[c->len++] = ' ';
                    out_char(' ');
                    esc = 1;
                    break;
                }
                if (nmatch == 0 && n) {
                    c->buf[c->len] = 0;
                    cm_error(spec_text(spec, S_ERROR, "Does not match keyword"));
                }
                out_char('\a');
                continue;
            }
            if (ch == 8 || ch == 127) { cm_delete(start); continue; }
            if (ch == 21) {   /* ^U: start the line again */
                c->len = 0;
                out_str(" ^U");
                cm_prompt();
                cm_reparse();
                continue;
            }
            if (ch == 23) {   /* ^W: delete a word */
                while (c->len && c->buf[c->len - 1] == ' ') cm_delete(start);
                while (c->len && c->buf[c->len - 1] != ' ') cm_delete(start);
                continue;
            }
            if (ch == 18) { out_str("^R"); cm_retype(); continue; }
            if (ch == '\r') {
                if (c->len < (int)sizeof c->buf - 1) c->buf[c->len++] = '\r';
                out_char('\n');
                break;
            }
            if (ch == '\t') ch = ' ';
            if (ch < ' ') { out_char('\a'); continue; }
            if (ch == ' ' && flen == 0) {   /* spaces before a field */
                if (c->len < (int)sizeof c->buf - 2) { c->buf[c->len++] = ' '; out_char(' '); }
                start = c->len;
                c->pos = c->len;
                continue;
            }
            if (c->len < (int)sizeof c->buf - 2) { c->buf[c->len++] = (char)ch; out_char(ch); }
            if (ch == ' ') break;
        }
    }
}

static Obj *comred(Obj *spec)
{
    Comnd *c = cm();
    char field[256];
    int term;

    if (spec == S_CONFIRM) {
        for (;;) {
            while (c->pos < c->len && c->buf[c->pos] == ' ') c->pos++;
            if (c->pos < c->len) {
                if (c->buf[c->pos] == '\r') { c->pos++; c->last_esc = 0; return T; }
                cm_field(FLD_CONFIRM, spec, field, sizeof field, &term);
                cm_error("Not confirmed");
            }
            {
                int ch = tty_raw();
                if (ch < 0) quit_program();
                if (ch == '\r') {
                    c->buf[c->len++] = '\r';
                    c->pos = c->len;
                    out_char('\n');
                    c->last_esc = 0;
                    return T;
                }
                if (ch == '?') { cm_help(FLD_CONFIRM, spec, ""); continue; }
                if (ch == 8 || ch == 127) { cm_delete(c->pos); continue; }
                if (ch == 21) { c->len = 0; out_str(" ^U"); cm_prompt(); cm_reparse(); continue; }
                if (ch == 18) { out_str("^R"); cm_retype(); continue; }
                if (ch == ' ' || ch == '\t') { c->buf[c->len++] = ' '; out_char(' '); continue; }
                if (ch < ' ' ) { out_char('\a'); continue; }
                c->buf[c->len++] = (char)ch;
                out_char(ch);
            }
        }
    }

    if (spec == S_TEXT) {
        char up[256];
        cm_field(FLD_TEXT, spec, field, sizeof field, &term);
        upcase_str(field, up, sizeof up);
        return intern(up);
    }

    {
        Obj *kw[256];
        int nk, k, match = -1, nmatch = 0, first;
        first = c->pos == 0 || (c->pos <= c->len && strspn(c->buf, " ") >= (size_t)c->pos);
        cm_field(FLD_KEYWORD, spec, field, sizeof field, &term);
        if (!field[0]) {
            if (term == '\r' && first) {   /* empty command line: prompt again */
                int i = comred_frame();
                c->len = 0;
                if (i >= 0) jump_to(i, JMP_RESTART);
            }
            cm_error(spec_text(spec, S_ERROR, "Keyword expected"));
        }
        nk = spec_keywords(spec, kw, 256);
        for (k = 0; k < nk; k++) {
            char *s = obj_string(kw[k], 0);
            if (same_name(field, s)) { match = k; nmatch = 1; free(s); break; }
            if (prefix_of(field, s)) { match = k; nmatch++; }
            free(s);
        }
        if (nmatch == 1) return kw[match];
        if (nmatch > 1) cm_error("Ambiguous");
        cm_error(spec_text(spec, S_ERROR, "Does not match keyword"));
    }
    return NIL;
}

/* (comred-force-guideword text): after ESC recognition the guide word is
   shown as "(text) "; one typed in parentheses is skipped. */
static Obj *s_force_guideword(Obj **a, int n)
{
    Comnd *c = cm();
    const char *text = STRP(a[0]) ? a[0]->u.str : SYMP(a[0]) ? pname(a[0]) : "";
    (void)n;
    while (c->pos < c->len && c->buf[c->pos] == ' ') c->pos++;
    if (c->pos < c->len) {
        if (c->buf[c->pos] == '(') {
            int i = c->pos;
            while (i < c->len && c->buf[i] != ')') i++;
            if (i < c->len) {
                c->pos = i + 1;
                while (c->pos < c->len && c->buf[c->pos] == ' ') c->pos++;
            }
        }
    } else if (c->last_esc) {
        const char *p;
        if (c->len < (int)sizeof c->buf - 2) { c->buf[c->len++] = '('; out_char('('); }
        for (p = text; *p && c->len < (int)sizeof c->buf - 3; p++) { c->buf[c->len++] = *p; out_char(*p); }
        c->buf[c->len++] = ')'; out_char(')');
        c->buf[c->len++] = ' '; out_char(' ');
        c->pos = c->len;
    }
    c->last_esc = 0;
    return NIL;
}

/* (let-comred prompt body...): one command line.  A block for RETURN, and
   the point a reparse or an error goes back to. */
static Obj *sf_let_comred(Obj *a)
{
    Frame f;
    Obj *ptext = eval(car(a));
    Obj *body = cdr(a);
    Obj *v;
    volatile int level;
    Comnd *c;
    if (ncomnd >= MAXCOMND) lisp_error(NULL, "COMRED NESTED TOO DEEPLY");
    level = ncomnd;
    c = &comnd[ncomnd++];
    memset(c, 0, sizeof *c);
    {
        const char *p = STRP(ptext) ? ptext->u.str : SYMP(ptext) ? pname(ptext) : "";
        strncpy(c->prompt, p, sizeof c->prompt - 1);
    }
    push_frame(&f, FR_COMRED);
    switch (setjmp(f.jb)) {
    case 0:
        cm_prompt();
        break;
    case JMP_RETURN:
        v = f.val;
        pop_frame(&f);
        ncomnd = level;
        return v;
    case JMP_REPARSE:
        ncomnd = level + 1;
        comnd[level].pos = 0;
        comnd[level].last_esc = 0;
        break;
    case JMP_RESTART:
        ncomnd = level + 1;
        comnd[level].len = comnd[level].pos = 0;
        comnd[level].last_esc = 0;
        cm_prompt();
        break;
    }
    v = progn(body);
    pop_frame(&f);
    ncomnd = level;
    return v;
}

/* (defspec name nil help-text error-text) */
static Obj *sf_defspec(Obj *a)
{
    Obj *name = car(a);
    put_prop(name, S_HELP, eval(car(cdr(cdr(a)))));
    put_prop(name, S_ERROR, eval(car(cdr(cdr(cdr(a))))));
    return name;
}

static Obj *s_comspec_add_items(Obj **a, int n)
{
    Obj *items = get_prop(a[0], S_ITEMS), *p;
    (void)n;
    if (!SYMP(a[0])) lisp_error(a[0], "NOT A COMMAND SPEC");
    for (p = a[1]; CONSP(p); p = p->u.c.cdr) {
        Obj *q;
        char *s = obj_string(p->u.c.car, 0);
        int dup = 0;
        for (q = items; CONSP(q); q = q->u.c.cdr) {
            char *t = obj_string(q->u.c.car, 0);
            if (same_name(s, t)) dup = 1;
            free(t);
        }
        free(s);
        if (!dup) items = cons(p->u.c.car, items);
    }
    put_prop(a[0], S_ITEMS, items);
    return a[0];
}

static Obj *s_comred(Obj **a, int n) { (void)n; return comred(a[0]); }
static Obj *s_true(Obj **a, int n) { (void)a; (void)n; return T; }

/* ---------------------------------------------------------- functions */

static int eq(Obj *x, Obj *y)
{
    if (x == y) return 1;
    return FIXP(x) && FIXP(y) && x->u.fix == y->u.fix;
}

static int equal(Obj *x, Obj *y)
{
    if (eq(x, y)) return 1;
    if (x->type != y->type) return 0;
    if (STRP(x)) return !strcmp(x->u.str, y->u.str);
    if (CONSP(x)) return equal(x->u.c.car, y->u.c.car) && equal(x->u.c.cdr, y->u.c.cdr);
    return 0;
}

static long num(Obj *o)
{
    if (!FIXP(o)) lisp_error(o, "NON-NUMERIC VALUE");
    return o->u.fix;
}

static Obj *bool_obj(int b) { return b ? T : NIL; }

/* MacLisp: CAR/CDR of NIL is NIL, of any other atom an error. */
static Obj *xcar(Obj *o) { if (o != NIL && !CONSP(o)) lisp_error(o, "ILLEGAL DATUM - CAR"); return car(o); }
static Obj *xcdr(Obj *o) { if (o != NIL && !CONSP(o)) lisp_error(o, "ILLEGAL DATUM - CDR"); return cdr(o); }
static void proper(Obj *l) { while (CONSP(l)) l = l->u.c.cdr; if (l != NIL) lisp_error(l, "ARGUMENT MUST BE A PROPER LIST"); }
static Obj *s_car(Obj **a, int n) { (void)n; return xcar(a[0]); }
static Obj *s_cdr(Obj **a, int n) { (void)n; return xcdr(a[0]); }
static Obj *s_cadr(Obj **a, int n) { (void)n; return xcar(xcdr(a[0])); }
static Obj *s_cddr(Obj **a, int n) { (void)n; return xcdr(xcdr(a[0])); }
static Obj *s_caar(Obj **a, int n) { (void)n; return xcar(xcar(a[0])); }
static Obj *s_cdar(Obj **a, int n) { (void)n; return xcdr(xcar(a[0])); }
static Obj *s_cons(Obj **a, int n) { (void)n; return cons(a[0], a[1]); }
static Obj *s_list(Obj **a, int n) { Obj *l = NIL; while (n > 0) l = cons(a[--n], l); return l; }
static Obj *s_eq(Obj **a, int n) { (void)n; return bool_obj(eq(a[0], a[1])); }
static Obj *s_neq(Obj **a, int n) { (void)n; return bool_obj(!eq(a[0], a[1])); }
static Obj *s_equal(Obj **a, int n) { (void)n; return bool_obj(equal(a[0], a[1])); }
static Obj *s_null(Obj **a, int n) { (void)n; return bool_obj(a[0] == NIL); }
static Obj *s_atom(Obj **a, int n) { (void)n; return bool_obj(!CONSP(a[0])); }
static Obj *s_numberp(Obj **a, int n) { (void)n; return bool_obj(FIXP(a[0])); }
static Obj *s_symbolp(Obj **a, int n) { (void)n; return bool_obj(SYMP(a[0])); }
static Obj *s_zerop(Obj **a, int n) { (void)n; return bool_obj(num(a[0]) == 0); }
static Obj *s_add1(Obj **a, int n) { (void)n; return mkfix(num(a[0]) + 1); }
static Obj *s_sub1(Obj **a, int n) { (void)n; return mkfix(num(a[0]) - 1); }
static Obj *s_plus(Obj **a, int n) { long v = 0; int i; for (i = 0; i < n; i++) v += num(a[i]); return mkfix(v); }
static Obj *s_times(Obj **a, int n) { long v = 1; int i; for (i = 0; i < n; i++) v *= num(a[i]); return mkfix(v); }
static Obj *s_difference(Obj **a, int n)
{
    long v; int i;
    if (n == 1) return mkfix(-num(a[0]));
    v = num(a[0]);
    for (i = 1; i < n; i++) v -= num(a[i]);
    return mkfix(v);
}
static Obj *s_greaterp(Obj **a, int n) { int i; for (i = 1; i < n; i++) if (!(num(a[i - 1]) > num(a[i]))) return NIL; return T; }
static Obj *s_lessp(Obj **a, int n) { int i; for (i = 1; i < n; i++) if (!(num(a[i - 1]) < num(a[i]))) return NIL; return T; }
static Obj *s_numeq(Obj **a, int n) { (void)n; return bool_obj(num(a[0]) == num(a[1])); }
static Obj *s_length(Obj **a, int n) { (void)n; return mkfix(list_length(a[0])); }

static Obj *s_nth(Obj **a, int n)
{
    long i = num(a[0]);
    Obj *l = a[1];
    (void)n;
    while (i-- > 0) l = xcdr(l);
    return xcar(l);
}

static Obj *s_nthcdr(Obj **a, int n)
{
    long i = num(a[0]);
    Obj *l = a[1];
    (void)n;
    while (i-- > 0) l = cdr(l);
    return l;
}

static Obj *s_reverse(Obj **a, int n) { Obj *r = NIL, *p; (void)n; for (p = a[0]; CONSP(p); p = p->u.c.cdr) r = cons(p->u.c.car, r); return r; }

static Obj *s_nreverse(Obj **a, int n)
{
    Obj *prev = NIL, *p = a[0];
    (void)n;
    while (CONSP(p)) { Obj *next = p->u.c.cdr; p->u.c.cdr = prev; prev = p; p = next; }
    return prev;
}

static Obj *s_append(Obj **a, int n)
{
    Obj *head = NIL, *tail = NIL, *p;
    int i;
    if (n == 0) return NIL;
    for (i = 0; i < n - 1; i++)
        for (p = a[i]; CONSP(p); p = p->u.c.cdr) {
            Obj *c = cons(p->u.c.car, NIL);
            if (head == NIL) head = c; else tail->u.c.cdr = c;
            tail = c;
        }
    if (head == NIL) return a[n - 1];
    tail->u.c.cdr = a[n - 1];
    return head;
}

static Obj *s_nconc(Obj **a, int n)
{
    Obj *head = NIL, *tail = NIL;
    int i;
    for (i = 0; i < n; i++) {
        if (!CONSP(a[i])) { if (i == n - 1 && tail != NIL) tail->u.c.cdr = a[i]; continue; }
        if (head == NIL) head = a[i]; else tail->u.c.cdr = a[i];
        tail = a[i];
        while (CONSP(tail->u.c.cdr)) tail = tail->u.c.cdr;
    }
    return head;
}

static Obj *s_last(Obj **a, int n) { Obj *p = a[0]; (void)n; while (CONSP(p) && CONSP(p->u.c.cdr)) p = p->u.c.cdr; return p; }
static Obj *s_member(Obj **a, int n) { Obj *p; (void)n; proper(a[1]); for (p = a[1]; CONSP(p); p = p->u.c.cdr) if (equal(a[0], p->u.c.car)) return p; return NIL; }
static Obj *s_memq(Obj **a, int n) { Obj *p; (void)n; proper(a[1]); for (p = a[1]; CONSP(p); p = p->u.c.cdr) if (eq(a[0], p->u.c.car)) return p; return NIL; }
static Obj *s_assq(Obj **a, int n) { Obj *p; (void)n; proper(a[1]); for (p = a[1]; CONSP(p); p = p->u.c.cdr) if (CONSP(p->u.c.car) && eq(a[0], p->u.c.car->u.c.car)) return p->u.c.car; return NIL; }
static Obj *s_assoc(Obj **a, int n) { Obj *p; (void)n; proper(a[1]); for (p = a[1]; CONSP(p); p = p->u.c.cdr) if (CONSP(p->u.c.car) && equal(a[0], p->u.c.car->u.c.car)) return p->u.c.car; return NIL; }

static Obj *s_apply(Obj **a, int n)
{
    Obj *args[MAXARGS], *p;
    int k = 0;
    (void)n;
    for (p = a[1]; CONSP(p) && k < MAXARGS; p = p->u.c.cdr) args[k++] = p->u.c.car;
    return apply(a[0], args, k);
}

static Obj *s_funcall(Obj **a, int n) { return apply(a[0], a + 1, n - 1); }

static Obj *s_mapcar(Obj **a, int n)
{
    Obj *head = NIL, *tail = NIL, *p;
    (void)n;
    for (p = a[1]; CONSP(p); p = p->u.c.cdr) {
        Obj *arg = p->u.c.car;
        Obj *c = cons(apply(a[0], &arg, 1), NIL);
        if (head == NIL) head = c; else tail->u.c.cdr = c;
        tail = c;
    }
    return head;
}

static Obj *s_mapc(Obj **a, int n)
{
    Obj *p;
    (void)n;
    for (p = a[1]; CONSP(p); p = p->u.c.cdr) { Obj *arg = p->u.c.car; apply(a[0], &arg, 1); }
    return a[1];
}

static Obj *s_eval(Obj **a, int n) { (void)n; return eval(a[0]); }
static Obj *s_set(Obj **a, int n) { (void)n; if (!SYMP(a[0])) lisp_error(a[0], "CAN'T BE SET"); a[0]->u.sym->value = a[1]; return a[1]; }
static Obj *s_get(Obj **a, int n) { (void)n; return get_prop(a[0], a[1]); }
static Obj *s_putprop(Obj **a, int n) { (void)n; if (!SYMP(a[0])) lisp_error(a[0], "NOT A SYMBOL"); put_prop(a[0], a[2], a[1]); return a[1]; }

/* The characters of an object as PRIN1 (explode) or PRINC (explodec) would
   print it. */
static Obj *explode_obj(Obj *o, int slash, int codes)
{
    char *s = obj_string(o, slash);
    Obj *head = NIL, *tail = NIL;
    char *p;
    for (p = s; *p; p++) {
        Obj *c = cons(codes ? mkfix((unsigned char)*p) : char_sym((unsigned char)*p), NIL);
        if (head == NIL) head = c; else tail->u.c.cdr = c;
        tail = c;
    }
    free(s);
    return head;
}

static Obj *s_explode(Obj **a, int n) { (void)n; return explode_obj(a[0], 1, 0); }
static Obj *s_explodec(Obj **a, int n) { (void)n; return explode_obj(a[0], 0, 0); }
static Obj *s_exploden(Obj **a, int n) { (void)n; return explode_obj(a[0], 0, 1); }

static void chars_to_buf(Obj *l, Buf *b)
{
    for (; CONSP(l); l = l->u.c.cdr) {
        Obj *c = l->u.c.car;
        if (FIXP(c)) buf_add(b, (int)c->u.fix);
        else if (SYMP(c)) buf_str(b, pname(c));
        else if (STRP(c)) buf_str(b, c->u.str);
    }
}

static Obj *s_implode(Obj **a, int n)
{
    Buf b = {0};
    Obj *o;
    (void)n;
    buf_add(&b, 0); b.n = 0; b.p[0] = 0;
    chars_to_buf(a[0], &b);
    o = intern(b.p);
    free(b.p);
    return o;
}

static Obj *s_readlist(Obj **a, int n)
{
    Buf b = {0};
    Reader r;
    Obj *o;
    (void)n;
    buf_add(&b, 0); b.n = 0; b.p[0] = 0;
    chars_to_buf(a[0], &b);
    r.s = b.p; r.pos = 0; r.len = b.n; r.line = 1;
    read_err = 0;
    o = read_form(&r);
    if (read_err) { free(b.p); lisp_error(NULL, "NOT ENOUGH CHARS - READLIST"); }
    free(b.p);
    return o;
}

static Obj *s_samepnamep(Obj **a, int n)
{
    char *x = obj_string(a[0], 0), *y = obj_string(a[1], 0);
    int r = !strcmp(x, y);
    (void)n;
    free(x); free(y);
    return bool_obj(r);
}

/* PRINC/PRIN1/PRINT: a second argument names an output file. */
static void check_file_arg(Obj **a, int n)
{
    if (n > 1 && a[1] != NIL && a[1] != T) lisp_error(a[1], "LOSING OUTPUT FILE SPECS");
}
/* They return T, as MacLisp's do: the game's (princ x (crlf)) passes that T
   on as the output file, which is the terminal. */
static Obj *s_princ(Obj **a, int n) { check_file_arg(a, n); out_obj(a[0], 0); return T; }
static Obj *s_prin1(Obj **a, int n) { check_file_arg(a, n); out_obj(a[0], 1); return T; }
static Obj *s_print(Obj **a, int n) { check_file_arg(a, n); out_char('\n'); out_obj(a[0], 1); out_char(' '); return T; }
static Obj *s_terpri(Obj **a, int n) { (void)a; (void)n; out_char('\n'); return NIL; }
static Obj *s_tyo(Obj **a, int n) { (void)n; out_char((int)num(a[0])); return a[0]; }
static Obj *s_readline(Obj **a, int n) { (void)a; (void)n; return read_line(); }
static Obj *s_readch(Obj **a, int n) { (void)a; (void)n; return read_char(); }
static Obj *s_tyi(Obj **a, int n)
{
    Obj *c = read_char();
    (void)a; (void)n;
    return mkfix((unsigned char)pname(c)[0]);
}

static Obj *s_sleep(Obj **a, int n)
{
    long s = FIXP(a[0]) ? a[0]->u.fix : 1;
    (void)n;
    flush_out();
#ifdef _WIN32
    Sleep((DWORD)(s * 1000));
#else
    sleep((unsigned)s);
#endif
    return T;
}

static Obj *s_ignore(Obj **a, int n) { (void)a; (void)n; return T; }

/* -------------------------------------------------------------- setup */

static void defsubr(const char *name, Subr f, int min, int max)
{
    Sym *s = intern(name)->u.sym;
    s->fnkind = FN_SUBR; s->subr = f; s->minargs = min; s->maxargs = max;
}

static void deffsubr(const char *name, Fsubr f)
{
    Sym *s = intern(name)->u.sym;
    s->fnkind = FN_FSUBR; s->fsubr = f;
}

static void init(void)
{
    Sym *s = newsym("NIL");
    NIL = s->self;
    s->plist = NIL;
    s->next = symtab[hash("NIL")];
    symtab[hash("NIL")] = s;
    T = intern("T");
    DOT = xalloc(sizeof(Obj)); DOT->type = T_SYM; DOT->u.sym = newsym(".");
    S_HELP = intern("HELP-TEXT");
    S_ERROR = intern("ERROR-TEXT");
    S_ITEMS = intern("COMMANDS");
    S_CONFIRM = intern("CONFIRM");
    S_TEXT = intern("TEXT-STRING");

    deffsubr("QUOTE", sf_quote);
    deffsubr("FUNCTION", sf_function);
    deffsubr("SETQ", sf_setq);
    deffsubr("COND", sf_cond);
    deffsubr("AND", sf_and);
    deffsubr("OR", sf_or);
    deffsubr("PROGN", sf_progn);
    deffsubr("PROG1", sf_prog1);
    deffsubr("PROG2", sf_prog2);
    deffsubr("DEFUN", sf_defun);
    deffsubr("LET", sf_let);
    deffsubr("DO", sf_do);
    deffsubr("RETURN", sf_return);
    deffsubr("SSTATUS", sf_sstatus);
    deffsubr("HERALD", sf_ignore);
    deffsubr("DECLARE", sf_ignore);
    deffsubr("LET-COMRED", sf_let_comred);
    deffsubr("DEFSPEC", sf_defspec);

    defsubr("CAR", s_car, 1, 1);
    defsubr("CDR", s_cdr, 1, 1);
    defsubr("CADR", s_cadr, 1, 1);
    defsubr("CDDR", s_cddr, 1, 1);
    defsubr("CAAR", s_caar, 1, 1);
    defsubr("CDAR", s_cdar, 1, 1);
    defsubr("CONS", s_cons, 2, 2);
    defsubr("LIST", s_list, 0, -1);
    defsubr("EQ", s_eq, 2, 2);
    defsubr("NEQ", s_neq, 2, 2);
    defsubr("EQUAL", s_equal, 2, 2);
    defsubr("NULL", s_null, 1, 1);
    defsubr("NOT", s_null, 1, 1);
    defsubr("ATOM", s_atom, 1, 1);
    defsubr("NUMBERP", s_numberp, 1, 1);
    defsubr("FIXP", s_numberp, 1, 1);
    defsubr("SYMBOLP", s_symbolp, 1, 1);
    defsubr("ZEROP", s_zerop, 1, 1);
    defsubr("ADD1", s_add1, 1, 1);
    defsubr("1+", s_add1, 1, 1);
    defsubr("SUB1", s_sub1, 1, 1);
    defsubr("1-", s_sub1, 1, 1);
    defsubr("PLUS", s_plus, 0, -1);
    defsubr("+", s_plus, 0, -1);
    defsubr("TIMES", s_times, 0, -1);
    defsubr("*", s_times, 0, -1);
    defsubr("DIFFERENCE", s_difference, 1, -1);
    defsubr("-", s_difference, 1, -1);
    defsubr("GREATERP", s_greaterp, 1, -1);
    defsubr(">", s_greaterp, 1, -1);
    defsubr("LESSP", s_lessp, 1, -1);
    defsubr("<", s_lessp, 1, -1);
    defsubr("=", s_numeq, 2, 2);
    defsubr("LENGTH", s_length, 1, 1);
    defsubr("NTH", s_nth, 2, 2);
    defsubr("NTHCDR", s_nthcdr, 2, 2);
    defsubr("REVERSE", s_reverse, 1, 1);
    defsubr("NREVERSE", s_nreverse, 1, 1);
    defsubr("APPEND", s_append, 0, -1);
    defsubr("NCONC", s_nconc, 0, -1);
    defsubr("LAST", s_last, 1, 1);
    defsubr("MEMBER", s_member, 2, 2);
    defsubr("MEMQ", s_memq, 2, 2);
    defsubr("ASSQ", s_assq, 2, 2);
    defsubr("ASSOC", s_assoc, 2, 2);
    defsubr("APPLY", s_apply, 2, 2);
    defsubr("FUNCALL", s_funcall, 1, -1);
    defsubr("MAPCAR", s_mapcar, 2, 2);
    defsubr("MAPC", s_mapc, 2, 2);
    defsubr("EVAL", s_eval, 1, 1);
    defsubr("SET", s_set, 2, 2);
    defsubr("GET", s_get, 2, 2);
    defsubr("PUTPROP", s_putprop, 3, 3);
    defsubr("EXPLODE", s_explode, 1, 1);
    defsubr("EXPLODEC", s_explodec, 1, 1);
    defsubr("EXPLODEN", s_exploden, 1, 1);
    defsubr("IMPLODE", s_implode, 1, 1);
    defsubr("READLIST", s_readlist, 1, 1);
    defsubr("SAMEPNAMEP", s_samepnamep, 2, 2);
    defsubr("PRINC", s_princ, 1, 2);
    defsubr("PRIN1", s_prin1, 1, 2);
    defsubr("PRINT", s_print, 1, 2);
    defsubr("TERPRI", s_terpri, 0, 1);
    defsubr("TYO", s_tyo, 1, 2);
    defsubr("TYI", s_tyi, 0, 1);
    defsubr("READLINE", s_readline, 0, 2);
    defsubr("READCH", s_readch, 0, 2);
    defsubr("SLEEP", s_sleep, 1, 1);
    defsubr("LOAD", s_ignore, 1, 2);       /* the libraries are built in */
    defsubr("VALRET", s_ignore, 0, 1);

    defsubr("COMRED", s_comred, 1, 1);
    defsubr("COMRED-INITIALIZE", s_true, 0, 0);
    defsubr("COMRED-FORCE-GUIDEWORD", s_force_guideword, 1, 1);
    defsubr("COMSPEC-ADD-ITEMS", s_comspec_add_items, 2, 3);

    /* MacLisp's reader defaults */
    intern("IBASE")->u.sym->value = mkfix(8);
    intern("BASE")->u.sym->value = mkfix(8);
}

static void load_source(const char *src, const char *name)
{
    Reader r;
    Frame f;
    r.s = src; r.pos = 0; r.len = strlen(src); r.line = 1;
    for (;;) {
        Obj *form;
        int line;
        skip_ws(&r);
        if (rd_peek(&r) < 0) break;
        line = r.line;
        read_err = 0;
        form = read_form(&r);
        if (read_err) {
            fprintf(stderr, "%s:%d: unbalanced form\n", name, line);
            break;
        }
        push_frame(&f, FR_GUARD);
        if (setjmp(f.jb)) {
            pop_frame(&f);
            if (debug) fprintf(stderr, "%s:%d: error while loading\n", name, line);
            continue;
        }
        eval(form);
        pop_frame(&f);
    }
}

static void usage(void)
{
    puts("Usage: dunnet [--no-fixes] [--debug]\n"
         "\n"
         "  --no-fixes  run the archived source exactly, bugs included\n"
         "  --debug     report errors while loading on standard error\n"
         "\n"
         "In the TOPS-20 part: ? lists the choices, ESC completes a keyword,\n"
         "DEL deletes, ^U starts the line again, ^R retypes it.");
}

int main(int argc, char **argv)
{
    int i;
    Frame top;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--no-fixes")) fixes = 0;
        else if (!strcmp(argv[i], "--debug") || !strcmp(argv[i], "-debug")) debug = 1;
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "/?")) { usage(); return 0; }
        else { fprintf(stderr, "dunnet: unknown option %s (try --help)\n", argv[i]); return 2; }
    }
#ifdef _WIN32
    {
        DWORD mode;
        hin = GetStdHandle(STD_INPUT_HANDLE);
        is_console = GetConsoleMode(hin, &mode) != 0;
    }
#else
    is_console = 0;
#endif
    init();
    load_source(SRC_FOO, "foo.lsp");
    if (fixes) load_source(SRC_FIXES, "fixes.lsp");
    guarded[nguarded++] = intern("LISTEN");
    guarded[nguarded++] = intern("LIST-WORDS");
    guarded[nguarded++] = intern("PARSE");

    push_frame(&top, FR_GUARD);
    if (setjmp(top.jb)) {
        fresh_line();
        flush_out();
        return 1;
    }
    eval(cons(intern("DUNGEON"), NIL));
    pop_frame(&top);
    fresh_line();
    flush_out();
    return 0;
}
