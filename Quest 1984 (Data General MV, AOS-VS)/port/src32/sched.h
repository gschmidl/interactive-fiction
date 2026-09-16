/* sched.h -- the cooperative scheduler over processes and tasks.
 *
 * Included by mv32.c after the shim, because saving a process means saving
 * the shim's own state as well as the machine's.
 *
 * Only one task anywhere runs at a time.  A switch happens when a task
 * blocks -- which it does by asking to be re-executed rather than by
 * remembering where it was -- or when its quantum expires, and never while
 * the process holds the scheduler off with ?DRSCH.
 *
 * What belongs to what:
 *   the machine   AC, PC, carry, and the four stack registers, which the
 *                 MV keeps in ring page zero at 0x10..0x17.  Per TASK.
 *   the process   its whole memory image, and the shim's channels, shared
 *                 partition and page counts.
 *   the world     the shared files, the ?SPAGE mappings onto them, the IPC
 *                 message queue and the port names.  One copy, shared.
 */
#ifndef SCHED_H
#define SCHED_H

#define QUANTUM 4000

static void shim_save(int pi)
{
    ShimState *s = &proc[pi].shim;
    memcpy(s->chan, chan, sizeof chan);
    memcpy(s->chan_console, chan_console, sizeof chan_console);
    memcpy(s->chan_pos, chan_pos, sizeof chan_pos);
    memcpy(s->chan_spos, chan_spos, sizeof chan_spos);
    memcpy(s->chan_recl, chan_recl, sizeof chan_recl);
    memcpy(s->chan_ibad, chan_ibad, sizeof chan_ibad);
    memcpy(s->chan_vrec, chan_vrec, sizeof chan_vrec);
    memcpy(s->chan_vnrec, chan_vnrec, sizeof chan_vnrec);
    memcpy(s->chan_vscan, chan_vscan, sizeof chan_vscan);
    memcpy(s->schan_name, schan_name, sizeof schan_name);
    memcpy(s->schan_open, schan_open, sizeof schan_open);
    memcpy(s->schan_file, schan_file, sizeof schan_file);
    s->cur_pages = cur_pages; s->max_pages = max_pages;
    s->shpt_start = shpt_start; s->shpt_pages = shpt_pages;
}

static void shim_load(int pi)
{
    ShimState *s = &proc[pi].shim;
    memcpy(chan, s->chan, sizeof chan);
    memcpy(chan_console, s->chan_console, sizeof chan_console);
    memcpy(chan_pos, s->chan_pos, sizeof chan_pos);
    memcpy(chan_spos, s->chan_spos, sizeof chan_spos);
    memcpy(chan_recl, s->chan_recl, sizeof chan_recl);
    memcpy(chan_ibad, s->chan_ibad, sizeof chan_ibad);
    memcpy(chan_vrec, s->chan_vrec, sizeof chan_vrec);
    memcpy(chan_vnrec, s->chan_vnrec, sizeof chan_vnrec);
    memcpy(chan_vscan, s->chan_vscan, sizeof chan_vscan);
    memcpy(schan_name, s->schan_name, sizeof schan_name);
    memcpy(schan_open, s->schan_open, sizeof schan_open);
    memcpy(schan_file, s->schan_file, sizeof schan_file);
    cur_pages = s->cur_pages; max_pages = s->max_pages;
    shpt_start = s->shpt_start; shpt_pages = s->shpt_pages;
}

static void shim_reset(unsigned bl, unsigned st)
{
    memset(chan, 0, sizeof chan);
    memset(chan_console, 0, sizeof chan_console);
    memset(chan_pos, 0, sizeof chan_pos);
    memset(chan_spos, 0, sizeof chan_spos);
    memset(chan_recl, 0, sizeof chan_recl);
    memset(chan_ibad, 0, sizeof chan_ibad);
    memset(chan_vrec, 0, sizeof chan_vrec);
    memset(chan_vnrec, 0, sizeof chan_vnrec);
    memset(chan_vscan, 0, sizeof chan_vscan);
    memset(schan_name, 0, sizeof schan_name);
    memset(schan_open, 0, sizeof schan_open);
    memset(schan_file, 0, sizeof schan_file);
    cur_pages = bl; max_pages = st;
    shpt_start = SHPT_PAGE0; shpt_pages = 0;
}

