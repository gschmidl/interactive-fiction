/* vms.c - OpenVMS system service shim.
 *
 * Images reach system services through vectors in the P1 vector page
 * (P1SYSVECTORS, 0x7FFEDE00).  Those pages are left unmapped; CALLS/CALLG to
 * one of them is intercepted here, carried out natively, and control returns
 * to the caller with the status in R0.
 */
#include "vax.h"
#include "rmsdef.h"
#include <time.h>
#include <ctype.h>

#ifndef RAB_L_FAB
#define RAB_L_FAB 0x3C           /* between RAB$L_BKT (0x38) and RAB$L_XAB (0x40) */
#endif

int svc_verbose = 0;
int trap_after_read = 0;   /* -k: abort this many instructions after the first terminal read */
const char *data_dir = ".";
int term_width = 80, term_height = 24;
static int term_col;   /* column the program's own output left the terminal on */
u32 vms_p0_break = 0x00200000;   /* first free P0 page; set by the loader */

#define SS_NORMAL_    1
#define SS_BADPARAM_  20
#define SS_ACCVIO_    12
#define SS_NOPRIV_    36
#define SS_IVCHAN_    1140

/* ------------------------------------------------------------------ */
/* service identity                                                    */
enum {
    SVC_UNKNOWN = 0,
    SVC_OPEN, SVC_CREATE, SVC_CLOSE, SVC_CONNECT, SVC_DISCONNECT,
    SVC_GET, SVC_PUT, SVC_FIND, SVC_READ, SVC_WRITE, SVC_UPDATE,
    SVC_DELETE, SVC_DISPLAY, SVC_PARSE, SVC_SEARCH, SVC_ERASE,
    SVC_EXTEND, SVC_FLUSH, SVC_TRUNCATE, SVC_REWIND, SVC_SPACE,
    SVC_FREE, SVC_WAIT, SVC_MODIFY, SVC_NXTVOL, SVC_ENTER, SVC_REMOVE,
    SVC_RENAME, SVC_SETDDIR, SVC_SETDFPROT, SVC_FILESCAN,
    SVC_EXIT, SVC_ASSIGN, SVC_DASSGN, SVC_QIO, SVC_QIOW, SVC_CANCEL,
    SVC_GETTIM, SVC_ASCTIM, SVC_NUMTIM, SVC_BINTIM, SVC_GETMSG, SVC_PUTMSG,
    SVC_FAO, SVC_FAOL, SVC_TRNLOG, SVC_TRNLNM, SVC_CRELOG, SVC_CRELNM,
    SVC_SETIMR, SVC_CANTIM, SVC_WAITFR, SVC_SETEF, SVC_CLREF, SVC_READEF,
    SVC_SETAST, SVC_CLRAST, SVC_DCLAST, SVC_SETEXV, SVC_UNWIND, SVC_HIBER,
    SVC_WAKE, SVC_SCHDWK, SVC_GETJPI, SVC_GETDVI, SVC_GETSYI, SVC_GETDVIW,
    SVC_SETPRV, SVC_SETPRT, SVC_EXPREG, SVC_CRETVA, SVC_DELTVA, SVC_CRMPSC,
    SVC_IMGSTA, SVC_IMGACT, SVC_IMGFIX, SVC_LKWSET, SVC_ADJSTK, SVC_ADJWSL,
    SVC_BRKTHRU, SVC_SNDOPR, SVC_SNDJBC, SVC_GETQUI, SVC_CMKRNL, SVC_CMEXEC,
    SVC_NOP
};

typedef struct { u32 addr; short id; const char *name; } SvcEnt;

/* Addresses come from RMS.STB / SYS.STB of this OpenVMS VAX V7.1 system. */
static SvcEnt svctab[] = {
    /* --- RMS, exec mode --- */
    { 0x7FFEE168, SVC_DELETE,     "SYS$DELETE"     },
    { 0x7FFEE170, SVC_FIND,       "SYS$FIND"       },
    { 0x7FFEE178, SVC_FREE,       "SYS$FREE"       },
    { 0x7FFEE180, SVC_GET,        "SYS$GET"        },
    { 0x7FFEE188, SVC_PUT,        "SYS$PUT"        },
    { 0x7FFEE190, SVC_READ,       "SYS$READ"       },
    { 0x7FFEE198, SVC_WAIT,       "SYS$WAIT"       },
    { 0x7FFEE1A0, SVC_UPDATE,     "SYS$UPDATE"     },
    { 0x7FFEE1B0, SVC_WRITE,      "SYS$WRITE"      },
    { 0x7FFEE1B8, SVC_CLOSE,      "SYS$CLOSE"      },
    { 0x7FFEE1C0, SVC_CONNECT,    "SYS$CONNECT"    },
    { 0x7FFEE1C8, SVC_CREATE,     "SYS$CREATE"     },
    { 0x7FFEE1D0, SVC_DISCONNECT, "SYS$DISCONNECT" },
    { 0x7FFEE1D8, SVC_DISPLAY,    "SYS$DISPLAY"    },
    { 0x7FFEE1E0, SVC_ERASE,      "SYS$ERASE"      },
    { 0x7FFEE1E8, SVC_EXTEND,     "SYS$EXTEND"     },
    { 0x7FFEE1F0, SVC_FLUSH,      "SYS$FLUSH"      },
    { 0x7FFEE1F8, SVC_MODIFY,     "SYS$MODIFY"     },
    { 0x7FFEE200, SVC_NXTVOL,     "SYS$NXTVOL"     },
    { 0x7FFEE208, SVC_OPEN,       "SYS$OPEN"       },
    { 0x7FFEE210, SVC_REWIND,     "SYS$REWIND"     },
    { 0x7FFEE218, SVC_SPACE,      "SYS$SPACE"      },
    { 0x7FFEE220, SVC_TRUNCATE,   "SYS$TRUNCATE"   },
    { 0x7FFEE228, SVC_ENTER,      "SYS$ENTER"      },
    { 0x7FFEE230, SVC_PARSE,      "SYS$PARSE"      },
    { 0x7FFEE238, SVC_REMOVE,     "SYS$REMOVE"     },
    { 0x7FFEE240, SVC_RENAME,     "SYS$RENAME"     },
    { 0x7FFEE248, SVC_SEARCH,     "SYS$SEARCH"     },
    { 0x7FFEE250, SVC_SETDDIR,    "SYS$SETDDIR"    },
    { 0x7FFEE258, SVC_SETDFPROT,  "SYS$SETDFPROT"  },
    { 0x7FFEE270, SVC_FILESCAN,   "SYS$FILESCAN"   },
    /* --- general services --- */
    { 0x7FFEDE48, SVC_ASCTIM,   "SYS$ASCTIM"   },
    { 0x7FFEDE50, SVC_ASSIGN,   "SYS$ASSIGN"   },
    { 0x7FFEDE58, SVC_BINTIM,   "SYS$BINTIM"   },
    { 0x7FFEDE60, SVC_CANCEL,   "SYS$CANCEL"   },
    { 0x7FFEDE68, SVC_CANTIM,   "SYS$CANTIM"   },
    { 0x7FFEDE78, SVC_CRMPSC,   "SYS$CRMPSC"   },
    { 0x7FFEDE90, SVC_CMKRNL,   "SYS$CMKRNL"   },
    { 0x7FFEDE98, SVC_CLREF,    "SYS$CLREF"    },
    { 0x7FFEDEB0, SVC_CRELOG,   "SYS$CRELOG"   },
    { 0x7FFEDEE0, SVC_DASSGN,   "SYS$DASSGN"   },
    { 0x7FFEDEE8, SVC_DCLAST,   "SYS$DCLAST"   },
    { 0x7FFEDF10, SVC_DELTVA,   "SYS$DELTVA"   },
    { 0x7FFEDF48, SVC_EXPREG,   "SYS$EXPREG"   },
    { 0x7FFEDF50, SVC_FAO,      "SYS$FAO"      },
    { 0x7FFEDF58, SVC_FAOL,     "SYS$FAOL"     },
    { 0x7FFEDF68, SVC_IMGSTA,   "SYS$IMGSTA"   },
    { 0x7FFEDF70, SVC_SNDJBC,   "SYS$SNDJBC"   },
    { 0x7FFEDF78, SVC_GETTIM,   "SYS$GETTIM"   },
    { 0x7FFEDFB8, SVC_NUMTIM,   "SYS$NUMTIM"   },
    { 0x7FFEDFC0, SVC_SNDOPR,   "SYS$SNDOPR"   },
    { 0x7FFEDFC8, SVC_QIO,      "SYS$QIO"      },
    { 0x7FFEDE00, SVC_QIOW,     "SYS$QIOW"     },
    { 0x7FFEDFF8, SVC_SETAST,   "SYS$SETAST"   },
    { 0x7FFEE000, SVC_SETEF,    "SYS$SETEF"    },
    { 0x7FFEE020, SVC_SETIMR,   "SYS$SETIMR"   },
    { 0x7FFEE030, SVC_SETPRT,   "SYS$SETPRT"   },
    { 0x7FFEE058, SVC_TRNLOG,   "SYS$TRNLOG"   },
    { 0x7FFEE078, SVC_WAITFR,   "SYS$WAITFR"   },
    { 0x7FFEE0B0, SVC_GETMSG,   "SYS$GETMSG"   },
    { 0x7FFEE0D8, SVC_GETDVI,   "SYS$GETDVI?"  },
    { 0x7FFEE0E0, SVC_PUTMSG,   "SYS$PUTMSG"   },
    { 0x7FFEE118, SVC_UNWIND,   "SYS$UNWIND"   },
    { 0x7FFEDEF0, SVC_NOP,      "SYS$(unit)"   },
    { 0x7FFEE100, SVC_SETPRV,   "SYS$SETPRV"   },
    { 0x7FFEE108, SVC_CLRAST,   "SYS$CLRAST"   },
    { 0x7FFEE418, SVC_GETDVIW,  "SYS$GETDVIW"  },
    { 0x7FFEE420, SVC_GETJPI,   "SYS$GETJPI?"  },
    { 0x7FFEE428, SVC_GETDVI,   "SYS$GET?(428)" },
    { 0x7FFEE430, SVC_GETDVI,   "SYS$GET?(430)" },
    { 0x7FFEE438, SVC_GETDVI,   "SYS$GET?(438)" },
    { 0x7FFEE490, SVC_TRNLNM,   "SYS$TRNLNM"   },
    { 0, 0, NULL }
};

