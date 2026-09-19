/* Z80 CPU core for the Wang OIS 928 workstation emulation.
   Documented flag behaviour throughout; the undocumented X/Y bits are
   maintained as well, though the original code never depends on them. */
#include "z80.h"

static Z80 *z;

static uint8_t parity_tab[256];
static int parity_init = 0;

static void init_parity(void)
{
    int i, j, p;
    for (i = 0; i < 256; i++) {
        p = 0;
        for (j = 0; j < 8; j++) p ^= (i >> j) & 1;
        parity_tab[i] = p ? 0 : FLAG_P;
    }
    parity_init = 1;
}

void z80_reset(Z80 *cpu)
{
    int i;
    uint8_t *p = (uint8_t *)cpu;
    for (i = 0; i < (int)sizeof(Z80); i++) p[i] = 0;
    if (!parity_init) init_parity();
}

#define BC ((uint16_t)((z->b << 8) | z->c))
#define DE ((uint16_t)((z->d << 8) | z->e))
#define HL ((uint16_t)((z->h << 8) | z->l))
#define AF ((uint16_t)((z->a << 8) | z->f))
static void set_bc(uint16_t v) { z->b = v >> 8; z->c = v & 0xFF; }
static void set_de(uint16_t v) { z->d = v >> 8; z->e = v & 0xFF; }
static void set_hl(uint16_t v) { z->h = v >> 8; z->l = v & 0xFF; }
static void set_af(uint16_t v) { z->a = v >> 8; z->f = v & 0xFF; }

static uint8_t fetch(void)    { return mem_rd(z->pc++); }
static uint16_t fetch16(void) { uint16_t v = fetch(); v |= (uint16_t)fetch() << 8; return v; }

static void push16(uint16_t v)
{
    z->sp -= 2;
    mem_wr(z->sp, (uint8_t)(v & 0xFF));
    mem_wr((uint16_t)(z->sp + 1), (uint8_t)(v >> 8));
}

static uint16_t pop16(void)
{
    uint16_t v = mem_rd(z->sp) | ((uint16_t)mem_rd((uint16_t)(z->sp + 1)) << 8);
    z->sp += 2;
    return v;
}

static void set_sz53p(uint8_t v)
{
    z->f = (uint8_t)((z->f & FLAG_C) | (v & 0xA8) | (v ? 0 : FLAG_Z) | parity_tab[v]);
}

static void alu_add(uint8_t v, int carry)
{
    unsigned c = carry ? (unsigned)(z->f & FLAG_C) : 0u;
    unsigned r = (unsigned)z->a + v + c;
    unsigned hc = (unsigned)(z->a & 0x0F) + (v & 0x0F) + c;
    uint8_t res = (uint8_t)r;
    z->f = (uint8_t)((res & 0xA8) | (res ? 0 : FLAG_Z) | ((hc & 0x10) ? FLAG_H : 0)
         | (((z->a ^ ~v) & (z->a ^ res) & 0x80) ? FLAG_P : 0)
         | ((r & 0x100) ? FLAG_C : 0));
    z->a = res;
}

static void alu_sub(uint8_t v, int carry, int store)
{
    unsigned c = carry ? (unsigned)(z->f & FLAG_C) : 0u;
    unsigned r = (unsigned)z->a - v - c;
    unsigned hc = (unsigned)(z->a & 0x0F) - (v & 0x0F) - c;
    uint8_t res = (uint8_t)r;
    uint8_t nf = (uint8_t)((res & 0x80) | (res ? 0 : FLAG_Z) | ((hc & 0x10) ? FLAG_H : 0)
         | (((z->a ^ v) & (z->a ^ res) & 0x80) ? FLAG_P : 0)
         | ((r & 0x100) ? FLAG_C : 0) | FLAG_N);
    nf |= store ? (uint8_t)(res & 0x28) : (uint8_t)(v & 0x28);
    z->f = nf;
    if (store) z->a = res;
}

static void alu_and(uint8_t v) { z->a &= v; set_sz53p(z->a); z->f |= FLAG_H; }
static void alu_xor(uint8_t v) { z->a ^= v; set_sz53p(z->a); }
static void alu_or (uint8_t v) { z->a |= v; set_sz53p(z->a); }

