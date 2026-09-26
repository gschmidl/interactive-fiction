/* net.h -- the Windows services the multiplayer host needs: sockets for the
 * players' terminals, the console for the player at the host, a clock and
 * Ctrl-C.  They live in net.c behind this plain interface because windows.h
 * defines SC_CLOSE and friends, which are this shim's names for AOS/VS
 * system calls.
 */
#ifndef NET_H
#define NET_H

unsigned long long net_now(void);               /* milliseconds, monotonic   */

int  net_listen(int port, int lan);   /* 0 listening, 1 port taken, -1 failed;
                                         LAN: every interface, IPv6 and IPv4  */
void net_unlisten(void);
int  net_accept(char *peer, int peerlen);       /* a connection, or -1       */
int  net_recv(int id, unsigned char *buf, int max); /* >0 bytes, 0 none yet,
                                                   -1 the line has dropped   */
int  net_send(int id, const char *buf, int n);  /* 0, or -1 when it is gone  */
void net_close(int id);                         /* once what is queued is sent */
int  net_closing(void);                         /* connections still sending */
void net_service(void);                         /* push queued output along  */
void net_wait(int ms, int console);             /* until something happens   */
void net_shutdown(void);

int  con_interactive(void);             /* stdin and stdout are a console    */
void con_ansi(void);                    /* let the console take ANSI output  */
int  con_poll(int *keys, int max);      /* keys typed: ASCII or an NK_ code  */
enum { NK_UP = 0x100, NK_DOWN, NK_LEFT, NK_RIGHT, NK_HOME, NK_DEL };

void net_on_ctrl(volatile long *stop, volatile long *stopped); /* Ctrl-C, closing */
int  net_host_name(char *buf, int n);           /* this computer, for --join */
int  net_join(const char *host, int port, int god); /* a player's terminal  */

#endif /* NET_H */
