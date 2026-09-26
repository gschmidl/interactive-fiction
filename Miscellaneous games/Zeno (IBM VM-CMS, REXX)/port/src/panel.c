/* ------------------------------------------------------------------
 * panel.c - the IOS3270 panel engine.
 *
 * IOS3270 (IBM 5785-HAX, written at IBM Uithoorn) formatted 3270
 * full-screen panels from a flat file, substituting REXX variables
 * through the CMS EXECCOMM interface and handing back what the user
 * typed.  Zeno drives it with ten of its thirty-odd directives.
 *
 * The directive set and the field-definition characters below are as
 * documented in the product's own online help, IOS3270 IOS3270, which
 * shipped alongside the game.  Where that help is ambiguous the
 * behaviour here is the one that makes the author's box-drawing line
 * up; see notes on elastic blanks in render_line().
 * ------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "zeno.h"

#define MAXLINE 256
#define MAXSECT 4096

/* ---- field-definition characters ------------------------------------
 * Slot order is taken from the help file's own legend:
 *   1 intensify              6 pen select
 *   2 input                  7 pen select, intensify
 *   3 input, intensify       8 variable place holder
 *   4 input, intensify, skip 9 fill character   (initially unset)
 *   5 input, non display    10 input, autoskip  (initially unset)
 * Slots 9 and 10 start out undefined, which is what lets Zeno draw its
 * boxes out of '_' and '|'.
 */
#define FC_SLOTS 10
#define SLOT_PLACEHOLDER 8
#define SLOT_FILL        9

static const char fc_default[FC_SLOTS + 1] = {
    0, '%', '$', '^', '#', '\xa2', '@', '!', '.', 0, 0
};

static const unsigned fc_flags[FC_SLOTS + 1] = {
    0,
    F_INTENS,                          /* 1 %  */
    F_INPUT,                           /* 2 $  */
    F_INPUT | F_INTENS,                /* 3 ^  */
    F_INPUT | F_INTENS | F_SKIP,       /* 4 #  */
    F_INPUT | F_NONDISP,               /* 5 ¢  */
    F_PEN,                             /* 6 @  */
    F_PEN | F_INTENS,                  /* 7 !  */
    0,                                 /* 8 placeholder - not a field */
    0,                                 /* 9 fill - not a field */
    F_INPUT | F_SKIP                   /* 10 | */
};

typedef struct {
    char  fc[FC_SLOTS + 1];
    int   row;                 /* next output row, 0-based */
    int   intens_lines;        /* .h */
    int   fieldchars;          /* .c */
    int   in_bottom;           /* .b seen: text goes to the bottom title */
    char  bottom[8][MAXLINE];
    int   nbottom;
    Screen *s;
    int   depth;
} PState;

static char libdir[512] = ".";

void panel_set_libdir(const char *dir)
{
    snprintf(libdir, sizeof libdir, "%s", dir);
}

/* ---- screen primitives ---------------------------------------------- */

static void put_cell(Screen *s, int row, int col, char c, unsigned char fl, int fid)
{
    int p;
    if (row < 0 || row >= SROWS || col < 0 || col >= SCOLS) return;
    p = row * SCOLS + col;
    s->ch[p]  = c;
    s->fl[p]  = fl;
    s->fid[p] = (short)fid;
}

static int new_field(Screen *s, int row, int col, int len, unsigned flags,
                     const char *var)
{
    Field *f;
    if (s->nf >= MAXFIELDS) return -1;
    f = &s->f[s->nf];
    f->attrpos = row * SCOLS + col;
    f->len     = len;
    f->flags   = flags;
    f->mdt     = 0;
    f->var[0]  = 0;
    if (var) snprintf(f->var, MAXVAR, "%s", var);
    return s->nf++;
}

/* ---- variable references --------------------------------------------
 * A name runs from '&' over the REXX symbol characters.  The variable
 * place-holder character (default '.', which Zeno remaps to '$' so that
 * stems like &rmsg.1 stay in one piece) terminates it explicitly.
 */
static int is_symchar(char c, char placeholder)
{
    if (c == placeholder) return 0;
    return isalnum((unsigned char)c) || c == '.' || c == '_' ||
           c == '@' || c == '#' || c == '$' || c == '?' || c == '!';
}

/* Parse "&name" at text[i].  Returns bytes consumed, name in out. */
static int parse_varname(const char *text, int i, char placeholder,
                         char *out, int outsz, int *had_placeholder)
{
    int start = i, n = 0;
    i++;                                  /* skip '&' */
    while (text[i] && is_symchar(text[i], placeholder)) {
        if (n < outsz - 1) out[n++] = (char)toupper((unsigned char)text[i]);
        i++;
    }
    out[n] = 0;
    *had_placeholder = 0;
    if (n && text[i] == placeholder && placeholder) {
        *had_placeholder = 1;
        i++;
    }
    return i - start;
}

