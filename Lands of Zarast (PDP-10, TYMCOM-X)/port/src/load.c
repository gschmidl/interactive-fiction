/* load.c -- unpack a TYMCOM-X .SHR save image into emulated core.
 *
 * The tape files store one 36-bit word per five bytes: five 7-bit septets,
 * each right-justified in its byte, with the word's low-order 36th bit in
 * the high bit of the fifth byte.  ("5 septets per word", the ANSI-ASCII
 * magtape mode.)  Every file on this tape checks out against that layout --
 * no byte outside position 4 of a group ever has its high bit set.
 *
 * The save format is the simplest one there is: the file *is* the high
 * segment, word for word, loading at 400000.  There is no header to skip
 * and no low segment on disk -- a compiled TBA program builds its low
 * segment at run time, copying the initial contents down out of the high
 * segment with a BLT.  The first words of the file are high segment like
 * any other, but the compiler puts its equivalent of the job data area
 * there, and the monitor reads it back out:
 *
 *   400000   XWD  first free low-seg address, start address   (.JBSA)
 *   400001   the REENTER instruction                          (.JBREN)
 *   400002   RH = highest legal low-segment address           (.JBREL)
 *   400003   LH = length of the whole file in words
 *   400004   XWD 002006, 000001
 *
 * One word is missing.  Every TBA-compiled .SHR file on the tape is one
 * word short at a module boundary inside the runtime -- at 407164 for the
 * build these five were linked against -- and tools/mkimages.py derives
 * that index from each image and records it here.  Below it the image
 * resolves perfectly; above it everything is displaced by one, which is
 * why the routine at 467545 ends SUB 17,467605 over a literal that is
 * plainly a byte pointer until the word goes back in.  Nothing in any of
 * the five images addresses the word itself, so zero restores a working
 * image; see tools/mkimages.py for the test that pins the position.
 *
 * The check that settles the origin is the pair at 400333/400334,
 * MOVE 1,401124 / BLT 1,2401: at this origin 401124 holds
 * XWD 401026,002364, which copies fourteen words of initial low core
 * from 401026 down to 2364..2401.  One word further along in either
 * direction it is text or an instruction, and the BLT reads from
 * nowhere.  The start address then comes out as a JRST to the real
 * entry at 401120, with the REENTER path one word behind it.
 */
#include <string.h>
#include "pdp10.h"

int image_start;

static w36 word_at(const unsigned char *b)
{
    return ((w36)(b[0] & 0177) << 29) | ((w36)(b[1] & 0177) << 22)
         | ((w36)(b[2] & 0177) << 15) | ((w36)(b[3] & 0177) <<  8)
         | ((w36)(b[4] & 0177) <<  1) | ((w36)(b[4] >> 7) & 1);
}

int load_shr(const char *name)
{
    const shrfile_t *f;
    unsigned n, i;
    int lowtop, hilen, nwords;

    for (f = shrfiles; f->name; f++)
        if (!strcmp(f->name, name)) break;
    if (!f->name) return 0;
    if (f->len % 5) return 0;
    n = f->len / 5;
    if (n < 9) return 0;

    memset(M, 0, sizeof M);

    nwords = (int)((word_at(f->data + 3 * 5) >> 18) & HMASK);
    if (nwords != (int)n) return 0;
    image_start = (int)(word_at(f->data) & HMASK);
    lowtop = (int)(word_at(f->data + 2 * 5) & HMASK);

    for (i = 0; i < n; i++) {
        int a = 0400000 + (int)i + (i >= f->hole ? 1 : 0);
        if (a >= MEMTOP) break;
        M[a] = word_at(f->data + i * 5);
    }
    hilen = (int)n + 1;

    /* The job data area is the monitor's to fill in, not the file's, so
     * copy across what the header carries and synthesise the rest. */
    M[0120] = word_at(f->data);                           /* .JBSA  */
    M[0124] = word_at(f->data + 5);                       /* .JBREN */
    M[0121] = XWD(0, (word_at(f->data) >> 18) & HMASK);   /* .JBFF  */
    M[0044] = XWD(0, lowtop);                             /* .JBREL */
    M[0115] = XWD(0400000 + hilen - 1, 0400000);          /* .JBHRL */
    return 1;
}
