/* cpu990.c - a TI 990/10 processor in user mode (see cpu990.h).
 *
 * The instruction semantics, status bits included, follow Dave Pitts'
 * sim990 (simops.c), the simulator the original was run on for reference.
 * A task runs non-privileged: privileged and CRU instructions, the 990/12
 * extensions, and XOPs other than 15 end the run with a fault, as they would
 * end the task under DX10. */
#include <stdio.h>
#include "cpu990.h"

uint8_t mem[65536];
uint16_t cpu_wp, cpu_pc, cpu_st;
unsigned long long cpu_count;
svc_fn cpu_svc;
char cpu_fault[128];

#define WP cpu_wp
#define PC cpu_pc
#define ST cpu_st

static int stop;

static inline uint16_t reg(int n)
{
    return rdw((uint16_t)(WP + 2 * n));
}

static inline void setreg(int n, uint16_t v)
{
    wrw((uint16_t)(WP + 2 * n), v);
}

static inline uint16_t fetch(void)
{
    uint16_t v = rdw(PC);
    PC = (uint16_t)(PC + 2);
    return v;
}

static void fault(const char *what, uint16_t inst)
{
    snprintf(cpu_fault, sizeof cpu_fault, "%s (>%04X) at >%04X", what, inst,
             (uint16_t)(PC - 2));
    stop = CPU_FAULT;
}

/* effective address of a general operand; SIZE is the *Rn+ increment */
static uint16_t ea(int t, int r, int size)
{
    uint16_t a;

    switch (t) {
    case 0:
        return (uint16_t)(WP + 2 * r);
    case 1:
        return reg(r);
    case 2:
        a = fetch();
        if (r)
            a = (uint16_t)(a + reg(r));
        return a;
    default:
        a = reg(r);
        setreg(r, (uint16_t)(a + size));
        return a;
    }
}

static void cmpzw(uint16_t v)
{
    ST &= (uint16_t)~(ST_LGT | ST_AGT | ST_EQ);
    if (v)
        ST |= ST_LGT;
    if ((int16_t)v > 0)
        ST |= ST_AGT;
    if (!v)
        ST |= ST_EQ;
}

static void cmpzb(uint8_t v)
{
    ST &= (uint16_t)~(ST_LGT | ST_AGT | ST_EQ);
    if (v)
        ST |= ST_LGT;
    if ((int8_t)v > 0)
        ST |= ST_AGT;
    if (!v)
        ST |= ST_EQ;
}

/* compare A with B (C and CB compare the source with the destination) */
static void cmpw(uint16_t a, uint16_t b)
{
    ST &= (uint16_t)~(ST_LGT | ST_AGT | ST_EQ);
    if (a > b)
        ST |= ST_LGT;
    if ((int16_t)a > (int16_t)b)
        ST |= ST_AGT;
    if (a == b)
        ST |= ST_EQ;
}

static void cmpb(uint8_t a, uint8_t b)
{
    ST &= (uint16_t)~(ST_LGT | ST_AGT | ST_EQ);
    if (a > b)
        ST |= ST_LGT;
    if ((int8_t)a > (int8_t)b)
        ST |= ST_AGT;
    if (a == b)
        ST |= ST_EQ;
}

static void parity(uint8_t v)
{
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    if (v & 1)
        ST |= ST_OP;
    else
        ST &= (uint16_t)~ST_OP;
}

/* R = S + D */
static void addw(uint16_t r, uint16_t s, uint16_t d)
{
    ST &= (uint16_t)~(ST_C | ST_OV);
    if (r < s)
        ST |= ST_C;
    if ((s & 0x8000) == (d & 0x8000) && (d & 0x8000) != (r & 0x8000))
        ST |= ST_OV;
}

static void addb(uint8_t r, uint8_t s, uint8_t d)
{
    ST &= (uint16_t)~(ST_C | ST_OV);
    if (r < s)
        ST |= ST_C;
    if ((s & 0x80) == (d & 0x80) && (d & 0x80) != (r & 0x80))
        ST |= ST_OV;
}

/* R = D - S */
static void subw(uint16_t r, uint16_t s, uint16_t d)
{
    ST &= (uint16_t)~(ST_C | ST_OV);
    if (d >= s)
        ST |= ST_C;
    if ((s & 0x8000) != (d & 0x8000) && (d & 0x8000) != (r & 0x8000))
        ST |= ST_OV;
}

static void subb(uint8_t r, uint8_t s, uint8_t d)
{
    ST &= (uint16_t)~(ST_C | ST_OV);
    if (d >= s)
        ST |= ST_C;
    if ((s & 0x80) != (d & 0x80) && (d & 0x80) != (r & 0x80))
        ST |= ST_OV;
}

static void exec(uint16_t in);

