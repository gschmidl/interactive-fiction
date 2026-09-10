/*
 * FAEMAR -- a small dynamically-scoped Lisp interpreter that runs the
 * original 1982 Faemar adventure source verbatim (game.lsp engine +
 * faemar world data), compiled to a native Windows executable.
 *
 * Rather than hand-porting each game routine to C, this file implements
 * just enough of the original MacLisp-style dialect (reader, evaluator,
 * ~70 primitives) to interpret the actual archived source text, embedded
 * unmodified in embedded_sources.h. This preserves original behavior
 * (including its quirks) exactly, since the real logic never leaves Lisp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>

#include "embedded_sources.h"

/* ---------------------------------------------------------------- types */

typedef struct Node Node;
typedef struct Symbol Symbol;

typedef enum { T_CONS, T_SYM, T_NUM, T_STR } NType;

struct Node {
    NType type;
    union {
        struct { Node *car, *cdr; } cons;
        Symbol *sym;
        double num;
        char *str;
    } u;
};

typedef enum { F_NONE, F_EXPR, F_FEXPR, F_NATIVE } FKind;

typedef Node* (*NativeFn)(Symbol *self, Node **args, int n);

struct Symbol {
    char *name;
    Node *value;
    int bound;
    Node *plist;       /* flat list: prop1 val1 prop2 val2 ... */
    FKind fkind;
    Node *params;      /* param list (DE/DF) */
    Node *body;        /* body forms (DE/DF) */
    NativeFn native;
    int has_array;
    int arr_lo, arr_hi;
    Node **arr;
    Symbol *hnext;
};

#define NIL ((Node*)0)

static int g_debug = 0;

#define CALL_TRACE_MAX 4000
static const char *call_trace[CALL_TRACE_MAX];
static int call_depth = 0;

/* Top-level recovery point: some interactions in the original 1982 engine
   (e.g. certain object/verb combinations re-triggering their own handler
   through MOVE's built-in OBJECT-ACTION override check) are genuine
   infinite-recursion traps in the game logic itself, not corruption in the
   archived source. Rather than "fixing" that original control flow, treat
   a runaway command the way a real Lisp system would treat a genuine stack
   overflow: unwind to the top level and keep the session alive, instead of
   crashing the whole process. */
static jmp_buf top_level_recovery;
static int have_recovery = 0;
static int recovery_binding_mark = 0;
static int recovery_prog_mark = 0;

/* --------------------------------------------------------- arena alloc */

#define CHUNK_SZ (1<<20)
static char *arena_cur = NULL;
static size_t arena_left = 0;

static void *arena_alloc(size_t sz) {
    sz = (sz + 7) & ~(size_t)7;
    if (sz > arena_left) {
        size_t csz = CHUNK_SZ;
        if (sz > csz) csz = sz;
        arena_cur = (char*)malloc(csz);
        if (!arena_cur) { fprintf(stderr, "out of memory\n"); exit(1); }
        arena_left = csz;
    }
    void *p = arena_cur;
    arena_cur += sz;
    arena_left -= sz;
    return p;
}

static Node *new_node(NType t) {
    Node *n = (Node*)arena_alloc(sizeof(Node));
    n->type = t;
    return n;
}

static Node *mk_num(double v) { Node *n = new_node(T_NUM); n->u.num = v; return n; }
static Node *mk_str(const char *s) {
    Node *n = new_node(T_STR);
    size_t len = strlen(s);
    char *c = (char*)arena_alloc(len+1);
    memcpy(c, s, len+1);
    n->u.str = c;
    return n;
}
static Node *cons(Node *a, Node *d) {
    Node *n = new_node(T_CONS);
    n->u.cons.car = a; n->u.cons.cdr = d;
    return n;
}
static Node *mk_sym_node(Symbol *s) { Node *n = new_node(T_SYM); n->u.sym = s; return n; }

static int is_cons(Node *n) { return n && n->type == T_CONS; }
static int is_sym(Node *n)  { return n && n->type == T_SYM; }
static int is_num(Node *n)  { return n && n->type == T_NUM; }

static Node *car(Node *n) { return is_cons(n) ? n->u.cons.car : NIL; }
static Node *cdr(Node *n) { return is_cons(n) ? n->u.cons.cdr : NIL; }

/* ------------------------------------------------------------- symbols */

#define HTAB_SZ 2048
static Symbol *htab[HTAB_SZ];

