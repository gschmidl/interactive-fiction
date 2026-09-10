/* wideexec.h -- the ECLIPSE MV instructions.
 *
 * Opcodes and format types come from mvops.h, which is generated from the
 * assembler's own table; see the header there for the operand placement and
 * for how it was checked.  Included by mv32.c after the accumulators, memory
 * and die() exist.
 */

#define CBIT ((dword)C << 31)

static dword sx16(word w) { return (dword)(int32_t)(int16_t)w; }

/* The type-10 skips compare ACD with ACS, or with zero when the two
 * fields name the same accumulator; see the note at the skips. */
#define SKA(s)   AC[s]
#define SKB(d)   ((acs) == (acd) ? 0u : AC[d])


static dword rdw(dword a) { return ((dword)M[MADDR(a)] << 16) | M[MADDR(a + 1)]; }
static void  wrw(dword a, dword v)
{ M[MADDR(a)] = (word)(v >> 16); M[MADDR(a + 1)] = (word)v; }

/* Effective address for the X and L forms.  The displacement is 15 bits
 * signed with bit 15 meaning indirect for the X form, 31 bits with bit 31
 * meaning indirect for the L form, and PC-relative counts from the
 * displacement word itself. */
static dword xea(word ir, dword at, int lform, int idxhi, unsigned *used)
{
    unsigned ix = idxhi ? ((ir >> 13) & 3) : ((ir >> 11) & 3);
    dword a;
    int32_t d;
    int ind;
    if (lform) {
        dword w = rdw(at + 1);
        ind = (w & 0x80000000u) != 0;
        d = (int32_t)(w & 0x7FFFFFFFu);
        if (w & 0x40000000u) d |= (int32_t)0x80000000u;
        *used = 3;
    } else {
        word w = M[MADDR(at + 1)];
        ind = (w & 0x8000u) != 0;
        d = (int32_t)(w & 0x7FFFu);
        if (w & 0x4000u) d -= 0x8000;
        *used = 2;
    }
    switch (ix) {
    case 0:  a = (dword)d; break;                        /* absolute       */
    case 1:  a = (dword)((int32_t)(at + 1) + d); break;  /* PC relative    */
    case 2:  a = (dword)((int32_t)AC[2] + d); break;
    default: a = (dword)((int32_t)AC[3] + d); break;
    }
    a &= OFFMASK;
    if (ind) {
        int hops = 0;
        for (;;) {
            dword v = rdw(a);
            a = v & OFFMASK;
            if (!(v & 0x80000000u)) break;
            if (++hops > 16) { die("indirection loop", ir); break; }
        }
    }
    return a & OFFMASK;
}

/* Effective BYTE address, for the instructions whose names end in B.  Their
 * displacement is a byte count from a byte-pointer base, not a word count:
 * ZORK.PR builds the two file names it opens with
 *
 *   75580  XLEFB 3, -26(PC)     and    7559D  XLEFB 3, -92(PC)
 *
 * and -26 bytes from the displacement word at 75581 is word 75574, which is
 * "@INPUT", while -92 from 7559E is 75570, "@OUTPUT".  Read as word counts
 * they miss by half the distance.  There is no indirect bit here either --
 * the displacement is a plain 16-bit signed byte offset. */
static dword bea(word ir, dword at, int lform, int idxhi, unsigned *used)
{
    unsigned ix = idxhi ? ((ir >> 13) & 3) : ((ir >> 11) & 3);
    dword b;
    int32_t d;
    if (lform) { d = (int32_t)rdw(at + 1); *used = 3; }
    else       { d = (int32_t)(int16_t)M[MADDR(at + 1)]; *used = 2; }
    switch (ix) {
    case 0:  b = 0; break;                                  /* absolute */
    case 1:  b = (RING | ((at + 1) & OFFMASK)) << 1; break; /* PC       */
    case 2:  b = AC[2] << 1; break;
    default: b = AC[3] << 1; break;
    }
    return (dword)(b + (dword)d);
}

/* ---- the wide stack ---------------------------------------------------
 * PARU.32 gives the frame block relative to the frame pointer: AC0 at -8,
 * AC1 at -6, AC2 at -4, the caller's frame pointer at -2 and carry plus the
 * return address at 0, all doublewords. */
/* The wide stack pointer names the ADDRESS of the doubleword on top -- its
 * low word -- not its last word, and so does the frame pointer.  That is a
 * one-word distinction and it stays invisible until something treats the two
 * as interchangeable.  O.ON in ZORK.PR does: at 7F084 it relocates its own
 * frame block up the stack with WBLM and then, at 7F08E, does
 *
 *      LDASP 3 / STAFP 3 / XWLDA 2,-2,3
 *
 * -- the new frame pointer is simply the stack pointer, and the very next
 * instruction reads ?OFP through it.  The copy leaves ?ORTN's two words at
 * 6FD-6FE, so only a stack pointer of 6FD makes that read fetch the caller's
 * frame pointer rather than half of one field and half of the next. */
/* A bit address: ACS gives the word and ACD the bit within it, unless the
 * two name the same accumulator, when it holds the whole bit address. */
static dword bitaddr(unsigned acs, unsigned acd)
{
    return (acs == acd) ? AC[acd] : ((AC[acs] << 4) + AC[acd]);
}

static void wpushdw(dword v)
{
    dword sp = ((DW(WSP_A) & OFFMASK) + 2) & OFFMASK;
    wrw(sp, v);
    SETDW(WSP_A, RING | sp);
}

static dword wpopdw(void)
{
    dword sp = DW(WSP_A) & OFFMASK;
    dword v = rdw(sp);
    SETDW(WSP_A, RING | ((sp - 2) & OFFMASK));
    return v;
}

static void wsave(dword frame, dword ret)
{
    dword sp;
    wpushdw(AC[0]); wpushdw(AC[1]); wpushdw(AC[2]);
    wpushdw(DW(WFP_A));
    (void)ret;
    wpushdw((C ? 0x80000000u : 0) | (AC[3] & 0x7FFFFFFFu));
    sp = DW(WSP_A) & OFFMASK;
    SETDW(WFP_A, RING | sp);
    SETDW(WSP_A, RING | ((sp + 2 * frame) & OFFMASK));
    AC[3] = RING | sp;
}

static void wpopblock(void)
{
    dword ret = wpopdw();
    SETDW(WFP_A, wpopdw());
    AC[2] = wpopdw(); AC[1] = wpopdw(); AC[0] = wpopdw();
    C = (int)((ret >> 31) & 1);
    PC = ret & OFFMASK;
    AC[3] = DW(WFP_A);
}

/* WRTN leaves the carry the routine set rather than restoring the caller's:
 * the call sites test it straight away (LCALL, then an ALC with SNC over an
 * error path), so the carry is the return status and not saved state. */
static void wreturn(void)
{
    /* Clearing the stack is the callee's job: the frame block goes, and so
     * do the argument count LCALL pushed and the arguments below it.  The
     * count sits at WFP-10, just under the saved AC0.  Nothing at the call
     * site adjusts the stack -- 7E664 in FERRET.PR calls and then pushes its
     * next argument straight away -- so if WRTN leaves the count behind, the
     * two words leak on every call and the first WPOPJ after a few of them
     * jumps somewhere in page zero. */
    int status = C;
    dword fp = DW(WFP_A) & OFFMASK;
    dword nargs = rdw(fp - 10) & 0xFFFFu;
    SETDW(WSP_A, RING | (fp & OFFMASK));
    wpopblock();
    SETDW(WSP_A, RING | ((fp - 12 - 2 * nargs) & OFFMASK));
    C = status;
}

/* Effective address for the Eclipse two-word forms the MV kept: the
 * accumulator is in bits 12-11 and the index mode in bits 9-8, and bit 15 of
 * the displacement word means indirect in the page-zero mode. */
static dword e2ea(word ir, dword at, unsigned *used)
{
    unsigned ix = (ir >> 8) & 3;
    word w = M[MADDR(at + 1)];
    dword a;
    *used = 2;
    switch (ix) {
    case 0:  a = w & 0x7FFF; break;
    case 1:  a = (dword)((int32_t)(at + 1) + (int16_t)w); break;
    case 2:  a = (dword)((int32_t)AC[2] + (int16_t)w); break;
    default: a = (dword)((int32_t)AC[3] + (int16_t)w); break;
    }
    a &= OFFMASK;
    if (ix == 0 && (w & 0x8000)) {
        int hops = 0;
        for (;;) { dword v = rdw(a); a = v & OFFMASK;
                   if (!(v & 0x80000000u)) break;
                   if (++hops > 16) break; }
    }
    return a & OFFMASK;
}

/* Skip the next instruction, whatever its length. */
static void wskip(void)
{
    word ir = M[MADDR(PC)];
    unsigned n = 1;
    if ((ir & 0x8000) && ((ir & 0xF) == 8 || (ir & 0xF) == 9)) {
        const mvop *m = mvfind(ir);
        if (m) n = m->len;
    } else if ((ir & 0x8000) && (ir & 0x00FF) == 0x38) n = 2;
    PC = (PC + n) & OFFMASK;
}

