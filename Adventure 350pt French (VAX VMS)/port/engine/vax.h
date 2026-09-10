/* vax.h - VAX user-mode emulator + OpenVMS shim, shared declarations */
#ifndef VAX_H
#define VAX_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

/* ---------------- memory ---------------- */
#define VA_PAGE      512
#define L2_ENTRIES   4096                    /* pages per L2 table (2MB) */
#define L1_ENTRIES   2048                    /* 2048 * 2MB = 4GB */
#define L1_IDX(va)   ((va) >> 21)
#define L2_IDX(va)   (((va) >> 9) & (L2_ENTRIES-1))
#define PG_OFF(va)   ((va) & (VA_PAGE-1))

void  mem_init(void);
u8   *mem_page(u32 va, int alloc);          /* page base or NULL */
void  mem_map(u32 va, u32 npages, const u8 *src, u32 nbytes);
int   mem_present(u32 va);

u8    rd8 (u32 va);
u16   rd16(u32 va);
u32   rd32(u32 va);
u64   rd64(u32 va);
void  wr8 (u32 va, u8 v);
void  wr16(u32 va, u16 v);
void  wr32(u32 va, u32 v);
void  wr64(u32 va, u64 v);
void  mem_read (u32 va, void *dst, u32 n);
void  mem_write(u32 va, const void *src, u32 n);
char *mem_cstr(u32 va, u32 maxlen);          /* static buffer */

/* ---------------- cpu ---------------- */
#define R0 0
#define AP 12
#define FP 13
#define SP 14
#define PC 15

typedef struct {
    u32 r[16];
    u32 psl;
    int halted;
    u32 halt_code;
    u64 icount;
} VAXCPU;

extern VAXCPU cpu;

/* PSL condition code bits */
#define PSL_C 0x01
#define PSL_V 0x02
#define PSL_Z 0x04
#define PSL_N 0x08
#define PSL_IV 0x20   /* integer overflow trap enable */
#define PSL_FU 0x40
#define PSL_DV 0x80

#define IMG_EXIT_PC 0x7FFFFFF0u

void cpu_init(void);
void cpu_enter(u32 entry);
void cpu_run(void);
void cpu_step(void);

/* SS$ status codes used by the shim */
#define SS_NORMAL     1
#define SS_BADPARAM   20
#define SS_ACCVIO     12
#define SS_ENDOFFILE  2072   /* not real; RMS uses RMS$_EOF */

/* fatal / diagnostics */
void vax_fatal(const char *fmt, ...);
extern int trace_level;
extern FILE *tracef;

/* ---------------- image loader ---------------- */
typedef struct ShrImage {
    char  name[64];
    u32   base;            /* assigned P0 base */
    u32   size;            /* bytes of address space consumed */
    u32   fixup_va;        /* VA of its fixup vector page, 0 if none */
    struct ShrImage *next;
} ShrImage;

int  img_load_main(const char *path, u32 *entry);
ShrImage *img_find(const char *name);
extern u32 img_next_base;

/* search path for shareable images */
void img_add_libpath(const char *dir);

/* ---------------- vms services ---------------- */
void vms_init(void);
u32  dis_one(u32 va, char *out, size_t osz);
void dis_range(u32 lo, u32 hi);
int  vms_dispatch(u32 va);      /* returns 1 if handled */
int  vms_is_service(u32 va);    /* address is a P1 system service vector */
u32  vms_service(u32 va);       /* perform it; returns the status for R0 */
extern const char *data_dir;
extern int svc_verbose;
extern int trap_after_read;
extern u64 abort_at;
extern u32 dump_addr;
extern u32 vms_p0_break;
extern int term_width, term_height;
extern u32 vms_caller;
extern u32 watch_lo, watch_hi;
extern u32 brk_pc;
extern int brk_fatal;
u32  svc_arg(int n);            /* CALLS/CALLG arg n (1-based) */
int  svc_argc(void);
void svc_return(u32 status);

#endif
