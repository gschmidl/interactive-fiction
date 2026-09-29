# Minimal Z80 disassembler sufficient for analysis
R = ['B','C','D','E','H','L','(HL)','A']
RP = ['BC','DE','HL','SP']
RP2 = ['BC','DE','HL','AF']
CC = ['NZ','Z','NC','C','PO','PE','P','M']
ALU = ['ADD A,','ADC A,','SUB ','SBC A,','AND ','XOR ','OR ','CP ']
ROT = ['RLC','RRC','RL','RR','SLA','SRA','SLL','SRL']

def dis(mem, pc, base=0):
    """returns (text, length, targets, kind) ; mem is bytes indexed by absolute addr-base"""
    start = pc
    def b(i=0):
        return mem[pc - base + i]
    def w(i=0):
        return mem[pc-base+i] | (mem[pc-base+i+1] << 8)
    op = b()
    t = None; kind='n'
    def nn(): return w(1)
    x = op >> 6; y = (op >> 3) & 7; z = op & 7; p = y >> 1; q = y & 1
    if op == 0xCB:
        op2 = b(1); x2=op2>>6; y2=(op2>>3)&7; z2=op2&7
        if x2==0: s = "%s %s"%(ROT[y2],R[z2])
        elif x2==1: s = "BIT %d,%s"%(y2,R[z2])
        elif x2==2: s = "RES %d,%s"%(y2,R[z2])
        else: s = "SET %d,%s"%(y2,R[z2])
        return s,2,[],'n'
    if op in (0xDD,0xFD):
        ix = 'IX' if op==0xDD else 'IY'
        op2 = b(1)
        if op2==0xCB:
            d=b(2); op3=b(3); x3=op3>>6;y3=(op3>>3)&7
            nm = {0:ROT[y3],1:'BIT %d,'%y3,2:'RES %d,'%y3,3:'SET %d,'%y3}[x3]
            return "%s (%s%+d)"%(nm,ix,d if d<128 else d-256),4,[],'n'
        sub,l,_,_ = dis(mem, pc+1, base)
        sub = sub.replace('(HL)','(%s+d)'%ix).replace('HL',ix)
        extra = 1 if '+d' in sub else 0
        return sub, l+1+extra, [], 'n'
    if op == 0xED:
        op2=b(1); y2=(op2>>3)&7; z2=op2&7; p2=y2>>1; q2=y2&1
        if op2 in (0x43,0x53,0x63,0x73):
            return "LD (%04X),%s"%(w(2),RP[p2]),4,[],'n'
        if op2 in (0x4B,0x5B,0x6B,0x7B):
            return "LD %s,(%04X)"%(RP[p2],w(2)),4,[],'n'
        names={0x44:'NEG',0x45:'RETN',0x4D:'RETI',0x46:'IM 0',0x56:'IM 1',0x5E:'IM 2',
               0x47:'LD I,A',0x4F:'LD R,A',0x57:'LD A,I',0x5F:'LD A,R',0x67:'RRD',0x6F:'RLD',
               0xA0:'LDI',0xA1:'CPI',0xA2:'INI',0xA3:'OUTI',0xA8:'LDD',0xA9:'CPD',0xAA:'IND',0xAB:'OUTD',
               0xB0:'LDIR',0xB1:'CPIR',0xB2:'INIR',0xB3:'OTIR',0xB8:'LDDR',0xB9:'CPDR',0xBA:'INDR',0xBB:'OTDR'}
        if op2 in names: return names[op2],2,[],('r' if op2 in (0x45,0x4D) else 'n')
        if z2==0: return "IN %s,(C)"%R[y2],2,[],'n'
        if z2==1: return "OUT (C),%s"%R[y2],2,[],'n'
        if z2==2: return ("SBC HL,%s" if q2==0 else "ADC HL,%s")%RP[p2],2,[],'n'
        return "DB ED,%02X"%op2,2,[],'n'
    if x==0:
        if z==0:
            if y==0: return "NOP",1,[],'n'
            if y==1: return "EX AF,AF'",1,[],'n'
            d=b(1); tgt=(pc+2+(d if d<128 else d-256))&0xFFFF
            if y==2: return "JR %04X"%tgt,2,[tgt],'j'
            return "JR %s,%04X"%(CC[y-4],tgt),2,[tgt],'c'
        if z==1:
            if q==0: return "LD %s,%04X"%(RP[p],nn()),3,[],'n'
            return "ADD HL,%s"%RP[p],1,[],'n'
        if z==2:
            if q==0:
                if p==0: return "LD (BC),A",1,[],'n'
                if p==1: return "LD (DE),A",1,[],'n'
                if p==2: return "LD (%04X),HL"%nn(),3,[],'n'
                return "LD (%04X),A"%nn(),3,[],'n'
            else:
                if p==0: return "LD A,(BC)",1,[],'n'
                if p==1: return "LD A,(DE)",1,[],'n'
                if p==2: return "LD HL,(%04X)"%nn(),3,[],'n'
                return "LD A,(%04X)"%nn(),3,[],'n'
        if z==3: return ("INC %s" if q==0 else "DEC %s")%RP[p],1,[],'n'
        if z==4: return "INC %s"%R[y],1,[],'n'
        if z==5: return "DEC %s"%R[y],1,[],'n'
        if z==6: return "LD %s,%02X"%(R[y],b(1)),2,[],'n'
        return ['RLCA','RRCA','RLA','RRA','DAA','CPL','SCF','CCF'][y],1,[],'n'
    if x==1:
        if z==6 and y==6: return "HALT",1,[],'n'
        return "LD %s,%s"%(R[y],R[z]),1,[],'n'
    if x==2:
        return "%s%s"%(ALU[y],R[z]),1,[],'n'
    # x==3
    if z==0: return "RET %s"%CC[y],1,[],'n'
    if z==1:
        if q==0: return "POP %s"%RP2[p],1,[],'n'
        return ['RET','EXX','JP (HL)','LD SP,HL'][p],1,[],('r' if p==0 else ('r' if p==2 else 'n'))
    if z==2: return "JP %s,%04X"%(CC[y],nn()),3,[nn()],'c'
    if z==3:
        if y==0: return "JP %04X"%nn(),3,[nn()],'j'
        if y==2: return "OUT (%02X),A"%b(1),2,[],'n'
        if y==3: return "IN A,(%02X)"%b(1),2,[],'n'
        if y==4: return "EX (SP),HL",1,[],'n'
        if y==5: return "EX DE,HL",1,[],'n'
        if y==6: return "DI",1,[],'n'
        return "EI",1,[],'n'
    if z==4: return "CALL %s,%04X"%(CC[y],nn()),3,[nn()],'C'
    if z==5:
        if q==0: return "PUSH %s"%RP2[p],1,[],'n'
        if p==0: return "CALL %04X"%nn(),3,[nn()],'C'
        return "DB %02X"%op,1,[],'n'
    if z==6: return "%s%02X"%(ALU[y],b(1)),2,[],'n'
    return "RST %02X"%(y*8),1,[y*8],'C'