/* Substitute every &var in a directive argument (used by .i and friends) */
static void subst_vars(const char *in, char *out, int outsz, char placeholder)
{
    int i = 0, o = 0;
    while (in[i] && o < outsz - 1) {
        if (in[i] == '&' && is_symchar(in[i + 1], placeholder)) {
            char name[MAXVAR], val[512];
            int hp, used = parse_varname(in, i, placeholder, name, sizeof name, &hp);
            int vl = rx_fetch(name, val, sizeof val);
            if (vl < 0) vl = 0;
            if (vl > outsz - 1 - o) vl = outsz - 1 - o;
            memcpy(out + o, val, vl);
            o += vl;
            i += used;
        } else {
            out[o++] = in[i++];
        }
    }
    out[o] = 0;
}

/* ---- one text line --------------------------------------------------- */

/* Number of screen columns a value occupies.  A 3270 Start Field order
 * (0x1D followed by an attribute byte) is two bytes but one column. */
static int display_width(const char *v, int len)
{
    int i, w = 0;
    for (i = 0; i < len; i++) {
        if ((unsigned char)v[i] == 0x1D && i + 1 < len) { i++; w++; }
        else w++;
    }
    return w;
}

/* Write a value into the screen starting at (row,col), honouring any
 * embedded Start Field orders.  Returns the column just past it. */
static int emit_value(PState *p, int row, int col, const char *v, int len,
                      unsigned char base_fl, int fid)
{
    int i;
    unsigned char fl = base_fl;
    for (i = 0; i < len && col < SCOLS; i++) {
        unsigned char c = (unsigned char)v[i];
        if (c == 0x1D && i + 1 < len) {
            unsigned char a = (unsigned char)v[++i];
            /* 3270 attribute: bits 2..3 of the low nibble select the
             * display class; 0x08 in that field means intensified. */
            fl = 0;
            if ((a & 0x0C) == 0x08) fl |= C_INTENS;
            put_cell(p->s, row, col, ' ', (unsigned char)(fl | C_ATTR), fid);
            col++;
        } else {
            put_cell(p->s, row, col, (char)c, fl, fid);
            col++;
        }
    }
    return col;
}

static int slot_of(PState *p, char c)
{
    int k;
    if (!c) return 0;
    for (k = 1; k <= FC_SLOTS; k++)
        if (p->fc[k] && p->fc[k] == c) return k;
    return 0;
}

/*
 * A field-definition character only opens a field when something can
 * follow it into one: a length, a variable, or text.  Zeno's book pages
 * contain eight literal '!' - "Be warned!", "stops you suffocating!)" -
 * and '!' happens to be the pen-select-intensify character.  Requiring a
 * letter, digit or '&' next keeps the author's punctuation intact while
 * still honouring '%same', where the attribute byte itself supplies the
 * space between the two words.
 */
static int opens_field(PState *p, const char *text, int i)
{
    int slot = slot_of(p, text[i]);
    char n;
    if (!slot || slot == SLOT_PLACEHOLDER || slot == SLOT_FILL) return 0;
    n = text[i + 1];
    if (n == '&' || isalnum((unsigned char)n)) return slot;
    return 0;
}

/* Length of the blank run starting at text[i] */
static int blank_run(const char *text, int i)
{
    int n = 0;
    while (text[i + n] == ' ') n++;
    return n;
}

/*
 * Render one panel text line at row `row`.
 *
 * Columns: literal non-blanks advance the cursor one column each, but a
 * run of literal blanks is ELASTIC - the item that follows is pulled
 * back to its own source column if the cursor has not already passed it.
 * That single rule is what keeps the author's boxes square whether a
 * substituted value came out shorter than its placeholder (the room
 * title) or very much longer (a 60-column VDU line dropped into an
 * eleven-character &vduline.1$).
 */
