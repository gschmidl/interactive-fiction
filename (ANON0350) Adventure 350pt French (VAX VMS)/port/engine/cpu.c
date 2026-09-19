/* cpu.c - VAX-11 user-mode instruction interpreter */
#include "vax.h"
#include <math.h>
#include <stdarg.h>

VAXCPU cpu;
int trace_level = 0;
u32 pcring[1024];
int pcring_i;
u32 vms_caller;
u32 brk_pc;
u64 abort_at;
u32 dump_addr;
int brk_fatal;
FILE *tracef;

void vax_fatal(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "\n*** vax: ");
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fprintf(stderr, "\n    PC=%08X SP=%08X FP=%08X AP=%08X icount=%llu\n",
            cpu.r[PC], cpu.r[SP], cpu.r[FP], cpu.r[AP],
            (unsigned long long)cpu.icount);
    {   int i;
        for (i = 0; i < 12; i++)
            fprintf(stderr, "    R%-2d=%08X%s", i, cpu.r[i], (i % 4) == 3 ? "\n" : "");
        fprintf(stderr, "    recent PCs (oldest first):");
        for (i = 0; i < 1024; i++) {
            u32 p = pcring[(pcring_i + i) & 1023];
            if (p) fprintf(stderr, "%s%08X", (i % 8) ? " " : "\n      ", p);
        }
        fprintf(stderr, "\n");
    }
    exit(2);
}

/* ------------------------------------------------------------------ */
/* instruction stream                                                  */
static u8  fetch8(void)  { u8  v = rd8 (cpu.r[PC]); cpu.r[PC] += 1; return v; }
static u16 fetch16(void) { u16 v = rd16(cpu.r[PC]); cpu.r[PC] += 2; return v; }
static u32 fetch32(void) { u32 v = rd32(cpu.r[PC]); cpu.r[PC] += 4; return v; }

/* ------------------------------------------------------------------ */
/* operands                                                            */
enum { OP_REG = 1, OP_MEM, OP_IMM };

typedef struct { u8 kind; u32 a; } Opnd;

static void decode(Opnd *o, int size)
{
    u8 spec = fetch8();
    int mode = spec >> 4, reg = spec & 15;
    switch (mode) {
    case 0: case 1: case 2: case 3:
        o->kind = OP_IMM; o->a = spec & 0x3F; break;
    case 4: {                                   /* indexed */
        u32 idx = cpu.r[reg];
        Opnd base;
        decode(&base, size);
        if (base.kind != OP_MEM)
            vax_fatal("indexed mode with non-memory base (PC=%08X)", cpu.r[PC]);
        o->kind = OP_MEM; o->a = base.a + idx * (u32)size;
        break; }
    case 5:  o->kind = OP_REG; o->a = reg; break;
    case 6:  o->kind = OP_MEM; o->a = cpu.r[reg]; break;
    case 7:  cpu.r[reg] -= size; o->kind = OP_MEM; o->a = cpu.r[reg]; break;
    case 8:                                     /* (Rn)+ , 8F = immediate */
        o->kind = OP_MEM; o->a = cpu.r[reg];
        cpu.r[reg] += size;
        break;
    case 9:                                     /* @(Rn)+ , 9F = absolute */
        o->kind = OP_MEM; o->a = rd32(cpu.r[reg]);
        cpu.r[reg] += 4;
        break;
    case 10: { i8  d = (i8) fetch8();  o->kind = OP_MEM; o->a = cpu.r[reg] + d; break; }
    case 11: { i8  d = (i8) fetch8();  o->kind = OP_MEM; o->a = rd32(cpu.r[reg] + d); break; }
    case 12: { i16 d = (i16)fetch16(); o->kind = OP_MEM; o->a = cpu.r[reg] + d; break; }
    case 13: { i16 d = (i16)fetch16(); o->kind = OP_MEM; o->a = rd32(cpu.r[reg] + d); break; }
    case 14: { i32 d = (i32)fetch32(); o->kind = OP_MEM; o->a = cpu.r[reg] + d; break; }
    default: { i32 d = (i32)fetch32(); o->kind = OP_MEM; o->a = rd32(cpu.r[reg] + d); break; }
    }
}

static u32 op_rd(const Opnd *o, int size)
{
    if (o->kind == OP_IMM) return o->a;
    if (o->kind == OP_REG) {
        u32 v = cpu.r[o->a];
        if (size == 1) return v & 0xFF;
        if (size == 2) return v & 0xFFFF;
        return v;
    }
    if (size == 1) return rd8(o->a);
    if (size == 2) return rd16(o->a);
    return rd32(o->a);
}

static u64 op_rdq(const Opnd *o)
{
    if (o->kind == OP_IMM) return o->a;
    if (o->kind == OP_REG)
        return (u64)cpu.r[o->a] | ((u64)cpu.r[(o->a + 1) & 15] << 32);
    return rd64(o->a);
}

static void op_wr(const Opnd *o, int size, u32 v)
{
    if (o->kind == OP_REG) {
        if (size == 1)      cpu.r[o->a] = (cpu.r[o->a] & 0xFFFFFF00u) | (v & 0xFF);
        else if (size == 2) cpu.r[o->a] = (cpu.r[o->a] & 0xFFFF0000u) | (v & 0xFFFF);
        else                cpu.r[o->a] = v;
        return;
    }
    if (o->kind == OP_IMM) vax_fatal("write to literal operand (PC=%08X)", cpu.r[PC]);
    if (size == 1)      wr8 (o->a, (u8)v);
    else if (size == 2) wr16(o->a, (u16)v);
    else                wr32(o->a, v);
}

static void op_wrq(const Opnd *o, u64 v)
{
    if (o->kind == OP_REG) {
        cpu.r[o->a] = (u32)v; cpu.r[(o->a + 1) & 15] = (u32)(v >> 32); return;
    }
    if (o->kind == OP_IMM) vax_fatal("write to literal quadword (PC=%08X)", cpu.r[PC]);
    wr64(o->a, v);
}

/* address of an operand (for MOVAx / PUSHAx / string descriptors) */
static u32 op_addr(const Opnd *o)
{
    if (o->kind != OP_MEM) vax_fatal("address of non-memory operand (PC=%08X)", cpu.r[PC]);
    return o->a;
}

/* ------------------------------------------------------------------ */
/* condition codes                                                     */
#define SETNZ(res, size) do { \
    i32 _r = (size)==1 ? (i32)(i8)(res) : (size)==2 ? (i32)(i16)(res) : (i32)(res); \
    cpu.psl &= ~(PSL_N|PSL_Z); \
    if (_r < 0) cpu.psl |= PSL_N; \
    if (_r == 0) cpu.psl |= PSL_Z; \
} while (0)
#define CLRVC() (cpu.psl &= ~(PSL_V|PSL_C))
#define CLRV()  (cpu.psl &= ~PSL_V)
#define SETV(x) do { if (x) cpu.psl |= PSL_V; else cpu.psl &= ~PSL_V; } while (0)
#define SETC(x) do { if (x) cpu.psl |= PSL_C; else cpu.psl &= ~PSL_C; } while (0)

static i32 sext(u32 v, int size)
{
    if (size == 1) return (i32)(i8)v;
    if (size == 2) return (i32)(i16)v;
    return (i32)v;
}
static u32 masksz(u32 v, int size)
{
    if (size == 1) return v & 0xFF;
    if (size == 2) return v & 0xFFFF;
    return v;
}

static u32 do_add(u32 a, u32 b, int size, int carryin)
{
    u64 lim = size == 1 ? 0xFFull : size == 2 ? 0xFFFFull : 0xFFFFFFFFull;
    u64 ua = a & lim, ub = b & lim;
    u64 s  = ua + ub + (unsigned)carryin;
    i32 sa = sext(a, size), sb = sext(b, size), sr = sext((u32)s, size);
    SETNZ((u32)s, size);
    SETV(((sa < 0) == (sb < 0)) && ((sr < 0) != (sa < 0)));
    SETC(s > lim);
    return masksz((u32)s, size);
}

static u32 do_sub(u32 minuend, u32 sub, int size, int borrow)
{
    u64 lim = size == 1 ? 0xFFull : size == 2 ? 0xFFFFull : 0xFFFFFFFFull;
    u64 um = minuend & lim, us = sub & lim;
    u64 d  = um - us - (unsigned)borrow;
    i32 sm = sext(minuend, size), ss = sext(sub, size), sr = sext((u32)d, size);
    SETNZ((u32)d, size);
    SETV(((sm < 0) != (ss < 0)) && ((sr < 0) != (sm < 0)));
    SETC((um < us + (unsigned)borrow) || (us + (unsigned)borrow > lim));
    return masksz((u32)d, size);
}

static void do_cmp(u32 a, u32 b, int size)
{
    i32 sa = sext(a, size), sb = sext(b, size);
    u32 ua = masksz(a, size), ub = masksz(b, size);
    cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
    if (sa < sb) cpu.psl |= PSL_N;
    if (sa == sb) cpu.psl |= PSL_Z;
    if (ua < ub) cpu.psl |= PSL_C;
}

