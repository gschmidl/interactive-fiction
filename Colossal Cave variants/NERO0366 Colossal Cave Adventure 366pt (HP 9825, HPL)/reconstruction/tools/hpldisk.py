"""Read files from an HP 9825 mass-storage floppy image (HP 9895, 256-byte sectors).

Directory: 16-byte entries from offset 0x100: name (6 bytes, NUL padded),
start sector, sector count, length word, type word, spare word.
Types seen: 0x0800 program, 0x0400 data, 0x2400 data (128-word records),
0x0c00 special-function keys.

Data files are sequences of items (HPL sprt/rprt format), each starting
with a header word:
  low 6 bits 0x1a / 0x1b   number, positive / negative; exponent = header >> 6
                           (10-bit two's complement); then 12 BCD digits d.ddddd...
  low 6 bits 0x3c          whole string: length word, bytes, pad to even
             0x1c          first part of a split string: total length word,
                           bytes to the end of the record
             0x0c          middle part: remaining length word, bytes to end of record
             0x2c          last part: remaining length word, bytes, pad
  0x001e                   end of record (the rest of the record is old data)
  0x003e                   end of file
"""
import struct
from decimal import Decimal

SECTOR = 256


def directory(disk):
    entries = []
    for off in range(0x100, 0x1000, 16):
        e = disk[off:off + 16]
        if e[0] == 0:
            break
        name = e[:6].rstrip(b'\0').decode('latin-1')
        start, nsec, length, ftype = struct.unpack('>HHHH', e[6:14])
        entries.append({'name': name, 'start': start, 'sectors': nsec, 'length': length, 'type': ftype})
    return entries


def file_bytes(disk, entry):
    return disk[entry['start'] * SECTOR:(entry['start'] + entry['sectors']) * SECTOR]


def number(header, body):
    exp = header >> 6
    if exp >= 512:
        exp -= 1024
    digits = body.hex()
    if not digits.isdigit():
        raise ValueError(f'bad BCD {digits}')
    value = Decimal(int(digits)).scaleb(exp - 11)
    if header & 1:
        value = -value
    # exact integers come out as ints, everything else as a Decimal
    if value == value.to_integral_value():
        return int(value)
    return value.normalize()


def records(buf, recsize=SECTOR):
    """Return [(record number, [items])], split strings joined into the
    record where they start.  Record numbers count from 1 as in HPL's
    rread/rprt.  Items are int/Decimal/str."""
    return list(_records(buf, recsize))


def serial(buf, recsize=SECTOR):
    """All items of a file as one stream (what sread sees)."""
    return [item for _, items in records(buf, recsize) for item in items]


def _records(buf, recsize):
    pending = None          # [items list, index, collected str]
    for rec in range(len(buf) // recsize):
        r = buf[rec * recsize:(rec + 1) * recsize]
        items, pos = [], 0
        while pos + 2 <= recsize:
            w = struct.unpack('>H', r[pos:pos + 2])[0]
            kind = w & 0x3f
            if w in (0x001e, 0x003e):
                break
            if kind in (0x1a, 0x1b):
                items.append(number(w, r[pos + 2:pos + 8]))
                pos += 8
            elif kind == 0x3c:
                ln = struct.unpack('>H', r[pos + 2:pos + 4])[0]
                items.append(r[pos + 4:pos + 4 + ln].decode('latin-1'))
                pos += 4 + ln + (ln & 1)
            elif kind == 0x1c:
                ln = struct.unpack('>H', r[pos + 2:pos + 4])[0]
                part = r[pos + 4:].decode('latin-1')
                items.append(None)
                pending = [items, len(items) - 1, part]
                pos = recsize
            elif kind in (0x0c, 0x2c):
                ln = struct.unpack('>H', r[pos + 2:pos + 4])[0]
                take = ln if kind == 0x2c else recsize - pos - 4
                part = r[pos + 4:pos + 4 + take].decode('latin-1')
                if pending:
                    pending[2] += part
                    if kind == 0x2c:
                        pending[0][pending[1]] = pending[2]
                        pending = None
                pos += 4 + take + (take & 1)
                if kind == 0x0c:
                    pos = recsize
            else:
                items.append(f'<?{r[pos:pos + 8].hex()}>')
                break
        yield rec + 1, items


def words(s):
    """2-byte big-endian integers packed into a string with HPL's fti()."""
    b = s.encode('latin-1')
    return [struct.unpack('>h', b[i:i + 2])[0] for i in range(0, len(b) - 1, 2)]