static void task_save(void)
{
    Task *t = &proc[curproc].task[proc[curproc].cur];
    int i;
    memcpy(t->AC, AC, sizeof AC);
    t->PC = PC; t->C = C;
    for (i = 0; i < 8; i++) t->pz[i] = M[0x10 + i];
}

static void task_load(void)
{
    Task *t = &proc[curproc].task[proc[curproc].cur];
    int i;
    memcpy(AC, t->AC, sizeof AC);
    PC = t->PC; C = t->C;
    for (i = 0; i < 8; i++) M[0x10 + i] = t->pz[i];
}

static void switch_to(int pi, int ti)
{
    if (pi == curproc && ti == proc[curproc].cur) return;
    task_save();
    if (pi != curproc) {
        shim_save(curproc);
        maps_sync(curproc, M, 1);       /* my pages -> the shared files   */
        curproc = pi;
        M = proc[pi].mem;
        maps_sync(curproc, M, 0);       /* the shared files -> my pages   */
        shim_load(curproc);
        if (verbose)
            fprintf(stderr, "   [-> process %d %s]\n", pi, proc[pi].name);
    }
    proc[pi].cur = ti;
    task_load();
}

static int q_curpid(void) { return proc[curproc].pid; }

static void q_block(int kind, dword a, dword b)
{
    Task *t = &proc[curproc].task[proc[curproc].cur];
    t->wait = kind; t->wa = a; t->wb = b;
    if (kind == W_IREC && curproc == 0 && !server_ready) {
        server_ready = 1;
        if (verbose) fprintf(stderr, "   [server is ready for customers]\n");
    }
    if (kind == W_DELAY) t->wake = icount + (long long)a;
}

static void q_wake(int kind, dword a)
{
    int pi, ti;
    for (pi = 0; pi < nproc; pi++)
        for (ti = 0; ti < MAXTASK; ti++) {
            Task *t = &proc[pi].task[ti];
            if (t->used && t->wait == kind && t->wa == a) {
                t->wait = W_NONE;
                if (kind == W_SIG) t->woken = 1;
            }
        }
}

static int eligible(int pi, int ti, int kind)
{
    if (!proc[pi].alive) return 0;
    if (pi > 0 && !server_ready) return 0;
    return proc[pi].task[ti].used && proc[pi].task[ti].wait == kind;
}

/* Is anything other than the current task able to run? */
static int q_others_runnable(void)
{
    int pi, ti;
    for (pi = 0; pi < nproc; pi++)
        for (ti = 0; ti < MAXTASK; ti++) {
            Task *t = &proc[pi].task[ti];
            if (pi == curproc && ti == proc[curproc].cur) continue;
            if (eligible(pi, ti, W_DELAY) && icount >= t->wake) return 1;
            if (eligible(pi, ti, W_NONE)) return 1;
        }
    return 0;
}

/* Pick the next runnable task, starting just after the current one so that
 * equal claims take turns.  When nothing can run, time is what is missing:
 * a task waiting for a key is let through to the keyboard, and failing that
 * the clock is moved on to the earliest delay. */
