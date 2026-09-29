"""Multiplayer check, over real network connections.

    python tests/netplay.py            (from the port directory, or anywhere)

A world with nobody playing at its window (-server -E, on a spare port, in
a throwaway save directory) and two players who join it at the same moment:

  1. ALICE and BOB both connect.  ALICE types her initials first and so holds
     the logon; BOB, typing his straight after, must wait -- with the note on
     his bottom line -- until ALICE has made her character.  (The server
     would otherwise hand both the same player slot; see quest.h.)
  2. Both play at the same time: a step north and back, each.
  3. ALICE leaves with ESC and is told her character is saved; BOB's line
     simply drops, the way a terminal hung up.
  4. The world stops by itself once both have gone, and USER_DATA_FILE holds
     both characters.
  5. A second world on the same save directory lets ALICE back in without
     making a new character -- and this time she joins with the program's
     own terminal (--join, as quest.bat joins) fed from a file, asking for
     --god: her panel shows strength 1024.
  6. A world started --server --god: BOB, joining with a plain telnet
     connection, is a god as well.

Each player's screen is kept by decoding what the world sends (only changed
cells are ever sent, so looking for text in the stream is not enough).
data/ is only read.  Exit status 0 when everything held.
"""
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'aosvs32.exe')
DATA = os.path.join(ROOT, 'data')
PR = os.path.join(DATA, 'QUEST.PR')
PROMPT_ROW = 14                      # where QUEST reads commands: "-> "


def free_port():
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    p = s.getsockname()[1]
    s.close()
    return p


def stream_text(raw):
    t = re.sub(rb'\xff[\xfb-\xfe].', b'', raw)
    t = re.sub(rb'\x1b\][^\x07]*\x07', b'', t)
    t = re.sub(rb'\x1b\[[0-9;]*[A-Za-z]', b' ', t)
    return t.decode('latin-1')


class Player:
    def __init__(self, name, port, host='127.0.0.1'):
        self.name = name
        self.s = socket.create_connection((host, port), timeout=30)
        self.s.setblocking(False)
        self.raw = b''
        self.closed = False
        self.rows = [[' '] * 80 for _ in range(24)]
        self.r = self.c = 0
        self.pending = b''
        self.last_rx = time.time()

    # -- the terminal ------------------------------------------------------
    def feed(self, data):
        data = self.pending + data
        self.pending = b''
        i = 0
        n = len(data)
        while i < n:
            b = data[i]
            if b == 0xFF:                                  # telnet
                if i + 2 >= n:
                    self.pending = data[i:]
                    return
                i += 3
                continue
            if b == 0x1B:
                m = re.compile(rb'\x1b\[([0-9;]*)([A-Za-z])').match(data, i)
                o = re.compile(rb'\x1b\][^\x07]*\x07').match(data, i)
                if m:
                    args, fin = m.group(1).decode(), m.group(2).decode()
                    if fin == 'H':
                        p = [int(x) if x else 1 for x in args.split(';')] if args else [1, 1]
                        self.r = min(max(p[0] - 1, 0), 23)
                        self.c = min(max((p[1] if len(p) > 1 else 1) - 1, 0), 79)
                    elif fin == 'J' and args == '2':
                        self.rows = [[' '] * 80 for _ in range(24)]
                    i = m.end()
                    continue
                if o:
                    i = o.end()
                    continue
                self.pending = data[i:]                    # incomplete sequence
                return
            if b == 13:
                self.c = 0
            elif b == 10:
                self.r = min(self.r + 1, 23)
            elif 32 <= b < 127:
                self.rows[self.r][self.c] = chr(b)
                if self.c < 79:
                    self.c += 1
            i += 1

    def pump(self):
        try:
            while True:
                b = self.s.recv(65536)
                if not b:
                    self.closed = True
                    return
                self.raw += b
                self.feed(b)
                self.last_rx = time.time()
        except (BlockingIOError, InterruptedError):
            pass
        except OSError:
            self.closed = True

    def row(self, k):
        return ''.join(self.rows[k]).rstrip()

    def screen(self):
        return '\n'.join(self.row(k) for k in range(24))

    def send(self, keys):
        self.sent_at = len(self.raw)
        self.s.sendall(keys.encode('latin-1'))

    # -- waiting -------------------------------------------------------------
    def settle(self, what, timeout=60, changed=True, fail=True):
        """Wait until the world has answered (if `changed`) and gone quiet,
        and what(self) holds.  Without `fail`, say whether it did."""
        end = time.time() + timeout
        start = getattr(self, 'sent_at', 0) if changed else 0
        while time.time() < end:
            self.pump()
            quiet = time.time() - self.last_rx > 0.25
            if quiet and len(self.raw) > start and what(self):
                return True
            if self.closed:
                break
            time.sleep(0.02)
        if not fail and len(self.raw) > start and not self.closed:
            return False
        raise AssertionError('%s: timed out; cursor %d,%d; screen:\n%s'
                             % (self.name, self.r, self.c, self.screen()))

    def shows(self, text):
        return text in self.screen()

    def at_prompt(self):
        """The cursor back after "-> " on the command line.  What else is on
        that line varies: a move the world refused leaves its words there."""
        return (self.r, self.c) == (PROMPT_ROW, 3) and self.row(PROMPT_ROW).startswith('->')

    def wants_key(self):
        row = self.row(self.r)
        return 'Hit any character' in row or 'Hit space bar' in row

    def wait_closed(self, timeout=30):
        end = time.time() + timeout
        while time.time() < end and not self.closed:
            self.pump()
            time.sleep(0.02)
        if not self.closed:
            raise AssertionError('%s: the world did not hang up' % self.name)


