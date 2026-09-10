/* syscall32.h -- the AOS/VS shim for 32-bit programs.
 *
 * The call numbers are the same as the 16-bit ones (SYSID.32.SR gives ?OPEN
 * 300, ?READ 302, ?WRITE 303 ... exactly as SYSID.16.SR does), but the I/O
 * packet is laid out differently and its two pointers are 32-bit:
 *
 *   16-bit   ICH 0  ISTI 1  ISTO 2  IBAD 3  IRCL 5  IRLR 6  IRNH 7  IRNL 8
 *            IFNP 9  IMRS 10
 *   32-bit   ICH 0  ISTI 1  ISTO 2  IMRS 3  IBAD 4-5  IRCL 7  IRLR 8
 *            IRNH 10 IRNL 11  IFNP 12-13
 */
#define P_ICH   0
#define P_ISTI  1
#define P_ISTO  2
#define P_IMRS  3
#define P_IBAD  4
#define P_IRES  6
#define P_IRCL  7
#define P_IRLR  8
#define P_IRNH  10
#define P_IRNL  11
#define P_IFNP  12

#define ICRF    0x4000u
#define OFOT    0x0008u
#define RF_DY   1
#define RF_DS   2
#define RF_FX   3
#define RF_VR   4
#define RF_UN   5

#define SC_CREATE   0
#define SC_DELETE   1
#define SC_MEM      3
#define SC_MEMI    12
#define SC_GTOD    30
#define SC_GDAY    33
#define SC_SOPEN   51
#define SC_GHRZ    60
#define SC_SCLOSE 113
#define SC_RNGPR  169
#define SC_OPEN   192
#define SC_CLOSE  193
#define SC_READ   194
#define SC_WRITE  195
#define SC_TERM   196
#define SC_GPOS   198
#define SC_GTMES  199
#define SC_RETURN 200
#define SC_ERMSG  201
#define SC_GCHR   202
#define SC_SCHR   203
#define SC_SPOS   210
#define SC_UIDSTAT 219
#define SC_GTNAM  233
#define SC_IFPU   354

static FILE *chan[64];
static int   chan_console[64];
static long  chan_pos[64];
static int   chan_spos[64];
static int   chan_recl[64];    /* the record length declared at ?OPEN */
static dword chan_ibad[64];    /* the buffer declared at ?OPEN */
static long *chan_vrec[64];    /* variable-length records: off, len */
static long  chan_vnrec[64];
static int   chan_vscan[64];   /* 0 unlooked, 1 framed, -1 plain */
static unsigned cur_pages, max_pages;

static char datadir_buf[512] = "";
static char savedir_buf[512] = ".";
static void set_datadir(const char *d)
{ strncpy(datadir_buf, d, sizeof datadir_buf - 1); datadir_buf[sizeof datadir_buf - 1] = 0; }
static void set_savedir(const char *d)
{ strncpy(savedir_buf, d, sizeof savedir_buf - 1); savedir_buf[sizeof savedir_buf - 1] = 0; }
static int  datadir_set(void) { return datadir_buf[0] != 0; }

static dword pkt_dw(dword pkt, unsigned off)
{ return ((dword)M[MADDR(pkt + off)] << 16) | M[MADDR(pkt + off + 1)]; }

/* A 32-bit byte pointer is the word address shifted up one with the byte
 * half in bit 0, ring bits and all. */
static char *getbstr(dword bp, char *buf, int max)
{
    int i = 0;
    while (i < max - 1) {
        word w = M[MADDR(bp >> 1)];
        int c = (bp & 1) ? (w & 0xFF) : (w >> 8);
        if (!c) break;
        buf[i++] = (char)c; bp++;
    }
    buf[i] = 0;
    return buf;
}

static void getbytes(dword bp, char *dst, int len)
{
    int i;
    for (i = 0; i < len; i++, bp++) {
        word w = M[MADDR(bp >> 1)];
        dst[i] = (char)((bp & 1) ? (w & 0xFF) : (w >> 8));
    }
}

static void putbstr(dword bp, const char *src, int len)
{
    int i;
    for (i = 0; i < len; i++, bp++) {
        dword a = MADDR(bp >> 1);
        if (bp & 1) M[a] = (word)((M[a] & 0xFF00) | (unsigned char)src[i]);
        else        M[a] = (word)((M[a] & 0x00FF) | ((unsigned char)src[i] << 8));
    }
}