static int sched_pick(int *ppi, int *pti)
{
    int n, pi, ti, start, k, pass;
    int slots = nproc * MAXTASK;
    start = curproc * MAXTASK + proc[curproc].cur;
    for (pass = 0; pass < 3; pass++) {
        for (pi = 0; pi < nproc; pi++)
            for (ti = 0; ti < MAXTASK; ti++) {
                Task *t = &proc[pi].task[ti];
                if (t->used && t->wait == W_DELAY && icount >= t->wake)
                    { t->wait = W_NONE; t->woken = 1; }
            }
        for (k = 1; k <= slots; k++) {
            n = (start + k) % slots;
            pi = n / MAXTASK; ti = n % MAXTASK;
            if (eligible(pi, ti, W_NONE)) { *ppi = pi; *pti = ti; return 1; }
        }
        if (pass == 0) {
            for (k = 1; k <= slots; k++) {
                n = (start + k) % slots;
                pi = n / MAXTASK; ti = n % MAXTASK;
                if (eligible(pi, ti, W_KEY)) {
                    proc[pi].task[ti].wait = W_NONE;
                    key_turn = 1;
                    *ppi = pi; *pti = ti;
                    return 1;
                }
            }
        } else if (pass == 1) {
            long long soonest = -1;
            for (pi = 0; pi < nproc; pi++)
                for (ti = 0; ti < MAXTASK; ti++)
                    if (eligible(pi, ti, W_DELAY) &&
                        (soonest < 0 || proc[pi].task[ti].wake < soonest))
                        soonest = proc[pi].task[ti].wake;
            if (soonest < 0) break;
            if (soonest > icount) icount = soonest;
        }
    }
    return 0;
}

static void sched_yield(void)
{
    int pi, ti;
    if (proc[curproc].rsched > 0) return;       /* inside ?DRSCH          */
    if (!sched_pick(&pi, &ti)) {
        Task *t = &proc[curproc].task[proc[curproc].cur];
        if (t->used && t->wait == W_NONE && proc[curproc].alive)
            return;                                 /* only me, carry on  */
        if (session_ending) { halted = 1; return; }
        fprintf(stderr, "\n*** every task is blocked -- deadlock\n");
        { int a, b;
          for (a = 0; a < nproc; a++)
            for (b = 0; b < MAXTASK; b++)
              if (proc[a].task[b].used)
                fprintf(stderr, "    %s task %d: wait %d on %08X PC=%06X\n",
                        proc[a].name, b, proc[a].task[b].wait,
                        proc[a].task[b].wa, proc[a].task[b].PC); }
        halted = 1;
        return;
    }
    switch_to(pi, ti);
}

/* A process ends.  Its pages go back to the shared files and its mappings
 * are dropped -- a dead process's stale window must never be copied over
 * what the living ones write later.
 *
 * If it was a customer, its server is told, the way AOS/VS tells a server
 * about a customer that terminates: a message from system port 8 with the
 * PID in the low byte of ?IUFL.  QUEST_SERVER's IPC_TASK checks for exactly
 * that origin (17A669) and hands the PID to HANDLE_TERM, which finds the
 * player by it, writes the character back and frees the slot.
 *
 * This does not switch tasks itself.  It is called from inside ?RETURN,
 * and the gate would otherwise write the dead task's return PC into
 * whichever task ran next; the caller reschedules. */
static void q_proc_exit(void)
{
    int pi, ti, any = 0, i;
    if (verbose)
        fprintf(stderr, "\n[%s has finished]\n", proc[curproc].name);
    maps_sync(curproc, M, 1);
    for (i = 0; i < MAXMAP; i++)
        if (mapping[i].used && mapping[i].proc == curproc) mapping[i].used = 0;
    sf_flush_all();
    proc[curproc].alive = 0;
    if (curproc == 0) server_ready = 1;
    for (ti = 0; ti < MAXTASK; ti++) proc[curproc].task[ti].used = 0;
    if (proc[curproc].con_pid) {
        mq_post((dword)proc[curproc].con_pid << 16, 8, 0,
                (word)(proc[curproc].pid & 0xFF), 0, NULL, 0);
        session_ending = 1;
        if (verbose)
            fprintf(stderr, "[obituary for pid %d sent to pid %d]\n",
                    proc[curproc].pid, proc[curproc].con_pid);
    }
    for (pi = 0; pi < nproc; pi++) if (proc[pi].alive) any = 1;
    if (!any) { halted = 1; return; }
    if (!strcmp(proc[curproc].name, "QUEST") && !session_ending) halted = 1;
}

#endif /* SCHED_H */