/* format I: the two-operand instructions, >4000 - >FFFF */
static void two_op(uint16_t in)
{
    int op = in >> 12, byte = op & 1;
    int td = (in >> 10) & 3, d = (in >> 6) & 15, ts = (in >> 4) & 3, s = in & 15;
    uint16_t sa = ea(ts, s, byte ? 1 : 2);
    uint16_t da = ea(td, d, byte ? 1 : 2);

    if (byte) {
        uint8_t sv = mem[sa], dv, r;

        switch (op) {
        case 5:                         /* SZCB */
            r = (uint8_t)(mem[da] & ~sv);
            mem[da] = r;
            cmpzb(r);
            parity(r);
            break;
        case 7:                         /* SB */
            dv = mem[da];
            r = (uint8_t)(dv - sv);
            subb(r, sv, dv);
            mem[da] = r;
            cmpzb(r);
            parity(r);
            break;
        case 9:                         /* CB */
            cmpb(sv, mem[da]);
            parity(sv);
            break;
        case 11:                        /* AB */
            dv = mem[da];
            r = (uint8_t)(dv + sv);
            addb(r, sv, dv);
            mem[da] = r;
            cmpzb(r);
            parity(r);
            break;
        case 13:                        /* MOVB */
            mem[da] = sv;
            cmpzb(sv);
            parity(sv);
            break;
        default:                        /* SOCB */
            r = (uint8_t)(mem[da] | sv);
            mem[da] = r;
            cmpzb(r);
            parity(r);
            break;
        }
    } else {
        uint16_t sv = rdw(sa), dv, r;

        switch (op) {
        case 4:                         /* SZC */
            r = (uint16_t)(rdw(da) & ~sv);
            wrw(da, r);
            cmpzw(r);
            break;
        case 6:                         /* S */
            dv = rdw(da);
            r = (uint16_t)(dv - sv);
            subw(r, sv, dv);
            wrw(da, r);
            cmpzw(r);
            break;
        case 8:                         /* C */
            cmpw(sv, rdw(da));
            break;
        case 10:                        /* A */
            dv = rdw(da);
            r = (uint16_t)(dv + sv);
            addw(r, sv, dv);
            wrw(da, r);
            cmpzw(r);
            break;
        case 12:                        /* MOV */
            wrw(da, sv);
            cmpzw(sv);
            break;
        default:                        /* SOC */
            r = (uint16_t)(rdw(da) | sv);
            wrw(da, r);
            cmpzw(r);
            break;
        }
    }
}

/* >1000 - >1FFF: the jumps (the CRU bit instructions are privileged here) */
static void jump(uint16_t in)
{
    int take;

    switch ((in >> 8) & 15) {
    case 0:  take = 1; break;                                           /* JMP */
    case 1:  take = !(ST & ST_AGT) && !(ST & ST_EQ); break;             /* JLT */
    case 2:  take = !(ST & ST_LGT) || (ST & ST_EQ); break;              /* JLE */
    case 3:  take = (ST & ST_EQ) != 0; break;                           /* JEQ */
    case 4:  take = (ST & ST_LGT) || (ST & ST_EQ); break;               /* JHE */
    case 5:  take = (ST & ST_AGT) != 0; break;                          /* JGT */
    case 6:  take = !(ST & ST_EQ); break;                               /* JNE */
    case 7:  take = !(ST & ST_C); break;                                /* JNC */
    case 8:  take = (ST & ST_C) != 0; break;                            /* JOC */
    case 9:  take = !(ST & ST_OV); break;                               /* JNO */
    case 10: take = !(ST & ST_LGT) && !(ST & ST_EQ); break;             /* JL */
    case 11: take = (ST & ST_LGT) && !(ST & ST_EQ); break;              /* JH */
    case 12: take = (ST & ST_OP) != 0; break;                           /* JOP */
    default:
        fault("CRU instruction", in);
        return;
    }
    if (take) {
        if ((in & 0xFF) == 0xFF && (in & 0x0F00) == 0) {
            fault("jump to itself (the task would hang)", in);
            return;
        }
        PC = (uint16_t)(PC + 2 * (int8_t)(in & 0xFF));
    }
}

/* >2000 - >2FFF: COC, CZC, XOR, XOP */
static void logical(uint16_t in)
{
    int d = (in >> 6) & 15, ts = (in >> 4) & 3, s = in & 15;
    uint16_t sv, r;

    switch ((in >> 10) & 3) {
    case 0:                             /* COC */
        sv = rdw(ea(ts, s, 2));
        if ((sv & reg(d)) == sv)
            ST |= ST_EQ;
        else
            ST &= (uint16_t)~ST_EQ;
        break;
    case 1:                             /* CZC */
        sv = rdw(ea(ts, s, 2));
        if ((sv & ~reg(d)) == sv)
            ST |= ST_EQ;
        else
            ST &= (uint16_t)~ST_EQ;
        break;
    case 2:                             /* XOR */
        sv = rdw(ea(ts, s, 2));
        r = sv ^ reg(d);
        setreg(d, r);
        cmpzw(r);
        break;
    default:                            /* XOP */
        sv = ea(ts, s, 2);
        if (d != 15) {
            fault("XOP other than 15", in);
            break;
        }
        r = (uint16_t)cpu_svc(sv);
        if (r)
            stop = (int16_t)r;
        break;
    }
}

