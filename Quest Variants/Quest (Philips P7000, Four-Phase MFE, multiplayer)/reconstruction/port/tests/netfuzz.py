"""Random play from several terminals at once: the window that runs the game (terminal 0,
scripted) and players joined over TCP, all typing random commands.

    python netfuzz.py [GAMES [PLAYERS [SECONDS [SEED]]]] [--real-clock]
                                        (defaults: 3 games, 3 players, 60 seconds, seed 1)

In each game every player signs on, types a random command (fuzz.py's mix) every half to
two seconds for SECONDS, then QUITs with YES - the players over TCP first, the window that
runs the game last, and QUEST ends with its last player.  A game fails if the machine stops,
the run does not end by itself with exit 0, a player's screen goes quiet for 60 seconds (a
hang), or a player is not told "You have left QUEST".  QUEST refuses keys with a beep while
it still holds a player's last line, so the beeps are counted and QUIT is typed again until
QUEST asks.

With --real-clock the game runs with -u on the real clock, at the capped speed run.bat has.
The port's log (QUEST_NETLOG) gives how far the machine ever fell behind the clock, and the
port's CPU time while everyone types is measured: the test of whether PLAYERS busy players
are more than the cap allows.
"""
import ctypes
import ctypes.wintypes as W
import os
import random
import subprocess
import sys
import threading
import time

from common import EXE, Player, squeeze
from fuzz import sentence

NET_PORT = 17415
NAMES = ['Anna', 'Bo', 'Carl', 'Dora', 'Emil', 'Frida']


