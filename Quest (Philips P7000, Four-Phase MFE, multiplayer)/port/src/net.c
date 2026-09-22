/* net.c - sockets and the console for QUEST's players (Windows); see net.h. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"

#define MAXCONN 16

typedef struct {
    SOCKET s;
    int used, closing;
    char *pend;                         /* output the socket would not take yet */
    int pend_n, pend_cap;
    unsigned long long give_up;         /* a closing connection that will not drain */
} Conn;

static Conn cn[MAXCONN];
static SOCKET lsock = INVALID_SOCKET;
static WSAEVENT ev = WSA_INVALID_EVENT;
static int wsa_up;

unsigned long long net_ms(void)
{
    return GetTickCount64();
}

static int wsa_init(void)
{
    WSADATA d;

    if (wsa_up)
        return 0;
    if (WSAStartup(MAKEWORD(2, 2), &d))
        return -1;
    ev = WSACreateEvent();
    wsa_up = 1;
    return 0;
}

/* SO_EXCLUSIVEADDRUSE makes a second game on the same port fail to bind
 * instead of sharing it - which is how a second window finds out that it
 * should join the first */
int net_listen(int port, int lan)
{
    struct sockaddr_in a;
    int one = 1;

    if (wsa_init())
        return -1;
    lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lsock == INVALID_SOCKET)
        return -1;
    setsockopt(lsock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(lan ? INADDR_ANY : INADDR_LOOPBACK);
    if (bind(lsock, (struct sockaddr *)&a, sizeof a) == SOCKET_ERROR) {
        int e = WSAGetLastError();

        closesocket(lsock);
        lsock = INVALID_SOCKET;
        return e == WSAEADDRINUSE || e == WSAEACCES ? 1 : -1;
    }
    if (listen(lsock, 8) == SOCKET_ERROR) {
        closesocket(lsock);
        lsock = INVALID_SOCKET;
        return -1;
    }
    WSAEventSelect(lsock, ev, FD_ACCEPT);
    return 0;
}

int net_accept(void)
{
    struct sockaddr_in a;
    int al = sizeof a, i, one = 1;
    SOCKET s;

    if (lsock == INVALID_SOCKET)
        return -1;
    s = accept(lsock, (struct sockaddr *)&a, &al);
    if (s == INVALID_SOCKET)
        return -1;
    for (i = 0; i < MAXCONN && cn[i].used; i++)
        ;
    if (i >= MAXCONN) {
        closesocket(s);
        return -1;
    }
    memset(&cn[i], 0, sizeof cn[i]);
    cn[i].s = s;
    cn[i].used = 1;
    WSAEventSelect(s, ev, FD_READ | FD_WRITE | FD_CLOSE);      /* non-blocking now */
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    return i;
}

static Conn *conn_of(int id)
{
    return id >= 0 && id < MAXCONN && cn[id].used ? &cn[id] : NULL;
}

int net_recv(int id, unsigned char *buf, int max)
{
    Conn *c = conn_of(id);
    int n;

    if (!c || c->closing)
        return -1;
    n = recv(c->s, (char *)buf, max, 0);
    if (n > 0)
        return n;
    if (n == 0)
        return -1;
    return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
}