/* addresses discovered at run time but not yet named */
static u32 unknown_seen[64];
static int n_unknown;

static SvcEnt *find_svc(u32 va)
{
    int i;
    for (i = 0; svctab[i].name; i++) if (svctab[i].addr == va) return &svctab[i];
    return NULL;
}

int vms_is_service(u32 va)
{
    return va >= 0x7FFEDE00 && va < 0x7FFEF000;
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */
static u32 arg(int n) { return rd32(cpu.r[AP] + 4 * n); }
static int argc(void) { return (int)(rd32(cpu.r[AP]) & 0xFF); }

/* read a VMS string descriptor: [len:w][dtype:b][class:b][addr:l] */

static int desc_str(u32 d, char *buf, int max)
{
    u32 len, adr, i;
    if (!d) { buf[0] = 0; return 0; }
    len = rd16(d);
    adr = rd32(d + 4);
    if ((int)len > max - 1) len = max - 1;
    for (i = 0; i < len; i++) buf[i] = (char)rd8(adr + i);
    buf[len] = 0;
    return (int)len;
}

/* ------------------------------------------------------------------ */
/* file table                                                          */
#define MAXF 64
typedef struct {
    int    used;
    FILE  *fp;
    char   name[256];
    int    isterm;        /* 0 = disk, 1 = terminal in, 2 = terminal out */
    int    rfm;           /* FAB$B_RFM */
    int    rat;           /* FAB$B_RAT: record attributes (carriage control) */
    int    org;           /* FAB$B_ORG */
    int    mrs;           /* max record size */
    int    fixlen;        /* record length for fixed format */
    u32    fab;
    long   nextrec;       /* 0-based record counter for relative/direct access */
    int    eof;
} VFile;
static VFile ftab[MAXF];

#define MAXS 64
typedef struct { int used; int fidx; } VStream;
static VStream stab[MAXS];

static int alloc_file(void) { int i; for (i = 1; i < MAXF; i++) if (!ftab[i].used) return i; return 0; }
static int alloc_str(void)  { int i; for (i = 1; i < MAXS; i++) if (!stab[i].used) return i; return 0; }

/* Turn a VMS file spec into a host path: drop node/device/directory,
 * drop the version number, and look in the data directory. */
/* The handful of logical names these images look up. */
static const struct { const char *name, *val; } lognames[] = {
    { "TERM$TABLOC", "TERMTABLE" },      /* SMG's terminal capability table */
    { "ZK$KEY_DEF",  "ZK$KEY_DEF.COM" },
    { NULL, NULL }
};

static const char *translate_logical(const char *s)
{
    int i;
    for (i = 0; lognames[i].name; i++)
        if (!strcmp(s, lognames[i].name)) return lognames[i].val;
    return NULL;
}

static void vms_to_host(const char *spec, char *out, size_t n)
{
    const char *p = spec, *q;
    char base[256];
    size_t i = 0;
    while (*p == ' ' || *p == '\t') p++;      /* FORTRAN pads the name with blanks */
    /* "TERM$TABLOC:.EXE" - device is a logical name and the name part is empty */
    q = strchr(p, ':');
    if (q && (q[1] == 0 || q[1] == '.')) {
        char dev[64]; size_t dl = (size_t)(q - p);
        const char *tr;
        if (dl < sizeof dev) {
            memcpy(dev, p, dl); dev[dl] = 0;
            tr = translate_logical(dev);
            if (tr) {
                /* keep the file type the caller supplied, e.g. ":.EXE" */
                const char *typ = (q[1] == '.' && !strchr(tr, '.')) ? q + 1 : "";
                snprintf(out, n, "%s/%s%s", data_dir, tr, typ);
                return;
            }
        }
    }
    q = strrchr(p, ']'); if (q) p = q + 1;
    q = strrchr(p, '>'); if (q) p = q + 1;
    q = strrchr(p, ':'); if (q) p = q + 1;
    while (*p && *p != ';' && i < sizeof base - 1) base[i++] = (char)toupper((unsigned char)*p++);
    base[i] = 0;
    while (i && base[i-1] == ' ') base[--i] = 0;
    snprintf(out, n, "%s/%s", data_dir, base);
}

/* Names that resolve to the terminal.  FOR$TYPE / FOR$ACCEPT are the logical
 * names VAX FORTRAN uses for its TYPE and ACCEPT statements. */
static int is_terminal_name(const char *s)
{
    return  strstr(s, "SYS$INPUT")  || strstr(s, "SYS$OUTPUT") ||
            strstr(s, "SYS$ERROR")  || strstr(s, "SYS$COMMAND") ||
            strstr(s, "FOR$TYPE")   || strstr(s, "FOR$ACCEPT") ||
            strstr(s, "FOR$PRINT")  || strstr(s, "FOR$READ")   ||
            /* VAX FORTRAN preconnects units 5 and 6 to SYS$INPUT/SYS$OUTPUT,
             * so an implicit open of FOR005/FOR006 is the terminal. */
            strstr(s, "FOR005")     || strstr(s, "FOR006")     ||
            strstr(s, "TT:") || !strcmp(s, "TT");
}

static int is_terminal_input(const char *s)
{
    return strstr(s, "SYS$INPUT") || strstr(s, "SYS$COMMAND") ||
           strstr(s, "FOR$ACCEPT") || strstr(s, "FOR$READ") ||
           strstr(s, "FOR005");
}

/* ------------------------------------------------------------------ */
/* record I/O                                                          */

/* Read one record from a disk file in VMS format.  Returns length, -1 at EOF. */
static int read_record(VFile *f, u8 *buf, int max)
{
    if (f->rfm == FAB_C_FIX) {
        int n = f->fixlen ? f->fixlen : (f->mrs ? f->mrs : 512);
        size_t got;
        if (n > max) n = max;
        got = fread(buf, 1, n, f->fp);
        if (got == 0) return -1;
        while ((int)got < n) buf[got++] = 0;
        return n;
    }
    if (f->rfm == FAB_C_VAR || f->rfm == FAB_C_VFC) {
        u8 hdr[2]; int len; size_t got;
        for (;;) {
            long pos = ftell(f->fp);
            if (fread(hdr, 1, 2, f->fp) != 2) return -1;
            len = hdr[0] | (hdr[1] << 8);
            if (len == 0xFFFF) {                       /* skip to next block */
                fseek(f->fp, ((pos / 512) + 1) * 512, SEEK_SET);
                continue;
            }
            break;
        }
        if (len > max) len = max;
        got = fread(buf, 1, len, f->fp);
        if ((int)got < len) return -1;
        if (len & 1) fgetc(f->fp);                     /* records are word aligned */
        return len;
    }
    /* stream formats: read up to newline */
    {
        int c, n = 0;
        while ((c = fgetc(f->fp)) != EOF) {
            if (c == '\n') return n;
            if (c == '\r') continue;
            if (n < max) buf[n++] = (u8)c;
        }
        return n ? n : -1;
    }
}

/* (declared above do_qio) Column the program's own output has left on.  The VMS terminal
 * driver tracks this to decide whether a record needs a leading carriage
 * return; terminal echo of typed input does not count towards it. */

static void term_out(const u8 *s, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        fputc(s[i], stdout);
        if (s[i] == '\n' || s[i] == '\r') term_col = 0;
        else term_col++;
    }
}

