/* wang.c -- Wang OIS 928 workstation shim for WANG 928 ADVENTURE (Version 2.1).
 *
 * The original program is a bare-metal Z80 image for a Wang OIS workstation.
 * It is loaded at 0100h and reaches the rest of the machine through:
 *
 *   0004h / 0005h   request mailbox: a command byte plus a pointer to a
 *                   parameter block. The supervisor clears 0004h once the
 *                   request has been accepted.
 *   command 01h     file I/O; the parameter block is an IOCB (see below)
 *   command 02h     read the time-of-day counters (used to seed the RNG)
 *   command 10h     spool/print request
 *   C000h + r*100h  screen attribute plane, 24 rows of 80
 *   E000h + r*100h  screen character plane, 24 rows of 80 (00h is a space)
 *   IN  00h         keyboard scan code, delivered by a mode-0 interrupt
 *   OUT 00h         keyboard controller acknowledge
 *   OUT 01h         halt the workstation
 *   OUT 03h         display refresh
 *   IN  07h         workstation sub-model, read when 0007h says model 5
 *   0030h / 0038h   supervisor entries: abort and normal exit
 *
 * All of the above was recovered from the two program images; there is no
 * emulator for the OIS, so this file stands in for the hardware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "z80.h"
#include "wang.h"

#define IMG_BASE   0x0100
#define STACK_TOP  0xC000      /* top of user RAM; the heap stops 4 pages below */
#define KEYTAB     0x434D      /* resident scan code -> character table */
#define KEYBUF     0x4272      /* resident one-character keyboard buffer */

uint8_t mem[0x10000];
int     running = 1;
int     debug = 0;
int     cur_row = 0, cur_col = 0;

static Z80 cpu;
static int screen_dirty = 1;

static uint16_t *trace_ring = 0;
static int trace_len = 0, trace_pos = 0, trace_full = 0;

/* ------------------------------------------------------------------ files */

#define MAX_FILES 8
typedef struct { FILE *fp; long blocks; int used; } WFile;
static WFile files[MAX_FILES];
static char  exedir[1024];

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "\n[wangadv] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

/* Look for a game file in <exedir>\data, <exedir>, .\data and . */
static FILE *open_game_file(const char *name, long *blocks)
{
    char path[2048];
    FILE *fp = NULL;
    int i;
    const char *dirs[2];

    dirs[0] = exedir;
    dirs[1] = ".";

    for (i = 0; i < 2 && !fp; i++) {
        sprintf(path, "%s/data/%s", dirs[i], name);
        fp = fopen(path, "rb");
        if (!fp) {
            sprintf(path, "%s/%s", dirs[i], name);
            fp = fopen(path, "rb");
        }
    }
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    *blocks = (ftell(fp) + 255) / 256;
    fseek(fp, 0, SEEK_SET);
    return fp;
}

/* A file spec reads "/library:pir=NAME////"; pull NAME out of it. */
static int spec_to_name(uint16_t p, char *out, int outsz)
{
    int i, j = 0;
    for (i = 0; i < 64; i++) {
        uint16_t q = (uint16_t)(p + i);
        if (mem[q] == 'p' && mem[(uint16_t)(q + 1)] == 'i' &&
            mem[(uint16_t)(q + 2)] == 'r' && mem[(uint16_t)(q + 3)] == '=') {
            q = (uint16_t)(q + 4);
            while (j < outsz - 1) {
                uint8_t c = mem[q++];
                if (c == '/' || c == 0 || c == ' ' || c == ':') break;
                out[j++] = (char)c;
            }
            out[j] = 0;
            return j > 0;
        }
    }
    return 0;
}

/* ------------------------------------------------------------- screen text */

char wang_to_ascii(uint8_t c)
{
    if (c == 0x00) return ' ';
    if (c >= 0x20 && c < 0x7F) return (char)c;
    if (c >= 0xA0 && c < 0xFF) {
        char m = (char)(c & 0x7F);
        if (m >= 0x20) return m;
    }
    return ' ';
}

