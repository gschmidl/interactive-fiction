/* host.h -- QUEST as it was played: one server, and a player at every
 * terminal.
 *
 * QUP.CLI started QUEST_SERVER once, and then anyone at a D200 could type
 * QUEST.  Here the emulator plays both parts.  It runs the server, listens
 * on a TCP port, and gives each player who connects a D200 of their own
 * (d200.h) and a QUEST process of their own, loaded from the same .PR --
 * exactly as each X QUEST did.  The player at this window, if there is one,
 * is simply the first of them, on the console instead of a socket.
 *
 * The game needs nothing else: after logon every player's process works
 * straight on the shared world through the mapped files, and the server
 * hears of a player leaving from the obituary AOS/VS sent it.  What the
 * host adds is plumbing -- the keys each terminal typed, a wall clock for
 * the game's pauses, one logon at a time (quest.h), and saving everyone's
 * character when the window is closed.
 *
 *   --port <n>    multiplayer.  If a world is already listening on that
 *                 port on this computer, this program joins it instead.
 *   --lan         listen on every network, not just this computer.
 *   --join <host>[:port]  be a player's terminal for a world elsewhere.
 *   --server      run the world with no player at this window.
 *   --god         the player at this window cannot die (quest.h); with
 *                 --server, nobody who joins can.
 *   --exit-when-empty  with --server: stop when the last player has left.
 */
#ifndef HOST_H
#define HOST_H

static int  host_port, host_lan, host_dedicated, host_exit_empty;
static int  host_god;                      /* --god                         */
static const char *host_client_pr;         /* QUEST.PR, for every player    */
static const char *host_intro;             /* -c: the title typed first     */
static int  host_players_ever;
static int  host_local_gone;               /* the player here has left      */
static int  host_ansi;                     /* this window is a console      */
static int  host_shown = -1;               /* player count last shown       */
static char host_where[96];                /* how others join               */
static volatile long host_stop, host_stopped; /* Ctrl-C, closing the window */
static int  host_stopping;
static unsigned long long host_stop_t0, host_next_poll;
static int  host_next_pid = 3;

static void host_poll(void);

/* A line on the host's console -- only when no game is being shown there. */
static void host_say(const char *fmt, ...)
{
    char b[256];
    va_list ap;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (!host_dedicated && !host_local_gone) return;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (host_ansi) printf("\r\x1b[K");
    printf("[%02d:%02d] %s\n", lt->tm_hour, lt->tm_min, b);
    fflush(stdout);
    host_shown = -1;
}

static int host_players(void)
{
    int pi, n = 0;
    for (pi = 1; pi < nproc; pi++) if (proc[pi].alive) n++;
    return n;
}

/* The window's title counts the players and says how to join; without a
 * game on the screen the bottom line says it too. */
static void host_status(void)
{
    int n = host_players();
    if (n == host_shown || !host_ansi) return;
    host_shown = n;
    printf("\x1b]0;Quest - %d player%s - others join with: %s\x07",
           n, n == 1 ? "" : "s", host_where);
    if (host_dedicated)
        printf("\r\x1b[K  %d player%s in the world.  Ctrl-C stops it.", n, n == 1 ? "" : "s");
    else if (host_local_gone && n > 0)
        printf("\r\x1b[K  %d other player%s still in the world.  Keep this window open"
               " until they have left.", n, n == 1 ? " is" : "s are");
    fflush(stdout);
}

static int host_pid(void)
{
    int tries, pi;
    for (tries = 0; tries < 300; tries++) {
        int pid = host_next_pid++;
        if (host_next_pid > 250) host_next_pid = 3;
        for (pi = 0; pi < nproc; pi++)
            if (proc[pi].alive && proc[pi].pid == pid) break;
        if (pi >= nproc) return pid;
    }
    return 3;
}

/* A new QUEST process for terminal `tix`.  proc_start works on the machine
 * registers and the shim's globals, so whatever is running is filed away
 * first and put back after. */
static int host_spawn(int tix)
{
    int pi, old = curproc, running = nproc > 0;
    Term *was = T;
    for (pi = 1; pi < MAXPROC; pi++)
        if (!proc[pi].alive && pi != curproc) break;
    if (pi >= MAXPROC) return -1;
    if (running) { task_save(); shim_save(old); }
    {
        word *mem = proc[pi].mem;
        memset(&proc[pi], 0, sizeof proc[pi]);
        proc[pi].mem = mem;
    }
    if (load_pr(host_client_pr, pi) == 0) {
        proc_start(pi, "QUEST", host_pid(), 0);
        proc[pi].term = tix;
        term[tix].proc = pi;
        if (term[tix].god) god_enable(pi);
        if (pi >= nproc) nproc = pi + 1;
        host_players_ever++;
    } else {
        pi = -1;
    }
    if (running) { curproc = old; M = proc[old].mem; shim_load(old); task_load(); }
    T = was;
    return pi;
}

