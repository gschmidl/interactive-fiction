"""Dump every data file on the NERO0366 disk as text, record by record.

usage: dumpall.py adventure.dsk outdir
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hpldisk  # noqa: E402

disk = open(sys.argv[1], 'rb').read()
outdir = sys.argv[2]
os.makedirs(outdir, exist_ok=True)

with open(os.path.join(outdir, 'DIRECTORY.txt'), 'w', encoding='utf-8', newline='\n') as f:
    f.write('name    start  sectors  length  type\n')
    for e in hpldisk.directory(disk):
        f.write(f"{e['name']:7} {e['start']:5}  {e['sectors']:7}  {e['length']:6}  {e['type']:#06x}\n")

for e in hpldisk.directory(disk):
    if e['type'] not in (0x0400, 0x2400):
        continue
    buf = hpldisk.file_bytes(disk, e)
    with open(os.path.join(outdir, e['name'] + '.records.txt'), 'w', encoding='utf-8', newline='\n') as f:
        for rec, items in hpldisk.records(buf):
            if items:
                f.write(f'{rec:4}: ' + ' | '.join(repr(i) if isinstance(i, str) else str(i) for i in items) + '\n')
    print(e['name'], 'ok')
