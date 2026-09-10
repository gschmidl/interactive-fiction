/* dis.c - VAX disassembler, enough to read compiler and RTL output */
#include "vax.h"

/* size codes: 1=byte 2=word 4=long 8=quad 16=octa; f/d/g share 4/8/8 */
typedef struct {
    const char *name;
    signed char nops;      /* operand count, -1 = special */
    signed char size;      /* default operand size */
    signed char br;        /* 1 = trailing byte displacement, 2 = word displacement */
} Op;

static Op ops[512];
static int inited;

static void def(int op, const char *nm, int nops, int size, int br)
{
    ops[op].name = nm; ops[op].nops = (signed char)nops;
    ops[op].size = (signed char)size; ops[op].br = (signed char)br;
}

static void init(void)
{
    int i;
    if (inited) return;
    inited = 1;
    for (i = 0; i < 512; i++) def(i, NULL, 0, 4, 0);

    def(0x00,"HALT",0,4,0);  def(0x01,"NOP",0,4,0);   def(0x02,"REI",0,4,0);
    def(0x03,"BPT",0,4,0);   def(0x04,"RET",0,4,0);   def(0x05,"RSB",0,4,0);
    def(0x0A,"INDEX",6,4,0);
    def(0x0C,"PROBER",3,1,0); def(0x0D,"PROBEW",3,1,0);
    def(0x0E,"INSQUE",2,1,0); def(0x0F,"REMQUE",2,1,0);
    def(0x10,"BSBB",0,4,1);  def(0x11,"BRB",0,4,1);
    def(0x12,"BNEQ",0,4,1);  def(0x13,"BEQL",0,4,1);  def(0x14,"BGTR",0,4,1);
    def(0x15,"BLEQ",0,4,1);  def(0x16,"JSB",1,1,0);   def(0x17,"JMP",1,1,0);
    def(0x18,"BGEQ",0,4,1);  def(0x19,"BLSS",0,4,1);  def(0x1A,"BGTRU",0,4,1);
    def(0x1B,"BLEQU",0,4,1); def(0x1C,"BVC",0,4,1);   def(0x1D,"BVS",0,4,1);
    def(0x1E,"BGEQU",0,4,1); def(0x1F,"BLSSU",0,4,1);
    def(0x28,"MOVC3",3,1,0); def(0x29,"CMPC3",3,1,0);
    def(0x2A,"SCANC",4,1,0); def(0x2B,"SPANC",4,1,0);
    def(0x2C,"MOVC5",5,1,0); def(0x2D,"CMPC5",5,1,0);
    def(0x2E,"MOVTC",6,1,0); def(0x2F,"MOVTUC",6,1,0);
    def(0x30,"BSBW",0,4,2);  def(0x31,"BRW",0,4,2);
    def(0x32,"CVTWL",2,2,0); def(0x33,"CVTWB",2,2,0);
    def(0x39,"MATCHC",4,1,0);
    def(0x3A,"LOCC",3,1,0);  def(0x3B,"SKPC",3,1,0);
    def(0x3C,"MOVZWL",2,2,0); def(0x3D,"ACBW",3,2,2);
    def(0x3E,"MOVAW",2,2,0); def(0x3F,"PUSHAW",1,2,0);
    def(0x40,"ADDF2",2,4,0); def(0x41,"ADDF3",3,4,0);
    def(0x42,"SUBF2",2,4,0); def(0x43,"SUBF3",3,4,0);
    def(0x44,"MULF2",2,4,0); def(0x45,"MULF3",3,4,0);
    def(0x46,"DIVF2",2,4,0); def(0x47,"DIVF3",3,4,0);
    def(0x48,"CVTFB",2,4,0); def(0x49,"CVTFW",2,4,0); def(0x4A,"CVTFL",2,4,0);
    def(0x4B,"CVTRFL",2,4,0); def(0x4C,"CVTBF",2,1,0); def(0x4D,"CVTWF",2,2,0);
    def(0x4E,"CVTLF",2,4,0); def(0x4F,"ACBF",3,4,2);
    def(0x50,"MOVF",2,4,0);  def(0x51,"CMPF",2,4,0);  def(0x52,"MNEGF",2,4,0);
    def(0x53,"TSTF",1,4,0);  def(0x54,"EMODF",5,4,0); def(0x55,"POLYF",3,4,0);
    def(0x56,"CVTFD",2,4,0);
    def(0x58,"ADAWI",2,2,0);
    def(0x5C,"INSQHI",2,1,0); def(0x5D,"INSQTI",2,1,0);
    def(0x5E,"REMQHI",2,8,0); def(0x5F,"REMQTI",2,8,0);
    def(0x60,"ADDD2",2,8,0); def(0x61,"ADDD3",3,8,0);
    def(0x62,"SUBD2",2,8,0); def(0x63,"SUBD3",3,8,0);
    def(0x64,"MULD2",2,8,0); def(0x65,"MULD3",3,8,0);
    def(0x66,"DIVD2",2,8,0); def(0x67,"DIVD3",3,8,0);
    def(0x68,"CVTDB",2,8,0); def(0x69,"CVTDW",2,8,0); def(0x6A,"CVTDL",2,8,0);
    def(0x6B,"CVTRDL",2,8,0); def(0x6C,"CVTBD",2,1,0); def(0x6D,"CVTWD",2,2,0);
    def(0x6E,"CVTLD",2,4,0); def(0x6F,"ACBD",3,8,2);
    def(0x70,"MOVD",2,8,0);  def(0x71,"CMPD",2,8,0);  def(0x72,"MNEGD",2,8,0);
    def(0x73,"TSTD",1,8,0);  def(0x74,"EMODD",5,8,0); def(0x75,"POLYD",3,8,0);
    def(0x76,"CVTDF",2,8,0);
    def(0x78,"ASHL",3,4,0);  def(0x79,"ASHQ",3,8,0);
    def(0x7A,"EMUL",4,4,0);  def(0x7B,"EDIV",4,4,0);
    def(0x7C,"CLRQ",1,8,0);  def(0x7D,"MOVQ",2,8,0);
    def(0x7E,"MOVAQ",2,8,0); def(0x7F,"PUSHAQ",1,8,0);
    def(0x80,"ADDB2",2,1,0); def(0x81,"ADDB3",3,1,0);
    def(0x82,"SUBB2",2,1,0); def(0x83,"SUBB3",3,1,0);
    def(0x84,"MULB2",2,1,0); def(0x85,"MULB3",3,1,0);
    def(0x86,"DIVB2",2,1,0); def(0x87,"DIVB3",3,1,0);
    def(0x88,"BISB2",2,1,0); def(0x89,"BISB3",3,1,0);
    def(0x8A,"BICB2",2,1,0); def(0x8B,"BICB3",3,1,0);
    def(0x8C,"XORB2",2,1,0); def(0x8D,"XORB3",3,1,0);
    def(0x8E,"MNEGB",2,1,0); def(0x8F,"CASEB",3,1,0);
    def(0x90,"MOVB",2,1,0);  def(0x91,"CMPB",2,1,0);  def(0x92,"MCOMB",2,1,0);
    def(0x93,"BITB",2,1,0);  def(0x94,"CLRB",1,1,0);  def(0x95,"TSTB",1,1,0);
    def(0x96,"INCB",1,1,0);  def(0x97,"DECB",1,1,0);
    def(0x98,"CVTBL",2,1,0); def(0x99,"CVTBW",2,1,0);
    def(0x9A,"MOVZBL",2,1,0); def(0x9B,"MOVZBW",2,1,0);
    def(0x9C,"ROTL",3,4,0);  def(0x9D,"ACBB",3,1,2);
    def(0x9E,"MOVAB",2,1,0); def(0x9F,"PUSHAB",1,1,0);
    def(0xA0,"ADDW2",2,2,0); def(0xA1,"ADDW3",3,2,0);
    def(0xA2,"SUBW2",2,2,0); def(0xA3,"SUBW3",3,2,0);
    def(0xA4,"MULW2",2,2,0); def(0xA5,"MULW3",3,2,0);
    def(0xA6,"DIVW2",2,2,0); def(0xA7,"DIVW3",3,2,0);
    def(0xA8,"BISW2",2,2,0); def(0xA9,"BISW3",3,2,0);
    def(0xAA,"BICW2",2,2,0); def(0xAB,"BICW3",3,2,0);
    def(0xAC,"XORW2",2,2,0); def(0xAD,"XORW3",3,2,0);
    def(0xAE,"MNEGW",2,2,0); def(0xAF,"CASEW",3,2,0);
    def(0xB0,"MOVW",2,2,0);  def(0xB1,"CMPW",2,2,0);  def(0xB2,"MCOMW",2,2,0);
    def(0xB3,"BITW",2,2,0);  def(0xB4,"CLRW",1,2,0);  def(0xB5,"TSTW",1,2,0);
    def(0xB6,"INCW",1,2,0);  def(0xB7,"DECW",1,2,0);
    def(0xB8,"BISPSW",1,2,0); def(0xB9,"BICPSW",1,2,0);
    def(0xBA,"POPR",1,2,0);  def(0xBB,"PUSHR",1,2,0);
    def(0xBC,"CHMK",1,2,0);  def(0xBD,"CHME",1,2,0);
    def(0xBE,"CHMS",1,2,0);  def(0xBF,"CHMU",1,2,0);
    def(0xC0,"ADDL2",2,4,0); def(0xC1,"ADDL3",3,4,0);
    def(0xC2,"SUBL2",2,4,0); def(0xC3,"SUBL3",3,4,0);
    def(0xC4,"MULL2",2,4,0); def(0xC5,"MULL3",3,4,0);
    def(0xC6,"DIVL2",2,4,0); def(0xC7,"DIVL3",3,4,0);
    def(0xC8,"BISL2",2,4,0); def(0xC9,"BISL3",3,4,0);
    def(0xCA,"BICL2",2,4,0); def(0xCB,"BICL3",3,4,0);
    def(0xCC,"XORL2",2,4,0); def(0xCD,"XORL3",3,4,0);
    def(0xCE,"MNEGL",2,4,0); def(0xCF,"CASEL",3,4,0);
    def(0xD0,"MOVL",2,4,0);  def(0xD1,"CMPL",2,4,0);  def(0xD2,"MCOML",2,4,0);
    def(0xD3,"BITL",2,4,0);  def(0xD4,"CLRL",1,4,0);  def(0xD5,"TSTL",1,4,0);
    def(0xD6,"INCL",1,4,0);  def(0xD7,"DECL",1,4,0);
    def(0xD8,"ADWC",2,4,0);  def(0xD9,"SBWC",2,4,0);
    def(0xDC,"MOVPSL",1,4,0); def(0xDD,"PUSHL",1,4,0);
    def(0xDE,"MOVAL",2,4,0); def(0xDF,"PUSHAL",1,4,0);
    def(0xE0,"BBS",2,4,1);   def(0xE1,"BBC",2,4,1);
    def(0xE2,"BBSS",2,4,1);  def(0xE3,"BBCS",2,4,1);
    def(0xE4,"BBSC",2,4,1);  def(0xE5,"BBCC",2,4,1);
    def(0xE6,"BBSSI",2,4,1); def(0xE7,"BBCCI",2,4,1);
    def(0xE8,"BLBS",1,4,1);  def(0xE9,"BLBC",1,4,1);
    def(0xEA,"FFS",4,4,0);   def(0xEB,"FFC",4,4,0);
    def(0xEC,"CMPV",4,4,0);  def(0xED,"CMPZV",4,4,0);
    def(0xEE,"EXTV",4,4,0);  def(0xEF,"EXTZV",4,4,0);
    def(0xF0,"INSV",4,4,0);  def(0xF1,"ACBL",3,4,2);
    def(0xF2,"AOBLSS",2,4,1); def(0xF3,"AOBLEQ",2,4,1);
    def(0xF4,"SOBGEQ",1,4,1); def(0xF5,"SOBGTR",1,4,1);
    def(0xF6,"CVTLB",2,4,0); def(0xF7,"CVTLW",2,4,0);
    def(0xFA,"CALLG",2,4,0); def(0xFB,"CALLS",2,4,0);
    def(0xFC,"XFC",0,4,0);
}

