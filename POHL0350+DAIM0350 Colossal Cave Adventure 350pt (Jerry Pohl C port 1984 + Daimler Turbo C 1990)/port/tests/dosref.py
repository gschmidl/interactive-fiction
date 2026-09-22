#!/usr/bin/env python3
"""Run the original DOS program under DOSBox, as the reference.

    python tests\\dosref.py pohl|daimler INPUT [ARGS...]    transcript to stdout

The DOS executables are the authors' own builds, each run with the text
files it was built for, in a scratch folder mounted as C:, with DOS's own
redirection (ADVENT < IN.TXT > OUT.TXT):

  pohl     ..\\reference\\pohl-dos: ADVENT.EXE of 10 June 1984 and its text
           files, from the IF Archive's games/pc/adv.arc.
  daimler  ..\\reference\\daimler-dos: ADVENT.EXE of 6 May 1990 from
           games/pc/advtc2.zip.  That zip's ADVENT4.TXT is of 23 June:
           Daimler added the line "-Conversion to TurboC 2.0 by Daimler" to
           the welcome (message 65) without rebuilding, so the program he
           shipped prints every message from 66 on 39 bytes too early.  The
           May file is that one without the line - with it removed, all 201
           offsets compiled into the EXE match - and that is what runs here.

DOSBox 0.74-3 (SDL 1.2, whose dummy video driver draws nothing), cycles=max,
no sound.  The emulator is killed if the program does not end by itself:
the originals never do at end of input, so every input must end with QUIT
and y.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.dirname(os.path.dirname(HERE))
DOSBOX = r'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
REF = {'pohl': os.path.join(FOLDER, 'reference', 'pohl-dos'),
       'daimler': os.path.join(FOLDER, 'reference', 'daimler-dos')}
CREDIT = b'\t-Conversion to TurboC 2.0 by Daimler\r\n'
CONF = '''[sdl]
fullscreen=false
output=surface
[cpu]
core=auto
cycles=max
[mixer]
nosound=true
[speaker]
pcspeaker=false
[autoexec]
mount c .
c:
%s
exit
'''


def texts(program):
    """the four text files the DOS program was built for"""
    out = {}
    for n in (1, 2, 3, 4):
        with open(os.path.join(REF[program], 'ADVENT%d.TXT' % n), 'rb') as f:
            out['ADVENT%d.TXT' % n] = f.read()
    if program == 'daimler':
        june = out['ADVENT4.TXT']
        out['ADVENT4.TXT'] = june.replace(CREDIT, b'', 1)
        assert len(out['ADVENT4.TXT']) == len(june) - len(CREDIT)
    return out


def run(program, lines, args=(), timeout=120, files=None):
    """(transcript with LF line ends, {file: bytes} left in the folder) of
    the DOS program given LINES; FILES are put in the folder first (a saved
    game, say)"""
    tmp = tempfile.mkdtemp(prefix='advent-dos-')
    try:
        shutil.copy(os.path.join(REF[program], 'ADVENT.EXE'), tmp)
        for n, data in list(texts(program).items()) + list((files or {}).items()):
            with open(os.path.join(tmp, n), 'wb') as f:
                f.write(data)
        with open(os.path.join(tmp, 'IN.TXT'), 'wb') as f:
            f.write(''.join(l + '\r\n' for l in lines).encode('latin-1'))
        cmd = 'ADVENT %s< IN.TXT > OUT.TXT' % ''.join(a + ' ' for a in args)
        with open(os.path.join(tmp, 'RUN.CONF'), 'w', newline='\n') as f:
            f.write(CONF % cmd)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        p = subprocess.Popen([DOSBOX, '-noconsole', '-conf', 'RUN.CONF'], cwd=tmp, env=env,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            p.wait(timeout=timeout)
            hung = False
        except subprocess.TimeoutExpired:
            p.kill()
            p.wait()
            hung = True
        out = b''
        if os.path.exists(os.path.join(tmp, 'OUT.TXT')):
            with open(os.path.join(tmp, 'OUT.TXT'), 'rb') as f:
                out = f.read()
        keep = ('ADVENT.EXE', 'IN.TXT', 'OUT.TXT', 'RUN.CONF', 'STDOUT.TXT', 'STDERR.TXT',
                'ADVENT1.TXT', 'ADVENT2.TXT', 'ADVENT3.TXT', 'ADVENT4.TXT')
        after = {}
        for n in os.listdir(tmp):
            if n.upper() not in keep:
                with open(os.path.join(tmp, n), 'rb') as f:
                    after[n] = f.read()
        text = out.decode('latin-1').replace('\r\n', '\n')
        if hung:
            text += '\n[the DOS program did not end within %d s]\n' % timeout
        return text, after
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in REF:
        print(__doc__)
        return 2
    with open(sys.argv[2], encoding='latin-1') as f:
        lines = f.read().splitlines()
    text, _ = run(sys.argv[1], lines, sys.argv[3:])
    sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
