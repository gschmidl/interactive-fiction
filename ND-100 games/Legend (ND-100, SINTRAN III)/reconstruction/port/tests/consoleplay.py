"""Legend at a real console, the way it is played.

    python tests/consoleplay.py [path/to/legend.exe]

Runs the port in a Windows pseudo console (ConPTY), so it reads real key
events and writes to a real console, with nothing on the desktop:

  1. The title and the signature question; a signature typed in small
     letters comes back in capitals (SINTRAN's @TERMINAL-MODE).
  2. A world, no instructions, RETURN, the menu; a player created.
  3. The game: the market street with its Swedish letters (på, står, Öst).
  4. Backspace rubs out a letter (the command still works), Esc and the
     arrow keys do nothing, Enter on an empty line repeats the last command.
  5. SLU ("So Long"), and the window held open with [press any key].

Everything runs on a throwaway copy of data\.
"""
import ctypes
import ctypes.wintypes as W
import datetime
import os
import re
import shutil
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CR, ESC = chr(13), chr(27)
UP, DOWN, RIGHT, LEFT, HOME = '[A', '[B', '[C', '[D', '[H'
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'legend.exe')
EXE = os.path.abspath(EXE)
DATA = os.path.join(HERE, '..', 'data')

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
    work = tempfile.mkdtemp(prefix='legendcon')
    for f in os.listdir(DATA):
        shutil.copy(os.path.join(DATA, f), work)
    try:
        w = Window([EXE, '--data', work, '--no-hold'], work)
        w.wait_for('Legend    v10.0', timeout=60)
        w.wait_for('Din signatur:')
        w.type('gs' + CR)
        w.wait_for('GS')
        print('ok    title; a signature typed small comes back in capitals')
        w.wait_for('Vilket scenario? (1-9, 0):')
        w.type('6' + CR)
        w.wait_for("('J'a / 'N'ej)  <N>")
        w.type('n')
        w.wait_for('Tryck RETURN', timeout=60)
        w.type(CR)
        w.wait_for('Välj:', timeout=60)
        w.type('B')
        w.wait_for('(1-4):')
        w.type('3' + CR)
        w.wait_for('2.Klanen?')
        w.type('1' + CR)
        w.wait_for('Vad ska spelaren heta?')
        w.type('magda' + CR)
        w.wait_for('MAGDA')
        w.wait_for('Välj:', timeout=60)
        print('ok    a world, the menu, a player created')
        w.type('C')
        w.wait_for('Vad heter du?', timeout=60)
        w.type('Kalle Anka' + CR)
        w.wait_for('Du står mitt på stadens huvudgata', timeout=60)
        w.wait_for('Utgångar: Norr Söder Öst Väst')
        print('ok    the market street, with its Swedish letters')
        w.wait_for('$')
        # Backspace: "tittx" rubbed out to "titt", then "a"
        w.type('tittx', gap=0.05)
        w.write('\x08')
        w.type('a' + CR, gap=0.05)
        w.wait_for('Du ser:', timeout=30)
        w.wait_for('$')
        with w.lock:
            raw = w.raw
        assert 'Qu' not in raw[-600:], 'the rubbed-out command was not understood'
        print('ok    Backspace rubs out a character')
        for key in (ESC, UP, LEFT):
            w.write(key)
            time.sleep(0.3)
        w.type('status' + CR, gap=0.05)
        w.wait_for('Magda har gått', timeout=30)
        w.wait_for('$')
        with w.lock:
            raw = w.raw
        assert 'Qu' not in raw[-900:] and not w.exited(), 'Esc or an arrow key reached the game'
        print('ok    Esc and the arrow keys do nothing')
        w.type(CR)
        w.wait_for('STATUS', timeout=30)
        w.wait_for('Magda har gått', timeout=30)
        print('ok    Enter on an empty line repeats the last command')
        w.wait_for('$')
        w.type('slu' + CR)
        w.wait_for('So Long, GS.', timeout=60)
        w.wait_for('[press any key]', timeout=60)
        w.type(' ')
        w.wait_exit()
        w.close()
        print('ok    SLU, and the window held open')
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
