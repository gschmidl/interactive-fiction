/* quest.h -- the AOS/VS calls QUEST.PR and QUEST_SERVER.PR need that the
 * single-process games did not: shared files, the shared partition, IPC,
 * connections and the multitask scheduler.
 *
 * Included by syscall32.h, which calls q_syscall() before its own switch.
 * A return of -2 means "not one of mine, carry on"; -3 means "this task
 * cannot proceed yet", and the gate winds the PC back so the call is simply
 * re-executed when the task next runs.
 *
 * Every routine named in a comment here is the program's own: QUEST shipped
 * its linker symbol table, so the shape of each call could be read off the
 * code that makes it rather than guessed.
 *
 *   INIT_SHARED_DATA       ?GSHPT, then ?SSHPT to grow the partition by 150
 *   ?OPEN_SHARED_IO_FILE   ?SOPEN with a byte pointer, NOT a packet
 *   ?GET_SHARED_PAGE       ?SPAGE with the block-I/O packet of PARU.32.SR
 *   ?LOOKUP_PORT           ?ILKUP on the name "QUEST"
 *   ?CONNECT               ?CON to the server's PID, which the server left
 *                          in the shared data at SD_PTR+42
 *   LOGON                  ?IS.R -- send a request, wait for the reply
 */
#ifndef QUEST_H
#define QUEST_H

#define SC_ISEND    21
#define SC_IREC     22
#define SC_ILKUP    23
#define SC_INTWT    14
#define SC_PNAME    78
#define SC_DADID    87
#define SC_ISR      98
#define SC_SCLOSE_Q 113
#define SC_CON     119
#define SC_DCON    120
#define SC_SERVE   121
#define SC_UPDATE  154
#define SC_SIGNL   165
#define SC_WTSIG   166
#define SC_WDELAY  179
#define SC_RECREATE 222
#define SC_DEBUG   226
#define SC_TASK    320
#define SC_SUS     322
#define SC_PRI     323
#define SC_KILL    324
#define SC_KILAD   325
#define SC_IDRDY   330
#define SC_IDSUS   331
#define SC_IDKIL   332
#define SC_IDPRI   333
#define SC_IDGOTO  334
#define SC_RECNW   338
#define SC_XMT     339
#define SC_XMTW    340
#define SC_REC     341
#define SC_ERSCH   342
#define SC_DRSCH   343
#define SC_DFRSCH  360

#define ERNEF 052    /* IPC message exceeds buffer length */
#define ERIVP 053    /* invalid port number               */
#define ERNAE 026    /* file name already exists          */
#define ERIDP 057    /* illegal destination port          */

/* IPC message header, PARU.32.SR */
#define Q_ISFL  0
#define Q_IUFL  1
#define Q_IDPH  2
#define Q_IDPL  3
#define Q_IOPN  4
#define Q_IOPH  2
#define Q_IOPL  3
#define Q_IDPN  4
#define Q_ILTH  5
#define Q_IPTR  6
#define Q_IRLT  9
#define Q_IRPT 10

static char schan_name[NCHAN][64];
static int  schan_open[NCHAN];
static int  schan_file[NCHAN];

/* ---- writable copies -------------------------------------------------- *
 * data/ holds the files exactly as they came off the tape and the game
 * writes to most of them, so anything opened for update is copied into the
 * save directory the first time it is touched and used from there.
 * Deleting the save directory is what "reset the world" means -- which is
 * what QUP.CLI did on the real machine by deleting QUEST.OUT and letting
 * the server build everything again. */
static int copy_file(const char *from, const char *to)
{
    FILE *a = fopen(from, "rb"), *b;
    char buf[65536];
    size_t n;
    if (!a) return 0;
    b = fopen(to, "wb");
    if (!b) { fclose(a); return 0; }
    while ((n = fread(buf, 1, sizeof buf, a)) > 0) fwrite(buf, 1, n, b);
    fclose(a); fclose(b);
    return 1;
}

static void writable_path(char *o, size_t n, const char *nm)
{
    char src[512];
    save_path(o, n, nm);
    if (file_exists(o)) return;
    snprintf(src, sizeof src, "%s/%s", datadir_buf, nm);
    if (file_exists(src)) copy_file(src, o);
}

/* ---- the shared files ------------------------------------------------- *
 * Each file is held as words in the machine's order, and every 512-byte
 * block carries the value of sf_clock when it last changed.  A switch then
 * costs only what changed: going out, a process's window is compared with
 * the file block by block and only the blocks it wrote are copied back (and
 * stamped); coming in, only blocks stamped since that window last looked are
 * copied.  A process that is not running cannot have changed its copy, so
 * that is exactly the old copy-everything-both-ways -- which, with the
 * 1100-page WORLD_DATA_FILE mapped by every player, was two megabytes each
 * way on every switch, and with several players switching often, most of
 * the emulator's time. */
static unsigned long long sf_clock = 1;

static int sf_find(const char *nm)
{
    char path[512];
    FILE *f;
    int i;
    long k, nb;
    unsigned char *bytes;
    for (i = 0; i < MAXSFILE; i++)
        if (sfile[i].used && !strcmp(sfile[i].name, nm)) return i;
    for (i = 0; i < MAXSFILE && sfile[i].used; i++) ;
    if (i >= MAXSFILE) return -1;
    writable_path(path, sizeof path, nm);
    f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END); sfile[i].len = ftell(f); rewind(f);
    /* Room to grow: the program maps more blocks than some of these files
     * hold (150 pages over a 504-block SHARED_DATA_FILE), and on the real
     * machine the rest was simply new file. */
    sfile[i].cap = (sfile[i].len + 0x100000 + 511) / 512;
    sfile[i].w   = (word *)calloc((size_t)sfile[i].cap * 256, sizeof(word));
    sfile[i].ver = (unsigned long long *)calloc((size_t)sfile[i].cap, sizeof(unsigned long long));
    bytes = (unsigned char *)calloc(1, (size_t)sfile[i].len + 2);
    if (!sfile[i].w || !sfile[i].ver || !bytes) { fclose(f); free(bytes); return -1; }
    nb = (long)fread(bytes, 1, (size_t)sfile[i].len, f);
    /* a short read is not fatal: the tail is zeros either way */
    fclose(f);
    for (k = 0; k < (nb + 1) / 2; k++)
        sfile[i].w[k] = (word)((bytes[2 * k] << 8) | bytes[2 * k + 1]);
    free(bytes);
    snprintf(sfile[i].name, sizeof sfile[i].name, "%s", nm);
    sfile[i].used = 1;
    return i;
}