/* >3800 - >3FFF: MPY, DIV */
static void muldiv(uint16_t in)
{
    int d = (in >> 6) & 15, ts = (in >> 4) & 3, s = in & 15;
    uint16_t sv = rdw(ea(ts, s, 2));
    uint32_t acc;

    if (!(in & 0x0400)) {                       /* MPY */
        acc = (uint32_t)reg(d) * sv;
        setreg(d, (uint16_t)(acc >> 16));
        setreg(d + 1, (uint16_t)acc);
    } else if (sv <= reg(d)) {                  /* DIV, quotient too large */
        ST |= ST_OV;
    } else {
        ST &= (uint16_t)~ST_OV;
        acc = (uint32_t)reg(d) << 16 | reg(d + 1);
        setreg(d, (uint16_t)(acc / sv));
        setreg(d + 1, (uint16_t)(acc % sv));
    }
}

/* >0400 - >07FF: the single-operand instructions */
static void one_op(uint16_t in)
{
    int ts = (in >> 4) & 3, s = in & 15;
    uint16_t a, v, r, nwp, npc;

    switch ((in >> 6) & 15) {
    case 0:                             /* BLWP */
        if (ts == 0) {
            nwp = reg(s);
            npc = reg(s + 1);
        } else {
            a = ea(ts, s, 2);
            nwp = rdw(a);
            npc = rdw((uint16_t)(a + 2));
        }
        a = WP;
        WP = nwp & 0xFFFE;
        setreg(13, a);
        setreg(14, PC);
        setreg(15, ST);
        PC = npc & 0xFFFE;
        break;
    case 1:                             /* B */
        PC = ea(ts, s, 2) & 0xFFFE;
        break;
    case 2:                             /* X */
        a = ea(ts, s, 2);
        exec(rdw(a));
        break;
    case 3:                             /* CLR */
        wrw(ea(ts, s, 2), 0);
        break;
    case 4:                             /* NEG */
        a = ea(ts, s, 2);
        v = rdw(a);
        r = (uint16_t)-v;
        ST &= (uint16_t)~(ST_C | ST_OV);
        if (r == 0)
            ST |= ST_C;
        if (v == 0x8000)
            ST |= ST_OV;
        wrw(a, r);
        cmpzw(r);
        break;
    case 5:                             /* INV */
        a = ea(ts, s, 2);
        r = (uint16_t)~rdw(a);
        wrw(a, r);
        cmpzw(r);
        break;
    case 6:                             /* INC */
    case 7:                             /* INCT */
        a = ea(ts, s, 2);
        v = rdw(a);
        r = (uint16_t)(v + ((in & 0x0040) ? 2 : 1));
        addw(r, v, (in & 0x0040) ? 2 : 1);
        wrw(a, r);
        cmpzw(r);
        break;
    case 8:                             /* DEC */
    case 9:                             /* DECT */
        a = ea(ts, s, 2);
        v = rdw(a);
        r = (uint16_t)(v - ((in & 0x0040) ? 2 : 1));
        subw(r, (in & 0x0040) ? 2 : 1, v);
        wrw(a, r);
        cmpzw(r);
        break;
    case 10:                            /* BL */
        a = ea(ts, s, 2);
        setreg(11, PC);
        PC = a & 0xFFFE;
        break;
    case 11:                            /* SWPB */
        a = ea(ts, s, 2);
        v = rdw(a);
        wrw(a, (uint16_t)(v >> 8 | v << 8));
        break;
    case 12:                            /* SETO */
        wrw(ea(ts, s, 2), 0xFFFF);
        break;
    case 13:                            /* ABS */
        a = ea(ts, s, 2);
        v = rdw(a);
        ST &= (uint16_t)~(ST_C | ST_OV);
        cmpzw(v);
        if ((int16_t)v < 0) {
            if (v == 0x8000)
                ST |= ST_OV;
            v = (uint16_t)-v;
        }
        wrw(a, v);
        break;
    default:                            /* LDS, LDD */
        fault("privileged instruction", in);
        break;
    }
}