static void render_line(PState *p, const char *text, int row, int intens)
{
    Screen *s = p->s;
    char placeholder = p->fc[SLOT_PLACEHOLDER];
    int i = 0, col = 0;
    unsigned char base = intens ? C_INTENS : 0;

    while (text[i] && col < SCOLS) {
        int slot;

        /* Elastic blanks.  A blank run always renders at least one blank
         * - so a floated value keeps its separator - but otherwise it
         * stretches or shrinks to land the next item on its own source
         * column. */
        if (text[i] == ' ') {
            int n = blank_run(text, i);
            int want = i + n;                     /* source column of next item */
            if (!text[i + n]) break;              /* trailing blanks: drop */
            /* A value long enough to reach the next literal has already
             * drawn it - this is what VDUSHOW's "nasty fiddle" does when
             * it appends its own box edge - so do not draw it twice. */
            if (col > want && want < SCOLS &&
                s->ch[row * SCOLS + want] == text[want] &&
                !(s->fl[row * SCOLS + want] & C_INPUT)) {
                col = want + 1;
                i = want + 1;
                continue;
            }
            if (col < SCOLS) put_cell(s, row, col, ' ', base, 0);
            col++;
            if (col < want) col = want;
            i += n;
            continue;
        }

        slot = p->fieldchars ? opens_field(p, text, i) : 0;

        if (slot) {
            /* ---- a field ---- */
            unsigned flags = fc_flags[slot];
            int attrcol = col, width = 0, j = i + 1;
            char name[MAXVAR] = "";
            char val[1024];
            int vlen = 0, hp = 0;

            while (isdigit((unsigned char)text[j])) width = width * 10 + (text[j++] - '0');

            if (text[j] == '&') {
                j += parse_varname(text, j, placeholder, name, sizeof name, &hp);
                vlen = rx_fetch(name, val, sizeof val);
                if (vlen < 0) vlen = 0;
            } else {
                /* literal content: up to the next field character */
                int k = j;
                while (text[k] && !opens_field(p, text, k)) k++;
                vlen = k - j;
                if (vlen > (int)sizeof val) vlen = (int)sizeof val;
                memcpy(val, text + j, vlen);
                j = k;
            }

            if (!width) {
                /* no explicit length: run to the next field char or to
                 * the end of the trailing blank run */
                int k = j;
                while (text[k] == ' ') k++;
                if (!text[k]) k = (int)strlen(text);
                width = k - (i + 1);
                if (width < display_width(val, vlen)) width = display_width(val, vlen);
                j = k;
            }
            if (attrcol + 1 + width > SCOLS) width = SCOLS - attrcol - 1;
            if (width < 0) width = 0;

            {
                int fid = new_field(s, row, attrcol, width, flags, name);
                unsigned char cfl = 0;
                int c2;
                if (flags & F_INTENS)  cfl |= C_INTENS;
                if (flags & F_INPUT)   cfl |= C_INPUT;
                if (flags & F_NONDISP) cfl |= C_NONDISP;
                put_cell(s, row, attrcol, ' ', (unsigned char)(cfl | C_ATTR), fid + 1);
                c2 = emit_value(p, row, attrcol + 1, val, vlen, cfl, fid + 1);
                while (c2 < attrcol + 1 + width && c2 < SCOLS)
                    put_cell(s, row, c2++, ' ', cfl, fid + 1);
                col = attrcol + 1 + width;
            }
            i = j;
            continue;
        }

        if (text[i] == '&' && is_symchar(text[i + 1], placeholder)) {
            char name[MAXVAR], val[1024];
            int hp, used = parse_varname(text, i, placeholder, name, sizeof name, &hp);
            int vlen = rx_fetch(name, val, sizeof val);
            if (vlen < 0) vlen = 0;
            col = emit_value(p, row, col, val, vlen, base, 0);
            i += used;
            continue;
        }

        put_cell(s, row, col, text[i], base, 0);
        col++; i++;
    }
}

/* ---- panel file access ----------------------------------------------- */

static FILE *open_panel_file(const char *fn, const char *ft)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s.%s", libdir, fn, ft);
    if ((f = fopen(path, "rb"))) return f;
    snprintf(path, sizeof path, "%s/%s %s", libdir, fn, ft);
    return fopen(path, "rb");
}

static void chomp(char *s)
{
    int n = (int)strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = 0;
}

/* ---- directives ------------------------------------------------------- */

static int render_section(PState *p, const char *fn, const char *ft,
                          const char *label);

/*
 * A control line is '.' followed by a run of function letters, some of
 * which take an argument.  '.j' swallows the rest of the line there and
 * then (its argument may start with blanks); '.f' and '.i' take what
 * follows the whole letter run, which is why '.bcfy HELP % QUIT ...'
 * still honours the trailing 'y'.
 */
