"""Shared helpers for reading the two ADVENTURE 6.1 builds.

    6.1/9   archive_original/new-adventure.exe   (the 1982 archive, 13-Feb-81)
    6.1/3   ../LONG0751 Colossal Cave Adventure 751pt (PDP-10, TOPS-20)/dump_original/ADVENTURE.EXE.36

That archive stores a 36-bit word as five bytes: four 7-bit groups
(word bits 0-27), then bits 28-34 in the low seven bits of the fifth byte
and bit 35 in its top bit.  The 751 port's dumps are eight-byte big-endian
words.  Both unpack to the same integers.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ARCH = os.path.join(ROOT, "archive_original")
PORT751 = os.path.join(os.path.dirname(ROOT), "Colossal Cave Adventure 751pt (PDP-10, TOPS-20)")
DUMP751 = os.path.join(PORT751, "dump_original")

sys.path.insert(0, HERE)
import dis_tab as _dt

# The UUO numbers the 751 port confirmed by running the game.
for _k, _v in {0o054: None, 0o055: 'RENAME', 0o056: 'IN', 0o057: 'OUT', 0o060: 'SETSTS',
               0o061: 'STATO', 0o062: 'GETSTS', 0o063: 'STATZ', 0o064: 'INBUF',
               0o065: 'OUTBUF', 0o066: 'INPUT', 0o067: 'OUTPUT', 0o070: 'CLOSE',
               0o071: 'RELEAS', 0o072: 'MTAPE', 0o073: 'UGETF', 0o074: 'USETI',
               0o075: 'USETO', 0o076: 'LOOKUP', 0o077: 'ENTER', 0o100: 'UJEN'}.items():
    if _v:
        _dt.OPS[_k] = _v
    else:
        _dt.OPS.pop(_k, None)

# JSYS numbers, from the pack's own MONSYM.UNV.
JSYS = {0o11: 'ERSTR', 0o13: 'GJINF', 0o14: 'TIME', 0o15: 'RUNTM', 0o20: 'GTJFN',
        0o21: 'OPENF', 0o22: 'CLOSF', 0o34: 'CLZFF', 0o36: 'SIZEF', 0o41: 'DIRST',
        0o56: 'PMAP', 0o76: 'PSOUT', 0o150: 'RPCAP', 0o151: 'EPCAP', 0o167: 'DISMS',
        0o170: 'HALTF', 0o224: 'NOUT', 0o501: 'HPTIM', 0o502: 'CRLNM', 0o556: 'STPPN',
        0o574: 'GETOK'}


def load5(path):
    d = open(path, "rb").read()
    return [((d[i] & 0x7F) << 29) | ((d[i+1] & 0x7F) << 22) | ((d[i+2] & 0x7F) << 15)
            | ((d[i+3] & 0x7F) << 8) | ((d[i+4] & 0x7F) << 1) | (d[i+4] >> 7)
            for i in range(0, len(d) - len(d) % 5, 5)]


def load36(path):
    d = open(path, "rb").read()
    return [struct.unpack(">Q", d[i:i+8])[0] for i in range(0, len(d), 8)]


def exe_image(words):
    """TOPS-20 .EXE -> ({address: word}, start address, directory entries)."""
    mem, start, dirs, i = {}, None, [], 0
    while i < len(words):
        typ, ln = words[i] >> 18, words[i] & 0o777777
        if ln == 0:
            break
        if typ == 0o1776:
            for j in range(i + 1, i + ln, 2):
                w1, w2 = words[j], words[j + 1]
                fpage, ppage, count = w1 & 0o777777777, w2 & 0o777777777, (w2 >> 27) + 1
                dirs.append((w1 >> 27, fpage, ppage, count))
                if fpage == 0:
                    continue                    # allocated but zero
                for k in range(count):
                    base = (fpage + k) * 512
                    for o, x in enumerate(words[base:base + 512]):
                        mem[(ppage + k) * 512 + o] = x
        elif typ == 0o1775:
            start = words[i + 2] & 0o777777
        elif typ == 0o1777:
            break
        i += ln
    if not start:
        start = mem.get(0o120, 0) & 0o777777     # .JBSA
    return mem, start, dirs


def build(name):
    """'6.1/9' or '6.1/3' -> (mem, start, dirs)."""
    if name == "6.1/9":
        return exe_image(load5(os.path.join(ARCH, "new-adventure.exe")))
    if name == "6.1/3":
        return exe_image(load36(os.path.join(DUMP751, "ADVENTURE.EXE.36")))
    raise KeyError(name)


def txt(x):
    return "".join(chr((x >> (29 - 7 * k)) & 0o177) for k in range(5))


def six(x):
    return "".join(chr(((x >> (30 - 6 * k)) & 0o77) + 32) for k in range(6))


def text_file(words):
    """A 7-bit file as a string, NULs dropped."""
    return "".join(txt(w) for w in words).replace("\0", "")


R50 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.$%"


def r50(v):
    s = ""
    for _ in range(6):
        s = R50[v % 40] + s
        v //= 40
    return s.strip()


def symbols(mem):
    """The LINK symbol table behind .JBSYM: [(kind, name, value)], kind 0 module,
    1 global, 2 local (the top four bits of the name word, shifted down)."""
    jbsym = mem.get(0o116, 0)
    if not jbsym:
        return []
    n, base, out = (1 << 18) - (jbsym >> 18), jbsym & 0o777777, []
    for i in range(0, n, 2):
        w1 = mem.get(base + i, 0)
        out.append((w1 >> 32, r50(w1 & 0o37777777777), mem.get(base + i + 1, 0)))
    return out


def dis(x):
    if (x >> 27) == 0o104:
        return "JSYS   %s" % JSYS.get(x & 0o777777, "%o" % (x & 0o777777))
    return _dt.dis(x)


def line(mem, a):
    x = mem.get(a, 0)
    return "%06o: %012o  %-26s |%s| %s" % (a, x, dis(x), "".join(
        c if 32 <= ord(c) < 127 else ("." if ord(c) else " ") for c in txt(x)), six(x))


_NAME = re.compile(r"[A-Z][A-Z0-9.%$]{0,5} *$")


def routines(mem, top):
    """FORTRAN-10 puts a subprogram's SIXBIT name in the word before its entry
    point.  Return [(entry, name)] for every word that looks like one: a clean
    SIXBIT name, preceded by the end of something (POPJ, JRST, zero) and
    followed by an instruction a FORTRAN-10 entry starts with."""
    out = []
    for a in sorted(mem):
        if a >= top:
            break
        x = mem[a]
        if not x or not _NAME.match(six(x)):
            continue
        prev, nxt = mem.get(a - 1, 0) >> 27, mem.get(a + 1, 0) >> 27
        if prev in (0o254, 0o000, 0o263) and nxt in (
                0o201, 0o200, 0o260, 0o265, 0o202, 0o254, 0o550, 0o551, 0o402,
                0o400, 0o505, 0o500, 0o504, 0o205, 0o476, 0o474, 0o552, 0o261, 0o304):
            out.append((a + 1, six(x).strip()))
    return out
