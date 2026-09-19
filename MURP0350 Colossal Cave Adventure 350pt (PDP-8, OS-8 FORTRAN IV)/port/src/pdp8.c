/* Colossal Cave Adventure (350 points), PDP-8 / OS-8 FORTRAN IV version.
 *
 * Dick Murphy's recoding of Bob Supnik's RT-11 Adventure into RALF, run here
 * as the original OS/8 pack: this program is a PDP-8/E with a console
 * teleprinter and one RK05 drive, and everything above that line -- the OS/8
 * monitor, the FRTS FORTRAN run-time system with its software FPP-8
 * interpreter, and ADVENT.LD itself -- is the 1978 code, untouched.
 *
 * The machine is deliberately a bare one: the reference SIMH configuration
 * was stripped device by device until only the CPU, the KL8E console and the
 * RK8E disk remained, and the game still played identically, so there is no
 * EAE, no line clock and no second terminal here either.  Adding an EAE
 * would not be more faithful; FRTS probes for one and would then take a
 * different path through its floating-point interpreter than the reference.
 *
 * Build: see build.sh (or build.bat).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "rk05image.h"

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define isatty_fd(f) _isatty(f)
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#define isatty_fd(f) isatty(f)
#endif

typedef unsigned short u16;

/* ------------------------------------------------------------------ */
/* Machine state                                                      */
/* ------------------------------------------------------------------ */

#define MEMWORDS 32768                  /* 8 fields of 4K, the full KM8E */
static u16 M[MEMWORDS];

static int AC, LINK, PC, MQ;
static int IFR, DFR, IBR, SFR;          /* instruction/data/buffer/save field */
static int int_ena, ion_delay, int_inhibit;
static int halted;
static int console_tick = 1000;

/* Console teleprinter (KL8E, devices 03 and 04).  SIMH's default KSR mode is
 * mirrored here: input is folded to upper case and delivered with mark
 * parity (bit 7 set), which is what OS/8 expects from an ASR-33. */
static int tti_flag, tti_buf, tto_flag, tt_ie = 1;

/* RK8E controller and RK05 pack (device 74). */
#define RK_NBLK   6496                  /* 203 cylinders x 2 surfaces x 16 */
#define RK_BLKW   256                   /* words per block */
static u16 *disk;
static unsigned char *dirty;            /* per-block, for the save file */
static int rk_cmd, rk_da, rk_ca, rk_sta, rk_blk;
static int rk_done, rk_err, rk_delay;

#define RKS_NRDY  00400
#define RKS_TMO   00100

/* ------------------------------------------------------------------ */
/* Console plumbing                                                   */
/* ------------------------------------------------------------------ */

static void hold_console(void);

static int kbq[256], kbq_head, kbq_tail;
static int stdin_tty, stdin_eof;
static int banner_seen;                 /* the game has printed its first line */
static int frts_started;                /* already typed the startup command */
static int monitor_prompt;              /* a bare "." sits on the output line */
static char outline[256];
static int outlen;
static FILE *transcript;

/* Everything OS/8 prints before the game's first line is machine noise --
 * the monitor prompt, the echo of the command typed for the player, the FRTS
 * "*" prompts.  Hold it back and start showing output at the banner, so what
 * the player sees is a game and not a boot. */
#define BANNER "welcome to adventure"
static char preroll[4096];
static int prerolllen;

/* A character only sticks once the game has a read pending.  OS/8's console
 * interrupt handler reads the receiver on every interrupt to clear the flag,
 * and until somebody is reading it has nowhere to put what it read; the
 * reference machine loses fast type-ahead in exactly the same way.  Once the
 * read is open the handler buffers happily -- a whole session's worth of
 * commands can be typed ahead and every one of them lands.
 *
 * Printing "> " is the game asking for a line, but the read is not open the
 * instant the prompt appears, so the first character of each line waits for a
 * gap in the machine's own output.  PROMPT_TICKS is that gap, in units of
 * 1000 instructions: comfortably longer than the longest pause measured
 * inside a message (about 1400), short enough to be invisible.  Once one
 * character has gone in, the rest of the line follows at queue speed. */
#define PROMPT_TICKS 4000
static int prompt_pending = 1;          /* first character of a line is due */
static long quiet_ticks;                /* ticks since the machine last printed */
static long idle_ticks;                 /* ticks with nothing at all to do */

