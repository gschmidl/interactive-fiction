/* dx10.c - the DX10 3.7 supervisor calls (XOP 15) the Adventure task makes,
 * and the SCI environment it expects.
 *
 * The task is TI 990 FORTRAN; its run-time does all I/O through SVC >00.  At
 * the start it reads the station's record of SCI's synonym file (.S$FGTCA,
 * pre-assigned to LUNO >01) to find what FORTRAN units 1, 2, 3 and 5 are:
 * the ADVENTUR procedure sets UNIT1 = UNIT2 = ME (the terminal), UNIT3 = the
 * data file .GAMES.FILES.CAVE and UNIT5 = the save file or DUMY.  LUNO >00
 * is the terminal, for the run-time's own messages ("STOP 1").
 *
 * Call blocks, flags and file semantics are from the DX10 manuals (Vol. III,
 * 946250-9703, sections 8-10 and appendix E). */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include "cpu990.h"
#include "dx10.h"
#include "images.h"

struct dx10_config dx10 = { 0, 1, 0, NULL, 0, 0 };

#define TRACE(...) do { if (dx10.trace) fprintf(stderr, __VA_ARGS__); } while (0)

/* ------------------------------------------------------------------ */
/* the terminal */

static int tty = -1;            /* is stdin a terminal (it echoes input)? */
static int skip_lf;             /* the user's Enter already began a new line */
static int at_col0 = 1;

void term_write(const char *s, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];

        if (c == '\r')
            continue;
        if (c == '\n') {
            if (skip_lf)
                skip_lf = 0;
            else
                putchar('\n');
            at_col0 = 1;
            continue;
        }
        skip_lf = 0;
        if (c < ' ' || c > '~')
            continue;
        putchar(c);
        at_col0 = 0;
    }
}

void term_puts(const char *s)
{
    term_write(s, (int)strlen(s));
}

/* read a line: at most MAX characters kept, upper-cased (the terminals of
 * the reference system send capitals only) unless RAW.  When the input is
 * not a terminal the line is echoed, so that a transcript reads as a
 * session. */
static int term_read(char *buf, int max, int raw)
{
    char line[512];
    int n, i;

    if (tty < 0)
        tty = isatty(fileno(stdin));
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin))
        return -1;
    n = (int)strcspn(line, "\r\n");
    if (line[n] == 0 && n == (int)sizeof line - 1) {    /* drop the rest */
        int c;

        while ((c = getchar()) != EOF && c != '\n')
            ;
    }
    line[n] = 0;
    for (i = 0; i < n && !raw; i++)
        line[i] = (char)toupper((unsigned char)line[i]);
    if (tty) {
        skip_lf = 1;
        at_col0 = 1;
    } else
        term_write(line, n);
    if (n > max)
        n = max;
    memcpy(buf, line, (size_t)n);
    return n;
}

int term_gets(char *buf, int max)
{
    return term_read(buf, max, 0);
}

int term_gets_raw(char *buf, int max)
{
    return term_read(buf, max, 1);
}

