/* misty.c -- run Misty Marsh's own Wang word-processing glossary.
 *
 * The game was not written as a program.  It is a **glossary** -- a Wang OIS
 * word-processing keystroke macro -- that types a document at you and reads
 * your answers back.  `tools/extract.py` lifts it off the disk image and
 * `tools/gendata.py` compiles it into `data.h`; every word this prints and
 * every branch it takes is the glossary's own.
 *
 * What the machine has to provide:
 *
 *   the document   what the glossary types.  Here it is just the terminal:
 *                  text, (-RETURN-) and (-TAB-) go straight out, word-wrapped
 *                  to 76 columns the way the workstation wrapped to 80.
 *
 *   the prompt     (-PROMPT-)text(-EXECUTE-) writes the workstation's prompt
 *                  line -- the "instructions and taunts" the game's own
 *                  instructions tell you to watch.
 *
 *   the keyboard   (-N-KEYS-) collects what the operator types until EXECUTE.
 *                  What was typed goes into the document, so the cursor sits
 *                  just past it; the glossary then walks back over the word
 *                  with (-BACKSPACE-) and spells it out, one (-IF-) per
 *                  letter with (-EAST-) between.  So the answer needs a
 *                  cursor of its own -- see answer_is() below.
 *
 *   page f         the game's memory.  Doors already opened are recorded by
 *                  inserting a mark on a scratch page and found again with
 *                  (-SEARCH-); (-IF-)(-PAGE-) asks whether the search ran off
 *                  the end, i.e. whether the mark was absent.  Modelled here
 *                  as a list of marks and a substring test, which is all the
 *                  glossary ever asks of it.
 *
 *   page w         the scoreboard, and the one place the game does arithmetic.
 *                  Each prize writes its points there as a decimal-tabbed
 *                  number -- "5", "10", "15";
 *                  at the end, entries (1)(2)(3) walk the column and add it up
 *                  with the word processor's own math command -- `+a=` opens
 *                  accumulator a on the number under the cursor, `a+` adds the
 *                  next one, `a#` totals -- then copy the total into the
 *                  document after "Your score = ".  That is why the game's own
 *                  closing note apologises for scores that come out as "00".
 *
 * Nothing is simulated beyond that.  Cursor moves and formatting keys are
 * carried through as spacing or ignored, because that is all they did to a
 * document that only ever gets read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"

#define WRAP     76
#define MAXMARK  64
#define MARKLEN  16

/* A scratch page: the marks written on it, and where the cursor sits.  Only
 * two exist -- f, the doors already opened, and w, the score column. */
typedef struct {
    char id;
    char item[MAXMARK][MARKLEN];
    int  n;
    int  cur;
} Page;

static Page  pages[4];
static int   npages;
static Page *page;                    /* 0 = the document                 */
static int   page_cond;               /* "the cursor is on a page mark"   */
static long  acc, total;              /* the math accumulator and its sum */
static char  clip[24];                /* what (-COPY-) picked up          */
static char  answer[64];              /* what (-N-KEYS-) collected        */
static int   apos;                    /* the cursor inside it             */
static char  prompt[160];
static int   col;                     /* output column, for wrapping      */
static long  steps;

/* ------------------------------------------------------------ output */

static void put_nl(void)
{
    putchar('\n');
    col = 0;
}

static void put_word(const char *w, int n)
{
    if (n == 0) return;
    if (col + n > WRAP) put_nl();
    fwrite(w, 1, n, stdout);
    col += n;
}

/* Text arrives in fragments, so wrapping is done a word at a time. */
static void emit(const char *s)
{
    while (*s) {
        const char *w;
        if (*s == ' ') {
            if (col > 0 && col < WRAP) { putchar(' '); col++; }
            s++;
            continue;
        }
        w = s;
        while (*s && *s != ' ') s++;
        put_word(w, (int)(s - w));
    }
}

static void show_prompt(void)
{
    if (prompt[0]) printf("\n[ %s ]\n", prompt);
}

/* ------------------------------------------------ the scratch pages */

/* (-GO-TO-PAGE-) names a page.  f and w are the two the glossary writes on;
 * every other id it uses is a page of the document itself, which is the same
 * thing here as being back in the document. */
