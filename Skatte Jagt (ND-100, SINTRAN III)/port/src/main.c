/*
 * main.c - run a SINTRAN III :PROG image as a native program.
 *
 * One source for the ND-100 game ports; GAME picks the game at build time:
 *   GAME=0  skattejakt.exe  Skattejakt (1995 dump of SKATTEJAKT:PROG)
 *   GAME=1  svha.exe        SVHA Adventure (SVHA-ADVENTURE:PROG and its files)
 *
 * Saving works two ways:
 *
 * - Skattejakt suspends Woods-style ("SPAR"): SINTRAN users @DUMPed the
 *   memory after the game stopped and started the dump later; the port writes
 *   that dump for you and starts it again when you name it on the command line.
 *
 * - SVHA Adventure knows SAVE, SUSPEND and PAUSE but only answers "I don't
 *   know how (yet)."  The port keeps a snapshot of the machine each time the
 *   game starts reading a command; SAVE writes that snapshot to a file and
 *   RESTORE (or naming the file on the command line) puts it back.  The game
 *   never sees those words.
 *
 * SVHA Adventure also has two bugs that stop it being finished; the port fixes
 * them in memory (svha_fixes), unless --no-fixes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nd100.h"
#include "sintran.h"
#include "term.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

#define VERSION "1.0"

#ifndef GAME
#define GAME 0
#endif

enum { SAVE_NONE, SAVE_DUMP, SAVE_SNAPSHOT };

/* A fix to the game's own code: the n words at addr change from orig to fixed.  The
   port applies a fix only where it finds the original words, so another program is
   left alone, and takes it out again for --no-fixes. */
typedef struct {
    uint16_t addr;
    int n;
    uint16_t orig[12], fixed[12];
} Fix;

static const Fix svha_fixes[] = {
    /* OPEN COFFIN.  The OPEN routine calls the LIFT routine with the coffin (104)
       for an argument, where LIFT's ring test is for the lid (97); afterwards it
       goes on into the per-location OPEN code, which at the crypt is the vault's
       ("You don't have the necessary piece of metal").  Pass the lid, and return
       as the gate's branch does (073107 is a RETURN).  LIFT LID always worked. */
    { 073207, 1, { 0000150 }, { 0000141 } },
    { 073147, 1, { 0125032 }, { 0124340 } },             /* JMP I 073201 -> JMP 073107 */
    /* Lifting the lid a second time closed the coffin again and put another metal
       piece in it.  The code worked out the coffin's table index twice; the second
       copy now tests the entry and, if the coffin is already open, just describes
       the room (the call at 035237). */
    { 035207, 4, { 0045127, 0172777, 0120136, 0172464 },
                 { 0054617, 0047140, 0130026, 0044617 } },  /* LDX -113,B; LDA I 035350,X;
                                                              JAP 035237; LDA -113,B */
    /* POUR SOUP.  The routine emptied the cauldron (134322 = 0) before it looked for
       the ice pit, and in the pit called the soup routine, which then found no
       soup.  Test the location first and empty the cauldron only elsewhere; in the
       pit the soup routine empties it. */
    { 0113500, 5, { 0170400, 0005152, 0050153, 0045324, 0140065 },
                  { 0050155, 0045326, 0140065, 0001150, 0140065 } },  /* LDT 195; LDA I LOC;
                                                              SKP EQL; STZ I 134322; SKP EQL */
    /* OPEN VAULT on an open vault closed it again ("OK").  Plain OPEN goes through the
       per-location code at 073321, which answers "It was already open." for an open
       door, but OPEN's VAULT branch jumped straight to the crypt's unlock-and-toggle
       code (073714).  The GATE and VAULT tests at 073150 are rewritten to send VAULT
       at the crypt through 073321 like plain OPEN (via 073163), and elsewhere to
       073714 as before ("There's no vault here!"). */
    { 073150, 11, { 0171147, 0045615, 0140065, 0125035, 0125035, 0125024,
                    0171151, 0045615, 0140065, 0125031, 0125031 },
                  { 0045615, 0171147, 0142065, 0125036, 0171151, 0140065,
                    0124005, 0171037, 0045023, 0140065, 0125031 } },
                  /* LDA I OBJ; SAT GATE; SKP UEQ; JMP I ->073706; SAT VAULT; SKP EQL;
                     JMP 073163; SAT 31; LDA I LOC; SKP EQL; JMP I ->073714 */
    /* A command that takes no turn (LOOK, INVENTORY, GO, WASH, BOW, HELP, INFO, BLAST,
       CALM, WAKE, DIG, SCORE, HOURS, LOST, FUCK, REMOVE, JUJU) went back to read the
       next command at 012777, past the line that clears the object (012775).  The
       object stayed, and the next command took it for its own: REMOVE RING ("Can't!")
       then OPEN COFFIN was OPEN RING.  Those exits now go to 012775.  (A noun on its
       own - "LAMP", "What do you want to do with the lamp?" - keeps its object on
       purpose, through 013410, which is left alone.) */
    { 013641, 1, { 0012777 }, { 0012775 } },             /* LOOK, INVENTORY, GO */
    { 014252, 1, { 0012777 }, { 0012775 } },             /* WASH, BOW, HELP, INFO, BLAST, CALM */
    { 014474, 1, { 0012777 }, { 0012775 } },             /* WAKE, DIG, SCORE, HOURS */
    { 014704, 1, { 0012777 }, { 0012775 } },             /* LOST, FUCK */
    { 015066, 1, { 0012777 }, { 0012775 } },             /* REMOVE, JUJU: "Can't!" */
};

