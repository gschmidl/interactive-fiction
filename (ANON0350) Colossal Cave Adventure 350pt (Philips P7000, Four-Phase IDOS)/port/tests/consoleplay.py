"""The port at a real console, the way play.bat runs it.

    python consoleplay.py

The console is a Windows pseudo console (ConPTY): the program sees a real
console and reads real key events, and nothing appears on the desktop.

  1. A game: NO to the instructions, IN, SUSPEND, YES; the status line then
     says the game is over, and a key closes the window with exit 0.
  2. The next window goes on inside the building: QUIT, YES, a key.
  3. What is typed shows on the line under the "==>" prompt (the machine's
     entry line is its top line; the console shows it at the bottom).
     Backspace: "NOX", Backspace, Enter is taken as NO.
  4. Ctrl+C in the middle of a game leaves at once with exit 0.
  5. play.bat (from a copy of the port in a scratch folder) makes the saves
     folder and its pack, and plays.

Every pack is a scratch copy, thrown away afterwards.
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

from common import EXE, PORT

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


k32.CreatePipe.argtypes = [ctypes.POINTER(W.HANDLE), ctypes.POINTER(W.HANDLE), ctypes.c_void_p,
                           W.DWORD]
k32.CreatePseudoConsole.argtypes = [COORD, W.HANDLE, W.HANDLE, W.DWORD, ctypes.POINTER(W.HANDLE)]
k32.CreatePseudoConsole.restype = ctypes.c_long
k32.ClosePseudoConsole.argtypes = [W.HANDLE]
k32.InitializeProcThreadAttributeList.argtypes = [ctypes.c_void_p, W.DWORD, W.DWORD,
                                                  ctypes.POINTER(ctypes.c_size_t)]
k32.UpdateProcThreadAttribute.argtypes = [ctypes.c_void_p, W.DWORD, ctypes.c_size_t,
                                          ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
                                          ctypes.c_void_p]
k32.CreateProcessW.argtypes = [W.LPCWSTR, W.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, W.BOOL,
                               W.DWORD, ctypes.c_void_p, W.LPCWSTR, ctypes.c_void_p,
                               ctypes.POINTER(PROCESS_INFORMATION)]
k32.ReadFile.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD),
                         ctypes.c_void_p]
k32.WriteFile.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD),
                          ctypes.c_void_p]
k32.WaitForSingleObject.argtypes = [W.HANDLE, W.DWORD]
k32.GetExitCodeProcess.argtypes = [W.HANDLE, ctypes.POINTER(W.DWORD)]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]


def screen_rows(raw, cols=81, rows=25):
    """the rows a terminal shows after RAW: the text, cursor moves and erases
    a pseudo console writes (CSI H, f, A-D, G, d, J, K, X; CR, LF, BS)"""
    cells = [[' '] * cols for _ in range(rows)]
    r = c = 0
    i = 0
    while i < len(raw):
        m = re.match(r'\x1b\[([?]?)([0-9;]*)[ -/]*([@-~])', raw[i:])
        if m:
            i += m.end()
            if m.group(1):
                continue
            ps = [int(p) if p else 0 for p in m.group(2).split(';')] if m.group(2) else []
            n = ps[0] if ps and ps[0] else 1
            f = m.group(3)
            if f in 'Hf':
                r = min((ps[0] if ps and ps[0] else 1) - 1, rows - 1)
                c = min((ps[1] if len(ps) > 1 and ps[1] else 1) - 1, cols - 1)
            elif f == 'A':
                r = max(0, r - n)
            elif f == 'B':
                r = min(rows - 1, r + n)
            elif f == 'C':
                c = min(cols - 1, c + n)
            elif f == 'D':
                c = max(0, c - n)
            elif f == 'G':
                c = min(n - 1, cols - 1)
            elif f == 'd':
                r = min(n - 1, rows - 1)
            elif f == 'J' and (ps[0] if ps else 0) in (2, 3):
                cells = [[' '] * cols for _ in range(rows)]
            elif f == 'J':
                cells[r][c:] = [' '] * (cols - c)
                for k in range(r + 1, rows):
                    cells[k] = [' '] * cols
            elif f == 'K':
                cells[r][c:] = [' '] * (cols - c)
            elif f == 'X':
                cells[r][c:c + n] = [' '] * len(cells[r][c:c + n])
            continue
        m = re.match(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', raw[i:])
        if m:
            i += m.end()
            continue
        ch = raw[i]
        i += 1
        if ch == '\r':
            c = 0
        elif ch == '\n':
            r = min(rows - 1, r + 1)
        elif ch == '\b':
            c = max(0, c - 1)
        elif ch >= ' ' and ch != '\x7f':
            if c >= cols:
                c, r = 0, min(rows - 1, r + 1)
            cells[r][c] = ch
            c += 1
    return [''.join(row).rstrip() for row in cells]


def squeezed(raw):
    """the text of a console stream without escape sequences or white space:
    the console redraws as it likes, so a phrase can arrive in pieces"""
    t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', raw)
    t = re.sub(r'\x1b\[[0-9;?]*[ -/]*[@-~]', ' ', t)
    return re.sub(r'\s+', '', t)


class Window:
    """a console window nobody can see, running one command"""

    def __init__(self, name, args):
        self.name = name
        in_r, in_w, out_r, out_w = W.HANDLE(), W.HANDLE(), W.HANDLE(), W.HANDLE()
        k32.CreatePipe(ctypes.byref(in_r), ctypes.byref(in_w), None, 0)
        k32.CreatePipe(ctypes.byref(out_r), ctypes.byref(out_w), None, 0)
        self.hpc = W.HANDLE()
        hr = k32.CreatePseudoConsole(COORD(81, 25), in_r, out_w, 0, ctypes.byref(self.hpc))
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
        # empty standard handles: otherwise the program inherits this script's
        # redirected ones and never sees the pseudo console
        si.StartupInfo.dwFlags = 0x00000100             # STARTF_USESTDHANDLES
        si.lpAttributeList = ctypes.addressof(self.attrs)
        self.pi = PROCESS_INFORMATION()
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a if ' ' in a else a for a in args))
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, None,
                                  ctypes.byref(si), ctypes.byref(self.pi)):
            raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
        self.raw = ''
        self.mark = 0
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = ctypes.create_string_buffer(65536)
        got = W.DWORD()
        while k32.ReadFile(self.out, buf, 65536, ctypes.byref(got), None) and got.value:
            with self.lock:
                self.raw += buf.raw[:got.value].decode('utf-8', 'replace')

    def type(self, keys, gap=0.05):
        for ch in keys:
            data = ch.encode('latin-1')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def wait_for(self, what, timeout=60):
        """wait for WHAT to be drawn after the last thing found"""
        end = time.time() + timeout
        want = re.sub(r'\s+', '', what)
        while True:
            gone = self.exited()
            with self.lock:
                t = squeezed(self.raw)
            k = t.find(want, self.mark)
            if k >= 0:
                self.mark = k + len(want)
                return
            if gone or time.time() > end:
                break
            time.sleep(0.05)
        with self.lock:
            tail = squeezed(self.raw)[-600:]
        raise AssertionError('%s: %r never appeared; the end of the stream:\n%s'
                             % (self.name, what, tail))

    def exited(self):
        return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=30):
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
    ok = True
    tmp = tempfile.mkdtemp(prefix='advp7c-')
    windows = []
    try:
        pack = '--pack=' + os.path.join(tmp, 'console.pack')
        w = Window('the game', [EXE, '-u', pack])
        windows.append(w)
        w.wait_for('WOULD YOU LIKE INSTRUCTIONS?')
        w.type('NO\r')
        w.wait_for('END OF A ROAD')
        w.type('IN\r')
        w.wait_for('INSIDE A BUILDING')
        w.type('SUSPEND\r')
        w.wait_for('IS THIS ACCEPTABLE?')
        w.type('YES\r')
        w.wait_for('The game is over')
        time.sleep(0.5)
        assert not w.exited(), 'the window closed before a key was pressed'
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    SUSPEND at the console; a key closes the window (exit 0)')

        w = Window('the suspended game', [EXE, '-u', pack])
        windows.append(w)
        w.wait_for("YOU'RE INSIDE BUILDING.")
        w.type('QUIT\r')
        w.wait_for('DO YOU REALLY WANT TO QUIT NOW?')
        w.type('YES\r')
        w.wait_for('YOU SCORED')
        w.wait_for('The game is over')
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    the next window goes on with the suspended game; QUIT ends it')

        w = Window('Backspace', [EXE, '-u', pack])
        windows.append(w)
        w.wait_for('WOULD YOU LIKE INSTRUCTIONS?')
        w.type('NOX')
        end = time.time() + 20
        while True:
            with w.lock:
                rows = screen_rows(w.raw)
            prompt = max((k for k, row in enumerate(rows) if row.startswith('==>')), default=-1)
            if 0 <= prompt < len(rows) - 1 and rows[prompt + 1].strip() == 'NOX':
                break
            assert time.time() < end, 'what was typed is not under the prompt:\n' + '\n'.join(rows)
            time.sleep(0.1)
        print('ok    what is typed shows on the line under the prompt')
        w.type('\b\r')
        w.wait_for('END OF A ROAD')
        print('ok    Backspace deletes the last character typed')
        w.type('\x03')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    Ctrl+C leaves at once (exit 0)')

        copy = os.path.join(tmp, 'port copy')
        os.mkdir(copy)
        for f in ('play.bat', os.path.basename(EXE), 'p7000.pack'):
            shutil.copy(os.path.join(PORT, f), copy)
        w = Window('play.bat', [os.environ.get('COMSPEC', 'cmd.exe'), '/c',
                                os.path.join(copy, 'play.bat')])
        windows.append(w)
        w.wait_for('WOULD YOU LIKE INSTRUCTIONS?')
        w.type('NO\r')
        w.wait_for('END OF A ROAD')
        w.type('QUIT\r')
        w.wait_for('DO YOU REALLY WANT TO QUIT NOW?')
        w.type('YES\r')
        w.wait_for('The game is over')
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        assert os.path.exists(os.path.join(copy, 'saves', 'advent.pack')), 'no saves\\advent.pack'
        print('ok    play.bat makes saves\\advent.pack and plays')
    except AssertionError as e:
        print('FAIL ', e)
        ok = False
    finally:
        for w in windows:
            w.close()
        time.sleep(0.3)
        shutil.rmtree(tmp, ignore_errors=True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