static void sf_flush(int i)
{
    char path[512];
    FILE *f;
    unsigned char *bytes;
    long k;
    if (!sfile[i].used || !sfile[i].dirty) return;
    save_path(path, sizeof path, sfile[i].name);
    bytes = (unsigned char *)malloc((size_t)sfile[i].len + 2);
    if (!bytes) return;
    for (k = 0; k < (sfile[i].len + 1) / 2; k++) {
        bytes[2 * k]     = (unsigned char)(sfile[i].w[k] >> 8);
        bytes[2 * k + 1] = (unsigned char)sfile[i].w[k];
    }
    f = fopen(path, "r+b");
    if (!f) f = fopen(path, "wb");
    if (f) {
        fwrite(bytes, 1, (size_t)sfile[i].len, f);
        fclose(f);
        sfile[i].dirty = 0;
    }
    free(bytes);
}

static void sf_flush_all(void)
{ int i; for (i = 0; i < MAXSFILE; i++) sf_flush(i); }

/* Copy one mapping between a process's memory and the file image.  `out`
 * means memory -> file.  Called round a process switch, and that is what
 * makes the mapping behave like real shared memory: the process that is
 * not running is not looking.  A window that has just been mapped (`full`)
 * takes every block. */
static void map_sync(Mapping *m, word *mem, int out)
{
    SFile *f = &sfile[m->sf];
    long b, nb = m->nblk;
    if (m->blk + nb > f->cap) nb = f->cap - m->blk;
    if (nb <= 0) return;
    if ((m->blk + nb) * 512L > f->len) f->len = (m->blk + nb) * 512L;
    if (out && m->ro) return;
    for (b = 0; b < nb; b++) {
        long fb = m->blk + b;
        word *fw = f->w + fb * 256;
        dword a = (m->addr + (dword)(b * 256)) & (MEMWORDS - 1);
        if (a + 256 > MEMWORDS) {                   /* wraps: word by word */
            int k, differs = 0;
            for (k = 0; k < 256; k++) {
                word *mw = &mem[MADDR(a + (dword)k)];
                if (out) { if (*mw != fw[k]) { fw[k] = *mw; differs = 1; } }
                else if (m->full || f->ver[fb] > m->seen) *mw = fw[k];
            }
            if (differs) { f->ver[fb] = ++sf_clock; f->dirty = 1; }
            continue;
        }
        if (out) {
            if (memcmp(mem + a, fw, 256 * sizeof(word))) {
                memcpy(fw, mem + a, 256 * sizeof(word));
                f->ver[fb] = ++sf_clock;
            }
        } else if (m->full || f->ver[fb] > m->seen) {
            memcpy(mem + a, fw, 256 * sizeof(word));
        }
    }
    /* Written back is dirty whether or not a word changed, as it always was:
     * a file the game has mapped beyond its end is saved at its new length. */
    if (out) f->dirty = 1;
    else { m->seen = sf_clock; m->full = 0; }
}

static void maps_sync(int p, word *mem, int out)
{
    int i;
    for (i = 0; i < MAXMAP; i++)
        if (mapping[i].used && mapping[i].proc == p) map_sync(&mapping[i], mem, out);
}

/* ---- IPC -------------------------------------------------------------- */
/* A zero-length message carries its payload in the HEADER.  That is not a
 * guess: QUEST_SERVER's IPC_TASK builds its reply at 17B23D-17B241 by
 * storing into ?IUFL and ?IPTR and setting ?ILTH to nought, and LOGON in
 * QUEST reads the answer straight back out of ?IPTR at 175F3D --
 * PLAYER_NUM is the low half of it.  So a delivered message writes ?IUFL
 * back always, and ?ILTH and ?IPTR when there is no data to conflict with
 * them; ?IPTR is the receiver's own buffer pointer whenever there is. */
static int mq_post(dword dst, dword src, word sfl, word ufl, dword iptr,
                   const word *data, int len)
{
    int i;
    for (i = 0; i < MAXMSG && mq[i].used; i++) ;
    if (i >= MAXMSG || len > MSGWORDS) return -1;
    mq[i].used = 1; mq[i].dst = dst; mq[i].src = src;
    mq[i].sflags = sfl; mq[i].uflags = ufl; mq[i].len = len;
    mq[i].iptr = iptr;
    if (len > 0) memcpy(mq[i].data, data, (size_t)len * sizeof(word));
    q_wake(W_IREC, dst);
    q_wake(W_ISR, dst);
    return i;
}

static int mq_take(dword dst)
{
    int i;
    for (i = 0; i < MAXMSG; i++)
        if (mq[i].used && mq[i].dst == dst) return i;
    return -1;
}

static dword pk_dw(dword p, unsigned o)
{ return ((dword)M[MADDR(p + o)] << 16) | M[MADDR(p + o + 1)]; }
static void  pk_setdw(dword p, unsigned o, dword v)
{ M[MADDR(p + o)] = (word)(v >> 16); M[MADDR(p + o + 1)] = (word)v; }

/* Copy a message body out of the queue and into the packet's buffer.
 * ?ILTH is a count of WORDS and ?IPTR a word address, ring bits and all. */
static void msg_to_buf(int mi, dword buf, int room, dword pkt, unsigned lenoff)
{
    int n = mq[mi].len;
    int k;
    M[MADDR(pkt + Q_IUFL)] = mq[mi].uflags;
    if (n == 0) {
        M[MADDR(pkt + Q_ILTH)] = 0;
        pk_setdw(pkt, Q_IPTR, mq[mi].iptr);
        return;
    }
    if (room >= 0 && n > room) n = room;
    for (k = 0; k < n; k++) M[MADDR((buf & OFFMASK) + (dword)k)] = mq[mi].data[k];
    M[MADDR(pkt + lenoff)] = (word)n;
}

static void ipc_dump(const char *what, dword pkt)
{
    int k;
    if (!verbose) return;
    fprintf(stderr, "   [%s pkt %05X:", what, pkt);
    for (k = 0; k < 12; k++) fprintf(stderr, " %04X", M[MADDR(pkt + (dword)k)]);
    fputc(']', stderr); fputc(10, stderr);
}