static void host_start_player(int tix)
{
    Term *t = &term[tix];
    if (host_spawn(tix) >= 0) return;
    if (t->kind == TK_SOCKET) {
        static const char full[] = "\r\nQuest has no room for another player.\r\n";
        net_send(t->conn, full, (int)sizeof full - 1);
        net_close(t->conn);
    }
    term_free(t);
}

/* QUEST.CLI in the 1984 build typed CASTLE -- an animation -- between ROLL
 * DISABLE and ROLL ENABLE before X QUEST, at the terminal's line speed.
 * Each player's terminal gets it at 19200 baud, fed a little at every poll,
 * so nobody else waits while it draws.  Any key skips to the end. */
static void host_intro_start(int tix)
{
    Term *t = &term[tix], *was = T;
    FILE *f = fopen(host_intro, "rb");
    long len;
    if (!f) { host_start_player(tix); return; }
    fseek(f, 0, SEEK_END); len = ftell(f); rewind(f);
    t->intro = (char *)malloc((size_t)len + 1);
    if (!t->intro || (long)fread(t->intro, 1, (size_t)len, f) != len) {
        fclose(f);
        free(t->intro);
        t->intro = NULL;
        host_start_player(tix);
        return;
    }
    fclose(f);
    t->intro_len = len;
    t->intro_pos = 0;
    t->intro_t0 = net_now();
    T = t;
    d2_write("\223\n", 2);                      /* WRITE [!ascii 223]: roll off */
    T = was;
}

static void host_intro_step(int tix, unsigned long long now)
{
    Term *t = &term[tix], *was = T;
    long due = (long)((now - t->intro_t0) * 1920 / 1000);
    if (t->kq_n) {                              /* a key skips to the end -- */
        due = t->intro_len;                     /* that key, and only that  */
        t->kq_head = (t->kq_head + 1) % KQ;     /* one: anything typed after */
        t->kq_n--;                              /* it is for the game        */
    }
    if (due > t->intro_len) due = t->intro_len;
    T = t;
    if (due > t->intro_pos) {
        d2_write(t->intro + t->intro_pos, (int)(due - t->intro_pos));
        t->intro_pos = due;
    }
    if (t->intro_pos >= t->intro_len) {
        d2_write("\222\n", 2);                  /* WRITE [!ascii 222]: roll on  */
        v_show(1);
        free(t->intro);
        t->intro = NULL;
        T = was;
        host_start_player(tix);
        return;
    }
    T = was;
}

static void host_connect(int conn, const char *peer)
{
    /* telnet: we echo, no go-aheads, no line mode -- a key at a time */
    static const char hello[] = { (char)255, (char)251, 1, (char)255, (char)251, 3,
                                  (char)255, (char)254, 34 };
    int tix = term_new(TK_SOCKET);
    Term *t, *was = T;
    if (tix < 0) {
        static const char full[] = "\r\nQuest has no room for another player.\r\n";
        net_send(conn, full, (int)sizeof full - 1);
        net_close(conn);
        return;
    }
    t = &term[tix];
    t->conn = conn;
    snprintf(t->peer, sizeof t->peer, "%s", peer);
    if (host_dedicated && host_god) t->god = 1;    /* a --server --god world */
    net_send(conn, hello, (int)sizeof hello);
    T = t;
    t_puts("\x1b]0;Quest\x07");
    t_flush();
    T = was;
    host_say("a player has joined from %s", peer);
    if (host_intro) host_intro_start(tix);
    else            host_start_player(tix);
}

/* A player's QUEST has ended -- ESC, or the terminal hung up.  Called from
 * q_proc_exit, which has already sent the server its obituary. */
static void host_proc_ended(int pi)
{
    int tix = proc[pi].term;
    Term *t;
    proc[pi].term = -1;
    if (pi == 0) {
        host_say("the server has stopped");
        host_stop = 1;
        return;
    }
    if (tix < 0) return;
    t = &term[tix];
    t->proc = -1;
    T = t;
    term_done();
    if (t->kind == TK_SOCKET) {
        t_puts(host_stopping ?
               "\r\n  The world has been stopped.  Your character is saved.\r\n" :
               "\r\n  Your character is saved.  Goodbye.\r\n");
        t_flush();
        net_close(t->conn);
        host_say("the player from %s has left", t->peer);
    } else if (t->kind == TK_CONSOLE) {
        host_local_gone = 1;
        printf("\n  Your character is saved.\n\n");
        fflush(stdout);
        host_shown = -1;
    }
    term_free(t);
    T = &term_null;
}

