"""The reference machine: SINTRAN III VSX/500 L under RetroCore (NOTES.md).

Shared by rebuild.py and the recording tools.  RetroCore must be stopped
whenever the pack is written (pack_write), and nothing else may be using it.
"""
import os
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
WORK = os.environ.get('ND100_WORK', os.path.join(PORT, '..', '..', '_ND100_work'))
RC = os.path.join(WORK, 'rc-mordor')
PACK = os.path.join(RC, 'SMD0.IMG')
sys.path.insert(0, os.path.join(WORK, 'ext', 'norskdata-ndfs', 'ndfs-py', 'src'))
from ndfs import NdfsFileSystem  # noqa: E402


def text(d):
    """a SINTRAN text file as a str: parity off, up to the 027 that ends it"""
    return bytes(x & 0x7f for x in d).split(b'\x17')[0].decode('latin-1').replace('\r\n', '\n')


def pack_read(path):
    return NdfsFileSystem(open(PACK, 'rb').read(), read_only=True).read_file(path)


def pack_write(files, delete=()):
    """files: {'USER/NAME:TYPE': bytes}; checks what it wrote"""
    fs = NdfsFileSystem(bytearray(open(PACK, 'rb').read()))
    for p in delete:
        if fs.file_exists(p):
            fs.delete_file(p)
    for p, d in files.items():
        fs.write_file(p, d)
    open(PACK, 'wb').write(fs.to_buffer())
    check = NdfsFileSystem(open(PACK, 'rb').read(), read_only=True)
    for p, d in files.items():
        if check.read_file(p) != d:
            raise IOError('%s did not read back' % p)


class Machine:
    """with Machine(): ... boots RetroCore, waits until SINTRAN answers, and
    stops it again afterwards"""

    def __enter__(self):
        self.p = subprocess.Popen([os.path.join(RC, 'RetroCore.exe')], cwd=RC, stdin=subprocess.DEVNULL,
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(120):
            try:
                socket.create_connection(('127.0.0.1', 9000), 2).close()
                break
            except OSError:
                time.sleep(2)
        time.sleep(80)                       # SINTRAN's start-up
        return self

    def __exit__(self, *exc):
        self.p.kill()
        self.p.wait()
        time.sleep(2)


def session(cmdfile, log, timeout=900, user='DNF'):
    """run a command file (see _ND100_work\\tools\\ndsession.py) and return its log"""
    with open(log, 'w', encoding='latin-1') as f:
        subprocess.run([sys.executable, os.path.join(WORK, 'tools', 'ndsession.py'), '--user', user,
                        '--password', '', '--timeout', str(timeout), cmdfile],
                       stdout=f, stderr=subprocess.STDOUT, timeout=timeout + 120)
    return open(log, encoding='latin-1').read().replace('\r', '')