static unsigned hash_str(const char *s) {
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

static Symbol *intern(const char *name) {
    unsigned h = hash_str(name) % HTAB_SZ;
    for (Symbol *s = htab[h]; s; s = s->hnext)
        if (strcmp(s->name, name) == 0) return s;
    Symbol *s = (Symbol*)arena_alloc(sizeof(Symbol));
    memset(s, 0, sizeof(Symbol));
    size_t len = strlen(name);
    char *nm = (char*)arena_alloc(len+1);
    memcpy(nm, name, len+1);
    s->name = nm;
    s->hnext = htab[h];
    htab[h] = s;
    return s;
}

/* Well-known symbols, cached for fast pointer comparisons */
static Symbol *sym_T, *sym_QUOTE, *sym_COND, *sym_SETQ, *sym_PROG, *sym_PROG2,
    *sym_DE, *sym_DF, *sym_LAMBDA, *sym_FUNCTION, *sym_AND, *sym_OR,
    *sym_GO, *sym_RETURN, *sym_STORE, *sym_ARRAY, *sym_DSKINQ, *sym_DECIMAL,
    *sym_DEFPROP, *sym_SET, *sym_FAEMAR, *sym_LAPFNS;

static Node *SYMV(Symbol *s) { return mk_sym_node(s); }

/* T self-evaluates */
static Node *node_T;

/* -------------------------------------------------------------- reader */

#define PENDING_MAX 64
typedef struct { const char *buf; size_t len; size_t pos; int pending[PENDING_MAX]; int pending_n; } Reader;

static void rd_init(Reader *r, const char *buf) { r->buf = buf; r->len = strlen(buf); r->pos = 0; r->pending_n = 0; }

/* pending[] is a LIFO stack: pending[pending_n-1] is the next char to read */
static int rd_peek(Reader *r) {
    if (r->pending_n > 0) return r->pending[r->pending_n - 1];
    if (r->pos >= r->len) return -1;
    return (unsigned char)r->buf[r->pos];
}
static int rd_next(Reader *r) {
    if (r->pending_n > 0) return r->pending[--r->pending_n];
    if (r->pos >= r->len) return -1;
    return (unsigned char)r->buf[r->pos++];
}
/* pushes a single char back; call in REVERSE order (last-consumed first) to
   restore a multi-char sequence, since this is a LIFO stack */
static void rd_unread(Reader *r, int c) {
    if (r->pending_n < PENDING_MAX) r->pending[r->pending_n++] = c;
}

static int is_hard_delim(int c) {
    /* NOTE: ' is deliberately NOT a hard delimiter here -- it only acts as
       the quote reader-macro when immediately followed by '(' or another
       quote (see read_form). Elsewhere (e.g. inside "don't", or standalone
       as a literal printable quote-mark character) it is an ordinary
       token constituent, so word-internal apostrophes read correctly. */
    return c < 0 || c=='(' || c==')' || c=='"' || c==';' || isspace(c);
}

static void skip_ws(Reader *r) {
    for (;;) {
        int c = rd_peek(r);
        if (c < 0) return;
        if (isspace(c)) { rd_next(r); continue; }
        if (c == ';') { while ((c = rd_next(r)) >= 0 && c != '\n') ; continue; }
        break;
    }
}

static int looks_like_number(const char *s, int n) {
    int i = 0;
    if (n == 0) return 0;
    if (s[i]=='+'||s[i]=='-') i++;
    int digits_before = 0;
    while (i<n && isdigit((unsigned char)s[i])) { i++; digits_before++; }
    int digits_after = 0;
    if (i<n && s[i]=='.') { i++; while (i<n && isdigit((unsigned char)s[i])) { i++; digits_after++; } }
    if (digits_before==0 && digits_after==0) return 0;
    if (i<n && (s[i]=='E'||s[i]=='e')) {
        int save = i; i++;
        if (i<n && (s[i]=='+'||s[i]=='-')) i++;
        int expd = 0;
        while (i<n && isdigit((unsigned char)s[i])) { i++; expd++; }
        if (expd==0) { i = save; return 0; }
    }
    return i==n;
}

static Node *DOT_MARKER_NODE;

static Node *read_form(Reader *r);

static Node *read_token_form(Reader *r) {
    char buf[8192];
    char esc[8192];
    int n = 0;
    for (;;) {
        int c = rd_peek(r);
        if (c < 0) break;
        if (c == '/') {
            rd_next(r);
            int c2 = rd_next(r);
            if (c2 < 0) break;
            if (n < (int)sizeof(buf)-1) { buf[n]=(char)c2; esc[n]=1; n++; }
            continue;
        }
        if (is_hard_delim(c)) break;
        rd_next(r);
        if (n < (int)sizeof(buf)-1) { buf[n]=(char)c; esc[n]=0; n++; }
    }
    buf[n] = 0;

    if (looks_like_number(buf, n)) {
        return mk_num(atof(buf));
    }

    int dotpos = -1;
    for (int i = 0; i < n; i++) if (buf[i]=='.' && !esc[i]) { dotpos = i; break; }

    if (dotpos >= 0) {
        /* push back everything strictly after the dot, so it's re-read as
           the start of the next token (may be more than one character,
           e.g. "LAP.FNS" splits into "LAP", ".", then "FNS") */
        for (int i = n-1; i > dotpos; i--) rd_unread(r, (unsigned char)buf[i]);
        if (dotpos == 0) {
            /* the dot itself is the whole token: it's the dotted-pair marker */
            return DOT_MARKER_NODE;
        }
        /* push the dot back too, to be read fresh as its own token next */
        rd_unread(r, (unsigned char)buf[dotpos]);
        buf[dotpos] = 0;
        n = dotpos;
    }
    if (n == 0) {
        /* shouldn't happen, but guard: just recurse to read next form */
        return read_form(r);
    }
    if (strcmp(buf, "NIL") == 0) return NIL;
    if (strcmp(buf, "T") == 0) return node_T;
    return SYMV(intern(buf));
}

static Node *read_list(Reader *r) {
    Node *head = NIL, *tail = NIL;
    for (;;) {
        skip_ws(r);
        int c = rd_peek(r);
        if (c < 0) return head; /* unexpected EOF, be lenient */
        if (c == ')') { rd_next(r); return head; }
        Node *form = read_form(r);
        if (form == DOT_MARKER_NODE) {
            skip_ws(r);
            Node *tailv = read_form(r);
            skip_ws(r);
            if (rd_peek(r) == ')') rd_next(r);
            if (tail) tail->u.cons.cdr = tailv; else head = tailv;
            return head;
        }
        Node *cell = cons(form, NIL);
        if (tail) tail->u.cons.cdr = cell; else head = cell;
        tail = cell;
    }
}

static Node *read_form(Reader *r) {
    skip_ws(r);
    int c = rd_peek(r);
    if (c < 0) return NULL; /* real EOF signalled by caller checking position */
    if (c == '(') { rd_next(r); return read_list(r); }
    if (c == '\'') {
        /* Quote reader-macro, UNLESS immediately followed by whitespace or
           an escape ('/'): those mark a literal apostrophe character --
           e.g. a standalone printable quote-mark word (' %NS%), or part of
           an escaped literal ('/. -> the two characters "'."), or a
           word-internal apostrophe like "don't" (handled by
           is_hard_delim not treating ' as special there at all). Every
           other case (letter, digit, '(', another '\'', '"', '%', ...) is
           the ordinary "quote the next form" usage, e.g. 'eat, '%NULL%,
           '(...), ''(...), '"str". */
        rd_next(r);
        int c2 = rd_peek(r);
        if (c2 < 0 || isspace(c2) || c2 == '/') {
            rd_unread(r, '\'');
            return read_token_form(r);
        }
        Node *inner = read_form(r);
        return cons(SYMV(sym_QUOTE), cons(inner, NIL));
    }
    if (c == '"') {
        rd_next(r);
        char buf[8192]; int n = 0;
        for (;;) {
            int ch = rd_next(r);
            if (ch < 0 || ch == '"') break;
            if (n < (int)sizeof(buf)-1) buf[n++] = (char)ch;
        }
        buf[n] = 0;
        return mk_str(buf);
    }
    return read_token_form(r);
}

/* top-level: returns 1 and sets *out if a form was read, 0 at EOF */
static int read_top(Reader *r, Node **out) {
    skip_ws(r);
    if (rd_peek(r) < 0) return 0;
    *out = read_form(r);
    return 1;
}

/* ------------------------------------------------------- binding stack */

typedef struct { Symbol *sym; Node *old_value; int old_bound; } Binding;
#define MAX_BINDINGS 200000
static Binding binding_stack[MAX_BINDINGS];
static int binding_top = 0;

static void push_binding(Symbol *s, Node *newval) {
    if (binding_top >= MAX_BINDINGS) { fprintf(stderr, "binding stack overflow\n"); exit(1); }
    binding_stack[binding_top].sym = s;
    binding_stack[binding_top].old_value = s->value;
    binding_stack[binding_top].old_bound = s->bound;
    binding_top++;
    s->value = newval;
    s->bound = 1;
}

static void unwind_bindings(int mark) {
    while (binding_top > mark) {
        binding_top--;
        Binding *b = &binding_stack[binding_top];
        b->sym->value = b->old_value;
        b->sym->bound = b->old_bound;
    }
}

/* -------------------------------------------------------- prog frames */

typedef struct {
    Node *body;          /* full body list (labels + forms) */
    jmp_buf buf;
    Node *return_value;
    Node *goto_label;     /* set by GO before longjmp */
    int binding_mark;
    int call_depth_mark;  /* longjmp skips normal call_depth-- unwinding;
                              restore it explicitly at the catching frame */
} ProgFrame;

#define MAX_PROG_DEPTH 4096
static ProgFrame prog_stack[MAX_PROG_DEPTH];
static int prog_top = 0;

/* ------------------------------------------------------------- output */

static int g_column = 0;
static int g_linelength = 72;

static void out_reset_col(void) { g_column = 0; }

static void out_char(int c) {
    putchar(c);
    if (c == '\n') g_column = 0; else g_column++;
}

static void out_str(const char *s) {
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) out_char(*p);
}

