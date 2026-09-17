"""Multiplayer check at real consoles -- the way quest.bat is played.

    python tests/consoleplay.py

Each "window" is a Windows pseudo console (ConPTY): the program sees a real
console, reads real key events and writes ANSI, and nothing appears on the
desktop.  Both windows run the same command line, as two quest.bat would:

  1. The first window starts the world and plays: CAROL makes a character.
  2. The second window finds the world already running and joins it as a
     terminal: DORIS makes a character.
  3. CAROL leaves with ESC.  Her window says her character is saved and keeps
     the world running for DORIS.
  4. DORIS leaves with ESC; her window closes, and then the first window
     stops the world by itself and says it is saved.
  5. quest --god at a window that is hosting: EVE plays with strength 1024.
     Then Ctrl-C there, in the middle of the game: the world saves her and
     stops.

Every save directory is thrown away afterwards; data/ is only read.
"""
import ctypes
import ctypes.wintypes as W
import os
import re
import shutil
import socket
import struct
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'aosvs32.exe')

k32 = ctypes.WinDLL('kernel32', use_last_error=True)


class COORD(ctypes.Structure):
    _fields_ = [('X', W.SHORT), ('Y', W.SHORT)]


class STARTUPINFOW(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('lpReserved', W.LPWSTR), ('lpDesktop', W.LPWSTR),
                ('lpTitle', W.LPWSTR), ('dwX', W.DWORD), ('dwY', W.DWORD),
                ('dwXSize', W.DWORD), ('dwYSize', W.DWORD), ('dwXCountChars', W.DWORD),
                ('dwYCountChars', W.DWORD), ('dwFillAttribute', W.DWORD),
                ('dwFlags', W.DWORD), ('wShowWindow', W.WORD), ('cbReserved2', W.WORD),
                ('lpReserved2', ctypes.c_void_p), ('hStdInput', W.HANDLE),
                ('hStdOutput', W.HANDLE), ('hStdError', W.HANDLE)]


class STARTUPINFOEXW(ctypes.Structure):
    _fields_ = [('StartupInfo', STARTUPINFOW), ('lpAttributeList', ctypes.c_void_p)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [('hProcess', W.HANDLE), ('hThread', W.HANDLE),
                ('dwProcessId', W.DWORD), ('dwThreadId', W.DWORD)]


k32.CreatePipe.argtypes = [ctypes.POINTER(W.HANDLE), ctypes.POINTER(W.HANDLE), ctypes.c_void_p, W.DWORD]
k32.CreatePseudoConsole.argtypes = [COORD, W.HANDLE, W.HANDLE, W.DWORD, ctypes.POINTER(W.HANDLE)]
k32.CreatePseudoConsole.restype = ctypes.c_long
k32.ClosePseudoConsole.argtypes = [W.HANDLE]
k32.InitializeProcThreadAttributeList.argtypes = [ctypes.c_void_p, W.DWORD, W.DWORD, ctypes.POINTER(ctypes.c_size_t)]
k32.UpdateProcThreadAttribute.argtypes = [ctypes.c_void_p, W.DWORD, ctypes.c_size_t, ctypes.c_void_p,
                                          ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p]
k32.CreateProcessW.argtypes = [W.LPCWSTR, W.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, W.BOOL, W.DWORD,
                               ctypes.c_void_p, W.LPCWSTR, ctypes.c_void_p, ctypes.POINTER(PROCESS_INFORMATION)]
k32.ReadFile.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), ctypes.c_void_p]
k32.WriteFile.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), ctypes.c_void_p]
k32.WaitForSingleObject.argtypes = [W.HANDLE, W.DWORD]
k32.GetExitCodeProcess.argtypes = [W.HANDLE, ctypes.POINTER(W.DWORD)]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]


def text_of(raw):
    t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', raw)
    t = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', ' ', t)
    return t


def squeezed(text):
    """The console redraws as it likes -- a phrase can arrive in pieces with
    cursor movements in between -- so text is compared without white space."""
    return re.sub(r'\s+', '', text)


