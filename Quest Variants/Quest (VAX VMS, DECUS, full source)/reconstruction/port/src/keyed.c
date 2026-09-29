/*
 *  keyed.c -- CHARACTER.DTA, which on VMS was an RMS indexed file:
 *
 *      OPEN(UNIT=21,FILE='...CHARACTER.DTA',ACCESS='KEYED',
 *           ORGANIZATION='INDEXED',RECL=63,FORM='UNFORMATTED',
 *           KEY=(1:15:CHARACTER,26:37:CHARACTER),SHARED)
 *
 *  RECL is in longwords for an unformatted file, so the record is the
 *  252 byte PLAYER string.  Key 0 is the character name and forbids
 *  duplicates; key 1 is the owner's user name and allows them.  Records
 *  are held in primary key order, which is the order a sequential read
 *  of an indexed file returns them in, and the order LISTPLAYERS and
 *  SORT rely on.
 *
 *  The whole file is a few hundred records at most, so it is read into
 *  memory on open and written back on close.  That also gives QUEST's
 *  record locking (FOR$IOS_SPERECLOC, and the UNLOCK statements) nothing
 *  to do, which is right for a single player.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmsrt.h"

#define RECLEN  252
#define KEY0_OFF  0
#define KEY0_LEN 15
#define KEY1_OFF 25
#define KEY1_LEN 12

void qpath_(const char *name, char *path, size_t nlen, size_t plen);

typedef struct {
    char data[RECLEN];
    int  deleted;
} REC;

static REC *recs = NULL;
static int nrecs = 0, cap = 0;
static int opened = 0, dirty = 0;

static int cur = -1;            /* record a keyed read last settled on */
static int cur_keyid = 0;       /* which key drives a following sequential read */
static int seq = 0;             /* next slot a sequential read should look at */

static char path[1024];

/* ------------------------------------------------------------------ */

static int keycmp(const char *a, const char *b, int off, int len)
{
    return memcmp(a + off, b + off, (size_t) len);
}

/*  primary key order, which is how the file is kept  */
static int cmp_prim(const void *a, const void *b)
{
    const REC *x = (const REC *) a, *y = (const REC *) b;
    return memcmp(x->data + KEY0_OFF, y->data + KEY0_OFF, KEY0_LEN);
}

static void resort(void)
{
    qsort(recs, (size_t) nrecs, sizeof(REC), cmp_prim);
}

static void build_path(void)
{
    char name[32], out[1024];
    size_t i;

    memcpy(name, "character.dta", 13);
    for (i = 13; i < sizeof name; i++)
        name[i] = ' ';
    qpath_(name, out, (size_t) 13, sizeof out);
    for (i = sizeof out; i > 0 && out[i - 1] == ' '; i--)
        ;
    memcpy(path, out, i);
    path[i] = '\0';
}

void kopen_(void)
{
    FILE *f;
    char buf[RECLEN];

    if (opened)
        return;
    opened = 1;
    dirty = 0;
    cur = -1;
    cur_keyid = 0;
    seq = 0;
    nrecs = 0;
    build_path();

    f = fopen(path, "rb");
    if (!f)
        return;                        /* STATUS='UNKNOWN': a new file */
    while (fread(buf, 1, RECLEN, f) == RECLEN) {
        if (nrecs == cap) {
            cap = cap ? cap * 2 : 64;
            recs = (REC *) realloc(recs, (size_t) cap * sizeof(REC));
        }
        memcpy(recs[nrecs].data, buf, RECLEN);
        recs[nrecs].deleted = 0;
        nrecs++;
    }
    fclose(f);
    resort();
}

void kclose_(void)
{
    FILE *f;
    int i;

    if (!opened)
        return;
    opened = 0;
    if (!dirty)
        return;
    f = fopen(path, "wb");
    if (!f)
        return;
    resort();
    for (i = 0; i < nrecs; i++)
        if (!recs[i].deleted)
            fwrite(recs[i].data, 1, RECLEN, f);
    fclose(f);
    /* drop the deleted slots now that they are off the disk */
    {
        int j = 0;
        for (i = 0; i < nrecs; i++)
            if (!recs[i].deleted)
                recs[j++] = recs[i];
        nrecs = j;
    }
    dirty = 0;
}

/* ------------------------------------------------------------------ */

/*  A key argument is a FORTRAN string that may be shorter than the key
    it is matched against; RMS compares only the characters supplied and
    blank fills the rest, which is what the callers assume (GETPLAYER is
    handed NAME*15, COUNTCHARACTER a USER*12).  */
static void pad_key(char *dst, int len, const char *key, size_t klen)
{
    int i;
    for (i = 0; i < len; i++)
        dst[i] = (size_t) i < klen ? key[i] : ' ';
}

static int locate(const char *key, size_t klen, int keyid, int ge)
{
    char k[KEY0_LEN > KEY1_LEN ? KEY0_LEN : KEY1_LEN];
    int off = keyid ? KEY1_OFF : KEY0_OFF;
    int len = keyid ? KEY1_LEN : KEY0_LEN;
    int i, best = -1;

    pad_key(k, len, key, klen);
    for (i = 0; i < nrecs; i++) {
        int c;
        if (recs[i].deleted)
            continue;
        c = memcmp(recs[i].data + off, k, (size_t) len);
        if (c == 0)
            return i;                  /* first match, file is sorted */
        if (ge && c > 0 && best < 0)
            best = i;
    }
    return ge ? best : -1;
}

