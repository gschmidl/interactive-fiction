"""Play random games of MORDOR on the port and look for trouble.

usage: python tools\\portfuzz.py [SEEDS] [COMMANDS] [--keys DIR] [--original]

Each seed plays a game with tools\\play.py's random player, answering what
the game asks, for up to COMMANDS commands (default 8 seeds x 600).  The port
runs on a scratch copy of data\\ with --raw --unlimited and a fixed clock,
under a time limit, and every other seed builds a new map.  Printed as
trouble: a run that hangs, an unimplemented monitor call or instruction, a
Pascal run-time error, a user break, or a prompt the player does not know.
With --keys the typing of each game is kept (DIR\\portNN.keys).  --original
plays build\\original\\MORDOR-MJ.PROG, the program without the port's fixes.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from play import PROMPT, Player  # noqa: E402

PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'mordor.exe')
DATA = os.path.join(PORT, 'data')
TROUBLE = re.compile(r'unimplemented[^\]]*|ILLEGAL INSTRUCTION[^\r]*|PRIVILEGED[^\r]*|'
                     r'UNIMPLEMENTED INSTRUCTION[^\r]*|Failed to open PASCAL-ERR|'
                     r'\r\n[A-Z][A-Z /().,<=0-9]+\r\n AT ADDRESS +\d+|USER BREAK AT[^\r]*')


class Run:
    def __init__(self, args):
        self.p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
        self.chunks = []          # decoded output, parity stripped, as it arrives
        self.size = 0
        self.err = bytearray()
        self.lock = threading.Lock()
        threading.Thread(target=self._read_out, daemon=True).start()
        threading.Thread(target=self._read_err, daemon=True).start()

    def _read_out(self):
        while True:
            b = self.p.stdout.read1(65536)
            if not b:
                break
            s = bytes(x & 0x7f for x in b).decode('latin-1')
            with self.lock:
                self.chunks.append(s)
                self.size += len(s)

    def _read_err(self):
        while True:
            b = self.p.stderr.read1(65536)
            if not b:
                break
            self.err.extend(b)

    def text(self):
        with self.lock:
            if len(self.chunks) > 1:
                self.chunks = [''.join(self.chunks)]
            return self.chunks[0] if self.chunks else ''

    def tail(self, n=2000):
        """the last n characters, without joining everything"""
        with self.lock:
            out, got = [], 0
            for c in reversed(self.chunks):
                out.append(c)
                got += len(c)
                if got >= n:
                    break
            return ''.join(reversed(out))[-n:]

    def wait_prompt(self, since, timeout=60):
        """until the output after `since` ends with a prompt and the port is quiet, or it exits"""
        end = time.time() + timeout
        last, stamp = -1, time.time()
        while time.time() < end:
            size = self.size
            if size != last:
                last, stamp = size, time.time()
            elif time.time() - stamp > 0.02 and (size > since and PROMPT.search(self.tail(40)) or
                                                 self.p.poll() is not None):
                return True
            time.sleep(0.002)
        return False

    def send(self, keys):
        self.p.stdin.write(keys.encode('latin-1'))
        self.p.stdin.flush()


ORIGINAL = os.path.join(PORT, 'build', 'original', 'MORDOR-MJ.PROG')


def play(seed, commands, work, keep_keys, original=False):
    args = [EXE, '--data', work, '--raw', '--unlimited', '-Z', str(1789668000 + seed)]
    if original:
        args += ['--prog', ORIGINAL]
    if seed % 2:
        args.append('--new-map')
    player = Player(seed, commands, original)
    r = Run(args)
    typed = []
    problem = None
    try:
        if not r.wait_prompt(0):
            problem = 'no first prompt'
        else:
            opening = player.opening()
            k = opening[0][0]                      # the answer about instructions
            r.send(k)
            typed.append(k)
            end = time.time() + 30
            while time.time() < end:               # page through them, if any
                tail = r.tail(200)
                if re.search(r'Finished\.\x1b\(', tail):
                    break
                if tail.endswith('finished reading: ') and r.wait_prompt(r.size - 1, timeout=5):
                    r.send('\r')
                    typed.append('\r')
                    t0 = r.size
                    while r.size == t0 and time.time() < end:
                        time.sleep(0.001)
                time.sleep(0.002)
            for k, why, rx in opening[1:]:
                r.send(k)
                typed.append(k)
            while problem is None:
                if not r.wait_prompt(r.size - 1 if r.size else 0, timeout=15):
                    problem = 'HANG after %r' % (typed[-3:],)
                    break
                if r.p.poll() is not None:
                    break
                choice = player.answer(r.tail())
                if choice is None:
                    problem = 'unknown prompt %r' % r.tail(200)
                    break
                r.send(choice[0])
                typed.append(choice[0])
                # wait for the port to take it: the echo or new output
                t0 = r.size
                end = time.time() + 30
                while r.size == t0 and r.p.poll() is None and time.time() < end:
                    time.sleep(0.001)
        try:
            r.p.stdin.close()
        except OSError:
            pass
        try:
            r.p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            problem = (problem or '') + ' did not exit; busy after the end of input'
    finally:
        if r.p.poll() is None:
            r.p.kill()
            problem = problem or 'did not exit'
    out = r.text() + bytes(r.err).decode('latin-1')
    found = TROUBLE.findall(out)
    if keep_keys:
        open(os.path.join(keep_keys, 'port%02d.keys' % seed), 'wb').write(''.join(typed).encode('latin-1'))
    return problem, found, out, player.done


def main():
    argv = sys.argv[1:]
    keep = None
    original = '--original' in argv
    if original:
        argv.remove('--original')
    if '--keys' in argv:
        i = argv.index('--keys')
        keep = argv[i + 1]
        del argv[i:i + 2]
        os.makedirs(keep, exist_ok=True)
    seeds = int(argv[0]) if argv else 8
    commands = int(argv[1]) if len(argv) > 1 else 600
    work = tempfile.mkdtemp(prefix='mordorfuzz')
    bad = 0
    try:
        for f in ('MORDOR-MJ.PROG', 'MORDOR-RULES-MJ.DATA', 'PASCAL-ERR.SYMB'):
            shutil.copy(os.path.join(DATA, f), work)
        for seed in range(1, seeds + 1):
            problem, found, out, done = play(seed, commands, work, keep, original)
            verdict = re.search(r'overall performance was ([a-z ]+)', out)
            how = ('score: ' + verdict.group(1)) if verdict else ('Chicken!' if 'Chicken!' in out else 'no end')
            if problem or found:
                bad += 1
                print('seed %d: TROUBLE %s %r after %d commands' % (seed, problem or '', found[:2], done))
                i = out.find(found[0]) if found else len(out)
                print('   ...%r' % out[max(0, i - 400):i + 100])
            else:
                print('seed %d: ok, %d commands, %s' % (seed, done, how))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('%d of %d games had trouble' % (bad, seeds))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
