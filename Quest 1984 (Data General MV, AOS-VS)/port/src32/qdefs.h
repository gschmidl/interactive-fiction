/* qdefs.h -- processes, tasks and the shared world, for QUEST.
 *
 * Quest is two AOS/VS processes.  QUP.CLI starts the server
 *
 *     proc/def/out=quest.out/name=quest.server quest_server
 *
 * and then each player runs QUEST, which finds the server, connects to it
 * and talks to it over IPC while both map the same three data files into
 * their own address spaces with ?SPAGE.
 *
 * Here they are two contexts inside one emulator, run cooperatively: only
 * one task anywhere is executing at a time, and a switch happens when a
 * task blocks or its quantum runs out.  That makes the whole thing
 * deterministic and single-stepping possible, and it makes the shared
 * files exactly coherent -- a process that is not running cannot observe a
 * half-written page, so copying the mapped pages in and out around a
 * switch is equivalent to real shared memory.
 *
 * Included by mv32.c before the shim, which uses these declarations; the
 * scheduler itself is defined in mv32.c after the shim, because it has to
 * save and restore the shim's own state.
 */
#ifndef QDEFS_H
#define QDEFS_H

#define MAXPROC   2
#define MAXTASK  16
#define NCHAN    64

/* Why a task is not runnable.  A blocked task is resumed by re-executing
 * its system call: the gate winds the PC back to the LCALL, so when the
 * scheduler picks the task up again the call simply runs a second time and
 * this time succeeds.  Nothing has to be remembered about a half-finished
 * call. */
enum { W_NONE = 0, W_IREC, W_ISR, W_DELAY, W_SIG, W_INTWT, W_TREC, W_SUS,
       W_DEAD, W_KEY };

/* The keyboard is the one thing that blocks the whole emulator, so a task
 * that wants a key waits (W_KEY) until nothing else anywhere can run, and
 * only then is let through to block on the real keyboard.  key_turn is how
 * the scheduler says "now". */
static int key_turn;

typedef struct {
    dword AC[4], PC;
    int   C;
    word  pz[8];            /* page zero 0x10..0x17: WFP, WSP, WSL, WSB   */
    int   used, id, pri;
    int   wait;             /* one of W_*; W_NONE means runnable          */
    dword wa, wb;           /* what it is waiting for                     */
    dword kilad;            /* ?KILAD: where the task dies                */
    dword tmsg;             /* ?REC/?XMT message value                    */
    int   hastmsg;
    long long wake;         /* W_DELAY: the instruction count to wake at  */
    int   woken;            /* an event wait ended: the re-run succeeds   */
    int   isr_sent;         /* ?IS.R: the request is out, awaiting reply  */
} Task;

/* The shim state that belongs to one process rather than to the machine.
 * Saved and restored round a process switch by shim_save/shim_load in
 * mv32.c, which is why it is a plain blob here. */
typedef struct {
    FILE *chan[NCHAN];
    int   chan_console[NCHAN];
    long  chan_pos[NCHAN];
    int   chan_spos[NCHAN];
    int   chan_recl[NCHAN];
    dword chan_ibad[NCHAN];
    long *chan_vrec[NCHAN];
    long  chan_vnrec[NCHAN];
    int   chan_vscan[NCHAN];
    unsigned cur_pages, max_pages;
    unsigned shpt_start, shpt_pages;
    char  schan_name[NCHAN][64];
    int   schan_open[NCHAN];
    int   schan_file[NCHAN];      /* index into the shared-file table     */
} ShimState;

typedef struct {
    word *mem;
    Task  task[MAXTASK];
    int   ntask, cur;
    int   pid;
    int   alive;
    int   rsched;                 /* ?DRSCH nesting: >0 = do not switch   */
    int   con_pid;                /* the server it ?CONed to, or 0        */
    char  name[32];
    dword entry, ustbl, ustst, ustsz;
    ShimState shim;
} Proc;

static Proc  proc[MAXPROC];
static int   nproc, curproc;

/* QUP.CLI started the server and left it to build the world before any
 * player ran QUEST -- "startup time -> 10 mins !!!", says the macro.  The
 * player must not look for the server's PID in the shared data before the
 * server has put it there, so process 1 does not run at all until process
 * 0 has settled into its first ?IREC, which is the server saying it is
 * ready for customers. */
static int   server_ready;

/* The player has gone and the server is being given the chance to hear
 * about it: once nothing is left to run, the session is over. */
static int   session_ending;

/* ---- shared files ----------------------------------------------------- *
 * The three data files QUEST maps are the medium the two processes share.
 * Each is held once, in memory, and every ?SPAGE mapping is a window onto
 * it; the windows are synchronised at a process switch. */
#define MAXSFILE   8
#define MAXMAP    32

typedef struct {
    int  used;
    char name[64];
    unsigned char *data;
    long len;
    int  dirty;
} SFile;

typedef struct {
    int   used, proc, sf, ro;
    dword addr;                   /* word address in that process         */
    long  blk, nblk;              /* 512-byte blocks                      */
} Mapping;

static SFile   sfile[MAXSFILE];
static Mapping mapping[MAXMAP];

/* ---- IPC -------------------------------------------------------------- *
 * A global port number is (pid << 16) | local port, which is exactly how
 * the packets carry it: ?IDPH/?IDPL and ?IOPH/?IOPL are the high and low
 * words of one 32-bit port number, and ?IOPN/?IDPN the local half alone. */
#define MAXMSG    32
#define MSGWORDS 1024

typedef struct {
    int   used;
    dword dst, src;
    word  sflags, uflags;
    dword iptr;                   /* ?IPTR, carried as data -- see below  */
    int   len;
    word  data[MSGWORDS];
} Msg;

static Msg  mq[MAXMSG];

#define MAXPORT 8
typedef struct { int used; char name[32]; dword port; } PortName;
static PortName portname[MAXPORT];

/* Provided by mv32.c, used by the shim. */
static void q_block(int kind, dword a, dword b);
static void q_wake(int kind, dword a);
static int  q_curpid(void);
static void sched_yield(void);
static void q_proc_exit(void);
static int  q_others_runnable(void);

#endif /* QDEFS_H */