static void do_quit(void) {
    printf("\nGoodbye!\n");
    fflush(stdout);
    exit(0);
}

/* ------------------------------------------------------------- eval.h */

static Node *eval(Node *form);
static Node *eval_body(Node *body);
static Node *apply_fn(Node *fn, Node **args, int n);
static void load_source(const char *src);

static int eq_p(Node *a, Node *b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;
    if (a->type == T_NUM) return a->u.num == b->u.num;
    if (a->type == T_SYM) return a->u.sym == b->u.sym;
    return 0;
}

static int equal_p(Node *a, Node *b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;
    switch (a->type) {
        case T_NUM: return a->u.num == b->u.num;
        case T_SYM: return a->u.sym == b->u.sym;
        case T_STR: return strcmp(a->u.str, b->u.str) == 0;
        case T_CONS: return equal_p(a->u.cons.car, b->u.cons.car) && equal_p(a->u.cons.cdr, b->u.cons.cdr);
    }
    return 0;
}

static int list_len(Node *l) { int n=0; while (is_cons(l)) { n++; l=l->u.cons.cdr; } return n; }

/* get printed name of an atom (symbol, number, string) into buf */
static void print_name(Node *a, char *buf, size_t bufsz) {
    if (!a) { snprintf(buf, bufsz, "NIL"); return; }
    if (a->type == T_SYM) { snprintf(buf, bufsz, "%s", a->u.sym->name); return; }
    if (a->type == T_STR) { snprintf(buf, bufsz, "%s", a->u.str); return; }
    if (a->type == T_NUM) {
        double v = a->u.num;
        if (v == (long long)v) snprintf(buf, bufsz, "%lld", (long long)v);
        else snprintf(buf, bufsz, "%g", v);
        return;
    }
    snprintf(buf, bufsz, "?");
}

static Node *plist_get(Symbol *s, Symbol *prop) {
    Node *p = s->plist;
    while (is_cons(p) && is_cons(p->u.cons.cdr)) {
        Node *k = p->u.cons.car;
        Node *v = p->u.cons.cdr->u.cons.car;
        if (is_sym(k) && k->u.sym == prop) return v;
        p = p->u.cons.cdr->u.cons.cdr;
    }
    return NIL;
}

static void plist_put(Symbol *s, Symbol *prop, Node *val) {
    Node *p = s->plist;
    while (is_cons(p) && is_cons(p->u.cons.cdr)) {
        Node *k = p->u.cons.car;
        if (is_sym(k) && k->u.sym == prop) { p->u.cons.cdr->u.cons.car = val; return; }
        p = p->u.cons.cdr->u.cons.cdr;
    }
    s->plist = cons(SYMV(prop), cons(val, s->plist));
}

static void plist_rem(Symbol *s, Symbol *prop) {
    Node *p = s->plist, *prev = NIL, *newlist = NIL, *tail = NIL;
    while (is_cons(p) && is_cons(p->u.cons.cdr)) {
        Node *k = p->u.cons.car;
        Node *v = p->u.cons.cdr->u.cons.car;
        Node *rest = p->u.cons.cdr->u.cons.cdr;
        if (!(is_sym(k) && k->u.sym == prop)) {
            Node *cell = cons(k, cons(v, NIL));
            if (tail) { tail->u.cons.cdr->u.cons.cdr = cell; tail = cell; }
            else { newlist = cell; tail = cell; }
        }
        p = rest;
    }
    (void)prev;
    s->plist = newlist;
}

/* ---------------------------------------------------------- TYI / GETLINE support */

static int tyi_pending = -1;

/* START's body opens with two unconditional (TYI) calls, meant on the
   original PDP-10 to discard leftover characters left in the terminal
   buffer from invoking "(START)" at the Lisp top level. A standalone
   .exe has no such leftover input, so without help those two calls would
   silently eat the player's first two real keystrokes. Prime two harmless
   throwaway characters for them to consume instead -- a pure I/O-layer
   adaptation, not a change to any game logic. */
static int startup_prime_remaining = 2;

static int do_tyi(void) {
    if (startup_prime_remaining > 0) { startup_prime_remaining--; return ' '; }
    if (tyi_pending >= 0) { int c = tyi_pending; tyi_pending = -1; return c; }
    int c = getchar();
    if (c == EOF) { do_quit(); }
    if (c == '\n') { tyi_pending = 10; return 13; }
    return c;
}

/* ---------------------------------------------------------------- RAN */

static void seed_rng(void) { srand((unsigned)time(NULL)); }

