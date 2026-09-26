/* ======================================================================
 *  svm.c - Eric Roberts' SVM (the stack machine of his "WebFor" FORTRAN
 *  for the web, SVM_VERSION 5) and the WebFor run-time library, written
 *  in C from the JavaScript that runs the browser edition:
 *
 *      js/edu/stanford/cs/svm.js      the machine, Core/Console/Global
 *      js/edu/stanford/cs/webfor.js   the WFLib FORTRAN run-time
 *      js/edu/stanford/cs/exp.js      Value (numbers print as JS does)
 *      js/edu/stanford/cs/utf8.js     strings packed into the code array
 *
 *  It runs the compiled Adventure images Small.js ("Starter Adventure",
 *  240 points, embedded) and Big.js ("Wellesley Adventure", --image).
 *  Everything the images use is implemented with the JavaScript's
 *  semantics - including its errors, which stop the machine as they did
 *  in the browser.  Instructions and library methods the images never
 *  use stop the machine with "not implemented".
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

/* ---------------------------------------------------------------------- */
/*  Code image                                                            */
/* ---------------------------------------------------------------------- */

extern const int svm_image[];           /* image.c, generated from Small.js */
extern const int svm_image_len;

static const int32_t *code;
static int ncode;

/* ---------------------------------------------------------------------- */
/*  Values and the heap                                                   */
/* ---------------------------------------------------------------------- */

typedef enum { V_UNDEF, V_NULL, V_BOOL, V_INT, V_DBL, V_STR, V_ARR, V_CLASS } VType;

typedef struct Obj { struct Obj *next; unsigned char kind, mark, perm; } Obj;
typedef struct Str { Obj h; uint32_t hash; int hashed; size_t len; char s[1]; } Str;
struct Arr;
typedef struct Value {
    VType t;
    union { double n; int b; Str *s; struct Arr *a; } u;
} Value;
typedef struct Arr { Obj h; size_t len, cap; Value *v; } Arr;

static Obj *heap;
static size_t heap_bytes, gc_threshold = 16u << 20;

static const Value UNDEF = { V_UNDEF, { 0 } };

static void fatal(const char *fmt, ...);

static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) { fputs("svm: out of memory\n", stderr); exit(3); }
    return p;
}

static Str *str_alloc(size_t len, int perm)
{
    Str *s = xmalloc(sizeof(Str) + len);
    s->h.kind = V_STR; s->h.mark = 0; s->h.perm = (unsigned char)perm;
    s->hashed = 0; s->len = len; s->s[len] = 0;
    if (!perm) { s->h.next = heap; heap = &s->h; heap_bytes += sizeof(Str) + len; }
    else s->h.next = NULL;
    return s;
}

static Str *str_new(const char *p, size_t len)
{
    Str *s = str_alloc(len, 0);
    memcpy(s->s, p, len);
    return s;
}

static Arr *arr_new(void)
{
    Arr *a = xmalloc(sizeof(Arr));
    a->h.kind = V_ARR; a->h.mark = 0; a->h.perm = 0;
    a->len = a->cap = 0; a->v = NULL;
    a->h.next = heap; heap = &a->h; heap_bytes += sizeof(Arr);
    return a;
}

static void arr_push(Arr *a, Value v)
{
    if (a->len == a->cap) {
        size_t nc = a->cap ? a->cap * 2 : 8;
        a->v = realloc(a->v, nc * sizeof(Value));
        if (!a->v) { fputs("svm: out of memory\n", stderr); exit(3); }
        heap_bytes += (nc - a->cap) * sizeof(Value);
        a->cap = nc;
    }
    a->v[a->len++] = v;
}

static Value mk_int(double n)  { Value v; v.t = V_INT;  v.u.n = n; return v; }
static Value mk_dbl(double n)  { Value v; v.t = V_DBL;  v.u.n = n; return v; }
static Value mk_bool(int b)    { Value v; v.t = V_BOOL; v.u.b = b != 0; return v; }
static Value mk_str(Str *s)    { Value v; v.t = V_STR;  v.u.s = s; return v; }
static Value mk_arr(Arr *a)    { Value v; v.t = V_ARR;  v.u.a = a; return v; }

static uint32_t str_hash(Str *s)
{
    if (!s->hashed) {
        uint32_t h = 2166136261u;
        size_t i;
        for (i = 0; i < s->len; i++) { h ^= (unsigned char)s->s[i]; h *= 16777619u; }
        s->hash = h; s->hashed = 1;
    }
    return s->hash;
}

static int str_eq(Str *a, Str *b)
{
    return a == b || (a->len == b->len && memcmp(a->s, b->s, a->len) == 0);
}

/* ---------------------------------------------------------------------- */
/*  Hash tables (globals, interned names)                                 */
/* ---------------------------------------------------------------------- */

typedef struct { Str *key; Value v; } Slot;
typedef struct { Slot *s; size_t n, used; } Table;

static Slot *tab_find(Table *t, Str *key, int create)
{
    size_t i, mask;
    if (t->n == 0) {
        if (!create) return NULL;
        t->n = 1024; t->used = 0;
        t->s = calloc(t->n, sizeof(Slot));
    }
    if (create && (t->used + 1) * 2 > t->n) {
        Table nt; size_t j;
        nt.n = t->n * 2; nt.used = 0; nt.s = calloc(nt.n, sizeof(Slot));
        for (j = 0; j < t->n; j++)
            if (t->s[j].key) { Slot *d = tab_find(&nt, t->s[j].key, 1); d->v = t->s[j].v; }
        free(t->s); *t = nt;
    }
    mask = t->n - 1;
    i = str_hash(key) & mask;
    while (t->s[i].key) {
        if (str_eq(t->s[i].key, key)) return &t->s[i];
        i = (i + 1) & mask;
    }
    if (!create) return NULL;
    t->s[i].key = key; t->s[i].v = UNDEF; t->used++;
    return &t->s[i];
}

static Table globals, interned;

/* strings in the code array are decoded once and interned (permanent) */
static Str **codestr;

static Str *intern(const char *p, size_t len)
{
    Str tmp_hdr; Str *probe; Slot *sl;
    probe = xmalloc(sizeof(Str) + len);
    memcpy(probe->s, p, len); probe->s[len] = 0; probe->len = len; probe->hashed = 0;
    (void)tmp_hdr;
    sl = tab_find(&interned, probe, 0);
    if (sl) { free(probe); return sl->key; }
    probe->h.kind = V_STR; probe->h.perm = 1; probe->h.mark = 0; probe->h.next = NULL;
    sl = tab_find(&interned, probe, 1);
    return sl->key;
}

