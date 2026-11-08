"""Build a Commodore 64 disk image of Weird Wood II (1987 listing) from the reconstruction sources.

Inputs (relative to the 'reconstruction' folder):
  transcription/WW2-listing.txt   1987 BASIC listing (printout conventions, see WW2-OCR-notes.md)
  WWDAT.TXT                        sequential data files (later 1988 revision, edited for the Windows port)
  windows version/WW 0.DAT         room file (200-byte records, each byte XOR (position+1))
Output: '../Weird Wood II.d64' (and a report on stdout).

See README section 'C64 disk image' for every decision taken here.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from d64 import D64
from basic import build_prg

REC = os.path.dirname(HERE)
LISTING = os.path.join(REC, "transcription", "WW2-listing.txt")
WWDAT = os.path.join(REC, "WWDAT.TXT")
WW0 = os.path.join(REC, "windows version", "WW 0.DAT")
GAME = os.path.dirname(REC)
OUT_PROG = os.path.join(GAME, "Weird Wood II (disk 1 program).d64")
OUT_DATA = os.path.join(GAME, "Weird Wood II (disk 2 data).d64")
OUT_VFL = os.path.join(GAME, "fliplist.vfl")

CR = 13
report = []


def note(msg):
    report.append(msg)
    print(msg)


# ---------------------------------------------------------------------------------------------
# 1. BASIC program
# ---------------------------------------------------------------------------------------------
BOX = {"0": 0xB0, ".": 0xAE, "-": 0xAD, "=": 0xBD, "@": 0xC0, "]": 0xDD}
# zero-width control characters inside "" (WW2-OCR-notes.md, plus three CLRs inferred from context)
EMPTY_STR = {808: 157, 828: 157, 23572: 142, 23576: 142, 100: 147, 690: 147, 23674: 147}
FIXES = {11909: ("C0:WW 45=0:WW 44", "C0:WW 45=0:WW 46")}
# the decode routine is blanked out on the printout (REM********); rebuilt for the room data's markers:
# '/' ends a line, '^' (our marker for the final '/') ends the description and the 11-line read loop
RESTORED = {793: 'IFA$="/"THENA$=CR$', 795: 'IFA$="^"THENA$=CR$:T=10'}


def listing_char(ch: str) -> int:
    c = ord(ch)
    if "a" <= ch <= "z":
        return c - 0x61 + 0xC1  # printed lowercase = shifted PETSCII
    if "A" <= ch <= "Z":
        return c  # printed uppercase = unshifted PETSCII
    if ch == "|":
        return 0xDD  # star-grid separator (notes: almost certainly PETSCII vertical bar)
    if c > 126:
        raise ValueError(f"unexpected char {ch!r}")
    return c


def convert_line(num: int, body: str) -> bytes:
    if num in RESTORED:
        assert body == "REM********", (num, body)
        body = RESTORED[num]
        note(f"line {num}: blanked decode line restored as {body}")
    if num in FIXES:
        old, new = FIXES[num]
        assert old in body, (num, body)
        body = body.replace(old, new)
        note(f"line {num}: '{old}' -> '{new}' (WW 44 is the rank table; WW 46 is the clean save)")
    out = bytearray()
    in_q = False
    q_index = 0
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == '"':
            if not in_q and body.startswith('""', i) and num in EMPTY_STR:
                out += b'"' + bytes([EMPTY_STR[num]]) + b'"'
                i += 2
                continue
            in_q = not in_q
            if in_q:
                q_index += 1
            out.append(0x22)
            i += 1
            continue
        if ch == "\x8d":
            out.append(0x8D)
        elif in_q and 3550 <= num <= 3558 and q_index == 2 and ch in BOX:
            out.append(BOX[ch])
        elif in_q and num == 100 and ch == "7":
            out.append(0xB7)  # title underline: PETSCII 183 printed as '7'
        else:
            out.append(listing_char(ch))
        i += 1
    return bytes(out)


def read_listing():
    lines = []
    started = False
    for raw in open(LISTING, encoding="utf-8").read().splitlines():
        if raw.startswith("--- page"):
            started = True
            continue
        if not started or not raw.strip() or raw.strip() == "READY.":
            continue
        m = re.match(r"^(\d+) (.*)$", raw)
        if m and (not lines or int(m.group(1)) > lines[-1][0]):
            lines.append([int(m.group(1)), m.group(2)])
        else:
            lines[-1][1] += "\x8d" + raw  # line 497: string continues after an embedded shifted-RETURN
            note(f"line {lines[-1][0]}: joined continuation with CHR$(141)")
    return [(n, convert_line(n, b)) for n, b in lines]


# ---------------------------------------------------------------------------------------------
# 2. text conversion for data files
# ---------------------------------------------------------------------------------------------
CP437 = {}
for k, v in {0xC4: 0xC0, 0xCD: 0xC0, 0xB3: 0xDD, 0xBA: 0xDD,
             0xDA: 0xB0, 0xC9: 0xB0, 0xD5: 0xB0, 0xD6: 0xB0,
             0xBF: 0xAE, 0xBB: 0xAE, 0xB8: 0xAE, 0xB7: 0xAE,
             0xC0: 0xAD, 0xC8: 0xAD, 0xD4: 0xAD, 0xD3: 0xAD,
             0xD9: 0xBD, 0xBC: 0xBD, 0xBE: 0xBD, 0xBD: 0xBD,
             0xC3: 0xAB, 0xCC: 0xAB, 0xC6: 0xAB, 0xC7: 0xAB,
             0xB4: 0xB3, 0xB9: 0xB3, 0xB5: 0xB3, 0xB6: 0xB3,
             0xC2: 0xB2, 0xCB: 0xB2, 0xD1: 0xB2, 0xD2: 0xB2,
             0xC1: 0xB1, 0xCA: 0xB1, 0xCF: 0xB1, 0xD0: 0xB1,
             0xC5: 0xDB, 0xCE: 0xDB, 0xD7: 0xDB, 0xD8: 0xDB,
             0xB0: 0xA6, 0xB1: 0xA6, 0xB2: 0xA6,
             0xDC: 0xA2, 0xDD: 0xA1, 0x9C: 0x5C}.items():
    CP437[k] = bytes([v])
# blocks drawn in reverse video: full block, upper half, right half
REVERSE = {0xDB: 0x20, 0xDF: 0xA2, 0xDE: 0xA1}


def petscii(text: bytes) -> bytes:
    """Display-case ASCII/CP437 -> PETSCII for lower/upper-case mode."""
    out = bytearray()
    rvs = False
    for c in text:
        if c in REVERSE:
            if not rvs:
                out.append(18)
                rvs = True
            out.append(REVERSE[c])
            continue
        if rvs:
            out.append(146)
            rvs = False
        if 0x61 <= c <= 0x7A:
            out.append(c - 0x20)
        elif 0x41 <= c <= 0x5A:
            out.append(c + 0x80)
        elif c == 0x5C:
            out.append(0x2F)  # backslash has no PETSCII equivalent in lower/upper mode
        elif c in CP437:
            out += CP437[c]
        elif 0x20 <= c < 0x7F:
            out.append(c)
        else:
            raise ValueError(f"unmapped byte {c:#x} in {text!r}")
    if rvs:
        out.append(146)
    return bytes(out)


def visible_len(text: bytes) -> int:
    return sum(1 for c in text if c not in (18, 146))


def read_wwdat():
    secs = {}
    cur = None
    for line in open(WWDAT, "rb").read().split(b"\r\n"):
        if line.startswith(b"*#*"):
            cur = line[3:].decode().replace(".DAT", "")
            secs[cur] = []
            continue
        if cur and cur != "END":
            secs[cur].append(line)
    for k in secs:
        while secs[k] and secs[k][-1] == b"":
            secs[k].pop()
    return secs


def get_file(lines, markers=b"@$#", page=None) -> bytes:
    """File read character by character with GET# (lines 650-690, 23678-23688)."""
    out = bytearray()
    for raw in lines:
        if raw == b"!":
            raw = b""
        if page:
            raw = raw.replace(b"$", page)
        body = petscii(raw)
        vis = visible_len(bytes(c for c in body if c not in markers))
        out += body
        if raw and vis == 0:
            continue  # marker-only line: no newline of its own
        if vis < 40:
            out.append(CR)
    return bytes(out) + b"#"


