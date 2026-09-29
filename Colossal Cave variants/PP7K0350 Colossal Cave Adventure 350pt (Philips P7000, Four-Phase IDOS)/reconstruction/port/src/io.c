/* io.c - the interrupt system and the devices of the Four-Phase IV/70:
 *   channel 0  the 60 Hz real-time clock (an INR at location 0)
 *   channel 2  the 8231 cartridge disc, unit 024 (select word 01120 + T);
 *              units 025-027 are drives the controller does not have
 *   channel 3  the 7200 video/keyboard terminals, units 0-037
 *   channel 7  the low-priority interrupt (EXCT with [EA] = 1)
 * From the Peripheral Unit Programming Manual (SIV/70-40-1D, 1973). */
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#include "fp4.h"

/* ---- the interrupt system ------------------------------------------- */

static int armed[8], active[8];
static uint64_t req[8];

void irq_raise(int level, int unit)
{
    req[level] |= 1ULL << unit;
}

static void irq_drop(int level, int unit)
{
    req[level] &= ~(1ULL << unit);
}

int irq_pending(void)
{
    int l, hi = 8;

    for (l = 0; l < 8; l++)
        if (active[l]) {
            hi = l;
            break;
        }
    for (l = 0; l < hi; l++)
        if (armed[l] && req[l])
            return l;
    return -1;
}

int irq_take(int level, word *ir)
{
    word w = mem[2 * level];
    int unit = 0;

    while (!(req[level] >> unit & 1))
        unit++;
    irq_drop(level, unit);
    if ((w >> 15) == 0577) {                    /* IOID */
        word a = (w & A15 & ~077u) | (word)unit;

        w = mem[a];
    }
    *ir = w;
    return unit;
}

void irq_mark_active(int level)
{
    active[level] = 1;
}

void irq_debreak(void)
{
    int l;

    for (l = 0; l < 8; l++)
        if (active[l]) {
            active[l] = 0;
            return;
        }
}

void irq_arm(word mask, int on)
{
    int l;

    for (l = 0; l < 8; l++)
        if (mask >> l & 1)
            armed[l] = on;
}

void irq_reset_levels(word mask)
{
    int l;

    for (l = 0; l < 8; l++)
        if (mask >> l & 1) {
            req[l] = 0;
            active[l] = 0;
            armed[l] = 1;
        }
}

/* ---- the clock ----------------------------------------------------------- */

#define CLOCK_TICKS 8000ULL             /* instructions per 1/60 second */
static unsigned long long next_clock;

static int clock_rt;                    /* follow the host's clock */
static unsigned long long rt_base, rt_ticks, rt_pending;

static unsigned long long host_us(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;

    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (unsigned long long)(now.QuadPart / freq.QuadPart * 1000000 +
                                now.QuadPart % freq.QuadPart * 1000000 / freq.QuadPart);
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000 + (unsigned long long)ts.tv_nsec / 1000;
#endif
}

void clock_set_realtime(int on)
{
    clock_rt = on;
    rt_base = host_us();
    rt_ticks = rt_pending = 0;
}

static void clock_tick(void)
{
    if (!clock_rt) {
        if (icount >= next_clock) {
            irq_raise(0, 0);
            next_clock += CLOCK_TICKS;
        }
        return;
    }
    /* real time: count the host's 1/60 seconds, and hand them over one at
     * a time (a tick waits while the one before is still pending) */
    if ((icount & 1023) == 0) {
        unsigned long long due = (host_us() - rt_base) * 60 / 1000000;

        if (due > rt_ticks) {
            rt_pending += due - rt_ticks;
            rt_ticks = due;
        }
    }
    if (rt_pending && !(req[0] & 1)) {
        irq_raise(0, 0);
        rt_pending--;
    }
}

/* ---- transfers ------------------------------------------------------------ */

/* the buffer address word at BP: store or fetch the next word */
static void put_word(word bp, word v)
{
    word b = mem[bp];

    mem[b & A15] = v & W24;
    mem[bp] = (b + 1) & W24;
}

