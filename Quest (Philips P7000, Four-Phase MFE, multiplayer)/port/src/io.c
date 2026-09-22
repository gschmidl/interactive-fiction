/* io.c - the interrupt system and the devices of the Four-Phase IV/90:
 *   channel 0  the 60 Hz real-time clock (an INR at location 0)
 *   channel 2  the 8231 cartridge disc, unit 024 (select word 01120 + T);
 *              units 025-027 are drives the controller does not have
 *   channel 3  the 7200 video/keyboard terminals, units 0-037
 *   channel 7  the low-priority interrupt (EXCT with [EA] = 1)
 * From the Peripheral Unit Programming Manual (SIV/70-40-1D, 1973), with
 * what MFE showed about the keyboards. */
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
    word w = vrd(2 * (word)level);
    int unit = 0;

    while (!(req[level] >> unit & 1))
        unit++;
    irq_drop(level, unit);
    if ((w >> 15) == 0577) {                    /* IOID */
        word a = (w & A15 & ~077u) | (word)unit;

        w = vrd(a);
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
unsigned long long clock_ticks;

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

/* the ticks due by now: by the host's clock in real time */
unsigned long long clock_due(void)
{
    return clock_rt ? (host_us() - rt_base) * 60 / 1000000 : clock_ticks;
}

static void clock_tick(void)
{
    if (!clock_rt) {
        if (icount >= next_clock) {
            irq_raise(0, 0);
            clock_ticks++;
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
        clock_ticks++;
        rt_pending--;
    }
}

/* ---- transfers ------------------------------------------------------------ */

/* the buffer address word at BP: store or fetch the next word.  Under IOXW
 * the buffer word names the window of the buffer (bits 1-8). */
int io_cross;

static int buf_win(word b)
{
    return io_cross ? (int)(b >> 15) & 0377 : -1;
}

static void put_word(word bp, word v)
{
    word b = vrd(bp);

    wwr(buf_win(b), b & A15, v);
    vwr(bp, (b & ~A15) | ((b + 1) & A15));
}

static word get_word(word bp)
{
    word b = vrd(bp);

    vwr(bp, (b & ~A15) | ((b + 1) & A15));
    return wrd(buf_win(b), b & A15);
}

/* ---- the 8231 disc --------------------------------------------------------- */

#define DISC_UNIT 024
#define DISC_SECT_WORDS 256
#define DISC_CYLS 0313                  /* 203 cylinders of 16 sectors */
#define SEEK_TIME 2000ULL
#define SECTOR_TIME 300ULL

enum {
    DS_NOTREADY = 1, DS_BUSY = 2, DS_CRC = 4, DS_TOOLATE = 010,
    DS_HEADER = 020, DS_RANGE = 040, DS_SEEKINC = 0100
};

static word *disc;                      /* DISC_CYLS * 16 sectors */
static long disc_len;                   /* sectors in the image file */
static char *disc_path;
static int disc_writable, disc_dirty;
static int disc_cyl, disc_status, disc_op, disc_sect, disc_count, disc_hdr;
static unsigned long long disc_irq_at;
static int disc_bootmode = 1;

int disc_open(const char *path, int writable)
{
    FILE *f = fopen(path, "rb");
    long n, i;
    unsigned char *raw;

    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    raw = malloc((size_t)n);
    if (!raw || fread(raw, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        return -1;
    }
    fclose(f);
    disc = calloc((size_t)DISC_CYLS * 16 * DISC_SECT_WORDS, sizeof(word));
    for (i = 0; i + 2 < n && i / 3 < (long)DISC_CYLS * 16 * DISC_SECT_WORDS; i += 3)
        disc[i / 3] = (word)raw[i] << 16 | (word)raw[i + 1] << 8 | raw[i + 2];
    disc_len = n / (3 * DISC_SECT_WORDS);
    free(raw);
    disc_path = strdup(path);
    disc_writable = writable;
    return 0;
}

void disc_close(void)
{
    FILE *f;
    long i, n = disc_len * DISC_SECT_WORDS;

    if (!disc || !disc_writable || !disc_dirty)
        return;
    f = fopen(disc_path, "wb");
    if (!f)
        return;
    for (i = 0; i < n; i++) {
        fputc((int)(disc[i] >> 16) & 0xFF, f);
        fputc((int)(disc[i] >> 8) & 0xFF, f);
        fputc((int)disc[i] & 0xFF, f);
    }
    fclose(f);
}

int disc_busy(void)
{
    return disc_irq_at != 0 || disc_op != 0;
}

#define DISC_WORDS ((long)DISC_CYLS * 16 * DISC_SECT_WORDS)

word disc_peek(long w)
{
    return disc && w >= 0 && w < DISC_WORDS ? disc[w] : 0;
}

void disc_poke(long w, word v)
{
    if (disc && w >= 0 && w < DISC_WORDS) {
        disc[w] = v & W24;
        disc_dirty = 1;
    }
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
    if (cyl != disc_cyl && trace_fp)
        fprintf(trace_fp, "** disc: transfer on cylinder %o with the heads on %o\n",
                cyl, disc_cyl);
    disc_cyl = cyl;
    disc_op = (w & (1u << 23)) ? 2 : 1;         /* write : read */
    disc_sect = sec;
    disc_count = ((int)(w >> 12) & 017) + 1;   /* bits 8-11 */
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

        if (k >= (long)DISC_CYLS * 16 * DISC_SECT_WORDS)
            break;
        if (input)
            put_word(bp, disc[k]);
        else {
            disc[k] = get_word(bp);
            disc_dirty = 1;
        }
    }
    if (trace_fp)
        fprintf(trace_fp, "** disc: %s cylinder %o sector %o, %d sectors, buffer %05o\n",
                input ? "read" : "write", disc_cyl, disc_sect, disc_count,
                (vrd(bp) - (word)nwords) & A15);
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

/* IDOS keeps one key in one word, so keys come at a typist's pace: one per
 * kbd_gap instructions, the first after kbd_start */
unsigned long long kbd_gap = 100000ULL, kbd_start = 0;

/* A program's keyboard interrupt routine stores each key in a word that its
 * main loop polls ([07733] for $BATCH).  That word is learned from the first
 * STA after a keyboard data-in; a key is typed only while the program polls
 * it and it is empty - or, for a program with its own word, after a long
 * silence (the word is then learned again). */
word kbd_word;
int kbd_learn;
unsigned long long kbd_polled;
#define KBD_SILENCE 3000000ULL

/* is the program waiting for a key now? */
int kbd_waiting(void)
{
    if (!cpu_idle() || disc_irq_at)
        return 0;
    if (!kbd_word)
        return 1;
    return (icount - kbd_polled < 5000 && vrd(kbd_word) == 0) ||
           icount - kbd_polled >= KBD_SILENCE;
}

static unsigned char kbuf[NKBD];
static int kfull[NKBD];
static unsigned char *kq[NKBD];
static int kqlen[NKBD], kqpos[NKBD];
static unsigned long long knext;

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

int kbd_free;                           /* type without waiting for the program */
int kbd_beeps[NKBD];                    /* alarms sounded at each terminal */

static void kbd_tick(void)
{
    int k;

    /* a program idling while a disc operation is under way waits for the
     * disc, not for the keyboard */
    if (icount < knext || icount < kbd_start || (!kbd_free && !kbd_waiting()))
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

/* the keyboard whose key a data-in or a status reports: the unit's own, or
 * else the first with a key.  MFE takes every key with a status and a data-in
 * on unit 0 and finds the keyboard in the status word. */
static int kbd_ready(int unit)
{
    int k;

    if (kfull[unit & 037])
        return unit & 037;
    for (k = 0; k < NKBD; k++)
        if (kfull[k])
            return k;
    return unit & 037;
}

static void kbd_io(int unit, int type, word bp)
{
    int k = kbd_ready(unit);

    switch (type) {
    case 1:                                     /* data in */
        put_word(bp, kbuf[k]);
        kfull[k] = 0;
        irq_drop(3, k);
        kbd_learn = 1;
        break;
    case 3:                                     /* status: a ready keyboard */
        put_word(bp, kfull[k] ? 1u << 7 | (word)k : 0);
        break;
    case 2: {                                   /* control: the audible alarm */
        word v = get_word(bp);

        if (trace_fp)
            fprintf(trace_fp, "** keyboard %o: control %08o at %05o\n", unit, v, last_pc);
        kbd_beeps[unit & 037]++;
        break;
    }
    default: {                                  /* data out: not known; taken and ignored */
        word v = get_word(bp);

        if (trace_fp)
            fprintf(trace_fp, "** keyboard %o: data out %08o at %05o (ignored)\n", unit, v,
                    last_pc);
        break;
    }
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
    word sel = vrd(se);
    int chan = (int)(sel >> 8) & 7, unit = (int)(sel >> 2) & 077, type = (int)sel & 3;

    cc_z = 1;
    if (trace_fp && icount >= trace_from)
        fprintf(trace_fp, "** IO%s%s channel %d unit %02o type %d, buffer %08o\n",
                bytes ? "B" : "", io_cross ? "XW" : "", chan, unit, type, vrd(bp));
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
    if (trace_fp)
        fprintf(trace_fp, "** EXCT %08o at %05o (window %o)\n", v, last_pc, cur_win);
    if ((v & 017) == 1)                         /* EXC3: the level 7 interrupt */
        irq_raise(7, 0);
}

int io_exsn(word v)
{
    (void)v;
    return 0;
}
