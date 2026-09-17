/*
 * sintran.c - the SINTRAN III monitor calls a FORTRAN background program
 * makes, answered on the host.  Conventions are from ND-860228.2 (SINTRAN III
 * Monitor Calls) and ND-60.050.06 (Users Guide); behaviour was checked against
 * SINTRAN III L running under RetroCore.
 *
 * Calls that take their arguments in registers return to P+1 on error
 * (A = error number) and skip to P+2 on success.  Calls given a parameter
 * list in A return to P+1.
 *
 * Files: (USER)NAME:TYPE is the host file NAME.TYPE in the data directory
 * (the directory the program image came from, or --data).  Files are only
 * ever opened for reading; a program asking to write gets "not write access".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sintran.h"
#include "term.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

Sintran snt;

#define R(x) (c->r[R_##x])
#define M(a) (c->mem[(uint16_t)(a)])

/* SINTRAN background error numbers (ND-860228.2 appendix A) */
#define E_EOF               03
#define E_NO_SUCH_FILE      056
#define E_NO_SUCH_ACCESS    0104
#define E_NOT_WRITE_ACCESS  0106
#define E_TOO_MANY_FILES    0107
#define E_NOT_OPEN          0132
#define E_NOT_MASS_STORAGE  0133
#define E_NO_SUCH_BLOCK     0143

static const struct { int code; const char *text; } errtext[] = {
    { 02, "BAD FILE NUMBER" }, { 03, "END OF FILE" }, { 021, "ILLEGAL CHARACTER IN PARAMETER" },
    { 046, "NO SUCH USER NAME" }, { 056, "NO SUCH FILE NAME" }, { 057, "AMBIGUOUS FILE NAME" },
    { 0104, "NO SUCH ACCESS CODE" }, { 0105, "FILE ALREADY OPEN" }, { 0106, "NOT WRITE ACCESS" },
    { 0107, "ATTEMPT TO OPEN TOO MANY FILES" }, { 0111, "NOT READ ACCESS" },
    { 0123, "NOT OPEN FOR SEQUENTIAL WRITE" }, { 0124, "NOT OPEN FOR SEQUENTIAL READ" },
    { 0125, "NOT OPEN FOR RANDOM WRITE" }, { 0126, "NOT OPEN FOR RANDOM READ" },
    { 0127, "FILE NUMBER OUT OF RANGE" }, { 0132, "NO FILE OPENED WITH THIS NUMBER" },
    { 0133, "NOT MASS STORAGE FILE" }, { 0143, "NO SUCH BLOCK" },
};

void sintran_init(void)
{
    memset(&snt, 0, sizeof snt);
    snt.year_shift = 28;              /* as on the reference machine; weekdays agree */
    snt.escape_enabled = 1;
    snt.break_strategy = 1;
    snt.terminal_type = 0;            /* "not set", as the RetroCore telnet terminals answer */
}

static void logcall(Cpu *c, int n, const char *what)
{
    if (!snt.verbose)
        return;
    fprintf(stderr, "[MON %o %s at %06o] A=%06o D=%06o T=%06o X=%06o B=%06o L=%06o\n",
            n, what, (uint16_t)(R(P) - 1), R(A), R(D), R(T), R(X), R(B), R(L));
}

static void now(struct tm *out, int *ticks)
{
    time_t t;
    struct tm *lt;
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    *ticks = st.wMilliseconds / 20;
#else
    *ticks = 0;
#endif
    t = snt.fixed_clock ? snt.fixed_clock : time(NULL);
    if (snt.fixed_clock)
        *ticks = 0;
    lt = localtime(&t);
    *out = *lt;
    out->tm_year -= snt.year_shift;
}

/* a SINTRAN string: characters up to the apostrophe, starting at word a */
static void getstr(Cpu *c, uint16_t a, char *buf, int n)
{
    int i;
    for (i = 0; i < n - 1; i++) {
        uint16_t w = M(a + i / 2);
        int ch = (i & 1) ? (w & 0x7F) : ((w >> 8) & 0x7F);
        if (ch == '\'' || ch == 0)
            break;
        buf[i] = (char)ch;
    }
    buf[i] = 0;
}