static word get_word(word bp)
{
    word b = mem[bp];

    mem[bp] = (b + 1) & W24;
    return mem[b & A15];
}

/* ---- the 8231 disc --------------------------------------------------------- */

#define DISC_UNIT 024
#define DISC_SECT_WORDS 256
#define DISC_CYLS 0313                  /* 203 cylinders of 16 sectors */
#define DISC_WORDS ((long)DISC_CYLS * 16 * DISC_SECT_WORDS)
#define SEEK_TIME 2000ULL
#define SECTOR_TIME 300ULL

enum {
    DS_NOTREADY = 1, DS_BUSY = 2, DS_CRC = 4, DS_TOOLATE = 010,
    DS_HEADER = 020, DS_RANGE = 040, DS_SEEKINC = 0100
};

static word *disc;                      /* the whole pack, in memory */
static long disc_len;                   /* sectors in the image file */
static FILE *disc_fp;                   /* writes go through to it */
static int disc_cyl, disc_status, disc_op, disc_sect, disc_count, disc_hdr;
static unsigned long long disc_irq_at;
static int disc_bootmode = 1;

int disc_open(const char *path, int writable)
{
    FILE *f = fopen(path, writable ? "r+b" : "rb");
    long n, i;
    unsigned char *raw;

    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    raw = malloc((size_t)n + 1);
    if (!raw || fread(raw, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(raw);
        return -1;
    }
    disc = calloc((size_t)DISC_WORDS, sizeof(word));
    for (i = 0; i + 2 < n && i / 3 < DISC_WORDS; i += 3)
        disc[i / 3] = (word)raw[i] << 16 | (word)raw[i + 1] << 8 | raw[i + 2];
    disc_len = n / (3 * DISC_SECT_WORDS);
    free(raw);
    if (writable)
        disc_fp = f;
    else
        fclose(f);
    return 0;
}

void disc_close(void)
{
    if (disc_fp) {
        fclose(disc_fp);
        disc_fp = NULL;
    }
}

int disc_busy(void)
{
    return disc_irq_at != 0 || disc_op != 0;
}

word disc_peek(long w)
{
    return disc && w >= 0 && w < DISC_WORDS ? disc[w] : 0;
}

/* write sectors FIRST.. to the image file */
static void disc_flush(long first, long count)
{
    long i;

    if (!disc_fp || first >= disc_len)
        return;
    if (first + count > disc_len)
        count = disc_len - first;
    fseek(disc_fp, first * 3 * DISC_SECT_WORDS, SEEK_SET);
    for (i = first * DISC_SECT_WORDS; i < (first + count) * DISC_SECT_WORDS; i++) {
        fputc((int)(disc[i] >> 16) & 0xFF, disc_fp);
        fputc((int)(disc[i] >> 8) & 0xFF, disc_fp);
        fputc((int)disc[i] & 0xFF, disc_fp);
    }
    fflush(disc_fp);
}

void disc_copy(long from, long to, long count)
{
    if (!disc || from < 0 || to < 0 || count <= 0 ||
        (from + count) * DISC_SECT_WORDS > DISC_WORDS || (to + count) * DISC_SECT_WORDS > DISC_WORDS)
        return;
    memmove(disc + to * DISC_SECT_WORDS, disc + from * DISC_SECT_WORDS,
            (size_t)(count * DISC_SECT_WORDS) * sizeof(word));
    disc_flush(to, count);
}

static void disc_control(word w)
{
    int cyl = (int)(w >> 4) & 0377, sec = (int)w & 017;

    disc_bootmode = 0;
    if (w & (1u << 20)) {                       /* restore */
        disc_cyl = 0;
        disc_status &= ~(DS_SEEKINC | DS_RANGE);
        disc_op = 0;
        disc_irq_at = icount + SEEK_TIME;
        return;
    }
    if (w & (1u << 22)) {                       /* seek */
        disc_op = 0;
        if (cyl >= DISC_CYLS) {
            disc_status |= DS_RANGE;
            disc_irq_at = icount + 1;
            return;
        }
        disc_irq_at = icount + (cyl == disc_cyl ? 1 : SEEK_TIME);
        disc_cyl = cyl;
        return;
    }
    disc_cyl = cyl;
    disc_op = (w & (1u << 23)) ? 2 : 1;         /* write : read */
    disc_sect = sec;
    disc_count = ((int)(w >> 12) & 017) + 1;    /* bits 8-11 */
    disc_hdr = (w >> 19) & 1;
    disc_irq_at = icount + SECTOR_TIME;
}

static void disc_transfer(word bp, int input)
{
    long s = (long)disc_cyl * 16 + disc_sect, i;
    long nwords = (long)disc_count * DISC_SECT_WORDS;

    if (!disc_op || (disc_op == 1) != input) {
        stop(STOP_IO, "disc data %s with no %s set up at %05o", input ? "in" : "out",
             input ? "read" : "write", last_pc);
        return;
    }
    if (disc_hdr) {                             /* the header word only */
        if (input)
            put_word(bp, 040000000u | (word)disc_cyl << 4 | (word)disc_sect);
        else
            (void)get_word(bp);
        disc_op = 0;
        return;
    }
    for (i = 0; i < nwords; i++) {
        long k = s * DISC_SECT_WORDS + i;

        if (k >= DISC_WORDS)
            break;
        if (input)
            put_word(bp, disc[k]);
        else
            disc[k] = get_word(bp);
    }
    if (!input)
        disc_flush(s, disc_count);
    if (trace_fp)
        fprintf(trace_fp, "** %llu disc: %s cylinder %o sector %o, %d sectors, buffer %05o\n",
                icount, input ? "read" : "write", disc_cyl, disc_sect, disc_count,
                (mem[bp] - (word)nwords) & A15);
    disc_op = 0;
}

static void disc_io(int type, word bp)
{
    switch (type) {
    case 0: disc_transfer(bp, 0); break;
    case 1: disc_transfer(bp, 1); break;
    case 2: disc_control(get_word(bp)); break;
    case 3:
        put_word(bp, (word)disc_status);
        disc_status &= ~(DS_CRC | DS_TOOLATE | DS_HEADER);
        disc_bootmode = 0;
        break;
    }
}

/* ---- the keyboards ----------------------------------------------------------- */

#define NKBD 040

/* keys come at a typist's pace: one per kbd_gap instructions at most, the
 * first after kbd_start */
unsigned long long kbd_gap = 20000ULL, kbd_start = 0;

/* An IDOS program keeps one key in one word: its keyboard interrupt routine
 * stores the key there and the main loop polls it ([07733] in $BATCH).  The
 * word is learned from the first STA after a keyboard data-in, and a key is
 * typed only while the program polls it and it is empty - or, for a program
 * with a word of its own, after a long silence (the word is then learned
 * again).  A key typed at any other time would be lost, as on the machine. */
word kbd_word;
int kbd_learn;
unsigned long long kbd_polled;
#define KBD_SILENCE 3000000ULL

static unsigned char kbuf[NKBD];
static int kfull[NKBD];
static unsigned char *kq[NKBD];
static int kqlen[NKBD], kqpos[NKBD];
static unsigned long long knext;

int kbd_waiting(void)
{
    if (!cpu_idle() || disc_busy())
        return 0;
    if (!kbd_word)
        return 1;
    return (icount - kbd_polled < 5000 && mem[kbd_word] == 0) ||
           icount - kbd_polled >= KBD_SILENCE;
}

void kbd_type(int kb, const char *s, int n)
{
    if (kqpos[kb] == kqlen[kb])
        kqpos[kb] = kqlen[kb] = 0;
    kq[kb] = realloc(kq[kb], (size_t)(kqlen[kb] + n));
    memcpy(kq[kb] + kqlen[kb], s, (size_t)n);
    kqlen[kb] += n;
}

int kbd_pending(int kb)
{
    return kqlen[kb] - kqpos[kb] + kfull[kb];
}

static void kbd_tick(void)
{
    int k;

    if (icount < knext || icount < kbd_start || !kbd_waiting())
        return;
    for (k = 0; k < NKBD; k++)
        if (!kfull[k] && kqpos[k] < kqlen[k]) {
            kbuf[k] = kq[k][kqpos[k]++];
            kfull[k] = 1;
            irq_raise(3, k);
            knext = icount + kbd_gap;
            if (trace_fp)
                fprintf(trace_fp, "** key %03o at keyboard %d after %llu\n", kbuf[k], k, icount);
            return;
        }
}

static void kbd_io(int unit, int type, word bp)
{
    int k = unit & 037;

    switch (type) {
    case 1:                                     /* data in */
        put_word(bp, kbuf[k]);
        kfull[k] = 0;
        irq_drop(3, k);
        kbd_learn = 1;
        break;
    case 3: {                                   /* status: the first ready keyboard */
        word v = 0;
        int i;

        for (i = 0; i < NKBD; i++)
            if (kfull[i]) {
                v = 1u << 7 | (word)i;
                break;
            }
        put_word(bp, v);
        break;
    }
    case 2:                                     /* control: the audible alarm */
        (void)get_word(bp);
        break;
    default:
        stop(STOP_IO, "keyboard %o: data out at %05o", unit, last_pc);
        break;
    }
}

/* ---- dispatch ---------------------------------------------------------------- */

void io_reset(void)
{
    memset(armed, 0, sizeof armed);
    memset(active, 0, sizeof active);
    memset(req, 0, sizeof req);
    next_clock = icount + CLOCK_TICKS;
    disc_status = 0;
    disc_op = 0;
    disc_irq_at = 0;
    disc_bootmode = 1;
}

void io_tick(void)
{
    clock_tick();
    if (disc_irq_at && icount >= disc_irq_at) {
        disc_irq_at = 0;
        irq_raise(2, DISC_UNIT);
    }
    kbd_tick();
}

void io_exec(word ea, int bytes)
{
    word se = ea & ~1u & A15, bp = se | 1;
    word sel = mem[se];
    int chan = (int)(sel >> 8) & 7, unit = (int)(sel >> 2) & 077, type = (int)sel & 3;

    cc_z = 1;
    if (trace_fp && icount >= trace_from)
        fprintf(trace_fp, "** IO%s channel %d unit %02o type %d, buffer %08o\n",
                bytes ? "B" : "", chan, unit, type, mem[bp]);
    if (chan == 2 && unit == DISC_UNIT) {
        disc_io(type, bp);
        return;
    }
    if (chan == 2 && unit > DISC_UNIT && unit <= DISC_UNIT + 3) {
        /* drives 1-3 of the same controller: not there, so not ready */
        if (type == 3)
            put_word(bp, DS_NOTREADY);
        else if (type == 2)
            (void)get_word(bp);
        else
            stop(STOP_IO, "disc drive %o: data transfer to a drive that is not there",
                 unit - DISC_UNIT);
        return;
    }
    if (chan == 3) {
        kbd_io(unit, type, bp);
        return;
    }
    stop(STOP_IO, "IO%s to channel %d unit %02o (type %d) at %05o: no such device",
         bytes ? "B" : "", chan, unit, type, last_pc);
}

void io_boot(word select)
{
    int chan = (int)(select >> 8) & 7, unit = (int)(select >> 2) & 077, i;

    if (chan == 2 && unit == DISC_UNIT && disc_bootmode) {
        word t;

        for (i = 0; i < DISC_SECT_WORDS; i++)
            mem[1 + i] = disc[i];
        disc_bootmode = 0;
        /* RP counted the words from 1; it and location 1 are swapped */
        t = mem[1];
        mem[1] = 1 + DISC_SECT_WORDS;
        RP = t & A15;
        return;
    }
    stop(STOP_IO, "BOOT from channel %d unit %02o at %05o", chan, unit, last_pc);
}

void io_exct(word v)
{
    if ((v & 017) == 1)                         /* EXC3: the level 7 interrupt */
        irq_raise(7, 0);
}

int io_exsn(word v)
{
    (void)v;
    return 0;
}
