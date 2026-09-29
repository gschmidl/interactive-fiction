/* ------------------------------------------------------------------
 * cms.c - the bits of CMS that ZENO EXEC issues commands to.
 *
 * The exec runs unmodified, so every host service it uses has to arrive
 * the way it did on VM: as a command string handed to the subcommand
 * environment, reading and writing the exec's own REXX variables.  On
 * VM that channel was EXECCOMM; here it is Regina's RexxVariablePool,
 * which is the same interface under a different name.
 *
 * Commands honoured:   IOS3270, EXECIO, STATE, ERASE, SUBSET
 * ------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define INCL_RXSUBCOM
#define INCL_RXSHV
#define INCL_RXQUEUE
#include "rexxsaa.h"
#include "zeno.h"

int zeno_debug = 0;
static FILE *logf;

static char gamedir[512] = "game";
static char savedir[512] = "saves";

void zlog(const char *fmt, ...)
{
    va_list ap;
    if (!zeno_debug) return;
    if (!logf) logf = fopen("zeno.log", "w");
    if (!logf) return;
    va_start(ap, fmt);
    vfprintf(logf, fmt, ap);
    va_end(ap);
    fputc('\n', logf);
    fflush(logf);
}

void cms_set_dirs(const char *g, const char *s)
{
    snprintf(gamedir, sizeof gamedir, "%s", g);
    snprintf(savedir, sizeof savedir, "%s", s);
}

/* ---- REXX variable pool ---------------------------------------------- */

int rx_fetch(const char *name, char *buf, int bufsize)
{
    SHVBLOCK b;
    char nm[MAXVAR];
    snprintf(nm, sizeof nm, "%s", name);
    memset(&b, 0, sizeof b);
    b.shvnext          = NULL;
    b.shvname.strptr   = nm;
    b.shvname.strlength= (ULONG)strlen(nm);
    b.shvnamelen       = (ULONG)strlen(nm);
    b.shvvalue.strptr  = buf;
    b.shvvalue.strlength = (ULONG)bufsize;
    b.shvvaluelen      = (ULONG)bufsize;
    b.shvcode          = RXSHV_SYFET;
    b.shvret           = 0;
    if (RexxVariablePool(&b) & ~(RXSHV_NEWV | RXSHV_TRUNC | RXSHV_LVAR))
        return -1;
    if (b.shvret & RXSHV_NEWV) return -1;         /* never assigned */
    if (b.shvret & (RXSHV_BADN | RXSHV_MEMFL | RXSHV_BADF | RXSHV_NOAVL))
        return -1;
    return (int)b.shvvaluelen;
}

int rx_set(const char *name, const char *val, int len)
{
    SHVBLOCK b;
    char nm[MAXVAR];
    snprintf(nm, sizeof nm, "%s", name);
    memset(&b, 0, sizeof b);
    b.shvnext            = NULL;
    b.shvname.strptr     = nm;
    b.shvname.strlength  = (ULONG)strlen(nm);
    b.shvnamelen         = (ULONG)strlen(nm);
    b.shvvalue.strptr    = (char *)val;
    b.shvvalue.strlength = (ULONG)len;
    b.shvvaluelen        = (ULONG)len;
    b.shvcode            = RXSHV_SYSET;
    b.shvret             = 0;
    RexxVariablePool(&b);
    return 0;
}

/* ---- CMS file names --------------------------------------------------- */

/* "ZENO INITDATA A" -> game/ZENO.INITDATA, but anything the game writes
 * (saves, and edited copies of the shipped data) goes to savedir so the
 * original files stay pristine. */
static void cms_path(char *out, int outsz, const char *fn, const char *ft, int forwrite)
{
    char f[32], t[32];
    int i;
    snprintf(f, sizeof f, "%s", fn);
    snprintf(t, sizeof t, "%s", ft);
    for (i = 0; f[i]; i++) f[i] = (char)toupper((unsigned char)f[i]);
    for (i = 0; t[i]; i++) t[i] = (char)toupper((unsigned char)t[i]);
    if (forwrite) { snprintf(out, outsz, "%s/%s.%s", savedir, f, t); return; }
    snprintf(out, outsz, "%s/%s.%s", savedir, f, t);
    {
        FILE *fp = fopen(out, "rb");
        if (fp) { fclose(fp); return; }
    }
    snprintf(out, outsz, "%s/%s.%s", gamedir, f, t);
}

/* Regina's external data queue.  The name matters: a NULL queue name is
 * rejected, and RXQUEUE_NOWAIT/RXQUEUE_WAIT are documented the wrong way
 * round in rexxsaa.h - 1 is the value that does not block. */