/*  READ(21,KEY=...,KEYID=...) with an I/O list  */
void kread_key_(char *rec, const char *key, int *keyid, int *ge, int *ios,
                size_t reclen, size_t keylen)
{
    int i = locate(key, keylen, *keyid, *ge);
    if (i < 0) {
        *ios = 36;                     /* FOR$IOS_ATTACCNON: no such record */
        return;
    }
    cur = i;
    cur_keyid = *keyid;
    seq = i + 1;
    memcpy(rec, recs[i].data, reclen < RECLEN ? reclen : RECLEN);
    *ios = 0;
}

/*  READ(21,KEY=...,KEYID=...) with no I/O list -- positions only  */
void kfind_key_(const char *key, int *keyid, int *ge, int *ios, size_t keylen)
{
    int i = locate(key, keylen, *keyid, *ge);
    if (i < 0) {
        *ios = 36;
        return;
    }
    cur = i;
    cur_keyid = *keyid;
    seq = i + 1;
    *ios = 0;
}

/*  the next record in the order of the key last used  */
void kread_next_(char *rec, int *ios, size_t len)
{
    if (cur_keyid == 1) {
        /*  alternate key order: stay with the same user name for as long
            as there is one, which is all COUNTCHARACTER and
            LISTPLAYERS_YOURS ever look at  */
        const char *want = cur >= 0 ? recs[cur].data + KEY1_OFF : NULL;
        int i;
        for (i = seq; i < nrecs; i++) {
            if (recs[i].deleted)
                continue;
            if (want && memcmp(recs[i].data + KEY1_OFF, want, KEY1_LEN) != 0)
                continue;
            cur = i;
            seq = i + 1;
            memcpy(rec, recs[i].data, len < RECLEN ? len : RECLEN);
            *ios = 0;
            return;
        }
        /*  none left under this user name: hand back the next record in
            primary order so the caller's "is it still my name?" test
            ends the loop, the way an indexed read did  */
        for (i = seq; i < nrecs; i++) {
            if (recs[i].deleted)
                continue;
            cur = i;
            seq = i + 1;
            memcpy(rec, recs[i].data, len < RECLEN ? len : RECLEN);
            *ios = 0;
            return;
        }
        *ios = -1;
        return;
    }
    while (seq < nrecs && recs[seq].deleted)
        seq++;
    if (seq >= nrecs) {
        *ios = -1;                     /* end of file */
        return;
    }
    cur = seq;
    seq++;
    memcpy(rec, recs[cur].data, len < RECLEN ? len : RECLEN);
    *ios = 0;
}

/*  a sequential open starts at the first record  */
void kseq_rewind_(void)
{
    cur = -1;
    cur_keyid = 0;
    seq = 0;
}

/*  WRITE(21) PLAYER -- add a record; the primary key must be unique  */
void kwrite_(char *rec, int *ios, size_t len)
{
    int i;

    for (i = 0; i < nrecs; i++)
        if (!recs[i].deleted && keycmp(recs[i].data, rec, KEY0_OFF, KEY0_LEN) == 0) {
            *ios = 43;                 /* duplicate key -- PUTPLAYER's ERR */
            return;
        }
    if (nrecs == cap) {
        cap = cap ? cap * 2 : 64;
        recs = (REC *) realloc(recs, (size_t) cap * sizeof(REC));
    }
    memset(recs[nrecs].data, ' ', RECLEN);
    memcpy(recs[nrecs].data, rec, len < RECLEN ? len : RECLEN);
    recs[nrecs].deleted = 0;
    nrecs++;
    dirty = 1;
    resort();
    cur = -1;
    *ios = 0;
}

/*  REWRITE(21) PLAYER -- replace the record the last read settled on  */
void krewrite_(const char *rec, int *ios, size_t len)
{
    if (cur < 0 || cur >= nrecs || recs[cur].deleted) {
        *ios = 36;
        return;
    }
    memcpy(recs[cur].data, rec, len < RECLEN ? len : RECLEN);
    dirty = 1;
    *ios = 0;
    /*  the primary key may have been rewritten; keep the file ordered */
    {
        char keep[RECLEN];
        memcpy(keep, recs[cur].data, RECLEN);
        resort();
        cur = -1;
        {
            int i;
            for (i = 0; i < nrecs; i++)
                if (!recs[i].deleted && memcmp(recs[i].data, keep, RECLEN) == 0) {
                    cur = i;
                    seq = i + 1;
                    break;
                }
        }
    }
}

/*  DELETE(UNIT=21)  */
void kdelete_(int *ios)
{
    if (cur < 0 || cur >= nrecs || recs[cur].deleted) {
        *ios = 36;
        return;
    }
    recs[cur].deleted = 1;
    dirty = 1;
    *ios = 0;
}
