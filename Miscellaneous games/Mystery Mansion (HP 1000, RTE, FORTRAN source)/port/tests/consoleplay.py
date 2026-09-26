"""A game at a real console -- the way run.bat is played.

    python tests/consoleplay.py

The "window" is a Windows pseudo console (ConPTY): the program sees a real
console, reads real key events and writes ANSI, and nothing appears on the
desktop.  What pipes cannot show is checked here, on a model of the screen:

  1. --site at 20:00: the security code is asked for; the game then moves
     the cursor up (HP 264x ESC A) and writes blanks over the line, so the
     code typed is not left on the screen.
  2. A record ending in '_' leaves the cursor after it: the name is typed
     on the line of the question, and so is every command after '>'.
  3. QUIT, YES, the comments, an empty line: the program ends by itself,
     with exit code 0.

It runs in a scratch directory (never the real saves\\).
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
EXE = os.path.join(ROOT, 'mmm.exe')
CWD = ROOT

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
                self.down()
            elif ch == '\b':
                self.c = max(self.c - 1, 0)
            elif ch >= ' ':
                if self.c >= self.cols:
                    self.c = 0
                    self.down()
                self.cells[self.r][self.c] = ch
                self.c += 1
            i += 1

    def down(self):
        # a new line at the bottom scrolls the screen up
        if self.r == self.rows - 1:
            self.cells.pop(0)
            self.cells.append([' '] * self.cols)
        else:
            self.r += 1

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
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, CWD,
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



def main():
    global CWD
    exe = sys.argv[1] if len(sys.argv) > 1 else EXE
    CWD = tempfile.mkdtemp(prefix='mmmcon')
    shutil.copy(exe, os.path.join(CWD, 'mmm.exe'))
    w = Window('mmm', [os.path.join(CWD, 'mmm.exe'), '--site',
                       '--time', '20:00', '--date', '2026-09-19'])
    try:
        w.wait_for('SECURITY CODE?')
        w.wait_screen(lambda s: s.row(s.r).endswith('SECURITY CODE?'))
        w.type('15815\r')
        w.wait_for('WHAT IS YOUR NAME?')
        w.wait_screen(lambda s: s.row(s.r).endswith('WHAT IS YOUR NAME?'))
        if '15815' in w.screen.text():
            raise AssertionError('the security code is still on the '
                                 'screen:\n' + w.screen.text())
        w.type('WILMA\r')
        w.wait_for('HI WILMA')
        w.wait_screen(lambda s: s.row(s.r) == '>' and s.c == 1)
        w.type('LOOK\r')
        w.wait_for('YOU ARE AT THE MAIN GATE')
        w.wait_screen(lambda s: s.row(s.r) == '>' and s.c == 1)
        rows = [w.screen.row(k) for k in range(w.screen.rows)]
        if not any(r == '>LOOK' for r in rows):
            raise AssertionError('the command is not on the line of the '
                                 'prompt:\n' + w.screen.text())
        if any(r.endswith('_') for r in rows):
            raise AssertionError('a record ends in _:\n' + w.screen.text())
        w.type('QUIT\r')
        w.wait_for('DO YOU REALLY WANT TO QUIT NOW?')
        w.type('YES\r')
        w.wait_for('PRESS', "'RETURN' TO EXIT.")
        w.wait_screen(lambda s: s.row(s.r) == ' -' and s.c == 2)
        w.type('\r')
        code = w.wait_exit(30)
        if code != 0:
            raise AssertionError('exit code %d' % code)
        print('console ok')
        return 0
    except AssertionError as e:
        print('console FAILED: %s' % e)
        return 1
    finally:
        w.close()
        shutil.rmtree(CWD, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