typedef struct {
    const char *exe;          /* program name */
    const char *title;
    const char *image;        /* file in data\ */
    int charset;              /* how the 7-bit text is shown by default */
    int save;                 /* SAVE_... */
    /* SAVE_DUMP: the word the game sets to suspend_value when suspended */
    uint16_t suspend_word, suspend_value;
    /* SAVE_SNAPSHOT: where the runtime reads a command line */
    uint16_t brkm_pc;         /* MON 4 at the start of the line editor */
    uint16_t inbt_pc;         /* its MON 1 */
    uint16_t cmd_frame;       /* frame of the input subroutine ... */
    uint16_t cmd_link;        /* ... whose return link says "the command prompt" */
    uint16_t line_vars;       /* editor's buffer base, end index, start index */
    const Fix *fixes;         /* bugs in the game the port fixes */
    int nfixes;
} Game;

static const Game games[] = {
    /* SETUP (Woods' name) is -1 after "SPAR"/"UTSETT"/"PAUSE" */
    { "skattejakt", "Skattejakt", "SKATTEJAKT.PROG", CS_NORWEGIAN, SAVE_DUMP, 071144, 0177777,
      0, 0, 0, 0, 0, NULL, 0 },
    /* the library line editor at 116275; GETIN (frame 017047) called from the main loop at 013010 */
    { "svha", "SVHA Adventure", "SVHA-ADVENTURE.PROG", CS_ASCII, SAVE_SNAPSHOT, 0, 0,
      0116334, 0116350, 017047, 013011, 0116430,
      svha_fixes, (int)(sizeof svha_fixes / sizeof svha_fixes[0]) },
};

static Cpu cpu;
static const Game *game = &games[GAME];
static const char *save_dir;
static int no_fixes;

/* put the fixes in (or, for --no-fixes, take them out) wherever the code is loaded */
static void apply_fixes(uint16_t *mem)
{
    int i;
    for (i = 0; i < game->nfixes; i++) {
        const Fix *f = &game->fixes[i];
        const uint16_t *from = no_fixes ? f->fixed : f->orig;
        const uint16_t *to = no_fixes ? f->orig : f->fixed;
        if (!memcmp(mem + f->addr, from, (size_t)f->n * sizeof *from))
            memcpy(mem + f->addr, to, (size_t)f->n * sizeof *to);
    }
}

static void usage(FILE *o)
{
    fprintf(o, "usage: %s [options]%s\n\n", game->exe, game->save ? " [SAVED-GAME]" : "");
    fprintf(o, "Runs %s, the original SINTRAN III program, on an ND-100 emulator.\n", game->title);
    if (game->save == SAVE_DUMP)
        fprintf(o, "SAVED-GAME is a game you suspended with SPAR (or UTSETT or PAUSE).\n");
    if (game->save == SAVE_SNAPSHOT)
        fprintf(o, "SAVED-GAME is a game you saved with SAVE (or SUSPEND or PAUSE).\n");
    fprintf(o, "\n");
    if (game->save)
        fprintf(o,
            "  -s, --save-dir DIR   where saved games are written and looked for\n"
            "                       (default: the current directory)\n");
    fprintf(o,
        "  -d, --data DIR       the game's files (default: data next to the program)\n"
        "      --ascii          show the 7-bit national characters as [ \\ ] { | }\n"
        "      --norwegian      show them as Æ Ø Å æ ø å\n"
        "      --raw            pass the terminal bytes through untranslated\n"
        "      --no-hold        do not wait where the program pauses\n"
        "      --vdu            tell the program the terminal is a screen (VT100)\n");
    if (game->nfixes)
        fprintf(o,
            "      --no-fixes       play the game with the bugs the port fixes\n");
    fprintf(o,
        "  -Z, --clock SECONDS  fix the clock at SECONDS since 1970 (before the\n"
        "                       28-year shift), for repeatable sessions\n"
        "      --prog FILE      run another :PROG image instead\n"
        "  -v, --verbose        log monitor calls on standard error\n"
        "  -T, --trace          trace every instruction on standard error\n"
        "  -h, --help           this text\n"
        "      --version        version\n");
}

