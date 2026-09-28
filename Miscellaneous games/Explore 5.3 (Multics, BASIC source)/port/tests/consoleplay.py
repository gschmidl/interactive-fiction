"""Explore at a real console, the way run.bat is played.

    python tests/consoleplay.py

Each "window" is a Windows pseudo console (ConPTY): explore.exe sees a real
console, reads real key events and writes ANSI, and nothing appears on the
desktop.  A pipe cannot show any of these:

  1. The "? " prompt is on the screen before a key is pressed, typed
     commands are echoed, and the sorcerer's magic word is not.
  2. Ctrl+Z Enter at the prompt is an empty line, not the end of the game
     (only a piped script or a file ends; a console has no end), and the
     game asks again.
  3. Ctrl+C ends the game in the default mode ("quit"), and is ignored
     after "stm ^quit" (the game's quit_off) - although Windows cuts the
     console read short, as it does for Ctrl+Z.

Every window gets a scratch --var folder, thrown away afterwards.
"""
import ctypes
import ctypes.wintypes as W
import os
import re
import shutil
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'explore.exe')

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
k32.SetConsoleCtrlHandler.argtypes = [ctypes.c_void_p, W.BOOL]

COLS, ROWS = 100, 30


def squeezed(text):
    """The console redraws as it likes -- a phrase can arrive in pieces with
    cursor movements in between -- so text is compared without white space."""
    t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', text)
    t = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', ' ', t)
    return re.sub(r'\s+', '', t)


CSI = re.compile(r'\x1b\[([0-9;?]*)[ -/]*([@-~])')


class Screen:
    """Just enough of a VT100 to follow what the pseudo console draws."""

    def __init__(self):
        self.cells = [[' '] * COLS for _ in range(ROWS)]
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
                self.newline()
            elif ch == '\b':
                self.c = max(self.c - 1, 0)
            elif ch >= ' ':
                if self.c >= COLS:
                    self.c = 0
                    self.newline()
                self.cells[self.r][self.c] = ch
                self.c += 1
            i += 1

    def newline(self):
        if self.r == ROWS - 1:
            self.cells = self.cells[1:] + [[' '] * COLS]
        else:
            self.r += 1

    def csi(self, args, fin):
        if args.startswith('?'):
            return
        p = [int(x) if x else 0 for x in args.split(';')] if args else []

        def arg(k, default):
            return p[k] if len(p) > k and p[k] else default
        if fin in 'Hf':
            self.r = min(arg(0, 1) - 1, ROWS - 1)
            self.c = min(arg(1, 1) - 1, COLS - 1)
        elif fin == 'K':
            mode = p[0] if p else 0
            row = self.cells[self.r]
            if mode == 0:
                row[self.c:] = [' '] * (COLS - self.c)
            elif mode == 1:
                row[:self.c + 1] = [' '] * (self.c + 1)
            else:
                self.cells[self.r] = [' '] * COLS
        elif fin == 'X':
            k = min(arg(0, 1), COLS - self.c)
            self.cells[self.r][self.c:self.c + k] = [' '] * k
        elif fin == 'C':
            self.c = min(self.c + arg(0, 1), COLS - 1)
        elif fin == 'D':
            self.c = max(self.c - arg(0, 1), 0)
        elif fin == 'A':
            self.r = max(self.r - arg(0, 1), 0)
        elif fin == 'B':
            self.r = min(self.r + arg(0, 1), ROWS - 1)
        elif fin == 'G':
            self.c = min(arg(0, 1) - 1, COLS - 1)
        elif fin == 'd':
            self.r = min(arg(0, 1) - 1, ROWS - 1)
        elif fin == 'J':
            mode = p[0] if p else 0
            if mode in (2, 3):
                self.cells = [[' '] * COLS for _ in range(ROWS)]
            elif mode == 0:
                self.cells[self.r][self.c:] = [' '] * (COLS - self.c)
                for k in range(self.r + 1, ROWS):
                    self.cells[k] = [' '] * COLS

    def row(self, k):
        return ''.join(self.cells[k]).rstrip()

    def text(self):
        return '\n'.join(self.row(k) for k in range(ROWS))