static int file_exists(const char *p)
{ FILE *f = fopen(p, "rb"); if (!f) return 0; fclose(f); return 1; }

static void save_path(char *o, size_t n, const char *nm)
{ snprintf(o, n, "%s/%s", savedir_buf, nm); }

static void read_path(char *o, size_t n, const char *nm)
{ save_path(o, n, nm); if (file_exists(o)) return;
  snprintf(o, n, "%s/%s", datadir_buf, nm); }


/* A ?ORVR file -- record format 4, "variable length" -- does not hold its
 * records end to end the way a fixed-length one does: each is stored with a
 * four-byte header, and the header is the record's whole stored length
 * written as four ASCII digits.  Both of ZORK.PR's heaps are such a file,
 * and both are made entirely of 512-byte records, so every header in them
 * reads "0516" -- four for itself and 512 of data.  Taken as a flat file the
 * first four bytes look like a stray magic number in front of the header the
 * program wants to read, and the version check fails on them; framed
 * properly the arithmetic is exact, 227 records in ZORK_RO_HEAP and 34 in
 * ZORK_RW_HEAP, which is the 261 the program asks for.
 *
 * The scan is a guess that has to prove itself: every header has to be four
 * digits and the last record has to end exactly at the end of the file.  If
 * it does not, the channel keeps the flat reading, which is what FERRET.PR's
 * heaps need. */
static void vr_scan(int ch)
{
    long off = 0, end, n = 0, cap = 0, *tab = NULL;

    chan_vscan[ch] = -1;
    if (fseek(chan[ch], 0, SEEK_END)) return;
    end = ftell(chan[ch]);
    while (off < end) {
        unsigned char h[4];
        long len = 0;
        int d;
        if (fseek(chan[ch], off, SEEK_SET)) break;
        if (fread(h, 1, 4, chan[ch]) != 4) break;
        for (d = 0; d < 4; d++) {
            if (h[d] < '0' || h[d] > '9') { len = -1; break; }
            len = len * 10 + (h[d] - '0');
        }
        if (len <= 4 || off + len > end) break;
        if (n == cap) {
            long ncap = cap ? cap * 2 : 64;
            long *nt = (long *)realloc(tab, (size_t)ncap * 2 * sizeof *nt);
            if (!nt) break;
            tab = nt; cap = ncap;
        }
        tab[2 * n] = off + 4;
        tab[2 * n + 1] = len - 4;
        n++;
        off += len;
    }
    if (off == end && n > 0) {
        chan_vrec[ch]  = tab;
        chan_vnrec[ch] = n;
        chan_vscan[ch] = 1;
    } else {
        free(tab);
    }
    fseek(chan[ch], 0, SEEK_SET);
}

static void vr_forget(int ch)
{
    free(chan_vrec[ch]);
    chan_vrec[ch] = NULL;
    chan_vnrec[ch] = 0;
    chan_vscan[ch] = 0;
}

#define IPST 0x2000u   /* ?ISTI bit 2: absolute positioning */

static int rec_format(dword pkt, int ch)
{
    word sti = M[MADDR(pkt + P_ISTI)];
    if (sti & ICRF) return (int)(sti & 7);
    return chan_console[ch] ? RF_DS : RF_FX;
}

static int is_delim(int c) { return c == 0 || c == 10 || c == 12; }

/* ?GTMES reads the CLI message the program was started with.  PARU gives the
 * packet -- ?GREQ the request, ?GNUM the argument number, ?GSW a byte pointer
 * to a switch name, ?GRES a byte pointer to where the answer goes -- and the
 * requests are ?GMES 0 (the whole message), ?GCMD 1 (the command), ?GCNT 2
 * (how many arguments), ?GARG 3 (one argument), ?GTSW 4 and ?GSWS 5 (switches).
 * These games are run with no arguments and no switches, so the honest
 * answers are the command name, a count of nought, and ERNAG, "no such
 * argument", for anything past argument zero.  Answering every request with
 * success and a nought, which is what this used to do, tells FERRET.PR it was
 * given an argument that is the empty string, and it asks for a filename it
 * was never going to get.
 *
 * The command name comes from the .PR being run, upper-cased and without its
 * directory or extension, which is what the CLI would have put there. */