static void page_open(char id)
{
    int i;
    if (id != 'f' && id != 'w') { page = 0; return; }
    for (i = 0; i < npages; i++)
        if (pages[i].id == id) { page = &pages[i]; page->cur = 0; return; }
    page = &pages[npages++];
    page->id = id;
    page->n = page->cur = 0;
}

static void page_insert(const char *m)
{
    if (!m || !*m || page->n >= MAXMARK) return;
    strncpy(page->item[page->n], m, MARKLEN - 1);
    page->item[page->n][MARKLEN - 1] = 0;
    page->n++;
}

/* Search forward from the cursor.  Not finding it means the cursor has run on
 * to the page mark at the end, which is what (-IF-)(-PAGE-) then tests. */
static int page_search(const char *m)
{
    int i;
    if (!page || !m || !*m) return 0;
    for (i = page->cur; i < page->n; i++)
        if (strstr(page->item[i], m)) { page->cur = i; return 1; }
    page->cur = page->n;
    return 0;
}

/* The number the cursor is on: marks are written tab-then-value. */
static long page_value(void)
{
    const char *s;
    if (!page || page->cur >= page->n) return 0;
    s = page->item[page->cur];
    while (*s && !isdigit((unsigned char)*s)) s++;
    return strtol(s, 0, 10);
}

/* ------------------------------------------------------------ keyboard */

static void read_answer(void)
{
    int c, n = 0;
    show_prompt();
    fputs("> ", stdout);
    fflush(stdout);
    answer[0] = 0;
    while ((c = getchar()) != EOF && c != '\n')
        if (n < (int)sizeof answer - 1 && c != '\r') answer[n++] = (char)c;
    answer[n] = 0;
    while (n > 0 && answer[n - 1] == ' ') answer[--n] = 0;
    if (c == EOF) { put_nl(); exit(0); }
    apos = n;                 /* cursor lands just past what was typed */
    col = 0;
}

/* (-IF-) tests the character **under the cursor**, not the whole answer.
 *
 * What the operator types goes into the document, so the cursor ends up just
 * past it; the glossary then walks back over the word with (-BACKSPACE-) and
 * spells it out, one (-IF-) per letter, stepping (-EAST-) between them.  The
 * pit puzzle is the clearest case -- three backspaces, then d, i, g:
 *
 *   (-N-KEYS-)(-BACKSPACE-)(-BACKSPACE-)(-BACKSPACE-)
 *   (-IF-)"d"(-EAST-)(-IF-)"i"(-EAST-)(-IF-)"g"(-GO-TO-GL-)l(-END-)(-END-)(-END-)
 *   (-GO-TO-GL-)j
 *
 * A one-key answer is the same thing with a single backspace, which is why the
 * y/n and 1-7 prompts look like a plain comparison. */
static int answer_is(const char *want)
{
    if (!want || !*want) return 0;
    if (apos < 0 || apos >= (int)strlen(answer)) return 0;
    return tolower((unsigned char)answer[apos]) == tolower((unsigned char)want[0]);
}

/* ------------------------------------------------------------ the loop */

static const Entry *find_entry(char key)
{
    int i;
    for (i = 0; i < n_entries; i++)
        if (entries[i].key == key) return &entries[i];
    return 0;
}

/* Skip a failed (-IF-) to its matching (-END-). */
static int skip_to_end(const Op *ops, int i)
{
    int depth = 1;
    while (ops[i].op != OP_NONE) {
        if (ops[i].op == OP_IFKEY || ops[i].op == OP_IFCOND) depth++;
        else if (ops[i].op == OP_END && --depth == 0) return i + 1;
        i++;
    }
    return i;
}

static int cond_true(const char *arg, int is_key)
{
    int neg = (arg && arg[0] == '!');
    const char *a = neg ? arg + 1 : arg;
    int v = is_key ? answer_is(a) : page_cond;
    return neg ? !v : v;
}