static double do_ran(double nd) {
    if (nd > 1e9) { seed_rng(); return 0; }
    long n = (long)nd;
    if (n <= 0) return 0;
    return (double)(rand() % n);
}

/* --------------------------------------------------------------- BOOLE */

static double do_boole(long code, unsigned long a, unsigned long b) {
    unsigned long result = 0;
    for (int bit = 0; bit < 32; bit++) {
        int abit = (a >> bit) & 1;
        int bbit = (b >> bit) & 1;
        int idx = (bbit << 1) | abit; /* 0..3 */
        int outbit = (code >> (3 - idx)) & 1;
        if (outbit) result |= (1UL << bit);
    }
    return (double)result;
}

/* ---------------------------------------------------------------- EVAL */

static Node *eval_list_values(Node *forms) {
    /* returns a freshly-consed list of evaluated forms */
    if (!is_cons(forms)) return NIL;
    Node *head = NIL, *tail = NIL;
    for (Node *p = forms; is_cons(p); p = p->u.cons.cdr) {
        Node *v = eval(p->u.cons.car);
        Node *cell = cons(v, NIL);
        if (tail) tail->u.cons.cdr = cell; else head = cell;
        tail = cell;
    }
    return head;
}

static int list_to_array(Node *l, Node **arr, int maxn) {
    int n = 0;
    while (is_cons(l) && n < maxn) { arr[n++] = l->u.cons.car; l = l->u.cons.cdr; }
    return n;
}

static Node *eval_body(Node *body) {
    Node *result = NIL;
    for (Node *p = body; is_cons(p); p = p->u.cons.cdr) {
        result = eval(p->u.cons.car);
    }
    return result;
}

static Node *call_expr_like(Node *params, Node *body, Node **args, int nargs) {
    int mark = binding_top;
    int i = 0;
    for (Node *p = params; is_cons(p); p = p->u.cons.cdr, i++) {
        Symbol *ps = p->u.cons.car->u.sym;
        Node *val = (i < nargs) ? args[i] : NIL;
        push_binding(ps, val);
    }
    Node *result = eval_body(body);
    unwind_bindings(mark);
    return result;
}

static Node *apply_fn(Node *fn, Node **args, int n) {
    if (is_sym(fn)) {
        Symbol *s = fn->u.sym;
        if (s == sym_OR) { for (int i=0;i<n;i++) if (args[i]) return args[i]; return NIL; }
        if (s == sym_AND) { Node *r = node_T; for (int i=0;i<n;i++) { r = args[i]; if (!r) return NIL; } return r; }
        if (s->fkind == F_NATIVE) return s->native(s, args, n);
        if (s->fkind == F_EXPR) return call_expr_like(s->params, s->body, args, n);
        if (s->fkind == F_FEXPR) {
            /* apply-ing a fexpr with evaluated args: bind formal to arg list */
            Node *arglist = NIL, *tail=NIL;
            for (int i=0;i<n;i++) { Node *cell=cons(args[i],NIL); if(tail) tail->u.cons.cdr=cell; else arglist=cell; tail=cell; }
            int mark = binding_top;
            push_binding(s->params->u.cons.car->u.sym, arglist);
            Node *result = eval_body(s->body);
            unwind_bindings(mark);
            return result;
        }
        if (s->has_array) {
            int idx = (int)(is_num(args[0]) ? args[0]->u.num : 0);
            if (idx < s->arr_lo || idx > s->arr_hi) return NIL;
            return s->arr[idx - s->arr_lo];
        }
        return NIL;
    }
    if (is_cons(fn) && is_sym(car(fn)) && car(fn)->u.sym->name && strcmp(car(fn)->u.sym->name, "LAMBDA") == 0) {
        Node *params = car(cdr(fn));
        Node *body = cdr(cdr(fn));
        return call_expr_like(params, body, args, n);
    }
    return NIL;
}

static Node *do_eval_form(Node *form); /* forward */

static Node *eval(Node *form) {
    if (!form) return NIL;
    switch (form->type) {
        case T_NUM: case T_STR: return form;
        case T_SYM: {
            Symbol *s = form->u.sym;
            if (s == sym_T) return node_T;
            if (!s->bound) return NIL;
            return s->value;
        }
        case T_CONS: {
            /* universal recursion guard: catches runaway recursion through
               every call path (direct calls, MAPC/MAPCAR/APPLY, FEXPRs) */
            const char *name = is_sym(form->u.cons.car) ? form->u.cons.car->u.sym->name : "(...)";
            if (call_depth < CALL_TRACE_MAX) call_trace[call_depth] = name;
            call_depth++;
            if (call_depth >= CALL_TRACE_MAX) {
                if (g_debug) {
                    fprintf(stderr, "runaway recursion detected, last calls (innermost first):\n");
                    int top = call_depth < CALL_TRACE_MAX ? call_depth : CALL_TRACE_MAX;
                    for (int i = top - 1; i >= 0 && i >= top - 100; i--)
                        fprintf(stderr, "  %s\n", call_trace[i]);
                    fflush(stderr);
                }
                if (have_recovery) {
                    unwind_bindings(recovery_binding_mark);
                    prog_top = recovery_prog_mark;
                    call_depth = 0;
                    longjmp(top_level_recovery, 1);
                }
                fprintf(stderr, "runaway recursion before startup completed -- aborting\n");
                exit(1);
            }
            Node *result = do_eval_form(form);
            call_depth--;
            return result;
        }
    }
    return NIL;
}

static Node *sf_cond(Node *clauses) {
    for (Node *p = clauses; is_cons(p); p = p->u.cons.cdr) {
        Node *clause = p->u.cons.car;
        Node *test = car(clause);
        Node *tv = eval(test);
        if (tv) {
            Node *body = cdr(clause);
            if (!is_cons(body)) return tv;
            return eval_body(body);
        }
    }
    return NIL;
}

static Node *sf_setq(Node *args) {
    Node *result = NIL;
    Node *p = args;
    while (is_cons(p)) {
        Node *varf = p->u.cons.car;
        p = p->u.cons.cdr;
        Node *valf = is_cons(p) ? p->u.cons.car : NIL;
        if (is_cons(p)) p = p->u.cons.cdr;
        if (is_sym(varf)) {
            Node *val = eval(valf);
            Symbol *vs = varf->u.sym;
            vs->value = val; vs->bound = 1;
            result = val;
        }
    }
    return result;
}