static int raw_console;                 /* -r: show the OS/8 boot dialogue */
static int con_trace;                   /* -C: trace console traffic */
static long trace_n;                    /* -D N: trace N instructions */

/* strstr, ignoring case: the banner is mixed case and the search text is
 * whatever the game happened to print. */
static char *find_ci(const char *hay, const char *needle)
{
    size_t nl = strlen(needle);
    const char *p;
    for (p = hay; *p; p++) {
        size_t i;
        for (i = 0; i < nl; i++) {
            int a = p[i], b = needle[i];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (a != b) break;
        }
        if (i == nl) return (char *)p;
    }
    return NULL;
}

static void kbq_put(int c)
{
    int n = (kbq_tail + 1) & 255;
    if (n == kbq_head) return;          /* full: drop, as a real UART would */
    kbq[kbq_tail] = c;
    kbq_tail = n;
}

static int kbq_empty(void) { return kbq_head == kbq_tail; }

static int kbq_get(void)
{
    int c = kbq[kbq_head];
    kbq_head = (kbq_head + 1) & 255;
    return c;
}

static void type_string(const char *s)
{
    for (; *s; s++) kbq_put((unsigned char)*s);
}

static void out_char(int c)
{
    putchar(c);
    if (transcript) fputc(c, transcript);
}

static void flush_preroll(int from)
{
    int i;
    for (i = from; i < prerolllen; i++) out_char(preroll[i]);
    prerolllen = 0;
    fflush(stdout);
}

static void tto_out(int c)
{
    c &= 0177;                          /* strip the mark parity bit */
    if (con_trace) fprintf(stderr, "[out %03o]", c);
    if (!c || c == 0177) return;

    if (!banner_seen) {
        char *hit;
        if (prerolllen < (int)sizeof(preroll) - 1) preroll[prerolllen++] = (char)c;
        preroll[prerolllen] = 0;
        hit = find_ci(preroll, BANNER);
        if (hit) {
            banner_seen = 1;
            /* The console changes hands here: whatever OS/8 and FRTS were
             * willing to accept, the game has not opened its read yet. */
            prompt_pending = 1;
            if (raw_console) prerolllen = 0;
            else flush_preroll((int)(hit - preroll));
        }
        if (raw_console) out_char(c);
    } else if (raw_console || !(c == '.' && outlen == 0)) {
        out_char(c);
        if (c == '\n') fflush(stdout);
    }

    idle_ticks = 0;
    quiet_ticks = 0;

    /* Track the line being printed so the OS/8 "." prompt can be spotted. */
    if (c == '\r' || c == '\n') {
        outlen = 0;
    } else if (outlen < (int)sizeof(outline) - 1) {
        outline[outlen++] = (char)c;
        outline[outlen] = 0;
    }
    monitor_prompt = (outlen == 1 && outline[0] == '.');
    if (outlen >= 2 && outline[outlen - 2] == '>' && outline[outlen - 1] == ' ')
        prompt_pending = 1;
}

/* ------------------------------------------------------------------ */
/* Host keyboard                                                      */
/* ------------------------------------------------------------------ */

#ifndef _WIN32
static struct termios saved_tio;
static int tio_saved;

static void tty_restore(void)
{
    if (tio_saved) tcsetattr(0, TCSANOW, &saved_tio);
}

static void tty_raw(void)
{
    struct termios t;
    if (!stdin_tty) return;
    tcgetattr(0, &saved_tio);
    tio_saved = 1;
    atexit(tty_restore);
    t = saved_tio;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
}
#endif

static int host_getchar(void)
{
#ifdef _WIN32
    if (stdin_tty) {
        int c;
        if (!_kbhit()) return -1;
        c = _getch();
        if (c == 0 || c == 0xE0) { _getch(); return -1; }   /* function key */
        return c;
    } else {
        int c = getchar();
        if (c == EOF) { stdin_eof = 1; return -1; }
        return c;
    }
#else
    unsigned char b;
    int n = (int)read(0, &b, 1);
    if (n <= 0) {
        if (!stdin_tty) stdin_eof = 1;
        return -1;
    }
    return b;
#endif
}

/* Fold to the character set an ASR-33 could actually send. */
static int console_map(int c)
{
    if (c == 3) { printf("\n"); fflush(stdout); exit(0); }   /* ^C */
    if (c == '\n') c = '\r';
    if (c == 8) c = 0177;                                    /* BS -> RUBOUT */
    if (c >= 'a' && c <= 'z') c -= 32;
    return c;
}