static int load_prog(Cpu *c, const char *path, uint16_t *start)
{
    FILE *f = fopen(path, "rb");
    unsigned char h[14];
    unsigned first, last, i;
    if (!f)
        return -1;
    if (fread(h, 1, sizeof h, f) != sizeof h) {
        fclose(f);
        return -2;
    }
    if (!memcmp(h, "SVHASAVE", 8)) {
        fclose(f);
        return -4;                       /* a snapshot, not a program */
    }
    *start = (uint16_t)((h[0] << 8) | h[1]);
    first = (unsigned)((h[4] << 8) | h[5]);
    last = (unsigned)((h[6] << 8) | h[7]);
    if (((h[8] << 8) | h[9]) != 0xFFFF || last < first) {
        fclose(f);
        return -3;                       /* two-bank programs are not supported */
    }
    memset(c, 0, sizeof *c);
    fseek(f, 0x200, SEEK_SET);
    for (i = first; i <= last; i++) {
        int hi = fgetc(f), lo = fgetc(f);
        if (hi == EOF || lo == EOF)
            break;
        c->mem[i] = (uint16_t)((hi << 8) | lo);
    }
    fclose(f);
    return 0;
}

/* a one-bank :PROG file of the whole address space, started at 0 like @DUMP F,0,0 */
static int dump_prog(Cpu *c, const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned char h[512];
    long a;
    if (!f)
        return -1;
    memset(h, 0, sizeof h);
    h[6] = 0xFF; h[7] = 0xFF;            /* bank 1 = 0..177777 */
    h[8] = 0xFF; h[9] = 0xFF;            /* no bank 2 */
    fwrite(h, 1, sizeof h, f);
    for (a = 0; a < 65536; a++) {
        fputc(c->mem[a] >> 8, f);
        fputc(c->mem[a] & 0xFF, f);
    }
    return fclose(f);
}

static int exists(const char *p)
{
    FILE *f = fopen(p, "rb");
    if (f) fclose(f);
    return f != NULL;
}

static void join(char *out, size_t n, const char *dir, const char *name)
{
    size_t l;
    if (!dir || !*dir || strchr(name, ':') || name[0] == '\\' || name[0] == '/') {
        snprintf(out, n, "%s", name);
        return;
    }
    l = strlen(dir);
    snprintf(out, n, "%s%s%s", dir, (dir[l - 1] == '/' || dir[l - 1] == '\\') ? "" : "\\", name);
}

static int has_ext(const char *name)
{
    const char *b = strrchr(name, '\\'), *s = strrchr(name, '/'), *d;
    if (s > b) b = s;
    d = strrchr(name, '.');
    return d && (!b || d > b);
}

/* the directory part of a path ("" for a bare name) */
static void dir_of(char *out, size_t n, const char *path)
{
    const char *b = strrchr(path, '\\'), *s = strrchr(path, '/');
    if (s > b) b = s;
    if (b)
        snprintf(out, n, "%.*s", (int)(b - path), path);
    else
        out[0] = 0;
}

/* the directory holding the executable, for data\ */
static void exe_dir(char *out, size_t n, const char *argv0)
{
#ifdef _WIN32
    char *p;
    (void)argv0;
    if (!GetModuleFileNameA(NULL, out, (DWORD)n)) out[0] = 0;
    p = strrchr(out, '\\');
    if (p) *p = 0; else out[0] = 0;
#else
    const char *p = strrchr(argv0, '/');
    if (p) snprintf(out, n, "%.*s", (int)(p - argv0), argv0); else snprintf(out, n, ".");
#endif
}