static Node *sf_prog(Node *args) {
    Node *locals = car(args);
    Node *body = cdr(args);

    if (prog_top >= MAX_PROG_DEPTH) { fprintf(stderr, "prog stack overflow\n"); exit(1); }
    ProgFrame *fr = &prog_stack[prog_top++];
    fr->body = body;
    fr->return_value = NIL;
    fr->goto_label = NIL;

    fr->binding_mark = binding_top;
    fr->call_depth_mark = call_depth;
    for (Node *p = locals; is_cons(p); p = p->u.cons.cdr) {
        if (is_sym(p->u.cons.car)) push_binding(p->u.cons.car->u.sym, NIL);
    }

    Node *result = NIL;
    int sig = setjmp(fr->buf);
    if (sig == 0) {
        for (Node *p = body; is_cons(p); p = p->u.cons.cdr) {
            Node *f = p->u.cons.car;
            if (is_sym(f)) continue; /* label */
            eval(f);
        }
        result = NIL;
    } else if (sig == 2) {
        /* longjmp from RETURN skips the normal call_depth-- unwinding
           in every C frame it jumped past -- restore it here. */
        call_depth = fr->call_depth_mark;
        result = fr->return_value;
    } else {
        /* sig == 1: goto within this frame -- same call_depth leak concern */
        call_depth = fr->call_depth_mark;
        Node *lbl = fr->goto_label;
        Node *p = body;
        while (is_cons(p) && !(is_sym(p->u.cons.car) && p->u.cons.car->u.sym == lbl->u.sym)) p = p->u.cons.cdr;
        for (; is_cons(p); p = p->u.cons.cdr) {
            Node *f = p->u.cons.car;
            if (is_sym(f)) continue;
            eval(f);
        }
        result = NIL;
        /* fall through: normal completion after resumed execution */
        unwind_bindings(fr->binding_mark);
        prog_top--;
        return result;
    }

    unwind_bindings(fr->binding_mark);
    prog_top--;
    return result;
}

static Node *sf_go(Node *args) {
    Node *label = car(args);
    if (!is_sym(label)) return NIL;
    for (int i = prog_top - 1; i >= 0; i--) {
        Node *p = prog_stack[i].body;
        while (is_cons(p)) {
            if (is_sym(p->u.cons.car) && p->u.cons.car->u.sym == label->u.sym) {
                prog_stack[i].goto_label = label;
                longjmp(prog_stack[i].buf, 1);
            }
            p = p->u.cons.cdr;
        }
    }
    fprintf(stderr, "warning: GO to unknown label %s\n", label->u.sym->name);
    return NIL;
}

static Node *sf_return(Node *args) {
    Node *val = is_cons(args) ? eval(args->u.cons.car) : NIL;
    if (prog_top <= 0) { fprintf(stderr, "warning: RETURN outside PROG\n"); return NIL; }
    prog_stack[prog_top-1].return_value = val;
    longjmp(prog_stack[prog_top-1].buf, 2);
}

static Node *sf_prog2(Node *args) {
    Node *second = NIL;
    int idx = 0;
    for (Node *p = args; is_cons(p); p = p->u.cons.cdr, idx++) {
        Node *v = eval(p->u.cons.car);
        if (idx == 1) second = v;
    }
    return second;
}

static Node *sf_and(Node *args) {
    Node *r = node_T;
    for (Node *p = args; is_cons(p); p = p->u.cons.cdr) {
        r = eval(p->u.cons.car);
        if (!r) return NIL;
    }
    return r;
}
static Node *sf_or(Node *args) {
    for (Node *p = args; is_cons(p); p = p->u.cons.cdr) {
        Node *r = eval(p->u.cons.car);
        if (r) return r;
    }
    return NIL;
}

static Node *sf_de(Node *args, int fexpr) {
    Node *name = car(args);
    Node *params = car(cdr(args));
    Node *body = cdr(cdr(args));
    if (!is_sym(name)) return NIL;
    Symbol *s = name->u.sym;
    s->fkind = fexpr ? F_FEXPR : F_EXPR;
    s->params = params;
    s->body = body;
    return name;
}

static Node *sf_function(Node *arg) {
    if (is_sym(arg)) return arg;
    if (is_cons(arg)) return arg; /* (LAMBDA ...) form, used as-is */
    return arg;
}

static Node *sf_defprop(Node *args) {
    Node *name = car(args);
    Node *val = car(cdr(args));
    Node *prop = car(cdr(cdr(args)));
    if (is_sym(name) && is_sym(prop)) plist_put(name->u.sym, prop->u.sym, val);
    return name;
}

static Node *sf_array(Node *args) {
    Node *name = car(args);
    /* second arg (type) ignored */
    Node *dimsform = car(cdr(cdr(args)));
    Node *dims = eval(dimsform);
    if (!is_sym(name)) return NIL;
    Symbol *s = name->u.sym;
    int lo = 0, hi = 0;
    if (is_cons(dims)) {
        Node *a = car(dims), *d = cdr(dims);
        if (is_num(a)) lo = (int)a->u.num;
        if (is_num(d)) hi = (int)d->u.num;
        else if (is_cons(d) && is_num(car(d))) hi = (int)car(d)->u.num;
    }
    if (hi < lo) hi = lo;
    s->has_array = 1;
    s->arr_lo = lo; s->arr_hi = hi;
    s->arr = (Node**)arena_alloc(sizeof(Node*) * (size_t)(hi - lo + 1));
    for (int i = 0; i <= hi - lo; i++) s->arr[i] = NIL;
    return name;
}

static Node *sf_store(Node *args) {
    /* (STORE (ARRNAME IDXFORM) VALFORM) */
    Node *place = car(args);
    Node *valform = car(cdr(args));
    Node *arrname = car(place);
    Node *idxform = car(cdr(place));
    if (!is_sym(arrname)) return NIL;
    Symbol *s = arrname->u.sym;
    Node *idxv = eval(idxform);
    Node *val = eval(valform);
    if (!s->has_array) return NIL;
    int idx = (int)(is_num(idxv) ? idxv->u.num : 0);
    if (idx < s->arr_lo || idx > s->arr_hi) return NIL;
    s->arr[idx - s->arr_lo] = val;
    return val;
}

static Node *sf_dskinq(Node *args) {
    Node *a = car(args);
    if (is_sym(a) && a->u.sym == sym_FAEMAR) {
        load_source(FAEMAR_SOURCE);
    }
    return NIL;
}