#define RXQUEUE    ((PSZ)"SESSION")
#define RXQ_NOWAIT 1

/* ---- EXECIO ------------------------------------------------------------
 * Zeno issues exactly two shapes:
 *   EXECIO n DISKW fn ft fm 1 V 255 (FINIS      - queue -> file
 *   EXECIO * DISKR fn ft fm 1 (FINIS            - file  -> queue
 */
static int do_execio(const char *args)
{
    char cnt[32] = "", op[16] = "", fn[32] = "", ft[32] = "", fm[8] = "";
    char path[1024];
    int n;

    if (sscanf(args, "%31s %15s %31s %31s %7s", cnt, op, fn, ft, fm) < 4)
        return 24;
    for (n = 0; op[n]; n++) op[n] = (char)toupper((unsigned char)op[n]);

    if (!strcmp(op, "DISKW")) {
        FILE *f;
        int want = (cnt[0] == '*') ? -1 : atoi(cnt), i;
        cms_path(path, sizeof path, fn, ft, 1);
        if (!(f = fopen(path, "wb"))) { zlog("execio: cannot write %s", path); return 28; }
        for (i = 0; want < 0 || i < want; i++) {
            RXSTRING line;
            char buf[4096];
            line.strptr = buf; line.strlength = sizeof buf;
            if (RexxPullQueue(RXQUEUE, &line, NULL, RXQ_NOWAIT) != RXQUEUE_OK) break;
            fwrite(line.strptr, 1, line.strlength, f);
            fputc('\n', f);
            if (line.strptr != buf) RexxFreeMemory(line.strptr);
        }
        fclose(f);
        zlog("execio DISKW %s: %d records", path, i);
        return 0;
    }

    if (!strcmp(op, "DISKR")) {
        FILE *f;
        char buf[4096];
        int i = 0;
        cms_path(path, sizeof path, fn, ft, 0);
        if (!(f = fopen(path, "rb"))) { zlog("execio: cannot read %s", path); return 28; }
        while (fgets(buf, sizeof buf, f)) {
            RXSTRING line;
            int len = (int)strlen(buf);
            while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = 0;
            line.strptr = buf; line.strlength = (ULONG)len;
            RexxAddQueue(RXQUEUE, &line, RXQUEUE_FIFO);
            i++;
        }
        fclose(f);
        zlog("execio DISKR %s: %d records", path, i);
        return 0;
    }
    return 24;
}

/* ---- IOS3270 ------------------------------------------------------------ */

static void trim_trailing(char *s)
{
    int n = (int)strlen(s);
    while (n && s[n - 1] == ' ') s[--n] = 0;
}