void term_end(void)
{
    if (!at_col0)
        putchar('\n');
    at_col0 = 1;
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* the clock */

void dx10_now(struct tm *t)
{
    time_t now = dx10.virtual_clock ? dx10.clock : time(NULL);

    *t = *localtime(&now);
    if (dx10.virtual_clock)
        dx10.clock++;
}

/* ------------------------------------------------------------------ */
/* SCI's synonym record for the station (.S$FGTCA record 1): a header, then
 * length-prefixed name/value pairs from offset >3A, ended by a zero length.
 * The layout is that of the records on the reference system's disk. */

#define SAVE_NAME ".GAMES.SAVE"         /* UNIT5 when a save file is in use */

static uint8_t tca[864];

static void tca_build(void)
{
    static const char *syn[][2] = {
        {"ME", "ST01"}, {"$$ST", "01"}, {"$$UI", "SYS001"}, {"$$CC", "00000"},
        {"$$BC", "00000"}, {"GAMES", ".GAMES.GAMES"}, {"UNIT1", "ME"},
        {"UNIT2", "ME"}, {"UNIT3", ".GAMES.GAMES.FILES.CAVE"}, {"UNIT5", "DUMY"},
    };
    size_t i, n = sizeof syn / sizeof syn[0];
    int p = 0x3A;

    memset(tca, 0, sizeof tca);
    tca[0] = 0x03;                      /* the record length, 864 */
    tca[1] = 0x60;
    tca[3] = 0x10;
    tca[9] = 0x3A;                      /* where the synonyms start */
    tca[10] = 0x03;
    tca[11] = 0x60;
    memcpy(tca + 0x10, "SYS001", 6);
    for (i = 0; i < n; i++) {
        const char *v = syn[i][1];
        size_t a, b;

        if (!strcmp(syn[i][0], "UNIT5") && dx10.save_path)
            v = SAVE_NAME;
        a = strlen(syn[i][0]);
        b = strlen(v);
        tca[p++] = (uint8_t)a;
        memcpy(tca + p, syn[i][0], a);
        p += (int)a;
        tca[p++] = (uint8_t)b;
        memcpy(tca + p, v, b);
        p += (int)b;
    }
}

/* ------------------------------------------------------------------ */
/* the save file: a sequential file of records (the procedure creates it with
 * 80-byte records).  On the host each record is a 2-byte big-endian length
 * and the bytes; the whole file is kept in memory and written back on every
 * change. */

static struct {
    uint8_t *data;              /* the records, back to back as on the host */
    long size, cap;
    long pos;                   /* byte offset of the current record */
} sv;

static int save_load(void)
{
    FILE *f = fopen(dx10.save_path, "rb");
    long n;

    sv.size = sv.pos = 0;
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n > sv.cap) {
        sv.cap = n + 4096;
        sv.data = realloc(sv.data, (size_t)sv.cap);
    }
    sv.size = (long)fread(sv.data, 1, (size_t)n, f);
    fclose(f);
    return 0;
}

static int save_store(void)
{
    FILE *f = fopen(dx10.save_path, "wb");
    int ok;

    if (!f)
        return -1;
    ok = fwrite(sv.data, 1, (size_t)sv.size, f) == (size_t)sv.size;
    return fclose(f) == 0 && ok ? 0 : -1;
}

/* ------------------------------------------------------------------ */
/* LUNOs */

enum { L_NONE, L_TERM, L_DUMMY, L_TCA, L_CAVE, L_SAVE };

static struct {
    int kind;
    char path[64];
} lu[256];

static uint8_t *cave;           /* the data file, in memory for the run */
static int started;

static void start(void)
{
    started = 1;
    tca_build();
    cave = malloc(sizeof cave_data);
    memcpy(cave, cave_data, sizeof cave_data);
    lu[0].kind = L_TERM;
    strcpy(lu[0].path, "ME");
    lu[1].kind = L_TCA;
    strcpy(lu[1].path, ".S$FGTCA");
}

static uint16_t rw16(uint16_t blk, int off)
{
    return rdw((uint16_t)(blk + off));
}

static void ww16(uint16_t blk, int off, uint16_t v)
{
    wrw((uint16_t)(blk + off), v);
}

/* DX10 error codes returned in byte 1 */
#define E_LUNO   0x0002         /* LUNO not assigned */
#define E_OP     0x0005         /* operation not valid for the device */
#define E_NOFILE 0x0034         /* pathname not found */
#define E_IO     0x00FF