int main(void)
{
    const Entry *e = find_entry('\001');   /* the damaged opening block */
    char key = '\001';
    int i = 0, self = 0;

    if (!e) { fputs("no start entry\n", stderr); return 1; }

    for (;;) {
        const Op *o = &e->ops[i];
        if (o->op == OP_NONE) {
            /* The glossary ran out without chaining: the macro has stopped and
             * the operator is back in the document.  Ask again. */
            if (prompt[0]) { read_answer(); i = 0; continue; }
            put_nl();
            emit("[the glossary ends here]");
            put_nl();
            return 0;
        }
        i++;
        if (++steps > 2000000L) { put_nl(); emit("[runaway glossary]"); put_nl(); return 1; }

        switch (o->op) {
        case OP_TEXT:      emit(o->arg ? o->arg : ""); break;
        case OP_RETURN:    put_nl(); break;
        case OP_TAB:       /* to the next 8-column stop, indent included */
                           do { putchar(' '); col++; } while (col % 8); break;
        case OP_EAST:
            /* On a page the cursor is the one walking the column; in the
             * document it is the one walking what the operator typed. */
            if (page) { if (page->cur < page->n) page->cur++; }
            else if (apos < (int)strlen(answer)) apos++;
            break;
        case OP_SOUTH:     put_nl(); break;
        case OP_BEEP:      putchar('\a'); break;

        case OP_PROMPT: {
            /* the prompt text is the ops up to EXECUTE */
            prompt[0] = 0;
            while (e->ops[i].op != OP_NONE && e->ops[i].op != OP_EXECUTE) {
                if (e->ops[i].op == OP_TEXT && e->ops[i].arg) {
                    strncat(prompt, e->ops[i].arg,
                            sizeof prompt - strlen(prompt) - 1);
                }
                i++;
            }
            if (e->ops[i].op == OP_EXECUTE) i++;
            break;
        }

        case OP_NKEYS:     read_answer(); break;

        case OP_IFKEY:     if (!cond_true(o->arg, 1)) i = skip_to_end(e->ops, i); break;
        case OP_IFCOND:    if (!cond_true(o->arg, 0)) i = skip_to_end(e->ops, i); break;
        case OP_END:       break;

        case OP_GOTO: {
            char t = o->arg ? o->arg[0] : 0;
            const Entry *n = find_entry(t);
            if (!n) { put_nl(); printf("[no glossary (%c)]\n", t ? t : '?'); return 1; }
            self = (t == key) ? self + 1 : 0;
            if (self > 6) {
                /* (=) chains to itself for ever: the beeping cursor walk the
                 * operator was meant to CANCEL out of.  Stop after a few. */
                put_nl();
                emit("[the glossary is looping -- CANCEL]");
                put_nl();
                return 0;
            }
            key = t; e = n; i = 0;
            break;
        }

        case OP_PAGE:
            /* (-GO-TO-PAGE-) with no id is a move a page at a time; it does
             * not change which page we are on for our purposes. */
            if (o->arg && o->arg[0]) page_open(o->arg[0]);
            if (page) page_cond = (page->n == 0);
            break;
        case OP_PAGERET:   page = 0; break;

        case OP_SEARCH:    page_cond = !page_search(o->arg); break;

        case OP_INSERT:
            if (page) page_insert(o->arg);
            else {
                /* An insert into the document types at the cursor, and the
                 * cursor is sitting on whatever was last pasted there. */
                emit(o->arg ? o->arg : "");
                if (clip[0]) { emit(clip); clip[0] = 0; }
            }
            break;

        case OP_CMD:
            /* The word processor's column arithmetic; see the head of the file. */
            if (!o->arg) break;
            else if (!strcmp(o->arg, "+a=")) acc = page_value();
            else if (!strcmp(o->arg, "a+"))  acc += page_value();
            else if (!strcmp(o->arg, "a#"))  total = acc;
            break;

        case OP_COPY:
            sprintf(clip, "%ld", total);
            break;

        /* Editor keys with nothing to do to a document that is only read. */
        case OP_BACKSPACE: if (apos > 0) apos--; break;

        case OP_EXECUTE: case OP_CANCEL: case OP_NOTE:
        case OP_CENTER:  case OP_UNDERSCORE: case OP_COMMAND: case OP_DECTAB:
        case OP_ERROR:   case OP_DELETE: case OP_FORMAT:
        case OP_INDENT:
            break;
        default:
            break;
        }
    }
}