static Node *do_eval_form_inner(Node *form) {
    Node *head = form->u.cons.car;
    Node *args = form->u.cons.cdr;

    if (is_sym(head)) {
        Symbol *hs = head->u.sym;
        if (hs == sym_QUOTE) return car(args);
        if (hs == sym_COND) return sf_cond(args);
        if (hs == sym_SETQ) return sf_setq(args);
        if (hs == sym_PROG) return sf_prog(args);
        if (hs == sym_PROG2) return sf_prog2(args);
        if (hs == sym_GO) return sf_go(args);
        if (hs == sym_RETURN) return sf_return(args);
        if (hs == sym_AND) return sf_and(args);
        if (hs == sym_OR) return sf_or(args);
        if (hs == sym_DE) return sf_de(args, 0);
        if (hs == sym_DF) return sf_de(args, 1);
        if (hs == sym_LAMBDA) return form; /* self-quoting when evaluated directly */
        if (hs == sym_FUNCTION) return sf_function(car(args));
        if (hs == sym_DEFPROP) return sf_defprop(args);
        if (hs == sym_ARRAY) return sf_array(args);
        if (hs == sym_STORE) return sf_store(args);
        if (hs == sym_DSKINQ) return sf_dskinq(args);
        if (hs == sym_DECIMAL) return NIL;
        if (hs == sym_SET) {
            Node *sy = eval(car(args));
            Node *val = eval(car(cdr(args)));
            if (is_sym(sy)) { sy->u.sym->value = val; sy->u.sym->bound = 1; }
            return val;
        }

        if (hs->fkind == F_FEXPR) {
            int mark = binding_top;
            push_binding(hs->params->u.cons.car->u.sym, args);
            Node *result = eval_body(hs->body);
            unwind_bindings(mark);
            return result;
        }

        /* normal call: evaluate args then apply */
        Node *argarr_list = eval_list_values(args);
        Node *arr[64]; int n = list_to_array(argarr_list, arr, 64);
        if (hs->fkind == F_EXPR) return call_expr_like(hs->params, hs->body, arr, n);
        if (hs->fkind == F_NATIVE) return hs->native(hs, arr, n);
        if (hs->has_array) {
            int idx = (int)(n>0 && is_num(arr[0]) ? arr[0]->u.num : 0);
            if (idx < hs->arr_lo || idx > hs->arr_hi) return NIL;
            return hs->arr[idx - hs->arr_lo];
        }
        fprintf(stderr, "warning: undefined function %s\n", hs->name);
        return NIL;
    }

    /* head is a cons, e.g. ((LAMBDA (x) ...) arg) */
    Node *fnval = eval(head);
    Node *argarr_list = eval_list_values(args);
    Node *arr[64]; int n = list_to_array(argarr_list, arr, 64);
    return apply_fn(fnval, arr, n);
}

static Node *do_eval_form(Node *form) { return do_eval_form_inner(form); }

/* ------------------------------------------------------------ natives */

static Node *n_car(Symbol *s, Node **a, int n) { (void)s;(void)n; return car(a[0]); }
static Node *n_cdr(Symbol *s, Node **a, int n) { (void)s;(void)n; return cdr(a[0]); }

static Node *n_cxxxr(Symbol *s, Node **a, int n) {
    (void)n;
    const char *nm = s->name;
    int len = (int)strlen(nm);
    /* name like C, A, D, R possibly repeated: CxxR */
    Node *v = a[0];
    for (int i = len - 2; i >= 1; i--) {
        v = (nm[i] == 'A') ? car(v) : cdr(v);
    }
    return v;
}

static Node *n_cons(Symbol *s, Node **a, int n) { (void)s;(void)n; return cons(a[0], a[1]); }
static Node *n_list(Symbol *s, Node **a, int n) {
    (void)s; Node *head=NIL, *tail=NIL;
    for (int i=0;i<n;i++) { Node *cell=cons(a[i],NIL); if(tail) tail->u.cons.cdr=cell; else head=cell; tail=cell; }
    return head;
}
static Node *append2(Node *a, Node *b) {
    if (!is_cons(a)) return b;
    Node *head=NIL, *tail=NIL;
    for (Node *p=a; is_cons(p); p=p->u.cons.cdr) {
        Node *cell = cons(p->u.cons.car, NIL);
        if (tail) tail->u.cons.cdr=cell; else head=cell;
        tail=cell;
    }
    tail->u.cons.cdr = b;
    return head;
}
static Node *n_append(Symbol *s, Node **a, int n) {
    (void)s;
    if (n==0) return NIL;
    Node *result = a[n-1];
    for (int i=n-2;i>=0;i--) result = append2(a[i], result);
    return result;
}
static Node *n_member(Symbol *s, Node **a, int n) { (void)s;(void)n; for (Node *p=a[1]; is_cons(p); p=p->u.cons.cdr) if (equal_p(a[0], p->u.cons.car)) return p; return NIL; }
static Node *n_memq(Symbol *s, Node **a, int n) { (void)s;(void)n; for (Node *p=a[1]; is_cons(p); p=p->u.cons.cdr) if (eq_p(a[0], p->u.cons.car)) return p; return NIL; }
static Node *n_assoc(Symbol *s, Node **a, int n) { (void)s;(void)n; for (Node *p=a[1]; is_cons(p); p=p->u.cons.cdr) { Node *pair=p->u.cons.car; if (is_cons(pair) && equal_p(a[0], pair->u.cons.car)) return pair; } return NIL; }
static Node *n_eq(Symbol *s, Node **a, int n) { (void)s;(void)n; return eq_p(a[0],a[1]) ? node_T : NIL; }
static Node *n_equal(Symbol *s, Node **a, int n) { (void)s;(void)n; return equal_p(a[0],a[1]) ? node_T : NIL; }
static Node *n_null(Symbol *s, Node **a, int n) { (void)s;(void)n; return a[0]==NIL ? node_T : NIL; }
static Node *n_atom(Symbol *s, Node **a, int n) { (void)s;(void)n; return is_cons(a[0]) ? NIL : node_T; }
static Node *n_length(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(list_len(a[0])); }
static Node *n_reverse(Symbol *s, Node **a, int n) { (void)s;(void)n; Node *r=NIL; for (Node *p=a[0]; is_cons(p); p=p->u.cons.cdr) r=cons(p->u.cons.car, r); return r; }
static Node *n_last(Symbol *s, Node **a, int n) { (void)s;(void)n; Node *p=a[0]; if (!is_cons(p)) return NIL; while (is_cons(p->u.cons.cdr)) p=p->u.cons.cdr; return cons(p->u.cons.car, NIL); }