#define P_GREQ  0
#define P_GNUM  1
#define P_GSW   2
#define P_GRES  4
#define ERNAG 228

static char progname_buf[64] = "";
static char progpath_buf[80] = "";
static void set_progname(const char *pr)
{
    const char *sl = strrchr(pr, '/'), *bs = strrchr(pr, '\\'), *dot;
    size_t k;
    if (bs > sl) sl = bs;
    sl = sl ? sl + 1 : pr;
    strncpy(progname_buf, sl, sizeof progname_buf - 1);
    progname_buf[sizeof progname_buf - 1] = 0;
    dot = strrchr(progname_buf, '.');
    if (dot) progname_buf[dot - progname_buf] = 0;
    for (k = 0; progname_buf[k]; k++)
        progname_buf[k] = (char)toupper((unsigned char)progname_buf[k]);
    snprintf(progpath_buf, sizeof progpath_buf, ":%s.PR", progname_buf);
}

/* ?RNGPR -- "get pathname from logical address", PARU calls it -- is asked
 * which program is running in a ring.  Its packet is a byte pointer to a
 * buffer, the ring number, and the buffer's length in bytes; there is only
 * one program here, and it is in ring 7.  FERRET.PR asks before it saves. */
#define P_RNGBP 0
#define P_RNGNM 2
#define P_RNGLB 3

static int do_rngpr(dword pkt)
{
    dword bp = pkt_dw(pkt, P_RNGBP);
    int lim = (int)M[MADDR(pkt + P_RNGLB)];
    int n = (int)strlen(progpath_buf);
    if (lim <= 0 || lim > 255) lim = 256;
    if (n > lim - 1) n = lim - 1;
    if ((((bp >> 1) & OFFMASK)) >= MEMWORDS) { AC[0] = 0; return 0; }
    putbstr(bp, progpath_buf, n);
    putbstr(bp + (dword)n, "", 1);
    AC[0] = (dword)n;
    return 1;
}

static int do_gtmes(dword pkt)
{
    word req = M[MADDR(pkt + P_GREQ)];
    word num = M[MADDR(pkt + P_GNUM)];
    dword res = pkt_dw(pkt, P_GRES);
    const char *ans = NULL;

    if (verbose)
        fprintf(stderr, "   [gtmes req=%u num=%u res=%08X]\n", req, num, res);
    switch (req) {
    case 0: case 1:                       /* ?GMES, ?GCMD */
        ans = progname_buf; break;
    case 2:                               /* ?GCNT */
        AC[1] = 0; AC[0] = 0; return 1;
    case 3:                               /* ?GARG */
        if (num == 0) { ans = progname_buf; break; }
        AC[0] = ERNAG; return 0;
    default:                              /* ?GTSW, ?GSWS and the rest */
        AC[0] = ERNAG; AC[1] = 0; return 0;
    }
    if ((((res >> 1) & OFFMASK)) < MEMWORDS) {
        putbstr(res, ans, (int)strlen(ans));
        putbstr(res + (dword)strlen(ans), "", 1);
    }
    AC[0] = (dword)strlen(ans);
    return 1;
}

static int do_rw32(word code, dword pkt);