static uint8_t alu_inc(uint8_t v)
{
    uint8_t r = (uint8_t)(v + 1);
    z->f = (uint8_t)((z->f & FLAG_C) | (r & 0xA8) | (r ? 0 : FLAG_Z)
         | (((v & 0x0F) == 0x0F) ? FLAG_H : 0) | ((r == 0x80) ? FLAG_P : 0));
    return r;
}

static uint8_t alu_dec(uint8_t v)
{
    uint8_t r = (uint8_t)(v - 1);
    z->f = (uint8_t)((z->f & FLAG_C) | (r & 0xA8) | (r ? 0 : FLAG_Z)
         | (((v & 0x0F) == 0x00) ? FLAG_H : 0) | ((r == 0x7F) ? FLAG_P : 0) | FLAG_N);
    return r;
}

static uint16_t add16(uint16_t a, uint16_t b)
{
    unsigned r = (unsigned)a + b;
    z->f = (uint8_t)((z->f & (FLAG_S | FLAG_Z | FLAG_P))
         | (((a ^ b ^ r) & 0x1000) ? FLAG_H : 0)
         | ((r & 0x10000) ? FLAG_C : 0)
         | ((r >> 8) & 0x28));
    return (uint16_t)r;
}

static void adc16(uint16_t v)
{
    unsigned c = (unsigned)(z->f & FLAG_C);
    unsigned a = HL, r = a + v + c;
    uint16_t res = (uint16_t)r;
    z->f = (uint8_t)(((res >> 8) & 0xA8) | (res ? 0 : FLAG_Z)
         | (((a ^ v ^ r) & 0x1000) ? FLAG_H : 0)
         | (((a ^ ~(unsigned)v) & (a ^ r) & 0x8000) ? FLAG_P : 0)
         | ((r & 0x10000) ? FLAG_C : 0));
    set_hl(res);
}

static void sbc16(uint16_t v)
{
    unsigned c = (unsigned)(z->f & FLAG_C);
    unsigned a = HL, r = a - v - c;
    uint16_t res = (uint16_t)r;
    z->f = (uint8_t)(((res >> 8) & 0xA8) | (res ? 0 : FLAG_Z)
         | (((a ^ v ^ r) & 0x1000) ? FLAG_H : 0)
         | (((a ^ v) & (a ^ r) & 0x8000) ? FLAG_P : 0)
         | ((r & 0x10000) ? FLAG_C : 0) | FLAG_N);
    set_hl(res);
}

static void do_daa(void)
{
    int a = z->a, adjust = 0;
    uint8_t carry = (uint8_t)(z->f & FLAG_C);
    uint8_t nflag = (uint8_t)(z->f & FLAG_N);
    uint8_t half;

    if ((z->f & FLAG_H) || (a & 0x0F) > 9) adjust |= 0x06;
    if (carry || a > 0x99) { adjust |= 0x60; carry = FLAG_C; }

    if (nflag) {
        half = (uint8_t)(((((a & 0x0F) - (adjust & 0x0F)) & 0x10)) ? FLAG_H : 0);
        a -= adjust;
    } else {
        half = (uint8_t)(((((a & 0x0F) + (adjust & 0x0F)) & 0x10)) ? FLAG_H : 0);
        a += adjust;
    }
    z->a = (uint8_t)(a & 0xFF);
    z->f = (uint8_t)((z->a & 0xA8) | (z->a ? 0 : FLAG_Z) | parity_tab[z->a]
                     | half | nflag | carry);
}