def input_line(raw: bytes) -> bytes:
    """One line for INPUT#: quoted so commas, colons and leading spaces survive."""
    if raw == b"!":
        raw = b""
    body = petscii(raw)
    if b'"' in body:
        if any(c in body for c in b",:") or body.startswith(b" "):
            body = body.replace(b'"', b"'")
        else:
            return body + bytes([CR])
    return b'"' + body + b'"' + bytes([CR])


def input_file(lines, end_marker=True) -> bytes:
    out = b"".join(input_line(l) for l in lines)
    return out + (b"#\r" if end_marker else b"")


def plain_lines(lines) -> bytes:
    return b"".join(petscii(l) + bytes([CR]) for l in lines)


# ---------------------------------------------------------------------------------------------
# 3. vocabulary tables: CHR$(number)+word per entry (lines 120, 132, 406-412, 610-620)
# ---------------------------------------------------------------------------------------------
NOUN_PATCHED = {84: 32, 90: 13, 91: 32, 92: 34, 93: 44}  # set up by lines 134-135


def vocab_file(lines, name, patched=None) -> (bytes, int):
    pairs = [(int(lines[i].strip()), lines[i + 1]) for i in range(0, len(lines), 2)]
    out = bytearray()
    for idx, (n, word) in enumerate(pairs, 1):
        if patched and idx in patched:
            # the program builds these itself (lines 134-135); the file holds the plain word, which the
            # port split into 'number' (its first letter) and the rest
            if not 0x61 <= n <= 0x7A:
                raise SystemExit(f"{name}: entry {idx} expected a plain word, got number {n}")
            out += b'"' + petscii(bytes([n]) + word) + b'"\r'
            continue
        if n in (13, 32, 34, 44, 58) or n == 0:
            raise SystemExit(f"{name}: entry {idx} ({word!r}) has number {n}, which INPUT# cannot carry")
        out += b'"' + bytes([n]) + petscii(word) + b'"\r'
    return bytes(out), len(pairs)