static int io_svc(uint16_t blk)
{
    int sub = mem[(uint16_t)(blk + 2)], luno = mem[(uint16_t)(blk + 3)];
    uint16_t buf = rw16(blk, 6), len = rw16(blk, 8), cnt = rw16(blk, 10);
    int kind = lu[luno].kind, err = 0;

    TRACE("SVC >00 >%02X LUNO >%02X (%s) buf >%04X len %u count %u\n", sub, luno,
          kind ? lu[luno].path : "unassigned", buf, len, cnt);
    mem[(uint16_t)(blk + 4)] = 0;       /* the system flags */
    if (sub != 0x91 && kind == L_NONE) {
        err = E_LUNO;
        goto done;
    }
    switch (sub) {
    case 0x91: {                        /* assign LUNO */
        uint16_t pa = rw16(blk, 22);
        int n = pa ? mem[pa] : 0, i;
        char path[64];

        for (i = 0; i < n && i < 63; i++)
            path[i] = (char)mem[(uint16_t)(pa + 1 + i)];
        path[i] = 0;
        if (mem[(uint16_t)(blk + 16)] & 0x04) {         /* generate a LUNO */
            for (i = 0x10; i < 0xFF && lu[i].kind; i++)
                ;
            luno = i;
            mem[(uint16_t)(blk + 3)] = (uint8_t)luno;
        }
        if (!strcmp(path, "ME") || !strcmp(path, "ST01"))
            kind = L_TERM;
        else if (!strcmp(path, "DUMY"))
            kind = L_DUMMY;
        else if (!strcmp(path, ".GAMES.GAMES.FILES.CAVE"))
            kind = L_CAVE;
        else if (!strcmp(path, SAVE_NAME) && dx10.save_path)
            kind = L_SAVE;
        else {
            err = E_NOFILE;
            break;
        }
        lu[luno].kind = kind;
        strcpy(lu[luno].path, path);
        TRACE("  \"%s\" -> LUNO >%02X\n", path, luno);
        break;
    }
    case 0x93:                          /* release LUNO */
        lu[luno].kind = L_NONE;
        break;
    case 0x00:                          /* open */
    case 0x03:                          /* open rewind */
    case 0x12:                          /* open extend */
        switch (kind) {
        case L_TERM:
            ww16(blk, 6, 0x0001);                       /* a teleprinter */
            if (!len)
                ww16(blk, 8, 80);
            break;
        case L_DUMMY:
            ww16(blk, 6, 0x0000);
            break;
        case L_TCA:
        case L_CAVE:
            ww16(blk, 6, 0x02FF);                       /* relative record */
            if (!len)
                ww16(blk, 8, kind == L_TCA ? 864 : CAVE_LRL);
            break;
        case L_SAVE:
            if (save_load() < 0) {
                err = E_NOFILE;
                break;
            }
            if (sub == 0x12)
                sv.pos = sv.size;
            ww16(blk, 6, 0x01FF);                       /* sequential */
            if (!len)
                ww16(blk, 8, 80);
            break;
        }
        break;
    case 0x01:                          /* close */
    case 0x04:                          /* close unload */
        break;
    case 0x02:                          /* close, write EOF */
    case 0x0D:                          /* write EOF */
        if (kind == L_TERM)             /* the teleprinter feeds paper */
            term_write("\r\n\n\n\n", 5);
        else if (kind == L_SAVE) {
            sv.size = sv.pos;
            if (save_store() < 0)
                err = E_IO;
        }
        break;
    case 0x0E:                          /* rewind */
        if (kind == L_SAVE)
            sv.pos = 0;
        break;
    case 0x09:                          /* read ASCII */
    case 0x0A:                          /* read direct */
        switch (kind) {
        case L_TERM: {
            int max = len;
            int n;

            if (buf + max > (int)sizeof mem)
                max = (int)sizeof mem - buf;
            n = term_gets((char *)mem + buf, max);

            if (n < 0)
                return DX10_HANGUP;
            ww16(blk, 10, (uint16_t)n);
            break;
        }
        case L_DUMMY:
            mem[(uint16_t)(blk + 4)] |= 0x20;           /* end of file */
            ww16(blk, 10, 0);
            break;
        case L_TCA: {
            int n = len < sizeof tca ? len : (int)sizeof tca;

            memcpy(mem + buf, tca, (size_t)n);
            ww16(blk, 10, (uint16_t)n);
            break;
        }
        case L_CAVE: {
            long rec = (long)rw16(blk, 12) << 16 | rw16(blk, 14);
            int n = len < CAVE_LRL ? len : CAVE_LRL;

            if (rec < 0 || rec >= CAVE_RECS) {
                mem[(uint16_t)(blk + 4)] |= 0x20;
                ww16(blk, 10, 0);
                break;
            }
            memcpy(mem + buf, cave + rec * CAVE_LRL, (size_t)n);
            ww16(blk, 10, (uint16_t)n);
            rec++;
            ww16(blk, 12, (uint16_t)(rec >> 16));
            ww16(blk, 14, (uint16_t)rec);
            TRACE("  record %ld\n", rec - 1);
            break;
        }
        case L_SAVE: {
            int n, rl;

            if (sv.pos + 2 > sv.size) {
                mem[(uint16_t)(blk + 4)] |= 0x20;
                ww16(blk, 10, 0);
                break;
            }
            rl = sv.data[sv.pos] << 8 | sv.data[sv.pos + 1];
            if (sv.pos + 2 + rl > sv.size) {
                err = E_IO;
                break;
            }
            n = rl < len ? rl : len;
            memcpy(mem + buf, sv.data + sv.pos + 2, (size_t)n);
            if ((mem[(uint16_t)(blk + 5)] & 0x01) && n < len) {  /* blank adjust */
                memset(mem + buf + n, ' ', (size_t)(len - n));
                n = len;
            }
            ww16(blk, 10, (uint16_t)n);
            sv.pos += 2 + rl;
            break;
        }
        }
        break;
    case 0x0B:                          /* write ASCII */
    case 0x0C:                          /* write direct */
    case 0x10:                          /* rewrite */
        switch (kind) {
        case L_TERM: {
            int n = cnt;

            if (mem[(uint16_t)(blk + 5)] & 0x01)        /* blank adjustment: */
                while (n > 0 && mem[(uint16_t)(buf + n - 1)] == ' ')
                    n--;                                /* no trailing blanks */
            term_write((const char *)mem + buf, n);
            break;
        }
        case L_DUMMY:
            break;
        case L_TCA:
            memcpy(tca, mem + buf, cnt < sizeof tca ? cnt : sizeof tca);
            break;
        case L_CAVE: {
            long rec = (long)rw16(blk, 12) << 16 | rw16(blk, 14);

            if (rec < 0 || rec >= CAVE_RECS || cnt > CAVE_LRL) {
                err = E_IO;
                break;
            }
            memset(cave + rec * CAVE_LRL, 0, CAVE_LRL);
            memcpy(cave + rec * CAVE_LRL, mem + buf, cnt);
            rec++;
            ww16(blk, 12, (uint16_t)(rec >> 16));
            ww16(blk, 14, (uint16_t)rec);
            TRACE("  record %ld written\n", rec - 1);
            break;
        }
        case L_SAVE: {
            long need = sv.pos + 2 + cnt;

            if (sub == 0x10)            /* rewrite: the record before */
                break;
            if (need > sv.cap) {
                sv.cap = need + 4096;
                sv.data = realloc(sv.data, (size_t)sv.cap);
            }
            sv.data[sv.pos] = (uint8_t)(cnt >> 8);
            sv.data[sv.pos + 1] = (uint8_t)cnt;
            memcpy(sv.data + sv.pos + 2, mem + buf, cnt);
            sv.pos = sv.size = need;    /* a write ends the file there */
            if (save_store() < 0)
                err = E_IO;
            break;
        }
        }
        break;
    case 0x05:                          /* read characteristics */
    case 0x06:                          /* forward space */
    case 0x07:                          /* backward space */
    case 0x0F:                          /* unload */
    default:
        fprintf(stderr, "dx10: I/O sub-opcode >%02X on %s is not supported\n",
                sub, lu[luno].path);
        err = E_OP;
        break;
    }
done:
    mem[(uint16_t)(blk + 1)] = (uint8_t)err;
    if (err) {
        mem[(uint16_t)(blk + 4)] |= 0x40;
        TRACE("  error >%02X\n", err);
    }
    return 0;
}