/* >0800 - >0BFF: SRA, SRL, SLA, SRC */
static void shift(uint16_t in)
{
    int w = in & 15, c = (in >> 4) & 15;
    uint16_t v = reg(w);
    uint32_t acc;

    if (c == 0) {
        c = reg(0) & 15;
        if (c == 0)
            c = 16;
    }
    switch ((in >> 8) & 3) {
    case 0:                             /* SRA */
        acc = (uint32_t)((int32_t)((uint32_t)v << 16) >> c);
        if (acc & 0x8000)
            ST |= ST_C;
        else
            ST &= (uint16_t)~ST_C;
        v = (uint16_t)(acc >> 16);
        break;
    case 1:                             /* SRL */
        acc = ((uint32_t)v << 16) >> c;
        if (acc & 0x8000)
            ST |= ST_C;
        else
            ST &= (uint16_t)~ST_C;
        v = (uint16_t)(acc >> 16);
        break;
    case 2: {                           /* SLA */
        uint32_t sbit;

        ST &= (uint16_t)~ST_OV;
        acc = v;
        sbit = acc & 0x8000;
        while (c--) {
            acc <<= 1;
            if (sbit != (acc & 0x8000))
                ST |= ST_OV;
            sbit = acc & 0x8000;
        }
        if (acc & 0x10000)
            ST |= ST_C;
        else
            ST &= (uint16_t)~ST_C;
        v = (uint16_t)acc;
        break;
    }
    default:                            /* SRC */
        while (c--) {
            int ob = v & 1;

            v = (uint16_t)(v >> 1);
            if (ob) {
                v |= 0x8000;
                ST |= ST_C;
            } else
                ST &= (uint16_t)~ST_C;
        }
        break;
    }
    setreg(w, v);
    cmpzw(v);
}

/* >0200 - >03FF: the immediates and the control instructions */
static void imm_ctl(uint16_t in)
{
    int w = in & 15;
    uint16_t v, t;

    if (in >= 0x0300) {
        if ((in & 0xFFF0) == 0x0380) {  /* RTWP (non-privileged form) */
            uint16_t r13 = reg(13), r14 = reg(14), r15 = reg(15);

            ST = (uint16_t)((ST & 0x01DF) | (r15 & 0xFE20));
            PC = r14 & 0xFFFE;
            WP = r13 & 0xFFFE;
        } else
            fault("privileged instruction", in);
        return;
    }
    switch ((in >> 4) & 15) {
    case 0:                             /* LI */
        v = fetch();
        setreg(w, v);
        cmpzw(v);
        break;
    case 2:                             /* AI */
        v = fetch();
        t = reg(w);
        setreg(w, (uint16_t)(t + v));
        addw((uint16_t)(t + v), v, t);
        cmpzw((uint16_t)(t + v));
        break;
    case 4:                             /* ANDI */
        v = (uint16_t)(reg(w) & fetch());
        setreg(w, v);
        cmpzw(v);
        break;
    case 6:                             /* ORI */
        v = (uint16_t)(reg(w) | fetch());
        setreg(w, v);
        cmpzw(v);
        break;
    case 8:                             /* CI */
        v = fetch();
        cmpw(reg(w), v);
        break;
    case 10:                            /* STWP */
        setreg(w, WP);
        break;
    case 12:                            /* STST */
        setreg(w, ST);
        break;
    case 14:                            /* LWPI */
        WP = fetch() & 0xFFFE;
        break;
    default:
        fault("illegal instruction", in);
        break;
    }
}

static void exec(uint16_t in)
{
    if (in >= 0x4000)
        two_op(in);
    else if (in >= 0x3800)
        muldiv(in);
    else if (in >= 0x3000)
        fault("CRU instruction", in);
    else if (in >= 0x2000)
        logical(in);
    else if (in >= 0x1000)
        jump(in);
    else if (in >= 0x0C00)
        fault("illegal instruction (990/12)", in);
    else if (in >= 0x0800)
        shift(in);
    else if (in >= 0x0400)
        one_op(in);
    else if (in >= 0x0200)
        imm_ctl(in);
    else
        fault("illegal instruction (990/12)", in);
}

/* instructions allowed between two supervisor calls before the run is
 * taken to be stuck (0: no limit) */
unsigned long long cpu_watchdog;

int cpu_run(void)
{
    unsigned long long quiet = 0;

    stop = 0;
    while (!stop) {
        uint16_t in = fetch();

        cpu_count++;
        if ((in & 0xFC00) == 0x2C00)    /* an XOP */
            quiet = 0;
        else if (cpu_watchdog && ++quiet > cpu_watchdog) {
            PC = (uint16_t)(PC - 2);
            snprintf(cpu_fault, sizeof cpu_fault,
                     "no supervisor call for %llu instructions, at >%04X", quiet, PC);
            return CPU_FAULT;
        }
        exec(in);
    }
    return stop;
}