static void poll_keyboard(void)
{
    int c;
    while (!stdin_eof && (c = host_getchar()) >= 0) {
        kbq_put(console_map(c));
        if (!stdin_tty) break;          /* piped: one char per poll is plenty */
    }
}

/* ------------------------------------------------------------------ */
/* RK8E                                                               */
/* ------------------------------------------------------------------ */

static void rk_start(void)
{
    int cyhi = rk_cmd & 01;
    int sect = rk_da & 017;
    int surf = (rk_da >> 4) & 01;
    int cyl  = ((rk_da >> 5) & 0177) | (cyhi << 7);

    /* An RK05 sector is one 256-word OS/8 block, and the image holds those
     * blocks end to end in the order the heads see them. */
    rk_blk = (cyl * 2 + surf) * 16 + sect;
    rk_done = 0;
    rk_err = 0;
    /* Not instantaneous, on purpose.  The bootstrap parks the CPU in a
     * jump-to-self that the arriving block 0 overwrites; completing the DMA
     * inside the DLAG would step past that loop into data. */
    rk_delay = 200;
}

static void rk_complete(void)
{
    int func = (rk_cmd >> 9) & 07;
    int mex  = (rk_cmd >> 3) & 07;
    int drv  = (rk_cmd >> 1) & 03;
    int nw   = (rk_cmd & 0100) ? RK_BLKW / 2 : RK_BLKW;   /* half-block bit */
    int i;

    rk_done = 1;
    if (drv != 0 || rk_blk < 0 || rk_blk >= RK_NBLK) {
        rk_sta |= RKS_NRDY | RKS_TMO;
        rk_err = 1;
        return;
    }
    switch (func) {
    case 0:                                             /* read data */
    case 1:                                             /* read all */
        for (i = 0; i < nw; i++)
            M[(mex << 12) | ((rk_ca + i) & 07777)] = disk[rk_blk * RK_BLKW + i];
        rk_ca = (rk_ca + nw) & 07777;
        break;
    case 4:                                             /* write data */
    case 5:                                             /* write all */
        for (i = 0; i < nw; i++)
            disk[rk_blk * RK_BLKW + i] = M[(mex << 12) | ((rk_ca + i) & 07777)];
        dirty[rk_blk] = 1;
        rk_ca = (rk_ca + nw) & 07777;
        break;
    default:                                            /* seek, write-lock */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Interrupts                                                         */
/* ------------------------------------------------------------------ */

static int int_req(void)
{
    if (tt_ie && (tti_flag || tto_flag)) return 1;
    if ((rk_cmd & 0400) && (rk_done || rk_err)) return 1;
    return 0;
}

static void clear_all_flags(void)
{
    tti_flag = 0;
    tto_flag = 0;
    tt_ie = 1;
    rk_done = 0;
    rk_err = 0;
    rk_sta = 0;
    rk_cmd = 0;
    rk_delay = 0;
}

/* ------------------------------------------------------------------ */
/* Waiting for the player                                             */
/* ------------------------------------------------------------------ */

/* Deliver a character to the KL8E if one is waiting, exactly as the receiver
 * would: the flag sets when the character arrives, not when the program asks
 * for it.  Called both from the instruction loop and from KSF, because OS/8
 * polls the flag but FRTS waits inside its own idle routine. */
static void service_console(void)
{
    if (tti_flag) return;

    if (kbq_empty()) {
        /* The OS/8 keyboard monitor is asking for a command.  The first time
         * that is the start of the session, and the command has to be queued
         * before any of the player's own typing is read, or a piped first
         * line would land at the monitor prompt.  The second time, the game
         * has exited back through FRTS and there is nothing left to do. */
        if (monitor_prompt && !frts_started) {
            frts_started = 1;
            monitor_prompt = 0;
            type_string("R FRTS\rADVENT\033");
        } else if (monitor_prompt) {
            if (banner_seen) out_char('\n');
            fflush(stdout);
            hold_console();
            exit(0);
        } else if (banner_seen) {
            /* Not one keystroke before the game has spoken.  OS/8 hands the
             * console to a starting program by draining whatever is already
             * in the buffer, so anything read off stdin during the boot -- or
             * during FRTS loading the game -- would simply be thrown away. */
            poll_keyboard();
        }
    }

    if (!kbq_empty() && (!prompt_pending || quiet_ticks >= PROMPT_TICKS)) {
        int c = kbq_get() & 0177;
        tti_buf = c | 0200;
        tti_flag = 1;
        /* A carriage return ends the line and the read with it, so the next
         * line has to wait for the next prompt all over again. */
        prompt_pending = (c == 015);
        if (con_trace) fprintf(stderr, "[in %03o]", c);
        idle_ticks = 0;
        return;
    }

    if (++idle_ticks > 40) {
        if (stdin_tty && kbq_empty()) {
            /* FRTS spins a background counter while it waits, and that counter
             * is the game's random seed, so the CPU never really blocks.  Give
             * the host a rest anyway; the seed still follows the player's own
             * timing, which is what it was there for. */
#ifdef _WIN32
            Sleep(1);
#else
            struct timeval tv;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            select(1, &fds, NULL, NULL, &tv);
#endif
            idle_ticks = 20;
        } else if (stdin_eof && idle_ticks > 100000) {
            /* Piped input ran out without a QUIT.  Nothing more can happen. */
            fflush(stdout);
            exit(0);
        }
    }
}

/* ------------------------------------------------------------------ */
/* IOT                                                                */
/* ------------------------------------------------------------------ */

static void skip(void) { PC = (PC + 1) & 07777; }

static void iot(int ir)
{
    int dev = (ir >> 3) & 077;
    int f = ir & 07;

    if (dev >= 020 && dev <= 027) {             /* KM8E memory extension */
        int fld = dev - 020;
        switch (f) {
        case 1: DFR = fld; break;                                /* CDF */
        case 2: IBR = fld; int_inhibit = 1; break;               /* CIF */
        case 3: DFR = fld; IBR = fld; int_inhibit = 1; break;    /* CDF CIF */
        case 4:
            switch (fld) {
            case 1: AC |= DFR << 3; break;                       /* RDF */
            case 2: AC |= IFR << 3; break;                       /* RIF */
            case 3: AC |= SFR; break;                            /* RIB */
            case 4: IBR = (SFR >> 3) & 7; DFR = SFR & 7;         /* RMF */
                    int_inhibit = 1; break;
            default: break;
            }
            break;
        default: break;
        }
        return;
    }

    switch (dev) {
    case 000:                                   /* processor / interrupt */
        switch (f) {
        case 0: if (int_ena) skip(); int_ena = 0; ion_delay = 0; break; /* SKON */
        case 1: ion_delay = 2; break;                                   /* ION */
        case 2: int_ena = 0; ion_delay = 0; break;                      /* IOF */
        case 3: if (int_req()) skip(); break;                           /* SRQ */
        case 4: AC = (LINK << 11) | (int_req() ? 0400 : 0)              /* GTF */
                     | (int_ena ? 0200 : 0) | SFR; break;
        case 5: LINK = (AC >> 11) & 1; IBR = (AC >> 3) & 7;             /* RTF */
                DFR = AC & 7; ion_delay = 2; int_inhibit = 1; AC = 0; break;
        case 6: break;                                                  /* SGT */
        case 7: AC = 0; LINK = 0; int_ena = 0; ion_delay = 0;            /* CAF */
                if (con_trace) fprintf(stderr, "[caf]");
                clear_all_flags(); break;
        }
        break;

    case 003:                                   /* keyboard */
        switch (f) {
        case 0: tti_flag = 0;                                     /* KCF */
                if (con_trace) fprintf(stderr, "[kcf]");
                break;
        case 1: service_console();                               /* KSF */
                if (tti_flag) skip();
                break;
        case 2: AC = 0; tti_flag = 0;                             /* KCC */
                if (con_trace) fprintf(stderr, "[kcc]");
                break;
        case 4: AC |= tti_buf;                                    /* KRS */
                if (con_trace) fprintf(stderr, "[krs]");
                break;
        case 5: tt_ie = AC & 1; break;                           /* KIE */
        case 6: AC = tti_buf; tti_flag = 0;                       /* KRB */
                if (con_trace) fprintf(stderr, "[krb]");
                break;
        default: break;
        }
        break;

    case 004:                                   /* teleprinter */
        switch (f) {
        case 0: tto_flag = 1; break;                             /* TFL */
        case 1: if (tto_flag) skip(); break;                     /* TSF */
        case 2: tto_flag = 0; break;                             /* TCF */
        case 4: tto_out(AC); tto_flag = 1; break;                /* TPC */
        case 5: if (tti_flag || tto_flag) skip(); break;          /* TSK */
        case 6: tto_out(AC); tto_flag = 1; break;                /* TLS */
        default: break;
        }
        break;

    case 074:                                   /* RK8E disk control */
        switch (f) {
        case 1: if (rk_done || rk_err) skip(); break;            /* DSKP */
        case 2:                                                  /* DCLR */
            switch (AC & 03) {
            case 0: rk_done = 0; rk_err = 0; rk_sta = 0; break;
            case 1: rk_done = 0; rk_err = 0; break;
            case 2: rk_done = 1; break;                          /* drive reset */
            default: break;
            }
            AC = 0;
            break;
        case 3: rk_da = AC; AC = 0; rk_start(); break;           /* DLAG */
        case 4: rk_ca = AC; AC = 0; break;                       /* DLCA */
        case 5: AC = rk_sta; break;                              /* DRST */
        case 6: rk_cmd = AC; AC = 0;                             /* DLDC */
                rk_done = 0; rk_err = 0; rk_sta = 0; break;
        default: break;                                          /* DMAN */
        }
        break;

    default:
        /* Any other device code is an option this machine does not have: no
         * skip, AC untouched, exactly as an empty Omnibus slot behaves. */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Operate group                                                      */
/* ------------------------------------------------------------------ */

static void opr(int ir)
{
    if (!(ir & 0400)) {                         /* group 1 */
        if (ir & 0200) AC = 0;
        if (ir & 0100) LINK = 0;
        if (ir & 0040) AC ^= 07777;
        if (ir & 0020) LINK ^= 1;
        if (ir & 0001) {
            AC = (AC + 1) & 07777;
            if (AC == 0) LINK ^= 1;
        }
        switch (ir & 0016) {
        case 0002:                              /* BSW */
            AC = ((AC >> 6) | (AC << 6)) & 07777;
            break;
        case 0010:                              /* RAR */
        case 0012: {                            /* RTR */
            int n = (ir & 0002) ? 2 : 1, i;
            for (i = 0; i < n; i++) {
                int l = AC & 1;
                AC = (AC >> 1) | (LINK << 11);
                LINK = l;
            }
            break;
        }
        case 0004:                              /* RAL */
        case 0006: {                            /* RTL */
            int n = (ir & 0002) ? 2 : 1, i;
            for (i = 0; i < n; i++) {
                int l = (AC >> 11) & 1;
                AC = ((AC << 1) | LINK) & 07777;
                LINK = l;
            }
            break;
        }
        default:
            break;
        }
        return;
    }

    if (!(ir & 0001)) {                         /* group 2 */
        int sk;
        if (ir & 0010) {                        /* AND group; none set => SKP */
            sk = 1;
            if ((ir & 0100) && (AC & 04000)) sk = 0;   /* SPA */
            if ((ir & 0040) && AC == 0) sk = 0;        /* SNA */
            if ((ir & 0020) && LINK) sk = 0;           /* SZL */
        } else {                                /* OR group */
            sk = 0;
            if ((ir & 0100) && (AC & 04000)) sk = 1;
            if ((ir & 0040) && AC == 0) sk = 1;
            if ((ir & 0020) && LINK) sk = 1;
        }
        if (sk) skip();
        if (ir & 0200) AC = 0;
        /* OSR: no switch register on this machine, so it contributes zero. */
        if (ir & 0002) halted = 1;              /* HLT */
        return;
    }

    /* Group 3.  With no KE8-E in the machine the arithmetic opcodes do
     * nothing, but MQ, MQA and MQL are part of the basic 8/E and stay. */
    if (ir & 0200) AC = 0;
    if (ir & 0100) AC |= MQ;                    /* MQA */
    /* SCA would OR in the step counter, which without an EAE is zero. */
    if (ir & 0020) { MQ = AC; AC = 0; }         /* MQL */
}

/* ------------------------------------------------------------------ */
/* Instruction loop                                                   */
/* ------------------------------------------------------------------ */

static void run(void)
{
    while (!halted) {
        int ir, op, a, ea, eaf, addr, t, here;

        if (rk_delay && --rk_delay == 0) rk_complete();
        if (ion_delay && --ion_delay == 0) int_ena = 1;
        if (--console_tick <= 0) {
            console_tick = 1000;
            quiet_ticks++;
            service_console();
        }

        if (int_ena && !int_inhibit && int_req()) {
            SFR = (IFR << 3) | DFR;
            M[0] = (u16)PC;
            IFR = IBR = 0;
            DFR = 0;
            PC = 1;
            int_ena = 0;
            ion_delay = 0;
        }

        here = PC;
        ir = M[(IFR << 12) | PC];
        if (trace_n > 0) {
            fprintf(stderr, "%o:%04o %04o ac=%04o l=%d df=%o ib=%o\n",
                    IFR, here, ir, AC, LINK, DFR, IBR);
            trace_n--;
        }
        PC = (PC + 1) & 07777;
        op = ir >> 9;

        if (op < 6) {
            a = ir & 0177;
            if (ir & 0200) a |= here & 07600;   /* the page the word is on */
            if (ir & 0400) {
                int pa = (IFR << 12) | a;
                if (a >= 010 && a <= 017)       /* auto-index, in the I field */
                    M[pa] = (u16)((M[pa] + 1) & 07777);
                ea = M[pa];
                eaf = (op >= 4) ? IBR : DFR;
            } else {
                ea = a;
                eaf = (op >= 4) ? IBR : IFR;
            }
            addr = (eaf << 12) | ea;

            switch (op) {
            case 0: AC &= M[addr]; break;                       /* AND */
            case 1: t = AC + M[addr];                           /* TAD */
                    if (t & 010000) LINK ^= 1;
                    AC = t & 07777; break;
            case 2: t = (M[addr] + 1) & 07777;                  /* ISZ */
                    M[addr] = (u16)t;
                    if (!t) skip();
                    break;
            case 3: M[addr] = (u16)AC; AC = 0; break;           /* DCA */
            case 4: M[addr] = (u16)PC; PC = (ea + 1) & 07777;   /* JMS */
                    IFR = IBR; int_inhibit = 0; break;
            case 5: PC = ea; IFR = IBR; int_inhibit = 0; break; /* JMP */
            }
        } else if (op == 6) {
            iot(ir);
        } else {
            opr(ir);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Pack image and the saved-game overlay                              */
/* ------------------------------------------------------------------ */

static char savepath[1024];

static void load_disk(void)
{
    uLongf n = RK05_RAW_BYTES;
    unsigned char *raw = (unsigned char *)malloc(RK05_RAW_BYTES);
    long i;

    disk = (u16 *)malloc((size_t)RK_NBLK * RK_BLKW * sizeof(u16));
    dirty = (unsigned char *)calloc(RK_NBLK, 1);
    if (!raw || !disk || !dirty) { fprintf(stderr, "out of memory\n"); exit(1); }

    if (uncompress(raw, &n, rk05_z, RK05_Z_BYTES) != Z_OK || n != RK05_RAW_BYTES) {
        fprintf(stderr, "corrupt embedded disk image\n");
        exit(1);
    }
    for (i = 0; i < (long)RK_NBLK * RK_BLKW; i++)
        disk[i] = (u16)((raw[i * 2] | (raw[i * 2 + 1] << 8)) & 07777);
    free(raw);
}

/* SAVE writes ASAVE.DA into the OS/8 file system, so what changes is the pack
 * itself.  Rather than keep a 3 MB copy of it, remember just the blocks the
 * game wrote and replay them over the embedded image next time. */
#define SAVE_MAGIC 0x38564441UL         /* "ADV8" */

static void load_overlay(void)
{
    FILE *f = fopen(savepath, "rb");
    unsigned long magic, count, i;
    if (!f) return;
    if (fread(&magic, 4, 1, f) != 1 || magic != SAVE_MAGIC) { fclose(f); return; }
    if (fread(&count, 4, 1, f) != 1) { fclose(f); return; }
    for (i = 0; i < count; i++) {
        unsigned long b;
        if (fread(&b, 4, 1, f) != 1 || b >= RK_NBLK) break;
        if (fread(&disk[b * RK_BLKW], sizeof(u16), RK_BLKW, f) != RK_BLKW) break;
        dirty[b] = 1;
    }
    fclose(f);
}

static void save_overlay(void)
{
    FILE *f;
    unsigned long magic = SAVE_MAGIC, count = 0, b;
    for (b = 0; b < RK_NBLK; b++) if (dirty[b]) count++;
    if (!count) return;
    f = fopen(savepath, "wb");
    if (!f) return;
    fwrite(&magic, 4, 1, f);
    fwrite(&count, 4, 1, f);
    for (b = 0; b < RK_NBLK; b++) {
        if (!dirty[b]) continue;
        fwrite(&b, 4, 1, f);
        fwrite(&disk[b * RK_BLKW], sizeof(u16), RK_BLKW, f);
    }
    fclose(f);
}

static void set_savepath(const char *override)
{
    if (override) {
        snprintf(savepath, sizeof(savepath), "%s", override);
        return;
    }
#ifdef _WIN32
    {
        char dir[768];
        DWORD n = GetModuleFileNameA(NULL, dir, sizeof(dir) - 1);
        if (n > 0 && n < sizeof(dir) - 1) {
            char *p;
            dir[n] = 0;
            p = strrchr(dir, '\\');
            if (p) {
                *p = 0;
                snprintf(savepath, sizeof(savepath), "%s\\adventure.sav", dir);
                return;
            }
        }
    }
#endif
    snprintf(savepath, sizeof(savepath), "adventure.sav");
}

/* Started from Explorer rather than from a shell, this process owns its
 * console alone and the window closes the moment it returns -- taking the
 * final score with it.  Hold it open in that case only. */
static void hold_console(void)
{
#ifdef _WIN32
    DWORD pids[4];
    if (!stdin_tty) return;
    if (GetConsoleProcessList(pids, 4) != 1) return;
    fputs("\n[press any key]", stdout);
    fflush(stdout);
    _getch();
#endif
}

/* ------------------------------------------------------------------ */
/* Bootstrap                                                          */
/* ------------------------------------------------------------------ */

/* The RK8E bootstrap, as toggled in from the front panel: point the control
 * at block 0, start it, and sit in a loop that the arriving block replaces. */
static const u16 boot_rom[] = {
    /* 023 */ 06007,        /* CAF                                   */
    /* 024 */ 06744,        /* DLCA   -- current address := 0         */
    /* 025 */ 01032,        /* TAD 032                                */
    /* 026 */ 06746,        /* DLDC   -- command := read, field 0     */
    /* 027 */ 06743,        /* DLAG   -- disk address := 0, and go    */
    /* 030 */ 01032,        /* TAD 032                                */
    /* 031 */ 05031,        /* JMP .                                  */
    /* 032 */ 00000
};

static void boot(void)
{
    unsigned i;
    for (i = 0; i < sizeof(boot_rom) / sizeof(boot_rom[0]); i++)
        M[023 + i] = boot_rom[i];
    AC = 0;
    LINK = 0;
    IFR = DFR = IBR = SFR = 0;
    PC = 023;
}

/* ------------------------------------------------------------------ */

static void usage(const char *me)
{
    fprintf(stderr,
        "Colossal Cave Adventure (350 points) -- PDP-8 / OS-8 FORTRAN IV\n"
        "usage: %s [options]\n"
        "  -s FILE   save-file path (default: adventure.sav beside the program)\n"
        "  -t FILE   also write a transcript of the session to FILE\n"
        "  -n        do not read or write a save file\n"
        "  -r        show the OS/8 boot dialogue instead of hiding it\n"
        "  -C        trace console traffic to stderr\n"
        "  -D N      trace the first N instructions to stderr\n"
        "  -h        this message\n", me);
}

int main(int argc, char **argv)
{
    const char *save_override = NULL;
    const char *tpath = NULL;
    int nosave = 0, i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") && i + 1 < argc) save_override = argv[++i];
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) tpath = argv[++i];
        else if (!strcmp(argv[i], "-n")) nosave = 1;
        else if (!strcmp(argv[i], "-r")) raw_console = 1;
        else if (!strcmp(argv[i], "-C")) con_trace = 1;
        else if (!strcmp(argv[i], "-D") && i + 1 < argc) trace_n = atol(argv[++i]);
        else { usage(argv[0]); return strcmp(argv[i], "-h") ? 1 : 0; }
    }

    stdin_tty = isatty_fd(0);
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);   /* OS/8 sends its own CR LF */
    _setmode(_fileno(stdin), _O_BINARY);
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    tty_raw();
#endif
    if (tpath) transcript = fopen(tpath, "wb");

    set_savepath(save_override);
    load_disk();
    if (!nosave) {
        load_overlay();
        atexit(save_overlay);
    }

    boot();
    run();

    fflush(stdout);
    if (halted) {
        fprintf(stderr, "\n[machine halted at %o:%04o]\n", IFR, PC);
        return 1;
    }
    return 0;
}
