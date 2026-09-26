/* image.c - OpenVMS VAX native image (.EXE) loader with shareable-image fixups */
#include "vax.h"

#define MAXLIBPATH 8
static char libpath[MAXLIBPATH][512];
static int  nlibpath;
static ShrImage *shrlist;
u32 img_next_base = 0x00100000;    /* shareable images start above the main image */

int img_verbose = 0;

void img_add_libpath(const char *dir)
{
    if (nlibpath < MAXLIBPATH) snprintf(libpath[nlibpath++], 512, "%s", dir);
}

ShrImage *img_find(const char *name)
{
    ShrImage *s;
    for (s = shrlist; s; s = s->next) if (!strcmp(s->name, name)) return s;
    return NULL;
}

static u8 *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    u8 *b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); *len = ftell(f); fseek(f, 0, SEEK_SET);
    b = malloc(*len ? (size_t)*len : 1);
    if (fread(b, 1, *len, f) != (size_t)*len) { fclose(f); free(b); return NULL; }
    fclose(f);
    return b;
}

/* Try <dir>/<NAME>.EXE with a few case/suffix variants (VMS copies keep ";1"). */
static u8 *find_image(const char *name, long *len, char *found, size_t fsz)
{
    static const char *sfx[] = { ".EXE", ".EXE;1", ".exe", ".exe;1", "", NULL };
    int i, j;
    for (i = 0; i < nlibpath; i++)
        for (j = 0; sfx[j]; j++) {
            char p[700]; u8 *b;
            snprintf(p, sizeof p, "%s/%s%s", libpath[i], name, sfx[j]);
            b = slurp(p, len);
            if (b) { if (found) snprintf(found, fsz, "%s", p); return b; }
        }
    return NULL;
}

#define LE16(p,o) ((u16)((p)[(o)] | ((p)[(o)+1]<<8)))
#define LE32(p,o) ((u32)((p)[(o)] | ((p)[(o)+1]<<8) | ((p)[(o)+2]<<16) | ((u32)(p)[(o)+3]<<24)))

typedef struct {
    u32 vpn, flags, vbn; u16 pagcnt, size;
    char gblname[64];
} ISD;

/* Walk the ISD table.  Returns count; fills isds[]. */
static int parse_isds(const u8 *img, long len, ISD *isds, int maxisd, u32 *fixup_va)
{
    u32 hdrsize = LE16(img, 0);
    u32 off = hdrsize, end = 512;         /* ISDs live in the header block */
    int n = 0;
    (void)len;
    while (off + 4 <= end && n < maxisd) {
        u16 sz = LE16(img, off);
        ISD *d;
        if (sz < 8 || off + sz > end) break;
        d = &isds[n];
        d->size   = sz;
        d->pagcnt = LE16(img, off + 2);
        d->vpn    = LE32(img, off + 4) & 0x003FFFFF;
        d->flags  = LE32(img, off + 8);
        d->vbn    = (sz >= 16) ? LE32(img, off + 12) : 0;
        d->gblname[0] = 0;
        if (sz >= 22 && (d->flags & 1)) {          /* ISD$V_GBL: global section */
            u32 no = off + 20, l = img[no];
            if (l < sizeof d->gblname) { memcpy(d->gblname, img + no + 1, l); d->gblname[l] = 0; }
        }
        if ((d->flags >> 10) & 1) *fixup_va = d->vpn * VA_PAGE;   /* ISD$V_FIXUPVEC */
        n++;
        off += sz;
    }
    return n;
}

/* Strip the "_nnn" global-section suffix to get the image file name. */
static void gbl_to_name(const char *gbl, char *out, size_t n)
{
    size_t l = strlen(gbl);
    snprintf(out, n, "%s", gbl);
    if (l > 4 && out[l-4] == '_') out[l-4] = 0;
}

static ShrImage *load_shareable(const char *name);

/* Map the ISDs of one image at 'base'; returns the highest VA used. */
static u32 map_image(const u8 *img, long len, ISD *isds, int nisd, u32 base,
                     const char *what)
{
    int i; u32 top = base;
    for (i = 0; i < nisd; i++) {
        ISD *d = &isds[i];
        u32 va = base + d->vpn * VA_PAGE;
        u32 nb = (u32)d->pagcnt * VA_PAGE;
        if (d->flags & 1) continue;                    /* shareable ref, mapped separately */
        if ((d->flags >> 2) & 1) {                     /* ISD$V_DZRO */
            mem_map(va, d->pagcnt, NULL, 0);
        } else {
            long fo = (long)((i32)d->vbn - 1) * 512;
            u32 avail = 0;
            if (d->vbn && fo >= 0 && fo < len) avail = (u32)(len - fo);
            if (avail > nb) avail = nb;
            mem_map(va, d->pagcnt, d->vbn ? img + fo : NULL, avail);
        }
        if (va + nb > top) top = va + nb;
        if (img_verbose)
            fprintf(stderr, "  [%s] ISD %d: va=%08X pages=%u vbn=%u flags=%08X\n",
                    what, i, va, d->pagcnt, d->vbn, d->flags);
    }
    return top;
}