/* A byte pointer is the word address shifted up one, ring bits and all. */
static dword bytep(dword worda) { return (RING | (worda & OFFMASK)) << 1; }
static word  rdbyte(dword bp) { word w = M[MADDR(bp >> 1)];
                                return (word)((bp & 1) ? (w & 0xFF) : (w >> 8)); }
static void  wrbyte(dword bp, word v)
{
    dword a = MADDR(bp >> 1);
    if (bp & 1) M[a] = (word)((M[a] & 0xFF00) | (v & 0xFF));
    else        M[a] = (word)((M[a] & 0x00FF) | ((v & 0xFF) << 8));
}

/* The floating point unit needs xea(), rdw(), wrw() and wskip(), so it
 * comes in here rather than beside the other headers. */
#include "mvfpu.h"
#include "mvdec.h"

static void wide_exec(word ir, dword at)
{
    const mvop *m = mvfind(ir);
    unsigned acs = (ir >> 13) & 3, acd = (ir >> 11) & 3;
    unsigned used = 1;
    int hi;
    dword a;

    /* Not every encoding in the extension space is a named instruction.  The
     * ones MASM does not name behave as ordinary ALCs -- 81F8 for instance is
     * NEGSC# 0,0, a carry manipulation with no load and no skip -- so fall
     * back on the narrow decoder rather than stopping. */
    if (!m) {
        /* Unnamed encodings in the extension space behave as ordinary ALCs,
         * but a low byte of 38 or 78 is a two-word form whatever it does, and
         * getting its length wrong desynchronises everything after it. */
        if ((ir & 0x00FF) == 0x38 || (ir & 0x00FF) == 0x78)
            { PC = (at + 2) & OFFMASK; return; }
        narrow_exec(ir, at);
        return;
    }
    hi = mv_idx_hi(m->type);

    if (is_fp_op(m->op)) { fp_exec32(ir, at, m); return; }

    switch (m->op) {
    /* ---- register to register (type 0F) ------------------------------ */
    case 0x8379: AC[acd] = AC[acs]; return;                          /* WMOV */
    case 0x8459: AC[acd] = ~AC[acs]; return;                         /* WCOM */
    case 0x8269: AC[acd] = (dword)(-(int32_t)AC[acs]); return;       /* WNEG */
    case 0x8259: AC[acd] = AC[acs] + 1; return;                      /* WINC */
    case 0x8449: AC[acd] &= AC[acs]; return;                         /* WAND */
    case 0x8469: AC[acd] |= AC[acs]; return;                         /* WIOR */
    case 0x8479: AC[acd] ^= AC[acs]; return;                         /* WXOR */
    case 0x8549: AC[acd] &= ~AC[acs]; return;                        /* WANC */
    case 0x8369: { dword t = AC[acs]; AC[acs] = AC[acd]; AC[acd] = t; return; }
    case 0x8359: AC[acd] = AC[acs] & 0xFFFFu; return;                /* ZEX  */
    case 0x8349: AC[acd] = sx16((word)AC[acs]); return;              /* SEX  */
    case 0x8149: { uint64_t r = (uint64_t)AC[acd] + AC[acs];         /* WADD */
                   C = (int)((r >> 32) & 1); AC[acd] = (dword)r; return; }
    case 0x8159: { uint64_t r = (uint64_t)AC[acd] + (dword)~AC[acs] + 1;
                   C = (int)((r >> 32) & 1); AC[acd] = (dword)r; return; }
    case 0x8249: { uint64_t r = (uint64_t)AC[acd] + (dword)~AC[acs];
                   C = (int)((r >> 32) & 1); AC[acd] = (dword)r; return; }
    case 0x8169: AC[acd] = (dword)((int32_t)AC[acd] * (int32_t)AC[acs]); return;
    case 0x8179: if (AC[acs]) AC[acd] = (dword)((int32_t)AC[acd] / (int32_t)AC[acs]);
                 return;                                             /* WDIV */
    case 0x8559: { int n = (int8_t)(AC[acs] & 0xFF);                 /* WLSH */
                   AC[acd] = n >= 0 ? (n >= 32 ? 0 : AC[acd] << n)
                                    : (-n >= 32 ? 0 : AC[acd] >> -n); return; }
    case 0x8279: { int n = (int8_t)(AC[acs] & 0xFF);                 /* WASH */
                   AC[acd] = n >= 0 ? (n >= 32 ? 0 : AC[acd] << n)
                          : (dword)((int32_t)AC[acd] >> (-n >= 32 ? 31 : -n));
                   return; }
    case 0x8049: AC[acd] = sx16((word)(AC[acd] + AC[acs])); return;  /* NADD */
    case 0x8059: AC[acd] = sx16((word)(AC[acd] - AC[acs])); return;  /* NSUB */
    case 0x8509: AC[acd] = sx16((word)(-(int)(word)AC[acs])); return;/* NNEG */
    case 0x8069: AC[acd] = sx16((word)((int16_t)AC[acd] * (int16_t)AC[acs])); return;
    case 0x8079: if ((word)AC[acs])
                     AC[acd] = sx16((word)((int16_t)AC[acd] / (int16_t)AC[acs]));
                 return;                                             /* NDIV */
    case 0x8529: AC[acd] = rdbyte(AC[acs]); return;                  /* WLDB */
    case 0x8539: wrbyte(AC[acs], (word)AC[acd]); return;             /* WSTB */
    case 0x8579: { unsigned k = acs; for (;;) { wpushdw(AC[k]);      /* WPSH */
                     if (k == acd) break; k = (k + 1) & 3; } return; }
    /* WPOP undoes WPSH: the push runs from ACS up to ACD with wraparound,
     * so the pop has to run from ACS DOWN to ACD.  Popping the other way
     * round happens to look right for the adjacent pairs the compiler uses
     * most (WPSH 0,1 / WPOP 1,0) but is wrong for the wrapping ones -- the
     * storage allocator saves a pointer with WPSH 2,0, which pushes AC2, AC3
     * and AC0, and restores it with WPOP 0,2, which must fill AC0, AC3 and
     * AC2 in that order.  Filling AC2, AC1, AC0 instead leaves AC3 holding
     * whatever the routine last used it for. */
    case 0x8089: { unsigned k = acs; for (;;) { AC[k] = wpopdw();    /* WPOP */
                     if (k == acd) break; k = (k - 1) & 3; } return; }

    /* ---- skips (type 10) ---------------------------------------------
     * The two fields are ACS and ACD, and the comparison is ACD against
     * ACS -- EXCEPT that naming the same accumulator twice compares it
     * against ZERO.  DG say so themselves in :UTIL's SKIPS.SR, which
     * defines the compare-with-zero mnemonics as exactly that:
     *
     *     .MACRO WGTZ  **  WSGT ^1,^1  %      ;skip if AC > 0
     *     .MACRO WEQZ  **  WSEQ ^1,^1  %      ;skip if AC = 0
     *
     * The comparison reads ACS against ACD, not the other way round: the
     * guard on the case-folding loop at 7D2BE in ZORK.PR is WSLE 1,0 with
     * AC1 = 1 and AC0 = the string length, and the loop has to run when the
     * string is not empty.
     *
     * Over half of the type-10 instructions in these programs name one
     * accumulator twice, so reading them as AC-against-AC turns every
     * test against zero into "is X greater than itself", which is never,
     * and the branch goes the wrong way every time.  The range check at
     * 77091 in DISCO.PR is the clearest example: WSGTI 0,32 / WSGT 0,0 /
     * XVCT reads as "if it is above 32, or not above 0, complain". */
    case 0x80B9: if (SKA(acs) == SKB(acd)) wskip(); return;           /* WSEQ */
    case 0x8189: if (SKA(acs) != SKB(acd)) wskip(); return;           /* WSNE */
    case 0x81B9: if ((int32_t)SKA(acs) >  (int32_t)SKB(acd)) wskip(); return;
    case 0x8199: if ((int32_t)SKA(acs) >= (int32_t)SKB(acd)) wskip(); return;
    case 0x8289: if ((int32_t)SKA(acs) <  (int32_t)SKB(acd)) wskip(); return;
    case 0x81A9: if ((int32_t)SKA(acs) <= (int32_t)SKB(acd)) wskip(); return;
    case 0x80A9: if (SKA(acs) >  SKB(acd)) wskip(); return;           /* WUSGT */
    case 0x8099: if (SKA(acs) >= SKB(acd)) wskip(); return;           /* WUSGE */

    /* ---- WBR, the short branch (type 23) -----------------------------
     * MASM's table carries two records for opcode 0x8038: the Eclipse's
     * XOP1, and -- much later in the table, among the MV additions --
     * WBR.  Taking the first record hides the branch, and since 0x8038
     * also reads as a harmless no-load ALC it then executes as a no-op:
     * every short forward branch in every 32-bit program silently falls
     * through.  It is the commonest instruction in the extension space
     * after the loads and stores.
     *
     * The displacement is six bits signed, in the two accumulator fields
     * and the shift field, relative to the instruction's own address.
     * That is how a conditional branch is built here: a skip, then WBR
     * over the real branch, so the displacement is nearly always one
     * more than the length of the instruction after it -- 3 for a
     * two-word XVCT, 2 for a one-word one, which is what 1264 of the
     * 3651 of them in the utilities measure as. */
    case 0x8038: {
        /* An eight-bit signed word displacement from the instruction's own
         * address, scattered across the fields the ALC form leaves free:
         * bits 14-11, then 9-8, then 7-6.  Bit 10 is not part of it and is
         * always zero in the branches these programs contain.
         *
         * The displacement was solved from a loop that appears identically
         * in both 32-bit games -- I.DISPLA's copy loop, at 7EB4B in ZORK.PR
         * and 7EED0 in FERRET.PR:
         *
         *    WSBI 1,1 / WSGT 1,1 / 81F8 / ... / WADI 2,2 / FA38
         *
         * so 81F8 has to be +7 (out of the loop, to the LDAFP that follows)
         * and FA38 -8 (back to the WSBI).  Read that way, 7059 of the 7108
         * branches in the utilities land on an instruction boundary; read
         * with bit 10 included, only 5009 do. */
        int d = (int)((((ir >> 11) & 0xF) << 4) | (((ir >> 8) & 3) << 2)
                      | ((ir >> 6) & 3));
        if (d & 0x80) d -= 0x100;
        PC = (dword)((int32_t)at + d) & OFFMASK;
        return; }

    /* ---- one accumulator (type 08) ----------------------------------- */
    case 0xA649: AC[acd] = DW(WSP_A); return;                        /* LDASP */
    case 0xA659: SETDW(WSP_A, AC[acd]); return;                      /* STASP */
    case 0xA669: AC[acd] = DW(WSL_A); return;                        /* LDASL */
    case 0xA679: SETDW(WSL_A, AC[acd]); return;                      /* STASL */
    case 0xC649: AC[acd] = DW(WSB_A); return;                        /* LDASB */
    case 0xC659: SETDW(WSB_A, AC[acd]); return;                      /* STASB */
    case 0xC669: AC[acd] = DW(WFP_A); return;                        /* LDAFP */
    case 0xC679: SETDW(WFP_A, AC[acd]); return;                      /* STAFP */
    case 0xE649: SETDW(WSP_A, RING | ((DW(WSP_A) + AC[acd]) & OFFMASK)); return;
    case 0xE659: AC[acd] = (dword)((int32_t)AC[acd] >> 1); return;   /* WHLV */
    case 0xE669: AC[acd] = sx16((word)AC[acd]); return;              /* CVWN */

    /* ---- immediates of 1..4 in bits 14-13 (type 09) ------------------ */
    case 0x84B9: AC[acd] += acs + 1; return;                         /* WADI */
    case 0x8589: AC[acd] -= acs + 1; return;                         /* WSBI */
    case 0x8599: AC[acd] = sx16((word)(AC[acd] + acs + 1)); return;  /* NADI */
    case 0x85A9: AC[acd] = sx16((word)(AC[acd] - acs - 1)); return;  /* NSBI */
    case 0x85B9: AC[acd] <<= (acs + 1); return;                      /* WLSI */

    /* ---- a 16-bit immediate word (types 0B, 0C) ---------------------- */
    case 0xC629: AC[acd] = sx16(M[MADDR(at + 1)]); used = 2; break;  /* NLDAI */
    case 0xC639: AC[acd] = sx16((word)(AC[acd] + M[MADDR(at + 1)])); used = 2; break;
    case 0xE6C9: PC = (at + 2) & OFFMASK;                            /* WSEQI */
                 if (AC[acd] == sx16(M[MADDR(at + 1)])) wskip(); return;
    case 0xE6E9: PC = (at + 2) & OFFMASK;                            /* WSNEI */
                 if (AC[acd] != sx16(M[MADDR(at + 1)])) wskip(); return;
    case 0xE689: PC = (at + 2) & OFFMASK;                            /* WSGTI */
                 if ((int32_t)AC[acd] > (int32_t)sx16(M[MADDR(at + 1)])) wskip(); return;
    case 0xE6A9: PC = (at + 2) & OFFMASK;                            /* WSLEI */
                 if ((int32_t)AC[acd] <= (int32_t)sx16(M[MADDR(at + 1)])) wskip(); return;

    /* ---- a 32-bit immediate doubleword (types 11, 12) ---------------- */
    case 0xC689: AC[acd] = rdw(at + 1); used = 3; break;             /* WLDAI */
    case 0x8689: AC[acd] += rdw(at + 1); used = 3; break;            /* WADDI */
    case 0x8699: AC[acd] &= rdw(at + 1); used = 3; break;            /* WANDI */
    case 0x86A9: AC[acd] |= rdw(at + 1); used = 3; break;            /* WIORI */
    case 0x86B9: AC[acd] ^= rdw(at + 1); used = 3; break;            /* WXORI */
    case 0xC699: PC = (at + 3) & OFFMASK;                            /* WUGTI */
                 if (AC[acd] >  rdw(at + 1)) wskip(); return;
    case 0xC6B9: PC = (at + 3) & OFFMASK;                            /* WULEI */
                 if (AC[acd] <= rdw(at + 1)) wskip(); return;

    /* ---- frames (types 05, 1E) ---------------------------------------
     * WSAV is the prologue of a routine reached by LCALL, which has already
     * pushed the argument count; WSSV is the prologue of one reached by
     * LJSR, which has not, so it pushes a count of its own.  The two are not
     * interchangeable and the programs never mix them up: across the seven
     * utilities measured, all 1188 LCALL targets begin with WSAVS or WSAVR
     * and all 20 LJSR targets with WSSVS.  Making WSSV supply the missing
     * count is what lets WRTN clear the argument list without having to know
     * how its routine was entered. */
    case 0xA729: case 0xA739:                                        /* WSAV* */
        wsave(M[MADDR(at + 1)], at + 2);
        PC = (at + 2) & OFFMASK;
        return;
    case 0x8729: case 0x8739:                                        /* WSSV* */
        wpushdw(0);
        wsave(M[MADDR(at + 1)], at + 2);
        PC = (at + 2) & OFFMASK;
        return;
    case 0x87A9: wreturn(); return;                                  /* WRTN  */
    case 0xE779: wpopblock(); return;                                /* WPOPB */
    case 0x8789: { dword r = wpopdw(); C = (int)((r >> 31) & 1);     /* WPOPJ */
                   PC = r & OFFMASK; return; }

    /* ---- character and block moves ----------------------------------- */
    /* WCMV pairs AC0 with AC2 and AC1 with AC3: destination length and
     * pointer, then source length and pointer.  Both lengths are SIGNED, and
     * each string is walked in the direction of its own sign, so a positive
     * destination and a negative source copy the bytes out backwards -- which
     * is how the runtime's number formatter turns a string of digits into a
     * right-justified field.  It lays the digits down least significant
     * first, pads forward with blanks, and then hands WCMV the whole thing
     * with the source length negative and its pointer on the last byte; the
     * copy reverses it.  Pairing the lengths the other way round makes the
     * count come out negative, nothing is copied, and Zork reports its score
     * as whatever was on the stack. */
    case 0x8779: {                                                   /* WCMV */
        int32_t dl = (int32_t)AC[0], sl = (int32_t)AC[1];
        dword dp = AC[2], sp = AC[3];
        int32_t dn = dl < 0 ? -dl : dl, sn = sl < 0 ? -sl : sl;
        int ds = dl < 0 ? -1 : 1, ss = sl < 0 ? -1 : 1;
        int32_t n = sn < dn ? sn : dn, k;
        for (k = 0; k < n; k++)
            wrbyte(dp + (dword)(ds * k), rdbyte(sp + (dword)(ss * k)));
        for (; k < dn; k++)
            wrbyte(dp + (dword)(ds * k), 0x20);           /* blank filled */
        AC[2] = dp + (dword)(ds * dn);
        AC[3] = sp + (dword)(ss * n);
        AC[0] = 0; AC[1] = 0;
        C = sn > dn;
        return; }
    case 0xA759: {                                                   /* WCMP */
        /* Same operand placement as WCMV: lengths in AC0 and AC1, the two
         * byte pointers in AC3 and AC2.  Shorter strings compare as if blank
         * filled, and the answer comes back in AC1 as -1, 0 or 1. */
        int32_t dl = (int32_t)AC[0], sl = (int32_t)AC[1];
        dword dp = AC[2], sp = AC[3];
        int32_t n = sl > dl ? sl : dl, k, r = 0;
        for (k = 0; k < n && !r; k++) {
            int a1 = k < sl ? rdbyte(sp + (dword)k) : 0x20;
            int b1 = k < dl ? rdbyte(dp + (dword)k) : 0x20;
            if (a1 != b1) r = a1 < b1 ? -1 : 1;
        }
        AC[0] = 0; AC[1] = (dword)r;
        AC[2] = dp + (dword)dl; AC[3] = sp + (dword)sl;
        return; }
        /* WCMP's pairing is the same as WCMV's -- AC0 with AC2, AC1 with
         * AC3 -- but which of the two strings the -1 and the 1 are about has
         * had no test, so only the pairing is corrected here. */
    case 0xE749: {                                                   /* WBLM */
        /* Same operands as the Eclipse BLM -- count in AC1, source in AC2,
         * destination in AC3 -- but the count is SIGNED, and a negative one
         * runs the move downwards from the addresses given, the way you copy
         * overlapping blocks that move up.  Counting an unsigned AC1 down to
         * zero instead takes four billion steps that wrap the whole address
         * space and leave the stack registers in ring page zero overwritten;
         * ZORK.PR does exactly that at 7F08D with a count of -12. */
        int32_t n = (int32_t)AC[1];
        int step = n < 0 ? -1 : 1;
        if (n < 0) n = -n;
        while (n--) { M[MADDR(AC[3])] = M[MADDR(AC[2])];
                      AC[2] += (dword)step; AC[3] += (dword)step; }
        AC[1] = 0;
        return; }

    /* ---- the carry setters, and the bit instructions ------------------ */
    /* ISZTS and DSZTS work on the doubleword on top of the wide stack.  The
     * .SYSTM thunk uses two of them to step the caller's return address past
     * the call number and the error return, exactly as the 16-bit thunk uses
     * two ISZ instructions. */
    case 0xC7C9: { dword sp = DW(WSP_A) & OFFMASK, v = rdw(sp) + 1;
                   wrw(sp, v); if (!v) wskip(); return; }            /* ISZTS */
    case 0xC7D9: { dword sp = DW(WSP_A) & OFFMASK, v = rdw(sp) - 1;
                   wrw(sp, v); if (!v) wskip(); return; }            /* DSZTS */
    case 0xA7C9: C = 1; return;                                      /* CRYTO */
    case 0xA7D9: C = 0; return;                                      /* CRYTZ */
    case 0xA7E9: C = !C; return;                                     /* CRYTC */
    /* The bit instructions address a bit, not a word: ACS is a word address
     * and ACD a bit offset from it, sixteen bits to the word and numbered
     * DG's way with bit 0 at the top.  Naming one accumulator twice is the
     * same convention the skips and WCLM use -- there is no separate base
     * then, and the accumulator holds the whole bit address by itself.
     * FERRET.PR's world model is a bit array reached that way: it works out
     * `index * 5 * 16 + 08B54`, which is bit 4 of word 08B5, and asks WSZB
     * 1,1 whether that bit is set.  Read as base-plus-offset the address
     * comes out as AC1 + AC1/16, hundreds of words away, and every flag in
     * the game answers whatever happens to be lying there -- which is why
     * Ferret used to think it was too dark to do anything. */
    case 0x8299: { dword b2 = bitaddr(acs, acd);                     /* WBTO */
                   M[MADDR((b2 >> 4) & OFFMASK)] |= (word)(0x8000u >> (b2 & 15));
                   return; }
    case 0x82A9: { dword b2 = bitaddr(acs, acd);                     /* WBTZ */
                   M[MADDR((b2 >> 4) & OFFMASK)] &= (word)~(0x8000u >> (b2 & 15));
                   return; }
    case 0x82B9: { dword b2 = bitaddr(acs, acd);                     /* WSZB */
                   if (!(M[MADDR((b2 >> 4) & OFFMASK)] & (0x8000u >> (b2 & 15))))
                       wskip();
                   return; }
    case 0x8389: { dword b2 = bitaddr(acs, acd);                     /* WSNB */
                   if (M[MADDR((b2 >> 4) & OFFMASK)] & (0x8000u >> (b2 & 15)))
                       wskip();
                   return; }
    case 0x8399: { dword b2 = bitaddr(acs, acd);                     /* WSZBO */
                   dword a2 = (b2 >> 4) & OFFMASK;
                   word bm = (word)(0x8000u >> (b2 & 15));
                   int was = (M[MADDR(a2)] & bm) != 0;
                   M[MADDR(a2)] |= bm;
                   if (!was) wskip();
                   return; }
    case 0x83A9: { unsigned k;                                       /* WLOB */
                   for (k = 0; k < 32; k++)
                       if (AC[acs] & (0x80000000u >> k)) break;
                   AC[acd] = k; return; }
    case 0x8489: { unsigned k, n = 0;                                /* WCOB */
                   for (k = 0; k < 32; k++) if (AC[acd] & (1u << k)) n++;
                   AC[acs] += n; return; }
    /* WMULS and WDIVS name no accumulators -- MASM's table has them at the
     * full words E759 and E769, in the same no-operand block as WCMV and
     * WEDIT -- and work on the fixed pair AC0:AC1 the way the Eclipse's own
     * MULS and DIVS work on their sixteen-bit halves.  ZORK.PR asks for a
     * remainder with the standard sign-extend-into-AC0 preamble,
     *      WSUB 0,0 / WSGE 1,1 / WADC 0,0 / WDIVS
     * which is only a sixty-four bit dividend if the quotient comes back in
     * AC1 and the remainder in AC0. */
    case 0xE759: { int64_t r = (int64_t)(int32_t)AC[1] * (int32_t)AC[2]
                              + (int32_t)AC[0];
                   AC[0] = (dword)((uint64_t)r >> 32);
                   AC[1] = (dword)(uint64_t)r; return; }              /* WMULS */
    case 0xE769: { int64_t nu = (int64_t)(((uint64_t)AC[0] << 32) | AC[1]);
                   int32_t dv = (int32_t)AC[2];
                   if (!dv) { C = 1; return; }                        /* WDIVS */
                   AC[1] = (dword)(nu / dv);
                   AC[0] = (dword)(nu % dv); C = 0; return; }

    /* FXTD and FXTE sit in that same block, at A779 and C749, and bracket a
     * stretch of integer arithmetic: ZORK.PR wraps FXTD ... FXTE round the
     * one add in its buffer-number hash.  Beside them in the table are the
     * Eclipse's FTD and FTE, floating-point trap disable and enable, and
     * SNOVR, skip on no overflow -- so these are the fixed-point pair,
     * turning the overflow trap off and on.  Nothing here traps on integer
     * overflow, so the flag is kept and not acted on. */
    case 0xA779: fixtrap = 0; return;                                 /* FXTD */
    case 0xC749: fixtrap = 1; return;                                 /* FXTE */

    /* ---- X form with an accumulator, index in bits 14-13 ------------- */
    case 0x8309: a = xea(ir, at, 0, hi, &used); AC[acd] = rdw(a); break;      /* XWLDA */
    case 0x8319: a = xea(ir, at, 0, hi, &used); wrw(a, AC[acd]); break;       /* XWSTA */
    case 0x8329: a = xea(ir, at, 0, hi, &used); AC[acd] = sx16(M[MADDR(a)]); break;
    case 0x8339: a = xea(ir, at, 0, hi, &used); M[MADDR(a)] = (word)AC[acd]; break;
    case 0x8409: a = xea(ir, at, 0, hi, &used); AC[acd] = RING | a; break;    /* XLEF  */
    case 0x8439: AC[acd] = bea(ir, at, 0, hi, &used); break;                  /* XLEFB */
    case 0x8419: AC[acd] = rdbyte(bea(ir, at, 0, hi, &used)); break;          /* XLDB  */
    case 0x8429: wrbyte(bea(ir, at, 0, hi, &used), (word)AC[acd]); break;     /* XSTB  */
    case 0x8018: a = xea(ir, at, 0, hi, &used);
                 AC[acd] = sx16((word)(AC[acd] + M[MADDR(a)])); break;        /* XNADD */
    case 0x8058: a = xea(ir, at, 0, hi, &used);
                 AC[acd] = sx16((word)(AC[acd] - M[MADDR(a)])); break;        /* XNSUB */
    case 0x8118: a = xea(ir, at, 0, hi, &used); AC[acd] += rdw(a); break;     /* XWADD */
    case 0x8158: a = xea(ir, at, 0, hi, &used); AC[acd] -= rdw(a); break;     /* XWSUB */
    case 0x8098: a = xea(ir, at, 0, hi, &used);
                 AC[acd] = sx16((word)((int16_t)AC[acd] * (int16_t)M[MADDR(a)])); break;
    case 0x80D8: a = xea(ir, at, 0, hi, &used);
                 if (M[MADDR(a)])
                     AC[acd] = sx16((word)((int16_t)AC[acd] / (int16_t)M[MADDR(a)]));
                 break;
    case 0x8198: a = xea(ir, at, 0, hi, &used);
                 AC[acd] = (dword)((int32_t)AC[acd] * (int32_t)rdw(a)); break;
    case 0x81D8: a = xea(ir, at, 0, hi, &used);
                 if (rdw(a)) AC[acd] = (dword)((int32_t)AC[acd] / (int32_t)rdw(a));
                 break;

    /* ---- X form with no accumulator, index in bits 12-11 ------------- */
    case 0x8629: a = xea(ir, at, 0, hi, &used); wpushdw(RING | a); break;     /* XPEF */
    case 0xC609: a = xea(ir, at, 0, hi, &used); PC = a; return;               /* XJMP */
    /* A return address carries the carry in its top bit -- DG numbers that
     * bit 0 -- and WPOPJ puts it back.  That is the whole error-signalling
     * path for a system call: the caller sets the carry with CRYTO, the
     * .SYSTM thunk hands the return address up and down through WPSH/WPOPJ,
     * and the success return clears the carry again with CRYTZ.  A return
     * address built without the carry loses it at the first WPOPJ, every
     * call looks as if it succeeded, and FERRET.PR reads the error code from
     * a ?GTMES that had no argument to give as the length of one. */
    case 0xC619: a = xea(ir, at, 0, hi, &used);                               /* XJSR */
                 AC[3] = CBIT | RING | ((at + used) & OFFMASK); PC = a; return;
    case 0x8619: a = xea(ir, at, 0, hi, &used);                               /* XPSHJ */
                 wpushdw(CBIT | RING | ((at + used) & OFFMASK)); PC = a; return;
    case 0xA619: a = xea(ir, at, 0, hi, &used);                               /* XWISZ */
                 { dword v = rdw(a) + 1; wrw(a, v);
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0xA639: a = xea(ir, at, 0, hi, &used);                               /* XWDSZ */
                 { dword v = rdw(a) - 1; wrw(a, v);
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0x8639: a = xea(ir, at, 0, hi, &used);                               /* XNISZ */
                 { word v = (word)(M[MADDR(a)] + 1); M[MADDR(a)] = v;
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0xA609: a = xea(ir, at, 0, hi, &used);                               /* XNDSZ */
                 { word v = (word)(M[MADDR(a)] - 1); M[MADDR(a)] = v;
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;

    /* ---- L form ------------------------------------------------------ */
    case 0x83C9: a = xea(ir, at, 1, hi, &used); AC[acd] = sx16(M[MADDR(a)]); break;
    case 0x83D9: a = xea(ir, at, 1, hi, &used); M[MADDR(a)] = (word)AC[acd]; break;
    case 0x83E9: a = xea(ir, at, 1, hi, &used); AC[acd] = RING | a; break;    /* LLEF  */
    case 0x83F9: a = xea(ir, at, 1, hi, &used); AC[acd] = rdw(a); break;      /* LWLDA */
    case 0x84F9: a = xea(ir, at, 1, hi, &used); wrw(a, AC[acd]); break;       /* LWSTA */
    case 0x84E9: AC[acd] = bea(ir, at, 1, hi, &used); break;                  /* LLEFB */
    case 0x84C9: AC[acd] = rdbyte(bea(ir, at, 1, hi, &used)); break;         /* LLDB  */
    case 0x84D9: wrbyte(bea(ir, at, 1, hi, &used), (word)AC[acd]); break;    /* LSTB  */
    case 0x8218: a = xea(ir, at, 1, hi, &used);
                 AC[acd] = sx16((word)(AC[acd] + M[MADDR(a)])); break;        /* LNADD */
    case 0x8258: a = xea(ir, at, 1, hi, &used);
                 AC[acd] = sx16((word)(AC[acd] - M[MADDR(a)])); break;        /* LNSUB */
    case 0x8318: a = xea(ir, at, 1, hi, &used); AC[acd] += rdw(a); break;     /* LWADD */
    case 0x8358: a = xea(ir, at, 1, hi, &used); AC[acd] -= rdw(a); break;     /* LWSUB */
    case 0x8298: a = xea(ir, at, 1, hi, &used);
                 AC[acd] = sx16((word)((int16_t)AC[acd] * (int16_t)M[MADDR(a)])); break;
    case 0x82D8: a = xea(ir, at, 1, hi, &used);
                 if (M[MADDR(a)])
                     AC[acd] = sx16((word)((int16_t)AC[acd] / (int16_t)M[MADDR(a)]));
                 break;
    case 0x8398: a = xea(ir, at, 1, hi, &used);
                 AC[acd] = (dword)((int32_t)AC[acd] * (int32_t)rdw(a)); break;
    case 0x83D8: a = xea(ir, at, 1, hi, &used);
                 if (rdw(a)) AC[acd] = (dword)((int32_t)AC[acd] / (int32_t)rdw(a));
                 break;
    case 0xA6F9: a = xea(ir, at, 1, hi, &used); wpushdw(RING | a); break;     /* LPEF */
    case 0xC6F9: wpushdw(bea(ir, at, 1, hi, &used)); break;                   /* LPEFB */
    case 0xA6D9: a = xea(ir, at, 1, hi, &used); PC = a; return;               /* LJMP */
    case 0xA6E9: a = xea(ir, at, 1, hi, &used);                               /* LJSR */
                 AC[3] = RING | ((at + used) & OFFMASK); PC = a; return;
    case 0x86C9: a = xea(ir, at, 1, hi, &used);                               /* LNISZ */
                 { word v = (word)(M[MADDR(a)] + 1); M[MADDR(a)] = v;
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0x86E9: a = xea(ir, at, 1, hi, &used);                               /* LWISZ */
                 { dword v = rdw(a) + 1; wrw(a, v);
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;

    /* ---- the Eclipse extension set, which the MV kept ------------------
     * These work on the low half of the wide accumulators and leave the top
     * half alone.  Opcodes straight out of EBID.SR. */
    case 0x8008: SETLO(acd, LO(acd) + acs + 1); return;              /* ADI  */
    case 0x8048: SETLO(acd, LO(acd) - acs - 1); return;              /* SBI  */
    case 0x8108: SETLO(acd, LO(acd) | LO(acs)); return;              /* IOR  */
    case 0x8148: SETLO(acd, LO(acd) ^ LO(acs)); return;              /* XOR  */
    case 0x8188: SETLO(acd, LO(acd) & ~LO(acs)); return;             /* ANC  */
    case 0x81C8: { word t = LO(acs); SETLO(acs, LO(acd)); SETLO(acd, t); return; }
    case 0x8208: if ((int16_t)LO(acs) >  (int16_t)LO(acd)) wskip(); return;
    case 0x8248: if ((int16_t)LO(acs) >= (int16_t)LO(acd)) wskip(); return;
    case 0x8288: { int n = (int8_t)(LO(acs) & 0xFF); word v = LO(acd);
                   SETLO(acd, n >= 0 ? (n >= 16 ? 0 : (word)(v << n))
                                     : (-n >= 16 ? 0 : (word)(v >> -n)));
                   return; }                                          /* LSH  */
    case 0x8308: { unsigned n = 4 * (acs + 1);                        /* HXL  */
                   SETLO(acd, n >= 16 ? 0 : (word)(LO(acd) << n)); return; }
    case 0x8348: { unsigned n = 4 * (acs + 1);                        /* HXR  */
                   SETLO(acd, n >= 16 ? 0 : (word)(LO(acd) >> n)); return; }
    case 0x8408: { dword b = (AC[acs] + (LO(acd) >> 4)) & OFFMASK;    /* BTO  */
                   M[MADDR(b)] |= (word)(0x8000u >> (LO(acd) & 15)); return; }
    case 0x8448: { dword b = (AC[acs] + (LO(acd) >> 4)) & OFFMASK;    /* BTZ  */
                   M[MADDR(b)] &= (word)~(0x8000u >> (LO(acd) & 15)); return; }
    case 0x8488: { dword b = (AC[acs] + (LO(acd) >> 4)) & OFFMASK;    /* SZB  */
                   if (!(M[MADDR(b)] & (0x8000u >> (LO(acd) & 15)))) wskip();
                   return; }
    case 0x84C8: { dword b = (AC[acs] + (LO(acd) >> 4)) & OFFMASK;    /* SZBO */
                   word bm = (word)(0x8000u >> (LO(acd) & 15));
                   int was = (M[MADDR(b)] & bm) != 0;
                   M[MADDR(b)] |= bm; if (!was) wskip(); return; }
    case 0x85F8: { dword b = (AC[acs] + (LO(acd) >> 4)) & OFFMASK;    /* SNB  */
                   if (M[MADDR(b)] & (0x8000u >> (LO(acd) & 15))) wskip();
                   return; }
    case 0x8588: { unsigned k, n = 0;                                 /* COB  */
                   for (k = 0; k < 16; k++) if (LO(acd) & (1u << k)) n++;
                   SETLO(acs, LO(acs) + n); return; }
    case 0x85C8: SETLO(acd, rdbyte(AC[acs])); return;                 /* LDB  */
    case 0x8608: wrbyte(AC[acs], LO(acd)); return;                    /* STB  */
    case 0xC7C8: { uint32_t p = (uint32_t)LO(1) * LO(2) + LO(0);      /* MUL  */
                   SETLO(0, (word)(p >> 16)); SETLO(1, (word)p); return; }
    case 0xCFC8: { int32_t p = (int32_t)(int16_t)LO(1) * (int16_t)LO(2)
                              + (int16_t)LO(0);
                   SETLO(0, (word)(p >> 16)); SETLO(1, (word)p); return; }
    case 0xD7C8: { uint32_t nu = ((uint32_t)LO(0) << 16) | LO(1);     /* DIV  */
                   if (!LO(2) || LO(0) >= LO(2)) { C = 1; return; }
                   SETLO(1, (word)(nu / LO(2))); SETLO(0, (word)(nu % LO(2)));
                   C = 0; return; }
    case 0xDFC8: case 0xBFC8: {                                       /* DIVS/DIVX */
                   int32_t nu;
                   if ((ir & 0x87FF) == 0xBFC8)
                       SETLO(0, (LO(1) & 0x8000) ? 0xFFFFu : 0);
                   nu = (int32_t)(((uint32_t)LO(0) << 16) | LO(1));
                   if (!LO(2)) { C = 1; return; }
                   SETLO(1, (word)(nu / (int16_t)LO(2)));
                   SETLO(0, (word)(nu % (int16_t)LO(2))); C = 0; return; }
    /* Decimal add and subtract: one BCD digit out of the low four bits of
     * each accumulator, with the carry as the decimal carry in and out. */
    case 0x8088: { unsigned d = (LO(acd) & 0xF) + (LO(acs) & 0xF) + (unsigned)C;
                   C = d > 9; if (C) d -= 10;
                   SETLO(acd, (word)((LO(acd) & 0xFFF0) | d)); return; }
    case 0x80C8: { int d = (int)(LO(acd) & 0xF) - (int)(LO(acs) & 0xF) - (C ? 0 : 1);
                   C = d >= 0; if (d < 0) d += 10;
                   SETLO(acd, (word)((LO(acd) & 0xFFF0) | (unsigned)d)); return; }

    /* The double-length shifts.  On the Eclipse these work on an
     * accumulator pair; on the MV the accumulator is already 32 bits wide, so
     * they are shifts of the whole register. */
    case 0x8388: { unsigned n = 4 * (acs + 1);                        /* DHXL */
                   AC[acd] = n >= 32 ? 0 : AC[acd] << n; return; }
    case 0x83C8: { unsigned n = 4 * (acs + 1);                        /* DHXR */
                   AC[acd] = n >= 32 ? 0 : AC[acd] >> n; return; }
    case 0x82C8: { int n = (int8_t)(LO(acs) & 0xFF);                  /* DLSH */
                   AC[acd] = n >= 0 ? (n >= 32 ? 0 : AC[acd] << n)
                                    : (-n >= 32 ? 0 : AC[acd] >> -n); return; }
    case 0x87F8: SETLO(acd, LO(acd) | M[MADDR(at + 1)]); used = 2; break;
    case 0xA7F8: SETLO(acd, LO(acd) ^ M[MADDR(at + 1)]); used = 2; break;
    case 0xC7F8: SETLO(acd, LO(acd) & M[MADDR(at + 1)]); used = 2; break;
    case 0xE7F8: SETLO(acd, LO(acd) + M[MADDR(at + 1)]); used = 2; break;

    /* ---- the Eclipse two-word memory reference, which the MV kept ----- */
    case 0xA438: a = e2ea(ir, at, &used); AC[acd] = sx16(M[MADDR(a)]); break; /* ELDA */
    case 0xC438: a = e2ea(ir, at, &used); M[MADDR(a)] = (word)AC[acd]; break; /* ESTA */
    case 0xE438: a = e2ea(ir, at, &used); AC[acd] = RING | a; break;          /* ELEF */
    case 0x8438: a = e2ea(ir, at, &used); PC = a; return;                     /* EJMP */
    case 0x8C38: a = e2ea(ir, at, &used);                                     /* EJSR */
                 AC[3] = RING | ((at + used) & OFFMASK); PC = a; return;
    case 0x9438: a = e2ea(ir, at, &used);                                     /* EISZ */
                 { word v = (word)(M[MADDR(a)] + 1); M[MADDR(a)] = v;
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0x9C38: a = e2ea(ir, at, &used);                                     /* EDSZ */
                 { word v = (word)(M[MADDR(a)] - 1); M[MADDR(a)] = v;
                   if (!v) { PC = (at + used) & OFFMASK; wskip(); return; } } break;
    case 0x8478: a = e2ea(ir, at, &used); AC[acd] = rdbyte(bytep(a)); break;  /* ELDB */
    case 0xA478: a = e2ea(ir, at, &used); wrbyte(bytep(a), (word)AC[acd]); break;

    case 0x8609: {                                                   /* XCALL */
        /* The X form of LCALL: a 16-bit displacement to the target and then
         * an argument count, and the same extra doubleword on the stack. */
        dword tgt = xea(ir, at, 0, hi, &used);
        word  cnt = M[MADDR(at + 2)];
        used = 3;
        wpushdw(cnt);
        AC[3] = RING | ((at + used) & OFFMASK);
        PC = tgt & OFFMASK;
        return; }

    /* ---- add and subtract a small immediate to memory ----------------
     * The X forms (type 28) take a word displacement, the L forms (type 15)
     * a doubleword one; the immediate, 1 to 4, is in bits 14-13, and N or W
     * in the name says whether the cell is a word or a doubleword. */
    case 0x8418: case 0x8458: case 0x8518: case 0x8558:
    case 0x8618: case 0x8658: case 0x8718: case 0x8758: {
        int lform = (m->op >= 0x8618);
        int wide  = (m->op == 0x8518 || m->op == 0x8558 ||
                     m->op == 0x8718 || m->op == 0x8758);
        int sub   = (m->op == 0x8458 || m->op == 0x8558 ||
                     m->op == 0x8658 || m->op == 0x8758);
        int32_t k = (int32_t)acs + 1;
        a = xea(ir, at, lform, 0, &used);
        if (sub) k = -k;
        if (wide) wrw(a, rdw(a) + (dword)k);
        else      M[MADDR(a)] = (word)(M[MADDR(a)] + (word)k);
        break; }

    /* ---- immediates that act on the whole 32-bit accumulator ---------- */
    case 0xE6F9: AC[acd] += sx16(M[MADDR(at + 1)]); used = 2; break; /* WNADI */
    case 0xE6D9: { int n = (int16_t)M[MADDR(at + 1)];                /* WLSHI */
                   AC[acd] = n >= 0 ? (n >= 32 ? 0 : AC[acd] << n)
                                    : (-n >= 32 ? 0 : AC[acd] >> -n);
                   used = 2; break; }
    case 0xC6A9: { int n = (int16_t)M[MADDR(at + 1)];                /* WASHI */
                   AC[acd] = n >= 0 ? (n >= 32 ? 0 : AC[acd] << n)
                        : (dword)((int32_t)AC[acd] >> (-n >= 32 ? 31 : -n));
                   used = 2; break; }

    /* ---- push and pop the effective BYTE address ---------------------- */
    case 0xA629: wpushdw(bea(ir, at, 0, hi, &used)); break;          /* XPEFB */

    /* ---- the top of the stack as an accumulator ----------------------- */
    case 0x8649: AC[acd] = rdw(DW(WSP_A) & OFFMASK); return;         /* LDATS */
    case 0x8659: wrw(DW(WSP_A) & OFFMASK, AC[acd]); return;          /* STATS */
    case 0x8648: { unsigned k = acs; for (;;) { wpushdw(sx16(LO(k))); /* PSH */
                     if (k == acd) break; k = (k + 1) & 3; } return; }
    case 0x8688: { unsigned k = acs; for (;;) { SETLO(k, (word)wpopdw());
                     if (k == acd) break; k = (k - 1) & 3; } return; }

    /* WMOVR is "move right": the accumulator shifted down one place, the
     * wide relative of the Eclipse's MOVR.  Its neighbours in MASM's table
     * are WHLV, which shifts down one arithmetically, and CVWN, which
     * narrows -- so a third spelling of CVWN is what it is not.  The PL/I
     * file package settles it: at 7C462 in FERRET.PR it loads the caller's
     * buffer, which arrives as a byte pointer, does WMOVR on it, and files
     * the answer where everything afterwards treats it as a word address --
     * once as the target of an indirect store and once as the index of an
     * XLEFB that shifts it back up.  Only a shift down turns E0006010 into
     * 70003008 and makes both readings agree.  Sign-extending the low half
     * instead leaves 6010, the file package writes the line it just read a
     * whole segment away from the variable the game passed it, and Ferret
     * answers every command with "I'm sorry?" because the line it parses is
     * empty.  The carry is left alone; nothing seen so far reads it after. */
    case 0xE699: AC[acd] >>= 1; return;                              /* WMOVR */

    /* C719 is the one instruction in these programs that MASM's own table
     * does not have -- 897 records, one contiguous run, and no entry for it --
     * so the runtime's author wrote the word by hand.  It appears exactly
     * twice, in I?SALLOC with the operand 001C and in I.FREE with 001A, and it
     * is one step of a walk along the free-storage queue.
     *
     * The loop shape settles what it has to do.  At 7EC10 in ZORK.PR:
     *
     *     7EC10  C719 001C     one step of the scan
     *     7EC12  89B8          skip 0 -- the queue is exhausted
     *     7EC13  FB78          skip 1 -- WBR -3, straight back to 7EC10
     *     7EC14  C379 E7C9     skip 2 -- WMOV 2,0 / DEQUE, take this block
     *
     * The "try the next" landing branches back to the instruction itself with
     * nothing in between, so the instruction has to advance the walk.  What
     * follows the DEQUE confirms the reading: it loads the block's size word,
     * subtracts the request and compares the remainder with 8, i.e. it is a
     * first-fit allocator deciding whether to split what it just found.
     *
     * The state, measured at 7EC10 during ZORK.PR's SAVE: AC0 = 8, the words
     * wanted; AC1 = 700753E8, the block being looked at; AC2 = 70000160, the
     * queue header; AC3 = -2, the displacement of a block's size word.  That
     * block's size word held 24 and its link word FFFFFFFF.
     *
     * DEQUE, which the runtime already leans on and which was implemented long
     * before this, fixes the rest of the layout: it takes the cell in AC0,
     * follows what it points at to that element's own first word, and treats
     * FFFFFFFF as the end.  So the link lives at offset 0 and -1 ends the
     * chain -- and, because DEQUE unlinks whatever the cell in AC0 points at
     * rather than searching, AC2 has to walk along with AC1 as the
     * *predecessor's* link cell.  That is what makes the WMOV 2,0 / DEQUE on
     * the skip-2 landing unlink the block that was found instead of the one
     * at the head, and it is why AC2 is stepped here.
     *
     * What the operand distinguishes is still unknown, so only the one whose
     * behaviour has been observed is implemented.  Only the allocator's 001C
     * has ever been seen to run: a Zork session with two saves and two
     * restores executes it four times and never reaches I.FREE's 001A at all.
     * 001A therefore keeps the old conservative answer, "exhausted", which
     * makes the freer put the block at the head of the list -- correct for a
     * storage freer, only wasteful.
     *
     * Before this, 001C answered "exhausted" too, so the allocator extended
     * memory rather than reuse a block that fitted.  With it those four scans
     * all find their block, and ZORK.PR's transcript is byte-identical either
     * side of the change, saves and restores included.  FERRET.PR executes no
     * C719 at all, so it cannot be affected, and its transcript is unchanged
     * as well. */
    case 0xC719: {                                                   /* QSCAN */
        dword elem = AC[1], req = AC[0];
        if (M[MADDR(at + 1)] != 0x001C) { used = 2; break; } /* I.FREE: as before */
        if (elem == 0xFFFFFFFFu || (elem & OFFMASK) == 0)
            { used = 2; break; }                            /* queue exhausted   */
        if (rdw(elem + AC[3]) >= req) { used = 4; break; }   /* this block fits   */
        AC[2] = elem;                                       /* predecessor cell  */
        AC[1] = rdw(elem);                                  /* on to the next    */
        used = 3; break;
    }

    /* ---- limits check (CLM): trap if AC is outside a pair in memory ----
     * PL/I uses it for subscript checking.  Nothing here raises the trap;
     * the carry says whether the value was in range. */
    case 0x8569: {                                                   /* WCLM */
        /* Naming one accumulator twice puts the limits inline, as two
         * doublewords after the instruction; otherwise the value is in ACS and
         * they are the pair at the address in ACD -- the runtime's storage
         * freer does LLEF 0,[0168] to point AC0 at the heap's low and high
         * bounds and then WCLM 2,0 to ask whether the block in AC2 is one of
         * its own.  It skips when the value is within them, which
         * is what makes the bare `WCLM / WBR` pairs in DISCO.PR read as
         * range tests.
         *
         * It leaves the carry alone.  That is not a detail: the PL/I runtime's
         * .SYSTM thunk uses two WCLMs to decide whether the call number is one
         * it handles itself, and it sits inside a caller that sets the carry
         * with CRYTO before the call and clears it with CRYTZ on the word the
         * success return lands on.  The carry is how failure gets back.  A
         * WCLM that writes the carry destroys that, and every system call
         * looks like it succeeded -- FERRET.PR then takes the error code from
         * a ?GTMES that had no argument to give as the length of one. */
        int32_t lo, hi2, v = (int32_t)AC[acs];
        int in;
        if (acs == acd) { lo = (int32_t)rdw(at + 1); hi2 = (int32_t)rdw(at + 3);
                          used = 5; }
        else            { dword p = AC[acd];
                          lo = (int32_t)rdw(p); hi2 = (int32_t)rdw(p + 2); }
        in = (v >= lo && v <= hi2);
        if (in) { PC = (at + used) & OFFMASK; wskip(); return; }
        break; }

    /* ---- the floating point fix and float, wide ----------------------- */
    case 0x84A9: FPAC[acd] = d_to_dg((double)(int32_t)AC[acs]);      /* WFLAD */
                 fp_setcc(FPAC[acd]); return;
    case 0x8499: AC[acs] = (dword)(int32_t)dg_to_d(FPAC[acd]); return; /* WFFAD */
    case 0x87B9: return;                                             /* WFPSH */
    case 0xA789: return;                                             /* WFPOP */

    /* ---- XCT, execute the instruction in an accumulator --------------- */
    case 0xA6F8: { word ins = (word)AC[acd];                         /* XCT */
                   if ((ins & 0x8000) && (ins & 0xF) == 9) wide_exec(ins, at);
                   else narrow_exec(ins, at);
                   return; }

    /* ---- character scan and translate --------------------------------
     * WCST scans the string at AC3 for a byte that is set in the table at
     * AC0, counting AC1 bytes down as it goes; WCTR translates through a
     * 256-byte table.  Both leave the pointers past what they consumed.
     *
     * The table is a 256-bit map in sixteen words, addressed by word -- the
     * program hands it over with LLEF, not LPEFB -- and numbered the way DG
     * numbers bits, 0 at the top: character c is word c/16, mask 8000 shifted
     * right c mod 16.  The two tables ZORK.PR's tokeniser uses prove both
     * halves of that: one is a single 8000 in the first word, "stop at a
     * null", and the other is every bit but the first of the third word,
     * "stop at anything that is not a blank".
     *
     * AC1 is the count and comes back as the number of bytes still unscanned,
     * the matching byte included -- zero meaning the scan ran off the end.
     * The caller can then recover the position: it primes AC2 with the count
     * plus one and subtracts what is left.  Reading the count out of AC0
     * instead leaves AC1 untouched, so a word that ends at the end of the
     * line comes back one byte long, and ZORK.PR answers every command with
     * "I don't know the word ''." */
    case 0xE709: {                                                   /* WCST */
        int32_t n = (int32_t)AC[1], k;
        dword sp = AC[3], tb = AC[0] & OFFMASK;
        for (k = 0; k < n; k++) {
            unsigned ch = rdbyte(sp + (dword)k);
            if (M[MADDR((tb + (ch >> 4)) & OFFMASK)] & (0x8000u >> (ch & 15)))
                break;
        }
        AC[1] = (dword)(n - k); AC[3] = sp + (dword)k;
        C = (k < n);
        return; }
    case 0x8769: {                                                   /* WCTR */
        /* The translate table's byte pointer is on the stack, not in an
         * accumulator: FERRET.PR at 7D7E0 pushes it with LPEFB, loads the
         * stack pointer into AC0 with LDASP, runs WCTR, and pops it again
         * with WPOP 0,0.  So AC0 addresses the pushed doubleword, AC1 is the
         * length, AC2 the destination and AC3 the source -- the same
         * destination/source pair WCMV uses.  Reading the table out of an
         * accumulator instead writes the translated bytes through whatever
         * the caller left there, which in FERRET.PR is page zero. */
        dword tb = rdw((AC[0] - 1) & OFFMASK);
        int32_t n = (int32_t)AC[1], k;
        dword sp = AC[3], dp = AC[2];
        if (n < 0) n = -n;
        for (k = 0; k < n; k++)
            wrbyte(dp + (dword)k, rdbyte(tb + rdbyte(sp + (dword)k)));
        AC[1] = 0; AC[2] = dp + (dword)n; AC[3] = sp + (dword)n;
        return; }

    /* ---- LDSP, the dispatch ------------------------------------------
     * The Eclipse DSPA, widened.  The effective address is the first table
     * entry; the two doublewords in front of it are the low and high bounds,
     * and an index outside them falls through to the next instruction.  Each
     * entry is a word displacement from its own address, the same convention
     * the X and L forms use for PC-relative operands.
     *
     * FERRET.PR at 7E4D6 dispatches AC1 over a table of four with bounds 1
     * and 4 whose entries read 8, 11, 13, 13; taken from each entry's own
     * address those land on 7E4E6, 7E4EB, 7E4EF and 7E4F1, all instruction
     * boundaries and all four arms of the same routine.  Taken from the
     * table base instead, one of the four lands mid-instruction. */
    case 0x8519: {                                                   /* LDSP */
        int32_t lo, hi2, v = (int32_t)AC[acd];
        a = xea(ir, at, 1, hi, &used);
        lo  = (int32_t)rdw(a - 4);
        hi2 = (int32_t)rdw(a - 2);
        if (v >= lo && v <= hi2) {
            dword e = (a + (dword)((v - lo) * 2)) & OFFMASK;
            dword d = rdw(e);
            if (d != 0xFFFFFFFFu) {
                PC = (dword)((int32_t)e + (int32_t)d) & OFFMASK;
                return;
            }
        }
        break; }

    /* ---- skip on masked bits (types 0C and 12) ------------------------
     * NSANA / WSANA "skip if all of the immediate's bits are on"; the NSAN*
     * pair take a 16-bit immediate and the WSAN* pair a 32-bit one, and the
     * M spelling tests the mask's bits being off instead. */
    case 0xE609: case 0xE619: case 0xE629: case 0xE639:
    case 0xA689: case 0xA699: case 0xA6A9: case 0xA6B9: {
        int wide = (m->op < 0xE000);
        dword mask = wide ? rdw(at + 1) : sx16(M[MADDR(at + 1)]);
        dword got  = AC[acd] & mask;
        int take;
        used = wide ? 3 : 2;
        switch (m->op) {
        case 0xE609: case 0xA699: take = (got == mask); break;  /* SAL A */
        case 0xE619: case 0xA6B9: take = (got != mask); break;  /* SAL M */
        case 0xE629: case 0xA689: take = (got == 0);    break;  /* SAN A */
        default:                  take = (got != 0);    break;  /* SAN M */
        }
        if (take) { PC = (at + used) & OFFMASK; wskip(); return; }
        break; }

    /* ---- the commercial decimal instructions ------------------------- */
    case 0xE679: {                                                   /* WLDI */
        int n = (int)(AC[1] & 0x1F), neg;
        FPAC[acd] = d_to_dg(dec_read(AC[3], n, &neg));
        fp_setcc(FPAC[acd]);
        AC[3] += (dword)n;
        return; }
    case 0xE6B9: {                                                   /* WSTI */
        int n = (int)(AC[1] & 0x1F);
        dec_write(AC[3], n, dg_to_d(FPAC[acd]));
        AC[3] += (dword)n;
        return; }
    case 0xA769: wedit(); return;                                    /* WEDIT */

    /* ---- WMESS ---------------------------------------------------------
     * The third instruction of I.GINIT, with AC0 and AC1 zero, a buffer
     * address in AC2 and -1 in AC3, and a WBR to the runtime's "cannot
     * initialise" exit immediately after it -- so it skips when it succeeds.
     * Whatever it reports (its name suggests the inter-task message
     * facility), reporting success is what lets a single-task program get on
     * with initialising itself. */
    case 0xE719:                                                     /* WMESS */
        PC = (at + 1) & OFFMASK; wskip(); return;

    /* ---- the queue instructions --------------------------------------
     * The PL/I runtime keeps its free storage on one of these.  Nothing in
     * the dump documents the layout, so it is taken from the code that uses
     * it: the header is two doublewords at the address in AC0 -- the first
     * and last elements -- and an empty queue has -1 in the first, because
     * the free routine at 7ED39 in ZORK.PR loads [AC2] and compares it with
     * -1 to decide between enqueueing at the head and at the tail.  An
     * element carries its own forward link at offset 0; the block size sits
     * at -2, which is where the allocator reads it (XWLDA 1,-2,2).
     *
     * The element is in AC2 for the two enqueues, and DEQUE takes the head
     * off and hands it back in AC1 -- Ferret's allocator does WMOV 2,0 to
     * point AC0 at the header, DEQUE, then WMOV 1,2 to keep what came back.
     *
     * All three are executed through XCT out of an accumulator the runtime
     * loads with the opcode, so they will not appear in a disassembly. */
    case 0xC7E9: {                                                   /* ENQH */
        dword q = AC[0] & OFFMASK, e = AC[2] & OFFMASK, first = rdw(q);
        wrw(e, first);
        wrw(q, RING | e);
        if (first == 0xFFFFFFFFu) wrw(q + 2, RING | e);
        return; }
    case 0xC7F9: {                                                   /* ENQT */
        dword q = AC[0] & OFFMASK, e = AC[2] & OFFMASK, first = rdw(q);
        wrw(e, 0xFFFFFFFFu);
        if (first == 0xFFFFFFFFu) wrw(q, RING | e);
        else                      wrw(rdw(q + 2) & OFFMASK, RING | e);
        wrw(q + 2, RING | e);
        return; }
    case 0xE7C9: {                                                   /* DEQUE */
        dword q = AC[0] & OFFMASK, head = rdw(q);
        if (head == 0xFFFFFFFFu) { AC[1] = 0xFFFFFFFFu; return; }
        { dword next = rdw(head & OFFMASK);
          wrw(q, next);
          if (next == 0xFFFFFFFFu) wrw(q + 2, 0xFFFFFFFFu);
          AC[1] = head; }
        PC = (at + 1) & OFFMASK; wskip(); return; }

    /* ---- the DO loop (types 2A and 17) --------------------------------
     * XNDO ac,index(idx),end -- the loop header.  The index variable is a
     * word (XNDO) or a doubleword (XWDO) in memory; the limit arrives in AC,
     * which the loop reloads before every pass because the header overwrites
     * it with the index.  That reload is why the branch closing the loop
     * lands two or three words BEFORE the header: at 7D2E0 in ZORK.PR a
     * WBR of -31 goes back to 7D2C1, an XWLDA of the limit, and only then
     * to the XNDO at 7D2C3.
     *
     * The index is stepped first and the stepped value is what both the
     * accumulator and memory hold for the body -- the case-folding loop
     * reads its source byte through the accumulator and writes the result
     * back through the memory copy, in place, so the two have to agree.
     *
     * The exit displacement is relative to the first displacement word. */
    case 0x8498: case 0x8598: {                              /* XNDO / XWDO */
        int wide = (m->op == 0x8598);
        unsigned ac = (ir >> 13) & 3, ix = (ir >> 11) & 3;
        word d1 = M[MADDR(at + 1)];
        word d2 = M[MADDR(at + 2)];
        dword ea;
        int32_t idx, lim = (int32_t)AC[ac];
        used = 3;
        switch (ix) {
        case 0:  ea = (dword)(int16_t)d1; break;
        case 1:  ea = (dword)((int32_t)(at + 1) + (int16_t)d1); break;
        case 2:  ea = (dword)((int32_t)AC[2] + (int16_t)d1); break;
        default: ea = (dword)((int32_t)AC[3] + (int16_t)d1); break;
        }
        ea &= OFFMASK;
        idx = wide ? (int32_t)rdw(ea) : (int32_t)(int16_t)M[MADDR(ea)];
        idx++;
        if (idx > lim) { PC = (dword)((int32_t)(at + 1) + (int16_t)d2) & OFFMASK;
                         return; }
        if (wide) wrw(ea, (dword)idx); else M[MADDR(ea)] = (word)idx;
        AC[ac] = (dword)idx;
        break; }

    /* ---- WSKBO / WSKBZ (type 24) --------------------------------------
     * Skip on a bit of AC0, numbered DG's way from the high end.  The bit
     * number is five bits, split across the fields the encoding leaves:
     * 14-12 above, 5-4 below.  The compiler uses them to copy one flag into
     * another, WBTO / WSKBO / WBTZ around a single bit. */
    case 0x8F49: case 0x8F89: {
        unsigned n = (((ir >> 12) & 7) << 2) | ((ir >> 4) & 3);
        int one = (AC[0] >> (31 - n)) & 1;
        if (m->op == 0x8F49 ? one : !one) { PC = (at + 1) & OFFMASK; wskip();
                                            return; }
        break; }

    /* ---- the procedure call, and the kernel gate --------------------- */
    case 0xA6C9: {                                                   /* LCALL */
        /* A 32-bit target then an argument count.  The count goes on the
         * stack as a doubleword of its own: the callee reads its arguments at
         * WFP-12, -14, -16 ... and without that extra word between the save
         * block and the pushed arguments every one of them is off by two.
         * The return address goes in AC3, which the callee's WSAV then files
         * as the frame's ?ORTN. */
        dword tgt = rdw(at + 1);
        word  cnt = M[MADDR(at + 3)];
        used = 4;
        if ((tgt & 0xF0000000u) == 0x30000000u) { gate_call(at); return; }
        wpushdw(cnt);
        AC[3] = RING | ((at + used) & OFFMASK);
        PC = tgt & OFFMASK;
        return; }

    default:
        { char why[120];
          snprintf(why, sizeof why, "unimplemented %s (type %02X, %u words)",
                   m->name, m->type, m->len);
          die(why, ir); }
        return;
    }
    PC = (at + used) & OFFMASK;
}