static Str *code_string(int addr)
{
    if (addr < 0 || addr >= ncode) fatal("Illegal string address %d", addr);
    if (!codestr[addr]) {
        /* UTF8.decode: bytes big-endian in successive words up to a 0 byte */
        size_t cap = 64, n = 0;
        char *buf = xmalloc(cap);
        int a = addr, shift = 24;
        for (;;) {
            int b = (a < ncode) ? (int)(((uint32_t)code[a] >> shift) & 0xFF) : 0;
            if (b == 0) break;
            if (n + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
            buf[n++] = (char)b;
            shift -= 8;
            if (shift < 0) { shift = 24; a++; }
        }
        codestr[addr] = intern(buf, n);
        free(buf);
    }
    return codestr[addr];
}

/* ---------------------------------------------------------------------- */
/*  Frames and stacks                                                     */
/* ---------------------------------------------------------------------- */

typedef struct Var { Str *name; Value v; } Var;
typedef struct Frame {
    Var *vars; int nvars, capvars;
    int argc, retaddr;
    long stackbase; int hasbase;
} Frame;

static Value *stack; static long sp, stackcap;
static Frame **frames; static int nframes, capframes;
static Frame *cf;
static long pc;

static Frame *frame_new(void)
{
    Frame *f = xmalloc(sizeof(Frame));
    f->vars = NULL; f->nvars = f->capvars = 0;
    f->argc = 0; f->retaddr = -1; f->stackbase = 0; f->hasbase = 0;
    return f;
}

static void frame_free(Frame *f) { free(f->vars); free(f); }

static Var *frame_lookup(Frame *f, Str *name)
{
    int i;
    for (i = f->nvars - 1; i >= 0; i--)
        if (f->vars[i].name == name || str_eq(f->vars[i].name, name)) return &f->vars[i];
    return NULL;
}

static void frame_declare(Frame *f, Str *name, Value v)   /* declareVar + setVar */
{
    Var *x = frame_lookup(f, name);
    if (!x) {
        if (f->nvars == f->capvars) {
            f->capvars = f->capvars ? f->capvars * 2 : 16;
            f->vars = realloc(f->vars, f->capvars * sizeof(Var));
        }
        x = &f->vars[f->nvars++];
        x->name = name;
    }
    x->v = v;
}

static void push(Value v)
{
    if (sp == stackcap) {
        stackcap = stackcap ? stackcap * 2 : 1024;
        stack = realloc(stack, stackcap * sizeof(Value));
        if (!stack) { fputs("svm: out of memory\n", stderr); exit(3); }
    }
    stack[sp++] = v;
}

static Value pop(void)
{
    if (sp <= 0) fatal("Stack underflow");
    return stack[--sp];
}

static Value peek(long k)
{
    if (sp - k - 1 < 0) fatal("Stack underflow");
    return stack[sp - k - 1];
}

static void push_frame(void)
{
    if (nframes == capframes) {
        capframes = capframes ? capframes * 2 : 64;
        frames = realloc(frames, capframes * sizeof(Frame *));
    }
    frames[nframes++] = cf;
    cf = frame_new();
}

static void pop_frame(void)
{
    frame_free(cf);
    cf = nframes > 0 ? frames[--nframes] : NULL;
}

/* ---------------------------------------------------------------------- */
/*  Garbage collection (at instruction boundaries only)                   */
/* ---------------------------------------------------------------------- */

static void mark_value(Value v);

static void mark_arr(Arr *a)
{
    size_t i;
    if (a->h.mark) return;
    a->h.mark = 1;
    for (i = 0; i < a->len; i++) mark_value(a->v[i]);
}

static void mark_value(Value v)
{
    if (v.t == V_STR) { if (!v.u.s->h.perm) v.u.s->h.mark = 1; }
    else if (v.t == V_ARR) mark_arr(v.u.a);
}

static void mark_frame(Frame *f)
{
    int i;
    for (i = 0; i < f->nvars; i++) mark_value(f->vars[i].v);
}

static void gc(void)
{
    long i; size_t j; Obj **pp;
    for (i = 0; i < sp; i++) mark_value(stack[i]);
    for (i = 0; i < nframes; i++) mark_frame(frames[i]);
    if (cf) mark_frame(cf);
    for (j = 0; j < globals.n; j++)
        if (globals.s[j].key) { mark_value(globals.s[j].v); if (!globals.s[j].key->h.perm) globals.s[j].key->h.mark = 1; }
    heap_bytes = 0;
    pp = &heap;
    while (*pp) {
        Obj *o = *pp;
        if (o->mark) {
            o->mark = 0;
            heap_bytes += (o->kind == V_STR) ? sizeof(Str) + ((Str *)o)->len
                                             : sizeof(Arr) + ((Arr *)o)->cap * sizeof(Value);
            pp = &o->next;
        } else {
            *pp = o->next;
            if (o->kind == V_ARR) free(((Arr *)o)->v);
            free(o);
        }
    }
    gc_threshold = heap_bytes * 2 + (16u << 20);
}

/* ---------------------------------------------------------------------- */
/*  Conversions (exp.js Value, jslib)                                     */
/* ---------------------------------------------------------------------- */

static double truncate(double d) { return d < 0 ? ceil(d) : floor(d); }

static int is_numeric(Value v) { return v.t == V_INT || v.t == V_DBL; }

static int is_integral(Value v)
{
    if (v.t == V_INT) return 1;
    if (v.t == V_DBL) return truncate(v.u.n) == v.u.n;
    return 0;
}

/* Number.prototype.toString(): shortest digits that read back exactly */
static void js_number(double d, char *out)
{
    char buf[40], digits[24], *p = out;
    int k, n, prec, e;
    if (isnan(d)) { strcpy(out, "NaN"); return; }
    if (isinf(d)) { strcpy(out, d < 0 ? "-Infinity" : "Infinity"); return; }
    if (d == 0) { strcpy(out, "0"); return; }
    if (d < 0) { *p++ = '-'; d = -d; }
    for (prec = 1; prec <= 17; prec++) {
        snprintf(buf, sizeof buf, "%.*e", prec - 1, d);
        if (strtod(buf, NULL) == d) break;
    }
    /* buf = D[.DDD]e[+-]XX */
    k = 0;
    {
        char *q = buf;
        for (; *q && *q != 'e'; q++) if (*q >= '0' && *q <= '9') digits[k++] = *q;
        digits[k] = 0;
        e = atoi(q + 1);
    }
    while (k > 1 && digits[k - 1] == '0') digits[--k] = 0;
    n = e + 1;
    if (k <= n && n <= 21) {
        memcpy(p, digits, k); p += k;
        while (n-- > k) *p++ = '0';
        *p = 0;
    } else if (0 < n && n <= 21) {
        memcpy(p, digits, n); p += n; *p++ = '.';
        strcpy(p, digits + n);
    } else if (-6 < n && n <= 0) {
        *p++ = '0'; *p++ = '.';
        while (n++ < 0) *p++ = '0';
        strcpy(p, digits);
    } else {
        *p++ = digits[0];
        if (k > 1) { *p++ = '.'; memcpy(p, digits + 1, k - 1); p += k - 1; }
        sprintf(p, "e%c%d", e < 0 ? '-' : '+', e < 0 ? -e : e);
    }
}

/* Value.toString() (and SVMArray.toString() for arrays) */
static Str *to_string(Value v)
{
    char buf[64];
    switch (v.t) {
    case V_UNDEF: return intern("undefined", 9);
    case V_NULL:  return intern("null", 4);
    case V_BOOL:  return v.u.b ? intern("true", 4) : intern("false", 5);
    case V_INT: case V_DBL:
        js_number(is_integral(v) ? truncate(v.u.n) : v.u.n, buf);
        return str_new(buf, strlen(buf));
    case V_STR:   return v.u.s;
    case V_ARR: {
        size_t i, len = 0; Str *r; char *q;
        Str **parts = xmalloc((v.u.a->len + 1) * sizeof(Str *));
        for (i = 0; i < v.u.a->len; i++) { parts[i] = to_string(v.u.a->v[i]); len += parts[i]->len + (i > 0); }
        r = str_alloc(len, 0); q = r->s;
        for (i = 0; i < v.u.a->len; i++) {
            if (i > 0) *q++ = ',';
            memcpy(q, parts[i]->s, parts[i]->len); q += parts[i]->len;
        }
        free(parts);
        return r;
    }
    case V_CLASS: return v.u.s;
    }
    return intern("?", 1);
}

static double int_value(Value v)                 /* getIntegerValue() */
{
    if (v.t == V_INT) return v.u.n;
    if (v.t == V_DBL && truncate(v.u.n) == v.u.n) return truncate(v.u.n);
    fatal("Illegal integer");
    return 0;
}

static double dbl_value(Value v)                 /* getDoubleValue() */
{
    if (v.t == V_INT || v.t == V_DBL) return v.u.n;
    fatal("Illegal double");
    return 0;
}

static int bool_value(Value v)                   /* getBooleanValue() */
{
    if (v.t != V_BOOL) fatal("Illegal boolean");
    return v.u.b;
}

static int32_t to_int32(double d)                /* JS ToInt32 */
{
    double m;
    if (isnan(d) || isinf(d)) return 0;
    m = fmod(truncate(d), 4294967296.0);
    if (m < 0) m += 4294967296.0;
    return (int32_t)(uint32_t)m;
}

/* Integer.parseInt (java2js): '+' or '-' only first, digits otherwise; "" and
   "-" pass the check and give NaN */
static int parse_int(Str *s, double *out, char *err, size_t errlen)
{
    size_t i, start = 0; double r = 0; int neg = 0, any = 0;
    for (i = 0; i < s->len; i++) {
        char ch = s->s[i];
        if (ch == '+' && i == 0) start = 1;
        else if (!(ch >= '0' && ch <= '9') && (ch != '-' || i > 0)) {
            snprintf(err, errlen, "\"%s\" is not a legal integer", s->s);
            return 0;
        }
    }
    i = start;
    if (i < s->len && s->s[i] == '-') { neg = 1; i++; }
    for (; i < s->len; i++) { r = r * 10 + (s->s[i] - '0'); any = 1; }
    *out = any ? (neg ? -r : r) : NAN;
    return 1;
}

/* ---------------------------------------------------------------------- */
/*  Console                                                               */
/* ---------------------------------------------------------------------- */

static int echo_input;

static void out_str(Str *s) { fwrite(s->s, 1, s->len, stdout); }

static Str *read_line(void)
{
    size_t cap = 256, n = 0;
    char *buf = xmalloc(cap);
    int c;
    Str *s;
    fflush(stdout);
    for (;;) {
        c = getchar();
        if (c == EOF) {
            if (n == 0) { free(buf); fputs("\n", stdout); fflush(stdout); exit(0); }
            break;
        }
        if (c == '\n') break;
        if (n + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[n++] = (char)c;
    }
    if (n > 0 && buf[n - 1] == '\r') n--;
    s = str_new(buf, n);
    free(buf);
    if (echo_input) { out_str(s); fputs("\n", stdout); }
    return s;
}

/* ---------------------------------------------------------------------- */
/*  Errors: RuntimeException -> signalError -> the machine stops           */
/* ---------------------------------------------------------------------- */

static void fatal(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fputs("\n", stdout);
    fflush(stdout);
    exit(1);
}

/* ---------------------------------------------------------------------- */
/*  Library methods                                                       */
/* ---------------------------------------------------------------------- */

static int nargs(void) { return cf->argc; }

static int check_type(Value v, char t)
{
    switch (t) {
    case 'B': return v.t == V_BOOL;
    case 'D': return is_numeric(v);
    case 'I': return is_integral(v);
    case 'O': return v.t == V_ARR || v.t == V_CLASS;
    case 'S': return v.t == V_STR;
    case '*': return 1;
    }
    fatal("Illegal type code: %c", t);
    return 0;
}

static void check_sig(const char *name, const char *sig)
{
    int n = nargs(), len = (int)strlen(sig), i;
    if (n == -1) return;
    if (len != n) fatal("Wrong number of arguments to %s", name);
    for (i = 0; i < len; i++)
        if (!check_type(peek(len - i - 1), sig[i])) fatal("Type mismatch in call to %s", name);
}

static uint64_t rng_state;

static double js_random(void)              /* Math.random(): splitmix64 */
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= z >> 31;
    return (double)(z >> 11) * (1.0 / 9007199254740992.0);
}

/* FIX 1 (on unless --no-fixes): WebFor gives every FORTRAN array an element
   0 holding UNDEFINED, and the parser reads VERBS(0) (and similar) whenever a
   command starts with a preposition or "ALL": arithmetic on UNDEFINED is a
   run-time error, and in the browser the game silently stopped responding
   after commands such as "at", "with rod", "from", "off" or "all".  The
   FORTRAN program read whatever preceded the array and carried on ("I'm
   afraid I don't understand.").  With the fix element 0 reads as 0 (a zero
   sub-array for the outer dimensions of a multi-dimensional array). */
static int fixes = 1;

static Arr *dim_array(double *dims, int ndims, int k)
{
    Arr *a = arr_new();
    double i, n = dims[k];
    if (!fixes) arr_push(a, UNDEF);
    else if (k == ndims - 1) arr_push(a, mk_int(0));
    else arr_push(a, mk_arr(dim_array(dims, ndims, k + 1)));
    for (i = 0; i < n; i++) {
        if (k == ndims - 1) arr_push(a, mk_int(0));
        else arr_push(a, mk_arr(dim_array(dims, ndims, k + 1)));
    }
    return a;
}

static void call_method(Str *name)
{
    const char *m = name->s;
    Value v, ix, obj;
    char err[256];

    if (!strcmp(m, "Global.get")) {
        Slot *sl;
        check_sig(m, "S");
        v = pop();
        sl = tab_find(&globals, v.u.s, 0);
        push(sl ? sl->v : UNDEF);
    } else if (!strcmp(m, "Global.set")) {
        Slot *sl; Value key;
        check_sig(m, "S*");
        v = pop(); key = pop();
        sl = tab_find(&globals, key.u.s->h.perm ? key.u.s : intern(key.u.s->s, key.u.s->len), 1);
        sl->v = v;
    } else if (!strcmp(m, "Global.isDefined")) {
        check_sig(m, "S");
        v = pop();
        push(mk_bool(tab_find(&globals, v.u.s, 0) != NULL));
    } else if (!strcmp(m, "Core.FALSE")) {
        check_sig(m, ""); push(mk_bool(0));
    } else if (!strcmp(m, "Core.TRUE")) {
        check_sig(m, ""); push(mk_bool(1));
    } else if (!strcmp(m, "Core.NULL")) {
        Value nv; check_sig(m, ""); nv.t = V_NULL; push(nv);
    } else if (!strcmp(m, "Core.UNDEFINED")) {
        check_sig(m, ""); push(UNDEF);
    } else if (!strcmp(m, "Core.select")) {
        check_sig(m, "**");
        ix = pop(); obj = pop();
        if (obj.t == V_STR) {
            Str *s = obj.u.s; double index;
            if (ix.t == V_STR) {
                if (ix.u.s->len == 6 && !memcmp(ix.u.s->s, "length", 6)) { push(mk_int((double)s->len)); return; }
                fatal("String.%s is not implemented", ix.u.s->s);
            }
            index = int_value(ix);
            if (index < 0) index += (double)s->len;
            if (index >= 0 && index < (double)s->len) push(mk_str(str_new(s->s + (size_t)index, 1)));
            else fatal("String index out of bounds");
        } else if (obj.t == V_ARR) {
            Arr *a = obj.u.a;
            if (ix.t == V_STR) {
                if (ix.u.s->len == 6 && !memcmp(ix.u.s->s, "length", 6)) { push(mk_int((double)a->len)); return; }
                fatal("Array.%s is not implemented", ix.u.s->s);
            } else {
                double index = int_value(ix);
                if (index >= 0 && index < (double)a->len) push(a->v[(size_t)index]);
                else push(UNDEF);
            }
        } else if (obj.t == V_INT || obj.t == V_DBL) {
            fatal("Number methods are not implemented");
        } else if (obj.t == V_UNDEF || obj.t == V_NULL || obj.t == V_BOOL) {
            fatal("Illegal selection");
        } else {
            fatal("Illegal selection");
        }
    } else if (!strcmp(m, "Core.assign")) {
        check_sig(m, "***");
        v = pop(); ix = pop(); obj = pop();
        if (obj.t == V_ARR) {
            Arr *a = obj.u.a; double index = int_value(ix);
            if (index < 0) fatal("Array index out of bounds");
            while ((double)a->len <= index) arr_push(a, UNDEF);
            a->v[(size_t)index] = v;
        } else fatal("Illegal selection");
    } else if (!strcmp(m, "Core.list")) {
        int n = nargs(), i; Arr *a = arr_new();
        if (n < 0) n = 0;
        for (i = 0; i < n; i++) arr_push(a, UNDEF);
        for (i = n - 1; i >= 0; i--) a->v[i] = pop();
        push(mk_arr(a));
    } else if (!strcmp(m, "Core.array")) {
        double n, i; Arr *a;
        check_sig(m, "I");
        n = int_value(pop()); a = arr_new();
        for (i = 0; i < n; i++) arr_push(a, UNDEF);
        push(mk_arr(a));
    } else if (!strcmp(m, "Core.length")) {
        check_sig(m, "*");
        v = pop();
        if (v.t != V_ARR) fatal("Illegal argument to length");
        push(mk_int((double)v.u.a->len));
    } else if (!strcmp(m, "Core.display")) {
        check_sig(m, "*");
        v = pop();
        if (v.t != V_UNDEF) { out_str(to_string(v)); fputs("\n", stdout); }
    } else if (!strcmp(m, "Console.print")) {
        check_sig(m, "*");
        out_str(to_string(pop()));
    } else if (!strcmp(m, "Console.println")) {
        if (nargs() == 0) { check_sig(m, ""); fputs("\n", stdout); }
        else { check_sig(m, "*"); out_str(to_string(pop())); fputs("\n", stdout); }
    } else if (!strcmp(m, "Console.readline") || !strcmp(m, "WFLib.read")) {
        Str *prompt = NULL;
        if (m[0] == 'W') check_sig(m, "");
        else if (nargs() == 0) check_sig(m, "");
        else { check_sig(m, "S"); prompt = pop().u.s; }
        if (prompt) out_str(prompt);
        push(mk_str(read_line()));
    } else if (!strcmp(m, "Console.readint")) {
        Str *prompt = NULL, *line; double n;
        if (nargs() == 0) check_sig(m, "");
        else { check_sig(m, "S"); prompt = pop().u.s; }
        if (prompt) out_str(prompt);
        for (;;) {
            line = read_line();
            if (parse_int(line, &n, err, sizeof err)) break;
            /* SVMConsoleListener: show the error and ask again, no prompt */
            fputs(err, stdout); fputs("\n", stdout);
        }
        push(mk_int(n));
    } else if (!strcmp(m, "WFLib.abs")) {
        check_sig(m, "I"); push(mk_int(fabs(int_value(pop()))));
    } else if (!strcmp(m, "WFLib.mod")) {
        double i1, i2;
        check_sig(m, "II");
        i2 = int_value(pop()); i1 = int_value(pop());
        push(mk_int(fmod(i1, i2)));
    } else if (!strcmp(m, "WFLib.int")) {
        check_sig(m, "*");
        v = pop();
        if (v.t == V_INT) push(mk_int(int_value(v)));
        else if (v.t == V_DBL) push(mk_int(truncate(v.u.n)));
        else if (v.t == V_STR) {
            double n;
            if (!parse_int(v.u.s, &n, err, sizeof err)) fatal("%s", err);
            push(mk_int(n));
        } else fatal("Illegal argument to INT()");
    } else if (!strcmp(m, "WFLib.min") || !strcmp(m, "WFLib.max")) {
        double i1, i2;
        check_sig(m, "II");
        i1 = int_value(pop()); i2 = int_value(pop());
        if (isnan(i1) || isnan(i2)) push(mk_int(NAN));      /* Math.min/max */
        else if (m[7] == 'i') push(mk_int(i1 < i2 ? i1 : i2));
        else push(mk_int(i1 > i2 ? i1 : i2));
    } else if (!strcmp(m, "WFLib.rand")) {
        if (nargs() == 0) check_sig(m, "");
        else { check_sig(m, "I"); pop(); }
        push(mk_dbl(js_random()));
    } else if (!strcmp(m, "WFLib.lower") || !strcmp(m, "WFLib.upper")) {
        Str *s, *r; size_t i; int up = m[6] == 'u';
        check_sig(m, "S");
        s = pop().u.s; r = str_new(s->s, s->len);
        for (i = 0; i < r->len; i++) {
            char c = r->s[i];
            if (up && c >= 'a' && c <= 'z') r->s[i] = (char)(c - 32);
            if (!up && c >= 'A' && c <= 'Z') r->s[i] = (char)(c + 32);
        }
        push(mk_str(r));
    } else if (!strcmp(m, "WFLib.length")) {
        check_sig(m, "S"); push(mk_int((double)pop().u.s->len));
    } else if (!strcmp(m, "WFLib.substr")) {
        double p1, p2, len; Str *s;
        check_sig(m, "SII");
        p2 = int_value(pop()); p1 = int_value(pop()); s = pop().u.s;
        len = (double)s->len;             /* String.prototype.substring */
        if (isnan(p1)) p1 = 0;
        if (isnan(p2)) p2 = 0;
        p1 = p1 < 0 ? 0 : (p1 > len ? len : p1);
        p2 = p2 < 0 ? 0 : (p2 > len ? len : p2);
        if (p1 > p2) { double t = p1; p1 = p2; p2 = t; }
        push(mk_str(str_new(s->s + (size_t)p1, (size_t)(p2 - p1))));
    } else if (!strcmp(m, "WFLib.find")) {
        double start; Str *needle, *hay; long r = -1; size_t st, i;
        check_sig(m, "SSI");
        start = int_value(pop()); needle = pop().u.s; hay = pop().u.s;
        st = start < 0 ? 0 : (start > (double)hay->len ? hay->len : (size_t)start);
        for (i = st; i + needle->len <= hay->len; i++)
            if (!memcmp(hay->s + i, needle->s, needle->len)) { r = (long)i; break; }
        push(mk_int((double)r));
    } else if (!strcmp(m, "WFLib.parseint")) {
        double n;
        check_sig(m, "S");
        if (!parse_int(pop().u.s, &n, err, sizeof err)) fatal("%s", err);
        push(mk_int(n));
    } else if (!strcmp(m, "WFLib.ord")) {
        Str *s;
        check_sig(m, "S");
        s = pop().u.s;
        push(mk_int(s->len ? (double)(unsigned char)s->s[0] : NAN));
    } else if (!strcmp(m, "WFLib.chr")) {
        double n; char c;
        check_sig(m, "I");
        n = int_value(pop());
        if (n < 0) push(mk_str(str_new("", 0)));
        else { c = (char)(int)n; push(mk_str(str_new(&c, 1))); }
    } else if (!strcmp(m, "WFLib.write")) {
        check_sig(m, "*");
        v = pop();
        if (v.t != V_UNDEF) { out_str(to_string(v)); fputs("\n", stdout); }
    } else if (!strcmp(m, "WFLib.dim")) {
        int n = nargs(), i; double *dims;
        if (n < 0) n = 0;
        dims = xmalloc((n + 1) * sizeof(double));
        for (i = 0; i < n; i++) dims[i] = int_value(pop());
        push(n ? mk_arr(dim_array(dims, n, 0)) : mk_arr(arr_new()));
        free(dims);
    } else if (!strcmp(m, "WFLib.exit")) {
        /* FORTRAN STOP.  java2js ignores System.exit(), so in the browser the
           program ran on (into END after the last "[Hit return to exit]", or
           back out of the fatal-error routine BUG); here it stops. */
        fflush(stdout);
        exit(0);
    } else if (!strcmp(m, "WFLib.execjs")) {
        check_sig(m, "S");
        pop();                          /* setupDone() / gameOver(): page hooks */
    } else if (!strcmp(m, "WFLib.rdfile")) {
        /* the browser asked for a file with a file picker; here it is the
           named file in the current directory */
        Str *fn; FILE *f; char *buf; long len;
        check_sig(m, "S");
        fn = pop().u.s;
        f = fopen(fn->s, "rb");
        if (!f) { push(mk_str(str_new("", 0))); return; }
        fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
        buf = xmalloc(len + 1);
        len = (long)fread(buf, 1, (size_t)len, f);
        fclose(f);
        {   /* the browser read it as text: CR LF -> LF */
            long i, o = 0;
            for (i = 0; i < len; i++) if (!(buf[i] == '\r' && i + 1 < len && buf[i + 1] == '\n')) buf[o++] = buf[i];
            len = o;
        }
        push(mk_str(str_new(buf, (size_t)len)));
        free(buf);
    } else if (!strcmp(m, "WFLib.wrfile")) {
        /* the browser offered the text for download under this name */
        Str *text, *fn; FILE *f;
        check_sig(m, "SS");
        text = pop().u.s; fn = pop().u.s;
        f = fopen(fn->s, "wb");
        if (!f) { push(mk_str(intern("Cancel", 6))); return; }
        fwrite(text->s, 1, text->len, f);
        fclose(f);
        push(mk_str(intern("OK", 2)));
    } else {
        fatal("%s is not defined", m);
    }
}

/* ---------------------------------------------------------------------- */
/*  The machine                                                           */
/* ---------------------------------------------------------------------- */

enum {
    OP_END = 0x00, OP_VERSION = 0x01, OP_PSTACK = 0x02, OP_STMT = 0x03, OP_HALT = 0x04, OP_NOP = 0x05,
    OP_TRACE = 0x06, OP_PUSHINT = 0x10, OP_PUSHNUM = 0x11, OP_PUSHCH = 0x12, OP_PUSHSTR = 0x13,
    OP_PUSHFN = 0x14, OP_POP = 0x15, OP_DUP = 0x16, OP_EXCH = 0x17, OP_ROLL = 0x18, OP_COPY = 0x19,
    OP_ADD = 0x20, OP_SUB = 0x21, OP_MUL = 0x22, OP_DIV = 0x23, OP_IDIV = 0x24, OP_REM = 0x25,
    OP_NEG = 0x26, OP_EQ = 0x30, OP_NE = 0x31, OP_LT = 0x32, OP_LE = 0x33, OP_GT = 0x34, OP_GE = 0x35,
    OP_JUMP = 0x40, OP_JUMPT = 0x41, OP_JUMPF = 0x42, OP_DISPATCH = 0x43, OP_TRY = 0x44,
    OP_ENDTRY = 0x45, OP_THROW = 0x46, OP_NOT = 0x50, OP_AND = 0x51, OP_OR = 0x52, OP_XOR = 0x53,
    OP_LSH = 0x54, OP_ASH = 0x55, OP_CALL = 0x60, OP_CALLM = 0x61, OP_CALLFN = 0x62,
    OP_RETURN = 0x63, OP_LOCALS = 0x64, OP_PUSHLOC = 0x65, OP_POPLOC = 0x66, OP_ARG = 0x67,
    OP_VAR = 0x68, OP_PARAMS = 0x69, OP_NARGS = 0x6A, OP_VARGS = 0x6B, OP_PUSHVAR = 0x6C,
    OP_POPVAR = 0x6D, OP_PUSHFRM = 0x6E, OP_POPFRM = 0x6F
};

static int trace, warn, strcmp_warned;

static int nargs_count(void)                    /* getNARGSCount() */
{
    int32_t ins;
    if (pc < 0 || pc >= ncode) return -1;
    ins = code[pc];
    if ((((uint32_t)ins >> 24) & 0xFF) != OP_NARGS) return -1;
    return ins & 0xFFFFFF;
}

/* RelationalOp.execute; returns the boolean */
static int relational(int op, Value lhs, Value rhs)
{
    int c = 0;
    if (lhs.t == V_STR && rhs.t == V_STR) {
        /* String.localeCompare: for EQ/NE equality is all that matters; the
           ordering operators would need the browser's collation */
        c = (lhs.u.s->len == rhs.u.s->len && !memcmp(lhs.u.s->s, rhs.u.s->s, lhs.u.s->len)) ? 0 : 1;
        if (op != OP_EQ && op != OP_NE) {
            size_t n = lhs.u.s->len < rhs.u.s->len ? lhs.u.s->len : rhs.u.s->len;
            c = memcmp(lhs.u.s->s, rhs.u.s->s, n);
            if (c == 0) c = (lhs.u.s->len > rhs.u.s->len) - (lhs.u.s->len < rhs.u.s->len);
            if (!strcmp_warned && (trace || warn)) { strcmp_warned = 1; fprintf(stderr, "svm: ordered string comparison at %ld\n", pc - 1); }
        }
    } else if (is_numeric(lhs) && is_numeric(rhs)) {
        double x = lhs.u.n, y = rhs.u.n;
        switch (op) {
        case OP_EQ: return x == y;
        case OP_NE: return x != y;
        case OP_LT: return x < y;
        case OP_LE: return x <= y;
        case OP_GT: return x > y;
        case OP_GE: return x >= y;
        }
    } else {
        /* applyObject: JavaScript === on the underlying values */
        int same;
        if (op != OP_EQ && op != OP_NE) fatal("Illegal object comparison");
        if (lhs.t == V_BOOL && rhs.t == V_BOOL) same = lhs.u.b == rhs.u.b;
        else if (lhs.t == V_ARR && rhs.t == V_ARR) same = lhs.u.a == rhs.u.a;
        else if ((lhs.t == V_STR || lhs.t == V_UNDEF || lhs.t == V_NULL) &&
                 (rhs.t == V_STR || rhs.t == V_UNDEF || rhs.t == V_NULL)) {
            /* UNDEFINED and NULL hold the strings "undefined" and "null" */
            Str *a = to_string(lhs), *b = to_string(rhs);
            same = a->len == b->len && !memcmp(a->s, b->s, a->len);
        } else same = 0;
        return op == OP_EQ ? same : !same;
    }
    switch (op) {
    case OP_EQ: return c == 0;
    case OP_NE: return c != 0;
    case OP_LT: return c < 0;
    case OP_LE: return c <= 0;
    case OP_GT: return c > 0;
    case OP_GE: return c >= 0;
    }
    return 0;
}

static const char *opname(int op)
{
    switch (op) {
    case OP_END: return "END"; case OP_HALT: return "HALT"; case OP_NOP: return "NOP";
    case OP_PUSHINT: return "PUSHINT"; case OP_PUSHNUM: return "PUSHNUM"; case OP_PUSHSTR: return "PUSHSTR";
    case OP_POP: return "POP"; case OP_DUP: return "DUP"; case OP_ADD: return "ADD"; case OP_SUB: return "SUB";
    case OP_MUL: return "MUL"; case OP_DIV: return "DIV"; case OP_IDIV: return "IDIV"; case OP_REM: return "REM";
    case OP_NEG: return "NEG"; case OP_EQ: return "EQ"; case OP_NE: return "NE"; case OP_LT: return "LT";
    case OP_LE: return "LE"; case OP_GT: return "GT"; case OP_GE: return "GE"; case OP_JUMP: return "JUMP";
    case OP_JUMPT: return "JUMPT"; case OP_JUMPF: return "JUMPF"; case OP_NOT: return "NOT"; case OP_OR: return "OR";
    case OP_AND: return "AND"; case OP_CALL: return "CALL"; case OP_CALLM: return "CALLM"; case OP_RETURN: return "RETURN";
    case OP_ARG: return "ARG"; case OP_VAR: return "VAR"; case OP_PARAMS: return "PARAMS"; case OP_NARGS: return "NARGS";
    case OP_PUSHVAR: return "PUSHVAR"; case OP_POPVAR: return "POPVAR";
    }
    return "?";
}

static void run(void)
{
    for (;;) {
        int32_t ins; int op, addr;
        Value lhs, rhs, v;

        if (heap_bytes > gc_threshold) gc();
        if (pc < 0 || pc >= ncode) return;            /* FINISHED */
        ins = code[pc];
        op = (int)(((uint32_t)ins >> 24) & 0xFF);
        addr = ins & 0xFFFFFF;
        if (trace) fprintf(stderr, "(%ld) %s %d\n", pc, opname(op), addr);
        pc++;
        switch (op) {
        case OP_END: case OP_HALT:
            pc = -1;
            break;
        case OP_VERSION:
            if (addr != 5) fatal("Incompatible SVM version");
            break;
        case OP_NOP: case OP_NARGS:
            break;
        case OP_STMT:
            if (cf && cf->hasbase) while (sp > cf->stackbase) sp--;
            break;
        case OP_TRACE:
            trace = addr != 0;
            break;
        case OP_PUSHINT:
            push(mk_int(addr));
            break;
        case OP_PUSHNUM: {
            Str *s = code_string(addr);
            if (!strchr(s->s, '.') && !strchr(s->s, 'e') && !strchr(s->s, 'E')) {
                double n; char err[256];
                if (!parse_int(s, &n, err, sizeof err)) fatal("%s", err);
                push(mk_int(n));
            } else push(mk_dbl(strtod(s->s, NULL)));
            break;
        }
        case OP_PUSHSTR:
            push(mk_str(code_string(addr)));
            break;
        case OP_POP:
            pop();
            break;
        case OP_DUP:
            push(peek(0));
            break;
        case OP_EXCH:
            lhs = pop(); rhs = pop(); push(lhs); push(rhs);
            break;
        case OP_ADD:
            rhs = pop(); lhs = pop();
            if (lhs.t == V_STR || rhs.t == V_STR) {
                Str *a = to_string(lhs), *b = to_string(rhs), *r = str_alloc(a->len + b->len, 0);
                memcpy(r->s, a->s, a->len); memcpy(r->s + a->len, b->s, b->len);
                push(mk_str(r));
            } else if (!is_numeric(lhs) || !is_numeric(rhs)) {
                fatal("Illegal to apply ADD to %s and %s", to_string(lhs)->s, to_string(rhs)->s);
            } else if (lhs.t == V_INT && rhs.t == V_INT) push(mk_int(lhs.u.n + rhs.u.n));
            else push(mk_dbl(lhs.u.n + rhs.u.n));
            break;
        case OP_SUB: case OP_MUL:
            rhs = pop(); lhs = pop();
            if (!is_numeric(lhs) || !is_numeric(rhs))
                fatal("Illegal to apply %s to %s and %s", opname(op), to_string(lhs)->s, to_string(rhs)->s);
            {
                double r = op == OP_SUB ? lhs.u.n - rhs.u.n : lhs.u.n * rhs.u.n;
                push(lhs.t == V_INT && rhs.t == V_INT ? mk_int(r) : mk_dbl(r));
            }
            break;
        case OP_DIV:
            rhs = pop(); lhs = pop();
            if (!is_numeric(lhs) || !is_numeric(rhs))
                fatal("Illegal to apply DIV to %s and %s", to_string(lhs)->s, to_string(rhs)->s);
            if (lhs.t == V_INT && rhs.t == V_INT) {
                double num = lhs.u.n, den = rhs.u.n;
                if (den != 0 && truncate(num / den) * den == num) push(mk_int(truncate(num / den)));
                else push(mk_dbl(num / den));
            } else push(mk_dbl(lhs.u.n / rhs.u.n));
            break;
        case OP_IDIV:
            rhs = pop(); lhs = pop();
            if (!is_numeric(lhs) || !is_numeric(rhs))
                fatal("Illegal to apply IDIV to %s and %s", to_string(lhs)->s, to_string(rhs)->s);
            push(mk_int(truncate(dbl_value(lhs) / dbl_value(rhs))));
            break;
        case OP_REM:
            rhs = pop(); lhs = pop();
            if (!is_numeric(lhs) || !is_numeric(rhs))
                fatal("Illegal to apply REM to %s and %s", to_string(lhs)->s, to_string(rhs)->s);
            if (lhs.t == V_INT && rhs.t == V_INT) {
                if (rhs.u.n == 0) push(mk_dbl(fmod(lhs.u.n, 0.0)));
                else push(mk_int(fmod(lhs.u.n, rhs.u.n)));
            } else push(mk_dbl(fmod(lhs.u.n, rhs.u.n)));
            break;
        case OP_NEG:
            v = pop();
            if (!is_numeric(v)) fatal("Illegal to apply NEG to %s", to_string(v)->s);
            push(v.t == V_INT ? mk_int(-v.u.n) : mk_dbl(-v.u.n));
            break;
        case OP_EQ: case OP_NE: case OP_LT: case OP_LE: case OP_GT: case OP_GE:
            rhs = pop(); lhs = pop();
            push(mk_bool(relational(op, lhs, rhs)));
            break;
        case OP_JUMP:
            pc = addr;
            break;
        case OP_JUMPT:
            if (bool_value(pop())) pc = addr;
            break;
        case OP_JUMPF:
            if (!bool_value(pop())) pc = addr;
            break;
        case OP_NOT:
            v = pop();
            if (v.t == V_BOOL) push(mk_bool(!v.u.b));
            else push(mk_int((double)~to_int32(int_value(v))));
            break;
        case OP_AND: case OP_OR: case OP_XOR:
            rhs = pop(); lhs = pop();
            if (lhs.t == V_BOOL && rhs.t == V_BOOL) {
                int32_t x = lhs.u.b ? -1 : 0, y = rhs.u.b ? -1 : 0;
                int32_t r = op == OP_AND ? (x & y) : op == OP_OR ? (x | y) : (x ^ y);
                push(mk_bool(r != 0));
            } else {
                int32_t x = to_int32(int_value(lhs)), y = to_int32(int_value(rhs));
                int32_t r = op == OP_AND ? (x & y) : op == OP_OR ? (x | y) : (x ^ y);
                push(mk_int((double)r));
            }
            break;
        case OP_CALL:
            push_frame();
            cf->retaddr = (int)pc;
            cf->argc = nargs_count();
            pc = addr;
            break;
        case OP_CALLM:
            cf->argc = nargs_count();
            call_method(code_string(addr));
            break;
        case OP_RETURN:
            pc = cf->retaddr;
            pop_frame();
            if (!cf) pc = -1;
            break;
        case OP_PARAMS: {
            int nparams = addr, na = cf->argc, i;
            if (na != -1) {
                for (i = nparams; i < na; i++) pop();
                for (i = na; i < nparams; i++) push(UNDEF);
                cf->stackbase = sp - na; cf->hasbase = 1;
            }
            break;
        }
        case OP_ARG:
            v = pop();
            frame_declare(cf, code_string(addr), v);
            break;
        case OP_VAR:
            frame_declare(cf, code_string(addr), UNDEF);
            break;
        case OP_PUSHVAR: {
            Str *name = code_string(addr);
            Var *x = frame_lookup(cf, name);
            if (x) push(x->v);
            else {
                Slot *sl = tab_find(&globals, name, 0);
                if (!sl) fatal("%s has not been declared", name->s);
                push(sl->v);
            }
            break;
        }
        case OP_POPVAR: {
            Str *name = code_string(addr);
            Var *x = frame_lookup(cf, name);
            v = pop();
            if (x) x->v = v;
            else {
                Slot *sl = tab_find(&globals, name, 0);
                if (!sl) fatal("%s has not been declared", name->s);
                sl->v = v;
            }
            break;
        }
        default:
            fatal("SVM instruction %02X at %ld is not implemented", op, pc - 1);
        }
    }
}

/* ---------------------------------------------------------------------- */
/*  Loading an image (the .js file the browser uses) and main             */
/* ---------------------------------------------------------------------- */

static int32_t *load_js(const char *path, int *n)
{
    FILE *f = fopen(path, "rb");
    long len; char *buf, *p; int32_t *w; int cap = 1 << 17, k = 0;
    if (!f) { perror(path); exit(2); }
    fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
    buf = xmalloc(len + 1);
    len = (long)fread(buf, 1, (size_t)len, f); buf[len] = 0;
    fclose(f);
    p = strchr(buf, '[');
    if (!p) { fprintf(stderr, "%s: not an SVM code array\n", path); exit(2); }
    w = xmalloc(cap * sizeof(int32_t));
    p++;
    while (*p && *p != ']') {
        char *e; long long x;
        while (*p && *p != '-' && (*p < '0' || *p > '9') && *p != ']') p++;
        if (!*p || *p == ']') break;
        x = strtoll(p, &e, 10);
        if (k == cap) { cap *= 2; w = realloc(w, cap * sizeof(int32_t)); }
        w[k++] = (int32_t)(uint32_t)(x & 0xFFFFFFFFLL);
        p = e;
    }
    free(buf);
    *n = k;
    return w;
}

#ifndef PROGNAME
#define PROGNAME "starter"
#endif

static void usage(FILE *f)
{
    fputs("usage: " PROGNAME " [--seed N] [--image FILE.js] [--no-fixes] [--echo | --no-echo]\n\n"
          "  --seed N          fixed random numbers (the browser used Math.random)\n"
          "  --image FILE.js   run another compiled image, e.g. the browser's Big.js\n"
          "  --no-fixes        keep the browser edition's bugs (commands such as \"at\"\n"
          "                    or \"all\" then stop the game with a run-time error)\n"
          "  --echo/--no-echo  repeat input lines in the output (default: when input\n"
          "                    is not a console)\n"
          "  -h, --help        this help\n", f);
}

int main(int argc, char **argv)
{
    int i, echo = -1, have_seed = 0;
    const char *image = NULL;
    uint64_t seed = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc) { seed = strtoull(argv[++i], NULL, 10); have_seed = 1; }
        else if (!strncmp(argv[i], "--seed=", 7)) { seed = strtoull(argv[i] + 7, NULL, 10); have_seed = 1; }
        else if (!strcmp(argv[i], "--image") && i + 1 < argc) image = argv[++i];
        else if (!strncmp(argv[i], "--image=", 8)) image = argv[i] + 8;
        else if (!strcmp(argv[i], "--echo")) echo = 1;
        else if (!strcmp(argv[i], "--no-echo")) echo = 0;
        else if (!strcmp(argv[i], "-T")) trace = 1;
        else if (!strcmp(argv[i], "-W")) warn = 1;
        else if (!strcmp(argv[i], "--no-fixes")) fixes = 0;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(stdout); return 0; }
        else { fprintf(stderr, PROGNAME ": unknown option '%s' (try --help)\n", argv[i]); return 2; }
    }
    echo_input = echo >= 0 ? echo : !isatty(fileno(stdin));
    rng_state = have_seed ? seed : ((uint64_t)time(NULL) << 20) ^ (uint64_t)clock();

    if (image) { int n; code = load_js(image, &n); ncode = n; }
    else { code = (const int32_t *)svm_image; ncode = svm_image_len; }
    codestr = calloc((size_t)ncode, sizeof(Str *));

    /* SVM(): Core, Console, Global and WFLib are globals naming classes */
    {
        const char *cls[] = { "Core", "Console", "Global", "WFLib" };
        for (i = 0; i < 4; i++) {
            Str *n = intern(cls[i], strlen(cls[i]));
            Slot *sl = tab_find(&globals, n, 1);
            sl->v.t = V_CLASS; sl->v.u.s = n;
        }
    }
    cf = frame_new();
    pc = 0;
    run();
    fflush(stdout);
    return 0;
}