CSI = re.compile(r'\x1b\[([0-9;?]*)[ -/]*([@-~])')


class Screen:
    """Just enough of a VT100 to follow what the pseudo console draws."""

    def __init__(self, cols, rows):
        self.cols, self.rows = cols, rows
        self.cells = [[' '] * cols for _ in range(rows)]
        self.r = self.c = 0
        self.held = ''

    def feed(self, s):
        s = self.held + s
        self.held = ''
        i, n = 0, len(s)
        while i < n:
            ch = s[i]
            if ch == '\x1b':
                if i + 1 >= n:
                    self.held = s[i:]
                    return
                if s[i + 1] == '[':
                    m = CSI.match(s, i)
                    if not m:
                        self.held = s[i:]
                        return
                    self.csi(m.group(1), m.group(2))
                    i = m.end()
                    continue
                if s[i + 1] == ']':
                    ends = [e for e in (s.find('\x07', i), s.find('\x1b\\', i)) if e >= 0]
                    if not ends:
                        self.held = s[i:]
                        return
                    i = min(ends) + (1 if s[min(ends)] == '\x07' else 2)
                    continue
                i += 2
                continue
            if ch == '\r':
                self.c = 0
            elif ch == '\n':
                self.r = min(self.r + 1, self.rows - 1)
            elif ch == '\b':
                self.c = max(self.c - 1, 0)
            elif ch >= ' ':
                if self.c >= self.cols:
                    self.c = 0
                    self.r = min(self.r + 1, self.rows - 1)
                self.cells[self.r][self.c] = ch
                self.c += 1
            i += 1

    def csi(self, args, fin):
        if args.startswith('?'):
            return
        p = [int(x) if x else 0 for x in args.split(';')] if args else []

        def arg(k, default):
            return p[k] if len(p) > k and p[k] else default
        if fin in 'Hf':
            self.r = min(arg(0, 1) - 1, self.rows - 1)
            self.c = min(arg(1, 1) - 1, self.cols - 1)
        elif fin == 'K':
            mode = p[0] if p else 0
            row = self.cells[self.r]
            if mode == 0:
                row[self.c:] = [' '] * (self.cols - self.c)
            elif mode == 1:
                row[:self.c + 1] = [' '] * (self.c + 1)
            else:
                self.cells[self.r] = [' '] * self.cols
        elif fin == 'X':
            k = min(arg(0, 1), self.cols - self.c)
            self.cells[self.r][self.c:self.c + k] = [' '] * k
        elif fin == 'C':
            self.c = min(self.c + arg(0, 1), self.cols - 1)
        elif fin == 'D':
            self.c = max(self.c - arg(0, 1), 0)
        elif fin == 'A':
            self.r = max(self.r - arg(0, 1), 0)
        elif fin == 'B':
            self.r = min(self.r + arg(0, 1), self.rows - 1)
        elif fin == 'G':
            self.c = min(arg(0, 1) - 1, self.cols - 1)
        elif fin == 'd':
            self.r = min(arg(0, 1) - 1, self.rows - 1)
        elif fin == 'J':
            mode = p[0] if p else 0
            if mode in (2, 3):
                self.cells = [[' '] * self.cols for _ in range(self.rows)]
            elif mode == 0:
                self.cells[self.r][self.c:] = [' '] * (self.cols - self.c)
                for k in range(self.r + 1, self.rows):
                    self.cells[k] = [' '] * self.cols

    def row(self, k):
        return ''.join(self.cells[k]).rstrip()

    def text(self):
        return '\n'.join(self.row(k) for k in range(self.rows))