/* does the typed word abbreviate the command name, part by part? */
static int abbrev(const char *typed, const char *name)
{
    while (*typed) {
        const char *te = strchr(typed, '-'), *ne = strchr(name, '-');
        size_t tl = te ? (size_t)(te - typed) : strlen(typed);
        size_t nl = ne ? (size_t)(ne - name) : strlen(name);
        if (tl > nl || strncmp(typed, name, tl) != 0)
            return 0;
        if (!te)
            return 1;
        if (!ne)
            return 0;
        typed = te + 1;
        name = ne + 1;
    }
    return 1;
}

/* MON 70 (COMND).  Only the commands a game gives itself mean anything
   here; anything else is ignored, which is also what SINTRAN does with the
   mangled "  SABLE-ESCAPE-FUNCTION" Skattejakt sends at start-up. */
static void command(const char *cmd)
{
    char word[64];
    int i = 0;
    while (*cmd == '@')
        cmd++;
    if (*cmd == ' ')
        return;
    while (*cmd && *cmd != ' ' && *cmd != ',' && i < 63)
        word[i++] = (char)toupper((unsigned char)*cmd++);
    word[i] = 0;
    if (!word[0] || abbrev(word, "CC"))
        return;
    if (abbrev(word, "TERMINAL-MODE")) {
        /* @TERMINAL-MODE capital-letters?, delay-after-CR?, stop-on-full-page?, ...:
           only the first matters here; an empty or other answer leaves it */
        while (*cmd == ' ' || *cmd == ',')
            cmd++;
        if (toupper((unsigned char)*cmd) == 'Y')
            snt.capitals = 1;
        else if (toupper((unsigned char)*cmd) == 'N')
            snt.capitals = 0;
    } else if (abbrev(word, "DISABLE-ESCAPE-FUNCTION"))
        snt.escape_enabled = 0;
    else if (abbrev(word, "ENABLE-ESCAPE-FUNCTION"))
        snt.escape_enabled = 1;
    else if (snt.verbose)
        fprintf(stderr, "[command ignored: %s]\n", word);
}

static void skip(Cpu *c) { R(P)++; }

static void out_char(int ch)
{
    term_putc(ch);
    if ((ch & 0x7F) == '\n')
        snt.out_tail_len = 0;
    else if (snt.out_tail_len < (int)sizeof snt.out_tail)
        snt.out_tail[snt.out_tail_len++] = (char)(ch & 0x7F);
}

/* SINTRAN's own echo of a character typed at the terminal, as seen on the
   reference machine: Return echoes as CR LF, printable characters as
   themselves, DEL and the other control characters not at all.  (SINTRAN
   echoes as the key is struck; the port echoes when the program reads it,
   which only differs for typing ahead.) */
static void echo(int ch)
{
    ch &= 0x7F;
    if (snt.echo_strategy == 1 && (ch < 32 || ch == 127))
        return;                               /* ECHOM 1: all but the control characters */
    if (ch == '\r') {
        out_char('\r');
        out_char('\n');
    } else if (ch >= 32 && ch < 127) {
        out_char(ch);
    }
}

static int fail(Cpu *c, int code)
{
    R(A) = (uint16_t)code;
    return SIN_RUN;
}

/* ---- files ---- */

static SinFile *file_of(int no)
{
    int i = no - SIN_FIRSTFILE;
    if (i < 0 || i >= SIN_MAXFILES || !snt.files[i].f)
        return NULL;
    return &snt.files[i];
}

static int is_terminal(int no)
{
    return no == 0 || no == 1 || (snt.terminal_no && no == snt.terminal_no);
}

