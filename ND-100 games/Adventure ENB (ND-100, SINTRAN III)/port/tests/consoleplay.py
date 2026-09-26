"""ADVENTURE-ENB at a real console, the way it is played.

    python tests/consoleplay.py [path/to/advenb.exe]

Runs the port in a Windows pseudo console (ConPTY), so it reads real key
events and writes to a real console, with nothing on the desktop, in the
reference land (-Z):

  1. The first question, answered in small letters; the land, with the
     Swedish letters shown as themselves.
  2. The arrow keys walk (the program reads ESC A ... ESC D), Home shows the
     map (ESC H).
  3. Commands typed in small letters (t, ?); Enter at the question after ?.
  4. S ends the game, and the window is held open with [press any key].
"""
import ctypes
import ctypes.wintypes as W
import os
import re
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CR, ESC = chr(13), chr(27)
UP, DOWN, RIGHT, LEFT, HOME = ESC + '[A', ESC + '[B', ESC + '[C', ESC + '[D', ESC + '[H'
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'advenb.exe')
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
    t = re.sub(r'\x1b[()][0-9A-Za-z]', '', t)
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

    def type(self, text, gap=0.05):
        for ch in text:
            self.write(ch)
            time.sleep(gap)

    def write(self, text):
        data = text.encode('utf-8')
        n = W.DWORD()
        k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
        time.sleep(0.003)

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
    w = Window([EXE, '-Z', '1'], os.path.dirname(EXE))
    try:
        w.wait_for('Vill du ha instruktioner?', timeout=60)
        w.type('n' + CR)
        w.wait_for('Vänta ett tag tack')
        w.wait_for('Du är vid en bro')
        w.wait_for('ORDER:')
        print('ok    the first question in small letters; the Swedish letters shown as themselves')
        w.write(UP)
        w.wait_for('Norrut')
        w.wait_for('Du är vid en väg')
        w.write(LEFT)
        w.wait_for('Västerut')
        w.wait_for('Du är vid ett fält')
        w.write(HOME)
        w.wait_for('Karta')
        w.wait_for('Du')
        print('ok    the arrow keys walk, Home shows the map')
        w.write(RIGHT)
        w.wait_for('Österut')
        w.write(DOWN)
        w.wait_for('Söderut')
        w.type('t')
        w.wait_for('Ta')
        w.type('?')
        w.wait_for('Dina koordinater är')
        w.wait_for("TRYCK 'RETURN' NÄR DU LÄST FÄRDIGT")
        w.type(CR)
        w.wait_for('ORDER:')
        print('ok    commands in small letters, Enter after ?')
        w.type('s')
        w.wait_for('Sluta')
        w.wait_for('[press any key]', timeout=30)
        w.type(' ')
        w.wait_exit()
        print('ok    S ends the game, and the window is held open')
    finally:
        w.close()


if __name__ == '__main__':
    main()
