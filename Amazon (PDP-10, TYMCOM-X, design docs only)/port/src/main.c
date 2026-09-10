/*  main.c -- AMAZON for Windows.
 *
 *  usage:  amazon [-name NAME] [-code CODE] [-world FILE] [-solo] [-new]
 *
 *  With no switches, every copy of the program on the machine shares one
 *  valley (amazon.wld beside the executable), which is the point: AMAZON
 *  was designed as "A game of fun, daring, suprises, and multiple players".
 *  -solo gives you a private throwaway valley instead.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "amazon.h"

static void banner(void)
{
    puts("");
    puts("                        A  M  A  Z  O  N");
    puts("");
    puts("        A game of fun, daring, suprises, and multiple players");
    puts("");
    puts("      Carl Baltrunas, Tymshare, 1977-1981.  Never finished.");
    puts("      Windows port built from the surviving design documents;");
    puts("      type SOURCE at the prompt for what is original and what");
    puts("      is not.");
    puts("");
}

static void default_world(char *buf, size_t n)
{
    char exe[MAX_PATH], *p;
    DWORD k = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (!k || k >= MAX_PATH) { strncpy(buf, "amazon.wld", n - 1); buf[n-1] = 0; return; }
    p = strrchr(exe, '\\');
    if (p) *(p + 1) = 0; else exe[0] = 0;
    snprintf(buf, n, "%samazon.wld", exe);
}

static void ask(const char *prompt, char *buf, int n)
{
    int i;
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(buf, n, stdin)) { buf[0] = 0; return; }
    for (i = 0; buf[i]; i++)
        if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = 0; break; }
}

int main(int argc, char **argv)
{
    char wpath[MAX_PATH] = "";
    char name[NAMELEN] = "", code[CODELEN] = "";
    int  solo = 0, fresh = 0, i, slot;
    World *w;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-solo")) solo = 1;
        else if (!strcmp(argv[i], "-new")) fresh = 1;
        else if (!strcmp(argv[i], "-world") && i + 1 < argc) {
            strncpy(wpath, argv[++i], sizeof wpath - 1);
        } else if (!strcmp(argv[i], "-name") && i + 1 < argc) {
            strncpy(name, argv[++i], NAMELEN - 1);
        } else if (!strcmp(argv[i], "-code") && i + 1 < argc) {
            strncpy(code, argv[++i], CODELEN - 1);
        } else {
            printf("usage: %s [-name NAME] [-code CODE] [-world FILE]"
                   " [-solo] [-new]\n", argv[0]);
            return 2;
        }
    }

    banner();

    if (!wpath[0]) {
        if (solo) {
            char tmp[MAX_PATH];
            DWORD n = GetTempPathA(MAX_PATH, tmp);
            if (!n || n > MAX_PATH - 40) strcpy(tmp, ".\\");
            snprintf(wpath, sizeof wpath, "%samazon-solo-%lu.wld",
                     tmp, (unsigned long)GetCurrentProcessId());
        } else {
            default_world(wpath, sizeof wpath);
        }
    }

    if (!name[0]) {
        ask("What is your name? ", name, sizeof name);
        if (!name[0]) strcpy(name, "CRETIN");
    }
    if (!code[0]) {
        ask("CODE (up to 6 characters, as in the statistics block)? ",
            code, sizeof code);
        if (!code[0]) strcpy(code, "PHREAD");
    }
    for (i = 0; name[i]; i++) name[i] = (char)toupper((unsigned char)name[i]);
    for (i = 0; code[i]; i++) code[i] = (char)toupper((unsigned char)code[i]);

    if (fresh) DeleteFileA(wpath);

    if (!world_open(wpath, solo)) return 1;
    if (!solo) printf("\n[valley: %s]\n", world_path());

    w = world_lock();
    if (!w) return 1;
    slot = game_join(w, name, code);
    if (slot < 0) {
        world_unlock();
        printf("no more channels\n");     /* PRTEST.SAI, when all 16 are used */
        world_close();
        return 1;
    }
    printf("[you are user # %d of %d]\n", slot + 1, MAXPLAYERS);
    {
        char m[MSGLEN];
        snprintf(m, sizeof m, "New User: %s added by Federal Marshal", name);
        /* PRTEST.SAI announces arrivals with exactly that line. */
        w->msgseq++;
        w->msgid[w->msghead] = w->msgseq;
        w->msgto[w->msghead] = slot;
        snprintf(w->msgtext[w->msghead], MSGLEN, "%s", m);
        w->msghead = (w->msghead + 1) % MSGRING;
    }
    world_unlock();

    game_loop(slot);

    world_close();
    return 0;
}