/* (USER)NAME:TYPE or NAME:TYPE or NAME, with a default type, to a host path */
static int host_path(const char *sname, const char *deftype, char *out, size_t n)
{
    char name[80], type[16];
    const char *p = sname, *colon;
    size_t i;
    while (*p == ' ')
        p++;
    if (*p == '"')                        /* "NEW-FILE" quoting */
        p++;
    if (*p == '(') {
        const char *e = strchr(p, ')');
        if (!e)
            return -1;
        p = e + 1;
    }
    colon = strchr(p, ':');
    if (!colon)
        colon = strchr(p, '.');
    for (i = 0; p[i] && p + i != colon && p[i] != '"' && p[i] != ' ' && i < sizeof name - 1; i++)
        name[i] = (char)toupper((unsigned char)p[i]);
    name[i] = 0;
    if (colon) {
        for (i = 0; colon[1 + i] && colon[1 + i] != '"' && colon[1 + i] != ' ' && i < sizeof type - 1; i++)
            type[i] = (char)toupper((unsigned char)colon[1 + i]);
        type[i] = 0;
    } else {
        snprintf(type, sizeof type, "%s", deftype);
    }
    if (!name[0] || strpbrk(name, "\\/:*?<>|"))
        return -1;
    if (snt.data_dir && *snt.data_dir)
        snprintf(out, n, "%s\\%s%s%s", snt.data_dir, name, type[0] ? "." : "", type);
    else
        snprintf(out, n, "%s%s%s", name, type[0] ? "." : "", type);
    return 0;
}

void sintran_close_all(void)
{
    int i;
    for (i = 0; i < SIN_MAXFILES; i++) {
        if (snt.files[i].f)
            fclose(snt.files[i].f);
        snt.files[i].f = NULL;
    }
}

/* read-only access codes: sequential read, random read, random read common */
static int read_only_access(int access)
{
    return access == 1 || access == 3 || access == 7;
}

int sintran_reopen(int index, const char *name, int access, int blocksize, long next_block)
{
    char path[1200];
    FILE *f;
    if (index < 0 || index >= SIN_MAXFILES || host_path(name, "SYMB", path, sizeof path) < 0)
        return -1;
    if (snt.files[index].f)
        fclose(snt.files[index].f);
    memset(&snt.files[index], 0, sizeof snt.files[index]);
    if (!(f = fopen(path, read_only_access(access) || !snt.allow_write ? "rb" : "r+b")))
        return -1;
    snt.files[index].f = f;
    snt.files[index].access = access;
    snt.files[index].blocksize = blocksize;
    snt.files[index].next_block = next_block;
    snt.files[index].writable = !read_only_access(access) && snt.allow_write;
    snprintf(snt.files[index].name, sizeof snt.files[index].name, "%s", name);
    return 0;
}

/* MON 50 OPEN: X -> file name, A -> default type, T = access code.
   As on SINTRAN, opening for writing neither creates nor truncates a file
   (a name in quotes creates it); the program sets the size with SMAX. */
static int mon_open(Cpu *c)
{
    char name[80], type[16], path[1200];
    int access = (int16_t)R(T), i, ro;
    FILE *f;
    getstr(c, R(X), name, sizeof name);
    getstr(c, R(A), type, sizeof type);
    if (snt.verbose)
        fprintf(stderr, "[MON 50 OPEN \"%s\" type \"%s\" access %d]\n", name, type, access);
    if (access < 0 || access > 9)
        return fail(c, E_NO_SUCH_ACCESS);
    ro = read_only_access(access);
    if (!ro && !snt.allow_write)
        return fail(c, E_NOT_WRITE_ACCESS);
    if (host_path(name, type, path, sizeof path) < 0)
        return fail(c, E_NO_SUCH_FILE);
    for (i = 0; i < SIN_MAXFILES && snt.files[i].f; i++)
        ;
    if (i == SIN_MAXFILES)
        return fail(c, E_TOO_MANY_FILES);
    f = fopen(path, ro ? "rb" : "r+b");
    if (!f && !ro && name[strspn(name, " ")] == '"')
        f = fopen(path, "w+b");
    if (!f)
        return fail(c, E_NO_SUCH_FILE);
    memset(&snt.files[i], 0, sizeof snt.files[i]);
    snt.files[i].f = f;
    snt.files[i].access = access;
    snt.files[i].blocksize = 256;
    snt.files[i].writable = !ro;
    if (access == 5)                          /* sequential write append */
        fseek(f, 0, SEEK_END);
    snprintf(snt.files[i].name, sizeof snt.files[i].name, "%s", name);
    R(A) = (uint16_t)(SIN_FIRSTFILE + i);
    skip(c);
    return SIN_RUN;
}