static int do_ios3270(const char *args)
{
    char work[512];
    char fn[32] = "ZENO", ft[32] = "IOS3270", label[64] = "";
    char *tok, *opt;
    int noread = 0, noquit = 0, cursor_field = 0;
    Screen scr;
    int key, aux, i;
    char iosk[16], iosc[16], iosd[64];

    snprintf(work, sizeof work, "%s", args);

    /* options follow '(' */
    opt = strchr(work, '(');
    if (opt) {
        *opt++ = 0;
        for (tok = strtok(opt, " )"); tok; tok = strtok(NULL, " )")) {
            char u[32];
            int k;
            snprintf(u, sizeof u, "%s", tok);
            for (k = 0; u[k]; k++) u[k] = (char)toupper((unsigned char)u[k]);
            if (!strcmp(u, "NOREAD") || !strcmp(u, "NOWAIT")) noread = 1;
            else if (!strcmp(u, "NOQUIT")) noquit = 1;
            else if (isdigit((unsigned char)u[0])) {
                /* rrccc cursor address; rr == 00 means nth input field */
                int v = atoi(u);
                if (v < 100) cursor_field = v;
                else cursor_field = v % 100;
            }
        }
    }

    /* positional: fn ft [fm] ;label */
    {
        int nw = 0;
        char words[8][64];
        for (tok = strtok(work, " "); tok && nw < 8; tok = strtok(NULL, " ")) {
            snprintf(words[nw], 64, "%s", tok);
            nw++;
        }
        for (i = 0; i < nw; i++) {
            if (words[i][0] == ';') { snprintf(label, sizeof label, "%s", words[i]); break; }
            if (i == 0) snprintf(fn, sizeof fn, "%s", words[i]);
            else if (i == 1) snprintf(ft, sizeof ft, "%s", words[i]);
        }
    }
    if (!label[0]) { zlog("ios3270: no panel label in <%s>", args); return 1; }

    {
        char file[80];
        snprintf(file, sizeof file, "%s.%s", fn, ft);
        if (!panel_render(&scr, file, label)) {
            zlog("ios3270: panel %s not found in %s", label, file);
            return 1;
        }
    }
    if (cursor_field) scr.cursor_field = cursor_field;

    if (noread) { con_show(&scr); return 0; }

    key = con_read(&scr, &aux);

    /* --- decide what to hand back ------------------------------------- */
    strcpy(iosd, "");
    if (key == K_PF) {
        snprintf(iosk, sizeof iosk, "PF%02d", aux);
        if (scr.pf[aux][0]) snprintf(iosd, sizeof iosd, "%s", scr.pf[aux]);
    } else if (key == K_PA) {
        snprintf(iosk, sizeof iosk, "PA%d", aux);
    } else if (key == K_QUIT) {
        strcpy(iosk, "PF03");
        strcpy(iosd, "QUIT");
    } else {
        strcpy(iosk, "ENTER");
    }

    /* input fields come back on ENTER, and on a PF key when .y was set */
    if (key == K_ENTER || key == K_QUIT || scr.pass_on_pf) {
        for (i = 0; i < scr.nf; i++) {
            Field *f = &scr.f[i];
            char val[512];
            int p, k = 0;
            if (!(f->flags & F_INPUT) || !f->var[0]) continue;
            for (p = 0; p < f->len && k < (int)sizeof val - 1; p++) {
                int q = f->attrpos + 1 + p;
                if (q >= SCELLS) break;
                val[k++] = scr.ch[q];
            }
            val[k] = 0;
            trim_trailing(val);
            rx_set(f->var, val, (int)strlen(val));
        }
    }

    snprintf(iosc, sizeof iosc, "%02d%03d", scr.cursor_row + 1, scr.cursor_col + 1);
    rx_set("IOSK", iosk, (int)strlen(iosk));
    rx_set("IOSC", iosc, (int)strlen(iosc));
    rx_set("IOSD", iosd, (int)strlen(iosd));
    zlog("ios3270 %s -> iosk=%s iosd=<%s>", label, iosk, iosd);

    if (!noquit && !strcmp(iosd, "QUIT")) return 4;
    return 0;
}

/* ---- STATE / ERASE ------------------------------------------------------ */

static int do_state(const char *args, int erase)
{
    char fn[32] = "", ft[32] = "", fm[8] = "", path[1024];
    FILE *f;
    if (sscanf(args, "%31s %31s %7s", fn, ft, fm) < 2) return 24;
    cms_path(path, sizeof path, fn, ft, 0);
    if (erase) return remove(path) == 0 ? 0 : 28;
    if ((f = fopen(path, "rb"))) { fclose(f); return 0; }
    return 28;
}

/* ---- the subcommand handler -------------------------------------------- */

APIRET APIENTRY zeno_subcom(PRXSTRING cmd, PUSHORT flags, PRXSTRING retstr)
{
    char buf[2048], verb[32];
    const char *rest;
    size_t n = cmd->strlength;
    int rc = 0, i;

    if (n >= sizeof buf) n = sizeof buf - 1;
    memcpy(buf, cmd->strptr, n);
    buf[n] = 0;
    while (n && (buf[n - 1] == ' ' || buf[n - 1] == '\r' || buf[n - 1] == '\n'))
        buf[--n] = 0;

    verb[0] = 0;
    sscanf(buf, "%31s", verb);
    for (i = 0; verb[i]; i++) verb[i] = (char)toupper((unsigned char)verb[i]);
    rest = buf + strlen(verb);
    while (*rest == ' ') rest++;

    if      (!strcmp(verb, "IOS3270")) rc = do_ios3270(rest);
    else if (!strcmp(verb, "EXECIO"))  rc = do_execio(rest);
    else if (!strcmp(verb, "STACKIO")) rc = do_execio(rest);
    else if (!strcmp(verb, "STATE"))   rc = do_state(rest, 0);
    else if (!strcmp(verb, "ERASE"))   rc = do_state(rest, 1);
    else if (!strcmp(verb, "SUBSET"))  { con_message("CMS SUBSET is not available in this port - press ENTER"); rc = 0; }
    else {
        zlog("unhandled command: <%s>", buf);
        rc = -3;                                  /* CMS 'unknown command' */
    }

    *flags = RXSUBCOM_OK;
    {
        char rcs[16];
        int l = snprintf(rcs, sizeof rcs, "%d", rc);
        if (retstr->strptr && l < 256) { memcpy(retstr->strptr, rcs, l); retstr->strlength = (ULONG)l; }
        else { retstr->strptr = NULL; retstr->strlength = 0; }
    }
    return 0;
}
