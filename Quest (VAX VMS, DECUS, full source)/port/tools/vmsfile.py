"""Read RMS files as they lie on a VMS disk.

The copy in ../quest/ was taken without conversion: every file is the
block-for-block content RMS stores, so it must be decoded by organisation.
Three kinds occur, and each reader asserts the structure it relies on, so a
file that is not what it seems stops the build instead of decoding wrongly.

  sequential, variable length   2-byte length word, the data, a pad byte to
                                an even boundary; a length of 0xFFFF ends
                                the data in a block.  The sources, MON.DTA,
                                DUNNAM.DTA, ACCESS.FIL.

  relative                      VBN 1 is the prolog; from VBN 2 on, buckets
                                of fixed cells: one control byte (0x08 = the
                                record exists) and RECL data bytes.  A cell
                                that was never written holds control 0.
                                DUNGEON.DTA, MAGIC.DTA, MORAL.DTA.

  indexed, prolog 3             VBN 1..2 key descriptors, then an area
                                descriptor, then 2-block buckets.  Primary
                                data records are key- and data-compressed.
                                CHARACTER.DTA.
"""
import struct

BLOCK = 512


def var_records(data):
    """Records of a variable-length sequential file."""
    out, i = [], 0
    while i + 2 <= len(data):
        n = struct.unpack_from('<H', data, i)[0]
        if n == 0xFFFF:                       # rest of this block is unused
            i = (i // BLOCK + 1) * BLOCK
            continue
        assert i + 2 + n <= len(data), 'record runs past end of file at %d' % i
        out.append(data[i + 2:i + 2 + n])
        i += 2 + n + (n & 1)
    assert i == len(data), 'stray bytes after the last record'
    return out


def var_text(data):
    """A variable-length file as LF-terminated text, one line per record."""
    return b''.join(r + b'\n' for r in var_records(data))


def relative_cells(data, recl, bucket_blocks=1):
    """Every cell of a relative file, in record-number order, up to the last
    record that exists.  Returns [(exists, bytes)], index 0 = record 1.

    The prolog's data bucket VBN (offset 0x68) and end-of-file VBN (0x70)
    are checked against the file, so a truncated copy is refused."""
    assert len(data) % BLOCK == 0, 'relative file is not whole blocks'
    dvbn, mrn, eof, ver = struct.unpack_from('<IIIH', data, 0x68)
    assert ver == 1, 'relative prolog version %d' % ver
    assert dvbn == 2, 'first data bucket at VBN %d' % dvbn
    assert eof - 1 == len(data) // BLOCK, (
        'prolog says %d blocks, file has %d' % (eof - 1, len(data) // BLOCK))
    cell = 1 + recl
    per = bucket_blocks * BLOCK // cell
    cells = []
    for vbn in range(dvbn, eof, bucket_blocks):
        b = data[(vbn - 1) * BLOCK:(vbn - 1 + bucket_blocks) * BLOCK]
        for c in range(per):
            ctl = b[c * cell]
            assert ctl in (0, 0x08), 'cell control 0x%02x' % ctl
            cells.append((ctl == 0x08, b[c * cell + 1:(c + 1) * cell]))
    while cells and not cells[-1][0]:
        cells.pop()
    return cells


def relative_records(data, recl, bucket_blocks=1):
    """A relative file with no holes, as a list of records."""
    cells = relative_cells(data, recl, bucket_blocks)
    holes = [i + 1 for i, (e, _) in enumerate(cells) if not e]
    assert not holes, 'records never written: %r' % holes[:10]
    return [d for _, d in cells]


# ---- indexed, prolog 3 ------------------------------------------------------

IRC_DELETED = 0x04
IRC_RRV = 0x08
IRC_NOPTRSZ = 0x10


def _bucket(data, vbn, blocks):
    b = data[(vbn - 1) * BLOCK:(vbn - 1 + blocks) * BLOCK]
    if len(b) < blocks * BLOCK:
        return None
    check, index, adrsample, free, nxtid, nxtbkt, level, bktcb = \
        struct.unpack_from('<BBHHHIBB', b, 0)
    assert adrsample == vbn & 0xFFFF, 'VBN %d: address sample %d' % (vbn, adrsample)
    assert check == b[-1], 'VBN %d: check byte' % vbn
    return dict(b=b, index=index, free=free, next=nxtbkt, level=level,
                last=bool(bktcb & 1), root=bool(bktcb & 2))


def _index_entries(bk):
    """Keys (front-compressed) and the 2-byte VBN pointers stored from the
    end of a level-1 index bucket."""
    b, p, prev, keys = bk['b'], 14, b'', []
    while p < bk['free']:
        n, front = b[p], b[p + 1]
        prev = prev[:front] + b[p + 2:p + 2 + n]
        keys.append(prev)
        p += 2 + n
    top = len(b) - 6
    ptrs = [struct.unpack_from('<H', b, top - 2 * i)[0] for i in range(len(keys))]
    return keys, ptrs


def indexed_file(data):
    """Decode a prolog-3 indexed file with one key-0 index level.

    Returns a dict:
      keysz, reclen    from the key-0 descriptor
      buckets          [(high key, VBN, present)] in key order, from the root
      records          live records in primary-key order, fully expanded
      deleted          how many deleted records the surviving buckets hold
      blocks_used      the area descriptor's next free VBN - 1
    """
    assert len(data) % BLOCK == 0
    k0 = data[:BLOCK]
    rootlev, idxbkt, datbkt = k0[9], k0[10], k0[11]
    rootvbn = struct.unpack_from('<I', k0, 0x0C)[0]
    flags, keysz = k0[0x10], k0[0x14]
    firstdata = struct.unpack_from('<I', k0, 0x54)[0]
    avbn, ver = k0[0x66], struct.unpack_from('<H', k0, 0x74)[0]
    assert ver == 3, 'prolog version %d' % ver
    assert rootlev == 1 and idxbkt == datbkt == 2
    assert flags & 0x80 and flags & 0x40, 'expected record and key compression'
    area = data[(avbn - 1) * BLOCK:avbn * BLOCK]
    next_free = struct.unpack_from('<I', area, 0x18)[0]

    root = _bucket(data, rootvbn, idxbkt)
    assert root and root['root'] and root['level'] == 1
    keys, ptrs = _index_entries(root)
    assert ptrs[0] == firstdata

    records, deleted, buckets = [], 0, []
    for key, vbn in zip(keys, ptrs):
        bk = _bucket(data, vbn, datbkt)
        buckets.append((key, vbn, bk is not None))
        if bk is None:
            continue
        assert bk['index'] == 0 and bk['level'] == 0
        b, p, prev = bk['b'], 14, b''
        while p < bk['free']:
            ctl = b[p]
            psz = (ctl & 3) + 2
            rrv = (struct.unpack_from('<H', b, p + 3)[0],
                   int.from_bytes(b[p + 5:p + 5 + psz], 'little'))
            q = p + 5 + psz
            if ctl & IRC_RRV:                 # forwarding pointer, no data
                p = q
                continue
            size = struct.unpack_from('<H', b, q)[0]
            q += 2
            end = q + size
            n, front = b[q], b[q + 1]
            k = prev[:front] + b[q + 2:q + 2 + n]
            prev = k
            rec = bytearray(k + k[-1:] * (keysz - len(k)))
            r = q + 2 + n
            while r < end:                    # [count][bytes][repeat last]
                m = struct.unpack_from('<H', b, r)[0]
                rec += b[r + 2:r + 2 + m]
                r += 2 + m
                rec += rec[-1:] * b[r]
                r += 1
            assert r == end, 'VBN %d: record overruns its size' % vbn
            if ctl & IRC_DELETED:
                assert len(rec) == keysz      # deleted: only the key is kept
                deleted += 1
            else:
                records.append(dict(data=bytes(rec), rrv=rrv, vbn=vbn))
            p = end
        assert p == bk['free']
    reclens = {len(r['data']) for r in records}
    assert len(reclens) == 1, reclens
    keys_seen = [r['data'][:keysz] for r in records]
    assert keys_seen == sorted(keys_seen), 'records not in key order'
    return dict(keysz=keysz, reclen=reclens.pop(), buckets=buckets,
                records=records, deleted=deleted, blocks_used=next_free - 1)


def sidr_pointers(data, keyno_vbn):
    """Live secondary-index pointers in the surviving buckets of one key's
    SIDR chain, found from that key's descriptor block.
    Returns ([(key, (id, vbn))], buckets present, buckets named)."""
    kd = data[(keyno_vbn - 1) * BLOCK:keyno_vbn * BLOCK]
    rootvbn = struct.unpack_from('<I', kd, 0x0C)[0]
    root = _bucket(data, rootvbn, kd[10])
    _, ptrs = _index_entries(root)
    out, present = [], 0
    for vbn in ptrs:
        bk = _bucket(data, vbn, kd[11])
        if bk is None:
            continue
        present += 1
        b, p, prev = bk['b'], 14, b''
        while p < bk['free']:
            size = struct.unpack_from('<H', b, p)[0]
            q, end = p + 2, p + 2 + size
            n, front = b[q], b[q + 1]
            prev = prev[:front] + b[q + 2:q + 2 + n]
            key = prev + prev[-1:] * (kd[0x14] - len(prev))
            r = q + 2 + n
            while r < end:
                ctl = b[r]
                r += 1
                if ctl & IRC_NOPTRSZ:
                    continue
                psz = (ctl & 3) + 2
                ptr = (struct.unpack_from('<H', b, r)[0],
                       int.from_bytes(b[r + 2:r + 2 + psz], 'little'))
                r += 2 + psz
                if not ctl & IRC_DELETED:
                    out.append((key, ptr))
            assert r == end
            p = end
    return out, present, len(ptrs)
