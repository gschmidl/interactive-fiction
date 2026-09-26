"""Build LEGEND on the reference machine, from the sources to the :PROG files.

usage: python tools\\rebuild.py        (RetroCore must not be running)

1. writes to the pack (user DNF): the recovered source LEGEND-LU:ZYMB, the
   port's source basic\\LEGEND-LU.ZYMB as LEGENDF:ZYMB, the club library as it
   survives (LIBRARY-MJ:BRF) and fixed (build\\LIBRARY-MJ.BRF as LIBRARY-MJF),
   the rebuilt ND BASIC runtime (build\\BASLIBR-H00.BRF), and the game's files;
2. starts RetroCore, creates the empty files (tools\\pack_empty.cmd) and runs
   tools\\build.cmd: both sources through the BASIC compiler of January 1985
   (DEFAULT-INTEGER, TABLE-SIZES 1500,35000, as the author's 2LEGEND mode file),
   then NRL: SIZE 2700, the program, the club library, BASLIBR-H00;
3. stops RetroCore and takes the results off the pack: build\\ (the port's
   program, its BRF and listing; also data\\LEGEND-LU.PROG) and
   build\\original\\ (the recovered source with the club library as it is);
4. writes LEGEND-REF:PROG and LEGENDF-REF:PROG to the pack, the programs with
   the three words changed that make the reference machine repeatable
   (NOTES.md, "The reference machine").
"""
import hashlib
import os
import shutil
import socket
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
WORK = os.environ.get('ND100_WORK',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '_ND100_work'))
RC = WORK + '/rc-mordor'
PACK = RC + '/SMD0.IMG'
sys.path.insert(0, WORK + '/ext/norskdata-ndfs/ndfs-py/src')
from ndfs import NdfsFileSystem  # noqa: E402


def text(d):
    return bytes(x & 0x7f for x in d).split(b'\x17')[0].decode('latin-1').replace('\r\n', '\n')


def ref_of(prog):
    """the reference machine's copy: SLOW does not wait for OSIZE = 64 (RetroCore
    never says 64), RANDOM does not stir in the uptime (MON 11)"""
    w = list(struct.unpack('>%dH' % (len(prog) // 2), prog))

    def find(seq):
        hits = [i for i in range(0x100, len(w) - len(seq)) if w[i:i + len(seq)] == seq]
        assert len(hits) == 1, ([oct(x) for x in seq], len(hits))
        return hits[0]
    i = find([0o153143, 0o153067, 0o153064, 0o172700, 0o131775])
    w[i + 4] = 0o124001
    j = find([0o153011, 0o146115, 0o171756, 0o006600])
    w[j] = w[j + 1] = 0o170400
    return struct.pack('>%dH' % len(w), *w)


def session(cmdfile, log, timeout):
    with open(log, 'w', encoding='latin-1') as f:
        subprocess.run([sys.executable, WORK + '/tools/ndsession.py', '--user', 'DNF', '--password', '',
                        '--timeout', str(timeout), cmdfile], stdout=f, stderr=subprocess.STDOUT,
                       timeout=timeout + 120)
    return open(log, encoding='latin-1').read().replace('\r', '')


def main():
    files = {
        'DNF/LEGEND-LU:ZYMB': open(os.path.join(PORT, '..', 'src_original', 'LEGEND-LU.ZYMB'), 'rb').read(),
        'DNF/LEGENDF:ZYMB': open(os.path.join(PORT, 'basic', 'LEGEND-LU.ZYMB'), 'rb').read(),
        'DNF/LIBRARY-MJ:BRF': open(WORK + '/files/basic/LIBRARY-MJ.BRF', 'rb').read(),
        'DNF/LIBRARY-MJF:BRF': open(os.path.join(PORT, 'build', 'LIBRARY-MJ.BRF'), 'rb').read(),
        'DNF/BASLIBR-H00:BRF': open(os.path.join(PORT, 'build', 'BASLIBR-H00.BRF'), 'rb').read(),
    }
    fs = NdfsFileSystem(bytearray(open(PACK, 'rb').read()))
    for p, d in files.items():
        fs.write_file(p, d)
    open(PACK, 'wb').write(fs.to_buffer())
    subprocess.run([sys.executable, os.path.join(HERE, 'pack_put.py'), '--data-only'], check=True,
                   stdout=subprocess.DEVNULL)

    rc = subprocess.Popen([RC + '/RetroCore.exe'], cwd=RC, stdin=subprocess.DEVNULL,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        for _ in range(120):
            try:
                socket.create_connection(('127.0.0.1', 9000), 2).close()
                break
            except OSError:
                time.sleep(2)
        time.sleep(80)
        os.makedirs(os.path.join(PORT, 'build', 'original'), exist_ok=True)
        session(os.path.join(HERE, 'pack_empty.cmd'), os.path.join(PORT, 'build', 'empty.log'), 200)
        out = session(os.path.join(HERE, 'build.cmd'), os.path.join(PORT, 'build', 'build.log'), 800)
        print('\n'.join(l for l in out.split('\n') if 'COMPILED' in l or 'FREE:' in l or ' U ' in l
                        or 'ERROR' in l.upper()))
    finally:
        rc.kill()
        rc.wait()
        time.sleep(2)

    fs = NdfsFileSystem(bytearray(open(PACK, 'rb').read()))
    for name, dest in (('LEGENDF', os.path.join(PORT, 'build')), ('LEGEND-LU', os.path.join(PORT, 'build', 'original'))):
        prog = fs.read_file('DNF/%s:PROG' % name)
        open(os.path.join(dest, 'LEGEND-LU.PROG'), 'wb').write(prog)
        open(os.path.join(dest, 'LEGEND-LU.BRF'), 'wb').write(fs.read_file('DNF/%s:BRF' % name))
        open(os.path.join(dest, 'LEGEND-LU.LIST.txt'), 'w', encoding='latin-1', newline='\n').write(
            text(fs.read_file('DNF/%s:LIST' % name)))
        ref = ref_of(prog)
        fs.write_file('DNF/%s-REF:PROG' % ('LEGENDF' if name == 'LEGENDF' else 'LEGEND'), ref)
        print('%-9s PROG sha1 %s, REF sha1 %s' % (name, hashlib.sha1(prog).hexdigest()[:12],
                                                  hashlib.sha1(ref).hexdigest()[:12]))
    open(PACK, 'wb').write(fs.to_buffer())
    shutil.copyfile(os.path.join(PORT, 'build', 'LEGEND-LU.PROG'), os.path.join(PORT, 'data', 'LEGEND-LU.PROG'))
    for log in ('empty.log',):
        os.remove(os.path.join(PORT, 'build', log))


if __name__ == '__main__':
    main()