/* ---------------------------------------------------------------- keyboard */

static uint8_t scan_of[256];      /* ASCII -> Wang scan code, 0 = none */
static int     kq[64], kq_head, kq_tail;

static void build_keymap(void)
{
    int sc;
    memset(scan_of, 0, sizeof scan_of);
    for (sc = 255; sc >= 0; sc--) {
        uint8_t ch = mem[KEYTAB + sc];
        if (ch == 0 || ch == 0xFF) continue;
        scan_of[ch] = (uint8_t)sc;    /* prefer the unshifted code */
    }
}

uint8_t key_scancode(unsigned char ascii) { return scan_of[ascii]; }
void    kq_push(int sc) { int n = (kq_tail + 1) % 64; if (n != kq_head) { kq[kq_tail] = sc; kq_tail = n; } }
int     kq_empty(void)  { return kq_head == kq_tail; }
static int kq_pop(void) { int v; if (kq_empty()) return 0; v = kq[kq_head]; kq_head = (kq_head + 1) % 64; return v; }

/* ------------------------------------------------------------- OS mailbox */

/* IOCB layout, relative to the pointer left in 0005h. */
#define IO_FUNC   0
#define IO_STAT   1
#define IO_HANDLE 2
#define IO_COUNT  3
#define IO_BUF    4
#define IO_BLOCK  6
#define IO_FLAG   8
#define IO_XSTAT  9
#define IO_EXTRA  10

static uint16_t rd16(uint16_t a) { return (uint16_t)(mem[a] | (mem[(uint16_t)(a + 1)] << 8)); }
static void     wr16(uint16_t a, uint16_t v) { mem[a] = (uint8_t)v; mem[(uint16_t)(a + 1)] = (uint8_t)(v >> 8); }

static void os_file_io(uint16_t iocb)
{
    uint8_t  fn     = mem[iocb + IO_FUNC];
    uint8_t  handle = mem[iocb + IO_HANDLE];
    uint8_t  count  = mem[iocb + IO_COUNT];
    uint16_t buf    = rd16((uint16_t)(iocb + IO_BUF));
    uint16_t block  = rd16((uint16_t)(iocb + IO_BLOCK));

    switch (fn) {
    case 0: {                                    /* open */
        char name[64];
        long blocks = 0;
        FILE *fp;
        int h;
        if (!spec_to_name(buf, name, sizeof name)) { mem[iocb + IO_STAT] = 1; return; }
        fp = open_game_file(name, &blocks);
        if (debug) fprintf(stderr, "[open] %s -> %s\n", name, fp ? "ok" : "not found");
        if (!fp) { mem[iocb + IO_STAT] = 1; return; }
        for (h = 1; h < MAX_FILES; h++) if (!files[h].used) break;
        if (h == MAX_FILES) { fclose(fp); mem[iocb + IO_STAT] = 2; return; }
        files[h].fp = fp; files[h].blocks = blocks; files[h].used = 1;
        mem[iocb + IO_HANDLE] = (uint8_t)h;
        wr16((uint16_t)(iocb + IO_BLOCK), (uint16_t)blocks);
        mem[iocb + IO_EXTRA] = (uint8_t)blocks;
        mem[iocb + IO_STAT] = 0x80;
        return; }

    case 1: {                                    /* read `count` 256-byte blocks */
        int i, n = count ? count : 256;
        if (handle >= MAX_FILES || !files[handle].used) { mem[iocb + IO_STAT] = 1; return; }
        if (debug) fprintf(stderr, "[read] h=%d blk=%u n=%d -> %04X\n", handle, block, n, buf);
        if (fseek(files[handle].fp, (long)block * 256, SEEK_SET) != 0) {
            mem[iocb + IO_STAT] = 5; return;
        }
        for (i = 0; i < n; i++) {
            uint8_t sect[256];
            int j;
            size_t got = fread(sect, 1, 256, files[handle].fp);
            if (got == 0) { mem[iocb + IO_XSTAT] = (uint8_t)i; mem[iocb + IO_STAT] = 5; return; }
            memset(sect + got, 0, 256 - got);
            for (j = 0; j < 256; j++) mem[(uint16_t)(buf + i * 256 + j)] = sect[j];
        }
        mem[iocb + IO_XSTAT] = count;
        mem[iocb + IO_STAT] = 0x80;
        return; }

    case 4:                                      /* close */
        if (handle < MAX_FILES && files[handle].used) {
            fclose(files[handle].fp);
            files[handle].used = 0;
        }
        mem[iocb + IO_STAT] = 0x80;
        return;

    case 8:                                      /* query extent */
        if (handle < MAX_FILES && files[handle].used)
            wr16((uint16_t)(iocb + IO_BLOCK), (uint16_t)files[handle].blocks);
        mem[iocb + IO_EXTRA] = 0;
        mem[iocb + IO_STAT] = 0x80;
        return;

    case 2: case 6: case 0x0B:                   /* write / extend: read-only media */
        if (debug) fprintf(stderr, "[io] write-type fn=%u refused\n", fn);
        mem[iocb + IO_STAT] = 5;
        return;

    default:
        if (debug) fprintf(stderr, "[io] unknown fn=%u\n", fn);
        mem[iocb + IO_STAT] = 5;
        return;
    }
}