class Window:
    """A console window nobody can see, running one command."""

    def __init__(self, name, args):
        self.name = name
        in_r, in_w, out_r, out_w = W.HANDLE(), W.HANDLE(), W.HANDLE(), W.HANDLE()
        k32.CreatePipe(ctypes.byref(in_r), ctypes.byref(in_w), None, 0)
        k32.CreatePipe(ctypes.byref(out_r), ctypes.byref(out_w), None, 0)
        self.hpc = W.HANDLE()
        hr = k32.CreatePseudoConsole(COORD(100, 30), in_r, out_w, 0, ctypes.byref(self.hpc))
        if hr != 0:
            raise OSError('CreatePseudoConsole failed: %x' % (hr & 0xFFFFFFFF))
        k32.CloseHandle(in_r)
        k32.CloseHandle(out_w)
        self.inp, self.out = in_w, out_r
        size = ctypes.c_size_t()
        k32.InitializeProcThreadAttributeList(None, 1, 0, ctypes.byref(size))
        self.attrs = (ctypes.c_byte * size.value)()
        k32.InitializeProcThreadAttributeList(self.attrs, 1, 0, ctypes.byref(size))
        k32.UpdateProcThreadAttribute(self.attrs, 0, 0x00020016, self.hpc,
                                      ctypes.sizeof(W.HANDLE), None, None)
        si = STARTUPINFOEXW()
        si.StartupInfo.cb = ctypes.sizeof(STARTUPINFOEXW)
        # Empty standard handles: otherwise the program inherits this
        # script's redirected ones and never sees the pseudo console.
        si.StartupInfo.dwFlags = 0x00000100             # STARTF_USESTDHANDLES
        si.lpAttributeList = ctypes.addressof(self.attrs)
        self.pi = PROCESS_INFORMATION()
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a if ' ' in a else a for a in args))
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, ROOT,
                                  ctypes.byref(si), ctypes.byref(self.pi)):
            raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
        self.raw = ''
        self.mark = 0
        self.screen = Screen(100, 30)
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = ctypes.create_string_buffer(65536)
        got = W.DWORD()
        while k32.ReadFile(self.out, buf, 65536, ctypes.byref(got), None) and got.value:
            text = buf.raw[:got.value].decode('utf-8', 'replace')
            with self.lock:
                self.raw += text
                self.screen.feed(text)

    def type(self, keys, gap=0.03):
        for ch in keys:
            data = ch.encode('latin-1')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def wait_for(self, *whats, timeout=60):
        """Wait for the first of `whats` to be drawn after the last one found;
        return which it was."""
        end = time.time() + timeout
        while True:
            gone = self.exited()
            with self.lock:
                t = squeezed(text_of(self.raw))
            hits = [(t.find(squeezed(w), self.mark), w) for w in whats]
            hits = [h for h in hits if h[0] >= 0]
            if hits:
                k, w = min(hits)
                self.mark = k + len(squeezed(w))
                return w
            if gone or time.time() > end:
                break
            time.sleep(0.05)
        raise AssertionError('%s: none of %r; the window shows:\n%s'
                             % (self.name, whats, self.screen.text()))

    def wait_screen(self, what, timeout=60):
        """Wait until what(screen) holds and the drawing has stopped."""
        end = time.time() + timeout
        while time.time() < end:
            with self.lock:
                before = len(self.raw)
                holds = what(self.screen)
            if holds:
                time.sleep(0.3)
                with self.lock:
                    if len(self.raw) == before:
                        return
                continue
            if self.exited():
                break
            time.sleep(0.05)
        raise AssertionError('%s: timed out; cursor %d,%d; the window shows:\n%s'
                             % (self.name, self.screen.r, self.screen.c, self.screen.text()))

    def exited(self):
        return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=60):
        if k32.WaitForSingleObject(self.pi.hProcess, int(timeout * 1000)) != 0:
            raise AssertionError('%s did not finish' % self.name)
        code = W.DWORD()
        k32.GetExitCodeProcess(self.pi.hProcess, ctypes.byref(code))
        return code.value

    def close(self):
        if not self.exited():
            k32.TerminateProcess(self.pi.hProcess, 1)
        k32.ClosePseudoConsole(self.hpc)


def free_port():
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    p = s.getsockname()[1]
    s.close()
    return p


