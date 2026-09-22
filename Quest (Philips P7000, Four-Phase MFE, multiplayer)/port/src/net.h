/* net.h - sockets and the console for QUEST's players (Windows).
 *
 * A player's terminal is a TCP connection: the window that runs the machine
 * listens, and each further player runs this same program, which joins as a
 * terminal (net_join).  The machine's side sends the terminal's screen as
 * ANSI text; the terminal's side sends keys, coded as below (a telnet client
 * can play too, with the characters, Enter and Backspace).
 *
 * Everything that needs windows.h is in net.c, away from the emulator's names. */
#ifndef NET_H
#define NET_H

/* the keys a terminal sends: 040-0176 the characters, and these */
#define K_ENTER     015                 /* the NEW LINE key */
#define K_BACKSPACE 010                 /* the cursor back, deleting */
#define K_ESC       033                 /* blank the line */
#define K_UP        0200                /* 0200 and above: 7200 key codes */
#define K_LEFT      0201
#define K_RIGHT     0202
#define K_DOWN      0203
#define K_HOME      0210
#define K_INSERT    0217
#define K_DELETE    0220
#define K_LOGOFF    0376                /* Control CURSOR RETURN */

unsigned long long net_ms(void);        /* a millisecond clock */

int  net_listen(int port, int lan);     /* 0 listening, 1 the port is taken, -1 failed */
int  net_accept(void);                  /* a new connection's id, or -1 */
int  net_recv(int id, unsigned char *buf, int max);     /* > 0 bytes, 0 none, -1 gone */
int  net_send(int id, const char *buf, int n);          /* -1: the connection is gone */
void net_close(int id);                 /* after what was sent has gone out */
void net_service(void);                 /* push out pending output */
int  net_pending(void);                 /* connections with output still to go */
void net_wait(int ms, int console);     /* until a socket (or the console) has input */
int  net_local_ip(char *buf, int n);
void net_shutdown(void);

/* the console */
int  con_interactive(void);             /* stdin and stdout are a console */
void con_open(const char *title, int cols, int rows);  /* raw keys, ANSI output */
void con_close(void);
int  con_keys(unsigned char *keys, int max, int *ctrl_c);   /* keys as above */
void con_write(const char *s, int n);
void con_wait_key(void);
void net_on_ctrl(volatile long *stop);  /* Ctrl+C, closing the window */

/* join the game at HOST:PORT as a terminal; returns the exit status */
int  net_join(const char *host, int port);

int  exe_path(char *buf, int n);        /* this program's file name */

/* a line from stdin, without its end: 1 a line, 0 none yet (a pipe with
 * nothing in it: the machine goes on meanwhile), -1 the end */
int  stdin_line(char *buf, int n);

#endif
