#!/usr/bin/env python3
"""The port at a real console, as run.bat runs it.

    python tests\\consoleplay.py

A Windows pseudo console (ConPTY) runs a scratch copy of adv462.exe: the
program sees a real console and reads real key events, and nothing appears
on the desktop.  What a pipe cannot show:

  1. the question is on the screen before the program waits for the answer,
     and the room before it waits for a command (the game has no prompt
     character: it prints a blank line and waits);
  2. typed mixed case is played;
  3. SUSPEND and yes: the program ends, exit 0, and the game is in saves\\;
  4. in a new window, RESTORE brings it back at once (no 90-minute wait);
     QUIT and yes: the score, and the program ends, exit 0;
  5. Ctrl-Z Enter at a command: the program ends at once, exit 0.
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

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)

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


def squeezed(raw):
    """what was drawn, without escape sequences or white space: the pseudo
    console redraws as it likes, and a phrase can arrive in pieces"""
    t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', raw)
    t = re.sub(r'\x1b\[[0-9;?]*[ -/]*[@-~]', ' ', t)
    return re.sub(r'\s+', '', t)


class Window:
    """A console window nobody can see, running one command."""

    def __init__(self, args, cwd):
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
        # empty standard handles, or the program inherits this script's
        # redirected ones and never sees the pseudo console
        si.StartupInfo.dwFlags = 0x00000100             # STARTF_USESTDHANDLES
        si.lpAttributeList = ctypes.addressof(self.attrs)
        self.pi = PROCESS_INFORMATION()
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a for a in args))
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000, None, cwd,
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

    def type(self, keys, gap=0.02):
        for ch in keys:
            data = ch.encode('latin-1')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def drawn(self):
        with self.lock:
            return squeezed(self.raw)

    def wait_for(self, what, timeout=30):
        """wait for WHAT to be drawn after what was last waited for"""
        end = time.time() + timeout
        want = squeezed(what)
        while True:
            t = self.drawn()
            k = t.find(want, self.mark)
            if k >= 0:
                self.mark = k + len(want)
                return
            if self.exited() or time.time() > end:
                break
            time.sleep(0.05)
        raise AssertionError('never drawn: %r; the window had:\n%s' % (what, self.raw[-2000:]))

    def waiting_after(self, what):
        """WHAT drawn, and nothing more: the program has shown it and waits
        for the keyboard"""
        self.wait_for(what)
        time.sleep(0.3)
        rest = self.drawn()[self.mark:]
        if rest or self.exited():
            raise AssertionError('not waiting after %r: %r' % (what, rest[:200]))

    def exited(self):
        return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=30):
        if k32.WaitForSingleObject(self.pi.hProcess, int(timeout * 1000)) != 0:
            raise AssertionError('the program did not end')
        code = W.DWORD()
        k32.GetExitCodeProcess(self.pi.hProcess, ctypes.byref(code))
        return code.value

    def close(self):
        if self.hpc:
            if not self.exited():
                k32.TerminateProcess(self.pi.hProcess, 1)
            k32.ClosePseudoConsole(self.hpc)
            self.hpc = None


QUESTION = 'Would you like instructions?'
BOARD = 'A bulletin board is nailed to a tree near the building.'
HOUSE = "You're inside building."     # short form: the room has been seen


def main():
    tmp = tempfile.mkdtemp(prefix='adv462-console-')
    exe = os.path.join(tmp, 'adv462.exe')
    for f in ('adv462.exe', 'adventure.newgame'):
        shutil.copy(os.path.join(PORT, f), tmp)
    failed = []

    def step(name, fn):
        try:
            fn()
            print('%-58s ok' % name)
            return True
        except AssertionError as e:
            print('%-58s FAILED\n    %s' % (name, str(e).replace('\n', '\n    ')))
            failed.append(name)
            return False

    def ends_ok(win):
        code = win.wait_exit()
        if code != 0:
            raise AssertionError('exit %d' % code)

    def first():
        win = Window([exe], tmp)
        try:
            if not step('the question on the screen before the first read',
                        lambda: win.waiting_after(QUESTION)):
                return

            def mixed():
                win.type('No\r')
                win.waiting_after(BOARD)
                win.type('IN\r')
                win.waiting_after('There is a bottle of water here.')
                win.type('Take All\r')
                win.waiting_after('Small bottle: taken.')
            if not step('typed mixed case; the room before each read', mixed):
                return

            def suspend():
                win.type('suspend console\r')
                win.waiting_after('Is this acceptable?')
                win.type('yes\r')
                ends_ok(win)
                if not os.path.isfile(os.path.join(tmp, 'saves', 'console.adv462')):
                    raise AssertionError('no saves\\console.adv462')
            step('SUSPEND, yes: exit 0, the game in saves\\', suspend)
        finally:
            win.close()

    def second():
        win = Window([exe], tmp)
        try:
            def restore():
                win.waiting_after(QUESTION)
                win.type('no\r')
                win.waiting_after(BOARD)
                win.type('restore console\r')
                win.waiting_after(HOUSE)
                win.type('inventory\r')
                win.wait_for('Small bottle')
                win.waiting_after('Water in the bottle')
            if not step('RESTORE at once: the house, the things carried', restore):
                return

            def quit_():
                win.type('quit\r')
                win.waiting_after('Do you really want to quit now?')
                win.type('yes\r')
                win.wait_for('You scored')
                ends_ok(win)
            step('QUIT, yes: the score, exit 0', quit_)
        finally:
            win.close()

    def ctrl_z():
        win = Window([exe], tmp)
        try:
            def fn():
                win.waiting_after(QUESTION)
                win.type('no\r')
                win.waiting_after(BOARD)
                win.type('\x1a\r')
                ends_ok(win)
            step('Ctrl-Z Enter at a command: the end, exit 0', fn)
        finally:
            win.close()

    try:
        first()
        second()
        ctrl_z()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('%d FAILED' % len(failed) if failed else 'all passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