/* Emit one record to the terminal honouring the file's record attributes.
 * With FORTRAN carriage control the first byte of the record is the control
 * character and is not part of the data. */
static void term_record(VFile *f, const u8 *buf, int len)
{
    if (f->rat & FAB_M_FTN) {
        int cc = len ? buf[0] : ' ';
        const u8 *d = buf + (len ? 1 : 0);
        int dl = len ? len - 1 : 0;
        switch (cc) {
        case '+':                                    /* overprint */
            term_out((const u8 *)"\r", 1);
            term_out(d, dl);
            break;
        case '$':                                    /* prompt: no trailing CRLF */
            if (term_col) term_out((const u8 *)"\r\n", 2);
            term_out(d, dl);
            break;
        case '0':                                    /* skip a line */
            if (term_col) term_out((const u8 *)"\r", 1);
            term_out((const u8 *)"\r\n", 2);
            term_out(d, dl);
            term_out((const u8 *)"\r\n", 2);
            break;
        case '1':                                    /* form feed */
            term_out((const u8 *)"\f", 1);
            term_out(d, dl);
            term_out((const u8 *)"\r\n", 2);
            break;
        case 0:                                      /* no carriage control */
            term_out(d, dl);
            break;
        default:                                     /* blank and anything else */
            if (term_col) term_out((const u8 *)"\r", 1);
            term_out(d, dl);
            term_out((const u8 *)"\r\n", 2);
            break;
        }
    } else {
        term_out(buf, len);
        term_out((const u8 *)"\r\n", 2);
    }
    fflush(stdout);
}

static void write_record(VFile *f, const u8 *buf, int len)
{
    if (f->isterm) { term_record(f, buf, len); return; }
    if (f->rfm == FAB_C_FIX) {
        int n = f->fixlen ? f->fixlen : len;
        int i;
        for (i = 0; i < n; i++) fputc(i < len ? buf[i] : 0, f->fp);
        return;
    }
    if (f->rfm == FAB_C_VAR || f->rfm == FAB_C_VFC) {
        fputc(len & 0xFF, f->fp); fputc((len >> 8) & 0xFF, f->fp);
        fwrite(buf, 1, len, f->fp);
        if (len & 1) fputc(0, f->fp);
        return;
    }
    fwrite(buf, 1, len, f->fp);
    fputc('\n', f->fp);
}