/* where the decoded settings of CAVE record 27 sit once the task has read
 * them: the prime-time windows ((first hour << 8) | last hour) for weekdays,
 * weekend days and holidays, and the minutes a suspended game must wait */
#define SET_WEEKDAY 0xD6CC
#define SET_WEEKEND 0xD6CE
#define SET_HOLIDAY 0xD466
#define SET_LATENCY 0xD468

/* the date and time call inside the run-time's RANDOM, which seeds the
 * generator from the minute and the second and calls again while the
 * minute is 0 */
#define RANDOM_SEED_CALL 0x3BB4

int dx10_svc(uint16_t blk)
{
    int code = mem[blk];

    if (!started)
        start();
    switch (code) {
    case 0x00:
        return io_svc(blk);
    case 0x03: {                        /* date and time */
        struct tm t;
        uint16_t b = rw16(blk, 2);
        int minute;

        dx10_now(&t);
        minute = t.tm_min;
        if (dx10.fixes && minute == 0 && (uint16_t)(cpu_pc - 2) == RANDOM_SEED_CALL)
            minute = 60;                /* fix: no busy wait for the next minute */
        mem[(uint16_t)(blk + 1)] = 0;
        wrw(b, (uint16_t)(t.tm_year + 1900));
        wrw((uint16_t)(b + 2), (uint16_t)(t.tm_yday + 1));
        wrw((uint16_t)(b + 4), (uint16_t)t.tm_hour);
        wrw((uint16_t)(b + 6), (uint16_t)minute);
        wrw((uint16_t)(b + 8), (uint16_t)t.tm_sec);
        if (dx10.unlimited) {
            wrw(SET_WEEKDAY, 0);
            wrw(SET_WEEKEND, 0);
            wrw(SET_HOLIDAY, 0);
            wrw(SET_LATENCY, 0);
        }
        TRACE("SVC >03 %02d:%02d:%02d at >%04X\n", t.tm_hour, minute, t.tm_sec,
              (uint16_t)(cpu_pc - 2));
        return 0;
    }
    case 0x04:                          /* end of task */
        TRACE("SVC >04\n");
        return DX10_END_TASK;
    case 0x16:                          /* end of program */
        TRACE("SVC >16\n");
        return DX10_END_PROGRAM;
    case 0x17:                          /* get parameters: from .BID in the */
        mem[(uint16_t)(blk + 1)] = 0;   /* foreground at station 1 */
        ww16(blk, 2, 0x0101);
        ww16(blk, 4, 0);
        TRACE("SVC >17\n");
        return 0;
    case 0x0A: {                        /* binary (R0) to decimal */
        int v = (int16_t)rdw(cpu_wp), i;
        unsigned u = (unsigned)(v < 0 ? -v : v);

        mem[(uint16_t)(blk + 1)] = 0;
        mem[(uint16_t)(blk + 2)] = v < 0 ? '-' : ' ';
        for (i = 7; i >= 3; i--) {
            mem[(uint16_t)(blk + i)] = (uint8_t)(u || i == 7 ? '0' + u % 10 : ' ');
            u /= 10;
        }
        TRACE("SVC >0A %d\n", v);
        return 0;
    }
    default:
        fprintf(stderr, "dx10: SVC >%02X (at >%04X) is not supported\n", code,
                (uint16_t)(cpu_pc - 2));
        return DX10_UNKNOWN;
    }
}