static const char *rn(int r)
{
    static const char *n[16] = { "R0","R1","R2","R3","R4","R5","R6","R7",
                                 "R8","R9","R10","R11","AP","FP","SP","PC" };
    return n[r & 15];
}

/* decode one operand starting at *pa; append text to out */
static void dis_opnd(u32 *pa, int size, char *out, size_t osz)
{
    u8 spec = rd8((*pa)++);
    int mode = spec >> 4, reg = spec & 15;
    char sub[64];
    switch (mode) {
    case 0: case 1: case 2: case 3:
        snprintf(out, osz, "S^#%02X", spec & 0x3F); break;
    case 4:
        dis_opnd(pa, size, sub, sizeof sub);
        snprintf(out, osz, "%s[%s]", sub, rn(reg)); break;
    case 5: snprintf(out, osz, "%s", rn(reg)); break;
    case 6: snprintf(out, osz, "(%s)", rn(reg)); break;
    case 7: snprintf(out, osz, "-(%s)", rn(reg)); break;
    case 8:
        if (reg == 15) {
            u32 v = 0; int i;
            for (i = 0; i < (size > 4 ? 4 : size); i++) v |= (u32)rd8(*pa + i) << (8*i);
            *pa += size;
            snprintf(out, osz, "I^#%X", v);
        } else snprintf(out, osz, "(%s)+", rn(reg));
        break;
    case 9:
        if (reg == 15) { u32 v = rd32(*pa); *pa += 4; snprintf(out, osz, "@#%08X", v); }
        else snprintf(out, osz, "@(%s)+", rn(reg));
        break;
    case 10: case 11: {
        i8 d = (i8)rd8((*pa)++);
        if (reg == 15) snprintf(out, osz, "%sB^%08X", mode==11?"@":"", *pa + d);
        else snprintf(out, osz, "%sB^%X(%s)", mode==11?"@":"", (u32)(i32)d, rn(reg));
        break; }
    case 12: case 13: {
        i16 d = (i16)rd16(*pa); *pa += 2;
        if (reg == 15) snprintf(out, osz, "%sW^%08X", mode==13?"@":"", *pa + d);
        else snprintf(out, osz, "%sW^%X(%s)", mode==13?"@":"", (u32)(i32)d, rn(reg));
        break; }
    default: {
        i32 d = (i32)rd32(*pa); *pa += 4;
        if (reg == 15) snprintf(out, osz, "%sL^%08X", mode==15?"@":"", *pa + d);
        else snprintf(out, osz, "%sL^%X(%s)", mode==15?"@":"", (u32)d, rn(reg));
        break; }
    }
}