/* ------------------------------------------------------------------ */
/* RMS services                                                        */
static u32 rms_open(u32 fab, int creating)
{
    char spec[300], dflt[300], path[600];
    int ln, ld, idx;
    VFile *f;
    u32 fna = rd32(fab + FAB_L_FNA), dna = rd32(fab + FAB_L_DNA);
    int fns = rd8(fab + FAB_B_FNS), dns = rd8(fab + FAB_B_DNS);
    int i;

    for (i = 0; i < fns && i < (int)sizeof spec - 1; i++) spec[i] = (char)rd8(fna + i);
    spec[fns < (int)sizeof spec ? fns : (int)sizeof spec - 1] = 0;
    ln = (int)strlen(spec);
    while (ln && spec[ln-1] == ' ') spec[--ln] = 0;
    { int lead = 0; while (spec[lead] == ' ') lead++;
      if (lead) { memmove(spec, spec + lead, strlen(spec + lead) + 1); ln -= lead; } }

    dflt[0] = 0;
    if (dna && dns) {
        for (i = 0; i < dns && i < (int)sizeof dflt - 1; i++) dflt[i] = (char)rd8(dna + i);
        dflt[dns < (int)sizeof dflt ? dns : (int)sizeof dflt - 1] = 0;
        ld = (int)strlen(dflt);
        while (ld && dflt[ld-1] == ' ') dflt[--ld] = 0;
    }
    if (!spec[0] && dflt[0]) snprintf(spec, sizeof spec, "%s", dflt);
    /* a bare name with no type picks up the type from the default spec */
    if (spec[0] && !strchr(spec, '.') && dflt[0] && strchr(dflt, '.')) {
        const char *dot = strchr(dflt, '.');
        size_t l = strlen(spec);
        snprintf(spec + l, sizeof spec - l, "%s", dot);
    }

    idx = alloc_file();
    if (!idx) return RMS_FUL;
    f = &ftab[idx];
    memset(f, 0, sizeof *f);
    f->used = 1;
    f->fab  = fab;
    f->rfm  = rd8(fab + FAB_B_RFM);
    f->rat  = rd8(fab + FAB_B_RAT);
    f->org  = rd8(fab + FAB_B_ORG);
    f->mrs  = rd16(fab + FAB_W_MRS);
    if (f->rfm == FAB_C_FIX) f->fixlen = f->mrs;
    snprintf(f->name, sizeof f->name, "%s", spec);

    if (is_terminal_name(spec)) {
        f->isterm = is_terminal_input(spec) ? 1 : 2;
        f->fp = f->isterm == 1 ? stdin : stdout;
        if (!f->rfm) { f->rfm = FAB_C_VAR; wr8(fab + FAB_B_RFM, FAB_C_VAR); }
        /* RMS reports the device buffer size as the maximum record size;
         * SMG sizes its input buffer from it. */
        if (!f->mrs) { f->mrs = term_width; wr16(fab + FAB_W_MRS, (u16)term_width); }
        wr16(fab + FAB_W_IFI, (u16)idx);
        wr32(fab + FAB_L_STS, RMS_NORMAL);
        wr32(fab + FAB_L_STV, idx);
        if (svc_verbose) fprintf(stderr, "[rms] %s terminal %s -> ifi %d\n",
                                 creating ? "create" : "open", spec, idx);
        return RMS_NORMAL;
    }

    vms_to_host(spec, path, sizeof path);
    f->fp = fopen(path, creating ? "w+b" : "r+b");
    if (!f->fp && !creating) f->fp = fopen(path, "rb");
    if (!f->fp) {
        char lower[600]; size_t k;
        snprintf(lower, sizeof lower, "%s", path);
        for (k = 0; lower[k]; k++) lower[k] = (char)tolower((unsigned char)lower[k]);
        f->fp = fopen(lower, creating ? "w+b" : "rb");
    }
    if (!f->fp) {
        f->used = 0;
        wr32(fab + FAB_L_STS, RMS_FNF);
        if (svc_verbose) fprintf(stderr, "[rms] open %s (%s) -> file not found\n", spec, path);
        return RMS_FNF;
    }
    if (!f->rfm) f->rfm = FAB_C_VAR;
    /* RMS fills in the file's longest record; callers size buffers from it. */
    if (!f->mrs) {
        if (f->rfm == FAB_C_VAR || f->rfm == FAB_C_VFC) {
            u8 h[2]; long pos = 0; int longest = 0;
            fseek(f->fp, 0, SEEK_SET);
            while (fread(h, 1, 2, f->fp) == 2) {
                int ln = h[0] | (h[1] << 8);
                if (ln == 0xFFFF) { pos = ((pos / 512) + 1) * 512; fseek(f->fp, pos, SEEK_SET); continue; }
                if (ln > longest) longest = ln;
                pos += 2 + ln + (ln & 1);
                if (fseek(f->fp, pos, SEEK_SET) != 0) break;
            }
            rewind(f->fp);
            f->mrs = longest ? longest : 255;
        } else f->mrs = f->fixlen ? f->fixlen : 512;
        wr16(fab + FAB_W_MRS, (u16)f->mrs);
    }
    wr16(fab + FAB_W_IFI, (u16)idx);
    wr32(fab + FAB_L_STS, RMS_NORMAL);
    wr32(fab + FAB_L_STV, idx);          /* the "channel", used by $CRMPSC */
    wr8 (fab + FAB_B_RFM, (u8)f->rfm);
    if (svc_verbose)
        fprintf(stderr, "[rms] %s '%s' -> %s ifi=%d rfm=%d org=%d mrs=%d\n",
                creating ? "create" : "open", spec, path, idx, f->rfm, f->org, f->mrs);
    return RMS_NORMAL;
}

static u32 rms_close(u32 fab)
{
    int ifi = rd16(fab + FAB_W_IFI);
    if (ifi > 0 && ifi < MAXF && ftab[ifi].used) {
        if (!ftab[ifi].isterm && ftab[ifi].fp) fclose(ftab[ifi].fp);
        ftab[ifi].used = 0;
    }
    wr16(fab + FAB_W_IFI, 0);
    wr32(fab + FAB_L_STS, RMS_NORMAL);
    return RMS_NORMAL;
}

static u32 rms_connect(u32 rab)
{
    u32 fab = rd32(rab + RAB_L_FAB);
    int ifi = fab ? rd16(fab + FAB_W_IFI) : 0;
    int si;
    if (ifi <= 0 || ifi >= MAXF || !ftab[ifi].used) {
        wr32(rab + RAB_L_STS, RMS_IFI);
        return RMS_IFI;
    }
    si = alloc_str();
    if (!si) { wr32(rab + RAB_L_STS, RMS_FUL); return RMS_FUL; }
    stab[si].used = 1; stab[si].fidx = ifi;
    wr16(rab + RAB_W_ISI, (u16)si);
    wr32(rab + RAB_L_STS, RMS_NORMAL);
    if (svc_verbose) fprintf(stderr, "[rms] connect rab=%08X ifi=%d -> isi=%d\n", rab, ifi, si);
    return RMS_NORMAL;
}

static u32 rms_disconnect(u32 rab)
{
    int si = rd16(rab + RAB_W_ISI);
    if (si > 0 && si < MAXS) stab[si].used = 0;
    wr16(rab + RAB_W_ISI, 0);
    wr32(rab + RAB_L_STS, RMS_NORMAL);
    return RMS_NORMAL;
}

static VFile *rab_file(u32 rab)
{
    int si = rd16(rab + RAB_W_ISI);
    if (si <= 0 || si >= MAXS || !stab[si].used) return NULL;
    return &ftab[stab[si].fidx];
}