static uint8_t rot_op(int op, uint8_t v)
{
    uint8_t c;
    switch (op) {
    case 0: c = (uint8_t)(v >> 7);  v = (uint8_t)((v << 1) | c); break;
    case 1: c = (uint8_t)(v & 1);   v = (uint8_t)((v >> 1) | (c << 7)); break;
    case 2: c = (uint8_t)(v >> 7);  v = (uint8_t)((v << 1) | (z->f & FLAG_C)); break;
    case 3: c = (uint8_t)(v & 1);   v = (uint8_t)((v >> 1) | ((z->f & FLAG_C) << 7)); break;
    case 4: c = (uint8_t)(v >> 7);  v = (uint8_t)(v << 1); break;
    case 5: c = (uint8_t)(v & 1);   v = (uint8_t)((v & 0x80) | (v >> 1)); break;
    case 6: c = (uint8_t)(v >> 7);  v = (uint8_t)((v << 1) | 1); break;
    default:c = (uint8_t)(v & 1);   v = (uint8_t)(v >> 1); break;
    }
    z->f = (uint8_t)((v & 0xA8) | (v ? 0 : FLAG_Z) | parity_tab[v] | (c ? FLAG_C : 0));
    return v;
}

static uint8_t *reg8(int i)
{
    switch (i) {
    case 0: return &z->b;
    case 1: return &z->c;
    case 2: return &z->d;
    case 3: return &z->e;
    case 4: return &z->h;
    case 5: return &z->l;
    case 7: return &z->a;
    }
    return 0;
}

static int cond(int i)
{
    switch (i) {
    case 0: return !(z->f & FLAG_Z);
    case 1: return  (z->f & FLAG_Z);
    case 2: return !(z->f & FLAG_C);
    case 3: return  (z->f & FLAG_C);
    case 4: return !(z->f & FLAG_P);
    case 5: return  (z->f & FLAG_P);
    case 6: return !(z->f & FLAG_S);
    default:return  (z->f & FLAG_S);
    }
}

static void alu(int op, uint8_t v)
{
    switch (op) {
    case 0: alu_add(v, 0); break;
    case 1: alu_add(v, 1); break;
    case 2: alu_sub(v, 0, 1); break;
    case 3: alu_sub(v, 1, 1); break;
    case 4: alu_and(v); break;
    case 5: alu_xor(v); break;
    case 6: alu_or(v);  break;
    default: alu_sub(v, 0, 0); break;
    }
}

static void do_cb(int disp_used, uint16_t ea)
{
    uint8_t op = fetch();
    int x = op >> 6, y = (op >> 3) & 7, zz = op & 7;
    uint8_t v;
    uint8_t *r = (zz == 6) ? 0 : reg8(zz);

    if (disp_used)  v = mem_rd(ea);
    else if (r)     v = *r;
    else            v = mem_rd(HL);

    switch (x) {
    case 0: v = rot_op(y, v); break;
    case 1: {
        uint8_t bit = (uint8_t)(v & (1 << y));
        z->f = (uint8_t)((z->f & FLAG_C) | FLAG_H | (bit ? 0 : (FLAG_Z | FLAG_P))
                         | (bit & 0x80));
        if (disp_used)  z->f |= (uint8_t)((ea >> 8) & 0x28);
        else if (r)     z->f |= (uint8_t)(v & 0x28);
        return; }
    case 2: v = (uint8_t)(v & ~(1 << y)); break;
    default: v = (uint8_t)(v | (1 << y)); break;
    }

    if (disp_used) { mem_wr(ea, v); if (r) *r = v; }
    else if (r)    *r = v;
    else           mem_wr(HL, v);
}

