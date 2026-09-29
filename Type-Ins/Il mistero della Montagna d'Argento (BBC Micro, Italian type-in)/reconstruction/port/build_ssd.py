# -*- coding: utf-8 -*-
"""Build a self-booting Acorn DFS single-sided disc image (.ssd).

Sector 0/1 hold the catalogue; files start at sector 2.  Boot option is set to
*OPT 4,3 (EXEC !BOOT), so SHIFT+BREAK runs the game.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SECTOR = 256
TRACKS, SECS_PER_TRACK = 80, 10
TOTAL_SECTORS = TRACKS * SECS_PER_TRACK

DISC_TITLE = "MONTAGNA"          # up to 12 characters
BOOT_OPTION = 3                  # *OPT 4,3 -> EXEC !BOOT

def entry(name, directory, load, exec_, data):
    return dict(name=name, dir=directory, load=load, exec_=exec_, data=data)

def build(files, out_path):
    cat0 = bytearray(SECTOR)
    cat1 = bytearray(SECTOR)
    title = (DISC_TITLE + " " * 12)[:12]
    cat0[0:8] = title[0:8].encode("ascii")
    cat1[0:4] = title[8:12].encode("ascii")

    sector = 2
    payload = bytearray()
    for i, f in enumerate(files):
        if i >= 31:
            raise SystemExit("DFS holds at most 31 files")
        nm = (f["name"] + " " * 7)[:7]
        o = 8 + i * 8
        cat0[o:o + 7] = nm.encode("ascii")
        cat0[o + 7] = ord(f["dir"])          # bit 7 clear = not locked

        data = f["data"]
        nsec = (len(data) + SECTOR - 1) // SECTOR
        load, exe, length = f["load"], f["exec_"], len(data)
        extra = ((sector >> 8) & 0x03) | (((load >> 16) & 0x03) << 2) \
                | (((length >> 16) & 0x03) << 4) | (((exe >> 16) & 0x03) << 6)
        c1 = cat1
        c1[o + 0] = load & 0xFF
        c1[o + 1] = (load >> 8) & 0xFF
        c1[o + 2] = exe & 0xFF
        c1[o + 3] = (exe >> 8) & 0xFF
        c1[o + 4] = length & 0xFF
        c1[o + 5] = (length >> 8) & 0xFF
        c1[o + 6] = extra
        c1[o + 7] = sector & 0xFF

        payload += data + bytes((-len(data)) % SECTOR)
        sector += nsec

    cat1[4] = 0                                   # cycle number
    cat1[5] = len(files) * 8
    cat1[6] = ((TOTAL_SECTORS >> 8) & 0x03) | ((BOOT_OPTION & 0x03) << 4)
    cat1[7] = TOTAL_SECTORS & 0xFF

    img = bytearray(TOTAL_SECTORS * SECTOR)
    img[0:SECTOR] = cat0
    img[SECTOR:2 * SECTOR] = cat1
    img[2 * SECTOR:2 * SECTOR + len(payload)] = payload
    open(out_path, "wb").write(img)
    used = sector - 2
    print("wrote %s  (%d sectors used of %d)" % (out_path, used, TOTAL_SECTORS - 2))
    for f in files:
        print("   $.%-7s %5d bytes" % (f["name"], len(f["data"])))

if __name__ == "__main__":
    tok = open(os.path.join(HERE, "montagna.tok"), "rb").read()
    boot = b'CHAIN"ARGENTO"\r'
    files = [
        # BBC BASIC's LOAD/CHAIN place the program at PAGE, but record the
        # conventional addresses anyway.
        entry("ARGENTO", "$", 0xFFFF1900, 0xFFFF8023, tok),
        entry("!BOOT",   "$", 0xFFFF1900, 0xFFFF8023, boot),
    ]
    build(files, os.path.join(HERE, "MONTAGNA.ssd"))