def characters(save):
    """The names in USER_DATA_FILE: 980-byte records in the NADGUG QUEST,
    eight 266-byte slots in the 1984 one."""
    size = 980 if os.path.getsize(os.path.join(ROOT, 'data', 'USER_DATA_FILE')) % 980 == 0 else 266
    d = open(os.path.join(save, 'USER_DATA_FILE'), 'rb').read()
    names = []
    for r in range(len(d) // size):
        rec = d[r * size:(r + 1) * size]
        n = struct.unpack('>H', rec[0:2])[0]
        names.append(rec[2:2 + n].decode('latin-1'))
    return names


def quest(save, port, title=False, *more):
    """quest.bat's command line.  With title, the 1984 QUEST's CASTLE is
    typed first, as its quest.bat does."""
    castle = os.path.join(ROOT, 'data', 'CASTLE')
    first = ['--title', 'data\\CASTLE'] if title and os.path.exists(castle) else ['--no-title']
    # -Z as in the other tests: the random numbers follow the clock, and a
    # dragon at the starting square would end the run.
    return [EXE, '--port', str(port), '-Z', '1000000000'] + first + list(more) + \
           ['-d', 'data', '-s', save, 'data\\QUEST.PR']


def at_prompt(s):
    """The cursor after "-> " on QUEST's command line, row 14."""
    return (s.r, s.c) == (14, 3) and s.row(14).startswith('->')


def new_character(w, initials, name, password):
    w.wait_for('initials')
    w.type(initials + '\r')
    w.wait_for('Player name')
    w.type(name + '\r')
    w.wait_for('Password')
    w.type(password + '\r')
    w.wait_for('create this character')
    w.type('Y')
    w.wait_for('Hit any character')
    w.type(' ')
    w.wait_screen(lambda s: "'W' = wizard" in s.text() or at_prompt(s))
    if not at_prompt(w.screen):                         # the NADGUG QUEST asks
        w.type('F')
        w.wait_screen(at_prompt)


def leave(w, what='Your character is saved'):
    for _ in range(6):
        w.type('\x1b')
        try:
            w.wait_for(what, timeout=5)
            return
        except AssertionError:
            pass
    raise AssertionError('%s could not leave' % w.name)


def main():
    ok = True
    saves = []
    windows = []
    try:
        save = tempfile.mkdtemp(prefix='questcon')
        saves.append(save)
        port = free_port()
        host = Window('first window', quest(save, port, title=True))
        windows.append(host)
        if '--title' in quest(save, port, title=True):
            host.wait_for('Welcome to')
            print('ok    the title is typed first')
        new_character(host, 'CA', 'CAROL', 'CAKE')
        print('ok    the first window hosts and plays')
        second = Window('second window', quest(save, port))
        windows.append(second)
        new_character(second, 'DO', 'DORIS', 'DOG')
        print('ok    the second window joined the running world')
        leave(host)
        host.wait_for('in the world')
        time.sleep(1)
        assert not host.exited(), 'the first window stopped the world with DORIS in it'
        print('ok    CAROL left; her window keeps the world up')
        leave(second)
        second.wait_exit(30)
        host.wait_for('The world is saved')
        host.wait_exit(30)
        names = characters(save)
        assert 'CAROL' in names and 'DORIS' in names, names
        print('ok    DORIS left; the world stopped and saved:', names)

        save = tempfile.mkdtemp(prefix='questcon')
        saves.append(save)
        port = free_port()
        w = Window('Ctrl-C window', quest(save, port, False, '--god'))
        windows.append(w)
        new_character(w, 'EV', 'EVE', 'APPLE')
        w.type('N')                                     # a move redraws the panel
        w.wait_for('Strength 1024')
        print('ok    quest --god: EVE plays with strength 1024')
        w.type('\x03')
        w.wait_exit(30)
        names = characters(save)
        assert 'EVE' in names, names
        print('ok    Ctrl-C saved EVE and stopped the world:', names)
    except AssertionError as e:
        print('FAIL ', e)
        ok = False
    finally:
        for w in windows:
            w.close()
        for s in saves:
            shutil.rmtree(s, ignore_errors=True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