static Node *n_mapc(Symbol *s, Node **a, int n) { (void)s;(void)n; for (Node *p=a[1]; is_cons(p); p=p->u.cons.cdr) { Node *e=p->u.cons.car; apply_fn(a[0], &e, 1); } return a[1]; }
static Node *n_mapcar(Symbol *s, Node **a, int n) { (void)s;(void)n; Node *head=NIL,*tail=NIL; for (Node *p=a[1]; is_cons(p); p=p->u.cons.cdr) { Node *e=p->u.cons.car; Node *v=apply_fn(a[0], &e, 1); Node *cell=cons(v,NIL); if(tail) tail->u.cons.cdr=cell; else head=cell; tail=cell; } return head; }
static Node *n_apply(Symbol *s, Node **a, int n) { (void)s;(void)n; Node *arr[64]; int cnt=list_to_array(a[1], arr, 64); return apply_fn(a[0], arr, cnt); }
static Node *n_eval(Symbol *s, Node **a, int n) { (void)s;(void)n; return eval(a[0]); }

static Node *n_get(Symbol *s, Node **a, int n) { (void)s;(void)n; if (!is_sym(a[0]) || !is_sym(a[1])) return NIL; return plist_get(a[0]->u.sym, a[1]->u.sym); }
static Node *n_putprop(Symbol *s, Node **a, int n) { (void)s;(void)n; if (is_sym(a[0]) && is_sym(a[2])) plist_put(a[0]->u.sym, a[2]->u.sym, a[1]); return a[1]; }
static Node *n_remprop(Symbol *s, Node **a, int n) { (void)s;(void)n; if (is_sym(a[0]) && is_sym(a[1])) plist_rem(a[0]->u.sym, a[1]->u.sym); return NIL; }
static Node *n_getl(Symbol *s, Node **a, int n) { (void)s;(void)a;(void)n; return NIL; }
static Node *n_set(Symbol *s, Node **a, int n) { (void)s;(void)n; if (is_sym(a[0])) { a[0]->u.sym->value=a[1]; a[0]->u.sym->bound=1; } return a[1]; }

static double numval(Node *n) { return is_num(n) ? n->u.num : 0.0; }

static Node *n_greaterp(Symbol *s, Node **a, int n) { (void)s; for (int i=0;i<n-1;i++) if (!(numval(a[i])>numval(a[i+1]))) return NIL; return node_T; }
static Node *n_lessp(Symbol *s, Node **a, int n) { (void)s; for (int i=0;i<n-1;i++) if (!(numval(a[i])<numval(a[i+1]))) return NIL; return node_T; }
static Node *n_plus(Symbol *s, Node **a, int n) { (void)s; double r=0; for (int i=0;i<n;i++) r+=numval(a[i]); return mk_num(r); }
static Node *n_times(Symbol *s, Node **a, int n) { (void)s; double r=1; for (int i=0;i<n;i++) r*=numval(a[i]); return mk_num(r); }
static Node *n_difference(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(numval(a[0])-numval(a[1])); }
static Node *n_sub1(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(numval(a[0])-1); }
static Node *n_add1(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(numval(a[0])+1); }
static Node *n_boole(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(do_boole((long)numval(a[0]), (unsigned long)numval(a[1]), (unsigned long)numval(a[2]))); }
static Node *n_numberp(Symbol *s, Node **a, int n) { (void)s;(void)n; return is_num(a[0]) ? node_T : NIL; }

static Node *n_terpri(Symbol *s, Node **a, int n) { (void)s;(void)a;(void)n; out_char('\n'); return NIL; }
static Node *n_princ(Symbol *s, Node **a, int n) {
    (void)s;(void)n;
    Node *x = a[0];
    char buf[4096];
    print_name(x, buf, sizeof(buf));
    out_str(buf);
    return x;
}

/* the "exit"/"stop"/"quit" verb-table entries all map to (EXIT), a
   zero-argument function call once (EVAL VERB) unwraps it */
static Node *n_exit(Symbol *s, Node **a, int n) { (void)s;(void)a;(void)n; do_quit(); return NIL; }
static int tyo_esc_pending = 0;

static Node *n_tyo(Symbol *s, Node **a, int n) {
    (void)s;(void)n;
    int code = (int)numval(a[0]);
    if (tyo_esc_pending) {
        /* swallow the parameter byte of a two-byte VT52-style escape
           sequence (e.g. ESC H / ESC J for home/clear-to-end) rather than
           printing it as a literal letter -- a clean modern console has
           no use for the original terminal-control codes. */
        tyo_esc_pending = 0;
        return a[0];
    }
    if (code == 27) { tyo_esc_pending = 1; return a[0]; }
    if (code == 13 || code == 10) { out_char('\n'); }
    else if (code >= 32 && code <= 126) { out_char(code); }
    /* other control codes: no-op for a clean modern console */
    return a[0];
}
static Node *n_tyi(Symbol *s, Node **a, int n) { (void)s;(void)a;(void)n; return mk_num(do_tyi()); }
static Node *n_chrct(Symbol *s, Node **a, int n) { (void)s;(void)a;(void)n; return mk_num(g_linelength - g_column); }
static Node *n_flatsize(Symbol *s, Node **a, int n) { (void)s;(void)n; char buf[4096]; print_name(a[0], buf, sizeof(buf)); return mk_num((double)strlen(buf)); }
static Node *n_linelength(Symbol *s, Node **a, int n) { (void)s;(void)n; g_linelength = (int)numval(a[0]); return a[0]; }

