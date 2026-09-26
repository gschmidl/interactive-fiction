/* net.c -- sockets and the console for QUEST's players.
 *
 * On the MV every player sat at a D200 of their own.  Here a player's
 * terminal is a TCP connection: the emulator listens, and a player joins
 * with this same program run as a terminal (net_join), or with any telnet
 * client.  The byte stream is the D200's screen already turned into ANSI by
 * d200.h, so the joining side has nothing to do but show it and send keys.
 *
 * Everything that needs windows.h is in this file; see net.h for why.
 */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"

#define MAXCONN 32

typedef struct {
    SOCKET s;
    int    used, closing;
    char  *pend;                /* output the socket would not take yet     */
    int    pend_n, pend_cap;
    unsigned long long give_up; /* a closing connection that will not drain */
} Conn;

static Conn     cn[MAXCONN];
static SOCKET   lsock = INVALID_SOCKET;
static WSAEVENT ev = WSA_INVALID_EVENT;
static int      wsa_up;

unsigned long long net_now(void) { return GetTickCount64(); }

static int wsa_init(void)
{
    WSADATA d;
    if (wsa_up) return 0;
    if (WSAStartup(MAKEWORD(2, 2), &d)) return -1;
    ev = WSACreateEvent();
    wsa_up = 1;
    return 0;
}

/* SO_EXCLUSIVEADDRUSE makes a second world on the same port fail to bind
 * instead of quietly sharing it -- which is how a second quest.bat finds out
 * that it should join the first.  With --lan the world listens on every
 * interface, for IPv6 as well as IPv4: a computer's name often resolves to
 * IPv6 addresses first, and a firewall drops a connection to a port nobody
 * listens on, so --join NAME would wait some 20 seconds for each of them
 * before it tried IPv4.  0, 1 when the port is taken, or -1. */
static int bind_listener(int port, int lan, int v6)
{
    union { struct sockaddr_in v4; struct sockaddr_in6 v6; } a;
    int one = 1, zero = 0, len;
    lsock = socket(v6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lsock == INVALID_SOCKET) return -1;
    setsockopt(lsock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&one, sizeof one);
    memset(&a, 0, sizeof a);                    /* IPv6: the address is "any" */
    if (v6) {
        setsockopt(lsock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&zero, sizeof zero);
        a.v6.sin6_family = AF_INET6;
        a.v6.sin6_port = htons((unsigned short)port);
        len = sizeof a.v6;
    } else {
        a.v4.sin_family = AF_INET;
        a.v4.sin_port = htons((unsigned short)port);
        a.v4.sin_addr.s_addr = htonl(lan ? INADDR_ANY : INADDR_LOOPBACK);
        len = sizeof a.v4;
    }
    if (bind(lsock, (struct sockaddr *)&a, len) == SOCKET_ERROR) {
        int e = WSAGetLastError();
        closesocket(lsock);
        lsock = INVALID_SOCKET;
        return (e == WSAEADDRINUSE || e == WSAEACCES) ? 1 : -1;
    }
    return 0;
}

int net_listen(int port, int lan)
{
    int r;
    if (wsa_init()) return -1;
    r = lan ? bind_listener(port, lan, 1) : -1;
    if (r < 0) r = bind_listener(port, lan, 0);     /* not --lan, or no IPv6 here */
    if (r) return r;
    if (listen(lsock, 8) == SOCKET_ERROR) {
        closesocket(lsock);
        lsock = INVALID_SOCKET;
        return -1;
    }
    WSAEventSelect(lsock, ev, FD_ACCEPT);
    return 0;
}

void net_unlisten(void)
{
    if (lsock != INVALID_SOCKET) closesocket(lsock);
    lsock = INVALID_SOCKET;
}

