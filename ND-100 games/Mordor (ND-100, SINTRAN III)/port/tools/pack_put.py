"""Put the ND Pascal J compiler, candidate libraries and MORDOR onto the
rc-mordor RetroCore pack (user DNF).  RetroCore must be stopped."""
import os
import sys

# set ND100_WORK to the working tree this repository does not publish
WORK = os.environ.get('ND100_WORK',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '_ND100_work'))
sys.path.insert(0, os.path.join(WORK, 'ext', 'norskdata-ndfs', 'ndfs-py', 'src'))
from ndfs import NdfsFileSystem  # noqa: E402

PACK = os.path.join(WORK, 'rc-mordor', 'SMD0.IMG')
F = os.path.join(WORK, 'files', 'mordor')

files = {
    'PASCAL-COD-J:BRF': open(F + '/DNF/PASCAL-COD-J.BRF', 'rb').read(),
    'PASCAL-LIB-J:BRF': open(F + '/DNF/PASCAL-LIB-J.BRF', 'rb').read(),
    'PASCAL-2LIB-J:BRF': open(F + '/DNF/PASCAL-2LIB-J.BRF', 'rb').read(),
    'PASCAL-ERR-J:SYMB': open(F + '/DNF/PASCAL-ERR-J.SYMB', 'rb').read(),
    'EXTRA-PAS-LIB:BRF': open(F + '/NILSSON-3/EXTRA-PAS-LIB.BRF', 'rb').read(),
    'EXTRA-PASLIB-PBL:BRF': open(F + '/HUMBUG/EXTRA-PASLIB-PBL.BRF', 'rb').read(),
    'LIBRARY-MJ:BRF': open(F + '/NILSSON-3/LIBRARY-MJ.BRF', 'rb').read(),
    'FTNLIBR-2091F:BRF': open(F + '/ND-10023K/FTNLIBR-2091F.BRF', 'rb').read(),
    'MORDOR-MJ:SYMB': open(F + '/LUNDIN-6-recovered.SYMB.pages', 'rb').read()[:72393],
    # blank instructions: a line count of 0, even parity like the real rules files
    'MORDOR-RULES-MJ:DATA': b'\x30\x8d\x0a',
}

img = bytearray(open(PACK, 'rb').read())
fs = NdfsFileSystem(img)
print('directory', fs.get_directory_name())
if fs.add_user('DNF', 3000):
    print('created user DNF')
for name, data in files.items():
    fs.write_file('DNF/' + name, data)
    print('wrote', name, len(data))
open(PACK, 'wb').write(fs.to_buffer())

check = NdfsFileSystem(open(PACK, 'rb').read(), read_only=True)
for e in check.get_object_entries():
    if e.user_name == 'DNF':
        path = 'DNF/%s:%s' % (e.object_name, e.type)
        data = check.read_file(path)
        want = files.get('%s:%s' % (e.object_name, e.type))
        print('  %-26s %7d %s' % (path, len(data), 'OK' if data == want else 'MISMATCH'))