# ---------------------------------------------------------------------------------------------
# 4. WW 47: machine code at $C000 (lost; rebuilt from how the program calls it)
# ---------------------------------------------------------------------------------------------
def build_ww47(descs) -> bytes:
    """$C000: recolour the screen with the cursor colour (SYS49152, after F1/F3/F5; first byte $A0, line 10).
    $C017: print the room description of object A; the program pokes 2A+118 / 2A+119 into $C01C / $C022
    (line 196), i.e. the low bytes of a word table at $C076 + 2A."""
    code = bytearray()
    code += bytes([0xA0, 0x00,              # C000 LDY #0
                   0xAD, 0x86, 0x02,        # C002 LDA $0286
                   0x99, 0x00, 0xD8,        # C005 STA $D800,Y
                   0x99, 0x00, 0xD9,        # C008 STA $D900,Y
                   0x99, 0x00, 0xDA,        # C00B STA $DA00,Y
                   0x99, 0xE8, 0xDA,        # C00E STA $DAE8,Y
                   0xC8,                    # C011 INY
                   0xD0, 0xF1,              # C012 BNE $C005
                   0x60,                    # C014 RTS
                   0xEA, 0xEA])             # C015
    assert len(code) == 0x17
    code += bytes([0xA0, 0x00,              # C017 LDY #0
                   0xEA, 0xEA,              # C019
                   0xAD, 0x00, 0xC0,        # C01B LDA $C0xx   (xx poked at $C01C)
                   0x85, 0xFB,              # C01E STA $FB
                   0xEA,                    # C020
                   0xAD, 0x00, 0xC0,        # C021 LDA $C0xx   (xx poked at $C022)
                   0x85, 0xFC,              # C024 STA $FC
                   0xB1, 0xFB,              # C026 LDA ($FB),Y
                   0xF0, 0x06,              # C028 BEQ $C030
                   0x20, 0xD2, 0xFF,        # C02A JSR $FFD2
                   0xC8,                    # C02D INY
                   0xD0, 0xF6,              # C02E BNE $C026
                   0x60])                   # C030 RTS
    assert code[0x1B] == 0xAD and code[0x21] == 0xAD
    code += b"\xEA" * (0x76 - len(code))
    table_at = 0x76
    strings_at = table_at + 2 * 67
    code += b"\0" * (strings_at - len(code))
    strs = bytearray()
    for a in range(67):
        addr = 0xC000 + strings_at + len(strs)
        struct.pack_into("<H", code, table_at + 2 * a, addr)
        if a >= 1 and descs[a - 1] != b"*":
            body = petscii(descs[a - 1])
            strs += body + (bytes([CR]) if visible_len(body) % 40 else b"") + b"\0"
        else:
            strs += b"\0"
    code += strs
    if 0xC000 + len(code) > 0xD000:
        raise SystemExit("WW 47 too big")
    return b"\x00\xC0" + bytes(code)