static void do_directive(PState *p, const char *line)
{
    int i = 1;                      /* skip the '.' */
    char consumer = 0;              /* 'f' or 'i': takes the rest of the line */

    while (line[i] && line[i] != ' ') {
        char c = (char)tolower((unsigned char)line[i]);
        switch (c) {
        case 'c': p->fieldchars = 1; i++; break;
        case 'n': p->s->no_edit = 1; i++; break;
        case 'y': p->s->pass_on_pf = 1; i++; break;
        case 'a': p->s->alarm = 1; i++; break;
        case 's': p->row++; i++; break;
        case 'p': p->row = 0; i++; break;
        case 't': i++; break;
        case 'h': {
            int n = 0;
            i++;
            while (isdigit((unsigned char)line[i])) n = n * 10 + (line[i++] - '0');
            p->intens_lines = n ? n : 1;
            break;
        }
        case 'l': {
            int n = 0, neg = 0;
            i++;
            if (line[i] == '-') { neg = 1; i++; }
            while (isdigit((unsigned char)line[i])) n = n * 10 + (line[i++] - '0');
            p->row = neg ? p->row + n : n - 1;
            if (p->row < 0) p->row = 0;
            break;
        }
        case 'b':
            p->in_bottom = 1;
            i++;
            break;
        case 'j': {
            int slot = 0;
            i++;
            if (line[i]) i++;                 /* separator character */
            while (line[i] && slot < FC_SLOTS) {
                slot++;
                if (line[i] == ' ')      { /* leave as-is */ }
                else if (line[i] == '&') p->fc[slot] = fc_default[slot];
                else                     p->fc[slot] = line[i];
                i++;
            }
            return;
        }
        case 'f':
            i++;
            if (line[i] == '2') { consumer = 'F'; i++; }   /* PF13-24 */
            else consumer = 'f';
            break;
        case 'i':
            consumer = 'i';
            i++;
            break;
        default:
            i++;
            break;
        }
    }

    while (line[i] == ' ') i++;
    if (!consumer || !line[i]) return;

    if (consumer == 'f' || consumer == 'F') {
        int hi = (consumer == 'F') ? 12 : 0, k = 1;
        char args[MAXLINE], *tok;
        snprintf(args, sizeof args, "%s", line + i);
        for (tok = strtok(args, " "); tok && k <= 12; tok = strtok(NULL, " "), k++) {
            if (!strcmp(tok, "%")) continue;              /* omitted key */
            snprintf(p->s->pf[hi + k], 32, "%s", tok);
        }
    } else {                                              /* .i - imbed */
        char arg[MAXLINE];
        char fn[32] = "", ft[32] = "", fm[32] = "", lab[64] = "";
        subst_vars(line + i, arg, sizeof arg, p->fc[SLOT_PLACEHOLDER]);
        if (sscanf(arg, "%31s %31s %31s %63s", fn, ft, fm, lab) < 4)
            sscanf(arg, "%31s %31s %63s", fn, ft, lab);
        if (lab[0] && lab[0] == ';' && p->depth < 8) {
            p->depth++;
            render_section(p, fn, ft, lab);
            p->depth--;
        }
    }
}

static int render_section(PState *p, const char *fn, const char *ft,
                          const char *label)
{
    FILE *f = open_panel_file(fn, ft);
    char line[MAXLINE];
    int found = 0;

    if (!f) { zlog("panel: cannot open %s %s", fn, ft); return 0; }

    while (fgets(line, sizeof line, f)) {
        chomp(line);
        if (!found) {
            if (line[0] == ';' && !strcmp(line, label)) found = 1;
            continue;
        }
        if (line[0] == ';') break;                  /* next section */

        if (line[0] == '.') {
            do_directive(p, line);
            continue;
        }
        if (p->in_bottom) {
            if (p->nbottom < 8)
                snprintf(p->bottom[p->nbottom++], MAXLINE, "%s", line);
            continue;
        }
        {
            int intens = 0;
            if (p->intens_lines > 0) { intens = 1; p->intens_lines--; }
            if (p->row < SROWS) render_line(p, line, p->row, intens);
            p->row++;
        }
    }
    fclose(f);
    return found;
}

/* ---- entry point ------------------------------------------------------ */

int panel_render(Screen *s, const char *file, const char *label)
{
    PState p;
    char fn[32], ft[32];
    const char *dot;
    int k, ok;

    memset(s, 0, sizeof *s);
    for (k = 0; k < SCELLS; k++) s->ch[k] = ' ';
    s->cursor_row = -1;

    memset(&p, 0, sizeof p);
    memcpy(p.fc, fc_default, sizeof p.fc);
    p.s = s;

    dot = strchr(file, '.');
    if (dot) {
        snprintf(fn, sizeof fn, "%.*s", (int)(dot - file), file);
        snprintf(ft, sizeof ft, "%s", dot + 1);
    } else {
        snprintf(fn, sizeof fn, "%s", file);
        snprintf(ft, sizeof ft, "IOS3270");
    }

    ok = render_section(&p, fn, ft, label);

    /* bottom title lines occupy the last rows, intensified */
    if (p.nbottom) {
        int base = SROWS - p.nbottom;
        for (k = 0; k < p.nbottom; k++)
            render_line(&p, p.bottom[k], base + k, 1);
    }
    return ok;
}