static void do_ed(void)
{
    uint8_t op = fetch();
    int y = (op >> 3) & 7, zz = op & 7, p = y >> 1, q = y & 1;
    uint16_t nn;

    switch (op) {
    case 0x44: case 0x4C: case 0x54: case 0x5C:
    case 0x64: case 0x6C: case 0x74: case 0x7C: {
        uint8_t v = z->a; z->a = 0; alu_sub(v, 0, 1); return; }
    case 0x45: case 0x4D: case 0x55: case 0x5D:
    case 0x65: case 0x6D: case 0x75: case 0x7D:
        z->pc = pop16(); z->iff1 = z->iff2; return;
    case 0x46: case 0x4E: case 0x66: case 0x6E: z->im = 0; return;
    case 0x56: case 0x76: z->im = 1; return;
    case 0x5E: case 0x7E: z->im = 2; return;
    case 0x47: z->i = z->a; return;
    case 0x4F: z->r = z->a; return;
    case 0x57: z->a = z->i;
        z->f = (uint8_t)((z->f & FLAG_C) | (z->a & 0xA8) | (z->a ? 0 : FLAG_Z)
                         | (z->iff2 ? FLAG_P : 0));
        return;
    case 0x5F: z->a = z->r;
        z->f = (uint8_t)((z->f & FLAG_C) | (z->a & 0xA8) | (z->a ? 0 : FLAG_Z)
                         | (z->iff2 ? FLAG_P : 0));
        return;
    case 0x67: {
        uint8_t m = mem_rd(HL);
        mem_wr(HL, (uint8_t)((m >> 4) | (z->a << 4)));
        z->a = (uint8_t)((z->a & 0xF0) | (m & 0x0F));
        set_sz53p(z->a); return; }
    case 0x6F: {
        uint8_t m = mem_rd(HL);
        mem_wr(HL, (uint8_t)((m << 4) | (z->a & 0x0F)));
        z->a = (uint8_t)((z->a & 0xF0) | (m >> 4));
        set_sz53p(z->a); return; }
    default: break;
    }

    if (zz == 0) {
        uint8_t v = io_in(BC);
        if (y != 6) *reg8(y) = v;
        set_sz53p(v);
        return;
    }
    if (zz == 1) {
        io_out(BC, (uint8_t)((y == 6) ? 0 : *reg8(y)));
        return;
    }
    if (zz == 2) {
        uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? HL : z->sp;
        if (q) adc16(v); else sbc16(v);
        return;
    }
    if (zz == 3) {
        nn = fetch16();
        if (q == 0) {
            uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? HL : z->sp;
            mem_wr(nn, (uint8_t)(v & 0xFF));
            mem_wr((uint16_t)(nn + 1), (uint8_t)(v >> 8));
        } else {
            uint16_t v = mem_rd(nn) | ((uint16_t)mem_rd((uint16_t)(nn + 1)) << 8);
            if (p == 0) set_bc(v);
            else if (p == 1) set_de(v);
            else if (p == 2) set_hl(v);
            else z->sp = v;
        }
        return;
    }

    if (op >= 0xA0 && op <= 0xBB && (op & 4) == 0) {
        int inc = (op & 8) ? -1 : 1;
        int rep = (op & 0x10) != 0;
        switch (op & 3) {
        case 0: {
            uint8_t v = mem_rd(HL);
            mem_wr(DE, v);
            set_hl((uint16_t)(HL + inc));
            set_de((uint16_t)(DE + inc));
            set_bc((uint16_t)(BC - 1));
            z->f = (uint8_t)((z->f & (FLAG_S | FLAG_Z | FLAG_C)) | (BC ? FLAG_P : 0));
            if (rep && BC) z->pc -= 2;
            return; }
        case 1: {
            uint8_t v = mem_rd(HL);
            uint8_t r = (uint8_t)(z->a - v);
            int hc = ((z->a & 0x0F) - (v & 0x0F)) & 0x10;
            set_hl((uint16_t)(HL + inc));
            set_bc((uint16_t)(BC - 1));
            z->f = (uint8_t)((z->f & FLAG_C) | (r & 0x80) | (r ? 0 : FLAG_Z)
                 | (hc ? FLAG_H : 0) | (BC ? FLAG_P : 0) | FLAG_N);
            if (rep && BC && r) z->pc -= 2;
            return; }
        case 2: {
            uint8_t v = io_in(BC);
            mem_wr(HL, v);
            z->b--;
            set_hl((uint16_t)(HL + inc));
            z->f = (uint8_t)((z->b ? 0 : FLAG_Z) | FLAG_N);
            if (rep && z->b) z->pc -= 2;
            return; }
        default: {
            uint8_t v = mem_rd(HL);
            z->b--;
            io_out(BC, v);
            set_hl((uint16_t)(HL + inc));
            z->f = (uint8_t)((z->b ? 0 : FLAG_Z) | FLAG_N);
            if (rep && z->b) z->pc -= 2;
            return; }
        }
    }
}