# ---------------------------------------------------------------------------------------------
# 5. WW 0: relative file, 200-byte records (lines 20-22, 705-760)
# ---------------------------------------------------------------------------------------------
def build_rooms():
    d = open(WW0, "rb").read()
    recs = [bytes(b ^ ((i + 1) & 0xFF) for i, b in enumerate(d[r * 200:(r + 1) * 200])) for r in range(len(d) // 200)]
    out = [None] * len(recs)
    r = 0
    joins = []
    while r < len(recs):
        group = [r]
        while recs[group[-1]][199] == 0xFE:
            group.append(group[-1] + 1)
        exits = recs[r][:40]
        text = bytearray()
        for k, g in enumerate(group):
            part = recs[g][40:] if k == 0 else recs[g]
            if part[-1] == 0xFE:
                part = part[:-1] + b" "  # the port's continuation marker sits where a space belongs
                if k + 1 < len(group):
                    joins.append((g + 1, bytes(part[-12:]), bytes(recs[g + 1][:12])))
            text += part
        text = bytes(text).rstrip(b"\0")
        if b"/" in text:
            text = text[:text.rindex(b"/") + 1]
        text = text.rstrip(b" ").replace(b"\0", b"")
        if text.endswith(b"/"):
            text = text[:-1]
        if text.count(b"/") > 10:
            raise SystemExit(f"room {r+1}: too many lines")
        body = petscii(text) + b"^"
        vals = [struct.unpack_from("<h", exits, 2 * k)[0] for k in range(20)]
        raw_exits = b"".join(struct.pack("<H", v + 1600) for v in vals)
        blob = raw_exits + bytes(body)
        cap = 200 * len(group)
        if len(blob) > cap:
            # no room for the end marker: end the 40-column lines that are padded with spaces with '/'
            rows = [text[i:i + 40] for i in range(0, len(text), 40)]
            text = b"".join((row.rstrip(b" ") + b"/") if len(row) == 40 and row.endswith(b" ") else row for row in rows)
            body = petscii(text) + b"^"
            blob = raw_exits + bytes(body)
            note(f"room {r+1}: padded line ends turned into '/' to make room for the end marker")
        if len(blob) > cap:
            raise SystemExit(f"room {r+1}: {len(blob)} bytes, room for {cap}")
        for k, g in enumerate(group):
            chunk = blob[200 * k:200 * (k + 1)]
            out[g] = chunk if chunk else bytes([CR])
        r = group[-1] + 1
    return out, joins


# ---------------------------------------------------------------------------------------------
def main():
    prog = read_listing()
    prg = build_prg(prog)
    note(f"program: {len(prog)} lines, {len(prg)} bytes ({(len(prg) + 253) // 254} blocks), ends at ${0x0801 + len(prg) - 2:04X}")

    secs = read_wwdat()
    files = []  # (name, type, data)

    # message files read with GET# (lines 650-690)
    for n in range(1, 14):
        files.append((f"WW {n}", "SEQ", get_file(secs[f"WW {n}"])))
    # star game instructions (lines 23678-23688): from the later WW 60; '$' page break -> '_' (press SPACE) + '^' (header)
    files.append(("WW 14", "SEQ", get_file([b"^"] + secs["WW 60"], markers=b"^_#", page=b"_^")))
    note("WW 14: star-game instructions taken from the later WW 60 (page breaks converted to _ and ^)")
    # pictures / examine texts read with INPUT# until '#' (lines 3061-3069): WW 15.. (A+14)
    for n in list(range(15, 40)) + list(range(49, 66)):
        if f"WW {n}" in secs:
            files.append((f"WW {n}", "SEQ", input_file(secs[f"WW {n}"])))

    v, nv = vocab_file(secs["WW 40"], "WW 40")
    nn_data, nn = vocab_file(secs["WW 41"], "WW 41", NOUN_PATCHED)
    w42 = secs["WW 42"]
    objs = w42[:132]
    descs = w42[132:198]
    assert len(descs) == 66, len(descs)
    o = bytearray()
    for i in range(0, 132, 2):
        o += input_line(objs[i]) + petscii(objs[i + 1].strip()) + b"\r"
    w43 = secs["WW 43"]
    assert len(w43) == 80, len(w43)
    t43 = bytearray()
    for i, l in enumerate(w43):
        is_num = 38 <= i < 59
        t43 += (petscii(l.strip()) + b"\r") if is_num else input_line(l)
    nums = [int(x.strip()) for x in w43[38:41]]
    note(f"WW 43: VB={nums[0]} OB={nums[1]} NU={nums[2]}; WW 40 has {nv} verbs, WW 41 {nn} nouns")
    assert nums == [nv, 66, nn]

    save = list(secs["WW 45"])
    assert len(save) == 114, len(save)
    assert save[81].startswith(b"40320"), save[81]  # SK$, followed by the three colours (line 34131)
    save[82:85] = [b" 0 ", b" 0 ", b" 3 "]  # background, border, text colour (all 0 in the port's file)
    note("WW 45/46: colours set to black background, black border, cyan text (port file had 0,0,0)")
    savedata = plain_lines(save)

    names = list(secs["WW 48"])
    if len(names) < 10:
        note(f"WW 48: {len(names)} names, program reads 10 (line 158); repeated '{names[0].decode()}' as the 10th")
        names.append(names[0])

    files += [
        ("WW 40", "SEQ", v), ("WW 41", "SEQ", nn_data), ("WW 42", "SEQ", bytes(o)), ("WW 43", "SEQ", bytes(t43)),
        ("WW 44", "SEQ", input_file(secs["WW 44"], end_marker=False)),
        ("WW 45", "SEQ", savedata), ("WW 46", "SEQ", savedata),
        ("WW 47", "PRG", build_ww47(descs)),
        ("WW 48", "SEQ", input_file(names, end_marker=False)),
    ]

    rooms, joins = build_rooms()
    for g, a, b in joins[:8]:
        note(f"continuation into record {g + 1}: ...{a!r} | {b!r}...")
    note(f"WW 0: {len(rooms)} records, {len(joins)} continuation records")

    # program disk + data disk, as the program expects (line 12: "PLEASE INSERT DATA DISK & PRESS RETURN")
    pdisk = D64("WEIRD WOOD II", "W1")
    pdisk.add_file("WEIRD WOOD II", "PRG", prg)
    ddisk = D64("WEIRD WOOD DATA", "W2")
    ddisk.add_rel("WW 0", 200, rooms)
    order = sorted(files, key=lambda f: int(f[0].split()[1]))
    for name, ftype, data in order:
        ddisk.add_file(name, ftype, data)
    note(f"program disk: {pdisk.blocks_free()} blocks free; data disk: {len(order) + 1} files, {ddisk.blocks_free()} blocks free")
    for path, disk in ((OUT_PROG, pdisk), (OUT_DATA, ddisk)):
        open(path, "wb").write(disk.image())
        note(f"written {path}")
    with open(OUT_VFL, "w", encoding="ascii", newline="\n") as f:
        f.write("# Vice fliplist file\n\nUNIT 8\n")
        f.write(os.path.basename(OUT_PROG) + "\n" + os.path.basename(OUT_DATA) + "\n")
    note(f"written {OUT_VFL}")


if __name__ == "__main__":
    main()