static Task *task_by_id(int id)
{
    int ti;
    for (ti = 0; ti < MAXTASK; ti++)
        if (proc[curproc].task[ti].used && proc[curproc].task[ti].id == id)
            return &proc[curproc].task[ti];
    return NULL;
}

/* A player's process that ends leaves its files closed, as AOS/VS closes
 * them.  In a multiplayer game players come and go for hours, and every one
 * of them opens USER_DATA_FILE. */
static void shim_close_all(void)
{
    int ch;
    for (ch = 0; ch < NCHAN; ch++) {
        if (chan[ch] && !chan_console[ch]) fclose(chan[ch]);
        chan[ch] = NULL;
        chan_console[ch] = 0;
        vr_forget(ch);
        schan_open[ch] = 0;
    }
}

/* ---- one logon at a time ------------------------------------------------
 * A player logs on with three requests to the server over ?IS.R, the same in
 * both QUESTs: 9 asks for a player number, 1 checks the name and password,
 * 2 makes a new character.  QUEST_SERVER hands out numbers 1..10 by counting
 * (the count is kept in SHARED_DATA_FILE, so it runs on across sessions) and
 * after ten reuses any slot whose in-use bit is clear -- but request 9 does
 * not set that bit.  Request 1 sets it on a good login (17AC43) and request
 * 2 on a new character (17B22E).  Two players logging on together once ten
 * numbers have gone out are therefore handed the same slot, and then share
 * one character: fifteen scripted players did exactly that.  For a new
 * character the window stays open while "Do you wish to create this
 * character?" waits for an answer.
 *
 * So the emulator lets one logon through at a time: from a player's request
 * 9 until the reply that marks the slot, or until that player's process ends.
 * A player who arrives meanwhile waits in the ?IS.R, with a note on the
 * bottom line of their screen. */
static int logon_holder = -1;
static unsigned long long logon_since;

static int logon_may_begin(int pi)
{
    if (logon_holder < 0 || logon_holder == pi) return 1;
    if (!proc[logon_holder].alive) { logon_holder = -1; return 1; }
    return 0;
}

static void logon_begin(int pi)
{
    logon_holder = pi;
    logon_since = netmode ? net_now() : 0;
}

static void logon_end(int pi)
{
    if (logon_holder != pi) return;
    logon_holder = -1;
    q_wake(W_GATE, 0);
}

/* The waiting player's bottom line.  The logon screen uses the top few
 * rows, and the game is held while the note is up, so nothing else can be
 * drawn over it or scroll it away. */
static void logon_note(int pi, int on)
{
    static const char msg[] = "  Another player is logging on -- one moment...";
    Term *t, *was = T;
    if (proc[pi].term < 0) return;
    t = &term[proc[pi].term];
    if (!term_interactive(t) || t->note == on) return;
    T = t;
    memset(t->vch[VR - 1], ' ', VC);
    memset(t->vat[VR - 1], 0, VC);
    if (on) memcpy(t->vch[VR - 1], msg, sizeof msg - 1);
    t->note = on;
    t->dirty = 1;
    v_show(0);
    T = was;
}

/* ---- god mode (--god) -----------------------------------------------------
 * For testing a world: the player's strength, maximum strength,
 * intelligence, experience, vision, perception and wealth are set high every
 * time the game reads a command, and dying does not happen.
 *
 * The values are the authors' own.  SETDAVE.CLI, SETJEFF.CLI and SETBERT.CLI
 * run FED to give their characters strength 1024 and wealth 20000; QUEST
 * sets up the operator with intelligence and experience 10000, perception 5
 * and vision 4, and 4 is also the world's own limit on vision.
 *
 * Where the values live was read off each build's DISPLAY_INVENTORY, which
 * compares each one with the copy it last showed: the player's slot is at
 * SD_PTR + slot size x PLAYER_NUM, and the values sit a fixed distance before
 * it -- confirmed by setting them and watching the panel.  The in-use bit is
 * the one IPC_TASK tests (17A6AC, 17BB67), and god mode waits for it, so that
 * nothing is written before the server has put the character in the slot.
 *
 * Cannot die: DIED builds its frame with its first instruction and takes it
 * down with its only WRTN, so for a god the emulator goes from one straight
 * to the other and the caller carries on as though death had passed by. */
#define GOD_STRENGTH  1024
#define GOD_WEALTH   20000
#define GOD_INTEL    10000
#define GOD_EXP      10000
#define GOD_VISION       4
#define GOD_PERCEP       5

typedef struct {
    const char *name;
    dword sd_ptr, player_num;           /* the client's SD_PTR, PLAYER_NUM  */
    int   slot, inuse;
    int   intel, exp, str, maxstr, vision, percep, wealth;
    dword died, died_body, died_wrtn;
} GodBuild;

static const GodBuild god_builds[] = {
    { "QUEST (NADGUG)", 0x210, 0x216, 686, -591,
      -379, -378, -377, -376, -375, -374, -372, 0x16603D, 0x16603F, 0x1663BA },
    { "QUEST (1984)",   0x1F4, 0x1FA, 434, -339,
      -226, -225, -224, -223, -222, -221, -219, 0x16DD4A, 0x16DD4C, 0x16E000 },
};
#define NGODBUILD ((int)(sizeof god_builds / sizeof god_builds[0]))

static int god_any;                     /* some process is a god: look at PCs */

/* Which build the program just loaded into M is: DIED's WSAVS and WRTN
 * where that build has them. */
static int god_identify(void)
{
    int k;
    for (k = 0; k < NGODBUILD; k++)
        if (M[MADDR(god_builds[k].died)] == 0xA739 &&
            M[MADDR(god_builds[k].died_wrtn)] == 0x87A9)
            return k;
    return -1;
}

static void god_enable(int pi)
{
    static int warned;
    if (proc[pi].god) return;
    if (proc[pi].god_build < 0) {
        if (!warned++)
            fprintf(stderr, "quest: --god knows the NADGUG and 1984 QUEST.PR only\n");
        return;
    }
    proc[pi].god = 1;
    god_any = 1;
}

