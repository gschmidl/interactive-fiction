"""Minimal 1541 .d64 image writer: PRG, SEQ and REL (with side sectors) files.

Layout follows CBM DOS 2.6: BAM at 18/0, directory from 18/1 (interleave 3),
file data with interleave 10, allocation starting next to the directory track.
"""

SPT = [0] + [21] * 17 + [19] * 7 + [18] * 6 + [17] * 5  # index 1..35
TYPES = {"DEL": 0x80, "SEQ": 0x81, "PRG": 0x82, "USR": 0x83, "REL": 0x84}


def petscii_name(name: str) -> bytes:
    out = bytearray()
    for ch in name:
        c = ord(ch)
        if 0x61 <= c <= 0x7A:  # lowercase ascii -> unshifted PETSCII (shows as uppercase in default mode)
            c -= 0x20
        out.append(c)
    if len(out) > 16:
        raise ValueError(f"name too long: {name}")
    return bytes(out) + b"\xA0" * (16 - len(out))


class D64:
    def __init__(self, disk_name: str, disk_id: str = "WW"):
        self.sectors = {(t, s): bytearray(256) for t in range(1, 36) for s in range(SPT[t])}
        self.free = {(t, s) for t in range(1, 36) for s in range(SPT[t]) if t != 18}
        self.dir_used = {(18, 0)}
        self.entries = []  # 30-byte dir entries (without the 2 link bytes)
        self.disk_name = disk_name
        self.disk_id = disk_id
        self.last = (17, 0)  # allocation cursor
        self.track_order = [t for d in range(1, 18) for t in (18 - d, 18 + d) if 1 <= t <= 35]

    # --- allocation -------------------------------------------------------------
    def blocks_free(self):
        return len(self.free)

    def _alloc(self):
        t0, s0 = self.last
        order = self.track_order[self.track_order.index(t0):] + self.track_order[:self.track_order.index(t0)]
        for t in order:
            n = SPT[t]
            start = (s0 + 10) % n if t == t0 else 0
            for k in range(n):
                s = (start + k) % n
                if (t, s) in self.free:
                    self.free.discard((t, s))
                    self.last = (t, s)
                    return t, s
        raise RuntimeError("disk full")

    def _write_chain(self, data: bytes, last_byte_ff=False):
        """Write data as a linked chain of 254-byte blocks. Returns list of (t,s)."""
        chunks = [data[i:i + 254] for i in range(0, max(len(data), 1), 254)] or [b""]
        blocks = [self._alloc() for _ in chunks]
        for i, (ts, chunk) in enumerate(zip(blocks, chunks)):
            sec = self.sectors[ts]
            sec[2:2 + len(chunk)] = chunk
            if i + 1 < len(blocks):
                sec[0], sec[1] = blocks[i + 1]
            else:
                sec[0] = 0
                sec[1] = 0xFF if last_byte_ff else len(chunk) + 1
        return blocks

    # --- files ------------------------------------------------------------------
    def add_file(self, name: str, ftype: str, data: bytes):
        blocks = self._write_chain(data)
        e = bytearray(30)
        e[0] = TYPES[ftype]
        e[1], e[2] = blocks[0]
        e[3:19] = petscii_name(name)
        e[28] = len(blocks) & 0xFF
        e[29] = len(blocks) >> 8
        self.entries.append(e)
        return len(blocks)

    def add_rel(self, name: str, reclen: int, records):
        data = bytearray()
        for r in records:
            if len(r) > reclen:
                raise ValueError("record too long")
            data += bytes(r) + b"\0" * (reclen - len(r))
        # fill the rest of the last block with empty records ($FF then $00s), like DOS does
        empty = b"\xFF" + b"\0" * (reclen - 1)
        while len(data) % 254:
            need = 254 - len(data) % 254
            data += empty[:need] if need < reclen else empty
        data = bytes(data)
        blocks = self._write_chain(data, last_byte_ff=True)
        nss = (len(blocks) + 119) // 120
        if nss > 6:
            raise ValueError("REL file too big for 6 side sectors")
        ss = [self._alloc() for _ in range(nss)]
        for i, ts in enumerate(ss):
            sec = self.sectors[ts]
            group = blocks[i * 120:(i + 1) * 120]
            if i + 1 < nss:
                sec[0], sec[1] = ss[i + 1]
            else:
                sec[0] = 0
                sec[1] = 15 + 2 * len(group)  # last used byte
            sec[2] = i
            sec[3] = reclen
            for j, (t, s) in enumerate(ss):
                sec[4 + 2 * j], sec[5 + 2 * j] = t, s
            for j, (t, s) in enumerate(group):
                sec[16 + 2 * j], sec[17 + 2 * j] = t, s
        e = bytearray(30)
        e[0] = TYPES["REL"]
        e[1], e[2] = blocks[0]
        e[3:19] = petscii_name(name)
        e[19], e[20] = ss[0]
        e[21] = reclen
        total = len(blocks) + nss
        e[28] = total & 0xFF
        e[29] = total >> 8
        self.entries.append(e)
        return total

    # --- output -----------------------------------------------------------------
    def image(self) -> bytes:
        # directory sectors on track 18: 1, 4, 7, 10, ...
        dir_secs = []
        s = 1
        for i in range(0, max(len(self.entries), 1), 8):
            dir_secs.append(s)
            s = (s + 3) % SPT[18]
        for k, ds in enumerate(dir_secs):
            sec = self.sectors[(18, ds)]
            if k + 1 < len(dir_secs):
                sec[0], sec[1] = 18, dir_secs[k + 1]
            else:
                sec[0], sec[1] = 0, 0xFF
            for j, e in enumerate(self.entries[k * 8:(k + 1) * 8]):
                sec[j * 32 + 2:j * 32 + 32] = e
            self.dir_used.add((18, ds))
        bam = self.sectors[(18, 0)]
        bam[0], bam[1], bam[2], bam[3] = 18, 1, 0x41, 0
        for t in range(1, 36):
            bits = 0
            n = 0
            for s in range(SPT[t]):
                free = (t, s) in self.free if t != 18 else (t, s) not in self.dir_used
                if free:
                    bits |= 1 << s
                    n += 1
            o = 4 * t
            bam[o] = n
            bam[o + 1], bam[o + 2], bam[o + 3] = bits & 0xFF, (bits >> 8) & 0xFF, (bits >> 16) & 0xFF
        bam[0x90:0xA0] = petscii_name(self.disk_name)
        bam[0xA0:0xA2] = b"\xA0\xA0"
        bam[0xA2:0xA4] = petscii_name(self.disk_id)[:2]
        bam[0xA4] = 0xA0
        bam[0xA5:0xA7] = b"2A"
        bam[0xA7:0xAB] = b"\xA0" * 4
        out = bytearray()
        for t in range(1, 36):
            for s in range(SPT[t]):
                out += self.sectors[(t, s)]
        assert len(out) == 174848
        return bytes(out)