static const char *find_image(char *buf, size_t n, const char *argv0, const char *data_dir)
{
    char dir[1024], tmp[1200];
    if (data_dir) {
        snprintf(tmp, sizeof tmp, "%s%c%s", data_dir, PATHSEP, game->image);
        if (exists(tmp)) { snprintf(buf, n, "%s", tmp); return buf; }
        return NULL;
    }
    exe_dir(dir, sizeof dir, argv0);
    snprintf(tmp, sizeof tmp, "%s%cdata%c%s", dir, PATHSEP, PATHSEP, game->image);
    if (exists(tmp)) { snprintf(buf, n, "%s", tmp); return buf; }
    snprintf(tmp, sizeof tmp, "%s%c%s", dir, PATHSEP, game->image);
    if (exists(tmp)) { snprintf(buf, n, "%s", tmp); return buf; }
    snprintf(tmp, sizeof tmp, "data%c%s", PATHSEP, game->image);
    if (exists(tmp)) { snprintf(buf, n, "%s", tmp); return buf; }
    return NULL;
}

/* ask for a file name; out gets the path, name what was typed.  0 = cancelled */
static int ask_file(const char *question, const char *ext, char *name, size_t nn, char *out, size_t n)
{
    char file[300];
    term_puts(question);
    if (term_gets(name, (int)nn) < 0 || !name[0])
        return 0;
    snprintf(file, sizeof file, "%s%s", name, has_ext(name) ? "" : ext);
    join(out, n, save_dir, file);
    return 1;
}

static void save_suspended(Cpu *c)
{
    char name[256], path[1400];
    for (;;) {
        if (!ask_file("\r\nSave the suspended game as (file name, or Enter to discard it): ",
                      ".PROG", name, sizeof name, path, sizeof path)) {
            term_puts("Not saved.\r\n");
            return;
        }
        if (dump_prog(c, path) == 0)
            break;
        term_puts("Cannot write that file.\r\n");
    }
    term_puts("Saved as ");
    term_puts(path);
    term_puts(".  Continue it later with:  ");
    term_puts(game->exe);
    term_puts(" ");
    term_puts(name);
    term_puts("\r\n");
}

/* ---- snapshots (SVHA) ---- */

typedef struct {
    uint16_t r[8];
    uint16_t mem[65536];
    int escape_enabled, echo_strategy, break_strategy;
    struct { int used, access, blocksize; long next_block; char name[80]; } files[SIN_MAXFILES];
    char prompt[256];
    int prompt_len;
} Snapshot;

static Snapshot line_snap;       /* the machine as the current command line began */
static int have_snap, line_fresh;

static void snap_take(Snapshot *s, Cpu *c, uint16_t pc)
{
    int i;
    memcpy(s->r, c->r, sizeof s->r);
    s->r[R_P] = pc;
    memcpy(s->mem, c->mem, sizeof s->mem);
    s->escape_enabled = snt.escape_enabled;
    s->echo_strategy = snt.echo_strategy;
    s->break_strategy = snt.break_strategy;
    for (i = 0; i < SIN_MAXFILES; i++) {
        s->files[i].used = snt.files[i].f != NULL;
        s->files[i].access = snt.files[i].access;
        s->files[i].blocksize = snt.files[i].blocksize;
        s->files[i].next_block = snt.files[i].next_block;
        memcpy(s->files[i].name, snt.files[i].name, sizeof s->files[i].name);
    }
    s->prompt_len = snt.out_tail_len;
    memcpy(s->prompt, snt.out_tail, (size_t)snt.out_tail_len);
}

static int snap_put(const Snapshot *s, Cpu *c)
{
    int i;
    sintran_close_all();
    for (i = 0; i < SIN_MAXFILES; i++)
        if (s->files[i].used &&
            sintran_reopen(i, s->files[i].name, s->files[i].access, s->files[i].blocksize,
                           s->files[i].next_block) < 0)
            return -1;
    memcpy(c->r, s->r, sizeof c->r);
    memcpy(c->mem, s->mem, sizeof c->mem);
    apply_fixes(c->mem);                  /* a game saved with or without them */
    snt.escape_enabled = s->escape_enabled;
    snt.echo_strategy = s->echo_strategy;
    snt.break_strategy = s->break_strategy;
    snt.out_tail_len = s->prompt_len;
    memcpy(snt.out_tail, s->prompt, (size_t)s->prompt_len);
    return 0;
}

static void put16(FILE *f, unsigned v) { fputc((v >> 8) & 0xFF, f); fputc(v & 0xFF, f); }
static void put32(FILE *f, unsigned long v) { put16(f, (unsigned)(v >> 16)); put16(f, (unsigned)v); }
static unsigned get16(FILE *f) { int a = fgetc(f), b = fgetc(f); return (a < 0 || b < 0) ? 0 : (unsigned)((a << 8) | b); }
static unsigned long get32(FILE *f) { unsigned long h = get16(f); return (h << 16) | get16(f); }

