"""SVHA Adventure at a real console, the way it is played.

    python tests/consoleplay.py [path/to/svha.exe]

Runs the port in a Windows pseudo console (ConPTY), so it reads real key
events and writes to a real console, with nothing on the desktop:

  1. The title and the question appear before anything is typed.
  2. Lower-case input works.
  3. Backspace sends the ND delete key; the game echoes ^.
  4. SAVE asks for a file name at the console and writes the game; the
     player walks off; RESTORE brings the game back.
  5. ESC stops the game with SINTRAN's USER BREAK message and its @, where
     CONTINUE goes back into the game and LOGOUT ends it.
  6. The saved game resumes from the command line, and QUIT ends it.

The program is alone on its pseudo console, as when it is started from
Explorer, so it holds the window with [press any key] at the end.

Everything is written in a throwaway directory.
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
CR, BS, ESC = chr(13), chr(127), chr(27)
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'svha.exe')
EXE = os.path.abspath(EXE)

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
    t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', raw)
    t = re.sub(r'\x1b\[[0-9;?]*[ -/]*[@-~]', '', t)
    return re.sub(r'\s+', '', t)


class Window:
    def __init__(self, args, cwd):
        in_r, in_w, out_r, out_w = W.HANDLE(), W.HANDLE(), W.HANDLE(), W.HANDLE()
        k32.CreatePipe(ctypes.byref(in_r), ctypes.byref(in_w), None, 0)
        k32.CreatePipe(ctypes.byref(out_r), ctypes.byref(out_w), None, 0)
        self.hpc = W.HANDLE()
        hr = k32.CreatePseudoConsole(COORD(100, 40), in_r, out_w, 0, ctypes.byref(self.hpc))
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
        si.StartupInfo.dwFlags = 0x00000100             # STARTF_USESTDHANDLES, empty
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

    def type(self, text, gap=0.03):
        for ch in text:
            data = ch.encode('utf-8')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def wait_for(self, what, timeout=30):
        end = time.time() + timeout
        want = squeezed(what)
        while True:
            with self.lock:
                t = squeezed(self.raw)
            k = t.find(want, self.mark)
            if k >= 0:
                self.mark = k + len(want)
                return
            if self.exited() or time.time() > end:
                raise AssertionError('never saw %r; the console showed:\n%s' % (what, self.raw[-1500:]))
            time.sleep(0.05)

    def exited(self):
        return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=30):
        if k32.WaitForSingleObject(self.pi.hProcess, int(timeout * 1000)) != 0:
            raise AssertionError('the program did not finish')
        code = W.DWORD()
        k32.GetExitCodeProcess(self.pi.hProcess, ctypes.byref(code))
        return code.value

    def close(self):
        if not self.exited():
            k32.TerminateProcess(self.pi.hProcess, 1)
        k32.ClosePseudoConsole(self.hpc)


def main():
    work = tempfile.mkdtemp(prefix='svhacon')
    t0 = 1789632000
    try:
        w = Window([EXE, '--no-hold', '-Z', str(t0)], work)
        w.wait_for('Welcome to Adventure!!  Would you like instructions?')
        print('ok    title and question drawn before any key')
        w.type('no' + CR)
        w.wait_for('You are standing at the end of a road before a small brick building.')
        print('ok    lower case accepted')
        w.type('in' + CR)
        w.wait_for('There is a shiny brass lamp nearby.')
        w.type('takeX' + BS + ' lamp' + CR)
        w.wait_for('takeX^ lamp')
        print('ok    Backspace is the ND delete key (the game echoes ^)')
        w.type('take lamp' + CR)
        w.wait_for('OK')
        w.type('save' + CR)
        w.wait_for('Save the game as')
        w.type('lamp taken' + CR)
        w.wait_for('RESTORE brings it back')
        w.type('out' + CR)
        w.wait_for('end of road')
        w.type('restore' + CR)
        w.wait_for('Restore the game from')
        w.type('lamp taken' + CR)
        w.wait_for('Restored.')
        w.type('look' + CR)
        w.wait_for('You are inside a building')
        print('ok    SAVE and RESTORE at the console')
        w.type(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('continue' + CR)
        w.type('inventory' + CR)
        w.wait_for('Brass lantern')
        print('ok    ESC is a SINTRAN user break; CONTINUE goes back into the game')
        w.type(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('logout' + CR)
        w.wait_for('[press any key]')
        w.type(' ')
        code = w.wait_exit()
        assert code == 1, code
        w.close()
        print('ok    LOGOUT at the @ ends it')

        w = Window([EXE, '--no-hold', 'lamp taken'], work)
        w.type('inventory' + CR)
        w.wait_for('Brass lantern')
        w.type('quit' + CR)
        w.wait_for('Welcome back!!')
        w.wait_for('[press any key]')
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, code
        w.close()
        print('ok    the saved game resumes from the command line; QUIT ends it')
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
