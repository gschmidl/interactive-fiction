/* con_test.c -- scripted front end.
 *
 * Mirrors the workstation screen to stdout and feeds it lines from stdin,
 * so the port can be exercised without a console. Used by tools\play.cmd
 * and during development; con_win.c is what ships.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wang.h"

static int frame = 0;
static int eof_seen = 0;

void con_init(void) { }

void con_refresh(void) { }

void con_poll(void) { }

static void dump_screen(void)
{
    int r, c;
    printf("+-[%d]", ++frame);
    for (r = 0; r < SCR_COLS - 5; r++) putchar('-');
    printf("+\n");
    for (r = 0; r < SCR_ROWS; r++) {
        char line[SCR_COLS + 1];
        int last = -1;
        for (c = 0; c < SCR_COLS; c++) {
            line[c] = wang_to_ascii(mem[SCR_CHAR + r * 0x100 + c]);
            if (line[c] != ' ') last = c;
        }
        line[last + 1] = 0;
        printf("|%s\n", line);
    }
    printf("+");
    for (r = 0; r < SCR_COLS; r++) putchar('-');
    printf("+\n");
    fflush(stdout);
}

/* The machine is idle and wants a key: show the screen, then send a line. */
void con_idle(void)
{
    char buf[256];
    int i;

    dump_screen();

    if (eof_seen) { running = 0; return; }
    if (!fgets(buf, sizeof buf, stdin)) { eof_seen = 1; running = 0; return; }

    printf(">> %s", buf);
    fflush(stdout);

    for (i = 0; buf[i] && buf[i] != '\n' && buf[i] != '\r'; i++) {
        uint8_t sc = key_scancode((unsigned char)buf[i]);
        if (sc) kq_push(sc);
    }
    kq_push(key_scancode(0x0D));       /* RETURN */
}

void con_shutdown(void)
{
    dump_screen();
    printf("[machine halted]\n");
}