/* Apply the image activator fixup section at 'fx' (a VA inside this image). */
static void apply_fixups(u32 fx, u32 self_base, ShrImage **shr, int nshr, const char *what)
{
    u32 gp, shl, off, imgcnt;
    if (!fx || !mem_present(fx)) return;
    gp     = rd32(fx + 0x0c);     /* code-address (G^) fixup list */
    shl    = rd32(fx + 0x18);     /* shareable image list, 64-byte entries */
    imgcnt = rd32(fx + 0x1c);
    if (img_verbose) {
        u32 k;
        fprintf(stderr, "  [%s] IAF at %08X: G=%X shl=%X images=%u\n",
                what, fx, gp, shl, imgcnt);
        for (k = 1; k < imgcnt && k < 16; k++) {
            u32 ne = fx + shl + 64 * k + 0x18;
            u32 l = rd8(ne);
            char nm[64];
            if (l < sizeof nm) { mem_read(ne + 1, nm, l); nm[l] = 0;
                fprintf(stderr, "  [%s]   image %u = \"%s\"\n", what, k, nm); }
        }
    }

    /* G^ reference fixups: each slot holds a transfer-vector offset; add the base. */
    for (off = gp; off && off < VA_PAGE * 16;) {
        u32 cnt = rd32(fx + off), idx = rd32(fx + off + 4), k, b = 0;
        if (!cnt) break;
        if (idx == 0) b = self_base;
        else if ((int)idx <= nshr && shr[idx-1]) b = shr[idx-1]->base;
        else { fprintf(stderr, "  [%s] fixup: unknown image index %u\n", what, idx); break; }
        for (k = 0; k < cnt; k++) {
            u32 slot = fx + off + 8 + k * 4;
            wr32(slot, rd32(slot) + b);
        }
        if (img_verbose)
            fprintf(stderr, "  [%s]   %u G fixups -> image %u (base %08X), slots %08X..%08X\n",
                    what, cnt, idx, b, fx + off + 8, fx + off + 8 + (cnt-1)*4);
        off += 8 + cnt * 4;
    }
}

#define MAXISD 64

static ShrImage *load_shareable(const char *name)
{
    ShrImage *s = img_find(name);
    u8 *img; long len; ISD isds[MAXISD]; int nisd, i, nshr = 0;
    u32 fx = 0, base, top;
    ShrImage *deps[16];
    char found[700];

    if (s) return s;
    img = find_image(name, &len, found, sizeof found);
    if (!img) { fprintf(stderr, "cannot find shareable image %s\n", name); return NULL; }

    s = calloc(1, sizeof *s);
    snprintf(s->name, sizeof s->name, "%s", name);
    s->next = shrlist; shrlist = s;              /* register before recursing */

    nisd = parse_isds(img, len, isds, MAXISD, &fx);
    base = img_next_base;
    if (img_verbose) fprintf(stderr, "load shareable %s from %s at base %08X (%d ISDs)\n",
                             name, found, base, nisd);
    top = map_image(img, len, isds, nisd, base, name);
    img_next_base = (top + 0xFFFF) & ~0xFFFFu;   /* reserve before recursing */

    for (i = 0; i < nisd; i++)
        if ((isds[i].flags & 1) && isds[i].gblname[0]) {
            char nm[64]; gbl_to_name(isds[i].gblname, nm, sizeof nm);
            if (nshr < 15) deps[nshr++] = load_shareable(nm);
        }

    s->base = base;
    s->size = top - base;
    s->fixup_va = fx ? base + fx : 0;
    apply_fixups(s->fixup_va, base, deps, nshr, name);
    free(img);
    return s;
}

int img_load_main(const char *path, u32 *entry)
{
    u8 *img; long len; ISD isds[MAXISD]; int nisd, i, nshr = 0;
    u32 fx = 0, activoff, t1, t2, t3;
    ShrImage *deps[16];

    img = slurp(path, &len);
    if (!img) { fprintf(stderr, "cannot open image %s\n", path); return 0; }
    if (LE16(img, 0) == 0 || LE16(img, 0) > 512) {
        fprintf(stderr, "%s: not an OpenVMS VAX image\n", path); return 0;
    }
    nisd = parse_isds(img, len, isds, MAXISD, &fx);
    activoff = LE16(img, 2);
    t1 = LE32(img, activoff); t2 = LE32(img, activoff + 4); t3 = LE32(img, activoff + 8);
    if (img_verbose)
        fprintf(stderr, "main image %s: %d ISDs, transfer %08X/%08X/%08X, fixup VA %08X\n",
                path, nisd, t1, t2, t3, fx);

    map_image(img, len, isds, nisd, 0, "main");

    for (i = 0; i < nisd; i++)
        if ((isds[i].flags & 1) && isds[i].gblname[0]) {
            char nm[64]; gbl_to_name(isds[i].gblname, nm, sizeof nm);
            if (nshr >= 15) break;
            deps[nshr] = load_shareable(nm);
            if (!deps[nshr]) { free(img); return 0; }
            nshr++;
        }

    apply_fixups(fx, 0, deps, nshr, "main");

    /* t1 is normally SYS$IMGSTA (P1 traceback startup); the real entry is then t2. */
    *entry = (t1 && t1 < 0x40000000) ? t1 : (t2 ? t2 : t1);
    free(img);
    return 1;
}
