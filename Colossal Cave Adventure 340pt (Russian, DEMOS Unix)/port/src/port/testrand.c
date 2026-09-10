/* ======================================================================
 *  testrand.c -- a fixed pseudo-random generator, used ONLY by the
 *  verification builds (-DTESTRAND).  Linked into both the Windows port
 *  and the Linux build of the untouched 1985 sources so that the two
 *  transcripts can be diffed byte for byte; without it glibc's rand()
 *  and msvcrt's rand() diverge on the first probability test and the
 *  diff says nothing.
 *
 *  srand() deliberately ignores its argument: the original seeds itself
 *  from the clock, the port from the clock as well, and neither may be
 *  allowed to influence the comparison.
 * ====================================================================== */

static unsigned long state = 12345UL;

void port_srand(unsigned s)
{
    (void) s;
    state = 12345UL;
}

int port_rand(void)
{
    state = state * 1103515245UL + 12345UL;
    return (int) ((state >> 16) & 0x7FFF);   /* 15 bits, like msvcrt */
}