static int push(Conn *c)
{
    int done = 0;

    while (done < c->pend_n) {
        int k = send(c->s, c->pend + done, c->pend_n - done, 0);

        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                return -1;
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

    if (!c)
        return -1;
    if (n > 0) {
        if (c->pend_n + n > c->pend_cap) {
            int cap = c->pend_cap ? c->pend_cap : 16384;
            char *nb;

            while (cap < c->pend_n + n)
                cap *= 2;
            if (cap > (4 << 20))
                return -1;                      /* the other end stopped reading */
            nb = realloc(c->pend, (size_t)cap);
            if (!nb)
                return -1;
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

    if (!c)
        return;
    if (push(c) < 0 || c->pend_n == 0) {
        drop(c);
        return;
    }
    c->closing = 1;
    c->give_up = net_ms() + 5000;
}

int net_pending(void)
{
    int i, n = 0;

    for (i = 0; i < MAXCONN; i++)
        n += cn[i].used && (cn[i].closing || cn[i].pend_n > 0);
    return n;
}

void net_service(void)
{
    int i;

    for (i = 0; i < MAXCONN; i++) {
        Conn *c = &cn[i];

        if (!c->used)
            continue;
        if (push(c) < 0) {
            if (c->closing)
                drop(c);
            continue;
        }
        if (c->closing && (c->pend_n == 0 || net_ms() > c->give_up))
            drop(c);
    }
}

/* one event object stands for every socket; it is reset after the wait and
 * the caller then reads every socket, so what comes later signals it again */
void net_wait(int ms, int console)
{
    HANDLE h[2];
    DWORD nh = 0;

    if (ev != WSA_INVALID_EVENT)
        h[nh++] = ev;
    if (console)
        h[nh++] = GetStdHandle(STD_INPUT_HANDLE);
    if (nh)
        WaitForMultipleObjects(nh, h, FALSE, (DWORD)(ms < 0 ? 0 : ms));
    else
        Sleep((DWORD)(ms < 0 ? 0 : ms));
    if (ev != WSA_INVALID_EVENT)
        WSAResetEvent(ev);
}

/* the first IPv4 address that is not the loopback: what a player on another
 * computer gives to --join */
int net_local_ip(char *buf, int n)
{
    char name[256];
    struct addrinfo hints, *res = NULL, *ai;

    if (wsa_init() || gethostname(name, sizeof name))
        return -1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    if (getaddrinfo(name, NULL, &hints, &res))
        return -1;
    for (ai = res; ai; ai = ai->ai_next) {
        unsigned char *b = (unsigned char *)&((struct sockaddr_in *)ai->ai_addr)->sin_addr;

        if (b[0] == 127)
            continue;
        snprintf(buf, (size_t)n, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
        freeaddrinfo(res);
        return 0;
    }
    freeaddrinfo(res);
    return -1;
}

void net_shutdown(void)
{
    int i;

    if (lsock != INVALID_SOCKET)
        closesocket(lsock);
    lsock = INVALID_SOCKET;
    for (i = 0; i < MAXCONN; i++)
        if (cn[i].used)
            drop(&cn[i]);
    if (wsa_up) {
        WSACloseEvent(ev);
        WSACleanup();
    }
    ev = WSA_INVALID_EVENT;
    wsa_up = 0;
}

/* ---- the console ------------------------------------------------------------ */

static HANDLE hin, hout;
static DWORD saved_in, saved_out;
static int saved_in_ok, saved_out_ok, have_sbi;
static CONSOLE_SCREEN_BUFFER_INFO saved_sbi;

int con_interactive(void)
{
    DWORD m;

    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m) &&
           GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &m);
}

void con_open(const char *title, int cols, int rows)
{
    COORD size;
    SMALL_RECT win;

    hin = GetStdHandle(STD_INPUT_HANDLE);
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA(title);
    have_sbi = GetConsoleScreenBufferInfo(hout, &saved_sbi) != 0;
    if (have_sbi && (saved_sbi.srWindow.Right - saved_sbi.srWindow.Left + 1 < cols ||
                     saved_sbi.srWindow.Bottom - saved_sbi.srWindow.Top + 1 < rows)) {
        /* a classic console window: make it the terminal's size */
        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        size.X = (SHORT)cols;
        size.Y = (SHORT)rows;
        SetConsoleScreenBufferSize(hout, size);
        win.Right = (SHORT)(cols - 1);
        win.Bottom = (SHORT)(rows - 1);
        SetConsoleWindowInfo(hout, TRUE, &win);
    } else
        have_sbi = 0;
    saved_out_ok = GetConsoleMode(hout, &saved_out) != 0;
    if (saved_out_ok)
        SetConsoleMode(hout, saved_out | ENABLE_PROCESSED_OUTPUT |
                       ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    saved_in_ok = GetConsoleMode(hin, &saved_in) != 0;
    if (saved_in_ok)
        SetConsoleMode(hin, saved_in & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                                ENABLE_PROCESSED_INPUT));
    con_write("\033[?1049h\033[?25l\033[2J", -1);
}

void con_close(void)
{
    con_write("\033[0m\033[?25h\033[?1049l", -1);
    if (saved_in_ok)
        SetConsoleMode(hin, saved_in);
    if (saved_out_ok)
        SetConsoleMode(hout, saved_out);
    if (have_sbi) {
        SMALL_RECT win;

        win.Left = 0; win.Top = 0; win.Right = 1; win.Bottom = 1;
        SetConsoleWindowInfo(hout, TRUE, &win);
        SetConsoleScreenBufferSize(hout, saved_sbi.dwSize);
        SetConsoleWindowInfo(hout, TRUE, &saved_sbi.srWindow);
    }
}

void con_write(const char *s, int n)
{
    DWORD done;

    if (n < 0)
        n = (int)strlen(s);
    if (n > 0 && !WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, (DWORD)n, &done, NULL))
        fwrite(s, 1, (size_t)n, stdout);
}

/* the keys typed since the last call, as net.h codes */
int con_keys(unsigned char *keys, int max, int *ctrl_c)
{
    INPUT_RECORD r[64];
    DWORD n = 0, got = 0, i;
    int k = 0;

    if (!GetNumberOfConsoleInputEvents(hin, &n) || n == 0)
        return 0;
    if (n > 64)
        n = 64;
    if (!ReadConsoleInputA(hin, r, n, &got))
        return 0;
    for (i = 0; i < got; i++) {
        KEY_EVENT_RECORD *ke;
        int c, rep, ctrl, shift;

        if (r[i].EventType != KEY_EVENT || !r[i].Event.KeyEvent.bKeyDown)
            continue;
        ke = &r[i].Event.KeyEvent;
        ctrl = (ke->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        shift = (ke->dwControlKeyState & SHIFT_PRESSED) != 0;
        switch (ke->wVirtualKeyCode) {
        case VK_RETURN: c = ctrl ? K_LOGOFF : K_ENTER; break;
        case VK_BACK:   c = K_BACKSPACE; break;
        case VK_ESCAPE: c = K_ESC; break;
        case VK_UP:     c = K_UP; break;
        case VK_DOWN:   c = K_DOWN; break;
        case VK_LEFT:   c = shift ? K_DELETE : K_LEFT; break;     /* as on the 7200 */
        case VK_RIGHT:  c = shift ? K_INSERT : K_RIGHT; break;
        case VK_HOME:   c = K_HOME; break;
        case VK_INSERT: c = K_INSERT; break;
        case VK_DELETE: c = K_DELETE; break;
        default:
            c = (unsigned char)ke->uChar.AsciiChar;
            if (c == 3) {
                if (ctrl_c)
                    *ctrl_c = 1;
                continue;
            }
            if (c == 012)                       /* Ctrl+Enter as a terminal sends it */
                c = K_LOGOFF;
            else if (c == 010 || c == 0177)
                c = K_BACKSPACE;
            else if (c == 015)
                c = K_ENTER;
            else if (c == 033)
                c = K_ESC;
            else if (c < 040 || c > 0176)
                continue;
            break;
        }
        for (rep = ke->wRepeatCount ? ke->wRepeatCount : 1; rep > 0 && k < max; rep--)
            keys[k++] = (unsigned char)c;
    }
    return k;
}

void con_wait_key(void)
{
    INPUT_RECORD rec;
    DWORD got;

    FlushConsoleInputBuffer(hin);
    for (;;) {
        if (!ReadConsoleInputA(hin, &rec, 1, &got) || !got)
            return;
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
            return;
    }
}

static volatile long *ctrl_stop;

/* closing the window gives a program a few seconds before Windows ends it */
static BOOL WINAPI on_ctrl(DWORD type)
{
    if (ctrl_stop)
        *ctrl_stop = 1;
    if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT)
        Sleep(500);
    return TRUE;
}

void net_on_ctrl(volatile long *stop)
{
    ctrl_stop = stop;
    /* a program started from a shell that ignores Ctrl+C inherits that */
    SetConsoleCtrlHandler(NULL, FALSE);
    SetConsoleCtrlHandler(on_ctrl, TRUE);
}

int exe_path(char *buf, int n)
{
    DWORD k = GetModuleFileNameA(NULL, buf, (DWORD)n);

    return k > 0 && k < (DWORD)n ? 0 : -1;
}

int stdin_line(char *buf, int n)
{
    static char acc[4096];
    static int len, eof;
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD type = GetFileType(h);

    for (;;) {
        char *nl = memchr(acc, '\n', (size_t)len);
        int k;

        if (nl || (eof && len) || len == (int)sizeof acc) {
            k = nl ? (int)(nl - acc) : len;
            {
                int m = k, j;

                while (m > 0 && acc[m - 1] == '\r')
                    m--;
                j = m < n - 1 ? m : n - 1;
                memcpy(buf, acc, (size_t)j);
                buf[j] = 0;
            }
            k += nl != NULL;
            memmove(acc, acc + k, (size_t)(len - k));
            len -= k;
            return 1;
        }
        if (eof)
            return -1;
        {
            DWORD got = 0, want = (DWORD)(sizeof acc - (size_t)len);

            if (type == FILE_TYPE_PIPE) {
                DWORD avail = 0;

                if (!PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL)) {
                    eof = 1;                    /* the writer has gone */
                    continue;
                }
                if (!avail)
                    return 0;
                if (want > avail)
                    want = avail;
            }
            if (!ReadFile(h, acc + len, want, &got, NULL) || !got)
                eof = 1;
            len += (int)got;
        }
    }
}

