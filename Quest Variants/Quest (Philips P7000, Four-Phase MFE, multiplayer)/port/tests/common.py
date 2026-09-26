"""Shared by the tests: where the port is, scripted runs, a VT screen model, and a player's
terminal over TCP.

A scripted run is the program in transcript mode with the clock counted in instructions
(--fixed-clock): terminal 0 followed as a log, its lines read from stdin, and the same input
always gives the same output."""
import os
import re
import socket
import subprocess
import tempfile
import threading
import time

TESTS = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(TESTS)
EXE = os.path.join(PORT, 'quest.exe')
REFERENCE = os.path.join(TESTS, 'reference')


def run(lines, args=(), env=None, timeout=300, fixed_clock=True):
    """the transcript of a scripted game: LINES typed at terminal 0; returns (exit, out, err).
    The lines come from a file: from a pipe the port would take a line only once it has
    arrived, and the game would go on meanwhile (it runs in real time).  FIXED_CLOCK=False
    runs on the real clock (and at the real machine's speed), which is not repeatable."""
    e = dict(os.environ)
    e.pop('QUEST_TIME', None)
    if env:
        e.update(env)
    with tempfile.TemporaryFile('w+') as f:
        f.write(''.join(l + '\n' for l in lines))
        f.seek(0)
        p = subprocess.run([EXE] + (['--fixed-clock'] if fixed_clock else ['-u']) + list(args),
                           stdin=f, capture_output=True, text=True, timeout=timeout, env=e)
    return p.returncode, p.stdout.replace('\r\n', '\n'), p.stderr.replace('\r\n', '\n')


def reference(name):
    with open(os.path.join(REFERENCE, name), encoding='latin-1') as f:
        return f.read()


class Screen:
    """what a terminal shows, from the ANSI text the program writes (the part of VT100 that
    the program and Windows' pseudo console use)"""

    def __init__(self, cols=81, rows=25):
        self.cols, self.rows = cols, rows
        self.cells = [[' '] * cols for _ in range(rows)]
        self.r = self.c = 0
        self.pending = ''
        self.bells = 0

    def text(self):
        return '\n'.join(''.join(row).rstrip() for row in self.cells)

    def row(self, r):
        return ''.join(self.cells[r]).rstrip()

    def _scroll(self):
        self.cells.pop(0)
        self.cells.append([' '] * self.cols)

    def feed(self, data):
        s = self.pending + data
        self.pending = ''
        i = 0
        while i < len(s):
            ch = s[i]
            if ch == '\x1b':
                m = re.match(r'\x1b\[([?>]?)([0-9;]*)([ -/]*)([@-~])', s[i:])
                if not m:
                    m2 = re.match(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', s[i:])
                    if m2:
                        i += m2.end()
                        continue
                    if len(s) - i < 32 and ('\x1b[' == s[i:i + 2] or len(s) - i < 2 or s[i + 1] == ']'):
                        self.pending = s[i:]
                        return
                    i += 2
                    continue
                self._csi(m.group(1), m.group(2), m.group(4))
                i += m.end()
                continue
            if ch == '\r':
                self.c = 0
            elif ch == '\n':
                self.r += 1
                if self.r >= self.rows:
                    self._scroll()
                    self.r = self.rows - 1
            elif ch == '\b':
                self.c = max(0, self.c - 1)
            elif ch == '\x07':
                self.bells += 1
            elif ch >= ' ':
                if self.c >= self.cols:
                    self.c = 0
                    self.r += 1
                    if self.r >= self.rows:
                        self._scroll()
                        self.r = self.rows - 1
                self.cells[self.r][self.c] = ch
                self.c += 1
            i += 1

    def _csi(self, private, params, final):
        ps = [int(p) if p else 0 for p in params.split(';')] if params else []

        def p(k, default=1):
            return ps[k] if len(ps) > k and ps[k] else default
        if private:
            return
        if final in 'Hf':
            self.r, self.c = min(p(0) - 1, self.rows - 1), min(p(1) - 1, self.cols - 1)
        elif final == 'A':
            self.r = max(0, self.r - p(0))
        elif final == 'B':
            self.r = min(self.rows - 1, self.r + p(0))
        elif final == 'C':
            self.c = min(self.cols - 1, self.c + p(0))
        elif final == 'D':
            self.c = max(0, self.c - p(0))
        elif final == 'G':
            self.c = min(p(0) - 1, self.cols - 1)
        elif final == 'd':
            self.r = min(p(0) - 1, self.rows - 1)
        elif final == 'J':
            mode = ps[0] if ps else 0
            if mode == 2 or mode == 3:
                self.cells = [[' '] * self.cols for _ in range(self.rows)]
            elif mode == 0:
                self.cells[self.r][self.c:] = [' '] * (self.cols - self.c)
                for r in range(self.r + 1, self.rows):
                    self.cells[r] = [' '] * self.cols
            elif mode == 1:
                for r in range(self.r):
                    self.cells[r] = [' '] * self.cols
                self.cells[self.r][:self.c + 1] = [' '] * (self.c + 1)
        elif final == 'K':
            mode = ps[0] if ps else 0
            if mode == 0:
                self.cells[self.r][self.c:] = [' '] * (self.cols - self.c)
            elif mode == 1:
                self.cells[self.r][:self.c + 1] = [' '] * (self.c + 1)
            else:
                self.cells[self.r] = [' '] * self.cols
        elif final == 'X':
            n = p(0)
            for k in range(self.c, min(self.cols, self.c + n)):
                self.cells[self.r][k] = ' '
        elif final == 'P':
            n = p(0)
            row = self.cells[self.r]
            del row[self.c:self.c + n]
            row.extend([' '] * (self.cols - len(row)))
        elif final == '@':
            n = p(0)
            row = self.cells[self.r]
            for _ in range(n):
                row.insert(self.c, ' ')
            del row[self.cols:]


def squeeze(s):
    return re.sub(r'\s+', ' ', s)


class Player:
    """a player's terminal over TCP, the way quest --join is one: keys go out as the
    program's key codes, the screen comes back as ANSI text"""

    def __init__(self, name, port, host='127.0.0.1', timeout=20):
        self.name = name
        end = time.time() + timeout
        while True:
            try:
                self.sock = socket.create_connection((host, port), timeout=5)
                break
            except OSError:
                if time.time() > end:
                    raise
                time.sleep(0.2)
        self.screen = Screen()
        self.raw = ''
        self.closed = False
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        while True:
            try:
                b = self.sock.recv(65536)
            except OSError:
                b = b''
            if not b:
                self.closed = True
                return
            with self.lock:
                t = b.decode('latin-1')
                self.raw += t
                self.screen.feed(t)

    def send(self, data):
        if isinstance(data, str):
            data = data.encode('latin-1')
        self.sock.sendall(data)

    def type(self, text, gap=0.02):
        for ch in text:
            self.send(ch)
            time.sleep(gap)

    def wait_for(self, what, timeout=60):
        end = time.time() + timeout
        want = squeeze(what)
        while True:
            with self.lock:
                t = squeeze(self.screen.text())
            if want in t:
                return
            if self.closed or time.time() > end:
                break
            time.sleep(0.05)
        with self.lock:
            shown = self.screen.text()
        raise AssertionError('%s: %r never appeared; the screen:\n%s' % (self.name, what, shown))

    def wait_closed(self, timeout=30):
        end = time.time() + timeout
        while not self.closed:
            if time.time() > end:
                raise AssertionError('%s: the connection stayed open' % self.name)
            time.sleep(0.05)

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass
