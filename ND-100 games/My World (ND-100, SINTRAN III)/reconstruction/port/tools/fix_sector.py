"""..\\src_original\\ADVENTURE-MJ.SYMB -> pascal\\recovered\\ADVENTURE-MJ.SYMB with its one
bad floppy sector read again from the bits it holds.

usage: python tools\\fix_sector.py

On MIKAEL-6 (ND-disk-00299) the 512 bytes at file offset 16896-17407 (page 8
of the file, its second sector) are not text: 191 of them have odd parity.
They are the sector's own bits, read 53 bits late.  Shifted back (three zero
bits in front, the first 56 bits dropped: 48 zero bits and a byte that is
neither), 505 bytes come out as the source that belongs there, every byte
with even parity, joining the sectors on either side: "...IF (J=8) AND
(SAK[J].RUM=0) THEN J:=10; / IF " + "(J=22) AND (RUMNR=33) ...".  What the
late read lost is the sector's last 7 bytes.  The 3 bits left over after the
last whole byte are 101, the start of a space (240B with its parity bit); the
sector ends 'END ELSE' and 7 spaces, and the next begins with 5 spaces and
IF: 7 spaces more make that IF 19 in, between the END ELSE at 16 and the
IFs below it at 22, 25 and 28, the author's steps of three.
"""
import os

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(PORT, '..', 'src_original', 'ADVENTURE-MJ.SYMB')
OUT = os.path.join(PORT, 'pascal', 'recovered', 'ADVENTURE-MJ.SYMB')
START, SIZE = 16896, 512


def main():
    d = bytearray(open(SRC, 'rb').read())
    bits = '000' + ''.join(format(b, '08b') for b in d[START:START + SIZE])
    data = bytes(int(bits[i:i + 8], 2) for i in range(56, SIZE * 8, 8))
    assert len(data) == 505
    assert bits[SIZE * 8:] == '101', bits[SIZE * 8:]
    assert all(bin(b).count('1') % 2 == 0 for b in data)
    fixed = data + bytes([0o240]) * 7
    d[START:START + SIZE] = fixed
    assert all(bin(b).count('1') % 2 == 0 for b in d)
    open(OUT, 'wb').write(bytes(d))
    text = bytes(b & 0x7F for b in d[START - 60:START + SIZE + 60]).decode('latin-1')
    print(text)


if __name__ == '__main__':
    main()
