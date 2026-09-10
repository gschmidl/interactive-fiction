"""Tokenise ZX Spectrum BASIC listing text -> .tap"""
import struct

TOKENS = [
 "RND","INKEY$","PI","FN","POINT","SCREEN$","ATTR","AT","TAB","VAL$","CODE","VAL",
 "LEN","SIN","COS","TAN","ASN","ACS","ATN","LN","EXP","INT","SQR","SGN","ABS","PEEK",
 "IN","USR","STR$","CHR$","NOT","BIN","OR","AND","<=",">=","<>","LINE","THEN","TO",
 "STEP","DEF FN","CAT","FORMAT","MOVE","ERASE","OPEN #","CLOSE #","MERGE","VERIFY",
 "BEEP","CIRCLE","INK","PAPER","FLASH","BRIGHT","INVERSE","OVER","OUT","LPRINT",
 "LLIST","STOP","READ","DATA","RESTORE","NEW","BORDER","CONTINUE","DIM","REM","FOR",
 "GO TO","GO SUB","INPUT","LOAD","LIST","LET","PAUSE","NEXT","POKE","PRINT","PLOT",
 "RUN","SAVE","RANDOMIZE","IF","CLS","DRAW","CLEAR","RETURN","COPY"]
assert len(TOKENS)==91          # 0xA5..0xFF
CODE0 = 0xA5

def lead_space(i):   # i = index in TOKENS
    return i >= 32 and TOKENS[i][0].isalpha()
def trail_space(i):
    if i < 3: return False
    last = TOKENS[i][-1]
    return last == '$' or last >= 'A'

# longest first
ORDER = sorted(range(91), key=lambda i: -len(TOKENS[i]))

def fp5(x):
    """5-byte ZX Spectrum float"""
    if x == int(x) and 0 <= x < 65536:
        n = int(x)
        return bytes([0,0,n & 255, n >> 8, 0])
    e = 0; m = float(x)
    while m >= 1.0: m /= 2.0; e += 1
    while m < 0.5:  m *= 2.0; e -= 1
    man = int(round(m * (1 << 32)))
    if man >> 32:            # rounding overflow
        man >>= 1; e += 1
    man &= 0x7FFFFFFF        # clear implicit top bit (sign = 0, positive)
    return bytes([e + 128, (man >> 24) & 255, (man >> 16) & 255,
                  (man >> 8) & 255, man & 255])

def tokenise_line(text):
    out = bytearray(); i = 0; n = len(text); instr = False
    while i < n:
        c = text[i]
        if instr:
            out.append(ord(c)); i += 1
            if c == '"': instr = False
            continue
        if c == '"':
            out.append(34); instr = True; i += 1; continue
        # keyword?
        hit = None
        for t in ORDER:
            k = TOKENS[t]
            if text.startswith(k, i):
                hit = t; break
        if hit is not None:
            # drop the space the ROM will re-insert in front of the token
            if lead_space(hit) and out and out[-1] == 0x20:
                out.pop()
            out.append(CODE0 + hit)
            i += len(TOKENS[hit])
            if trail_space(hit) and i < n and text[i] == ' ':
                i += 1
            continue
        # number?
        if c.isdigit() or (c == '.' and i+1 < n and text[i+1].isdigit()):
            j = i
            while j < n and text[j].isdigit(): j += 1
            if j < n and text[j] == '.':
                j += 1
                while j < n and text[j].isdigit(): j += 1
            if j < n and text[j] in 'eE' and j+1 < n and (text[j+1].isdigit() or
                    (text[j+1] in '+-' and j+2 < n and text[j+2].isdigit())):
                j += 1
                if text[j] in '+-': j += 1
                while j < n and text[j].isdigit(): j += 1
            lit = text[i:j]
            out += lit.encode('latin-1')
            out.append(0x0E); out += fp5(float(lit))
            i = j; continue
        out.append(ord(c)); i += 1
    return bytes(out)

def program(lines):
    """lines: list of (number, text) -> BASIC program bytes"""
    b = bytearray()
    for num, txt in lines:
        body = tokenise_line(txt) + b'\x0d'
        b += struct.pack('>H', num) + struct.pack('<H', len(body)) + body
    return bytes(b)

def tap(prog, name, autostart=0x8000):
    def block(data):
        chk = 0
        for x in data: chk ^= x
        blk = bytes(data) + bytes([chk])
        return struct.pack('<H', len(blk)) + blk
    nm = (name + ' ' * 10)[:10].encode('latin-1')
    hdr = bytes([0x00, 0x00]) + nm + struct.pack('<HHH', len(prog), autostart, len(prog))
    return block(hdr) + block(bytes([0xFF]) + prog)

# ---- LIST renderer (reproduces the ROM's spacing) ----
def render(text_bytes):
    out = []; i = 0; n = len(text_bytes); instr = False
    while i < n:
        c = text_bytes[i]
        if instr:
            out.append(chr(c)); i += 1
            if c == 34: instr = False
            continue
        if c == 34:
            out.append('"'); instr = True; i += 1; continue
        if c == 0x0E:
            i += 6; continue          # skip binary number
        if c >= CODE0:
            t = c - CODE0
            if lead_space(t) and out and out[-1] != ' ':
                out.append(' ')
            out.append(TOKENS[t])
            if trail_space(t): out.append(' ')
            i += 1; continue
        out.append(chr(c)); i += 1
    return ''.join(out)
