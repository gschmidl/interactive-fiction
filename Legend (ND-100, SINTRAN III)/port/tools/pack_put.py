"""Put LEGEND, its tools and its files onto the reference machine's pack.

usage: python tools\\pack_put.py [--data-only]

The pack is _ND100_work\\rc-mordor\\SMD0.IMG (SINTRAN III VSX/500 L under
RetroCore, see NOTES.md); RetroCore must be stopped.  User DNF gets:
  LEGEND-LU:ZYMB        the source, as recovered (LUNDIN-4)
  BASLIBR-H00:BRF       the ND BASIC runtime library (HUMBUG, ND-disk-00437)
  LIBRARY-MJ:BRF        the club library (NILSSON-3, ND-disk-00305)
  the game's files      as in data\\, what the author's 20-IN-LEGEND mode file
                        installs, with the players in v10.0's layout
and user SCRATCH gets SIGNATURES-EAO:DATA.  User LEGENDORIG gets a second copy
of the files a game changes, for tools\reffuzz.py to put back.  With --data-only only the game's
files are written again (a fresh start for recording reference games).

The empty files are deleted rather than written: ndfs-py cannot store a file of
0 bytes (it keeps one NUL), so tools\pack_empty.cmd creates them on SINTRAN,
with CREATE-FILE as the mode file did.
"""
import os
import sys

sys.path.insert(0, r'D:/tools/IFBackup/_ND100_work/ext/norskdata-ndfs/ndfs-py/src')
from ndfs import NdfsFileSystem  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
ORIG = os.path.join(PORT, '..', 'src_original')
DATA = os.path.join(PORT, 'data')
WORK = r'D:/tools/IFBackup/_ND100_work'
PACK = WORK + '/rc-mordor/SMD0.IMG'
ORIGINALS = (['SPELARE-%d-LU' % n for n in range(1, 10)] + ['SAKKARE-%d-LU' % n for n in range(1, 10)] +
             ['VEMFIL-LU', 'BORT-LU'])


def game_files():
    """data\\ as the port ships it, with ND names"""
    out = {}
    for f in sorted(os.listdir(DATA)):
        stem, ext = os.path.splitext(f)
        if ext in ('.DATA', '.SMPH'):
            owner = 'SCRATCH' if stem == 'SIGNATURES-EAO' else 'DNF'
            out['%s/%s:%s' % (owner, stem, ext[1:])] = open(os.path.join(DATA, f), 'rb').read()
    return out


def main():
    files = game_files()
    if '--data-only' not in sys.argv:
        files['DNF/LEGEND-LU:ZYMB'] = open(os.path.join(ORIG, 'LEGEND-LU.ZYMB'), 'rb').read()
        files['DNF/BASLIBR-H00:BRF'] = open(WORK + '/files/basic/BASLIBR-H00.BRF', 'rb').read()
        files['DNF/LIBRARY-MJ:BRF'] = open(WORK + '/files/basic/LIBRARY-MJ.BRF', 'rb').read()
    fs = NdfsFileSystem(bytearray(open(PACK, 'rb').read()))
    # pristine copies, which tools\reffuzz.py copies back before every game
    fs.add_user('LEGENDORIG', 400)
    for path, data in list(files.items()):
        if path.startswith('DNF/') and data and path.split('/')[1].split(':')[0] in ORIGINALS:
            orig = 'LEGENDORIG/' + path.split('/')[1]
            fs.write_file(orig, data)
            files[orig] = data
    for path, data in list(files.items()):
        if not data:
            if fs.file_exists(path):
                fs.delete_file(path)
            print('  %-32s deleted (empty: pack_empty.cmd)' % path)
            del files[path]
        else:
            fs.write_file(path, data)
    open(PACK, 'wb').write(fs.to_buffer())
    check = NdfsFileSystem(open(PACK, 'rb').read(), read_only=True)
    bad = 0
    for path, data in files.items():
        ok = check.read_file(path) == data
        bad += not ok
        print('  %-32s %6d %s' % (path, len(data), 'ok' if ok else 'MISMATCH'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