/* Set the running player's values, once the server has logged them on. */
static void god_apply(void)
{
    const GodBuild *g;
    dword sd, slot;
    int pn;
    if (!proc[curproc].god) return;
    g = &god_builds[proc[curproc].god_build];
    sd = (((dword)M[MADDR(g->sd_ptr)] << 16) | M[MADDR(g->sd_ptr + 1)]) & OFFMASK;
    pn = (int16_t)M[MADDR(g->player_num)];
    if (!sd || pn < 1 || pn > 10) return;
    slot = sd + (dword)(g->slot * pn);
    if (!(M[MADDR(slot + (dword)g->inuse)] & 0x8000)) return;
    M[MADDR(slot + (dword)g->str)]    = GOD_STRENGTH;
    M[MADDR(slot + (dword)g->maxstr)] = GOD_STRENGTH;
    M[MADDR(slot + (dword)g->intel)]  = GOD_INTEL;
    M[MADDR(slot + (dword)g->exp)]    = GOD_EXP;
    M[MADDR(slot + (dword)g->vision)] = GOD_VISION;
    M[MADDR(slot + (dword)g->percep)] = GOD_PERCEP;
    M[MADDR(slot + (dword)g->wealth)] = GOD_WEALTH;
}

/* Called before every instruction once there is a god anywhere. */
static void god_check_died(void)
{
    const GodBuild *g;
    if (!proc[curproc].god) return;
    g = &god_builds[proc[curproc].god_build];
    if (PC != g->died_body) return;
    if (verbose) fprintf(stderr, "   [--god: %s does not die]\n", proc[curproc].name);
    god_apply();
    PC = g->died_wrtn;
}

/* ---- the player's console ----------------------------------------------
 * Reads and writes on @INPUT/@OUTPUT when the console is a D200.  The
 * interesting part is the extended packet ?READ_SCREEN builds: ?ISTI has
 * ?IPKL set, ?ETSP (packet+16) points at a three-word screen management
 * packet -- ?ESFC flags, ?ESEP edit position, ?ESCR <column><row> -- and
 * the flags ask for the things a form-filling read needs:
 *
 *   ?ESCP 0x0800  put the cursor at ?ESCR first
 *   ?ESRP 0x0200  hand the cursor position back in ?ESCR afterwards
 *   ?ESNE 0x0100  do not echo what is typed
 *   ?ESED 0x1000  do not echo the delimiter
 *
 * A binary read (?IBIN) is a raw keystroke: no echo, no delimiters, one
 * byte per count.  "Hit any character to continue" is one of those. */
static FILE *server_console(const char *nm)
{
    char path[512];
    if (!strcmp(nm, "@INPUT")) return tmpfile();
    save_path(path, sizeof path, "QUEST.OUT");
    return fopen(path, "ab");
}

static int d2_io(word code, dword pkt, int fmt, int rcl)
{
    word sti = M[MADDR(pkt + P_ISTI)];
    dword sp = 0;
    word esfc = 0;
    char buf[8192];
    int n = 0;
    Term *t = T;

    if (sti & 0x8000u) {
        sp = pk_dw(pkt, 16) & OFFMASK;
        if (sp && sp < MEMWORDS) esfc = M[MADDR(sp)];
        else sp = 0;
    }
    if (rcl < 0 || rcl > (int)sizeof buf) rcl = (int)sizeof buf;

    if (code == SC_WRITE) {
        getbytes(pk_dw(pkt, P_IBAD), buf, rcl);
        n = rcl;
        if (fmt == RF_DS) {
            int k;
            for (k = 0; k < rcl; k++)
                if (is_delim((unsigned char)buf[k])) { n = k + 1; break; }
        }
        if (sp && (esfc & 0x0800u)) {
            word cr = M[MADDR(sp + 2)];
            T->vx = (cr >> 8) % VC; T->vy = (cr & 0xFF) % VR;
        }
        if (verbose) {
            int q;
            fprintf(stderr, "   [console write %d at %d,%d esfc=%04X: ", n, T->vy, T->vx, esfc);
            for (q = 0; q < n && q < 200; q++) {
                int ch = (unsigned char)buf[q];
                if (ch >= 32 && ch < 127) fputc(ch, stderr);
                else fprintf(stderr, "<%03o>", ch);
            }
            fputs("]", stderr); fputc(10, stderr);
        }
        d2_write(buf, n);
        M[MADDR(pkt + P_IRLR)] = (word)n;
        if (sp && (esfc & 0x0200u)) M[MADDR(sp + 2)] = (word)((T->vx << 8) | T->vy);
        return 1;
    }

    /* A read.  For a scripted player, or the one player at a console, let
     * everything else run first: that keyboard is the one wait that stops
     * the whole machine, and scripted players take turns at their keys.  A
     * player in a multiplayer game only waits for their own keys, and the
     * others carry on meanwhile. */
    if (!term_interactive(t)) {
        if (nproc > 1 && !key_turn && (q_others_runnable() || q_others_want_key())) {
            q_block(W_KEY, 0, 0);
            return -3;
        }
        key_turn = 0;
    }

    /* The read begins: place the cursor and show the screen.  A read that
     * has to wait for keys is executed again when they come, and it must
     * carry on rather than begin again -- the echo has moved the cursor on,
     * and the line so far is kept in the terminal. */
    if (!t->reading) {
        if (sp && (esfc & 0x0800u)) {
            word cr = M[MADDR(sp + 2)];
            t->vx = (cr >> 8) % VC; t->vy = (cr & 0xFF) % VR;
            t->dirty = 1;
        }
        if (verbose)
            fprintf(stderr, "   [console read sti=%04X fmt=%d rcl=%d esfc=%04X at %d,%d]\n",
                    sti, fmt, rcl, esfc, t->vy, t->vx);
        god_apply();
        t->dirty = 1;
        v_show(1);
        t->reading = 1;
        t->line_n = 0;
    }

    if ((sti & 0x1000u) || fmt != RF_DS) {
        while (n < rcl) {
            int c = kb_get();
            if (c == -1 || c == -3) break;
            if (c < 0) continue;
            buf[n++] = (char)c;
            /* one key -- or all of a cursor report the terminal sends --
             * and a byte at a time from a pipe or a file */
            if (!t->gen_n && (t->kb_console || term_interactive(t))) break;
            if (!t->kb_console && !term_interactive(t)) break;
        }
        if (n == 0 && term_interactive(t) && !t->hungup) {
            q_block(W_KEY, (dword)(t - term), 0);
            return -3;
        }
    } else {
        int echo = !(esfc & 0x0100u), echo_delim = !(esfc & 0x1000u);
        for (;;) {
            int c = kb_get();
            if (c == -3) { q_block(W_KEY, (dword)(t - term), 0); return -3; }
            if (c == -1) break;
            if (c < 0) continue;
            if (c == 0177) {
                if (t->line_n > 0) {
                    t->line_n--;
                    if (echo) { d2_putc(031); d2_putc(' '); d2_putc(031); v_show(0); }
                }
                continue;
            }
            if (c == 012 || c == 015 || c == 014 || c == 0) {
                t->line[t->line_n++] = (char)c;
                if (echo && echo_delim && c == 012) { d2_putc(012); v_show(0); }
                break;
            }
            if (t->line_n < rcl - 1 && t->line_n < (int)sizeof t->line - 1) {
                t->line[t->line_n++] = (char)c;
                if (echo && c >= 040) { d2_putc(c); v_show(0); }
            }
        }
        n = t->line_n;
        memcpy(buf, t->line, (size_t)n);
    }
    t->reading = 0;
    t->line_n = 0;
    if (n == 0) {
        /* The end of the input is the terminal hanging up: the player's
         * process is terminated, and its server hears about it the same way
         * it would for ESC -- so a scripted session saves its character. */
        term_done();
        if (nproc > 1 && curproc > 0) { q_proc_exit(); return 1; }
        halted = 1;
        AC[0] = EREOF;
        return 0;
    }
    putbstr(pk_dw(pkt, P_IBAD), buf, n);
    M[MADDR(pkt + P_IRLR)] = (word)n;
    if (sp && (esfc & 0x0200u)) M[MADDR(sp + 2)] = (word)((t->vx << 8) | t->vy);
    if (verbose) {
        int q;
        fprintf(stderr, "   [console got %d:", n);
        for (q = 0; q < n && q < 40; q++) fprintf(stderr, " %02X", (unsigned char)buf[q]);
        fprintf(stderr, "]\n");
    }
    return 1;
}