static u32 rms_get(u32 rab)
{
    VFile *f = rab_file(rab);
    u32 ubf = rd32(rab + RAB_L_UBF);
    int usz = rd16(rab + RAB_W_USZ);
    int rac = rd8(rab + RAB_B_RAC);
    u8 buf[32768];
    int n;

    if (!f) { wr32(rab + RAB_L_STS, RMS_IFI); return RMS_IFI; }
    if (usz > (int)sizeof buf) usz = (int)sizeof buf;
    if (usz <= 0) usz = (int)sizeof buf;

    if (svc_verbose)
        fprintf(stderr, "[rms]   get pre: '%s' isterm=%d ubf=%08X usz=%d rop=%08X\n",
                f->name, f->isterm, ubf, usz, rd32(rab + RAB_L_ROP));
    if (f->isterm == 1) {                              /* read a line from stdin */
        int c, k = 0;
        while ((c = fgetc(stdin)) != EOF && c != '\n')
            if (c != '\r' && k < usz) buf[k++] = (u8)c;
        if (c == EOF && k == 0) {
            if (svc_verbose) fprintf(stderr, "[rms]   get: terminal at EOF\n");
            wr32(rab + RAB_L_STS, RMS_EOF); return RMS_EOF;
        }
        if (svc_verbose) fprintf(stderr, "[rms]   get: read %d chars '%.*s'\n", k, k, buf);
        if (trap_after_read) {
            abort_at = cpu.icount + (u64)trap_after_read + 4;
            trace_level = (int)(cpu.icount + trap_after_read);
            trap_after_read = 0;
        }
        /* A terminal read reports the terminator character in RAB$L_STV;
         * callers use it to tell a finished line from a truncated one. */
        wr32(rab + RAB_L_STV, 13);
        n = k;
    } else {
        /* keyed / random access selects the record first */
        if (rac == RAB_C_KEY && (f->org == FAB_C_REL || f->rfm == FAB_C_FIX)) {
            u32 kbf = rd32(rab + RAB_L_KBF);
            long rec = kbf ? (long)rd32(kbf) : 0;
            int rl = f->fixlen ? f->fixlen : (f->mrs ? f->mrs : 512);
            if (rec > 0) fseek(f->fp, (rec - 1) * rl, SEEK_SET);
            f->nextrec = rec;
        }
        n = read_record(f, buf, usz);
        if (n < 0) { wr32(rab + RAB_L_STS, RMS_EOF); return RMS_EOF; }
        f->nextrec++;
    }
    if (svc_verbose)
        fprintf(stderr, "[rms]   get '%s' rac=%d usz=%d bkt=%u -> %d bytes\n",
                f->name, rac, usz, rd32(rab + RAB_L_BKT), n);
    if (ubf) mem_write(ubf, buf, n);
    wr16(rab + RAB_W_RSZ, (u16)n);
    wr32(rab + RAB_L_RBF, ubf);
    wr32(rab + RAB_L_STS, RMS_NORMAL);
    return RMS_NORMAL;
}

static u32 rms_put(u32 rab)
{
    VFile *f = rab_file(rab);
    u32 rbf = rd32(rab + RAB_L_RBF);
    int rsz = rd16(rab + RAB_W_RSZ);
    int rac = rd8(rab + RAB_B_RAC);
    u8 buf[32768];

    if (!f) { wr32(rab + RAB_L_STS, RMS_IFI); return RMS_IFI; }
    if (rsz > (int)sizeof buf) rsz = (int)sizeof buf;
    if (rbf) mem_read(rbf, buf, rsz);

    if (!f->isterm && rac == RAB_C_KEY && (f->org == FAB_C_REL || f->rfm == FAB_C_FIX)) {
        u32 kbf = rd32(rab + RAB_L_KBF);
        long rec = kbf ? (long)rd32(kbf) : 0;
        int rl = f->fixlen ? f->fixlen : (f->mrs ? f->mrs : 512);
        if (rec > 0) fseek(f->fp, (rec - 1) * rl, SEEK_SET);
    }
    write_record(f, buf, rsz);
    f->nextrec++;
    wr32(rab + RAB_L_STS, RMS_NORMAL);
    return RMS_NORMAL;
}

static u32 rms_rewind(u32 rab)
{
    VFile *f = rab_file(rab);
    if (!f) { wr32(rab + RAB_L_STS, RMS_IFI); return RMS_IFI; }
    if (!f->isterm && f->fp) rewind(f->fp);
    f->nextrec = 0;
    wr32(rab + RAB_L_STS, RMS_NORMAL);
    return RMS_NORMAL;
}

/* ------------------------------------------------------------------ */
/* time                                                                */
static u64 vms_now(void)
{
    /* VMS time: 100-ns ticks since 17-NOV-1858 (the Smithsonian base date) */
    time_t t = time(NULL);
    return ((u64)t + 3506716800ULL) * 10000000ULL;
}

/* $ASCTIM(timlen, timbuf, timadr, cvtflg): format a VMS quadword time.
 * cvtflg 0 gives "dd-mmm-yyyy hh:mm:ss.cc", 1 gives just the time. */
static void put_asctim(u32 desc, u32 lenadr, u32 timadr, u32 cvtflg)
{
    char b[64];
    static const char *mon[] = { "JAN","FEB","MAR","APR","MAY","JUN",
                                 "JUL","AUG","SEP","OCT","NOV","DEC" };
    u64 vt;
    time_t t;
    struct tm *tm;
    int n, cs;
    if (timadr) {
        vt = rd64(timadr);
        /* VMS counts 100ns ticks from 17-NOV-1858; shift to the Unix epoch. */
        t  = (time_t)(vt / 10000000ULL - 3506716800ULL);
        cs = (int)((vt / 100000ULL) % 100ULL);
        tm = gmtime(&t);
    } else {
        t = time(NULL); cs = 0; tm = localtime(&t);
    }
    if (!tm) { t = 0; tm = gmtime(&t); cs = 0; }
    if (cvtflg == 1)
        n = snprintf(b, sizeof b, "%02d:%02d:%02d.%02d",
                     tm->tm_hour, tm->tm_min, tm->tm_sec, cs);
    else if (cvtflg == 2)
        n = snprintf(b, sizeof b, "%2d-%s-%04d",
                     tm->tm_mday, mon[tm->tm_mon], tm->tm_year + 1900);
    else
        n = snprintf(b, sizeof b, "%2d-%s-%04d %02d:%02d:%02d.%02d",
                     tm->tm_mday, mon[tm->tm_mon], tm->tm_year + 1900,
                     tm->tm_hour, tm->tm_min, tm->tm_sec, cs);
    if (desc) {
        u32 max = rd16(desc), adr = rd32(desc + 4);
        int k = n < (int)max ? n : (int)max;
        int i;
        for (i = 0; i < k; i++) wr8(adr + i, (u8)b[i]);
        for (; i < (int)max; i++) wr8(adr + i, ' ');
        if (lenadr) wr16(lenadr, (u16)k);
    }
}

/* ------------------------------------------------------------------ */
/* terminal QIO                                                        */
#define IO_READVBLK   0x21
#define IO_WRITEVBLK  0x30
#define IO_READPROMPT 0x37
#define IO_SETMODE    0x23
#define IO_SENSEMODE  0x27