/* ------------------------------------------------------------------ */
/* VAX floating point <-> host double                                  */
static double f_to_d(u32 v)
{
    int s = (v >> 15) & 1, e = (v >> 7) & 0xFF;
    u32 frac = ((v & 0x7F) << 16) | ((v >> 16) & 0xFFFF);
    double m;
    if (e == 0) return s ? 0.0 : 0.0;           /* reserved operand treated as 0 */
    m = 0.5 + (double)frac / (double)(1u << 24);
    return (s ? -m : m) * ldexp(1.0, e - 128);
}
static u32 d_to_f(double d)
{
    int s = 0, e;
    double m; u32 frac, out;
    if (d == 0.0 || !isfinite(d)) return 0;
    if (d < 0) { s = 1; d = -d; }
    m = frexp(d, &e);                            /* 0.5 <= m < 1 */
    e += 128;
    if (e <= 0) return 0;
    if (e > 255) e = 255;
    frac = (u32)((m - 0.5) * (double)(1u << 24) + 0.5);
    if (frac >> 23) { frac = 0; e++; }
    out = ((u32)s << 15) | ((u32)(e & 0xFF) << 7) | ((frac >> 16) & 0x7F);
    out |= (frac & 0xFFFF) << 16;
    return out;
}
static double dfl_to_d(u64 v)
{
    int s = (int)((v >> 15) & 1), e = (int)((v >> 7) & 0xFF);
    u64 frac = ((v & 0x7F) << 48) | (((v >> 16) & 0xFFFF) << 32)
             | (((v >> 32) & 0xFFFF) << 16) | ((v >> 48) & 0xFFFF);
    double m;
    if (e == 0) return 0.0;
    m = 0.5 + (double)frac / 9007199254740992.0 / 2.0;  /* frac / 2^55 */
    return (s ? -m : m) * ldexp(1.0, e - 128);
}
static u64 d_to_dfl(double d)
{
    int s = 0, e; double m; u64 frac, out;
    if (d == 0.0 || !isfinite(d)) return 0;
    if (d < 0) { s = 1; d = -d; }
    m = frexp(d, &e);
    e += 128;
    if (e <= 0) return 0;
    if (e > 255) e = 255;
    frac = (u64)((m - 0.5) * 2.0 * 9007199254740992.0 * 4.0 + 0.5);  /* * 2^55 */
    frac &= 0x7FFFFFFFFFFFFFull;
    out  = ((u64)s << 15) | ((u64)(e & 0xFF) << 7) | ((frac >> 48) & 0x7F);
    out |= ((frac >> 32) & 0xFFFF) << 16;
    out |= ((frac >> 16) & 0xFFFF) << 32;
    out |= ( frac        & 0xFFFF) << 48;
    return out;
}
static double g_to_d(u64 v)
{
    int s = (int)((v >> 15) & 1), e = (int)((v >> 4) & 0x7FF);
    u64 frac = ((v & 0xF) << 48) | (((v >> 16) & 0xFFFF) << 32)
             | (((v >> 32) & 0xFFFF) << 16) | ((v >> 48) & 0xFFFF);
    double m;
    if (e == 0) return 0.0;
    m = 0.5 + (double)frac / 18014398509481984.0 / 2.0;  /* frac / 2^52 / 2 */
    return (s ? -m : m) * ldexp(1.0, e - 1024);
}
static u64 d_to_g(double d)
{
    int s = 0, e; double m; u64 frac, out;
    if (d == 0.0 || !isfinite(d)) return 0;
    if (d < 0) { s = 1; d = -d; }
    m = frexp(d, &e);
    e += 1024;
    if (e <= 0) return 0;
    if (e > 2047) e = 2047;
    frac = (u64)((m - 0.5) * 2.0 * 18014398509481984.0 + 0.5);
    frac &= 0xFFFFFFFFFFFFFull;
    out  = ((u64)s << 15) | ((u64)(e & 0x7FF) << 4) | ((frac >> 48) & 0xF);
    out |= ((frac >> 32) & 0xFFFF) << 16;
    out |= ((frac >> 16) & 0xFFFF) << 32;
    out |= ( frac        & 0xFFFF) << 48;
    return out;
}

/* short literals denote small floats: value = (4 + frac/8) * 2^(exp-4) */
static double lit_to_d(u32 lit)
{
    int e = (lit >> 3) & 7, f = lit & 7;
    return (double)(4 + f) / 8.0 * ldexp(1.0, e);
}
static double op_rdf(const Opnd *o)
{
    if (o->kind == OP_IMM) return lit_to_d(o->a);
    return f_to_d(op_rd(o, 4));
}
static double op_rdd(const Opnd *o)
{
    if (o->kind == OP_IMM) return lit_to_d(o->a);
    return dfl_to_d(op_rdq(o));
}
static double op_rdg(const Opnd *o)
{
    if (o->kind == OP_IMM) return lit_to_d(o->a);
    return g_to_d(op_rdq(o));
}
static void setnz_d(double d)
{
    cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
    if (d < 0) cpu.psl |= PSL_N;
    if (d == 0) cpu.psl |= PSL_Z;
}

/* ------------------------------------------------------------------ */
/* stack helpers                                                       */
static void push32(u32 v) { cpu.r[SP] -= 4; wr32(cpu.r[SP], v); }
static u32  pop32(void)   { u32 v = rd32(cpu.r[SP]); cpu.r[SP] += 4; return v; }

/* ------------------------------------------------------------------ */
/* branch helpers                                                      */
static void branch8(int taken)
{
    i8 d = (i8)fetch8();
    if (taken) cpu.r[PC] += d;
}
static void branch16(int taken)
{
    i16 d = (i16)fetch16();
    if (taken) cpu.r[PC] += d;
}

#define CC_N ((cpu.psl & PSL_N) != 0)
#define CC_Z ((cpu.psl & PSL_Z) != 0)
#define CC_V ((cpu.psl & PSL_V) != 0)
#define CC_C ((cpu.psl & PSL_C) != 0)

/* ------------------------------------------------------------------ */
/* variable-length bit field                                           */
/* The size operand is a byte holding 0..32 - do not mask it to 5 bits. */
static int fldsize(u32 v)
{
    int s = (int)(v & 0xFF);
    return (s > 32) ? 32 : s;
}

static u32 field_read(const Opnd *base, i32 pos, int size, int sgn)
{
    u32 v;
    if (size == 0) return 0;
    if (base->kind == OP_REG) {
        u64 rr = (u64)cpu.r[base->a] | ((u64)cpu.r[(base->a + 1) & 15] << 32);
        v = (u32)((rr >> pos) & ((size >= 32) ? 0xFFFFFFFFu : ((1u << size) - 1)));
    } else {
        u32 a = base->a + (pos >> 3);
        int b = pos & 7, nb = (b + size + 7) >> 3, i;
        u64 chunk = 0;
        for (i = 0; i < nb; i++) chunk |= (u64)rd8(a + i) << (8 * i);
        v = (u32)((chunk >> b) & ((size >= 32) ? 0xFFFFFFFFu : ((1u << size) - 1)));
    }
    if (sgn && size < 32 && (v & (1u << (size - 1))))
        v |= ~((1u << size) - 1);
    return v;
}
static void field_write(const Opnd *base, i32 pos, int size, u32 val)
{
    u32 mask;
    if (size == 0) return;
    mask = (size >= 32) ? 0xFFFFFFFFu : ((1u << size) - 1);
    val &= mask;
    if (base->kind == OP_REG) {
        u64 rr = (u64)cpu.r[base->a] | ((u64)cpu.r[(base->a + 1) & 15] << 32);
        rr = (rr & ~(((u64)mask) << pos)) | (((u64)val) << pos);
        cpu.r[base->a] = (u32)rr;
        if (pos + size > 32) cpu.r[(base->a + 1) & 15] = (u32)(rr >> 32);
    } else {
        u32 a = base->a + (pos >> 3);
        int b = pos & 7, nb = (b + size + 7) >> 3, i;
        u64 chunk = 0;
        for (i = 0; i < nb; i++) chunk |= (u64)rd8(a + i) << (8 * i);
        chunk = (chunk & ~(((u64)mask) << b)) | (((u64)val) << b);
        for (i = 0; i < nb; i++) wr8(a + i, (u8)(chunk >> (8 * i)));
    }
}

/* ------------------------------------------------------------------ */
/* CALLS / CALLG / RET                                                 */
static void do_call(u32 dst, u32 arg_ap, int is_calls)
{
    u16 mask = rd16(dst);
    u32 spa, frame, tmpap = arg_ap;
    int i;
    spa = cpu.r[SP] & 3;
    cpu.r[SP] &= ~3u;
    for (i = 11; i >= 0; i--) if (mask & (1u << i)) push32(cpu.r[i]);
    push32(cpu.r[PC]);
    push32(cpu.r[FP]);
    push32(cpu.r[AP]);
    frame = (spa << 30) | ((u32)(is_calls ? 1 : 0) << 29)
          | (((u32)mask & 0x0FFF) << 16) | (cpu.psl & 0xFFFF);
    push32(frame);
    push32(0);                       /* condition handler */
    cpu.r[FP] = cpu.r[SP];
    cpu.r[AP] = tmpap;
    cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
    if (mask & 0x4000) cpu.psl |= PSL_IV; else cpu.psl &= ~PSL_IV;
    if (mask & 0x8000) cpu.psl |= PSL_DV; else cpu.psl &= ~PSL_DV;
    cpu.r[PC] = dst + 2;
}

static void do_ret(void)
{
    u32 frame, spa, n;
    int i, is_calls;
    cpu.r[SP] = cpu.r[FP] + 4;
    frame = pop32();
    cpu.r[AP] = pop32();
    cpu.r[FP] = pop32();
    cpu.r[PC] = pop32();
    for (i = 0; i <= 11; i++) if (frame & (1u << (16 + i))) cpu.r[i] = pop32();
    spa = (frame >> 30) & 3;
    cpu.r[SP] += spa;
    is_calls = (frame >> 29) & 1;
    if (is_calls) { n = pop32(); cpu.r[SP] += 4 * (n & 0xFF); }
    cpu.psl = (cpu.psl & ~0xFFFFu) | (frame & 0xFFFF);
}