/* stdio wants a seek between reading and writing the same stream */
static void switch_to(SinFile *sf, int op)
{
    if (sf->last_op && sf->last_op != op)
        fseek(sf->f, 0, SEEK_CUR);
    sf->last_op = op;
}

/* MON 120 WFILE: A -> (file number, return flag, buffer, block number, number of words) */
static int mon_wfile(Cpu *c)
{
    uint16_t pl = R(A);
    int no = (int16_t)M(M(pl));
    uint16_t buf = M(pl + 2);
    long block = (int16_t)M(M(pl + 3));
    unsigned words = M(M(pl + 4)), i;
    SinFile *sf = file_of(no);
    if (snt.verbose)
        fprintf(stderr, "[MON 120 WFILE file %o block %ld words %u <- %06o]\n", no, block, words, buf);
    if (!sf) {
        R(A) = E_NOT_OPEN;
        return SIN_RUN;
    }
    if (!sf->writable) {
        R(A) = E_NOT_WRITE_ACCESS;
        return SIN_RUN;
    }
    if (block < 0)
        block = sf->next_block;
    sf->last_op = 0;
    if (fseek(sf->f, block * sf->blocksize * 2L, SEEK_SET) != 0) {
        R(A) = E_NO_SUCH_BLOCK;
        return SIN_RUN;
    }
    for (i = 0; i < words; i++) {
        fputc(M(buf + i) >> 8, sf->f);
        fputc(M(buf + i) & 0xFF, sf->f);
    }
    sf->last_op = 'w';
    sf->next_block = block + (long)((words + sf->blocksize - 1) / sf->blocksize);
    R(A) = 0;
    return SIN_RUN;
}

/* MON 73 SMAX: T = file, AD = the maximum byte pointer, i.e. bytes - 1
   (the Pascal runtime passes 16383 after writing a 16384-byte record) */
static int mon_smax(Cpu *c)
{
    SinFile *sf = file_of((int16_t)R(T));
    long size = (long)(int32_t)(((uint32_t)R(A) << 16) | R(D)) + 1;
    if (snt.verbose)
        fprintf(stderr, "[MON 73 SMAX file %o bytes %ld]\n", (int16_t)R(T), size);
    if (!sf)
        return fail(c, E_NOT_OPEN);
    if (!sf->writable)
        return fail(c, E_NOT_WRITE_ACCESS);
    fflush(sf->f);
#ifdef _WIN32
    if (_chsize(_fileno(sf->f), size) != 0)
#else
    if (ftruncate(fileno(sf->f), size) != 0)
#endif
        return fail(c, E_NOT_WRITE_ACCESS);
    sf->last_op = 0;
    skip(c);
    return SIN_RUN;
}

/* MON 74 SETBT: T = file, AD = byte pointer */
static int mon_setbt(Cpu *c)
{
    SinFile *sf = file_of((int16_t)R(T));
    long pos = ((long)R(A) << 16) | R(D);
    if (snt.verbose)
        fprintf(stderr, "[MON 74 SETBT file %o byte %ld]\n", (int16_t)R(T), pos);
    if (!sf)
        return fail(c, E_NOT_OPEN);
    fseek(sf->f, pos, SEEK_SET);
    sf->last_op = 0;
    skip(c);
    return SIN_RUN;
}

/* MON 75 REABT: T = file -> AD = byte pointer */
static int mon_reabt(Cpu *c)
{
    SinFile *sf = file_of((int16_t)R(T));
    long pos;
    if (!sf)
        return fail(c, E_NOT_OPEN);
    pos = ftell(sf->f);
    R(A) = (uint16_t)(pos >> 16);
    R(D) = (uint16_t)pos;
    skip(c);
    return SIN_RUN;
}

/* MON 43 CLOSE: T = file number, -1 or -2 = all */
static int mon_close(Cpu *c)
{
    int no = (int16_t)R(T);
    SinFile *sf;
    if (no == -1 || no == -2) {
        sintran_close_all();
        skip(c);
        return SIN_RUN;
    }
    if (is_terminal(no)) {
        skip(c);
        return SIN_RUN;
    }
    if (!(sf = file_of(no)))
        return fail(c, E_NOT_OPEN);
    fclose(sf->f);
    sf->f = NULL;
    skip(c);
    return SIN_RUN;
}