/* ---- the calls -------------------------------------------------------- */
static int q_syscall(word code, dword pkt)
{
    char name[288];
    int ch, i;

    switch (code) {

    /* ?SOPEN: AC0 is a byte pointer to the pathname, AC1 is -1 and AC2 the
     * caller's channel wish.  Not the ?OPEN packet -- ?OPEN_SHARED_IO_FILE
     * builds no packet at all, it null-terminates the F77 character
     * argument on the stack and hands over the byte pointer.  The channel
     * comes back in AC1, not AC0: the routine files AC0, AC1 and AC2 at
     * FP+2, FP+4 and FP+6 after the call and its success path returns
     * FP+4.  AC0 is the error code, which is what its failure path passes
     * to ?LIB_ERROR_CODE. */
    case SC_SOPEN: {
        int sf;
        getbstr(AC[0], name, sizeof name);
        if (!name[0]) { AC[0] = ERFDE; return 0; }
        sf = sf_find(name);
        if (sf < 0) {
            if (verbose) fprintf(stderr, "[sopen failed: %s]\n", name);
            AC[0] = ERFDE;
            return 0;
        }
        for (ch = 1; ch < NCHAN && (chan[ch] || schan_open[ch]); ch++) ;
        if (ch >= NCHAN) { AC[0] = ERMEM; return 0; }
        schan_open[ch] = 1; schan_file[ch] = sf;
        snprintf(schan_name[ch], sizeof schan_name[ch], "%s", name);
        if (verbose)
            fprintf(stderr, "[sopen %-20s -> channel %d (%ld bytes)]\n",
                    name, ch, sfile[sf].len);
        AC[1] = (dword)ch;
        return 1;
    }

    /* ?SCLOSE takes the channel in AC0, not a packet. */
    case SC_SCLOSE_Q:
        ch = (int)(AC[0] & (NCHAN - 1));
        if (!schan_open[ch]) return -2;         /* the packet form: not mine */
        for (i = 0; i < MAXMAP; i++)
            if (mapping[i].used && mapping[i].proc == curproc &&
                mapping[i].sf == schan_file[ch]) {
                map_sync(&mapping[i], M, 1);
                mapping[i].used = 0;
            }
        sf_flush(schan_file[ch]);
        schan_open[ch] = 0;
        if (verbose) fprintf(stderr, "[sclose channel %d]\n", ch);
        return 1;

    /* ?SPAGE: AC1 is the channel and AC2 the block-I/O packet of
     * PARU.32.SR -- ?PSTI count right / status left, ?PCAD+?PCDL the word
     * address, ?PRNH+?PRNL the block number.  Counts are in 512-byte
     * blocks, four to a 1024-word page, which is why every count
     * ?GET_SHARED_PAGE builds is a multiple of four.  ?SPRO, bit 0 of the
     * status half, asks for read-only.
     *
     * The address is not always in the shared partition.  QUEST maps
     * SHARED_DATA_FILE into the partition it just grew, but WORLD_DATA_FILE
     * and CASTLE_DATA_FILE go straight over its own F77 COMMON: words
     * 17C00..12AC00 of the .PR are 1126400 words of nothing, and that is
     * exactly the 1100 pages it asks for. */
    case SC_SPAGE: {
        dword p = AC[2] & OFFMASK;
        unsigned sti = M[MADDR(p + 0)];
        long cnt = (long)(sti & 0x7FFF);
        dword addr = pk_dw(p, 2) & OFFMASK;
        long blk = ((long)M[MADDR(p + 4)] << 16) | M[MADDR(p + 5)];
        int ro = (sti & 0x8000) != 0;
        ch = (int)(AC[1] & (NCHAN - 1));

        if (verbose)
            fprintf(stderr, "   [spage ch=%d %s blk=%ld cnt=%ld -> %06X%s]\n",
                    ch, schan_name[ch], blk, cnt, addr, ro ? " ro" : "");
        if (!schan_open[ch] || cnt <= 0) { AC[0] = ERFDE; return 0; }
        for (i = 0; i < MAXMAP && mapping[i].used; i++) ;
        if (i >= MAXMAP) { AC[0] = ERMEM; return 0; }
        mapping[i].used = 1; mapping[i].proc = curproc;
        mapping[i].sf = schan_file[ch]; mapping[i].ro = ro;
        mapping[i].addr = addr; mapping[i].blk = blk; mapping[i].nblk = cnt;
        mapping[i].full = 1; mapping[i].seen = 0;
        map_sync(&mapping[i], M, 0);
        return 1;
    }

    /* ?ILKUP: AC0 is a byte pointer to the port name, and the global port
     * number comes back in AC1 -- ?LOOKUP_PORT returns FP+4 the same way
     * ?OPEN_SHARED_IO_FILE does.  LOGON looks up "QUEST" and keeps the
     * answer in GLOBAL_PORT_NUM at word 0x218. */
    case SC_ILKUP:
        getbstr(AC[0], name, sizeof name);
        { char *e = name + strlen(name);
          while (e > name && e[-1] == ' ') *--e = 0; }
        for (i = 0; i < MAXPORT; i++)
            if (portname[i].used && !strcmp(portname[i].name, name)) {
                AC[1] = portname[i].port;
                if (verbose) fprintf(stderr, "   [ilkup %s -> %08X]\n",
                                     name, AC[1]);
                return 1;
            }
        if (verbose) fprintf(stderr, "   [ilkup %s -- no such port]\n", name);
        AC[0] = ERIVP;
        return 0;

    /* ?SERVE makes this process one customers may connect to.  The name it
     * registers is the process's own; QUEST_SERVER is started as
     * "quest.server" but looks itself up as "QUEST", so the port is
     * registered under both. */
    case SC_SERVE:
        for (i = 0; i < MAXPORT && portname[i].used; i++) ;
        if (i >= MAXPORT) { AC[0] = ERMEM; return 0; }
        portname[i].used = 1;
        snprintf(portname[i].name, sizeof portname[i].name, "QUEST");
        portname[i].port = (dword)proc[curproc].pid << 16;
        if (verbose)
            fprintf(stderr, "   [serve: port QUEST = %08X]\n", portname[i].port);
        return 1;

    /* ?CON / ?DCON: connect to and disconnect from a server by PID.  With
     * one server and one player there is nothing to arrange. */
    case SC_CON:
        if (verbose) fprintf(stderr, "   [connect to pid %u]\n", AC[0]);
        proc[curproc].con_pid = (int)(AC[0] & 0xFFFF);
        return 1;
    case SC_DCON:
        if (verbose) fprintf(stderr, "   [disconnect pid %u]\n", AC[0]);
        return 1;

    /* ?ISEND: AC2 is the packet.  ?IDPH/?IDPL are the destination's global
     * port number and ?IOPN the sender's local port; ?ILTH words at ?IPTR
     * are the message. */
    case SC_ISEND: {
        dword dst = pk_dw(pkt, Q_IDPH);
        dword src = ((dword)proc[curproc].pid << 16) | M[MADDR(pkt + Q_IOPN)];
        int len = (int)M[MADDR(pkt + Q_ILTH)];
        dword buf = pk_dw(pkt, Q_IPTR) & OFFMASK;
        word body[MSGWORDS];
        int k;
        if (len > MSGWORDS) len = MSGWORDS;
        for (k = 0; k < len; k++) body[k] = M[MADDR(buf + (dword)k)];
        ipc_dump("isend", pkt);
        if (verbose)
            fprintf(stderr, "   [isend %08X -> %08X, %d words]\n", src, dst, len);
        if (mq_post(dst, src, M[MADDR(pkt + Q_ISFL)],
                    M[MADDR(pkt + Q_IUFL)], pk_dw(pkt, Q_IPTR), body, len) < 0)
            { AC[0] = ERMEM; return 0; }
        return 1;
    }

    /* ?IREC: receive on this process's local port ?IDPN.  With nothing
     * waiting the task blocks and the call is re-executed when a message
     * arrives -- unless ?IFNBK (bit 2 of the system flags) asks for an
     * error instead. */
    case SC_IREC: {
        dword me = ((dword)proc[curproc].pid << 16) | M[MADDR(pkt + Q_IDPN)];
        int mi = mq_take(me);
        ipc_dump("irec in", pkt);
        if (mi < 0) {
            if (M[MADDR(pkt + Q_ISFL)] & 0x2000) { AC[0] = ERNEF; return 0; }
            q_block(W_IREC, me, 0);
            return -3;
        }
        msg_to_buf(mi, pk_dw(pkt, Q_IPTR), (int)M[MADDR(pkt + Q_ILTH)],
                   pkt, Q_ILTH);
        pk_setdw(pkt, Q_IOPH, mq[mi].src);
        if (verbose)
            fprintf(stderr, "   [irec %08X <- %08X, %d words]\n",
                    me, mq[mi].src, mq[mi].len);
        mq[mi].used = 0;
        return 1;
    }

    /* ?IS.R is send-and-wait: the same header as ?ISEND, plus ?IRLT and
     * ?IRPT for where the reply goes.  It is done in two halves, and the
     * task is left blocked between them.  The re-execution rule means the
     * send must not happen twice, so "sent" is a flag of its own in the
     * task -- not the wait state, which the wake-up clears.  Keying it off
     * the wait state sent every request twice, and the server, handed a
     * second LOGON from the same port, answered the password check with
     * "This player was previously killed off!" for every player there
     * was. */
    case SC_ISR: {
        dword dst = pk_dw(pkt, Q_IDPH);
        dword me  = ((dword)proc[curproc].pid << 16) | M[MADDR(pkt + Q_IOPN)];
        Task *t = &proc[curproc].task[proc[curproc].cur];
        int mi;
        if (!t->isr_sent) {
            int len = (int)M[MADDR(pkt + Q_ILTH)];
            dword buf = pk_dw(pkt, Q_IPTR) & OFFMASK;
            word body[MSGWORDS];
            word req = M[MADDR(pkt + Q_IUFL)];
            int k;
            /* One player logs on at a time -- see logon_may_begin. */
            if (req == 9 && curproc > 0) {
                if (!logon_may_begin(curproc)) {
                    logon_note(curproc, 1);
                    q_block(W_GATE, 0, 0);
                    return -3;
                }
                logon_note(curproc, 0);
                logon_begin(curproc);
            }
            t->isr_req = req;
            if (len > MSGWORDS) len = MSGWORDS;
            for (k = 0; k < len; k++) body[k] = M[MADDR(buf + (dword)k)];
            ipc_dump("is.r out", pkt);
            if (verbose)
                fprintf(stderr, "   [is.r %08X -> %08X, %d words]\n",
                        me, dst, len);
            if (mq_post(dst, me, M[MADDR(pkt + Q_ISFL)],
                        M[MADDR(pkt + Q_IUFL)], pk_dw(pkt, Q_IPTR),
                        body, len) < 0)
                { AC[0] = ERMEM; return 0; }
            t->isr_sent = 1;
        }
        mi = mq_take(me);
        if (mi < 0) { q_block(W_ISR, me, 0); return -3; }
        t->isr_sent = 0;
        /* The logon is over once the server has marked the player's slot in
         * use: a good login (request 1 answered 0) or a new character
         * (request 2) -- or no slot at all (request 9 answered with player
         * number 0, "Maximum number of players exceeded"). */
        if (logon_holder == curproc &&
            ((t->isr_req == 1 && mq[mi].uflags == 0) || t->isr_req == 2 ||
             (t->isr_req == 9 && (mq[mi].iptr & 0xFFFF) == 0)))
            logon_end(curproc);
        msg_to_buf(mi, pk_dw(pkt, Q_IRPT), (int)M[MADDR(pkt + Q_IRLT)],
                   pkt, Q_IRLT);
        ipc_dump("is.r rep", pkt);
        if (verbose)
            fprintf(stderr, "   [is.r reply %d words]\n", mq[mi].len);
        mq[mi].used = 0;
        return 1;
    }

    /* ?CREATE must REFUSE a file that is already there, the way AOS/VS
     * does -- the server calls it on USER_DATA_FILE at startup and simply
     * ignores the error when the file exists.  Creating it regardless left
     * an empty USER_DATA_FILE in the save directory, and the ?READ that
     * followed hit end of file and took the server into ?FATAL. */
    case SC_CREATE: {
        char path[512], src[512];
        getbstr(AC[0], name, sizeof name);
        save_path(path, sizeof path, name);
        snprintf(src, sizeof src, "%s/%s", datadir_buf, name);
        /* Only a file that came with the game counts as already there.
         * QUEST_SERVER's other ?CREATE is ?CREATE_IPC_FILE, which makes the
         * transient file named after its port; AOS/VS would not have kept
         * that across a reboot, and refusing it stops the server dead. */
        if (file_exists(src)) {
            writable_path(path, sizeof path, name);
            if (verbose) fprintf(stderr, "[create %s: already exists]\n", name);
            AC[0] = ERNAE;
            return 0;
        }
        { FILE *f = fopen(path, "wb"); if (!f) { AC[0] = ERFDE; return 0; }
          fclose(f); }
        if (verbose) fprintf(stderr, "[create %s]\n", name);
        return 1;
    }

    /* ?PNAME and ?DADID: who am I, and who is my father. */
    case SC_PNAME:
        /* AC1 = -1 asks for the caller's own PID, and the answer comes back
         * in AC1: ?CURRENT_PID files AC0 and AC1 at FP+2 and FP+4 after the
         * call and its success path -- the WBR at 17E13E -- returns FP+4. */
        AC[1] = (dword)proc[curproc].pid;
        return 1;
    case SC_DADID:
        AC[0] = 1; AC[1] = 1;
        return 1;

    /* ?UPDATE and ?RECREATE: flush, and truncate-and-keep.  Both are about
     * files the shim already holds whole. */
    case SC_UPDATE:
        sf_flush_all();
        return 1;
    case SC_RECREATE:
        return 1;

    /* ?DEBUG is the SWAT breakpoint; there is nothing to break into. */
    case SC_DEBUG:
        return 1;

    /* ?DRSCH / ?ERSCH / ?DFRSCH: the critical-section brackets round the
     * scheduler.  Nesting them is what stops a switch inside a shared-data
     * update. */
    case SC_DRSCH: proc[curproc].rsched++; return 1;
    case SC_ERSCH:
    case SC_DFRSCH:
        if (proc[curproc].rsched > 0) proc[curproc].rsched--;
        return 1;

    /* ---- tasks ------------------------------------------------------- *
     * The wrappers MT?TASK, MT?REC, MT?XMT ... in QUEST's own runtime say
     * exactly what each call takes, and PARU.32.SR gives the ?TASK packet.
     * All of QUEST's tasks start at the same trampoline: MT?TASK puts
     * 17E784 in ?DPC and the routine the caller asked for in ?DAC2. */
    case SC_TASK: {
        dword p = AC[2] & OFFMASK;
        int pri = M[MADDR(p + 4)];
        int id  = M[MADDR(p + 5)];
        dword pc   = pk_dw(p, 6) & OFFMASK;
        dword ac2  = pk_dw(p, 8);
        dword stb  = pk_dw(p, 10);
        dword ssz  = pk_dw(p, 13);
        int num = M[MADDR(p + 17)];
        int k;
        if (num < 1) num = 1;
        if (verbose)
            fprintf(stderr, "   [task pri=%d id=%d pc=%06X ac2=%08X "
                            "stack=%08X+%X n=%d]\n",
                    pri, id, pc, ac2, stb, ssz, num);
        for (k = 0; k < num; k++) {
            Proc *pr = &proc[curproc];
            int ti;
            Task *t;
            for (ti = 0; ti < MAXTASK && pr->task[ti].used; ti++) ;
            if (ti >= MAXTASK) { AC[0] = ERMEM; return 0; }
            t = &pr->task[ti];
            memset(t, 0, sizeof *t);
            t->used = 1; t->wait = W_NONE;
            t->id = id ? id + k : 0;
            t->pri = pri ? pri : pr->task[pr->cur].pri;
            t->PC = pc;
            t->AC[2] = ac2;
            t->AC[3] = 0;
            /* A task gets its own stack: base, pointer and frame at the
             * bottom, limit at the top.  ?DSTB of -1 means it shares the
             * caller's, which is what the runtime asks for when it does
             * not want to carve one out. */
            if (stb != 0xFFFFFFFFu && (stb & OFFMASK) != 0) {
                dword b0 = stb & OFFMASK;
                t->pz[0] = (word)(stb >> 16); t->pz[1] = (word)stb;   /* WFP */
                t->pz[2] = (word)(stb >> 16); t->pz[3] = (word)stb;   /* WSP */
                t->pz[4] = (word)((RING | ((b0 + ssz) & OFFMASK)) >> 16);
                t->pz[5] = (word)(b0 + ssz);                          /* WSL */
                t->pz[6] = (word)(stb >> 16); t->pz[7] = (word)stb;   /* WSB */
            } else {
                int q; for (q = 0; q < 8; q++) t->pz[q] = M[0x10 + q];
            }
            if (pr->ntask <= ti) pr->ntask = ti + 1;
        }
        return 1;
    }

    /* ?XMT and ?REC pass one doubleword through a mailbox in memory: AC1
     * (?XMT) or AC0 (?REC) is its address, and a non-zero cell means a
     * message is waiting.  MT?REC hands the answer back in AC1. */
    case SC_XMT:
    case SC_XMTW: {
        dword box = AC[1] & OFFMASK;
        pk_setdw(box, 0, AC[0]);
        q_wake(W_TREC, box);
        if (verbose) fprintf(stderr, "   [xmt %08X -> box %06X]\n", AC[0], box);
        return 1;
    }
    case SC_REC:
    case SC_RECNW: {
        dword box = AC[0] & OFFMASK;
        dword v = pk_dw(box, 0);
        if (!v) {
            if (code == SC_RECNW) { AC[0] = ERNEF; return 0; }
            q_block(W_TREC, box, 0);
            return -3;
        }
        pk_setdw(box, 0, 0);
        AC[1] = v;
        if (verbose) fprintf(stderr, "   [rec box %06X -> %08X]\n", box, v);
        return 1;
    }

    /* Suspending, readying, killing and re-prioritising, by task id. */
    /* Suspending, delaying and waiting for a signal are EVENT waits: when
     * the task is woken the call has happened, so the re-execution that
     * resumes it must return rather than wait again.  ?WDELAY without this
     * slept for ever -- the "Hit space bar to attempt a seige" prompt waits
     * with ?DELAY, and each re-run was the same delay starting over. */
    case SC_SUS: {
        Task *t = &proc[curproc].task[proc[curproc].cur];
        if (t->woken) { t->woken = 0; return 1; }
        q_block(W_SUS, (dword)t->id, 0);
        return -3;
    }
    case SC_IDSUS: {
        Task *t = task_by_id((int)(AC[1] & 0xFFFF));
        if (!t) { AC[0] = ERIVP; return 0; }
        if (t == &proc[curproc].task[proc[curproc].cur]) {
            if (t->woken) { t->woken = 0; return 1; }
            q_block(W_SUS, (dword)t->id, 0);
            return -3;
        }
        t->wait = W_SUS; t->wa = (dword)t->id;
        return 1;
    }
    case SC_IDRDY: {
        Task *t = task_by_id((int)(AC[1] & 0xFFFF));
        if (!t) { AC[0] = ERIVP; return 0; }
        if (t->wait == W_SUS) { t->wait = W_NONE; t->woken = 1; }
        return 1;
    }
    case SC_IDKIL: {
        Task *t = task_by_id((int)(AC[1] & 0xFFFF));
        if (!t) { AC[0] = ERIVP; return 0; }
        t->used = 0;
        if (t == &proc[curproc].task[proc[curproc].cur]) { sched_yield(); return 1; }
        return 1;
    }
    case SC_IDPRI: {
        Task *t = task_by_id((int)(AC[1] & 0xFFFF));
        if (!t) { AC[0] = ERIVP; return 0; }
        t->pri = (int)(AC[0] & 0xFFFF);
        return 1;
    }
    case SC_IDGOTO: {
        Task *t = task_by_id((int)(AC[1] & 0xFFFF));
        if (!t) { AC[0] = ERIVP; return 0; }
        t->PC = AC[0] & OFFMASK; t->wait = W_NONE;
        return 1;
    }
    case SC_PRI:
        proc[curproc].task[proc[curproc].cur].pri = (int)(AC[0] & 0xFFFF);
        return 1;
    case SC_KILAD:
        proc[curproc].task[proc[curproc].cur].kilad = AC[0] & OFFMASK;
        return 1;
    case SC_KILL: {
        Proc *pr = &proc[curproc];
        int alive = 0, ti;
        pr->task[pr->cur].used = 0;
        for (ti = 0; ti < MAXTASK; ti++) if (pr->task[ti].used) alive = 1;
        if (verbose) fprintf(stderr, "   [task %d killed]\n", pr->cur);
        if (!alive) { q_proc_exit(); return 1; }
        sched_yield();
        return 1;
    }

    /* ?INTWT waits for the console interrupt key.  Nothing here raises one,
     * so the task that asks simply never runs again -- which is what it
     * would do on a terminal nobody interrupts. */
    case SC_INTWT:
        q_block(W_INTWT, 0, 0);
        return -3;

    /* ?WDELAY is in milliseconds.  There is no wall clock to keep to, so a
     * delay is a number of instructions -- enough to let the other side
     * make progress, which is all any of these waits is for. */
    case SC_WDELAY: {
        Task *t = &proc[curproc].task[proc[curproc].cur];
        if (t->woken) { t->woken = 0; return 1; }
        /* On a real console a delay is a real pause: the game uses them to
         * leave a message on the screen long enough to read ("You have been
         * hit by an arrow from an elven archer") before it redraws.  A
         * scripted run skips them and moves the instruction clock on.
         *
         * In a multiplayer game the pause is this task's alone: it waits on
         * the wall clock while everyone else plays on.  Sleeping here, as the
         * single player's console does, would stop every player's game. */
        if (netmode) {
            unsigned long ms = AC[0] & 0xFFFF;
            v_show(1);
            t->rt = 1;
            t->wake_ms = net_now() + (ms > 3000 ? 3000 : ms);
            q_block(W_DELAY, 0, 0);
            return -3;
        }
        if (term_d200 && term[0].mode == TM_ANSI) {
            unsigned long ms = AC[0] & 0xFFFF;
            v_show(1);
            Sleep(ms > 3000 ? 3000 : ms);
            return 1;
        }
        q_block(W_DELAY, (AC[0] & 0xFFFF) * 100 + 1000, 0);
        return -3;
    }

    /* ?SIGNL / ?WTSIG round LOCK_FILE: one task waits on an address, the
     * other signals it. */
    case SC_SIGNL:
        q_wake(W_SIG, AC[0] & OFFMASK);
        return 1;
    case SC_WTSIG: {
        Task *t = &proc[curproc].task[proc[curproc].cur];
        if (t->woken) { t->woken = 0; return 1; }
        q_block(W_SIG, AC[0] & OFFMASK, 0);
        return -3;
    }

    default:
        return -2;
    }
}

#endif /* QUEST_H */