/* ---- joining: this program as a player's terminal ------------------------------ */

static void send_all(SOCKET s, const char *b, int n)
{
    int tries = 0;

    while (n > 0) {
        int k = send(s, b, n, 0);

        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK || ++tries > 200)
                return;
            Sleep(10);
            continue;
        }
        b += k;
        n -= k;
    }
}

int net_join(const char *host, int port)
{
    struct addrinfo hints, *res = NULL, *ai;
    char ps[16];
    SOCKET s = INVALID_SOCKET;
    int incon, sent_eof = 0;
    volatile long stop = 0;
    WSAEVENT e;

    if (wsa_init())
        return 1;
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
        if (s == INVALID_SOCKET)
            continue;
        if (connect(s, ai->ai_addr, (int)ai->ai_addrlen) == 0)
            break;
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "quest: no game of QUEST is running at %s, port %d\n", host, port);
        return 1;
    }
    incon = con_interactive();
    if (incon)
        con_open("QUEST (Philips P7000) - a player's terminal", 81, 25);
    net_on_ctrl(&stop);
    e = WSACreateEvent();
    WSAEventSelect(s, e, FD_READ | FD_CLOSE);
    {
        int one = 1;

        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    }
    for (;;) {
        HANDLE hs[2];
        DWORD nh = 0;
        char buf[8192];
        int n, alive = 1;

        hs[nh++] = e;
        if (incon)
            hs[nh++] = GetStdHandle(STD_INPUT_HANDLE);
        WaitForMultipleObjects(nh, hs, FALSE, 50);
        WSAResetEvent(e);
        if (stop)
            break;
        for (;;) {                              /* the screen */
            n = recv(s, buf, sizeof buf, 0);
            if (n == 0 || (n < 0 && WSAGetLastError() != WSAEWOULDBLOCK)) {
                alive = 0;
                break;
            }
            if (n < 0)
                break;
            if (incon)
                con_write(buf, n);
            else {
                fwrite(buf, 1, (size_t)n, stdout);
                fflush(stdout);
            }
        }
        if (!alive)
            break;
        if (incon) {                            /* the keys */
            unsigned char keys[64];
            int ctrl_c = 0, k = con_keys(keys, 64, &ctrl_c);

            if (ctrl_c)
                break;
            if (k)
                send_all(s, (const char *)keys, k);
        } else if (!sent_eof) {
            /* keys from a pipe or a file (scripted play); its end hangs up */
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            DWORD avail = 0, got = 0;
            char in[1024];

            if (PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL)) {
                if (avail && ReadFile(h, in, avail < sizeof in ? avail : (DWORD)sizeof in, &got,
                                      NULL) && got)
                    send_all(s, in, (int)got);
            } else if (GetFileType(h) == FILE_TYPE_DISK &&
                       ReadFile(h, in, sizeof in, &got, NULL) && got)
                send_all(s, in, (int)got);
            else {
                shutdown(s, SD_SEND);
                sent_eof = 1;
            }
        }
    }
    closesocket(s);
    WSACloseEvent(e);
    if (incon) {
        if (!stop)
            con_wait_key();                     /* the game's last word stays up */
        con_close();
    }
    return 0;
}