/* MON 117 RFILE: A -> (file number, return flag, buffer, block number, number of words) */
static int mon_rfile(Cpu *c)
{
    uint16_t pl = R(A);
    int no = (int16_t)M(M(pl));
    uint16_t buf = M(pl + 2);
    long block = (int16_t)M(M(pl + 3));
    unsigned words = M(M(pl + 4)), i;
    SinFile *sf = file_of(no);
    if (snt.verbose)
        fprintf(stderr, "[MON 117 RFILE file %o block %ld words %u -> %06o]\n", no, block, words, buf);
    if (!sf) {
        R(A) = E_NOT_OPEN;
        return SIN_RUN;
    }
    if (block < 0)
        block = sf->next_block;
    if (fseek(sf->f, block * sf->blocksize * 2L, SEEK_SET) != 0) {
        R(A) = E_NO_SUCH_BLOCK;
        return SIN_RUN;
    }
    sf->last_op = 0;
    for (i = 0; i < words; i++) {
        int hi = fgetc(sf->f), lo = hi == EOF ? EOF : fgetc(sf->f);
        if (hi == EOF) {
            if (i == 0 && !sf->writable) {    /* the block starts at or past the end */
                R(A) = E_EOF;
                return SIN_RUN;
            }
            /* a file open for writing reads as 0 past its end: ND BASIC reads a
               block of a virtual array (DIM #) before it first writes it */
            for (; i < words; i++)            /* the last, short block: the rest reads as 0 */
                M(buf + i) = 0;
            break;
        }
        M(buf + i) = (uint16_t)((hi << 8) | (lo == EOF ? 0 : lo));
    }
    sf->next_block = block + (long)((words + sf->blocksize - 1) / sf->blocksize);
    R(A) = 0;
    return SIN_RUN;
}

/* MON 41 ROBJE: the file's 32-word object entry.  The terminal's is the one
   SYSTEM/TERMINAL has on the reference pack; a disk file gets an entry laid
   out like the pack's (in use, indexed, self-referencing versions, public
   access 2377B, pages and bytes-1 from the host file). */
static int mon_robje(Cpu *c)
{
    static const uint16_t terminal[32] = {
        0100000, 0052105, 0051115, 0044516, 0040514, 0023400, 0, 0, 0, 0023400, 0,
        0000005, 0000005, 0016347, 0000001, 0000001, 0, 0000005, 0, 0, 0115440, 0115260,
        0, 0, 0, 0, 0, 0, 0177777, 0177777, 0, 0 };
    int no = (int16_t)R(T), i;
    uint16_t b = R(A);
    SinFile *sf;
    char nm[80], *p;
    long here, size, pages;
    if (is_terminal(no)) {
        for (i = 0; i < 32; i++)
            M(b + i) = terminal[i];
        skip(c);
        return SIN_RUN;
    }
    if (!(sf = file_of(no)))
        return fail(c, E_NOT_OPEN);
    here = ftell(sf->f);
    fseek(sf->f, 0, SEEK_END);
    size = ftell(sf->f);
    fseek(sf->f, here, SEEK_SET);
    for (i = 0; i < 32; i++)
        M(b + i) = 0;
    /* name (bytes 2-17) and type (18-21), each ended by an apostrophe */
    snprintf(nm, sizeof nm, "%s", sf->name);
    if ((p = strchr(nm, ')')) != NULL)
        memmove(nm, p + 1, strlen(p + 1) + 1);
    for (i = 0; nm[i] && nm[i] != ':' && nm[i] != '\'' && i < 15; i++)
        M(b + 1 + i / 2) |= (uint16_t)(toupper((unsigned char)nm[i]) << ((i & 1) ? 0 : 8));
    M(b + 1 + i / 2) |= (uint16_t)('\'' << ((i & 1) ? 0 : 8));
    M(b) = 0100000;
    M(b + 11) = M(b + 12) = 0007410;
    M(b + 13) = 0002377;
    M(b + 14) = 0000010;
    M(b + 17) = 0007410;
    pages = (size + 2047) / 2048;
    M(b + 26) = (uint16_t)(pages >> 16);
    M(b + 27) = (uint16_t)pages;
    M(b + 28) = (uint16_t)((size - 1) >> 16);
    M(b + 29) = (uint16_t)(size - 1);
    skip(c);
    return SIN_RUN;
}