/* file: "SVHASAVE", version, registers, memory, SINTRAN state, open files, prompt */
static int snap_write(const Snapshot *s, const char *path)
{
    FILE *f = fopen(path, "wb");
    long i;
    if (!f)
        return -1;
    fwrite("SVHASAVE", 1, 8, f);
    put16(f, 1);
    for (i = 0; i < 8; i++) put16(f, s->r[i]);
    for (i = 0; i < 65536; i++) put16(f, s->mem[i]);
    put16(f, (unsigned)s->escape_enabled);
    put16(f, (unsigned)(uint16_t)s->echo_strategy);
    put16(f, (unsigned)(uint16_t)s->break_strategy);
    put16(f, SIN_MAXFILES);
    for (i = 0; i < SIN_MAXFILES; i++) {
        put16(f, (unsigned)s->files[i].used);
        put16(f, (unsigned)s->files[i].access);
        put16(f, (unsigned)s->files[i].blocksize);
        put32(f, (unsigned long)s->files[i].next_block);
        fwrite(s->files[i].name, 1, sizeof s->files[i].name, f);
    }
    put16(f, (unsigned)s->prompt_len);
    fwrite(s->prompt, 1, (size_t)s->prompt_len, f);
    return fclose(f);
}

static int snap_read(Snapshot *s, const char *path)
{
    FILE *f = fopen(path, "rb");
    char magic[8];
    long i;
    int ok;
    if (!f)
        return -1;
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "SVHASAVE", 8) || get16(f) != 1) {
        fclose(f);
        return -2;
    }
    memset(s, 0, sizeof *s);
    for (i = 0; i < 8; i++) s->r[i] = (uint16_t)get16(f);
    for (i = 0; i < 65536; i++) s->mem[i] = (uint16_t)get16(f);
    s->escape_enabled = (int)get16(f);
    s->echo_strategy = (int16_t)get16(f);
    s->break_strategy = (int16_t)get16(f);
    if (get16(f) != SIN_MAXFILES) {
        fclose(f);
        return -2;
    }
    for (i = 0; i < SIN_MAXFILES; i++) {
        s->files[i].used = (int)get16(f);
        s->files[i].access = (int)get16(f);
        s->files[i].blocksize = (int)get16(f);
        s->files[i].next_block = (long)get32(f);
        if (fread(s->files[i].name, 1, sizeof s->files[i].name, f) != sizeof s->files[i].name) {
            fclose(f);
            return -2;
        }
        s->files[i].name[sizeof s->files[i].name - 1] = 0;
    }
    s->prompt_len = (int)get16(f);
    if (s->prompt_len > (int)sizeof s->prompt)
        s->prompt_len = 0;
    ok = fread(s->prompt, 1, (size_t)s->prompt_len, f) == (size_t)s->prompt_len;
    fclose(f);
    return ok ? 0 : -2;
}

static void show_prompt(void)
{
    char buf[260];
    memcpy(buf, snt.out_tail, (size_t)snt.out_tail_len);
    buf[snt.out_tail_len] = 0;
    term_puts(buf);
}

static int at_command_prompt(Cpu *c)
{
    return c->mem[(uint16_t)(game->cmd_frame - 128)] == game->cmd_link;
}

/* the command typed so far, as the editor holds it, first word upper-cased */
static void command_word(Cpu *c, char *w, int n)
{
    uint16_t base = c->mem[game->line_vars];
    unsigned end = c->mem[(uint16_t)(game->line_vars + 1)];
    unsigned i = c->mem[(uint16_t)(game->line_vars + 2)];
    int k = 0;
    for (; i < end && i < 400; i++) {
        uint16_t v = c->mem[(uint16_t)(base + (i >> 1))];
        int ch = ((i & 1) ? v : (v >> 8)) & 0x7F;
        if (ch == ' ') {
            if (k) break;
            continue;
        }
        if (k < n - 1) w[k++] = (char)toupper(ch);
    }
    w[k] = 0;
}

/* the whole command line as typed, trailing blanks dropped */
static void command_line(Cpu *c, char *w, int n)
{
    uint16_t base = c->mem[game->line_vars];
    unsigned end = c->mem[(uint16_t)(game->line_vars + 1)];
    unsigned i = c->mem[(uint16_t)(game->line_vars + 2)];
    int k = 0;
    for (; i < end && i < 400 && k < n - 1; i++) {
        uint16_t v = c->mem[(uint16_t)(base + (i >> 1))];
        w[k++] = (char)(((i & 1) ? v : (v >> 8)) & 0x7F);
    }
    while (k > 0 && w[k - 1] == ' ')
        k--;
    w[k] = 0;
}

static int debug_mode;