static void os_request(uint8_t cmd)
{
    uint16_t p = rd16(0x0005);

    switch (cmd) {
    case 0x01:
        os_file_io(p);
        break;

    case 0x02: {                                 /* time of day, six counters */
        time_t t = time(NULL);
        struct tm *lt = localtime(&t);
        mem[(uint16_t)(p + 0)] = (uint8_t)(lt->tm_sec + 1);
        mem[(uint16_t)(p + 1)] = (uint8_t)(lt->tm_min + 1);
        mem[(uint16_t)(p + 2)] = (uint8_t)(lt->tm_hour + 1);
        mem[(uint16_t)(p + 3)] = (uint8_t)(lt->tm_mday);
        mem[(uint16_t)(p + 4)] = (uint8_t)(lt->tm_mon + 1);
        mem[(uint16_t)(p + 5)] = (uint8_t)((lt->tm_year % 100) + 1);
        break; }

    case 0x10:                                   /* spool request: accept it */
        mem[(uint16_t)(p + 2)] = 0;
        break;

    default:
        if (debug) fprintf(stderr, "[os] unknown command %02X\n", cmd);
        break;
    }
    mem[0x0004] = 0;                             /* request accepted */
}

/* ----------------------------------------------------------- bus callbacks */

uint8_t mem_rd(uint16_t a) { return mem[a]; }

void mem_wr(uint16_t a, uint8_t v)
{
    mem[a] = v;
    if (a == 0x0004) {
        if (v) os_request(v);
        return;
    }
    if (a >= SCR_CHAR) {
        int row = (a - SCR_CHAR) >> 8, col = a & 0xFF;
        if (row < SCR_ROWS && col < SCR_COLS) {
            screen_dirty = 1;
            cur_row = row;
            cur_col = (col + 1 < SCR_COLS) ? col + 1 : col;
        }
    }
}

uint8_t io_in(uint16_t port)
{
    switch (port & 0xFF) {
    case 0x00: return (uint8_t)kq_pop();     /* keyboard scan code   */
    case 0x07: return 0x0C;                  /* 928 sub-model, 48K   */
    default:   return 0xFF;
    }
}

void io_out(uint16_t port, uint8_t v)
{
    (void)v;
    switch (port & 0xFF) {
    case 0x00: break;                        /* keyboard acknowledge */
    case 0x01:                               /* halt the workstation */
        if (debug) fprintf(stderr, "[halt] OUT (01) from %04X\n", cpu.pc);
        running = 0;
        break;
    case 0x03: screen_dirty = 1; break;      /* display refresh      */
    default: break;
    }
}