/* 1 = the normal return, 0 = the error return, -1 = not implemented. */
static int do_syscall32(word code)
{
    dword pkt = AC[2] & OFFMASK;
    char name[288], path[512];
    int ch, i;

    switch (code) {
    case SC_RETURN: case SC_TERM:
        if (verbose) fprintf(stderr, "\n[terminated, AC0=%08X]\n", AC[0]);
        halted = 1;
        return 1;

    case SC_IFPU: case SC_SCHR: case SC_UIDSTAT:
        return 1;

    case SC_GHRZ: AC[0] = 1; return 1;
    case SC_GTMES: return do_gtmes(pkt);
    case SC_RNGPR: return do_rngpr(pkt);

    /* The addresses these two hand back are ring-qualified, like every other
     * address the machine deals in.  The PL/I runtime compares what ?MEMI
     * returns against a pointer it already holds -- I.GINIT at 7E058 in
     * FERRET.PR does WSGE 0,2 with the heap start in AC2 -- and a bare
     * offset always compares low, so the runtime concludes it has no room
     * and gives up with "cannot initialise". */
    case SC_MEM:
        AC[0] = max_pages - cur_pages;
        AC[1] = cur_pages;
        AC[2] = RING | ((cur_pages * 1024u - 1) & OFFMASK);
        return 1;

    case SC_MEMI: {
        int want = (int32_t)AC[0];
        unsigned np = cur_pages + (unsigned)want;
        if ((int)np < 1 || np > max_pages) return 0;
        cur_pages = np;
        AC[1] = RING | ((cur_pages * 1024u - 1) & OFFMASK);
        return 1;
    }

    case SC_GCHR:
        M[MADDR(pkt + 0)] = 0x0001;
        M[MADDR(pkt + 1)] = 0x8000;
        M[MADDR(pkt + 2)] = 80;
        M[MADDR(pkt + 3)] = 24;
        for (i = 4; i < 8; i++) M[MADDR(pkt + i)] = 0;
        return 1;

    case SC_GTOD: { time_t t = time(NULL); struct tm *lt = localtime(&t);
                    AC[0] = lt->tm_sec; AC[1] = lt->tm_min; AC[2] = lt->tm_hour;
                    return 1; }
    case SC_GDAY: { time_t t = time(NULL); struct tm *lt = localtime(&t);
                    AC[0] = lt->tm_mday; AC[1] = lt->tm_mon + 1; AC[2] = lt->tm_year;
                    return 1; }

    case SC_OPEN: case SC_SOPEN:
        if (verbose) {
            int q; fprintf(stderr, "   [packet %05X:", pkt);
            for (q = 0; q < 16; q++) fprintf(stderr, " %04X", M[MADDR(pkt + q)]);
            fprintf(stderr, "  ifnp=%08X]\n", pkt_dw(pkt, P_IFNP));
        }
        getbstr(pkt_dw(pkt, P_IFNP), name, sizeof name);
        for (ch = 1; ch < 64 && chan[ch]; ch++) ;
        if (ch >= 64) return 0;
        if (name[0] == 64) { chan[ch] = stdout; chan_console[ch] = 1; }
        else {
            int wants_out = (M[MADDR(pkt + P_ISTI)] & OFOT) != 0;
            save_path(path, sizeof path, name);
            /* An open for output creates the file when it is not there: a
             * save is the first thing a game writes, and AOS/VS would have
             * had ?CREATE called for it, or ?OPEN with the create bit.  The
             * new file goes in the save directory, never beside the data. */
            if (wants_out && file_exists(path)) chan[ch] = fopen(path, "r+b");
            else if (wants_out)                 chan[ch] = fopen(path, "w+b");
            else { read_path(path, sizeof path, name); chan[ch] = fopen(path, "rb"); }
            vr_forget(ch);
            chan_pos[ch] = 0; chan_spos[ch] = 0;
            /* A ?ORFX channel is opened with its record length in ?IRCL, and
             * a read may then leave ?IRCL as -1 to mean "one whole record":
             * FERRET.PR's file package builds its read packet from a template
             * of all ones and only overwrites ?IRCL when it wants less than a
             * record.  Remember what the open declared so -1 has an answer. */
            chan_recl[ch] = (int)M[MADDR(pkt + P_IRCL)];
            if (chan_recl[ch] == 0xFFFF) chan_recl[ch] = 0;
            /* ?OPEN also names a buffer, and it is the channel's own: a
             * later ?READ that leaves ?IBAD at -1 is asking to be read into
             * it rather than into a buffer of its own.  FERRET.PR's file
             * package depends on that.  It allocates a buffer at open, hands
             * the address to ?OPEN, and then, whenever the caller wants less
             * than a whole record, issues the read with the packet's ?IBAD
             * still the template's fill and copies out of that same buffer
             * afterwards.  Without the rule the read has nowhere to go. */
            chan_ibad[ch] = pkt_dw(pkt, P_IBAD);
            if ((((chan_ibad[ch] >> 1) & OFFMASK)) >= MEMWORDS) chan_ibad[ch] = 0;
            if (!chan[ch]) {
                if (verbose) fprintf(stderr, "[open failed: %s]\n", name);
                return 0;
            }
            chan_console[ch] = 0;
        }
        M[MADDR(pkt + P_ICH)] = (word)ch;
        if (verbose) fprintf(stderr, "[open %-16s -> channel %d]\n", name, ch);
        return 1;

    case SC_CLOSE: case SC_SCLOSE:
        ch = M[MADDR(pkt + P_ICH)] & 63;
        if (chan[ch] && !chan_console[ch]) fclose(chan[ch]);
        chan[ch] = NULL;
        vr_forget(ch);
        chan_recl[ch] = 0; chan_ibad[ch] = 0;
        return 1;

    case SC_SPOS: {
        long pos = (((long)AC[0] << 16) | (AC[1] & 0xFFFF));
        int rc;
        ch = M[MADDR(pkt + P_ICH)] & 63;
        rc = M[MADDR(pkt + P_IRCL)];
        if (!chan[ch] || chan_console[ch]) return 0;
        if (rc <= 0) rc = 1;
        if (fseek(chan[ch], pos * rc, SEEK_SET)) return 0;
        chan_pos[ch] = pos; chan_spos[ch] = 1;
        return 1;
    }

    case SC_GPOS:
        ch = M[MADDR(pkt + P_ICH)] & 63;
        if (!chan[ch] || chan_console[ch]) return 0;
        AC[0] = (dword)(chan_pos[ch] >> 16);
        AC[1] = (dword)(chan_pos[ch] & 0xFFFF);
        return 1;

    case SC_CREATE:
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        { FILE *f = fopen(path, "wb"); if (!f) return 0; fclose(f); }
        return 1;

    case SC_DELETE:
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        return remove(path) == 0;
    }
    return do_rw32(code, pkt);
}

