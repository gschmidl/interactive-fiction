/* main.c - driver for the VAX/VMS image runner */
#include "vax.h"

extern int img_verbose;

#define STACK_TOP   0x80000000u
#define STACK_PAGES 512                       /* 256 KB of user stack */

static void usage(void)
{
    fprintf(stderr,
        "usage: vaxvms [options] image.exe [args]\n"
        "  -L dir     directory to search for shareable images (repeatable)\n"
        "  -V         verbose image loading\n"
        "  -T n       instruction trace level\n"
        "  -S file    system service vector name table\n");
    exit(1);
}

int main(int argc, char **argv)
{
    u32 entry = 0, dis_lo = 0, dis_hi = 0;
    int i, argi = 1;

    mem_init();
    cpu_init();

    for (; argi < argc && argv[argi][0] == '-'; argi++) {
        switch (argv[argi][1]) {
        case 'L': img_add_libpath(argv[++argi]); break;
        case 'V': img_verbose = 1; break;
        case 'T': trace_level = atoi(argv[++argi]); break;
        case 'D': data_dir = argv[++argi]; break;
        case 's': svc_verbose++; break;
        case 'B': brk_pc = strtoul(argv[++argi], NULL, 16); break;
        case 'M': dump_addr = strtoul(argv[++argi], NULL, 16); break;
        case 'k': trap_after_read = atoi(argv[++argi]); break;
        case 'b': brk_pc = strtoul(argv[++argi], NULL, 16); brk_fatal = 1; break;
        case 'd': dis_lo = strtoul(argv[++argi], NULL, 16);
                  dis_hi = strtoul(argv[++argi], NULL, 16); break;
        case 'W': watch_lo = strtoul(argv[++argi], NULL, 16);
                  watch_hi = strtoul(argv[++argi], NULL, 16); break;
        default: usage();
        }
    }
    if (argi >= argc) usage();

    /* user stack in P1 space */
    mem_map(STACK_TOP - STACK_PAGES * VA_PAGE, STACK_PAGES, NULL, 0);
    cpu.r[SP] = STACK_TOP - 0x400;   /* leave room above SP, as VMS does */

    vms_init();
    if (!img_load_main(argv[argi], &entry)) return 1;
    if (!entry) { fprintf(stderr, "no transfer address\n"); return 1; }

    vms_p0_break = (img_next_base + 0xFFFF) & ~0xFFFFu;
    if (dis_hi) { dis_range(dis_lo, dis_hi); return 0; }

    /* Enter the image the way the activator does: an empty argument list,
     * then a CALLS-style frame so a RET from main() unwinds cleanly. */
    for (i = 0; i < 12; i++) cpu.r[i] = 0;
    cpu.r[AP] = cpu.r[SP];
    cpu.r[FP] = cpu.r[SP];
    if (img_verbose) fprintf(stderr, "starting at %08X, SP=%08X\n", entry, cpu.r[SP]);
    cpu_enter(entry);
    cpu_run();
    return 0;
}