/* Disassemble one instruction at va; returns the address of the next. */
u32 dis_one(u32 va, char *out, size_t osz)
{
    u32 a = va, op;
    Op *o;
    char line[256], opnd[64];
    int i;
    init();
    op = rd8(a++);
    if (op == 0xFD) op = 0x100 | rd8(a++);
    o = &ops[op];
    if (!o->name) { snprintf(out, osz, "%08X:  .BYTE %02X", va, op & 0xFF); return va + 1; }
    snprintf(line, sizeof line, "%-8s", o->name);
    for (i = 0; i < o->nops; i++) {
        dis_opnd(&a, o->size, opnd, sizeof opnd);
        strncat(line, i ? "," : " ", sizeof line - strlen(line) - 1);
        strncat(line, opnd, sizeof line - strlen(line) - 1);
    }
    if (o->br == 1) { i8 d = (i8)rd8(a++);
        snprintf(opnd, sizeof opnd, "%s%08X", o->nops ? "," : " ", a + d);
        strncat(line, opnd, sizeof line - strlen(line) - 1); }
    else if (o->br == 2) { i16 d = (i16)rd16(a); a += 2;
        snprintf(opnd, sizeof opnd, "%s%08X", o->nops ? "," : " ", a + d);
        strncat(line, opnd, sizeof line - strlen(line) - 1); }
    snprintf(out, osz, "%08X:  %s", va, line);
    return a;
}

void dis_range(u32 lo, u32 hi)
{
    char buf[300];
    while (lo < hi && mem_present(lo)) {
        lo = dis_one(lo, buf, sizeof buf);
        fprintf(stderr, "%s\n", buf);
    }
}