/* Ctrl-C or closing the window: every player's line is hung up, so each
 * QUEST ends at its next read the way a dropped terminal ended it, and the
 * server saves every character; then the world is written back. */
static void host_begin_stop(void)
{
    int k;
    host_stopping = 1;
    host_stop_t0 = net_now();
    net_unlisten();
    host_say("stopping the world -- saving everyone's character");
    for (k = 1; k < MAXTERM; k++) {
        Term *t = &term[k];
        if (!t->used) continue;
        t->hungup = 1;
        if (t->proc >= 0) {
            q_wake(W_KEY, (dword)k);
        } else {
            if (t->kind == TK_SOCKET) net_close(t->conn);
            term_free(t);
        }
    }
}

static void host_poll(void)
{
    unsigned char buf[4096];
    char peer[64];
    unsigned long long now = net_now();
    int k, c, n;

    if (host_stop && !host_stopping) host_begin_stop();
    if (!host_stopping)
        while ((c = net_accept(peer, sizeof peer)) >= 0) host_connect(c, peer);

    for (k = 1; k < MAXTERM; k++) {
        Term *t = &term[k];
        if (!t->used) continue;
        if (t->kind == TK_CONSOLE) {
            int keys[64], i;
            n = con_poll(keys, 64);
            for (i = 0; i < n; i++) term_key(t, console_key(keys[i]));
        } else if (t->kind == TK_SOCKET && !t->hungup) {
            while ((n = net_recv(t->conn, buf, sizeof buf)) > 0) {
                int i;
                for (i = 0; i < n; i++) sock_byte(t, buf[i]);
            }
            if (n < 0) t->hungup = 1;
            term_esc_due(t, now);
            if (t->god && t->proc >= 0 && !proc[t->proc].god) god_enable(t->proc);
        }
        if (t->intro && !host_stopping) host_intro_step(k, now);
        if (!t->used) continue;
        if (t->proc < 0 && !t->intro && t->hungup) {     /* left before playing */
            if (t->kind == TK_SOCKET) net_close(t->conn);
            host_say("the player from %s has left", t->peer);
            term_free(t);
            continue;
        }
        if (t->proc >= 0 && term_ready(t)) q_wake(W_KEY, (dword)k);
        if (t->kind == TK_SOCKET && t->ob_n) {
            Term *was = T;
            T = t;
            t_flush();
            T = was;
        }
    }
    net_service();
    host_status();
}

static int host_finished(void)
{
    int pi, k;
    for (pi = 1; pi < nproc; pi++) if (proc[pi].alive) return 0;
    for (k = 1; k < MAXTERM; k++) if (term[k].used) return 0;
    if (host_stopping) return 1;
    if (host_dedicated && !host_exit_empty) return 0;
    return host_players_ever > 0;
}

/* Nothing at all can run.  Wait for a key, a player, or a pause to end;
 * return 0 when the world should stop.  Every task that could run has had
 * its turn, so the server has heard every obituary by now. */
static int host_idle(void)
{
    long long ms = 1000;
    unsigned long long now;
    int pi, ti, k, console = 0;

    host_poll();
    now = net_now();
    if (host_finished()) return 0;
    if (host_stopping && now - host_stop_t0 > 5000) return 0;
    for (pi = 0; pi < nproc; pi++)
        for (ti = 0; ti < MAXTASK; ti++) {
            Task *t = &proc[pi].task[ti];
            if (eligible(pi, ti, W_NONE)) ms = 0;
            else if (eligible(pi, ti, W_DELAY) && t->rt) {
                long long left = (long long)(t->wake_ms - now);
                if ((long long)t->wake_ms <= (long long)now) left = 0;
                if (left < ms) ms = left;
            }
        }
    for (k = 1; k < MAXTERM; k++) {
        if (!term[k].used) continue;
        if ((term[k].esc == 1 || term[k].intro) && ms > 15) ms = 15;
        if (term[k].kind == TK_CONSOLE) console = 1;
    }
    if ((host_stopping || net_closing()) && ms > 50) ms = 50;
    if (ms > 0) {
        net_wait((int)ms, console);
        host_poll();
    }
    return 1;
}

#endif /* HOST_H */