/* octal, or decimal with a trailing '.'; a leading '-' negates */
static long number(const char *s, int *ok)
{
    char *e;
    long v;
    int neg = *s == '-';
    size_t n = strlen(s);
    if (neg) s++;
    if (n && s[strlen(s) - 1] == '.')
        v = strtol(s, &e, 10);
    else
        v = strtol(s, &e, 8);
    *ok = e != s && (*e == 0 || (*e == '.' && e[1] == 0));
    return neg ? -v : v;
}

/* --debug: "#peek ADDR [COUNT]", "#poke ADDR VALUE...", "#find VALUE [FROM [TO]]" at the
   command prompt, octal unless a number ends in '.'; the game never sees the line */
static void debug_command(Cpu *c, const char *line)
{
    char buf[400], out[200];
    char *tok[40];
    int nt = 0, ok = 1, i;
    long a, cnt;
    snprintf(buf, sizeof buf, "%s", line + 1);
    for (tok[nt] = strtok(buf, " "); tok[nt] && nt < 39; tok[++nt] = strtok(NULL, " "))
        ;
    term_puts("\r\n");
    snap_put(&line_snap, c);
    if (nt >= 2 && !strcmp(tok[0], "peek")) {
        a = number(tok[1], &ok);
        cnt = nt >= 3 ? number(tok[2], &ok) : 1;
        for (i = 0; ok && i < cnt && i < 512; i++) {
            uint16_t v = c->mem[(uint16_t)(a + i)];
            if (i % 8 == 0) {
                snprintf(out, sizeof out, "%s%06lo:", i ? "\r\n" : "", (unsigned long)(uint16_t)(a + i));
                term_puts(out);
            }
            snprintf(out, sizeof out, " %06o", v);
            term_puts(out);
        }
        term_puts("\r\n");
    } else if (nt >= 3 && !strcmp(tok[0], "poke")) {
        a = number(tok[1], &ok);
        for (i = 2; ok && i < nt; i++) {
            uint16_t v = (uint16_t)number(tok[i], &ok);
            if (!ok) break;
            c->mem[(uint16_t)(a + i - 2)] = v;
            line_snap.mem[(uint16_t)(a + i - 2)] = v;
        }
        term_puts(ok ? "ok\r\n" : "bad number\r\n");
    } else if (nt >= 2 && !strcmp(tok[0], "find")) {
        long v = number(tok[1], &ok), from = nt >= 3 ? number(tok[2], &ok) : 0;
        long to = nt >= 4 ? number(tok[3], &ok) : 0177777;
        int hits = 0;
        for (a = from; ok && a <= to && hits < 64; a++)
            if (c->mem[a] == (uint16_t)v) {
                snprintf(out, sizeof out, " %06lo", a);
                term_puts(out);
                if (++hits % 10 == 0) term_puts("\r\n");
            }
        term_puts("\r\n");
    } else {
        term_puts("#peek ADDR [COUNT] | #poke ADDR VALUE... | #find VALUE [FROM [TO]]  (octal, or decimal with '.')\r\n");
    }
    if (!ok)
        term_puts("bad number\r\n");
    show_prompt();
}

