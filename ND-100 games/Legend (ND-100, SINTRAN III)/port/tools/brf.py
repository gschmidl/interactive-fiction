"""ND-100 BRF (binary relocatable format) object files: read, lay out, write.

The record types are those of the table in the SINTRAN II Operator's Guide
(ND-60.044.01, "Binary Relocatable Format Code"): a control byte, then its
16-bit parameters, big-endian.

  01 LF   w       word w at CLC                    0d LBR  name  library entry
  02 LR   w       word w+PB at CLC                 0e ENTR name  entry at CLC
  08 SFL  w       CLC := w                         0f BEG        start of unit
  09 AFL  w       CLC += w (zeros)                 10 REF  name  word := address
  0a SFL  w       CLC := w+PB                      11 END  sum   end of unit
  14 LNF  n w..   n words at CLC                   13 EOF        end of file
  00 FEED         between units
  1a              names from here to END take three words (eight characters)

Found by checking every unit of BASLIBR-H00 (not in the manual):
  - a name is two words holding five 6-bit characters (ASCII & 077),
    right-justified: 7INIT = 0x3724E254, 7ASI = 0x00DC14C9;
  - END's word makes the 16-bit sum of every control byte and parameter
    word from BEG to END, END's own included, zero;
  - a unit's first word is loaded at PB+1.
"""
import struct

NPAR = {0x1a: 0, 0x01: 1, 0x02: 1, 0x03: 1, 0x04: 2, 0x05: 2, 0x06: 2, 0x07: 2, 0x08: 1, 0x09: 1, 0x0a: 1,
        0x0c: 2, 0x0d: 2, 0x0e: 2, 0x0f: 0, 0x10: 2, 0x11: 1, 0x13: 0, 0x16: 3, 0x17: 2,
        0x18: 3, 0x19: 2}
NAMES = {0x01: 'LF', 0x02: 'LR', 0x08: 'SFL', 0x09: 'AFL', 0x0a: 'SFR', 0x0c: 'MAIN',
         0x0d: 'LBR', 0x0e: 'ENTR', 0x0f: 'BEG', 0x10: 'REF', 0x11: 'END', 0x13: 'EOF',
         0x14: 'LNF'}


def name_of(*words):
    v, n = 0, 0
    for w in words:
        v, n = (v << 16) | w, n + 16
    s = ''
    for k in range(n // 6):
        c = (v >> (6 * (n // 6 - 1 - k))) & 0o77
        if c:
            s += chr(c + 0o100 if c < 0o40 else c)
    return s


def code_of(name, words=2):
    v = 0
    for ch in name:
        v = (v << 6) | (ord(ch) & 0o77)
    return [(v >> (16 * (words - 1 - k))) & 0xffff for k in range(words)]


def parse(data, start=0, stop=None):
    """records (offset, control, [words]) until stop or an unknown control;
    returns (records, offset where it stopped)"""
    stop = len(data) if stop is None else stop
    out, i, long_names = [], start, False
    while i < stop:
        c = data[i]
        if c == 0:
            i += 1
            continue
        if c == 0x1a:
            long_names = True
        elif c == 0x11:
            long_names = False
        if c == 0x14:
            if i + 3 > stop:
                break
            n = (data[i + 1] << 8) | data[i + 2]
            if i + 3 + 2 * n > stop:
                break
            out.append((i, c, list(struct.unpack('>%dH' % (n + 1), data[i + 1:i + 3 + 2 * n]))))
            i += 3 + 2 * n
            continue
        n = NPAR.get(c)
        if n is not None and long_names and c in (0x0c, 0x0d, 0x0e, 0x10):
            n = 3
        if n is None or i + 1 + 2 * n > stop:
            break
        out.append((i, c, list(struct.unpack('>%dH' % n, data[i + 1:i + 1 + 2 * n])) if n else []))
        i += 1 + 2 * n
    return out, i


def units(records):
    """split into units: lists of records from BEG to END"""
    out, cur = [], None
    for r in records:
        if r[1] == 0x0f:
            cur = [r]
        elif cur is not None:
            cur.append(r)
            if r[1] == 0x11:
                out.append(cur)
                cur = None
    return out


def checksum(unit_records):
    """the END word for these records (BEG .. the record before END)"""
    s = 0x11
    for _, c, ws in unit_records:
        if c == 0x11:
            break
        s += c + sum(ws)
    return (-s) & 0xffff


def layout(unit):
    """what the unit puts where, relative to PB: {rel: ('LF'|'LR'|'REF', value)},
    entries {name: rel}, library names [..]"""
    clc, words, entries, lbr = 1, {}, {}, []
    for _, c, ws in unit:
        if c == 0x01:
            words[clc] = ('LF', ws[0]); clc += 1
        elif c == 0x02:
            words[clc] = ('LR', ws[0]); clc += 1
        elif c == 0x10:
            words[clc] = ('REF', name_of(*ws)); clc += 1
        elif c == 0x14:
            for w in ws[1:]:
                words[clc] = ('LF', w); clc += 1
        elif c == 0x08:
            clc = ws[0]
        elif c == 0x09:
            clc += ws[0]
        elif c == 0x0a:
            clc = ws[0]
        elif c == 0x0e:
            entries[name_of(*ws)] = clc
        elif c == 0x0d:
            lbr.append(name_of(*ws))
    return words, entries, lbr


def encode(records):
    out = bytearray()
    for _, c, ws in records:
        out.append(c)
        for w in ws:
            out += struct.pack('>H', w & 0xffff)
    return bytes(out)