def start_world(save, port, *more):
    # -Z: QUEST seeds its random numbers from the clock; frozen, where the
    # players land and what meets them there is the same every run.
    return subprocess.Popen([EXE, '--port', str(port), '--server', '--exit-when-empty',
                             '--no-title', '-Z', '1000000000'] + list(more) +
                            ['-d', DATA, '-s', save, PR],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def wait_listening(port, world):
    """The world prints its banner once it is listening.  (Knocking on the
    port instead would count as a player joining.)"""
    while True:
        line = world.stdout.readline()
        if not line:
            raise AssertionError('the world stopped at once')
        if b'Quest world on port' in line:
            return


def answer(p):
    """After a command: the world has answered it and is waiting again.
    A message that wants a key first (an arrow from a tower, say -- the
    world is not scripted) gets one.  Anything else the world answers with
    and then sits quiet on counts too: this is about both players being
    served at once, not about what the answer was."""
    for _ in range(10):
        p.settle(lambda q: q.at_prompt() or q.wants_key(), timeout=8, fail=False)
        if p.wants_key():
            p.send(' ')
            continue
        return
    raise AssertionError('%s kept being asked for keys:\n%s' % (p.name, p.screen()))


def create(p, name, password):
    """From 'Player name' to the command prompt, for a new character."""
    p.send(name + '\r')
    p.settle(lambda q: q.shows('Password ?'))
    p.send(password + '\r')
    p.settle(lambda q: q.shows('create this character'))
    p.send('Y')
    p.settle(lambda q: q.shows('Hit any character'))
    p.send(' ')
    p.settle(lambda q: q.shows("'W' = wizard") or q.at_prompt())
    if not p.at_prompt():
        p.send('F')
    answer(p)


def leave(p):
    """ESC at the command prompt leaves; an ESC that lands on a message
    first just clears it."""
    for _ in range(6):
        p.send('\x1b')
        end = time.time() + 5
        while time.time() < end and not p.closed:
            p.pump()
            if 'Your character is saved' in stream_text(p.raw):
                return
            time.sleep(0.05)
    raise AssertionError('%s could not leave:\n%s' % (p.name, p.screen()))


def characters(save):
    """The names in USER_DATA_FILE: 980-byte records in the NADGUG QUEST,
    eight 266-byte slots in the 1984 one."""
    size = 980 if os.path.getsize(os.path.join(DATA, 'USER_DATA_FILE')) % 980 == 0 else 266
    d = open(os.path.join(save, 'USER_DATA_FILE'), 'rb').read()
    names = []
    for r in range(len(d) // size):
        rec = d[r * size:(r + 1) * size]
        n = struct.unpack('>H', rec[0:2])[0]
        names.append(rec[2:2 + n].decode('latin-1'))
    return names


def main():
    save = tempfile.mkdtemp(prefix='questnet')
    port = free_port()
    ok = True
    world = None
    try:
        # -- 1..4: two players at once --------------------------------------
        world = start_world(save, port)
        wait_listening(port, world)
        a = Player('ALICE', port)
        b = Player('BOB', port)
        for p in (a, b):
            p.settle(lambda q: q.shows('What are your initials'), changed=False)
        a.send('AL\r')
        a.settle(lambda q: q.shows('Player name ?'))
        b.send('BO\r')
        b.settle(lambda q: q.shows('Another player is logging on'))
        time.sleep(1.0)
        b.pump()
        assert not b.shows('Player name ?'), 'BOB got past the logon while ALICE held it'
        print('ok    one logon at a time')
        create(a, 'ALICE', 'RABBIT')
        b.settle(lambda q: q.shows('Player name ?') and not q.shows('Another player'),
                 changed=False)
        create(b, 'BOB', 'BUILDER')
        print('ok    both characters made')
        for key in 'NS':
            for p in (a, b):
                p.send(key)
            for p in (a, b):
                answer(p)
        print('ok    both playing at once')
        leave(a)
        a.wait_closed()
        b.s.close()
        try:
            world.wait(timeout=60)
        except subprocess.TimeoutExpired:
            raise AssertionError('the world did not stop after the last player left')
        log = world.stdout.read().decode('latin-1')
        names = characters(save)
        assert 'ALICE' in names and 'BOB' in names, 'saved characters: %r' % names
        assert log.count('has joined') == 2 and log.count('has left') == 2, log
        print('ok    both saved, world stopped:', names)

        # -- 5: back again, through the join terminal ------------------------
        port = free_port()
        world = start_world(save, port)
        wait_listening(port, world)
        keys = os.path.join(save, 'keys.txt')
        with open(keys, 'wb') as f:
            f.write(b'AL\rALICE\rRABBIT\r ')
        with open(keys, 'rb') as kin:
            join = subprocess.run([EXE, '--join', '127.0.0.1:%d' % port, '--god'], stdin=kin,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  timeout=120)
        shown = stream_text(join.stdout)
        assert 'does not exist' not in shown, 'ALICE was not known: ' + shown[-400:]
        assert 'Strength' in shown, 'no map for ALICE: ' + shown[-400:]
        assert 'Strength 1024' in shown, 'ALICE asked for --god and is not one: ' + shown[-400:]
        assert 'Your character is saved' in shown, shown[-400:]
        world.wait(timeout=60)
        print('ok    ALICE came back through --join, and --god made her strong')

        # -- 6: a world where everyone is a god ---------------------------------
        port = free_port()
        world = start_world(save, port, '--god')
        wait_listening(port, world)
        b = Player('BOB', port)
        b.settle(lambda q: q.shows('What are your initials'), changed=False)
        b.send('BO\r')
        b.settle(lambda q: q.shows('Player name ?'))
        b.send('BOB\r')
        b.settle(lambda q: q.shows('Password ?'))
        b.send('BUILDER\r')
        answer(b)
        assert b.shows('Strength 1024') and b.shows('Wealth 20000'), b.screen()
        leave(b)
        b.wait_closed()
        world.wait(timeout=60)
        print('ok    in a --server --god world BOB is strong too')
    except AssertionError as e:
        print('FAIL ', e)
        ok = False
    finally:
        if world is not None and world.poll() is None:
            world.kill()
        shutil.rmtree(save, ignore_errors=True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
