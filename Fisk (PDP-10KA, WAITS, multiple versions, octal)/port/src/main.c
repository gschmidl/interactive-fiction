#include "fisk.h"

const Image *image;

static void usage(void)
{
    int i;
    fprintf(stderr,
        "FisK - Sobotik & Beigel, Stanford 1980 - PDP-10/WAITS core image\n"
        "usage: fisk [-v NAME] [-t] [-m] [-D file] [-p A=V]\n"
        "  -v NAME   pick a game image (default: the first one)\n"
        "  -t        trace every instruction to stderr\n"
        "  -m        trace monitor calls to stderr\n"
        "  -D file   dump the 256K-word core to file on exit\n"
        "  -p A=V    poke octal word V into octal address A before starting\n"
        "images:");
    for (i = 0; i < fisk_nimages; i++)
        fprintf(stderr, " %s", fisk_images[i].name);
    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    int i, rc;
    const char *dumpfile = 0, *want = 0;
    const char *poke[16]; int npoke = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-m")) montrace = 1;
        else if (!strcmp(argv[i], "-D") && i + 1 < argc) dumpfile = argv[++i];
        else if (!strcmp(argv[i], "-v") && i + 1 < argc) want = argv[++i];
        else if (!strcmp(argv[i], "-p") && i + 1 < argc && npoke < 16) poke[npoke++] = argv[++i];
        else { usage(); return 2; }
    }

    image = &fisk_images[0];
    if (want) {
        for (i = 0; i < fisk_nimages; i++)
            if (!strcmp(fisk_images[i].name, want)) { image = &fisk_images[i]; break; }
        if (i == fisk_nimages) { usage(); return 2; }
    }

    setvbuf(stdout, NULL, _IOFBF, 8192);
    mon_init();
    cpu_reset();
    for (i = 0; i < npoke; i++) {
        unsigned long a = 0; unsigned long long v = 0;
        if (sscanf(poke[i], "%lo=%llo", &a, &v) == 2 && a < MEMSIZ)
            mem[a] = (word)v & WORDM;
    }
    rc = cpu_run();
    mon_flush();
    tty_flush();

    if (dumpfile) {
        FILE *f = fopen(dumpfile, "w");
        if (f) {
            for (i = 0; i < MEMSIZ; i++) fprintf(f, "%012llo\n", mem[i]);
            fclose(f);
        }
    }
    if (trace || montrace)
        fprintf(stderr, "\n[%lld instructions]\n", icount);
    return rc;
}