static u32 do_qio(int wait)
{
    /* $QIO(efn, chan, func, iosb, astadr, astprm, p1, p2, p3, p4, p5, p6) */
    u32 func = arg(3), iosb = arg(4);
    u32 p1 = argc() >= 7 ? arg(7) : 0;
    u32 p2 = argc() >= 8 ? arg(8) : 0;
    u32 fn = func & 0x3F;
    (void)wait;

    if (fn == (IO_WRITEVBLK & 0x3F) || fn == IO_WRITEVBLK) {
        u32 i;
        for (i = 0; i < p2; i++) fputc(rd8(p1 + i), stdout);
        fflush(stdout);
        if (iosb) { wr16(iosb, 1); wr16(iosb + 2, (u16)p2); wr32(iosb + 4, 0); }
        return SS_NORMAL_;
    }
    if (fn == (IO_READVBLK & 0x3F) || fn == IO_READPROMPT) {
        /* Terminal reads are line oriented; anything the caller's buffer
         * cannot take stays queued for the next read rather than being lost. */
        static char pend[1024];
        static int pend_len, pend_pos;
        u32 k = 0;
        if (fn == IO_READPROMPT && argc() >= 12) {      /* p5/p6 carry the prompt */
            u32 pb = arg(11), pl = arg(12), i;
            for (i = 0; i < pl; i++) fputc(rd8(pb + i), stdout);
            fflush(stdout);
            term_col = 1;
        }
        if (pend_pos >= pend_len) {
            int c;
            pend_len = pend_pos = 0;
            while ((c = fgetc(stdin)) != EOF && c != '\n')
                if (c != '\r' && pend_len < (int)sizeof pend) pend[pend_len++] = (char)c;
            if (c == EOF && pend_len == 0) {
                if (iosb) { wr16(iosb, 0x870); wr16(iosb + 2, 0); wr32(iosb + 4, 0); }
                return 0x870;                            /* SS$_ENDOFFILE */
            }
        }
        while (pend_pos < pend_len && k < p2) wr8(p1 + k++, (u8)pend[pend_pos++]);
        if (pend_pos >= pend_len) { pend_len = pend_pos = 0; }
        if (iosb) { wr16(iosb, 1); wr16(iosb + 2, (u16)k); wr32(iosb + 4, 0); }
        term_col = 0;
        return SS_NORMAL_;
    }
    if (fn == IO_SETMODE || fn == IO_SENSEMODE) {
        /* Terminal characteristics buffer: class, type, page width,
         * then the two device-dependent longwords. */
        if (p1 && p2 >= 8) {
            wr8 (p1,     66);                    /* DC$_TERM   */
            wr8 (p1 + 1, 96);                    /* DT$_VT100  */
            wr16(p1 + 2, (u16)term_width);
            wr32(p1 + 4, ((u32)term_height << 24) | 0x13A0);
            if (p2 >= 12) wr32(p1 + 8, 0x28FF0000);
        }
        if (iosb) { wr16(iosb, 1); wr16(iosb + 2, 0); wr32(iosb + 4, 0); }
        return SS_NORMAL_;
    }
    if (iosb) { wr16(iosb, 1); wr16(iosb + 2, 0); wr32(iosb + 4, 0); }
    return SS_NORMAL_;
}

/* ------------------------------------------------------------------ */
void vms_init(void)
{
    memset(ftab, 0, sizeof ftab);
    memset(stab, 0, sizeof stab);
}