static void do_ddfd(uint16_t *ixy)
{
    uint8_t op = fetch();
    int x = op >> 6, y = (op >> 3) & 7, zz = op & 7, p = y >> 1, q = y & 1;

    if (op == 0xCB) {
        int8_t d = (int8_t)fetch();
        do_cb(1, (uint16_t)(*ixy + d));
        return;
    }
    switch (op) {
    case 0xE1: *ixy = pop16(); return;
    case 0xE5: push16(*ixy); return;
    case 0xE9: z->pc = *ixy; return;
    case 0xF9: z->sp = *ixy; return;
    case 0xE3: {
        uint16_t t = mem_rd(z->sp) | ((uint16_t)mem_rd((uint16_t)(z->sp + 1)) << 8);
        mem_wr(z->sp, (uint8_t)(*ixy & 0xFF));
        mem_wr((uint16_t)(z->sp + 1), (uint8_t)(*ixy >> 8));
        *ixy = t; return; }
    case 0x21: *ixy = fetch16(); return;
    case 0x22: { uint16_t nn = fetch16();
        mem_wr(nn, (uint8_t)(*ixy & 0xFF));
        mem_wr((uint16_t)(nn + 1), (uint8_t)(*ixy >> 8)); return; }
    case 0x2A: { uint16_t nn = fetch16();
        *ixy = mem_rd(nn) | ((uint16_t)mem_rd((uint16_t)(nn + 1)) << 8); return; }
    case 0x23: (*ixy)++; return;
    case 0x2B: (*ixy)--; return;
    case 0x34: case 0x35: {
        int8_t d = (int8_t)fetch();
        uint16_t ea = (uint16_t)(*ixy + d);
        uint8_t v = mem_rd(ea);
        mem_wr(ea, (op == 0x34) ? alu_inc(v) : alu_dec(v));
        return; }
    case 0x36: { int8_t d = (int8_t)fetch(); uint8_t v = fetch();
        mem_wr((uint16_t)(*ixy + d), v); return; }
    default: break;
    }

    if (x == 0 && zz == 1 && q == 1) {
        uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? *ixy : z->sp;
        *ixy = add16(*ixy, v);
        return;
    }
    if (x == 1) {
        if (y == 6 || zz == 6) {
            int8_t d = (int8_t)fetch();
            uint16_t ea = (uint16_t)(*ixy + d);
            if (y == 6) mem_wr(ea, *reg8(zz));
            else        *reg8(y) = mem_rd(ea);
            return;
        }
        {
            uint8_t src = (zz == 4) ? (uint8_t)(*ixy >> 8)
                        : (zz == 5) ? (uint8_t)(*ixy & 0xFF) : *reg8(zz);
            if (y == 4)      *ixy = (uint16_t)((src << 8) | (*ixy & 0xFF));
            else if (y == 5) *ixy = (uint16_t)((*ixy & 0xFF00) | src);
            else             *reg8(y) = src;
            return;
        }
    }
    if (x == 2) {
        if (zz == 6) { int8_t d = (int8_t)fetch(); alu(y, mem_rd((uint16_t)(*ixy + d))); }
        else alu(y, (zz == 4) ? (uint8_t)(*ixy >> 8)
                  : (zz == 5) ? (uint8_t)(*ixy & 0xFF) : *reg8(zz));
        return;
    }
    if (x == 0 && (zz == 4 || zz == 5) && (y == 4 || y == 5)) {
        uint8_t v = (y == 4) ? (uint8_t)(*ixy >> 8) : (uint8_t)(*ixy & 0xFF);
        v = (zz == 4) ? alu_inc(v) : alu_dec(v);
        if (y == 4) *ixy = (uint16_t)((v << 8) | (*ixy & 0xFF));
        else        *ixy = (uint16_t)((*ixy & 0xFF00) | v);
        return;
    }
    if (x == 0 && zz == 6 && (y == 4 || y == 5)) {
        uint8_t v = fetch();
        if (y == 4) *ixy = (uint16_t)((v << 8) | (*ixy & 0xFF));
        else        *ixy = (uint16_t)((*ixy & 0xFF00) | v);
        return;
    }

    /* Anything else: the prefix is ignored and the opcode runs unprefixed. */
    z->pc--;
    z80_step(z);
}