int net_accept(char *peer, int peerlen)
{
    struct sockaddr_storage a;
    int al = sizeof a, i, one = 1;
    SOCKET s;
    if (lsock == INVALID_SOCKET) return -1;
    s = accept(lsock, (struct sockaddr *)&a, &al);
    if (s == INVALID_SOCKET) return -1;
    for (i = 0; i < MAXCONN && cn[i].used; i++) ;
    if (i >= MAXCONN) { closesocket(s); return -1; }
    memset(&cn[i], 0, sizeof cn[i]);
    cn[i].s = s;
    cn[i].used = 1;
    WSAEventSelect(s, ev, FD_READ | FD_WRITE | FD_CLOSE);    /* non-blocking now */
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    if (peer && peerlen > 0) {
        char h[NI_MAXHOST];
        const char *p = h;
        if (getnameinfo((struct sockaddr *)&a, al, h, sizeof h, NULL, 0, NI_NUMERICHOST))
            snprintf(h, sizeof h, "?");
        if (!strncmp(h, "::ffff:", 7) && strchr(h + 7, '.'))
            p = h + 7;                          /* IPv4, through the IPv6 socket */
        snprintf(peer, (size_t)peerlen, "%s", p);
    }
    return i;
}

static Conn *conn_of(int id)
{
    if (id < 0 || id >= MAXCONN || !cn[id].used) return NULL;
    return &cn[id];
}

int net_recv(int id, unsigned char *buf, int max)
{
    Conn *c = conn_of(id);
    int n;
    if (!c || c->closing) return -1;
    n = recv(c->s, (char *)buf, max, 0);
    if (n > 0) return n;
    if (n == 0) return -1;
    return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
}

static int push(Conn *c)
{
    int done = 0;
    while (done < c->pend_n) {
        int k = send(c->s, c->pend + done, c->pend_n - done, 0);
        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) return -1;
            break;
        }
        done += k;
    }
    if (done) {
        memmove(c->pend, c->pend + done, (size_t)(c->pend_n - done));
        c->pend_n -= done;
    }
    return 0;
}

int net_send(int id, const char *buf, int n)
{
    Conn *c = conn_of(id);
    if (!c) return -1;
    if (n > 0) {
        if (c->pend_n + n > c->pend_cap) {
            int cap = c->pend_cap ? c->pend_cap : 16384;
            char *nb;
            while (cap < c->pend_n + n) cap *= 2;
            if (cap > (8 << 20)) return -1;         /* the other end stopped reading */
            nb = (char *)realloc(c->pend, (size_t)cap);
            if (!nb) return -1;
            c->pend = nb;
            c->pend_cap = cap;
        }
        memcpy(c->pend + c->pend_n, buf, (size_t)n);
        c->pend_n += n;
    }
    return push(c);
}

static void drop(Conn *c)
{
    shutdown(c->s, SD_SEND);
    closesocket(c->s);
    free(c->pend);
    memset(c, 0, sizeof *c);
}

void net_close(int id)
{
    Conn *c = conn_of(id);
    if (!c) return;
    if (push(c) < 0 || c->pend_n == 0) { drop(c); return; }
    c->closing = 1;
    c->give_up = net_now() + 5000;
}

int net_closing(void)
{
    int i, n = 0;
    for (i = 0; i < MAXCONN; i++) if (cn[i].used && cn[i].closing) n++;
    return n;
}

void net_service(void)
{
    int i;
    for (i = 0; i < MAXCONN; i++) {
        Conn *c = &cn[i];
        if (!c->used) continue;
        if (push(c) < 0) { if (c->closing) drop(c); continue; }
        if (c->closing && (c->pend_n == 0 || net_now() > c->give_up)) drop(c);
    }
}

/* One event object stands for every socket.  It is reset after the wait and
 * the caller then drains every socket, so anything that arrives afterwards
 * signals it again. */
void net_wait(int ms, int console)
{
    HANDLE h[2];
    DWORD nh = 0;
    if (ms < 0) ms = 0;
    if (ev != WSA_INVALID_EVENT) h[nh++] = ev;
    if (console) h[nh++] = GetStdHandle(STD_INPUT_HANDLE);
    if (nh) WaitForMultipleObjects(nh, h, FALSE, (DWORD)ms);
    else    Sleep((DWORD)ms);
    if (ev != WSA_INVALID_EVENT) WSAResetEvent(ev);
}

/* This computer's name: what a player on another computer gives --join.
 * The first IPv4 address was often the wrong one to show: the virtual
 * switches of WSL, Hyper-V or VirtualBox come before the real network. */
int net_host_name(char *buf, int n)
{
    if (wsa_init() || gethostname(buf, n)) return -1;
    buf[n - 1] = 0;
    return 0;
}