/* --------------------------------------------------------------------- run */

static void load_image(void)
{
    FILE *fp;
    long blocks;
    size_t n;

    fp = open_game_file("DEMO.VENTURE.START", &blocks);
    if (!fp) die("cannot find DEMO.VENTURE.START (looked in .\\data and .)");
    n = fread(mem + IMG_BASE, 1, 0x10000 - IMG_BASE, fp);
    fclose(fp);
    if (n < 0x6000) die("DEMO.VENTURE.START is too short (%lu bytes)", (unsigned long)n);

    /* Communication area, as the OIS supervisor leaves it for a task. */
    mem[0x0004] = 0x00;      /* mailbox idle                                  */
    mem[0x0007] = 0x05;      /* workstation model 5; sub-model comes from I/O */
    mem[0x0011] = 0x04;      /* pages the supervisor keeps at the top for the stack */
}

static void find_exedir(const char *argv0)
{
    char *p;
    strncpy(exedir, argv0 ? argv0 : ".", sizeof exedir - 1);
    exedir[sizeof exedir - 1] = 0;
    p = strrchr(exedir, '\\');
    if (!p) p = strrchr(exedir, '/');
    if (p) *p = 0; else strcpy(exedir, ".");
}

int main(int argc, char **argv)
{
    int i;
    long idle = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) debug = 1;
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) {
            trace_len = atoi(argv[++i]);
            if (trace_len > 0) trace_ring = calloc((size_t)trace_len, sizeof *trace_ring);
        }
    }

    find_exedir(argv[0]);
    load_image();
    build_keymap();
    con_init();

    z80_reset(&cpu);
    cpu.pc = IMG_BASE;
    cpu.sp = STACK_TOP;

    while (running) {
        int n;
        for (n = 0; n < 20000 && running; n++) {
            /* Supervisor traps live in the first page. */
            if (cpu.pc >= 0x0008 && cpu.pc < IMG_BASE) {
                if (debug) fprintf(stderr, "[exit] supervisor trap at %04X\n", cpu.pc);
                running = 0;
                break;
            }
            if (trace_ring) {
                trace_ring[trace_pos] = cpu.pc;
                trace_pos = (trace_pos + 1) % trace_len;
                trace_full += (trace_full < trace_len);
            }
            if (debug) {
                if (cpu.pc == 0x2D24)
                    fprintf(stderr, "[fatal] runtime error, message block at %04X\n",
                            (unsigned)((cpu.b << 8) | cpu.c));
                if (cpu.pc == 0x3D8A)
                    fprintf(stderr, "[fatal] I/O error: fn=%02X stat=%02X x=%02X\n",
                            mem[0x2165], mem[0x2166], mem[0x216E]);
            }
            z80_step(&cpu);
        }
        if (!running) break;

        con_refresh();
        con_poll();

        /* Hand over a key only once the resident buffer is free, which is
           what the real keyboard controller waits for. */
        if (!kq_empty() && mem[KEYBUF] == 0 && cpu.iff1) {
            z80_interrupt(&cpu);
            idle = 0;
            continue;
        }

        if (screen_dirty) { screen_dirty = 0; idle = 0; }
        else if (++idle > 3 && kq_empty()) con_idle();
    }

    con_refresh();
    con_shutdown();
    if (trace_ring && trace_full) {
        int k, start = (trace_pos - trace_full + trace_len) % trace_len;
        fprintf(stderr, "[trace] last %d instructions:\n", trace_full);
        for (k = 0; k < trace_full; k++)
            fprintf(stderr, "%04X%s", trace_ring[(start + k) % trace_len],
                    ((k + 1) % 12) ? " " : "\n");
        fprintf(stderr, "\n");
    }
    for (i = 1; i < MAX_FILES; i++) if (files[i].used) fclose(files[i].fp);
    return 0;
}