void z80_step(Z80 *cpu)
{
    uint8_t op;
    int x, y, zz, p, q;

    z = cpu;
    if (z->halted) { z->cycles += 4; return; }

    op = fetch();
    z->r = (uint8_t)((z->r & 0x80) | ((z->r + 1) & 0x7F));
    z->cycles += 4;

    if (op == 0xCB) { do_cb(0, 0); return; }
    if (op == 0xED) { do_ed(); return; }
    if (op == 0xDD) { do_ddfd(&z->ix); return; }
    if (op == 0xFD) { do_ddfd(&z->iy); return; }

    x = op >> 6; y = (op >> 3) & 7; zz = op & 7; p = y >> 1; q = y & 1;

    if (x == 1) {
        if (y == 6 && zz == 6) { z->halted = 1; return; }
        if (y == 6)       mem_wr(HL, *reg8(zz));
        else if (zz == 6) *reg8(y) = mem_rd(HL);
        else              *reg8(y) = *reg8(zz);
        return;
    }
    if (x == 2) { alu(y, (zz == 6) ? mem_rd(HL) : *reg8(zz)); return; }

    if (x == 0) {
        switch (zz) {
        case 0:
            if (y == 0) return;
            if (y == 1) {
                uint8_t t;
                t = z->a; z->a = z->a_; z->a_ = t;
                t = z->f; z->f = z->f_; z->f_ = t;
                return;
            }
            {
                int8_t d = (int8_t)fetch();
                if (y == 2) { z->pc = (uint16_t)(z->pc + d); return; }
                if (y == 3) { z->b--; if (z->b) z->pc = (uint16_t)(z->pc + d); return; }
                if (cond(y - 4)) z->pc = (uint16_t)(z->pc + d);
                return;
            }
        case 1:
            if (q == 0) {
                uint16_t v = fetch16();
                if (p == 0) set_bc(v);
                else if (p == 1) set_de(v);
                else if (p == 2) set_hl(v);
                else z->sp = v;
            } else {
                uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? HL : z->sp;
                set_hl(add16(HL, v));
            }
            return;
        case 2:
            if (q == 0) {
                if (p == 0) mem_wr(BC, z->a);
                else if (p == 1) mem_wr(DE, z->a);
                else if (p == 2) { uint16_t nn = fetch16();
                    mem_wr(nn, z->l); mem_wr((uint16_t)(nn + 1), z->h); }
                else { uint16_t nn = fetch16(); mem_wr(nn, z->a); }
            } else {
                if (p == 0) z->a = mem_rd(BC);
                else if (p == 1) z->a = mem_rd(DE);
                else if (p == 2) { uint16_t nn = fetch16();
                    z->l = mem_rd(nn); z->h = mem_rd((uint16_t)(nn + 1)); }
                else { uint16_t nn = fetch16(); z->a = mem_rd(nn); }
            }
            return;
        case 3: {
            uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? HL : z->sp;
            v = (uint16_t)(q ? v - 1 : v + 1);
            if (p == 0) set_bc(v);
            else if (p == 1) set_de(v);
            else if (p == 2) set_hl(v);
            else z->sp = v;
            return; }
        case 4:
            if (y == 6) mem_wr(HL, alu_inc(mem_rd(HL)));
            else        *reg8(y) = alu_inc(*reg8(y));
            return;
        case 5:
            if (y == 6) mem_wr(HL, alu_dec(mem_rd(HL)));
            else        *reg8(y) = alu_dec(*reg8(y));
            return;
        case 6: {
            uint8_t v = fetch();
            if (y == 6) mem_wr(HL, v); else *reg8(y) = v;
            return; }
        default:
            switch (y) {
            case 0: { uint8_t c = (uint8_t)(z->a >> 7);
                      z->a = (uint8_t)((z->a << 1) | c);
                      z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | (z->a & 0x28) | c);
                      return; }
            case 1: { uint8_t c = (uint8_t)(z->a & 1);
                      z->a = (uint8_t)((z->a >> 1) | (c << 7));
                      z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | (z->a & 0x28) | c);
                      return; }
            case 2: { uint8_t c = (uint8_t)(z->a >> 7);
                      z->a = (uint8_t)((z->a << 1) | (z->f & FLAG_C));
                      z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | (z->a & 0x28) | c);
                      return; }
            case 3: { uint8_t c = (uint8_t)(z->a & 1);
                      z->a = (uint8_t)((z->a >> 1) | ((z->f & FLAG_C) << 7));
                      z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | (z->a & 0x28) | c);
                      return; }
            case 4: do_daa(); return;
            case 5: z->a = (uint8_t)~z->a;
                    z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P|FLAG_C))
                                     | FLAG_H | FLAG_N | (z->a & 0x28));
                    return;
            case 6: z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | FLAG_C | (z->a & 0x28));
                    return;
            default: {
                uint8_t c = (uint8_t)(z->f & FLAG_C);
                z->f = (uint8_t)((z->f & (FLAG_S|FLAG_Z|FLAG_P)) | (c ? FLAG_H : 0)
                                 | (c ? 0 : FLAG_C) | (z->a & 0x28));
                return; }
            }
        }
    }

    switch (zz) {
    case 0: if (cond(y)) z->pc = pop16(); return;
    case 1:
        if (q == 0) {
            uint16_t v = pop16();
            if (p == 0) set_bc(v);
            else if (p == 1) set_de(v);
            else if (p == 2) set_hl(v);
            else set_af(v);
        } else {
            switch (p) {
            case 0: z->pc = pop16(); break;
            case 1: {
                uint8_t t;
                t = z->b; z->b = z->b_; z->b_ = t;
                t = z->c; z->c = z->c_; z->c_ = t;
                t = z->d; z->d = z->d_; z->d_ = t;
                t = z->e; z->e = z->e_; z->e_ = t;
                t = z->h; z->h = z->h_; z->h_ = t;
                t = z->l; z->l = z->l_; z->l_ = t;
                break; }
            case 2: z->pc = HL; break;
            default: z->sp = HL; break;
            }
        }
        return;
    case 2: { uint16_t nn = fetch16(); if (cond(y)) z->pc = nn; return; }
    case 3:
        switch (y) {
        case 0: z->pc = fetch16(); return;
        case 1: do_cb(0, 0); return;
        case 2: { uint8_t port = fetch();
                  io_out((uint16_t)((z->a << 8) | port), z->a); return; }
        case 3: { uint8_t port = fetch();
                  z->a = io_in((uint16_t)((z->a << 8) | port)); return; }
        case 4: { uint16_t t = mem_rd(z->sp) | ((uint16_t)mem_rd((uint16_t)(z->sp + 1)) << 8);
                  mem_wr(z->sp, z->l);
                  mem_wr((uint16_t)(z->sp + 1), z->h);
                  set_hl(t); return; }
        case 5: { uint8_t t;
                  t = z->d; z->d = z->h; z->h = t;
                  t = z->e; z->e = z->l; z->l = t;
                  return; }
        case 6: z->iff1 = z->iff2 = 0; return;
        default: z->iff1 = z->iff2 = 1; return;
        }
    case 4: { uint16_t nn = fetch16();
              if (cond(y)) { push16(z->pc); z->pc = nn; }
              return; }
    case 5:
        if (q == 0) {
            uint16_t v = (p == 0) ? BC : (p == 1) ? DE : (p == 2) ? HL : AF;
            push16(v);
        } else if (p == 0) {
            uint16_t nn = fetch16();
            push16(z->pc);
            z->pc = nn;
        }
        return;
    case 6: alu(y, fetch()); return;
    default: push16(z->pc); z->pc = (uint16_t)(y * 8); return;
    }
}

int z80_interrupt(Z80 *cpu)
{
    z = cpu;
    if (!z->iff1) return 0;
    z->iff1 = z->iff2 = 0;
    if (z->halted) { z->halted = 0; }
    /* Mode 0, with the keyboard controller putting RST 00h on the bus. */
    push16(z->pc);
    z->pc = 0x0000;
    return 1;
}