void net_shutdown(void)
{
    int i;
    net_unlisten();
    for (i = 0; i < MAXCONN; i++) if (cn[i].used) drop(&cn[i]);
    if (wsa_up) { WSACloseEvent(ev); WSACleanup(); }
    ev = WSA_INVALID_EVENT;
    wsa_up = 0;
}

/* ---- the console ------------------------------------------------------- */
int con_interactive(void)
{
    DWORD m;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m) &&
           GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &m);
}

void con_ansi(void)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m;
    if (GetConsoleMode(h, &m))
        SetConsoleMode(h, m | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

/* Keys come as console input records rather than through getch, so waiting
 * for one can share a wait with the sockets.  Anything that is not a key
 * press -- focus, mouse, a resize -- is read and dropped here, which is what
 * keeps the input handle from staying signalled. */
int con_poll(int *keys, int max)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD r[64];
    DWORD n, got, i;
    int k = 0;
    if (!GetNumberOfConsoleInputEvents(h, &n) || n == 0) return 0;
    if (n > 64) n = 64;
    if (!ReadConsoleInputA(h, r, n, &got)) return 0;
    for (i = 0; i < got; i++) {
        KEY_EVENT_RECORD *ke;
        int c, rep;
        if (r[i].EventType != KEY_EVENT || !r[i].Event.KeyEvent.bKeyDown) continue;
        ke = &r[i].Event.KeyEvent;
        switch (ke->wVirtualKeyCode) {
        case VK_UP:     c = NK_UP; break;
        case VK_DOWN:   c = NK_DOWN; break;
        case VK_LEFT:   c = NK_LEFT; break;
        case VK_RIGHT:  c = NK_RIGHT; break;
        case VK_HOME:   c = NK_HOME; break;
        case VK_DELETE: c = NK_DEL; break;
        default:        c = (unsigned char)ke->uChar.AsciiChar; break;
        }
        if (c == 0 || (c >= 0x80 && c < 0x100)) continue;
        for (rep = ke->wRepeatCount ? ke->wRepeatCount : 1; rep > 0 && k < max; rep--)
            keys[k++] = c;
    }
    return k;
}

static volatile long *ctrl_stop, *ctrl_stopped;

/* Closing the window gives a program a few seconds before Windows ends it,
 * and only while this handler has not returned: long enough for everyone's
 * character to be saved and the world written back. */
static BOOL WINAPI on_ctrl(DWORD type)
{
    if (ctrl_stop) *ctrl_stop = 1;
    if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT ||
        type == CTRL_SHUTDOWN_EVENT) {
        int i;
        for (i = 0; i < 90 && !(ctrl_stopped && *ctrl_stopped); i++) Sleep(50);
    }
    return TRUE;
}

void net_on_ctrl(volatile long *stop, volatile long *stopped)
{
    ctrl_stop = stop;
    ctrl_stopped = stopped;
    /* A program started from a shell that ignores Ctrl-C inherits that, and
     * then Ctrl-C would never reach the handler below. */
    SetConsoleCtrlHandler(NULL, FALSE);
    SetConsoleCtrlHandler(on_ctrl, TRUE);
}

/* ---- joining: this program as a player's terminal ----------------------- */
static void send_all(SOCKET s, const char *b, int n)
{
    int tries = 0;
    while (n > 0) {
        int k = send(s, b, n, 0);
        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK || ++tries > 200) return;
            Sleep(10);
            continue;
        }
        b += k;
        n -= k;
    }
}