class Window:
    """A console window nobody can see, running explore.exe."""

    def __init__(self, name, *args):
        self.name = name
        self.var = tempfile.mkdtemp(prefix='explore-console-')
        in_r, in_w, out_r, out_w = W.HANDLE(), W.HANDLE(), W.HANDLE(), W.HANDLE()
        k32.CreatePipe(ctypes.byref(in_r), ctypes.byref(in_w), None, 0)
        k32.CreatePipe(ctypes.byref(out_r), ctypes.byref(out_w), None, 0)
        self.hpc = W.HANDLE()
        hr = k32.CreatePseudoConsole(COORD(COLS, ROWS), in_r, out_w, 0, ctypes.byref(self.hpc))
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
        argv = [EXE, '--var', self.var] + list(args)
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a if ' ' in a else a for a in argv))
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, ROOT,
                                  ctypes.byref(si), ctypes.byref(self.pi)):
            raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
        self.raw = ''
        self.mark = 0
        self.screen = Screen()
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

    def wait_for(self, what, timeout=60):
        """Wait until `what` is drawn after the last thing found."""
        end = time.time() + timeout
        while True:
            gone = self.exited()
            with self.lock:
                t = squeezed(self.raw)
            k = t.find(squeezed(what), self.mark)
            if k >= 0:
                self.mark = k + len(squeezed(what))
                return
            if gone or time.time() > end:
                break
            time.sleep(0.05)
        raise AssertionError('%s: no %r; the window shows:\n%s'
                             % (self.name, what, self.screen.text()))

    def settle(self):
        """Wait until nothing more is drawn for half a second."""
        while True:
            with self.lock:
                before = len(self.raw)
            time.sleep(0.5)
            with self.lock:
                if len(self.raw) == before:
                    return

    def cursor_row(self):
        with self.lock:
            return self.screen.row(self.screen.r), self.screen.c

    def exited(self):
        return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=30):
        if k32.WaitForSingleObject(self.pi.hProcess, int(timeout * 1000)) != 0:
            raise AssertionError('%s did not end; the window shows:\n%s'
                                 % (self.name, self.screen.text()))
        code = W.DWORD()
        k32.GetExitCodeProcess(self.pi.hProcess, ctypes.byref(code))
        return code.value

    def close(self):
        # once only: a second ClosePseudoConsole kills this script outright
        if not self.exited():
            k32.TerminateProcess(self.pi.hProcess, 1)
        if self.hpc:
            k32.ClosePseudoConsole(self.hpc)
            self.hpc = None
        shutil.rmtree(self.var, ignore_errors=True)


failures = []


def check(name, ok, detail=''):
    print('%-58s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failures.append(name)
        if detail:
            print('    ' + detail.strip().replace('\n', '\n    '))


def prompt_waiting(w):
    """the cursor sits right after "? " on its row"""
    w.settle()
    row, col = w.cursor_row()
    return row.endswith('?') and col == len(row) + 1, '%r, cursor at %d' % (row, col)


def play():
    w = Window('play')
    try:
        w.wait_for('Welcome to "Explore". Type "help" for information.')
        w.wait_for('To the south is a small wooden shack on the hillside.')
        ok, where = prompt_waiting(w)
        check('the "? " prompt shows before any key is pressed', ok, where + '\n' + w.screen.text())

        w.type('in')
        w.settle()
        row, _ = w.cursor_row()
        check('a typed command is echoed', row == '? in', repr(row))
        w.type('\r')
        w.wait_for('a small trapdoor in the floor.')

        w.type('sorcerer\r')
        w.wait_for('Magic word, please:')
        w.settle()
        w.type('hello')
        w.settle()
        row, _ = w.cursor_row()
        check('the magic word is not echoed', 'hello' not in w.screen.text(),
              repr(row) + '\n' + w.screen.text())
        w.type('\r')
        w.wait_for('Request?')
        w.type('quit\r')
        ok, where = prompt_waiting(w)
        check('... and the console echoes again afterwards', ok, where)
        w.type('look')
        w.settle()
        row, _ = w.cursor_row()
        check('... (typed text shows)', row == '? look', repr(row))
        w.type('\r')
        w.wait_for('a small trapdoor in the floor.')

        w.type('quit\r')
        w.wait_for('Do you really want to quit now (yes or no)?')
        w.type('yes\r')
        w.wait_for('You scored')
        code = w.wait_exit()
        check('quit ends the game with exit code 0', code == 0, 'exit code %d' % code)
    finally:
        w.close()


def end_of_input():
    w = Window('ctrl-z')
    try:
        w.wait_for('small wooden shack on the hillside.')
        w.settle()
        w.type('\x1a\r')
        ok, where = prompt_waiting(w)
        check('Ctrl+Z Enter at the prompt: the game asks again', ok and not w.exited(), where)
        w.type('score\r')
        w.wait_for('Your current score is  0  points.')
        w.type('quit\r')
        w.wait_for('(yes or no)?')
        w.type('yes\r')
        code = w.wait_exit()
        check('... and plays on', code == 0, 'exit code %d' % code)
    finally:
        w.close()


def ctrl_c():
    # processes started from some shells inherit "ignore Ctrl+C"; windows
    # started from here must not
    k32.SetConsoleCtrlHandler(None, False)
    w = Window('ctrl-c')
    try:
        w.wait_for('small wooden shack on the hillside.')
        w.settle()
        w.type('\x03')
        try:
            code = w.wait_exit(10)
            check('Ctrl+C ends the game (mode "quit", the default)', code != 0,
                  'exit code %d' % code)
        except AssertionError as e:
            check('Ctrl+C ends the game (mode "quit", the default)', False, str(e))
    finally:
        w.close()

    w = Window('^quit')
    try:
        w.wait_for('small wooden shack on the hillside.')
        w.settle()
        w.type('stm ^quit\r')
        w.settle()
        w.type('\x03')
        time.sleep(2)
        alive = not w.exited()
        w.type('modes\r')
        w.wait_for('^quit,full,^multip')
        check('Ctrl+C is ignored after "stm ^quit"', alive)
        w.type('quit\r')
        w.wait_for('(yes or no)?')
        w.type('yes\r')
        w.wait_for('You scored')
        code = w.wait_exit()
        check('... and quit still ends it with exit code 0', code == 0, 'exit code %d' % code)
    finally:
        w.close()


if __name__ == '__main__':
    play()
    end_of_input()
    ctrl_c()
    print('%d failed' % len(failures) if failures else 'all passed')
    sys.exit(1 if failures else 0)