/* ?READ and ?WRITE.  Same rules as the 16-bit shim: data sensitive records
 * carry their delimiter and it counts towards the returned length, and a
 * pending ?SPOS decides where a fixed record goes rather than the record
 * number in the packet. */
static int do_rw32(word code, dword pkt)
{
    char buf[8192];
    int n = 0, fmt, rcl, ch, k, delim = -1;

    if (code != SC_READ && code != SC_WRITE) return -1;
    ch  = M[MADDR(pkt + P_ICH)] & 63;
    /* A packet whose buffer pointer is still the fill pattern is a program
     * that has not finished building it -- FERRET.PR gets here with ?IBAD of
     * -1 because its file package never got a buffer out of the allocator.
     * Taking that literally writes eight kilobytes through the top of memory
     * and wraps into ring page zero, which destroys the stack registers and
     * hides the real fault; the error return is both safer and truer. */
    if ((((pkt_dw(pkt, P_IBAD) >> 1) & OFFMASK)) >= MEMWORDS && chan_ibad[ch]) {
        dword b = chan_ibad[ch];
        M[MADDR(pkt + P_IBAD)]     = (word)(b >> 16);
        M[MADDR(pkt + P_IBAD + 1)] = (word)b;
    }
    if ((((pkt_dw(pkt, P_IBAD) >> 1) & OFFMASK)) >= MEMWORDS) {
        if (verbose)
            fprintf(stderr, "   [%s ch=%d refused: ?IBAD = %08X]\n",
                    code == SC_READ ? "read" : "write", ch,
                    pkt_dw(pkt, P_IBAD));
        return 0;
    }
    fmt = rec_format(pkt, ch);
    rcl = M[MADDR(pkt + P_IRCL)];
    /* A record length of zero is a legal empty record, not "unspecified":
     * ZORK.PR names an empty word when it does not recognise one and asks to
     * write nought bytes of it.  A length of -1 is the packet template's fill
     * pattern showing through, and is refused for the same reason ?IBAD of -1
     * is: clamping it to the buffer size moves eight kilobytes through a
     * pointer the program meant for a few hundred, and the damage surfaces
     * far away.  A line read is exempt -- the delimiter ends it long before
     * the count does. */
    if (rcl == 0xFFFF && chan_recl[ch] > 0) rcl = chan_recl[ch];
    if (rcl == 0xFFFF && rec_format(pkt, ch) != RF_DS) {
        if (verbose)
            fprintf(stderr, "   [%s ch=%d refused: ?IRCL = -1]\n",
                    code == SC_READ ? "read" : "write", ch);
        return 0;
    }
    if (rcl < 0 || rcl > (int)sizeof buf) rcl = (int)sizeof buf;

    if (code == SC_READ) {
        if (!chan[ch]) return 0;
        if (fmt == RF_DS) {
            int c = 0;
            if (chan_console[ch]) {
                /* Flush the prompt before blocking on the keyboard -- see the
                 * note in the 16-bit cpu.c.  Without it the "-> " Ferret
                 * writes stays in stdio's buffer and appears after the line
                 * the player typed. */
                fflush(stdout);
                while (n < rcl - 1) {
                    c = fgetc(stdin);
                    if (c == EOF) { if (!n) { halted = 1; return 0; } break; }
                    if (c == 13) continue;
                    if (c == 10) break;
                    buf[n++] = (char)c;
                }
                buf[n++] = 10;
            } else {
                while ((c = fgetc(chan[ch])) == 0) ;
                if (c == EOF) return 0;
                while (n < rcl - 1 && c != EOF && !is_delim(c)) {
                    buf[n++] = (char)c; c = fgetc(chan[ch]);
                }
                buf[n++] = (c == EOF) ? 10 : (char)c;
            }
        } else {
            size_t got;
            if (chan_console[ch]) return 0;
            /* ?ISTI bit 2 -- PARU calls it ?IPST, "record positioning type
             * (1 = absolute)" -- says whether the record number in the
             * packet is an absolute record or a displacement from where the
             * channel already is.  ZORK.PR reads its heap with the bit clear
             * and the number zero, meaning "the next record", over and over;
             * treating that as absolute re-reads the first 512 bytes for
             * ever and the heap never loads. */
            long rec = ((long)M[MADDR(pkt + P_IRNH)] << 16) |
                                M[MADDR(pkt + P_IRNL)];
            /* The framing is a property of the file, not of what the
             * program declared, and the scan proves itself before it is
             * believed -- so ask it of every channel, not just the ones
             * opened ?ORVR.  FERRET.PR's heaps fail it on their first four
             * bytes and keep the flat reading. */
            if (!chan_vscan[ch]) vr_scan(ch);
            if (chan_vscan[ch] > 0) {
                long want = chan_spos[ch] ? chan_pos[ch]
                          : (M[MADDR(pkt + P_ISTI)] & IPST) ? rec
                          : chan_pos[ch] + rec;
                long avail;
                chan_spos[ch] = 0;
                if (want < 0 || want >= chan_vnrec[ch]) return 0;
                if (fseek(chan[ch], chan_vrec[ch][2 * want], SEEK_SET)) return 0;
                avail = chan_vrec[ch][2 * want + 1];
                if (avail > rcl) avail = rcl;
                got = fread(buf, 1, (size_t)avail, chan[ch]);
                if (!got) return 0;
                n = (int)got;
                chan_pos[ch] = want + 1;
            } else {
            if (!chan_spos[ch]) {
                long off = rec * (long)rcl;
                if (M[MADDR(pkt + P_ISTI)] & IPST) {
                    if (fseek(chan[ch], off, SEEK_SET)) return 0;
                } else if (off && fseek(chan[ch], off, SEEK_CUR)) return 0;
            }
            chan_spos[ch] = 0;
            got = fread(buf, 1, (size_t)rcl, chan[ch]);
            if (!got) return 0;
            n = (int)got;
            chan_pos[ch] = ftell(chan[ch]) / (rcl ? rcl : 1);
            }
        }
        putbstr(pkt_dw(pkt, P_IBAD), buf, n);
        M[MADDR(pkt + P_IRLR)] = (word)n;
        if (verbose) {
            int q;
            fprintf(stderr, "   [read ch=%d rcl=%d rec=%lu ibad=%08X fmt=%d]\n",
                    ch, rcl,
                    (unsigned long)(((long)M[MADDR(pkt + P_IRNH)] << 16) |
                                     M[MADDR(pkt + P_IRNL)]),
                    pkt_dw(pkt, P_IBAD), fmt);
            fprintf(stderr, "   [read ch=%d n=%d <", ch, n);
            for (q = 0; q < n && q < 60; q++)
                fprintf(stderr, (buf[q] >= 32 && buf[q] < 127) ? "%c" : "<%02X>",
                        (unsigned char)buf[q]);
            fprintf(stderr, ">]\n");
        }
        return 1;
    }

    getbytes(pkt_dw(pkt, P_IBAD), buf, rcl);
    n = rcl;
    if (fmt != RF_DS && !chan_console[ch] && chan[ch] && !chan_spos[ch]) {
        long off = (((long)M[MADDR(pkt + P_IRNH)] << 16) |
                            M[MADDR(pkt + P_IRNL)]) * (long)rcl;
        if (M[MADDR(pkt + P_ISTI)] & IPST) {
            if (fseek(chan[ch], off, SEEK_SET)) return 0;
        } else if (off && fseek(chan[ch], off, SEEK_CUR)) return 0;
    }
    chan_spos[ch] = 0;
    if (fmt == RF_DS) {
        for (k = 0; k < rcl; k++)
            if (is_delim((unsigned char)buf[k]))
                { delim = (unsigned char)buf[k]; n = k; break; }
        if (delim > 0) n++;
    }
    if (verbose) {
        int q; fprintf(stderr, "   [write ch=%d fmt=%d n=%d rcl=%d ibad=%08X <", ch, fmt, n, (int)M[MADDR(pkt + P_IRCL)], pkt_dw(pkt, P_IBAD));
        for (q = 0; q < n && q < 60; q++)
            fprintf(stderr, (buf[q] >= 32 && buf[q] < 127) ? "%c" : "<%02X>",
                    (unsigned char)buf[q]);
        fprintf(stderr, ">]\n");
    }
    if (chan[ch]) fwrite(buf, 1, (size_t)n, chan[ch]);
    if (fmt == RF_DS && delim < 0 && chan[ch]) fputc(10, chan[ch]);
    M[MADDR(pkt + P_IRLR)] = (word)n;
    return 1;
}