/* MON 262 CPUST: A = number, X -> 12 words.  Answered as an authorised
   SINTRAN III VSX/500 L on an ND-100 with 48-bit floating point would; the
   Pascal runtime asks it at start-up and gives up ("unauthorized use") if
   the call fails. */
static int mon_cpust(Cpu *c)
{
    static const uint16_t info[12] = {
        0,                  /* system number */
        (2 << 8) | 2,       /* ND-100, 48-bit floating point; instruction set ND-100/CX 4 PITs */
        0,                  /* microprogram version */
        100,                /* system type */
        (5 << 8) | 'L',     /* SINTRAN III VSX-500, version L */
        0, 0, 0, 0, 0, 0, 0 };
    int i;
    if (snt.verbose)
        fprintf(stderr, "[MON 262 CPUST number %06o buffer %06o]\n", R(A), R(X));
    for (i = 0; i < 12; i++)
        M(R(X) + i) = info[i];
    skip(c);
    return SIN_RUN;
}

static void hold(Cpu *c)
{
    uint16_t pl = R(A);
    long count = (int16_t)M(M(pl)), unit = (int16_t)M(M(pl + 1));
    long ms = 0;
    switch (unit) {
    case 1: ms = count * 20; break;
    case 2: ms = count * 1000; break;
    case 3: ms = count * 60000; break;
    case 4: ms = count * 3600000; break;
    default: break;
    }
    if (snt.verbose)
        fprintf(stderr, "[MON 104 HOLD %ld units of %ld = %ld ms]\n", count, unit, ms);
    term_flush();
    if (ms > 0 && !snt.no_hold) {
#ifdef _WIN32
        Sleep((DWORD)ms);
#endif
    }
}

static void error_message(int code)
{
    char msg[80];
    size_t i;
    const char *t = NULL;
    for (i = 0; i < sizeof errtext / sizeof errtext[0]; i++)
        if (errtext[i].code == code)
            t = errtext[i].text;
    if (t)
        snprintf(msg, sizeof msg, "\r\n%s\r\n", t);
    else
        snprintf(msg, sizeof msg, "\r\nFILE SYSTEM ERROR %oB\r\n", code);
    term_puts(msg);
}