int net_join(const char *host, int port, int god)
{
    struct addrinfo hints, *res = NULL, *ai;
    char ps[16];
    SOCKET s = INVALID_SOCKET;
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE), hout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD outmode;
    int incon, tn = 0, sent_eof = 0;
    volatile long stop = 0, stopped = 0;
    WSAEVENT e;

    if (wsa_init()) return 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(ps, sizeof ps, "%d", port);
    if (getaddrinfo(host, ps, &hints, &res) != 0) {
        fprintf(stderr, "quest: there is no computer called %s\n", host);
        return 1;
    }
    for (ai = res; ai; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        if (connect(s, ai->ai_addr, (int)ai->ai_addrlen) == 0) break;
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "quest: no Quest world is running at %s, port %d\n", host, port);
        return 1;
    }
    if (GetConsoleMode(hout, &outmode))
        SetConsoleMode(hout, outmode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    { DWORD m; incon = GetConsoleMode(hin, &m) != 0; }
    net_on_ctrl(&stop, &stopped);
    e = WSACreateEvent();
    WSAEventSelect(s, e, FD_READ | FD_CLOSE);
    { int one = 1; setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one); }
    if (god) {
        /* --god: ask the world for it -- IAC SB 198 "GOD" IAC SE (d200.h) */
        static const char ask[] = { (char)255, (char)250, (char)198, 'G', 'O', 'D',
                                    (char)255, (char)240 };
        send_all(s, ask, (int)sizeof ask);
    }

    for (;;) {
        HANDLE hs[2];
        DWORD nh = 0;
        char buf[8192], shown[8192];
        int n, alive = 1;

        hs[nh++] = e;
        if (incon) hs[nh++] = hin;
        WaitForMultipleObjects(nh, hs, FALSE, 50);
        WSAResetEvent(e);
        if (stop) break;

        /* The world's side: the screen, less the telnet negotiation. */
        for (;;) {
            int i, m = 0;
            n = recv(s, buf, sizeof buf, 0);
            if (n == 0 || (n < 0 && WSAGetLastError() != WSAEWOULDBLOCK)) { alive = 0; break; }
            if (n < 0) break;
            for (i = 0; i < n; i++) {
                unsigned char b = (unsigned char)buf[i];
                switch (tn) {
                case 1: tn = (b >= 251 && b <= 254) ? 2 : (b == 250) ? 3 : 0;
                        if (b == 255) shown[m++] = (char)b;
                        continue;
                case 2: tn = 0; continue;
                case 3: if (b == 255) tn = 4; continue;
                case 4: tn = (b == 240) ? 0 : 3; continue;
                }
                if (b == 255) { tn = 1; continue; }
                shown[m++] = (char)b;
            }
            if (m) { fwrite(shown, 1, (size_t)m, stdout); fflush(stdout); }
        }
        if (!alive) break;

        /* The player's side: keys as a VT100 would send them. */
        if (incon) {
            int keys[64], k, i, o = 0;
            char out[512];
            k = con_poll(keys, 64);
            for (i = 0; i < k && o < (int)sizeof out - 8; i++) {
                const char *seq = NULL;
                switch (keys[i]) {
                case NK_UP:    seq = "\x1b[A"; break;
                case NK_DOWN:  seq = "\x1b[B"; break;
                case NK_RIGHT: seq = "\x1b[C"; break;
                case NK_LEFT:  seq = "\x1b[D"; break;
                case NK_HOME:  seq = "\x1b[H"; break;
                case NK_DEL:   seq = "\x1b[3~"; break;
                case 8:        seq = "\x7f"; break;
                }
                if (seq) { size_t l = strlen(seq); memcpy(out + o, seq, l); o += (int)l; }
                else if (keys[i] < 0x80) out[o++] = (char)keys[i];
            }
            if (o) send_all(s, out, o);
        } else if (!sent_eof) {
            /* Not a console: keys from a pipe or a file, for scripted play.
             * Its end is the player hanging up. */
            DWORD avail = 0, got = 0;
            if (PeekNamedPipe(hin, NULL, 0, NULL, &avail, NULL)) {
                if (avail) {
                    char in[1024];
                    if (ReadFile(hin, in, avail < sizeof in ? avail : (DWORD)sizeof in, &got, NULL) && got)
                        send_all(s, in, (int)got);
                }
            } else if (GetFileType(hin) == FILE_TYPE_DISK) {
                char in[1024];
                if (ReadFile(hin, in, sizeof in, &got, NULL) && got) send_all(s, in, (int)got);
                else { shutdown(s, SD_SEND); sent_eof = 1; }
            } else {
                shutdown(s, SD_SEND);
                sent_eof = 1;
            }
        }
    }
    closesocket(s);
    WSACloseEvent(e);
    printf("\x1b[0m\n");
    fflush(stdout);
    stopped = 1;
    return 0;
}