class Host:
    """the port with terminal 0 scripted"""

    def __init__(self, players, real):
        clock = ['-u'] if real else ['--fixed-clock']
        extra = os.environ.get('NETFUZZ_HOST_ARGS', '').split()     # debugging, e.g. --trace=FILE
        self.p = subprocess.Popen([EXE] + clock + extra +
                                  ['--players=%d' % players, '--port=%d' % NET_PORT],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, text=True,
                                  env=dict(os.environ, QUEST_NETLOG='1'))
        self.out = self.log = ''
        self.lock = threading.Lock()
        threading.Thread(target=self._pump, args=('out', self.p.stdout), daemon=True).start()
        threading.Thread(target=self._pump, args=('log', self.p.stderr), daemon=True).start()

    def _pump(self, what, f):
        for line in f:
            with self.lock:
                setattr(self, what, getattr(self, what) + line)

    def say(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()

    def seen(self, what):
        with self.lock:
            return what in self.out

    def cpu(self):
        """the CPU seconds the port has used"""
        k32 = ctypes.WinDLL('kernel32')
        t = [W.FILETIME() for _ in range(4)]
        k32.GetProcessTimes(W.HANDLE(self.p._handle), *[ctypes.byref(x) for x in t])
        return sum((x.dwHighDateTime << 32 | x.dwLowDateTime) for x in t[2:]) / 1e7


def watch(player, stop, quiet):
    """note when PLAYER's screen last changed"""
    last = len(player.raw)
    while not stop.is_set():
        time.sleep(0.5)
        if len(player.raw) != last:
            last = len(player.raw)
            quiet[player.name] = time.time()


QUESTION = 'Do you really want to quit the game?'


def rows_with(p, text):
    """the rows of P's screen that show TEXT"""
    want = squeeze(text)
    with p.lock:
        return sum(want in squeeze(p.screen.row(r)) for r in range(p.screen.rows))


def quit_game(p):
    """QUIT and YES.  QUEST refuses keys with a beep while it still holds the
    player's last line (an action takes time), so QUIT is typed again until the
    question comes up anew."""
    for attempt in range(8):
        before = rows_with(p, QUESTION)
        p.type('\x05QUIT\r')                    # MODE blanks the line first
        end = time.time() + 15
        while time.time() < end and not p.closed and rows_with(p, QUESTION) <= before:
            time.sleep(0.1)
        if rows_with(p, QUESTION) > before:
            p.type('YES\r')
            return
        if p.closed:
            return
    raise AssertionError('%s: %r never appeared after 8 QUITs; the screen:\n%s'
                         % (p.name, QUESTION, p.screen.text()))


def one_game(g, n, seconds, rng, real):
    host = Host(n, real)
    players = [Player(NAMES[i], NET_PORT, timeout=60) for i in range(n - 1)]
    why = None
    stop = threading.Event()
    quiet = {}
    busy = None
    try:
        for p in players:
            p.wait_for('What is your first name?', timeout=90)
            p.type(p.name + '\r')
            p.wait_for('Are you male?')
            p.type(rng.choice(['YES', 'NO']) + '\r')
        host.say('Heino')
        host.say('yes')
        for p in players:
            p.wait_for('You are ', timeout=90)          # the game has begun
            quiet[p.name] = time.time()
            threading.Thread(target=watch, args=(p, stop, quiet), daemon=True).start()
        end = time.time() + seconds
        busy = [time.time(), host.cpu()]
        nxt = {p.name: time.time() + rng.uniform(0.5, 2) for p in players}
        nxt['host'] = time.time() + rng.uniform(0.5, 2)
        while time.time() < end:
            now = time.time()
            for p in players:
                if now >= nxt[p.name] and not p.closed:
                    p.type(sentence(rng) + '\r', gap=0.01)
                    nxt[p.name] = now + rng.uniform(0.5, 2)
                if now - quiet[p.name] > 60:
                    raise AssertionError('%s: the screen has been quiet for 60 s' % p.name)
            if now >= nxt['host']:
                host.say(sentence(rng))
                nxt['host'] = now + rng.uniform(0.5, 2)
            if host.p.poll() is not None:
                raise AssertionError('the port ended early, exit %d' % host.p.returncode)
            time.sleep(0.05)
        busy = (host.cpu() - busy[1]) / (time.time() - busy[0])
        for p in players:
            if p.closed:
                continue                        # died, or said QUIT and YES by chance
            quit_game(p)
        for p in players:
            p.wait_closed(timeout=60)
            p.wait_for('You have left QUEST', timeout=5)
        host.say('quit')
        host.say('yes')
        deadline = time.time() + 120
        while host.p.poll() is None and time.time() < deadline:
            time.sleep(0.2)
        if host.p.poll() is None:
            raise AssertionError('the port did not end after everyone left')
        if host.p.returncode != 0:
            raise AssertionError('the port ended with exit %d' % host.p.returncode)
        if 'machine stopped' in host.log:
            raise AssertionError('the machine stopped:\n' + host.log[-500:])
    except AssertionError as e:
        why = str(e)
    finally:
        stop.set()
        for p in players:
            p.close()
        if host.p.poll() is None:
            host.p.kill()
    lag = [l for l in host.log.split('\n') if 'the clock:' in l]
    info = ['%d beeps at the joined terminals' % sum(p.screen.bells for p in players),
            '%d lines typed again at terminal 0' % host.log.count('QUEST refused the line')]
    if isinstance(busy, float):
        info.append('%.0f%% of a CPU core while everyone typed' % (100 * busy))
    if lag:
        info.append(lag[-1].split('] ', 1)[-1])
    return why, '; '.join(info), host


def main():
    real = '--real-clock' in sys.argv[1:]
    argv = [a for a in sys.argv[1:] if a != '--real-clock']
    games = int(argv[0]) if len(argv) > 0 else 3
    n = int(argv[1]) if len(argv) > 1 else 3
    seconds = int(argv[2]) if len(argv) > 2 else 60
    seed = int(argv[3]) if len(argv) > 3 else 1
    rng = random.Random(seed)
    failed = 0
    for g in range(games):
        t0 = time.time()
        why, lag, host = one_game(g, n, seconds, rng, real)
        took = time.time() - t0
        if why:
            failed += 1
            print('FAIL  game %d (%d players): %s' % (g, n, why))
            print('      the port\'s log:\n' + host.log[-1500:])
        else:
            print('ok    game %d: %d players, %.0f s%s' % (g, n, took, '; ' + lag if lag else ''))
    print('%d of %d games failed' % (failed, games) if failed else 'all %d games passed' % games)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
