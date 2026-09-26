"""The port at a real console, the way run.bat runs it.

    python consoleplay.py

The console is a Windows pseudo console (ConPTY): the program sees a real
console, which does the echo and the line editing as it did for the player,
and nothing appears on the desktop.  What the console shows is kept in a small
screen model, so the checks see rows, not just text.

  1. Each answer stays on its prompt's line, and the game's reply starts on the
     line straight below, as on the reference console (the port drops the line
     feed that follows an Enter the console has already echoed).
  2. Backspace (the key sends DEL) edits the line; small letters are taken.
  3. QUIT, YES: the score and NORMAL PROGRAM COMPLETION, exit 0.
  4. Ctrl+C in the middle of a game ends it quietly, exit 0.
  5. run.bat, from a copy of the port in a scratch folder, makes the saves
     folder, and SAVE writes the game there.
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
COLS, ROWS = 81, 25
CLOCK = '--clock=2026-01-05 10:30:00'


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


class Screen:
    """the rows a terminal shows, from what the pseudo console writes: text, CR,
    LF (scrolling at the bottom), BS and the CSI sequences it uses"""

    def __init__(self):
        self.cells = [[' '] * COLS for _ in range(ROWS)]
        self.r = self.c = 0
        self.pending = ''

    def rows(self):
        return [''.join(row).rstrip() for row in self.cells]

    def _down(self):
        if self.r == ROWS - 1:
            self.cells.pop(0)
            self.cells.append([' '] * COLS)
        else:
            self.r += 1

    def feed(self, data):
        s = self.pending + data
        self.pending = ''
        i = 0
        while i < len(s):
            if s[i] == '\x1b':
                m = re.match(r'\x1b\[([?]?)([0-9;]*)[ -/]*([@-~])', s[i:])
                o = re.match(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', s[i:])
                if not m and not o:
                    if len(s) - i < 16:
                        self.pending = s[i:]
                        return
                    i += 1
                    continue
                if o:
                    i += o.end()
                    continue
                i += m.end()
                if m.group(1):
                    continue
                ps = [int(p) if p else 0 for p in m.group(2).split(';')] if m.group(2) else []
                n = ps[0] if ps and ps[0] else 1
                f = m.group(3)
                if f in 'Hf':
                    self.r = min((ps[0] if ps and ps[0] else 1) - 1, ROWS - 1)
                    self.c = min((ps[1] if len(ps) > 1 and ps[1] else 1) - 1, COLS - 1)
                elif f == 'A':
                    self.r = max(0, self.r - n)
                elif f == 'B':
                    self.r = min(ROWS - 1, self.r + n)
                elif f == 'C':
                    self.c = min(COLS - 1, self.c + n)
                elif f == 'D':
                    self.c = max(0, self.c - n)
                elif f == 'G':
                    self.c = min(n - 1, COLS - 1)
                elif f == 'd':
                    self.r = min(n - 1, ROWS - 1)
                elif f == 'J':
                    if (ps[0] if ps else 0) in (2, 3):
                        self.cells = [[' '] * COLS for _ in range(ROWS)]
                    else:
                        self.cells[self.r][self.c:] = [' '] * (COLS - self.c)
                        for k in range(self.r + 1, ROWS):
                            self.cells[k] = [' '] * COLS
                elif f == 'K':
                    self.cells[self.r][self.c:] = [' '] * (COLS - self.c)
                elif f == 'X':
                    end = min(COLS, self.c + n)
                    self.cells[self.r][self.c:end] = [' '] * (end - self.c)
                elif f == 'S':
                    for _ in range(n):
                        self.cells.pop(0)
                        self.cells.append([' '] * COLS)
                continue
            ch = s[i]
            i += 1
            if ch == '\r':
                self.c = 0
            elif ch == '\n':
                self._down()
            elif ch == '\b':
                self.c = max(0, self.c - 1)
            elif ch >= ' ' and ch != '\x7f':
                if self.c >= COLS:
                    self.c = 0
                    self._down()
                self.cells[self.r][self.c] = ch
                self.c += 1


class Window:
    """a console window nobody can see, running one command"""

    def __init__(self, name, args, cwd=None):
        self.name = name
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
        # empty standard handles: otherwise the program inherits this script's
        # redirected ones and never sees the pseudo console
        si.StartupInfo.dwFlags = 0x00000100             # STARTF_USESTDHANDLES
        si.lpAttributeList = ctypes.addressof(self.attrs)
        self.pi = PROCESS_INFORMATION()
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a if ' ' in a else a for a in args))
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, cwd,
                                  ctypes.byref(si), ctypes.byref(self.pi)):
            raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
        self.screen = Screen()
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = ctypes.create_string_buffer(65536)
        got = W.DWORD()
        while k32.ReadFile(self.out, buf, 65536, ctypes.byref(got), None) and got.value:
            with self.lock:
                self.screen.feed(buf.raw[:got.value].decode('utf-8', 'replace'))

    def rows(self):
        with self.lock:
            return self.screen.rows()

    def type(self, keys, gap=0.05):
        time.sleep(0.4)                 # a key typed as a prompt appears can be lost
        for ch in keys:
            data = ch.encode('latin-1')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def wait_for(self, what, timeout=60):
        """wait until a row holds WHAT; returns the rows then"""
        end = time.time() + timeout
        while True:
            gone = self.exited()
            rows = self.rows()
            if any(what in row for row in rows):
                return rows
            if gone or time.time() > end:
                raise AssertionError('%s: %r never appeared; the screen:\n%s'
                                     % (self.name, what, '\n'.join(rows)))
            time.sleep(0.05)

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


def below(rows, prompt, answer, reply):
    """the last row that is PROMPT with ANSWER after it has a row below it
    that starts with REPLY"""
    k = max((i for i, row in enumerate(rows)
             if row.startswith(prompt) and row.rstrip().endswith(answer)), default=-1)
    return 0 <= k < len(rows) - 1 and rows[k + 1].startswith(reply)


def main():
    ok = True
    tmp = tempfile.mkdtemp(prefix='ti990c-')
    windows = []
    try:
        w = Window('a game', [EXE, '-u', CLOCK], cwd=tmp)
        windows.append(w)
        w.wait_for('Will/did you save your game?:')
        w.type('no\r')
        rows = w.wait_for('Would you like instructions?')
        assert below(rows, ' Will/did you save your game?:', 'NO  no', 'Welcome to ADVENTURE!!'), \
            'the first answer:\n' + '\n'.join(rows)
        w.type('no\r')
        rows = w.wait_for('end of a road')
        assert below(rows, '[=]', 'no', 'You are standing at the end of a road'), \
            'the second answer:\n' + '\n'.join(rows)
        print('ok    each answer stays on its prompt\'s line; the reply starts right below')
        w.type('ix\x7fn\r')
        rows = w.wait_for('inside a building')
        assert below(rows, '[=]', 'in', 'You are inside a building'), '\n'.join(rows)
        print('ok    Backspace edits the line; small letters are taken')
        w.type('quit\r')
        w.wait_for('Do you really want to quit now?')
        w.type('yes\r')
        w.wait_for('NORMAL PROGRAM COMPLETION')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    QUIT ends with NORMAL PROGRAM COMPLETION (exit 0)')

        w = Window('Ctrl+C', [EXE, '-u', CLOCK], cwd=tmp)
        windows.append(w)
        w.wait_for('Will/did you save your game?:')
        w.type('no\r')
        w.wait_for('Would you like instructions?')
        w.type('no\r')
        w.wait_for('end of a road')
        w.type('\x03')
        code = w.wait_exit()
        assert code == 0, 'exit %#x' % code
        print('ok    Ctrl+C ends the game quietly (exit 0)')

        copy = os.path.join(tmp, 'port copy')
        os.mkdir(copy)
        for f in ('run.bat', os.path.basename(EXE)):
            shutil.copy(os.path.join(PORT, f), copy)
        # one quoted argument only (the path has a space): cmd /c keeps the
        # quotes then, so the clock is given as a time without a date
        w = Window('run.bat', [os.environ.get('COMSPEC', 'cmd.exe'), '/c',
                                os.path.join(copy, 'run.bat'), '--clock=10:30:00'], cwd=tmp)
        windows.append(w)
        w.wait_for('Will/did you save your game?:')
        w.type('yes\r')
        w.wait_for('SAVE/RESTORE PATHNAME:')
        w.type('t.sav\r')
        w.wait_for('Would you like instructions?')
        w.type('no\r')
        w.wait_for('end of a road')
        w.type('save\r')
        w.wait_for('Is this acceptable?')
        w.type('yes\r')
        w.wait_for('NORMAL PROGRAM COMPLETION')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        assert os.path.exists(os.path.join(copy, 'saves', 't.sav')), 'no saves\\t.sav'
        print('ok    run.bat plays, and SAVE writes saves\\t.sav')
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