int sintran_mon(Cpu *c, int n)
{
    switch (n) {
    case 0:                                     /* LEAVE */
        logcall(c, n, "LEAVE");
        snt.exit_pc = (uint16_t)(R(P) - 1);
        return SIN_EXIT;

    case 1: {                                   /* INBT: T = device, A = byte */
        int ch, no = (int16_t)R(T);
        if (!is_terminal(no)) {
            SinFile *sf = file_of(no);
            if (!sf)
                return fail(c, E_NOT_OPEN);
            switch_to(sf, 'r');
            if ((ch = fgetc(sf->f)) == EOF)
                return fail(c, E_EOF);
            R(A) = (uint16_t)ch;
            skip(c);
            return SIN_RUN;
        }
        if (no == 0 && snt.command_rest_len > 0) {
            /* the end of the command line that started the program: the
               Pascal runtime reads it for parameters before anything else */
            R(A) = (uint8_t)snt.command_rest[0];
            memmove(snt.command_rest, snt.command_rest + 1, (size_t)--snt.command_rest_len);
            skip(c);
            return SIN_RUN;
        }
        if (snt.typeahead_len > 0) {
            ch = (uint8_t)snt.typeahead[0];
            memmove(snt.typeahead, snt.typeahead + 1, (size_t)--snt.typeahead_len);
        } else {
            term_set_escape(snt.escape_enabled);
            ch = term_getc();
        }
        if (ch < 0)
            return SIN_EOF;
        if ((ch == 033 && snt.escape_enabled) || ch == 3) {
            snt.exit_pc = (uint16_t)(R(P) - 1);
            return SIN_BREAK;
        }
        if (snt.capitals && ch >= 0141 && ch <= 0175)      /* a-z { | }: not ~, as SINTRAN */
            ch -= 040;
        if (snt.input_hook && snt.input_hook(c, ch))
            return SIN_RUN;
        if (snt.sintran_echo && snt.echo_strategy >= 0)
            echo(ch);
        if (snt.input_parity && no != 0) {
            int b, ones = 0;
            for (b = ch & 0177; b; b >>= 1)
                ones += b & 1;
            ch = (ch & 0177) | (ones & 1 ? 0200 : 0);
        }
        R(A) = (uint16_t)ch;
        skip(c);
        return SIN_RUN;
    }

    case 2: {                                   /* OUTBT: T = device, A = byte */
        int no = (int16_t)R(T);
        if (!is_terminal(no)) {
            SinFile *sf = file_of(no);
            if (!sf)
                return fail(c, E_NOT_OPEN);
            if (!sf->writable)
                return fail(c, E_NOT_WRITE_ACCESS);
            switch_to(sf, 'w');
            fputc(R(A) & 0xFF, sf->f);
            skip(c);
            return SIN_RUN;
        }
        if (R(A) & 0x7F)                      /* a NUL never reaches the reference terminal */
            out_char(R(A));
        skip(c);
        return SIN_RUN;
    }

    case 3:                                     /* ECHOM: A = strategy, X = table */
        logcall(c, n, "ECHOM");
        snt.echo_strategy = (int16_t)R(A);
        return SIN_RUN;

    case 4:                                     /* BRKM: A = strategy, D = count, X = table */
        logcall(c, n, "BRKM");
        snt.break_strategy = (int16_t)R(A);
        return SIN_RUN;

    case 016:                                   /* MGTTY: T = device -> A = terminal type */
        logcall(c, n, "MGTTY");
        if (!is_terminal((int16_t)R(T)))
            return fail(c, E_NOT_MASS_STORAGE);
        R(A) = snt.terminal_type;
        skip(c);
        return SIN_RUN;

    case 033:                                   /* ALTON, ALTOFF: one bank, nothing to do */
    case 034:
        logcall(c, n, n == 033 ? "ALTON" : "ALTOFF");
        return SIN_RUN;

    case 041:                                   /* ROBJE: T = file, A -> 32 words */
        logcall(c, n, "ROBJE");
        return mon_robje(c);

    case 043:                                   /* CLOSE */
        logcall(c, n, "CLOSE");
        return mon_close(c);

    case 050:                                   /* OPEN */
        return mon_open(c);

    case 062: {                                 /* RMAX: T = file -> AD = bytes */
        SinFile *sf = file_of((int16_t)R(T));
        long here, size;
        logcall(c, n, "RMAX");
        if (!sf)
            return fail(c, E_NOT_OPEN);
        here = ftell(sf->f);
        fseek(sf->f, 0, SEEK_END);
        size = ftell(sf->f);
        fseek(sf->f, here, SEEK_SET);
        R(A) = (uint16_t)(size >> 16);
        R(D) = (uint16_t)size;
        skip(c);
        return SIN_RUN;
    }

    case 064:                                   /* ERMSG: A = error number */
        logcall(c, n, "ERMSG");
        error_message(R(A));
        return SIN_RUN;

    case 065:                                   /* QERMS: message, then the program ends */
        logcall(c, n, "QERMS");
        error_message(R(A));
        snt.exit_pc = (uint16_t)(R(P) - 1);
        return SIN_FATAL;

    case 070: {                                 /* COMND: A = string */
        char buf[256];
        getstr(c, R(A), buf, sizeof buf);
        if (snt.verbose)
            fprintf(stderr, "[MON 70 COMND \"%s\"]\n", buf);
        command(buf);
        return SIN_RUN;
    }

    case 071:                                   /* DESCF */
        logcall(c, n, "DESCF");
        snt.escape_enabled = 0;
        return SIN_RUN;

    case 072:                                   /* EESCF */
        logcall(c, n, "EESCF");
        snt.escape_enabled = 1;
        return SIN_RUN;

    case 073:                                   /* SMAX */
        return mon_smax(c);

    case 074:                                   /* SETBT */
        return mon_setbt(c);

    case 075:                                   /* REABT */
        logcall(c, n, "REABT");
        return mon_reabt(c);

    case 0120:                                  /* WFILE */
        return mon_wfile(c);

    case 076: {                                 /* SETBS: T = file, A = block size in words */
        SinFile *sf = file_of((int16_t)R(T));
        logcall(c, n, "SETBS");
        if (!sf)
            return fail(c, E_NOT_OPEN);
        sf->blocksize = R(A) ? R(A) : 256;
        skip(c);
        return SIN_RUN;
    }

    case 0104:                                  /* HOLD: A -> (count, unit) */
        hold(c);
        return SIN_RUN;

    case 0113: {                                /* CLOCK: A -> (address of 7 words) */
        uint16_t b = M(R(A));
        struct tm tm;
        int ticks;
        now(&tm, &ticks);
        M(b) = (uint16_t)ticks;
        M(b + 1) = (uint16_t)tm.tm_sec;
        M(b + 2) = (uint16_t)tm.tm_min;
        M(b + 3) = (uint16_t)tm.tm_hour;
        M(b + 4) = (uint16_t)tm.tm_mday;
        M(b + 5) = (uint16_t)(tm.tm_mon + 1);
        M(b + 6) = (uint16_t)(tm.tm_year + 1900);
        logcall(c, n, "CLOCK");
        return SIN_RUN;
    }

    case 0117:                                  /* RFILE */
        return mon_rfile(c);

    case 0262:                                  /* CPUST */
        return mon_cpust(c);

    case 011: {                                 /* TIME: AD = basic time units (1/50 s) since start */
        unsigned long t = 0;
        if (!snt.fixed_clock) {
#ifdef _WIN32
            t = (unsigned long)(GetTickCount64() / 20);
#else
            t = (unsigned long)time(NULL) * 50UL;
#endif
        }
        R(A) = (uint16_t)(t >> 16);
        R(D) = (uint16_t)t;
        return SIN_RUN;
    }

    case 013:                                   /* CIBUF: T = device; clear its input buffer */
        logcall(c, n, "CIBUF");
        if (is_terminal((int16_t)R(T))) {
            snt.typeahead_len = 0;
            term_clear_input();
        }
        R(A) = 0;
        return SIN_RUN;

    case 066:                                   /* ISIZE: T = device -> A = bytes waiting */
        logcall(c, n, "ISIZE");
        if (!is_terminal((int16_t)R(T)))
            return fail(c, E_NOT_OPEN);
        R(A) = (uint16_t)(term_pending() + snt.typeahead_len);
        skip(c);
        return SIN_RUN;

    case 067:                                   /* OSIZE: T = device -> A = free bytes in the output buffer */
        logcall(c, n, "OSIZE");
        if (!is_terminal((int16_t)R(T)))
            return fail(c, E_NOT_OPEN);
        term_flush();
        R(A) = 64;                              /* an empty buffer of a club terminal: see NOTES */
        skip(c);
        return SIN_RUN;

    case 0143:                                  /* RSIO: A = mode, T/D = command in/out, X = owner */
        logcall(c, n, "RSIO");
        R(A) = 0;                               /* interactive */
        R(T) = (uint16_t)(snt.terminal_no ? snt.terminal_no : 1);   /* the user's terminal */
        R(D) = R(T);
        R(X) = 0;
        return SIN_RUN;

    case 0214: {                                /* GUSNA: A -> 16-byte string, X = user index */
        static const char user[] = "DNF'";      /* the club's user; the port has no other */
        uint16_t a = R(A);
        int i;
        logcall(c, n, "GUSNA");
        for (i = 0; i < 16; i++) {
            int ch = user[i < (int)sizeof user - 1 ? i : (int)sizeof user - 2];
            uint16_t w = M(a + i / 2);
            if (i >= (int)sizeof user - 1) break;
            M(a + i / 2) = (uint16_t)(i % 2 ? (w & 0xFF00) | ch : (w & 0x00FF) | (ch << 8));
        }
        skip(c);
        return SIN_RUN;
    }

    default:
        fprintf(stderr, "\n[unimplemented monitor call MON %o at %06o: A=%06o D=%06o T=%06o X=%06o]\n",
                n, (uint16_t)(R(P) - 1), R(A), R(D), R(T), R(X));
        snt.exit_pc = (uint16_t)(R(P) - 1);
        return SIN_FATAL;
    }
}