static Node *n_readlist(Symbol *s, Node **a, int n) {
    (void)s;(void)n;
    char buf[4096]; buf[0]=0;
    for (Node *p=a[0]; is_cons(p); p=p->u.cons.cdr) {
        char part[512];
        print_name(p->u.cons.car, part, sizeof(part));
        strncat(buf, part, sizeof(buf)-strlen(buf)-1);
    }
    if (strcmp(buf,"NIL")==0) return NIL;
    if (strcmp(buf,"T")==0) return node_T;
    return SYMV(intern(buf));
}
static Node *n_explode(Symbol *s, Node **a, int n) {
    (void)s;(void)n;
    char buf[4096]; print_name(a[0], buf, sizeof(buf));
    Node *head=NIL, *tail=NIL;
    for (char *p=buf; *p; p++) {
        char one[2] = { *p, 0 };
        Node *cell = cons(SYMV(intern(one)), NIL);
        if (tail) tail->u.cons.cdr=cell; else head=cell;
        tail=cell;
    }
    return head;
}
static Node *n_ascii(Symbol *s, Node **a, int n) {
    (void)s;(void)n;
    int code = (int)numval(a[0]);
    char one[2] = { (char)code, 0 };
    return SYMV(intern(one));
}
static Node *n_ran(Symbol *s, Node **a, int n) { (void)s;(void)n; return mk_num(do_ran(numval(a[0]))); }
static Node *n_ncons(Symbol *s, Node **a, int n) { (void)s;(void)n; return cons(a[0], NIL); }

/* -------------------------------------------------------- registration */

static void reg_native(const char *name, NativeFn fn) {
    Symbol *s = intern(name);
    s->fkind = F_NATIVE;
    s->native = fn;
}

static void register_cxxxr(void) {
    const char *letters = "AD";
    char name[8];
    for (int len = 2; len <= 4; len++) {
        int total = 1; for (int i=0;i<len;i++) total *= 2;
        for (int mask = 0; mask < total; mask++) {
            name[0] = 'C';
            for (int i = 0; i < len; i++) {
                int bit = (mask >> (len-1-i)) & 1;
                name[1+i] = letters[bit];
            }
            name[1+len] = 'R';
            name[2+len] = 0;
            reg_native(name, n_cxxxr);
        }
    }
}

static void bootstrap_symbols(void) {
    sym_T = intern("T");
    sym_QUOTE = intern("QUOTE");
    sym_COND = intern("COND");
    sym_SETQ = intern("SETQ");
    sym_PROG = intern("PROG");
    sym_PROG2 = intern("PROG2");
    sym_DE = intern("DE");
    sym_DF = intern("DF");
    sym_LAMBDA = intern("LAMBDA");
    sym_FUNCTION = intern("FUNCTION");
    sym_AND = intern("AND");
    sym_OR = intern("OR");
    sym_GO = intern("GO");
    sym_RETURN = intern("RETURN");
    sym_STORE = intern("STORE");
    sym_ARRAY = intern("ARRAY");
    sym_DSKINQ = intern("DSKINQ");
    sym_DECIMAL = intern("DECIMAL");
    sym_DEFPROP = intern("DEFPROP");
    sym_SET = intern("SET");
    sym_FAEMAR = intern("FAEMAR");
    sym_LAPFNS = intern("LAP.FNS");

    node_T = SYMV(sym_T);
    sym_T->value = node_T; sym_T->bound = 1;

    DOT_MARKER_NODE = new_node(T_SYM); /* unique sentinel, never interned */
    DOT_MARKER_NODE->u.sym = NULL;

    reg_native("EXIT", n_exit);
    reg_native("CAR", n_car);
    reg_native("CDR", n_cdr);
    register_cxxxr();
    reg_native("CONS", n_cons);
    reg_native("LIST", n_list);
    reg_native("APPEND", n_append);
    reg_native("NCONC", n_append);
    reg_native("MEMBER", n_member);
    reg_native("MEMQ", n_memq);
    reg_native("ASSOC", n_assoc);
    reg_native("EQ", n_eq);
    reg_native("EQUAL", n_equal);
    reg_native("NULL", n_null);
    reg_native("NOT", n_null);
    reg_native("ATOM", n_atom);
    reg_native("LENGTH", n_length);
    reg_native("REVERSE", n_reverse);
    reg_native("LAST", n_last);
    reg_native("MAPC", n_mapc);
    reg_native("MAPCAR", n_mapcar);
    reg_native("APPLY", n_apply);
    reg_native("EVAL", n_eval);
    reg_native("SET", n_set);
    reg_native("GET", n_get);
    reg_native("PUTPROP", n_putprop);
    reg_native("REMPROP", n_remprop);
    reg_native("GETL", n_getl);
    reg_native("GREATERP", n_greaterp);
    reg_native("LESSP", n_lessp);
    reg_native("PLUS", n_plus);
    reg_native("TIMES", n_times);
    reg_native("DIFFERENCE", n_difference);
    reg_native("*DIF", n_difference);
    reg_native("*PLUS", n_plus);
    reg_native("SUB1", n_sub1);
    reg_native("ADD1", n_add1);
    reg_native("BOOLE", n_boole);
    reg_native("NUMBERP", n_numberp);
    reg_native("TERPRI", n_terpri);
    reg_native("PRINC", n_princ);
    reg_native("TYO", n_tyo);
    reg_native("TYI", n_tyi);
    reg_native("CHRCT", n_chrct);
    reg_native("FLATSIZE", n_flatsize);
    reg_native("LINELENGTH", n_linelength);
    reg_native("READLIST", n_readlist);
    reg_native("EXPLODE", n_explode);
    reg_native("ASCII", n_ascii);
    reg_native("RAN", n_ran);
    reg_native("NCONS", n_ncons);
}

/* ---------------------------------------------------------------- load */

static void load_source(const char *src) {
    Reader r;
    rd_init(&r, src);
    Node *form;
    int count = 0;
    while (read_top(&r, &form)) {
        if (g_debug) {
            char buf1[200], buf2[200];
            print_name(is_cons(form) ? car(form) : form, buf1, sizeof(buf1));
            print_name(is_cons(form) ? car(cdr(form)) : NIL, buf2, sizeof(buf2));
            fprintf(stderr, "[%d] pos=%zu form: (%s %s ...)\n", count, r.pos, buf1, buf2);
            fflush(stderr);
        }
        eval(form);
        if (g_debug) { fprintf(stderr, "[%d] eval done\n", count); fflush(stderr); }
        count++;
    }
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-debug") == 0) g_debug = 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    out_reset_col();
    bootstrap_symbols();
    seed_rng();

    load_source(GAME_LSP_SOURCE);

    /* start the game */
    Node *start_form = cons(SYMV(intern("START")), NIL);

    recovery_binding_mark = binding_top;
    recovery_prog_mark = prog_top;
    have_recovery = 1;
    if (setjmp(top_level_recovery) != 0) {
        out_str("\nSomething about that got too tangled to follow -- shaking it off...\n\n");
    }
    eval(start_form);

    return 0;
}