static int input_hook(Cpu *c, int ch)
{
    char word[16], name[256], path[1400];
    Snapshot *loaded;
    if (ch != '\r' || !have_snap || c->trap_pc != game->inbt_pc || !at_command_prompt(c))
        return 0;
    if (debug_mode) {
        char line[200];
        command_line(c, line, sizeof line);
        if (line[0] == '#') {
            debug_command(c, line);
            return 1;
        }
    }
    command_word(c, word, sizeof word);
    word[6] = 0;                               /* the game reads six letters */
    if (!strcmp(word, "SAVE") || !strcmp(word, "SUSPEN") || !strcmp(word, "PAUSE")) {
        term_puts("\r\n");
        for (;;) {
            if (!ask_file("Save the game as (file name, or Enter to cancel): ", ".SAV",
                          name, sizeof name, path, sizeof path)) {
                term_puts("Not saved.\r\n");
                break;
            }
            if (snap_write(&line_snap, path) == 0) {
                term_puts("Saved as ");
                term_puts(path);
                term_puts(".  RESTORE brings it back, or start it with:  ");
                term_puts(game->exe);
                term_puts(" ");
                term_puts(name);
                term_puts("\r\n");
                break;
            }
            term_puts("Cannot write that file.\r\n");
        }
        snap_put(&line_snap, c);
        show_prompt();
        return 1;
    }
    if (!strcmp(word, "RESTOR")) {
        term_puts("\r\n");
        loaded = malloc(sizeof *loaded);
        if (loaded && ask_file("Restore the game from (file name, or Enter to cancel): ", ".SAV",
                               name, sizeof name, path, sizeof path)) {
            int e = snap_read(loaded, path);
            if (e == -1) {
                term_puts("There is no file ");
                term_puts(path);
                term_puts(".\r\n");
            } else if (e != 0) {
                term_puts("That is not a saved game.\r\n");
            } else if (snap_put(loaded, c) != 0) {
                term_puts("The game's files are missing.\r\n");
                snap_put(&line_snap, c);
            } else {
                line_snap = *loaded;
                term_puts("Restored.\r\n");
                free(loaded);
                show_prompt();
                return 1;
            }
        } else {
            term_puts("Not restored.\r\n");
        }
        free(loaded);
        snap_put(&line_snap, c);
        show_prompt();
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *prog = NULL, *saved = NULL, *data_dir = NULL;
    static char image[1500], path[1500], image_dir[1500];
    int raw = 0, charset = game->charset, trace = 0, played = 0, i, rc = 0, lr;
    uint16_t start = 0;

    sintran_init();
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(stdout); return 0; }
        else if (!strcmp(a, "--version")) { printf("%s %s\n", game->exe, VERSION); return 0; }
        else if ((!strcmp(a, "-s") || !strcmp(a, "--save-dir")) && i + 1 < argc && game->save)
            save_dir = argv[++i];
        else if (!strncmp(a, "--save-dir=", 11) && game->save) save_dir = a + 11;
        else if ((!strcmp(a, "-d") || !strcmp(a, "--data")) && i + 1 < argc) data_dir = argv[++i];
        else if (!strncmp(a, "--data=", 7)) data_dir = a + 7;
        else if (!strcmp(a, "--ascii")) charset = CS_ASCII;
        else if (!strcmp(a, "--norwegian")) charset = CS_NORWEGIAN;
        else if (!strcmp(a, "--raw")) { raw = 1; charset = CS_ASCII; }
        else if (!strcmp(a, "--no-hold")) snt.no_hold = 1;
        else if (!strcmp(a, "--vdu")) snt.terminal_type = 0166006;   /* VT100: screen, handles BS */
        else if ((!strcmp(a, "-Z") || !strcmp(a, "--clock")) && i + 1 < argc)
            snt.fixed_clock = (time_t)strtoll(argv[++i], NULL, 10);
        else if (!strcmp(a, "--prog") && i + 1 < argc) prog = argv[++i];
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) snt.verbose = 1;
        else if (!strcmp(a, "-T") || !strcmp(a, "--trace")) trace = 1;
        else if (!strcmp(a, "--no-fixes") && game->nfixes) no_fixes = 1;
        else if (!strcmp(a, "--debug")) debug_mode = 1;
        else if (a[0] == '-' && a[1]) {
            fprintf(stderr, "%s: unknown option %s (try --help)\n", game->exe, a);
            return 2;
        } else if (!game->save) {
            fprintf(stderr, "%s: unexpected argument %s (try --help)\n", game->exe, a);
            return 2;
        } else if (!saved) saved = a;
        else {
            fprintf(stderr, "%s: only one saved game at a time (try --help)\n", game->exe);
            return 2;
        }
    }

    if (!find_image(image_dir, sizeof image_dir, argv[0], data_dir))
        image_dir[0] = 0;
    if (prog)
        snprintf(image_dir, sizeof image_dir, "%s", prog);
    if (saved) {
        const char *ext = game->save == SAVE_DUMP ? ".PROG" : ".SAV";
        char try1[1500];
        join(try1, sizeof try1, save_dir, saved);
        if (!exists(try1) && !has_ext(saved)) {
            snprintf(path, sizeof path, "%.1390s%s", try1, ext);
            if (exists(path)) snprintf(try1, sizeof try1, "%s", path);
        }
        if (!exists(try1) && save_dir && exists(saved))
            snprintf(try1, sizeof try1, "%s", saved);
        snprintf(image, sizeof image, "%s", try1);
    } else if (image_dir[0]) {
        snprintf(image, sizeof image, "%s", image_dir);
    } else {
        fprintf(stderr, "%s: cannot find %s in %s\n", game->exe, game->image,
                data_dir ? data_dir : "data\\ next to the program");
        return 1;
    }
    /* the game's files live beside its image, unless --data says otherwise */
    if (data_dir) {
        snt.data_dir = data_dir;
    } else {
        dir_of(path, sizeof path, image_dir);
        snprintf(image_dir, sizeof image_dir, "%s", path);
        snt.data_dir = image_dir;
    }

    lr = load_prog(&cpu, image, &start);
    if (lr == -4 && game->save == SAVE_SNAPSHOT) {
        Snapshot *s = malloc(sizeof *s);
        if (!s || snap_read(s, image) != 0) {
            fprintf(stderr, "%s: %s is not a saved game\n", game->exe, image);
            return 1;
        }
        if (snap_put(s, &cpu) != 0) {
            fprintf(stderr, "%s: cannot open the game's files in %s\n", game->exe,
                    snt.data_dir && *snt.data_dir ? snt.data_dir : ".");
            return 1;
        }
        line_snap = *s;
        have_snap = 1;
        free(s);
        start = cpu.r[R_P];
        lr = 0;
    } else if (lr == 0) {
        apply_fixes(cpu.mem);
        cpu.r[R_P] = start;
    }
    switch (lr) {
    case 0: break;
    case -1: fprintf(stderr, "%s: cannot open %s\n", game->exe, image); return 1;
    case -3: fprintf(stderr, "%s: %s is not a one-bank :PROG file\n", game->exe, image); return 1;
    default: fprintf(stderr, "%s: %s is not a :PROG file\n", game->exe, image); return 1;
    }
    if (game->save == SAVE_SNAPSHOT)
        snt.input_hook = input_hook;

    term_init(raw, charset);
    if (have_snap)
        show_prompt();
    for (;;) {
        int r;
        if (trace) {
            char buf[64];
            uint16_t pc = cpu.r[R_P];
            fprintf(stderr, "%06o %06o %-24s A=%06o D=%06o T=%06o X=%06o B=%06o L=%06o S=%03o\n",
                    pc, cpu.mem[pc], cpu_dis(cpu.mem[pc], pc, buf), cpu.r[R_A], cpu.r[R_D],
                    cpu.r[R_T], cpu.r[R_X], cpu.r[R_B], cpu.r[R_L], cpu.r[R_STS] & 0377);
        }
        r = cpu_step(&cpu);
        if (cpu.zset && snt.verbose) {
            char buf[64];
            fprintf(stderr, "[Z set at %06o by %06o %s]\n", cpu.trap_pc, cpu.trap_word,
                    cpu_dis(cpu.trap_word, cpu.trap_pc, buf));
        }
        if (r == TRAP_NONE)
            continue;
        if (r == TRAP_MON) {
            int s;
            if (game->save == SAVE_DUMP) {
                /* a game counts as played once it reads the terminal while not
                   suspended; a resume the game refuses never gets that far */
                if (cpu.mon == 1 && cpu.mem[game->suspend_word] != game->suspend_value)
                    played = 1;
            } else if (game->save == SAVE_SNAPSHOT) {
                if (cpu.mon == 4 && cpu.trap_pc == game->brkm_pc)
                    line_fresh = 1;
                else if (cpu.mon == 1 && cpu.trap_pc == game->inbt_pc && line_fresh) {
                    line_fresh = 0;
                    if (at_command_prompt(&cpu)) {
                        snap_take(&line_snap, &cpu, cpu.trap_pc);
                        have_snap = 1;
                    }
                }
            }
            s = sintran_mon(&cpu, cpu.mon);
            if (s == SIN_RUN)
                continue;
            if (s == SIN_EXIT) {
                term_puts("\r\n");
                if (game->save == SAVE_DUMP && played &&
                    cpu.mem[game->suspend_word] == game->suspend_value)
                    save_suspended(&cpu);
            } else if (s == SIN_BREAK) {
                char msg[64];
                /* as SINTRAN prints it: "USER BREAK AT   53031B" */
                snprintf(msg, sizeof msg, "\r\nUSER BREAK AT %7oB\r\n", snt.exit_pc);
                term_puts(msg);
                rc = 1;
            } else if (s == SIN_EOF) {
                term_puts("\r\n");
            } else {
                rc = 3;
            }
            break;
        }
        {
            char buf[64], msg[160];
            static const char *why[] = { "", "", "ILLEGAL INSTRUCTION", "PRIVILEGED INSTRUCTION",
                                         "UNIMPLEMENTED INSTRUCTION", "EXR OF EXR" };
            snprintf(msg, sizeof msg, "\r\n%s AT %6oB (%06o %s)\r\n", why[r], cpu.trap_pc,
                     cpu.trap_word, cpu_dis(cpu.trap_word, cpu.trap_pc, buf));
            term_puts(msg);
            rc = 3;
            break;
        }
    }
    sintran_close_all();
    term_hold();
    term_restore();
    return rc;
}
