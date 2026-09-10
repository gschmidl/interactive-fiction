/* mem.c - sparse VAX virtual memory (512-byte pages, two-level table) */
#include "vax.h"

static u8 ***l1;

void mem_init(void)
{
    l1 = calloc(L1_ENTRIES, sizeof(u8 **));
    if (!l1) { fprintf(stderr, "mem_init: out of memory\n"); exit(1); }
}

u8 *mem_page(u32 va, int alloc)
{
    u32 i1 = L1_IDX(va), i2 = L2_IDX(va);
    u8 **l2;
    if (i1 >= L1_ENTRIES) return NULL;
    l2 = l1[i1];
    if (!l2) {
        if (!alloc) return NULL;
        l2 = calloc(L2_ENTRIES, sizeof(u8 *));
        if (!l2) vax_fatal("mem: out of memory");
        l1[i1] = l2;
    }
    if (!l2[i2]) {
        if (!alloc) return NULL;
        l2[i2] = calloc(1, VA_PAGE);
        if (!l2[i2]) vax_fatal("mem: out of memory");
    }
    return l2[i2];
}

int mem_present(u32 va) { return mem_page(va, 0) != NULL; }

void mem_map(u32 va, u32 npages, const u8 *src, u32 nbytes)
{
    u32 i;
    for (i = 0; i < npages; i++) {
        u8 *p = mem_page(va + i * VA_PAGE, 1);
        u32 off = i * VA_PAGE;
        if (src && off < nbytes) {
            u32 n = nbytes - off; if (n > VA_PAGE) n = VA_PAGE;
            memcpy(p, src + off, n);
        }
    }
}

/* Byte accessors.  Unmapped reads are fatal so bugs surface immediately. */
u32 watch_lo, watch_hi;      /* set with -W to report writes to a range */

static u8 *xlate(u32 va, int wr)
{
    u8 *p = mem_page(va, 0);
    if (!p) vax_fatal("ACCVIO: %s unmapped VA %08X (PC=%08X)",
                      wr ? "write" : "read", va, cpu.r[PC]);
    (void)wr;
    return p + PG_OFF(va);
}

static void watched(u32 va, u32 v, int n)
{
    if (watch_hi && va + (u32)n > watch_lo && va <= watch_hi) {
        fflush(stdout);
        fprintf(stderr, "[watch] %08X <- %0*X (%d bytes) from PC=%08X icount=%llu\n",
                va, n * 2, v, n, cpu.r[PC], (unsigned long long)cpu.icount);
    }
}

u8  rd8 (u32 va) { return *xlate(va, 0); }
void wr8(u32 va, u8 v) { watched(va, v, 1); *xlate(va, 1) = v; }

/* Unaligned-safe multi-byte access (VAX allows any alignment). */
u16 rd16(u32 va)
{
    if (PG_OFF(va) <= VA_PAGE - 2) { u8 *p = xlate(va,0); return (u16)(p[0] | (p[1]<<8)); }
    return (u16)(rd8(va) | (rd8(va+1) << 8));
}
u32 rd32(u32 va)
{
    if (PG_OFF(va) <= VA_PAGE - 4) {
        u8 *p = xlate(va,0);
        return (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24);
    }
    return (u32)rd16(va) | ((u32)rd16(va+2) << 16);
}
u64 rd64(u32 va) { return (u64)rd32(va) | ((u64)rd32(va+4) << 32); }

void wr16(u32 va, u16 v)
{
    watched(va, v, 2);
    if (PG_OFF(va) <= VA_PAGE - 2) { u8 *p = xlate(va,1); p[0]=(u8)v; p[1]=(u8)(v>>8); return; }
    wr8(va, (u8)v); wr8(va+1, (u8)(v>>8));
}
void wr32(u32 va, u32 v)
{
    watched(va, v, 4);
    if (PG_OFF(va) <= VA_PAGE - 4) {
        u8 *p = xlate(va,1);
        p[0]=(u8)v; p[1]=(u8)(v>>8); p[2]=(u8)(v>>16); p[3]=(u8)(v>>24); return;
    }
    wr16(va, (u16)v); wr16(va+2, (u16)(v>>16));
}
void wr64(u32 va, u64 v) { wr32(va, (u32)v); wr32(va+4, (u32)(v>>32)); }

void mem_read(u32 va, void *dst, u32 n)
{
    u8 *d = dst; u32 i;
    for (i = 0; i < n; i++) d[i] = rd8(va + i);
}
void mem_write(u32 va, const void *src, u32 n)
{
    const u8 *s = src; u32 i;
    for (i = 0; i < n; i++) wr8(va + i, s[i]);
}

char *mem_cstr(u32 va, u32 maxlen)
{
    static char buf[4096];
    u32 i;
    if (maxlen > sizeof(buf) - 1) maxlen = sizeof(buf) - 1;
    for (i = 0; i < maxlen; i++) { buf[i] = (char)rd8(va + i); if (!buf[i]) break; }
    buf[i] = 0;
    return buf;
}