u32 vms_service(u32 va)
{
    SvcEnt *e = find_svc(va);
    int id = e ? e->id : SVC_UNKNOWN;

    if (svc_verbose && e)
        fprintf(stderr, "[svc] %s(argc=%d, a1=%08X a2=%08X a3=%08X) from %08X\n",
                e->name, argc(), argc() >= 1 ? arg(1) : 0,
                argc() >= 2 ? arg(2) : 0, argc() >= 3 ? arg(3) : 0, vms_caller);

    switch (id) {
    case SVC_OPEN:       return rms_open(arg(1), 0);
    case SVC_CREATE:     return rms_open(arg(1), 1);
    case SVC_CLOSE:      return rms_close(arg(1));
    case SVC_CONNECT:    return rms_connect(arg(1));
    case SVC_DISCONNECT: return rms_disconnect(arg(1));
    case SVC_GET:        return rms_get(arg(1));
    case SVC_PUT:        return rms_put(arg(1));
    case SVC_REWIND:     return rms_rewind(arg(1));
    case SVC_FIND:       return rms_get(arg(1));
    case SVC_READ:       return rms_get(arg(1));
    case SVC_WRITE:      return rms_put(arg(1));
    case SVC_UPDATE:     return rms_put(arg(1));
    case SVC_FLUSH:
    case SVC_WAIT:
    case SVC_FREE:
    case SVC_EXTEND:
    case SVC_TRUNCATE:
    case SVC_MODIFY:
    case SVC_NXTVOL:
    case SVC_DISPLAY:
        if (arg(1)) wr32(arg(1) + RAB_L_STS, RMS_NORMAL);
        return RMS_NORMAL;
    case SVC_PARSE:
    case SVC_SEARCH:
        if (arg(1)) wr32(arg(1) + FAB_L_STS, RMS_NORMAL);
        return RMS_NORMAL;

    case SVC_QIO:
    case SVC_QIOW:       return do_qio(id == SVC_QIOW);

    case SVC_FAO: case SVC_FAOL: {
        /* $FAO(ctrstr, outlen, outbuf, p1...) - formatted ASCII output.
         * $FAOL takes the parameters as a list instead of inline. */
        u32 ctr = arg(1), outlen = arg(2), outbuf = arg(3);
        u32 plist = (id == SVC_FAOL) ? arg(4) : 0;
        int pn = 0;                       /* next parameter index */
        char ctl[1024], out[2048];
        int cl, ci = 0, oi = 0;
        cl = desc_str(ctr, ctl, sizeof ctl);

        while (ci < cl && oi < (int)sizeof out - 64) {
            char c = ctl[ci++];
            int rep = 1, width = 0, hasw = 0;
            if (c != '!') { out[oi++] = c; continue; }
            if (ci >= cl) break;
            if (ctl[ci] == '!') { out[oi++] = '!'; ci++; continue; }
            if (ctl[ci] == '/') { out[oi++] = '\n'; ci++; continue; }
            if (ctl[ci] == '_') { out[oi++] = '\t'; ci++; continue; }
            if (ctl[ci] == '^') { out[oi++] = '\f'; ci++; continue; }
            if (ctl[ci] == '+') { ci++; pn++; continue; }        /* skip a parameter */
            if (ctl[ci] == '-') { ci++; if (pn) pn--; continue; } /* reuse the last one */
            while (ci < cl && isdigit((unsigned char)ctl[ci])) {
                if (!hasw) { width = 0; hasw = 1; }
                width = width * 10 + (ctl[ci++] - '0');
            }
            if (ci < cl && ctl[ci] == '(') {                     /* !n(...) repeat */
                ci++; rep = hasw ? width : 1; hasw = 0; width = 0;
            }
            if (ci >= cl) break;
            {
                char d1 = ctl[ci++], d2 = (ci < cl) ? ctl[ci] : 0;
                char field[512];
                int fl = 0, r;
                for (r = 0; r < rep && oi < (int)sizeof out - 64; r++) {
                    u32 p = plist ? rd32(plist + 4 * pn) : arg(4 + pn);
                    fl = 0;
                    if (d1 == 'A' && (d2 == 'S' || d2 == 'D' || d2 == 'C' || d2 == 'Z')) {
                        if (r == 0) ci++;
                        if (d2 == 'S') { fl = desc_str(p, field, sizeof field); pn++; }
                        else if (d2 == 'D') {
                            u32 len = p, adr = plist ? rd32(plist + 4 * (pn + 1))
                                                     : arg(4 + pn + 1);
                            u32 k;
                            if (len > sizeof field) len = sizeof field;
                            for (k = 0; k < len; k++) field[k] = (char)rd8(adr + k);
                            fl = (int)len; pn += 2;
                        } else if (d2 == 'C') {
                            u32 len = rd8(p), k;
                            if (len > sizeof field) len = sizeof field;
                            for (k = 0; k < len; k++) field[k] = (char)rd8(p + 1 + k);
                            fl = (int)len; pn++;
                        } else {
                            u32 k = 0;
                            while (k < sizeof field - 1) { u8 ch = rd8(p + k);
                                if (!ch) break; field[k++] = (char)ch; }
                            fl = (int)k; pn++;
                        }
                    } else if ((d1 == 'U' || d1 == 'S' || d1 == 'X' || d1 == 'Z' || d1 == 'O')
                               && (d2 == 'B' || d2 == 'W' || d2 == 'L')) {
                        u32 v = p;
                        if (r == 0) ci++;
                        if (d2 == 'B') v &= 0xFF; else if (d2 == 'W') v &= 0xFFFF;
                        if (d1 == 'S')
                            fl = snprintf(field, sizeof field, "%d",
                                          d2 == 'B' ? (int)(i8)v : d2 == 'W' ? (int)(i16)v : (int)v);
                        else if (d1 == 'X') fl = snprintf(field, sizeof field, "%X", v);
                        else if (d1 == 'O') fl = snprintf(field, sizeof field, "%o", v);
                        else fl = snprintf(field, sizeof field, "%u", v);
                        pn++;
                    } else if (d1 == '%' && (d2 == 'D' || d2 == 'T')) {
                        /* !%D date and time, !%T time only.  The parameter is
                         * the address of a quadword VMS time; 0 means now. */
                        static const char *mon[] = { "JAN","FEB","MAR","APR","MAY","JUN",
                                                     "JUL","AUG","SEP","OCT","NOV","DEC" };
                        time_t t; struct tm *tm; int cs = 0;
                        if (r == 0) ci++;
                        if (p && mem_present(p)) {
                            u64 vt = rd64(p);
                            t  = (time_t)(vt / 10000000ULL - 3506716800ULL);
                            cs = (int)((vt / 100000ULL) % 100ULL);
                            tm = gmtime(&t);
                        } else { t = time(NULL); tm = localtime(&t); }
                        if (!tm) { t = 0; tm = gmtime(&t); }
                        if (d2 == 'T')
                            fl = snprintf(field, sizeof field, "%02d:%02d:%02d.%02d",
                                          tm->tm_hour, tm->tm_min, tm->tm_sec, cs);
                        else
                            fl = snprintf(field, sizeof field,
                                          "%2d-%s-%04d %02d:%02d:%02d.%02d",
                                          tm->tm_mday, mon[tm->tm_mon], tm->tm_year + 1900,
                                          tm->tm_hour, tm->tm_min, tm->tm_sec, cs);
                        pn++;
                    } else {
                        fl = 0;                                   /* unknown: emit nothing */
                    }
                    if (hasw && d1 == 'Z' ) {
                        while (fl < width && fl < (int)sizeof field) {
                            memmove(field + 1, field, (size_t)fl); field[0] = '0'; fl++;
                        }
                    }
                    if (hasw && fl < width)
                        for (; fl < width && fl < (int)sizeof field; fl++) field[fl] = ' ';
                    if (hasw && fl > width) fl = width;
                    if (oi + fl < (int)sizeof out) { memcpy(out + oi, field, (size_t)fl); oi += fl; }
                }
                if (rep > 1 && ci < cl && ctl[ci] == ')') ci++;
            }
        }
        if (outbuf) {
            u32 max = rd16(outbuf), adr = rd32(outbuf + 4);
            int k = (u32)oi < max ? oi : (int)max, i;
            for (i = 0; i < k; i++) wr8(adr + i, (u8)out[i]);
            for (; i < (int)max; i++) wr8(adr + i, ' ');
            if (outlen) wr16(outlen, (u16)k);
        }
        return SS_NORMAL_;
    }

    case SVC_GETMSG: {
        /* $GETMSG(msgid, msglen, bufadr, flags, outadr): supply a generic text
         * so the RTL's message formatting has something to work with. */
        u32 msgid = arg(1), msglen = arg(2), bufdsc = arg(3);
        char txt[80];
        int n = snprintf(txt, sizeof txt, "condition %08X", msgid);
        if (bufdsc) {
            u32 max = rd16(bufdsc), adr = rd32(bufdsc + 4);
            int k = (u32)n < max ? n : (int)max, i;
            for (i = 0; i < k; i++) wr8(adr + i, (u8)txt[i]);
            if (msglen) wr16(msglen, (u16)k);
        }
        return SS_NORMAL_;
    }
    case SVC_PUTMSG: {
        /* $PUTMSG(msgvec, actrtn, facnam, actprm): print the condition. */
        u32 vec = arg(1);
        if (vec) {
            u32 argcount = rd32(vec) & 0xFFFF;
            u32 cond = argcount ? rd32(vec + 4) : 0;
            fflush(stdout);
            printf("%%condition %08X\n", cond);
            fflush(stdout);
        }
        return SS_NORMAL_;
    }
    case SVC_UNWIND:
        /* A condition handler asked to unwind.  Nothing in these games depends
         * on resuming afterwards, so end the image the way an unhandled
         * severe error does. */
        fflush(stdout);
        if (svc_verbose) fprintf(stderr, "[svc] SYS$UNWIND - ending image\n");
        cpu.halted = 1;
        return SS_NORMAL_;

    case SVC_GETTIM:     if (arg(1)) wr64(arg(1), vms_now()); return SS_NORMAL_;
    case SVC_ASCTIM:     put_asctim(arg(2), arg(1), argc() >= 3 ? arg(3) : 0,
                                    argc() >= 4 ? arg(4) : 0); return SS_NORMAL_;
    case SVC_NUMTIM: {                       /* 7-word broken-down time */
        u32 buf = arg(1);
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        if (buf) {
            wr16(buf,      (u16)(tm->tm_year + 1900));
            wr16(buf + 2,  (u16)(tm->tm_mon + 1));
            wr16(buf + 4,  (u16)tm->tm_mday);
            wr16(buf + 6,  (u16)tm->tm_hour);
            wr16(buf + 8,  (u16)tm->tm_min);
            wr16(buf + 10, (u16)tm->tm_sec);
            wr16(buf + 12, 0);
        }
        return SS_NORMAL_;
    }

    case SVC_EXIT:
        cpu.halted = 1;
        cpu.halt_code = arg(1);
        return SS_NORMAL_;

    case SVC_ASSIGN:  if (arg(2)) wr16(arg(2), 1); return SS_NORMAL_;
    case SVC_DASSGN:  return SS_NORMAL_;

    case SVC_GETJPI: case SVC_GETDVI: case SVC_GETDVIW: case SVC_GETSYI: {
        /* Walk the item list.  Device items describe the console as the VT100
         * these games were written for; the values match what $GETDVI reports
         * for a VMS terminal.  Everything else is returned as zero. */
        int isdev = (id == SVC_GETDVI || id == SVC_GETDVIW);
        u32 itmlst = arg(4), iosb = arg(5), p = itmlst;
        while (p && rd32(p)) {
            u32 buflen = rd16(p), code = rd16(p + 2);
            u32 bufadr = rd32(p + 4), retadr = rd32(p + 8);
            u32 val = 0, i, used = buflen;
            const char *str = NULL;
            if (isdev) switch (code) {
            case 2:  val = 0x0C041807; break;                 /* DVI$_DEVCHAR   */
            case 4:  val = 66;         break;                 /* DVI$_DEVCLASS: DC$_TERM */
            case 6:  val = 96;         break;                 /* DVI$_DEVTYPE:  DT$_VT100 */
            case 8:  val = term_width; break;                 /* DVI$_DEVBUFSIZ */
            case 10: val = ((u32)term_height << 24) | 0x13A0; break;  /* DVI$_DEVDEPEND */
            case 28: val = 0x28FF0000; break;                 /* DVI$_DEVDEPEND2 */
            case 32: str = "_TTA0:";   break;                 /* DVI$_DEVNAM    */
            default: val = 0;          break;
            }
            if (svc_verbose)
                fprintf(stderr, "[svc]   item code=%u len=%u buf=%08X -> %08X%s\n",
                        code, buflen, bufadr, val, str ? str : "");
            if (bufadr) {
                if (str) {
                    used = (u32)strlen(str);
                    if (used > buflen) used = buflen;
                    for (i = 0; i < used; i++) wr8(bufadr + i, (u8)str[i]);
                    for (; i < buflen; i++) wr8(bufadr + i, ' ');
                } else {
                    for (i = 0; i < buflen; i++)
                        wr8(bufadr + i, (u8)(i < 4 ? (val >> (8 * i)) : 0));
                }
            }
            if (retadr) wr32(retadr, used);
            p += 12;
        }
        if (iosb) { wr16(iosb, 1); wr16(iosb + 2, 0); wr32(iosb + 4, 0); }
        return SS_NORMAL_;
    }

    case SVC_CRMPSC: {
        /* $CRMPSC(inadr, retadr, acmode, flags, gsdnam, ident, relpag,
         *         chan, pagcnt, vbn, prot, pfc) - map part of a file. */
        u32 retadr = arg(2), chan = argc() >= 8 ? arg(8) : 0;
        u32 pagcnt = argc() >= 9 ? arg(9) : 0;
        u32 vbn    = argc() >= 10 ? arg(10) : 1;
        VFile *f = (chan > 0 && chan < MAXF && ftab[chan].used) ? &ftab[chan] : NULL;
        u32 start, i;
        if (!f || !f->fp) {
            if (svc_verbose) fprintf(stderr, "[svc] CRMPSC: no file on channel %u\n", chan);
            return SS_IVCHAN_;
        }
        if (!pagcnt) {                       /* zero means the whole file */
            long cur = ftell(f->fp);
            fseek(f->fp, 0, SEEK_END);
            pagcnt = (u32)((ftell(f->fp) + 511) / 512);
            fseek(f->fp, cur, SEEK_SET);
        }
        if (!vbn) vbn = 1;
        start = vms_p0_break;
        mem_map(start, pagcnt, NULL, 0);
        vms_p0_break += pagcnt * VA_PAGE;
        fseek(f->fp, (long)(vbn - 1) * 512, SEEK_SET);
        for (i = 0; i < pagcnt; i++) {
            u8 pg[512];
            size_t got = fread(pg, 1, 512, f->fp);
            while (got < 512) pg[got++] = 0;
            mem_write(start + i * VA_PAGE, pg, 512);
        }
        if (retadr) { wr32(retadr, start); wr32(retadr + 4, start + pagcnt * VA_PAGE - 1); }
        if (svc_verbose)
            fprintf(stderr, "[svc] CRMPSC: mapped %u pages of '%s' from VBN %u at %08X\n",
                    pagcnt, f->name, vbn, start);
        return SS_NORMAL_;
    }

    case SVC_EXPREG: {                       /* grow the P0 region */
        u32 npages = arg(1), retadr = arg(2);
        u32 start = vms_p0_break;
        if (npages > 65536) return SS_BADPARAM_;
        mem_map(start, npages, NULL, 0);
        vms_p0_break += npages * VA_PAGE;
        if (retadr) { wr32(retadr, start); wr32(retadr + 4, vms_p0_break - 1); }
        if (svc_verbose)
            fprintf(stderr, "[svc] EXPREG %u pages -> %08X..%08X\n",
                    npages, start, vms_p0_break - 1);
        return SS_NORMAL_;
    }
    case SVC_CRETVA: {                       /* map an explicit address range */
        u32 inadr = arg(1), retadr = arg(2);
        if (inadr) {
            u32 lo = rd32(inadr) & ~((u32)VA_PAGE - 1);
            u32 hi = rd32(inadr + 4) | (VA_PAGE - 1);
            if (hi >= lo) mem_map(lo, (hi - lo + 1) / VA_PAGE, NULL, 0);
            if (retadr) { wr32(retadr, lo); wr32(retadr + 4, hi); }
        }
        return SS_NORMAL_;
    }
    case SVC_DELTVA:
        if (arg(2) && arg(1)) { wr32(arg(2), rd32(arg(1))); wr32(arg(2) + 4, rd32(arg(1) + 4)); }
        return SS_NORMAL_;
    case SVC_LKWSET: case SVC_ADJWSL: case SVC_ADJSTK:
        return SS_NORMAL_;

    case SVC_TRNLOG:
    case SVC_TRNLNM:
        return 0x00000924;                       /* SS$_NOLOGNAM */

    case SVC_SETIMR: case SVC_CANTIM: case SVC_WAITFR: case SVC_SETEF:
    case SVC_CLREF:  case SVC_SETAST: case SVC_CLRAST: case SVC_DCLAST:
    case SVC_SETPRV: case SVC_SETPRT: case SVC_CANCEL: case SVC_SNDOPR:
    case SVC_SNDJBC: case SVC_CRELOG: case SVC_SETDDIR: case SVC_SETDFPROT:
    case SVC_IMGSTA: case SVC_NOP:
        return SS_NORMAL_;

    default: {
        int i, known = 0;
        for (i = 0; i < n_unknown; i++) if (unknown_seen[i] == va) known = 1;
        if (!known && n_unknown < 64 && svc_verbose) {
            unknown_seen[n_unknown++] = va;
            fflush(stdout);
            fprintf(stderr, "[svc] unimplemented service at %08X from %08X (argc=%d",
                    va, vms_caller, argc());
            for (i = 1; i <= argc() && i <= 6; i++) fprintf(stderr, " %08X", arg(i));
            fprintf(stderr, ") - returning success\n");
        }
        return SS_NORMAL_;
    }
    }
}

/* legacy entry kept for the unmapped-execution path */
int vms_dispatch(u32 va)
{
    if (!vms_is_service(va)) return 0;
    cpu.r[0] = vms_service(va);
    cpu.r[PC] = rd32(cpu.r[SP]); cpu.r[SP] += 4;
    return 1;
}