static const char *scname(word c)
{
    switch (c) {
    case SC_OPEN: return "?OPEN"; case SC_CLOSE: return "?CLOSE";
    case SC_READ: return "?READ"; case SC_WRITE: return "?WRITE";
    case SC_RETURN: return "?RETURN"; case SC_MEM: return "?MEM";
    case SC_MEMI: return "?MEMI"; case SC_GCHR: return "?GCHR";
    case SC_SCHR: return "?SCHR"; case SC_SOPEN: return "?SOPEN";
    case SC_SCLOSE: return "?SCLOSE"; case SC_ERMSG: return "?ERMSG";
    case SC_IFPU: return "?IFPU"; case SC_GTOD: return "?GTOD";
    case SC_GDAY: return "?GDAY"; case SC_SPOS: return "?SPOS";
    case SC_GPOS: return "?GPOS"; case SC_TERM: return "?TERM";
    case SC_GTNAM: return "?GTNAM"; case SC_RNGPR: return "?RNGPR";
    case SC_UIDSTAT: return "?UIDSTAT"; case SC_CREATE: return "?CREATE";
    case SC_DELETE: return "?DELETE"; case SC_GTMES: return "?GTMES";
    default: return "?";
    }
}

/* The kernel gate: XWDO to a ring 3 address inside the .SYSTM thunk, exactly
 * as on the 16-bit machine.  The call number is the word the return address
 * in AC3 points at, and the caller resumes one word further on for an error
 * and two for success -- the thunk does that bump itself on real hardware. */
static void gate_call(dword at)
{
    word code = M[MADDR(AC[3] & OFFMASK)];
    int r;
    if (verbose)
        fprintf(stderr, "  syst %3u (0%03o) %-9s at %05X  AC0=%08X AC1=%08X AC2=%08X\n",
                code, code, scname(code), AC[3] & OFFMASK, AC[0], AC[1], AC[2]);
    r = do_syscall32(code);
    if (r < 0) {
        fprintf(stderr, "\n*** unimplemented system call %u (0%03o) %s at %05X\n",
                code, code, scname(code), AC[3] & OFFMASK);
        halted = 1;
        return;
    }
    /* Do NOT touch the carry.  The runtime's system-call trampoline sets it
     * with CRYTO before the call and clears it with CRYTZ on the word the
     * *success* return lands on, so the carry is how the caller learns
     * whether the call failed; the error return skips the CRYTZ by bumping
     * the return address.  Clearing it here makes every failure look like a
     * success -- FERRET.PR then reads the error code from ?GTMES as the
     * length of an argument it was never given. */
    PC = (at + (r ? 5 : 4)) & OFFMASK;
}