/* current call frame argument access, for the VMS service shim */
int svc_argc(void) { return (int)(rd32(cpu.r[AP]) & 0xFF); }
u32 svc_arg(int n) { return rd32(cpu.r[AP] + 4 * n); }
void svc_return(u32 status) { cpu.r[0] = status; }

/* ------------------------------------------------------------------ */
static void unimpl(u32 op, u32 pc)
{
    vax_fatal("unimplemented opcode %02X%s at %08X",
              op & 0xFF, (op > 0xFF) ? " (FD-prefixed)" : "", pc);
}

void cpu_init(void)
{
    memset(&cpu, 0, sizeof cpu);
    cpu.psl = 0x03C00000;             /* user mode, IPL 0 */
}

/* Executed at an address with no mapped page: hand off to the VMS shim. */
static int service_trap(u32 pc)
{
    return vms_dispatch(pc);
}

/* Enter the image the way the image activator does: CALLS #0 with a sentinel
 * return address, so a RET out of the main program ends the run. */
void cpu_enter(u32 entry)
{
    push32(0);
    cpu.r[PC] = IMG_EXIT_PC;
    do_call(entry, cpu.r[SP], 1);
}

void cpu_step(void)
{
    u32 pc0 = cpu.r[PC];
    u32 op;
    Opnd a, b, c, d, e, f;

    pcring[pcring_i] = pc0; pcring_i = (pcring_i + 1) & 1023;
    if (pc0 == IMG_EXIT_PC) { cpu.halted = 1; return; }
    if (abort_at && cpu.icount >= abort_at) vax_fatal("trace window ended");
    if (brk_pc && pc0 == brk_pc) {
        int i, n = (int)(mem_present(cpu.r[AP]) ? (rd32(cpu.r[AP]) & 0xFF) : 0);
        fflush(stdout);
        fprintf(stderr, "[brk] PC=%08X AP=%08X argc=%d:", pc0, cpu.r[AP], n);
        for (i = 1; i <= n && i <= 8; i++) {
            u32 v = rd32(cpu.r[AP] + 4 * i);
            fprintf(stderr, " a%d=%08X", i, v);
            if (mem_present(v)) fprintf(stderr, "(->%08X)", rd32(v));
        }
        fprintf(stderr, "\n      R0=%08X R1=%08X R2=%08X R3=%08X R4=%08X R5=%08X",
                cpu.r[0], cpu.r[1], cpu.r[2], cpu.r[3], cpu.r[4], cpu.r[5]);
        fprintf(stderr, "\n      R6=%08X R7=%08X R8=%08X R9=%08X R10=%08X R11=%08X",
                cpu.r[6], cpu.r[7], cpu.r[8], cpu.r[9], cpu.r[10], cpu.r[11]);
        fprintf(stderr, "\n      SP=%08X stack:", cpu.r[SP]);
        for (i = 0; i < 5; i++)
            if (mem_present(cpu.r[SP] + 4 * i))
                fprintf(stderr, " %08X", rd32(cpu.r[SP] + 4 * i));
        if (dump_addr) {
            int m;
            fprintf(stderr, "\n      mem %08X:", dump_addr);
            for (m = 0; m < 16; m++)
                if (mem_present(dump_addr + m)) fprintf(stderr, " %02X", rd8(dump_addr + m));
        }
        fprintf(stderr, "\n");
        if (brk_fatal) vax_fatal("breakpoint at %08X", pc0);
    }
    if (trace_level && cpu.icount < (u64)trace_level) {
        int k;
        fprintf(stderr, "%08X:", pc0);
        for (k = 0; k < 8; k++)
            fprintf(stderr, " %02X", mem_present(pc0 + k) ? rd8(pc0 + k) : 0);
        fprintf(stderr, "   SP=%08X R0=%08X R1=%08X R11=%08X\n",
                cpu.r[SP], cpu.r[0], cpu.r[1], cpu.r[11]);
    }
    if (!mem_present(pc0)) {
        if (service_trap(pc0)) return;
        vax_fatal("execution at unmapped address %08X", pc0);
    }
    op = fetch8();
    if (op == 0xFD) op = 0x100 | fetch8();
    cpu.icount++;

    switch (op) {
    /* ---- no operand ---- */
    case 0x01: break;                                   /* NOP */
    case 0x00: vax_fatal("HALT at %08X", pc0);          /* HALT */
    case 0x04: do_ret(); break;                         /* RET */
    case 0x05: cpu.r[PC] = pop32(); break;              /* RSB */
    case 0x02: vax_fatal("REI at %08X", pc0);

    /* ---- branches ---- */
    case 0x11: branch8(1); break;                       /* BRB */
    case 0x31: branch16(1); break;                      /* BRW */
    case 0x12: branch8(!CC_Z); break;                   /* BNEQ */
    case 0x13: branch8(CC_Z); break;                    /* BEQL */
    case 0x14: branch8(!(CC_N || CC_Z)); break;         /* BGTR */
    case 0x15: branch8(CC_N || CC_Z); break;            /* BLEQ */
    case 0x18: branch8(!CC_N); break;                   /* BGEQ */
    case 0x19: branch8(CC_N); break;                    /* BLSS */
    case 0x1A: branch8(!(CC_C || CC_Z)); break;         /* BGTRU */
    case 0x1B: branch8(CC_C || CC_Z); break;            /* BLEQU */
    case 0x1C: branch8(!CC_V); break;                   /* BVC */
    case 0x1D: branch8(CC_V); break;                    /* BVS */
    case 0x1E: branch8(!CC_C); break;                   /* BGEQU / BCC */
    case 0x1F: branch8(CC_C); break;                    /* BLSSU / BCS */

    case 0x10: { i8 dd = (i8)fetch8();  push32(cpu.r[PC]); cpu.r[PC] += dd; break; } /* BSBB */
    case 0x30: { i16 dd = (i16)fetch16(); push32(cpu.r[PC]); cpu.r[PC] += dd; break; } /* BSBW */
    case 0x17: decode(&a, 1); cpu.r[PC] = op_addr(&a); break;                        /* JMP */
    case 0x16: decode(&a, 1); push32(cpu.r[PC]); cpu.r[PC] = op_addr(&a); break;     /* JSB */

    case 0xE8: decode(&a, 4); branch8((op_rd(&a,4) & 1) != 0); break;   /* BLBS */
    case 0xE9: decode(&a, 4); branch8((op_rd(&a,4) & 1) == 0); break;   /* BLBC */

    /* ---- bit branches ---- */
    case 0xE0: case 0xE1: {                             /* BBS / BBC */
        i32 pos; u32 bit;
        decode(&a, 4); decode(&b, 1);
        pos = (i32)op_rd(&a, 4);
        bit = field_read(&b, pos, 1, 0) & 1;
        branch8(op == 0xE0 ? bit : !bit);
        break; }
    case 0xE2: case 0xE3: case 0xE4: case 0xE5:         /* BBSS BBCS BBSC BBCC */
    case 0xE6: case 0xE7: {                             /* BBSSI BBCCI */
        i32 pos; u32 bit; int set;
        decode(&a, 4); decode(&b, 1);
        pos = (i32)op_rd(&a, 4);
        bit = field_read(&b, pos, 1, 0) & 1;
        set = (op == 0xE2 || op == 0xE3 || op == 0xE6);
        field_write(&b, pos, 1, set ? 1 : 0);
        branch8((op == 0xE2 || op == 0xE4 || op == 0xE6) ? bit : !bit);
        break; }

    /* ---- loops ---- */
    case 0xF2: {                                        /* AOBLSS */
        u32 lim, idx;
        decode(&a, 4); decode(&b, 4);
        lim = op_rd(&a, 4); idx = op_rd(&b, 4) + 1;
        op_wr(&b, 4, idx);
        SETNZ(idx, 4); CLRV();
        branch8((i32)idx < (i32)lim);
        break; }
    case 0xF3: {                                        /* AOBLEQ */
        u32 lim, idx;
        decode(&a, 4); decode(&b, 4);
        lim = op_rd(&a, 4); idx = op_rd(&b, 4) + 1;
        op_wr(&b, 4, idx);
        SETNZ(idx, 4); CLRV();
        branch8((i32)idx <= (i32)lim);
        break; }
    case 0xF4: {                                        /* SOBGEQ */
        u32 idx;
        decode(&a, 4); idx = op_rd(&a, 4) - 1; op_wr(&a, 4, idx);
        SETNZ(idx, 4); CLRV();
        branch8((i32)idx >= 0);
        break; }
    case 0xF5: {                                        /* SOBGTR */
        u32 idx;
        decode(&a, 4); idx = op_rd(&a, 4) - 1; op_wr(&a, 4, idx);
        SETNZ(idx, 4); CLRV();
        branch8((i32)idx > 0);
        break; }
    /* ---- CASE ---- */
    case 0x8F: case 0xAF: case 0xCF: {                  /* CASEB / CASEW / CASEL */
        int sz = (op == 0x8F) ? 1 : (op == 0xAF) ? 2 : 4;
        u32 sel, base, lim, tblpc; i32 idx;
        decode(&a, sz); decode(&b, sz); decode(&c, sz);
        sel = op_rd(&a, sz); base = op_rd(&b, sz); lim = op_rd(&c, sz);
        idx = (i32)sext(sel, sz) - (i32)sext(base, sz);
        tblpc = cpu.r[PC];
        do_cmp((u32)idx, masksz(lim, sz), 4);
        if ((u32)idx <= masksz(lim, sz)) {
            i16 disp = (i16)rd16(tblpc + 2 * idx);
            cpu.r[PC] = tblpc + disp;
        } else {
            cpu.r[PC] = tblpc + 2 * (masksz(lim, sz) + 1);
        }
        break; }

    /* ---- calls ---- */
    case 0xFB: {                                        /* CALLS */
        u32 n, dst;
        decode(&a, 4); decode(&b, 1);
        n = op_rd(&a, 4); dst = op_addr(&b);
        push32(n);
        if (vms_is_service(dst)) {                      /* system service vector */
            u32 saved_ap = cpu.r[AP];
            cpu.r[AP] = cpu.r[SP];
            vms_caller = cpu.r[PC];
            cpu.r[0] = vms_service(dst);
            cpu.r[SP] += 4 + 4 * (n & 0xFF);
            cpu.r[AP] = saved_ap;
            break;
        }
        do_call(dst, cpu.r[SP], 1);
        break; }
    case 0xFA: {                                        /* CALLG */
        u32 arglist, dst;
        decode(&a, 1); decode(&b, 1);
        arglist = op_addr(&a); dst = op_addr(&b);
        if (vms_is_service(dst)) {
            u32 saved_ap = cpu.r[AP];
            cpu.r[AP] = arglist;
            vms_caller = cpu.r[PC];
            cpu.r[0] = vms_service(dst);
            cpu.r[AP] = saved_ap;
            break;
        }
        do_call(dst, arglist, 0);
        break; }

    /* ---- CLR ---- */
    case 0x94: decode(&a,1); op_wr(&a,1,0); SETNZ(0,1); CLRVC(); break;
    case 0xB4: decode(&a,2); op_wr(&a,2,0); SETNZ(0,2); CLRVC(); break;
    case 0xD4: decode(&a,4); op_wr(&a,4,0); SETNZ(0,4); CLRVC(); break;
    case 0x7C: decode(&a,8); op_wrq(&a,0);  SETNZ(0,4); CLRVC(); break;   /* CLRQ */

    /* ---- MOV ---- */
    case 0x90: decode(&a,1); decode(&b,1); { u32 v=op_rd(&a,1); op_wr(&b,1,v); SETNZ(v,1); CLRV(); } break;
    case 0xB0: decode(&a,2); decode(&b,2); { u32 v=op_rd(&a,2); op_wr(&b,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xD0: decode(&a,4); decode(&b,4); { u32 v=op_rd(&a,4); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x7D: decode(&a,8); decode(&b,8); { u64 v=op_rdq(&a); op_wrq(&b,v);
                 cpu.psl &= ~(PSL_N|PSL_Z|PSL_V);
                 if ((i64)v < 0) cpu.psl |= PSL_N; if (v==0) cpu.psl |= PSL_Z; } break;

    /* ---- MOVZ / CVT ---- */
    case 0x9B: decode(&a,1); decode(&b,2); { u32 v=op_rd(&a,1)&0xFF; op_wr(&b,2,v); SETNZ(v,2); CLRVC(); } break; /* MOVZBW */
    case 0x9A: decode(&a,1); decode(&b,4); { u32 v=op_rd(&a,1)&0xFF; op_wr(&b,4,v); SETNZ(v,4); CLRVC(); } break; /* MOVZBL */
    case 0x3C: decode(&a,2); decode(&b,4); { u32 v=op_rd(&a,2)&0xFFFF; op_wr(&b,4,v); SETNZ(v,4); CLRVC(); } break; /* MOVZWL */
    case 0x99: decode(&a,1); decode(&b,2); { i32 v=sext(op_rd(&a,1),1); op_wr(&b,2,(u32)v); SETNZ(v,2); CLRVC(); } break; /* CVTBW */
    case 0x98: decode(&a,1); decode(&b,4); { i32 v=sext(op_rd(&a,1),1); op_wr(&b,4,(u32)v); SETNZ(v,4); CLRVC(); } break; /* CVTBL */
    case 0x33: decode(&a,2); decode(&b,1); { u32 v=op_rd(&a,2); op_wr(&b,1,v); SETNZ(v,1); SETV(sext(v,2)!=sext(v,1)); SETC(0);} break; /* CVTWB */
    case 0x32: decode(&a,2); decode(&b,4); { i32 v=sext(op_rd(&a,2),2); op_wr(&b,4,(u32)v); SETNZ(v,4); CLRVC(); } break; /* CVTWL */
    case 0xF6: decode(&a,4); decode(&b,1); { u32 v=op_rd(&a,4); op_wr(&b,1,v); SETNZ(v,1); SETV(sext(v,4)!=sext(v,1)); SETC(0);} break; /* CVTLB */
    case 0xF7: decode(&a,4); decode(&b,2); { u32 v=op_rd(&a,4); op_wr(&b,2,v); SETNZ(v,2); SETV(sext(v,4)!=sext(v,2)); SETC(0);} break; /* CVTLW */

    /* ---- MOVA / PUSHA ---- */
    case 0x9E: decode(&a,1); decode(&b,4); { u32 v=op_addr(&a); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break; /* MOVAB */
    case 0x3E: decode(&a,2); decode(&b,4); { u32 v=op_addr(&a); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break; /* MOVAW */
    case 0xDE: decode(&a,4); decode(&b,4); { u32 v=op_addr(&a); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break; /* MOVAL */
    case 0x7E: decode(&a,8); decode(&b,4); { u32 v=op_addr(&a); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break; /* MOVAQ */
    case 0x9F: decode(&a,1); { u32 v=op_addr(&a); push32(v); SETNZ(v,4); CLRV(); } break; /* PUSHAB */
    case 0x3F: decode(&a,2); { u32 v=op_addr(&a); push32(v); SETNZ(v,4); CLRV(); } break; /* PUSHAW */
    case 0xDF: decode(&a,4); { u32 v=op_addr(&a); push32(v); SETNZ(v,4); CLRV(); } break; /* PUSHAL */
    case 0x7F: decode(&a,8); { u32 v=op_addr(&a); push32(v); SETNZ(v,4); CLRV(); } break; /* PUSHAQ */
    case 0xDD: decode(&a,4); { u32 v=op_rd(&a,4); push32(v); SETNZ(v,4); CLRV(); } break; /* PUSHL */

    /* ---- arithmetic ---- */
    case 0x80: decode(&a,1); decode(&b,1); op_wr(&b,1,do_add(op_rd(&b,1),op_rd(&a,1),1,0)); break;
    case 0xA0: decode(&a,2); decode(&b,2); op_wr(&b,2,do_add(op_rd(&b,2),op_rd(&a,2),2,0)); break;
    case 0xC0: decode(&a,4); decode(&b,4); op_wr(&b,4,do_add(op_rd(&b,4),op_rd(&a,4),4,0)); break;
    case 0x81: decode(&a,1); decode(&b,1); decode(&c,1); op_wr(&c,1,do_add(op_rd(&b,1),op_rd(&a,1),1,0)); break;
    case 0xA1: decode(&a,2); decode(&b,2); decode(&c,2); op_wr(&c,2,do_add(op_rd(&b,2),op_rd(&a,2),2,0)); break;
    case 0xC1: decode(&a,4); decode(&b,4); decode(&c,4); op_wr(&c,4,do_add(op_rd(&b,4),op_rd(&a,4),4,0)); break;
    case 0x82: decode(&a,1); decode(&b,1); op_wr(&b,1,do_sub(op_rd(&b,1),op_rd(&a,1),1,0)); break;
    case 0xA2: decode(&a,2); decode(&b,2); op_wr(&b,2,do_sub(op_rd(&b,2),op_rd(&a,2),2,0)); break;
    case 0xC2: decode(&a,4); decode(&b,4); op_wr(&b,4,do_sub(op_rd(&b,4),op_rd(&a,4),4,0)); break;
    case 0x83: decode(&a,1); decode(&b,1); decode(&c,1); op_wr(&c,1,do_sub(op_rd(&b,1),op_rd(&a,1),1,0)); break;
    case 0xA3: decode(&a,2); decode(&b,2); decode(&c,2); op_wr(&c,2,do_sub(op_rd(&b,2),op_rd(&a,2),2,0)); break;
    case 0xC3: decode(&a,4); decode(&b,4); decode(&c,4); op_wr(&c,4,do_sub(op_rd(&b,4),op_rd(&a,4),4,0)); break;
    case 0xD8: decode(&a,4); decode(&b,4); op_wr(&b,4,do_add(op_rd(&b,4),op_rd(&a,4),4,CC_C)); break; /* ADWC */
    case 0xD9: decode(&a,4); decode(&b,4); op_wr(&b,4,do_sub(op_rd(&b,4),op_rd(&a,4),4,CC_C)); break; /* SBWC */
    case 0x58: decode(&a,2); decode(&b,2); op_wr(&b,2,do_add(op_rd(&b,2),op_rd(&a,2),2,0)); break;    /* ADAWI */

    case 0x96: decode(&a,1); op_wr(&a,1,do_add(op_rd(&a,1),1,1,0)); break;   /* INCB */
    case 0xB6: decode(&a,2); op_wr(&a,2,do_add(op_rd(&a,2),1,2,0)); break;   /* INCW */
    case 0xD6: decode(&a,4); op_wr(&a,4,do_add(op_rd(&a,4),1,4,0)); break;   /* INCL */
    case 0x97: decode(&a,1); op_wr(&a,1,do_sub(op_rd(&a,1),1,1,0)); break;   /* DECB */
    case 0xB7: decode(&a,2); op_wr(&a,2,do_sub(op_rd(&a,2),1,2,0)); break;   /* DECW */
    case 0xD7: decode(&a,4); op_wr(&a,4,do_sub(op_rd(&a,4),1,4,0)); break;   /* DECL */

    case 0xB1: decode(&a,2); decode(&b,2); do_cmp(op_rd(&a,2),op_rd(&b,2),2); break; /* CMPW */
    case 0xD1: decode(&a,4); decode(&b,4); do_cmp(op_rd(&a,4),op_rd(&b,4),4); break; /* CMPL */
    case 0x91: decode(&a,1); decode(&b,1); do_cmp(op_rd(&a,1),op_rd(&b,1),1); break; /* CMPB */
    case 0x95: decode(&a,1); { u32 v=op_rd(&a,1); SETNZ(v,1); CLRVC(); } break;      /* TSTB */
    case 0xB5: decode(&a,2); { u32 v=op_rd(&a,2); SETNZ(v,2); CLRVC(); } break;      /* TSTW */
    case 0xD5: decode(&a,4); { u32 v=op_rd(&a,4); SETNZ(v,4); CLRVC(); } break;      /* TSTL */

    case 0x8A: decode(&a,1); decode(&b,1); { u32 v=op_rd(&b,1)&~op_rd(&a,1); op_wr(&b,1,v); SETNZ(v,1); CLRV(); } break; /* BICB2 */
    case 0xAA: decode(&a,2); decode(&b,2); { u32 v=op_rd(&b,2)&~op_rd(&a,2); op_wr(&b,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xCA: decode(&a,4); decode(&b,4); { u32 v=op_rd(&b,4)&~op_rd(&a,4); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x8B: decode(&a,1); decode(&b,1); decode(&c,1); { u32 v=op_rd(&b,1)&~op_rd(&a,1); op_wr(&c,1,v); SETNZ(v,1); CLRV(); } break;
    case 0xAB: decode(&a,2); decode(&b,2); decode(&c,2); { u32 v=op_rd(&b,2)&~op_rd(&a,2); op_wr(&c,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xCB: decode(&a,4); decode(&b,4); decode(&c,4); { u32 v=op_rd(&b,4)&~op_rd(&a,4); op_wr(&c,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x88: decode(&a,1); decode(&b,1); { u32 v=op_rd(&b,1)|op_rd(&a,1); op_wr(&b,1,v); SETNZ(v,1); CLRV(); } break;  /* BISB2 */
    case 0xA8: decode(&a,2); decode(&b,2); { u32 v=op_rd(&b,2)|op_rd(&a,2); op_wr(&b,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xC8: decode(&a,4); decode(&b,4); { u32 v=op_rd(&b,4)|op_rd(&a,4); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x89: decode(&a,1); decode(&b,1); decode(&c,1); { u32 v=op_rd(&b,1)|op_rd(&a,1); op_wr(&c,1,v); SETNZ(v,1); CLRV(); } break;
    case 0xA9: decode(&a,2); decode(&b,2); decode(&c,2); { u32 v=op_rd(&b,2)|op_rd(&a,2); op_wr(&c,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xC9: decode(&a,4); decode(&b,4); decode(&c,4); { u32 v=op_rd(&b,4)|op_rd(&a,4); op_wr(&c,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x8C: decode(&a,1); decode(&b,1); { u32 v=op_rd(&b,1)^op_rd(&a,1); op_wr(&b,1,v); SETNZ(v,1); CLRV(); } break;  /* XORB2 */
    case 0xAC: decode(&a,2); decode(&b,2); { u32 v=op_rd(&b,2)^op_rd(&a,2); op_wr(&b,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xCC: decode(&a,4); decode(&b,4); { u32 v=op_rd(&b,4)^op_rd(&a,4); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x8D: decode(&a,1); decode(&b,1); decode(&c,1); { u32 v=op_rd(&b,1)^op_rd(&a,1); op_wr(&c,1,v); SETNZ(v,1); CLRV(); } break;
    case 0xAD: decode(&a,2); decode(&b,2); decode(&c,2); { u32 v=op_rd(&b,2)^op_rd(&a,2); op_wr(&c,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xCD: decode(&a,4); decode(&b,4); decode(&c,4); { u32 v=op_rd(&b,4)^op_rd(&a,4); op_wr(&c,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x93: decode(&a,1); decode(&b,1); { u32 v=op_rd(&a,1)&op_rd(&b,1); SETNZ(v,1); CLRV(); } break;  /* BITB */
    case 0xB3: decode(&a,2); decode(&b,2); { u32 v=op_rd(&a,2)&op_rd(&b,2); SETNZ(v,2); CLRV(); } break;
    case 0xD3: decode(&a,4); decode(&b,4); { u32 v=op_rd(&a,4)&op_rd(&b,4); SETNZ(v,4); CLRV(); } break;
    case 0x92: decode(&a,1); decode(&b,1); { u32 v=~op_rd(&a,1); op_wr(&b,1,v); SETNZ(v,1); CLRV(); } break; /* MCOMB */
    case 0xB2: decode(&a,2); decode(&b,2); { u32 v=~op_rd(&a,2); op_wr(&b,2,v); SETNZ(v,2); CLRV(); } break;
    case 0xD2: decode(&a,4); decode(&b,4); { u32 v=~op_rd(&a,4); op_wr(&b,4,v); SETNZ(v,4); CLRV(); } break;
    case 0x8E: decode(&a,1); decode(&b,1); op_wr(&b,1,do_sub(0,op_rd(&a,1),1,0)); break;  /* MNEGB */
    case 0xAE: decode(&a,2); decode(&b,2); op_wr(&b,2,do_sub(0,op_rd(&a,2),2,0)); break;
    case 0xCE: decode(&a,4); decode(&b,4); op_wr(&b,4,do_sub(0,op_rd(&a,4),4,0)); break;

    /* ---- multiply / divide ---- */
    case 0x84: case 0x85: case 0xA4: case 0xA5: case 0xC4: case 0xC5: {  /* MULx2 / MULx3 */
        int sz = (op == 0x84 || op == 0x85) ? 1 : (op == 0xA4 || op == 0xA5) ? 2 : 4;
        int three = (op == 0x85 || op == 0xA5 || op == 0xC5);
        i64 r; u32 res;
        decode(&a, sz); decode(&b, sz);
        r = (i64)sext(op_rd(&a,sz),sz) * (i64)sext(op_rd(&b,sz),sz);
        res = masksz((u32)r, sz);
        if (three) { decode(&c, sz); op_wr(&c, sz, res); } else op_wr(&b, sz, res);
        SETNZ(res, sz); SETV(r != (i64)sext(res, sz)); SETC(0);
        break; }
    case 0x86: case 0xA6: case 0xC6: case 0x87: case 0xA7: case 0xC7: { /* DIVx2/3 */
        int sz = (op == 0x86 || op == 0x87) ? 1 : (op == 0xA6 || op == 0xA7) ? 2 : 4;
        int three = (op == 0x87 || op == 0xA7 || op == 0xC7);
        i32 dv, dd, res;
        decode(&a, sz); decode(&b, sz);
        dv = sext(op_rd(&a,sz),sz); dd = sext(op_rd(&b,sz),sz);
        if (dv == 0) { res = dd; SETV(1); }
        else { res = dd / dv; SETV(0); }
        if (three) { decode(&c, sz); op_wr(&c, sz, (u32)res); } else op_wr(&b, sz, (u32)res);
        SETNZ((u32)res, sz); SETC(0);
        break; }
    case 0x7A: {                                        /* EMUL */
        i32 m1, m2; i32 add; i64 r;
        decode(&a,4); decode(&b,4); decode(&c,4); decode(&d,8);
        m1 = (i32)op_rd(&a,4); m2 = (i32)op_rd(&b,4); add = (i32)op_rd(&c,4);
        r = (i64)m1 * (i64)m2 + (i64)add;
        op_wrq(&d, (u64)r);
        cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
        if (r < 0) cpu.psl |= PSL_N; if (r == 0) cpu.psl |= PSL_Z;
        break; }
    case 0x7B: {                                        /* EDIV */
        i32 dvr; i64 dvd; i32 q = 0, rem = 0; int ovf = 0;
        decode(&a,4); decode(&b,8); decode(&c,4); decode(&d,4);
        dvr = (i32)op_rd(&a,4); dvd = (i64)op_rdq(&b);
        if (dvr == 0) { ovf = 1; q = (i32)dvd; rem = 0; }
        else {
            i64 qq = dvd / dvr;
            rem = (i32)(dvd % dvr);
            if (qq > 2147483647LL || qq < -2147483648LL) { ovf = 1; q = (i32)dvd; }
            else q = (i32)qq;
        }
        op_wr(&c,4,(u32)q); op_wr(&d,4,(u32)rem);
        SETNZ((u32)q,4); SETV(ovf); SETC(0);
        break; }

    /* ---- shifts ---- */
    case 0x78: {                                        /* ASHL */
        i32 cnt; u32 src; i64 r;
        decode(&a,1); decode(&b,4); decode(&c,4);
        cnt = sext(op_rd(&a,1),1); src = op_rd(&b,4);
        if (cnt >= 0) { r = (cnt >= 32) ? 0 : ((i64)(i32)src << cnt); }
        else          { r = (cnt <= -32) ? (((i32)src < 0) ? -1 : 0) : ((i32)src >> (-cnt)); }
        op_wr(&c,4,(u32)r);
        SETNZ((u32)r,4); SETV(cnt > 0 && (i64)(i32)(u32)r != r); SETC(0);
        break; }
    case 0x79: {                                        /* ASHQ */
        i32 cnt; i64 src, r;
        decode(&a,1); decode(&b,8); decode(&c,8);
        cnt = sext(op_rd(&a,1),1); src = (i64)op_rdq(&b);
        if (cnt >= 0) r = (cnt >= 64) ? 0 : (src << cnt);
        else          r = (cnt <= -64) ? ((src < 0) ? -1 : 0) : (src >> (-cnt));
        op_wrq(&c,(u64)r);
        cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
        if (r < 0) cpu.psl |= PSL_N; if (r == 0) cpu.psl |= PSL_Z;
        break; }
    case 0x9C: {                                        /* ROTL */
        i32 cnt; u32 src, r;
        decode(&a,1); decode(&b,4); decode(&c,4);
        cnt = sext(op_rd(&a,1),1) & 31; src = op_rd(&b,4);
        r = cnt ? ((src << cnt) | (src >> (32 - cnt))) : src;
        op_wr(&c,4,r); SETNZ(r,4); CLRV();
        break; }

    /* ---- bit fields ---- */
    case 0xEE: case 0xEF: {                             /* EXTV / EXTZV */
        i32 pos; int sz; u32 v;
        decode(&a,4); decode(&b,1); decode(&c,1); decode(&d,4);
        pos = (i32)op_rd(&a,4); sz = fldsize(op_rd(&b,1));
        v = field_read(&c, pos, sz, op == 0xEE);
        op_wr(&d,4,v); SETNZ(v,4); CLRVC();
        break; }
    case 0xF0: {                                        /* INSV */
        i32 pos; int sz; u32 v;
        decode(&a,4); decode(&b,4); decode(&c,1); decode(&d,1);
        v = op_rd(&a,4); pos = (i32)op_rd(&b,4); sz = fldsize(op_rd(&c,1));
        field_write(&d, pos, sz, v);
        break; }
    case 0xEC: case 0xED: {                             /* CMPV / CMPZV */
        i32 pos; int sz; u32 v;
        decode(&a,4); decode(&b,1); decode(&c,1); decode(&d,4);
        pos = (i32)op_rd(&a,4); sz = fldsize(op_rd(&b,1));
        v = field_read(&c, pos, sz, op == 0xEC);
        do_cmp(v, op_rd(&d,4), 4);
        break; }
    case 0xEA: case 0xEB: {                             /* FFS / FFC */
        i32 pos; int sz, i; u32 v; int found = 0;
        decode(&a,4); decode(&b,1); decode(&c,1); decode(&d,4);
        pos = (i32)op_rd(&a,4); sz = fldsize(op_rd(&b,1));
        v = field_read(&c, pos, sz, 0);
        if (op == 0xEB) v = ~v;
        for (i = 0; i < sz; i++) if (v & (1u << i)) { found = 1; break; }
        op_wr(&d, 4, found ? (u32)(pos + i) : (u32)(pos + sz));
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C|PSL_Z);
        if (!found) cpu.psl |= PSL_Z;
        break; }

    /* ---- PUSHR / POPR ---- */
    case 0xBB: { u16 m; int i; decode(&a,2); m = (u16)op_rd(&a,2);
                 for (i = 14; i >= 0; i--) if (m & (1u << i)) push32(cpu.r[i]); break; }
    case 0xBA: { u16 m; int i; decode(&a,2); m = (u16)op_rd(&a,2);
                 for (i = 0; i <= 14; i++) if (m & (1u << i)) cpu.r[i] = pop32(); break; }

    /* ---- PSL access ---- */
    case 0xDC: decode(&a,4); op_wr(&a,4,cpu.psl); break;                    /* MOVPSL */
    case 0xB8: decode(&a,2); cpu.psl |= (op_rd(&a,2) & 0xFFFF); break;      /* BISPSW */
    case 0xB9: decode(&a,2); cpu.psl &= ~(op_rd(&a,2) & 0xFFFF); break;     /* BICPSW */

    /* ---- INDEX (Pascal/BASIC subscript check) ---- */
    case 0x0A: {
        i32 sub, low, high, sz2, idxin; i32 res;
        decode(&a,4); decode(&b,4); decode(&c,4); decode(&d,4); decode(&e,4); decode(&f,4);
        sub = (i32)op_rd(&a,4); low = (i32)op_rd(&b,4); high = (i32)op_rd(&c,4);
        sz2 = (i32)op_rd(&d,4); idxin = (i32)op_rd(&e,4);
        /* indexout <- (indexin + subscript) * size.  low and high take no part
         * in the arithmetic; they only bound the subscript range check. */
        res = (idxin + sub) * sz2;
        op_wr(&f,4,(u32)res);
        SETNZ((u32)res,4); CLRVC();
        if ((sub < low || sub > high) && trace_level)
            fprintf(stderr, "INDEX out of range: %d not in [%d,%d] at %08X\n",
                    sub, low, high, pc0);
        break; }

    /* ---- PROBER / PROBEW: report the page as accessible ---- */
    case 0x0C: case 0x0D:
        decode(&a,1); decode(&b,2); decode(&c,1);
        cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
        break;

    /* ---- character strings ---- */
    case 0x28: {                                        /* MOVC3 */
        u32 len, s, dsp, i;
        decode(&a,2); decode(&b,1); decode(&c,1);
        len = op_rd(&a,2) & 0xFFFF; s = op_addr(&b); dsp = op_addr(&c);
        if (dsp > s) for (i = len; i-- > 0;) wr8(dsp+i, rd8(s+i));
        else         for (i = 0; i < len; i++) wr8(dsp+i, rd8(s+i));
        cpu.r[0]=0; cpu.r[1]=s+len; cpu.r[2]=0; cpu.r[3]=dsp+len; cpu.r[4]=0; cpu.r[5]=0;
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C); cpu.psl |= PSL_Z;
        break; }
    case 0x2C: {                                        /* MOVC5 */
        u32 slen, s, fill, dlen, dsp, i, n;
        decode(&a,2); decode(&b,1); decode(&c,1); decode(&d,2); decode(&e,1);
        slen = op_rd(&a,2)&0xFFFF; s = op_addr(&b); fill = op_rd(&c,1)&0xFF;
        dlen = op_rd(&d,2)&0xFFFF; dsp = op_addr(&e);
        n = slen < dlen ? slen : dlen;
        if (dsp > s) { for (i = n; i-- > 0;) wr8(dsp+i, rd8(s+i)); }
        else         { for (i = 0; i < n; i++) wr8(dsp+i, rd8(s+i)); }
        for (i = n; i < dlen; i++) wr8(dsp+i, (u8)fill);
        do_cmp(slen, dlen, 2);
        cpu.r[0]= slen > dlen ? slen-dlen : 0; cpu.r[1]=s+n;
        cpu.r[2]=0; cpu.r[3]=dsp+dlen; cpu.r[4]=0; cpu.r[5]=0;
        break; }
    case 0x29: case 0x2D: {                             /* CMPC3 / CMPC5 */
        u32 l1,s1,l2,s2,i,n; u8 fill = 0; u8 c1=0,c2=0;
        if (op == 0x29) {
            decode(&a,2); decode(&b,1); decode(&c,1);
            l1 = op_rd(&a,2)&0xFFFF; s1 = op_addr(&b); s2 = op_addr(&c); l2 = l1;
        } else {
            decode(&a,2); decode(&b,1); decode(&c,1); decode(&d,2); decode(&e,1);
            l1 = op_rd(&a,2)&0xFFFF; s1 = op_addr(&b); fill = (u8)op_rd(&c,1);
            l2 = op_rd(&d,2)&0xFFFF; s2 = op_addr(&e);
        }
        n = l1 > l2 ? l1 : l2;
        for (i = 0; i < n; i++) {
            c1 = i < l1 ? rd8(s1+i) : fill;
            c2 = i < l2 ? rd8(s2+i) : fill;
            if (c1 != c2) break;
        }
        do_cmp(c1, c2, 1);
        cpu.r[0] = i < l1 ? l1-i : 0; cpu.r[1] = s1 + (i < l1 ? i : l1);
        cpu.r[2] = i < l2 ? l2-i : 0; cpu.r[3] = s2 + (i < l2 ? i : l2);
        break; }
    case 0x3A: {                                        /* LOCC */
        u32 ch, len, s, i;
        decode(&a,1); decode(&b,2); decode(&c,1);
        ch = op_rd(&a,1)&0xFF; len = op_rd(&b,2)&0xFFFF; s = op_addr(&c);
        for (i = 0; i < len; i++) if (rd8(s+i) == (u8)ch) break;
        cpu.r[0] = len - i; cpu.r[1] = s + i;
        cpu.psl &= ~PSL_Z; if (i == len) cpu.psl |= PSL_Z;
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C);
        break; }
    case 0x3B: {                                        /* SKPC */
        u32 ch, len, s, i;
        decode(&a,1); decode(&b,2); decode(&c,1);
        ch = op_rd(&a,1)&0xFF; len = op_rd(&b,2)&0xFFFF; s = op_addr(&c);
        for (i = 0; i < len; i++) if (rd8(s+i) != (u8)ch) break;
        cpu.r[0] = len - i; cpu.r[1] = s + i;
        cpu.psl &= ~PSL_Z; if (i == len) cpu.psl |= PSL_Z;
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C);
        break; }
    case 0x2A: case 0x2B: {                             /* SCANC / SPANC */
        u32 len, s, tbl, msk, i;
        decode(&a,2); decode(&b,1); decode(&c,1); decode(&d,1);
        len = op_rd(&a,2)&0xFFFF; s = op_addr(&b); tbl = op_addr(&c); msk = op_rd(&d,1)&0xFF;
        for (i = 0; i < len; i++) {
            u32 t = rd8(tbl + rd8(s+i)) & msk;
            if (op == 0x2A ? (t != 0) : (t == 0)) break;
        }
        cpu.r[0] = len - i; cpu.r[1] = s + i; cpu.r[2] = 0; cpu.r[3] = tbl;
        cpu.psl &= ~PSL_Z; if (i == len) cpu.psl |= PSL_Z;
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C);
        break; }
    case 0x39: {                                        /* MATCHC */
        u32 ol, os, l, s, i, j;
        decode(&a,2); decode(&b,1); decode(&c,2); decode(&d,1);
        ol = op_rd(&a,2)&0xFFFF; os = op_addr(&b);
        l  = op_rd(&c,2)&0xFFFF; s  = op_addr(&d);
        for (i = 0; ol <= l && i + ol <= l; i++) {
            for (j = 0; j < ol; j++) if (rd8(os+j) != rd8(s+i+j)) break;
            if (j == ol) break;
        }
        if (ol && i + ol <= l) {
            cpu.r[0]=0; cpu.r[1]=os+ol; cpu.r[2]=l-i-ol; cpu.r[3]=s+i+ol;
            cpu.psl |= PSL_Z;
        } else {
            cpu.r[0]=ol; cpu.r[1]=os; cpu.r[2]=0; cpu.r[3]=s+l;
            cpu.psl &= ~PSL_Z;
        }
        cpu.psl &= ~(PSL_N|PSL_V|PSL_C);
        break; }
    case 0x2E: case 0x2F: {                             /* MOVTC / MOVTUC */
        u32 sl, ss, fill, tbl, dl, ds, i, n;
        decode(&a,2); decode(&b,1); decode(&c,1); decode(&d,1); decode(&e,2); decode(&f,1);
        sl = op_rd(&a,2)&0xFFFF; ss = op_addr(&b); fill = op_rd(&c,1)&0xFF;
        tbl = op_addr(&d); dl = op_rd(&e,2)&0xFFFF; ds = op_addr(&f);
        n = sl < dl ? sl : dl;
        for (i = 0; i < n; i++) wr8(ds+i, rd8(tbl + rd8(ss+i)));
        for (i = n; i < dl; i++) wr8(ds+i, (u8)fill);
        do_cmp(sl, dl, 2);
        cpu.r[0] = sl > dl ? sl-dl : 0; cpu.r[1] = ss+n;
        cpu.r[2] = 0; cpu.r[3] = tbl; cpu.r[4] = 0; cpu.r[5] = ds+dl;
        break; }

    /* ---- queues ---- */
    case 0x0E: {                                        /* INSQUE */
        u32 ent, pred, succ;
        decode(&a,1); decode(&b,1);
        ent = op_addr(&a); pred = op_addr(&b);
        succ = rd32(pred);
        wr32(ent, succ); wr32(ent+4, pred);
        wr32(pred, ent); wr32(succ+4, ent);
        cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
        if (succ == pred) cpu.psl |= PSL_Z;
        break; }
    case 0x0F: {                                        /* REMQUE */
        u32 ent, pred, succ;
        decode(&a,1); decode(&b,4);
        ent = op_addr(&a);
        succ = rd32(ent); pred = rd32(ent+4);
        cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
        if (succ == ent) {          /* entry is its own successor: queue empty */
            cpu.psl |= PSL_V | PSL_Z;
            break;                  /* nothing removed, destination untouched */
        }
        wr32(pred, succ); wr32(succ+4, pred);
        op_wr(&b,4,ent);
        if (succ == pred) cpu.psl |= PSL_Z;
        break; }

    /* ---- self-relative (interlocked) queues ---- */
    case 0x5C: case 0x5D: {                             /* INSQHI / INSQTI */
        u32 ent, hdr, fwd, bwd;
        int head = (op == 0x5C);
        decode(&a, 1); decode(&b, 8);
        ent = op_addr(&a); hdr = op_addr(&b);
        fwd = rd32(hdr); bwd = rd32(hdr + 4);
        cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
        if (head) {
            u32 first = hdr + fwd;
            wr32(ent,     (fwd ? first : hdr) - ent);
            wr32(ent + 4, hdr - ent);
            wr32(hdr, ent - hdr);
            if (!fwd) wr32(hdr + 4, ent - hdr);
            else      wr32(first + 4, ent - first);
        } else {
            u32 last = hdr + bwd;
            wr32(ent,     hdr - ent);
            wr32(ent + 4, (bwd ? last : hdr) - ent);
            wr32(hdr + 4, ent - hdr);
            if (!bwd) wr32(hdr, ent - hdr);
            else      wr32(last, ent - last);
        }
        if (!fwd && !bwd) cpu.psl |= PSL_Z;             /* the queue had been empty */
        break; }
    case 0x5E: case 0x5F: {                             /* REMQHI / REMQTI */
        u32 hdr, link, ent, adj;
        int head = (op == 0x5E);
        decode(&a, 8); decode(&b, 4);
        hdr = op_addr(&a);
        link = head ? rd32(hdr) : rd32(hdr + 4);
        cpu.psl &= ~(PSL_N | PSL_Z | PSL_V | PSL_C);
        if (!link) {                                    /* empty: nothing removed */
            cpu.psl |= PSL_V | PSL_Z;
            break;
        }
        ent = hdr + link;
        adj = head ? ent + rd32(ent) : ent + rd32(ent + 4);
        if (adj == hdr) {                               /* queue becomes empty */
            wr32(hdr, 0); wr32(hdr + 4, 0);
            cpu.psl |= PSL_Z;
        } else if (head) {
            wr32(hdr, adj - hdr);
            wr32(adj + 4, hdr - adj);
        } else {
            wr32(hdr + 4, adj - hdr);
            wr32(adj, hdr - adj);
        }
        op_wr(&b, 4, ent);
        break; }

    /* ---- floating point: F ---- */
    case 0x50: decode(&a,4); decode(&b,4); { double v=op_rdf(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break; /* MOVF */
    case 0x52: decode(&a,4); decode(&b,4); { double v=-op_rdf(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;/* MNEGF */
    case 0x51: decode(&a,4); decode(&b,4); { double x=op_rdf(&a),y=op_rdf(&b);
                 cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
                 if (x<y) cpu.psl|=PSL_N; if (x==y) cpu.psl|=PSL_Z; } break;                  /* CMPF */
    case 0x53: decode(&a,4); { double v=op_rdf(&a); setnz_d(v);} break;                        /* TSTF */
    case 0x40: decode(&a,4); decode(&b,4); { double v=op_rdf(&b)+op_rdf(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;
    case 0x41: decode(&a,4); decode(&b,4); decode(&c,4); { double v=op_rdf(&b)+op_rdf(&a); op_wr(&c,4,d_to_f(v)); setnz_d(v);} break;
    case 0x42: decode(&a,4); decode(&b,4); { double v=op_rdf(&b)-op_rdf(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;
    case 0x43: decode(&a,4); decode(&b,4); decode(&c,4); { double v=op_rdf(&b)-op_rdf(&a); op_wr(&c,4,d_to_f(v)); setnz_d(v);} break;
    case 0x44: decode(&a,4); decode(&b,4); { double v=op_rdf(&b)*op_rdf(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;
    case 0x45: decode(&a,4); decode(&b,4); decode(&c,4); { double v=op_rdf(&b)*op_rdf(&a); op_wr(&c,4,d_to_f(v)); setnz_d(v);} break;
    case 0x46: decode(&a,4); decode(&b,4); { double x=op_rdf(&a),v; if(x==0) vax_fatal("DIVF by zero at %08X",pc0); v=op_rdf(&b)/x; op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;
    case 0x47: decode(&a,4); decode(&b,4); decode(&c,4); { double x=op_rdf(&a),v; if(x==0) vax_fatal("DIVF3 by zero at %08X",pc0); v=op_rdf(&b)/x; op_wr(&c,4,d_to_f(v)); setnz_d(v);} break;
    case 0x4C: decode(&a,1); decode(&b,4); { double v=(double)sext(op_rd(&a,1),1); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break; /* CVTBF */
    case 0x4D: decode(&a,2); decode(&b,4); { double v=(double)sext(op_rd(&a,2),2); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break; /* CVTWF */
    case 0x4E: decode(&a,4); decode(&b,4); { double v=(double)(i32)op_rd(&a,4); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;    /* CVTLF */
    case 0x48: decode(&a,4); decode(&b,1); { i32 v=(i32)op_rdf(&a); op_wr(&b,1,(u32)v); SETNZ((u32)v,1); } break;   /* CVTFB */
    case 0x49: decode(&a,4); decode(&b,2); { i32 v=(i32)op_rdf(&a); op_wr(&b,2,(u32)v); SETNZ((u32)v,2); } break;   /* CVTFW */
    case 0x4A: decode(&a,4); decode(&b,4); { i32 v=(i32)op_rdf(&a); op_wr(&b,4,(u32)v); SETNZ((u32)v,4); } break;   /* CVTFL */
    case 0x4B: decode(&a,4); decode(&b,4); { double x=op_rdf(&a); i32 v=(i32)(x<0?x-0.5:x+0.5); op_wr(&b,4,(u32)v); SETNZ((u32)v,4);} break; /* CVTRFL */
    case 0x56: decode(&a,4); decode(&b,8); { double v=op_rdf(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;       /* CVTFD */

    /* ---- floating point: D ---- */
    case 0x70: decode(&a,8); decode(&b,8); { double v=op_rdd(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break; /* MOVD */
    case 0x72: decode(&a,8); decode(&b,8); { double v=-op_rdd(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;/* MNEGD */
    case 0x71: decode(&a,8); decode(&b,8); { double x=op_rdd(&a),y=op_rdd(&b);
                 cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
                 if (x<y) cpu.psl|=PSL_N; if (x==y) cpu.psl|=PSL_Z; } break;
    case 0x73: decode(&a,8); { double v=op_rdd(&a); setnz_d(v);} break;
    case 0x60: decode(&a,8); decode(&b,8); { double v=op_rdd(&b)+op_rdd(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x61: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdd(&b)+op_rdd(&a); op_wrq(&c,d_to_dfl(v)); setnz_d(v);} break;
    case 0x62: decode(&a,8); decode(&b,8); { double v=op_rdd(&b)-op_rdd(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x63: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdd(&b)-op_rdd(&a); op_wrq(&c,d_to_dfl(v)); setnz_d(v);} break;
    case 0x64: decode(&a,8); decode(&b,8); { double v=op_rdd(&b)*op_rdd(&a); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x65: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdd(&b)*op_rdd(&a); op_wrq(&c,d_to_dfl(v)); setnz_d(v);} break;
    case 0x66: decode(&a,8); decode(&b,8); { double x=op_rdd(&a),v; if(x==0) vax_fatal("DIVD by zero at %08X",pc0); v=op_rdd(&b)/x; op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x67: decode(&a,8); decode(&b,8); decode(&c,8); { double x=op_rdd(&a),v; if(x==0) vax_fatal("DIVD3 by zero at %08X",pc0); v=op_rdd(&b)/x; op_wrq(&c,d_to_dfl(v)); setnz_d(v);} break;
    case 0x6C: decode(&a,1); decode(&b,8); { double v=(double)sext(op_rd(&a,1),1); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x6D: decode(&a,2); decode(&b,8); { double v=(double)sext(op_rd(&a,2),2); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x6E: decode(&a,4); decode(&b,8); { double v=(double)(i32)op_rd(&a,4); op_wrq(&b,d_to_dfl(v)); setnz_d(v);} break;
    case 0x68: decode(&a,8); decode(&b,1); { i32 v=(i32)op_rdd(&a); op_wr(&b,1,(u32)v); SETNZ((u32)v,1);} break;
    case 0x69: decode(&a,8); decode(&b,2); { i32 v=(i32)op_rdd(&a); op_wr(&b,2,(u32)v); SETNZ((u32)v,2);} break;
    case 0x6A: decode(&a,8); decode(&b,4); { i32 v=(i32)op_rdd(&a); op_wr(&b,4,(u32)v); SETNZ((u32)v,4);} break;
    case 0x6B: decode(&a,8); decode(&b,4); { double x=op_rdd(&a); i32 v=(i32)(x<0?x-0.5:x+0.5); op_wr(&b,4,(u32)v); SETNZ((u32)v,4);} break;
    case 0x76: decode(&a,8); decode(&b,4); { double v=op_rdd(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;  /* CVTDF */

    /* ---- G floating (MTH$ uses these) ---- */
    case 0x150: decode(&a,8); decode(&b,8); { double v=op_rdg(&a); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;  /* MOVG */
    case 0x151: decode(&a,8); decode(&b,8); { double x=op_rdg(&a),y=op_rdg(&b);
                  cpu.psl &= ~(PSL_N|PSL_Z|PSL_V|PSL_C);
                  if (x<y) cpu.psl|=PSL_N; if (x==y) cpu.psl|=PSL_Z; } break;
    case 0x153: decode(&a,8); { double v=op_rdg(&a); setnz_d(v);} break;
    case 0x140: decode(&a,8); decode(&b,8); { double v=op_rdg(&b)+op_rdg(&a); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x141: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdg(&b)+op_rdg(&a); op_wrq(&c,d_to_g(v)); setnz_d(v);} break;
    case 0x142: decode(&a,8); decode(&b,8); { double v=op_rdg(&b)-op_rdg(&a); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x143: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdg(&b)-op_rdg(&a); op_wrq(&c,d_to_g(v)); setnz_d(v);} break;
    case 0x144: decode(&a,8); decode(&b,8); { double v=op_rdg(&b)*op_rdg(&a); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x145: decode(&a,8); decode(&b,8); decode(&c,8); { double v=op_rdg(&b)*op_rdg(&a); op_wrq(&c,d_to_g(v)); setnz_d(v);} break;
    case 0x146: decode(&a,8); decode(&b,8); { double x=op_rdg(&a),v; if(x==0) vax_fatal("DIVG by zero"); v=op_rdg(&b)/x; op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x147: decode(&a,8); decode(&b,8); decode(&c,8); { double x=op_rdg(&a),v; if(x==0) vax_fatal("DIVG3 by zero"); v=op_rdg(&b)/x; op_wrq(&c,d_to_g(v)); setnz_d(v);} break;
    case 0x14C: decode(&a,1); decode(&b,8); { double v=(double)sext(op_rd(&a,1),1); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x14D: decode(&a,2); decode(&b,8); { double v=(double)sext(op_rd(&a,2),2); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x14E: decode(&a,4); decode(&b,8); { double v=(double)(i32)op_rd(&a,4); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;
    case 0x148: decode(&a,8); decode(&b,1); { i32 v=(i32)op_rdg(&a); op_wr(&b,1,(u32)v); SETNZ((u32)v,1);} break;
    case 0x149: decode(&a,8); decode(&b,2); { i32 v=(i32)op_rdg(&a); op_wr(&b,2,(u32)v); SETNZ((u32)v,2);} break;
    case 0x14A: decode(&a,8); decode(&b,4); { i32 v=(i32)op_rdg(&a); op_wr(&b,4,(u32)v); SETNZ((u32)v,4);} break;
    case 0x14B: decode(&a,8); decode(&b,4); { double x=op_rdg(&a); i32 v=(i32)(x<0?x-0.5:x+0.5); op_wr(&b,4,(u32)v); SETNZ((u32)v,4);} break;
    case 0x133: decode(&a,4); decode(&b,8); { double v=op_rdf(&a); op_wrq(&b,d_to_g(v)); setnz_d(v);} break;  /* CVTFG */
    case 0x156: decode(&a,8); decode(&b,4); { double v=op_rdg(&a); op_wr(&b,4,d_to_f(v)); setnz_d(v);} break;  /* CVTGF */

    /* ---- ACB (add compare and branch) ---- */
    case 0x9D: case 0x3D: case 0xF1: {                  /* ACBB / ACBW / ACBL */
        int sz = (op == 0x9D) ? 1 : (op == 0x3D) ? 2 : 4;
        i32 lim, add, idx;
        decode(&a,sz); decode(&b,sz); decode(&c,sz);
        lim = sext(op_rd(&a,sz),sz); add = sext(op_rd(&b,sz),sz);
        idx = sext(op_rd(&c,sz),sz) + add;
        op_wr(&c,sz,(u32)idx);
        SETNZ((u32)idx,sz); CLRV();
        branch16(add >= 0 ? idx <= lim : idx >= lim);
        break; }
    case 0x4F: {                                        /* ACBF */
        double lim, add, idx;
        decode(&a,4); decode(&b,4); decode(&c,4);
        lim = op_rdf(&a); add = op_rdf(&b); idx = op_rdf(&c) + add;
        op_wr(&c,4,d_to_f(idx)); setnz_d(idx);
        branch16(add >= 0 ? idx <= lim : idx >= lim);
        break; }
    case 0x6F: {                                        /* ACBD */
        double lim, add, idx;
        decode(&a,8); decode(&b,8); decode(&c,8);
        lim = op_rdd(&a); add = op_rdd(&b); idx = op_rdd(&c) + add;
        op_wrq(&c,d_to_dfl(idx)); setnz_d(idx);
        branch16(add >= 0 ? idx <= lim : idx >= lim);
        break; }

    /* ---- POLY (used by MTH$) ---- */
    case 0x55: case 0x75: {                             /* POLYF / POLYD */
        int isd = (op == 0x75);
        double x, r = 0; u32 deg, tbl; u32 i;
        if (isd) { decode(&a,8); decode(&b,2); decode(&c,1); x = op_rdd(&a); }
        else     { decode(&a,4); decode(&b,2); decode(&c,1); x = op_rdf(&a); }
        deg = op_rd(&b,2) & 0xFF; tbl = op_addr(&c);
        for (i = 0; i <= deg; i++) {
            double cf = isd ? dfl_to_d(rd64(tbl + i*8)) : f_to_d(rd32(tbl + i*4));
            r = r * x + cf;
        }
        if (isd) { cpu.r[0]=(u32)d_to_dfl(r); cpu.r[1]=(u32)(d_to_dfl(r)>>32); }
        else       cpu.r[0]=d_to_f(r);
        cpu.r[2]=0; cpu.r[3]=tbl + (deg+1)*(isd?8:4);
        setnz_d(r);
        break; }

    /* ---- change mode: hand to the VMS shim ---- */
    case 0xBC: case 0xBD: case 0xBE: case 0xBF: {       /* CHMK CHME CHMS CHMU */
        decode(&a,4);
        vax_fatal("CHM%c #%u at %08X (system service call not via vector)",
                  "KESU"[op-0xBC], op_rd(&a,4), pc0);
        break; }

    case 0x03: vax_fatal("BPT at %08X", pc0);
    case 0x06: vax_fatal("LDPCTX at %08X", pc0);
    case 0xFC: vax_fatal("XFC at %08X", pc0);

    default:
        unimpl(op, pc0);
    }
}

void cpu_run(void)
{
    while (!cpu.halted) cpu_step();
}
