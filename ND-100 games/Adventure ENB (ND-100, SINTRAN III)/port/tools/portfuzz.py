"""Play random games of ADVENTURE-ENB on the port and look for trouble.

usage: python tools\portfuzz.py [SEEDS] [COMMANDS] [--keys DIR] [--first N] [--fixed-clock]

Each seed plays a game with tools\play.py's random player for up to COMMANDS
commands (default 8 seeds x 200) with --raw --no-hold, in an empty scratch
folder, under a time limit.  Printed as trouble: a run that hangs or does not
end, an unimplemented monitor call or instruction, a BASIC run-time error, or
a prompt the player does not know.  With --keys the typing of each game is
kept (DIR\portNN.keys).  Each seed has a world of its own (--uptime
SEED*1009), or with --fixed-clock the reference machine's (-Z).
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
from play import Player  # noqa: E402

PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'advenb.exe')
DATA = os.path.join(PORT, 'data')
PROG = os.path.join(DATA, 'ADVENTURE-ENB.PROG')
TROUBLE = re.compile(r'unimplemented[^\]]*|ILLEGAL INSTRUCTION[^\r]*|PRIVILEGED[^\r]*|'
                     r'UNIMPLEMENTED INSTRUCTION[^\r]*|\*\* Error +\d+|USER BREAK AT[^\r]*')


class Run:
    def __init__(self, args):
        self.p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
        self.chunks, self.size = [], 0
        self.lock = threading.Lock()
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        while True:
            b = self.p.stdout.read1(65536)
            if not b:
                break
            s = bytes(x & 0x7f for x in b).decode('latin-1').replace('\0', '')
            with self.lock:
                self.chunks.append(s)
                self.size += len(s)

    def text(self):
        with self.lock:
            if len(self.chunks) > 1:
                self.chunks = [''.join(self.chunks)]
            return self.chunks[0] if self.chunks else ''

    def tail(self, n=2000):
        with self.lock:
            out, got = [], 0
            for c in reversed(self.chunks):
                out.append(c)
                got += len(c)
                if got >= n:
                    break
            return ''.join(reversed(out))[-n:].replace('\r', '')

    def settle(self, since, quiet=0.15, limit=20):
        """until there is output after `since` and none for `quiet` seconds, or the run ends"""
        end = time.time() + limit
        last, stamp = -1, time.time()
        while time.time() < end:
            size = self.size
            if size != last:
                last, stamp = size, time.time()
            elif size > since and time.time() - stamp >= quiet:
                return True
            if self.p.poll() is not None:
                time.sleep(0.1)
                return True
            time.sleep(0.005)
        return False

    def send(self, keys):
        self.p.stdin.write(keys.encode('latin-1'))
        self.p.stdin.flush()


def play(seed, commands, work, keep=None, first=None, fixed=False):
    args = [EXE, '--data', work, '--raw', '--no-hold', '--prog', PROG]
    args += ['-Z', '1'] if fixed else ['--uptime', str(seed * 1009)]
    player = Player(seed, commands)
    r = Run(args)
    typed, problem = [], None
    try:
        since = 0
        while problem is None:
            if not r.settle(since):
                problem = 'HANG after %r' % typed[-3:]
                break
            if r.p.poll() is not None:
                break
            choice = player.answer(r.tail())
            if choice is None:
                time.sleep(1.0)
                choice = player.answer(r.tail())
                if choice is None:
                    problem = 'unknown prompt %r' % r.tail(300)
                    break
            since = r.size
            r.send(choice[0])
            typed.append(choice[0])
            if first and len(typed) >= first:
                break
        try:
            r.p.stdin.close()
        except OSError:
            pass
        try:
            r.p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            problem = (problem or '') + ' did not exit'
    finally:
        if r.p.poll() is None:
            r.p.kill()
    out = r.text()
    if keep:
        open(os.path.join(keep, 'port%02d.keys' % seed), 'wb').write(''.join(typed).encode('latin-1'))
    return problem, TROUBLE.findall(out), out, player.done


def main():
    argv = sys.argv[1:]
    keep = first = None
    fixed = '--fixed-clock' in argv
    if fixed:
        argv.remove('--fixed-clock')
    if '--keys' in argv:
        i = argv.index('--keys')
        keep = argv[i + 1]
        del argv[i:i + 2]
        os.makedirs(keep, exist_ok=True)
    if '--first' in argv:
        i = argv.index('--first')
        first = int(argv[i + 1])
        del argv[i:i + 2]
    seeds = int(argv[0]) if argv else 8
    commands = int(argv[1]) if len(argv) > 1 else 200
    bad = 0
    for seed in range(1, seeds + 1):
        work = tempfile.mkdtemp(prefix='advenbfuzz')
        try:
            problem, found, out, done = play(seed, commands, work, keep, first, fixed)
        finally:
            shutil.rmtree(work, ignore_errors=True)
        if problem or found:
            bad += 1
            print('seed %d: TROUBLE %s %r after %d commands' % (seed, problem or '', found[:3], done))
            k = out.find(found[0]) if found else len(out)
            print('   ...%r' % out[max(0, k - 600):k + 100])
        else:
            print('seed %d: ok, %d commands' % (seed, done))
    print('%d of %d games had trouble' % (bad, seeds))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
