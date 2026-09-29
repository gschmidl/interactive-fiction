#!/usr/bin/env python3
"""console_check.py - play adventure.exe at a real (invisible) Windows console.

Piped tests cannot see three things a player would: whether the prompt is on
the screen *before* the game waits for input, whether the VT100 sequences of
the .100 database reach the terminal, and whether the game ends by itself and
leaves the terminal in ANSI mode.  This runs the game inside a ConPTY pseudo
console and checks them.  Scratch save directory; port/save is not touched.

    python test/console_check.py
"""
import ctypes, ctypes.wintypes as W, os, re, shutil, sys, tempfile, threading, time

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(PORT, 'adventure.exe')
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


class Window:
    """a console nobody can see, running one command"""
    def __init__(self, args, env):
        in_r, in_w, out_r, out_w = W.HANDLE(), W.HANDLE(), W.HANDLE(), W.HANDLE()
        k32.CreatePipe(ctypes.byref(in_r), ctypes.byref(in_w), None, 0)
        k32.CreatePipe(ctypes.byref(out_r), ctypes.byref(out_w), None, 0)
        self.hpc = W.HANDLE()
        hr = k32.CreatePseudoConsole(COORD(80, 24), in_r, out_w, 0, ctypes.byref(self.hpc))
        if hr != 0: raise OSError('CreatePseudoConsole failed: %x' % (hr & 0xFFFFFFFF))
        k32.CloseHandle(in_r); k32.CloseHandle(out_w)
        self.inp, self.out = in_w, out_r
        size = ctypes.c_size_t()
        k32.InitializeProcThreadAttributeList(None, 1, 0, ctypes.byref(size))
        self.attrs = (ctypes.c_byte * size.value)()
        k32.InitializeProcThreadAttributeList(self.attrs, 1, 0, ctypes.byref(size))
        k32.UpdateProcThreadAttribute(self.attrs, 0, 0x00020016, self.hpc, ctypes.sizeof(W.HANDLE), None, None)
        si = STARTUPINFOEXW()
        si.StartupInfo.cb = ctypes.sizeof(STARTUPINFOEXW)
        si.StartupInfo.dwFlags = 0x100          # STARTF_USESTDHANDLES with no handles: use the console
        si.lpAttributeList = ctypes.addressof(self.attrs)
        self.pi = PROCESS_INFORMATION()
        cmd = ctypes.create_unicode_buffer(' '.join('"%s"' % a if ' ' in a else a for a in args))
        block = ''.join('%s=%s\0' % kv for kv in env.items()) + '\0'
        if not k32.CreateProcessW(None, cmd, None, None, False, 0x00080000 | 0x400, ctypes.c_wchar_p(block), PORT,
                                  ctypes.byref(si), ctypes.byref(self.pi)):
            raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
        self.raw = ''
        self.mark = 0
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = ctypes.create_string_buffer(65536); got = W.DWORD()
        while k32.ReadFile(self.out, buf, 65536, ctypes.byref(got), None) and got.value:
            with self.lock: self.raw += buf.raw[:got.value].decode('latin-1')

    def type(self, keys):
        for ch in keys:
            n = W.DWORD(); data = ch.encode('latin-1')
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(0.02)

    def squeezed(self):
        with self.lock: t = self.raw
        t = re.sub(r'\x1b\][^\x07\x1b]*(\x07|\x1b\\)', '', t)
        t = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', t)
        return re.sub(r'\s+', '', t)

    def wait_for(self, what, timeout=30):
        """wait until `what` has been drawn after the last thing waited for"""
        want = re.sub(r'\s+', '', what); end = time.time() + timeout
        while time.time() < end:
            t = self.squeezed(); k = t.find(want, self.mark)
            if k >= 0:
                self.mark = k + len(want); return
            if self.exited(): break
            time.sleep(0.05)
        raise AssertionError('never saw %r; the console got:\n%s' % (what, self.raw[-1500:]))

    def exited(self): return k32.WaitForSingleObject(self.pi.hProcess, 0) == 0

    def wait_exit(self, timeout=20):
        if k32.WaitForSingleObject(self.pi.hProcess, int(timeout * 1000)) != 0:
            raise AssertionError('the game did not end by itself')
        code = W.DWORD(); k32.GetExitCodeProcess(self.pi.hProcess, ctypes.byref(code))
        return code.value

    def close(self):
        if not self.exited(): k32.TerminateProcess(self.pi.hProcess, 1)
        k32.ClosePseudoConsole(self.hpc)


def main():
    save = tempfile.mkdtemp(prefix='advpas-console-')
    env = dict(os.environ, ADVPAS_DATA=os.path.join(PORT, 'data'), ADVPAS_SAVE=save)
    w = Window([EXE, '-u', '-d', '28-OCT-1980', '-t', '120000'], env)
    try:
        # 1. every question is on the screen before anything is typed
        w.wait_for('Are you using a VT100?'); w.wait_for('->')
        w.type('y\r')
        w.wait_for('Would you like instructions?'); w.wait_for('->')
        w.type('no\r')
        w.wait_for('end of a road'); w.wait_for('->')
        print('ok  prompts are drawn before the game waits for input')
        # 2. the VT100 database: the debris room note is double height
        for cmd, see in (('enter\r', 'inside a building'), ('take lamp\r', 'OK'), ('xyzzy\r', 'pitch dark'),
                         ('on\r', 'debris room')):
            w.type(cmd); w.wait_for(see); w.wait_for('->')
        with w.lock: raw = w.raw
        # the double height line is sent twice (top half, bottom half); ConPTY
        # passes the line renditions on to the terminal it is attached to
        if raw.count('"Magic Word XYZZY"') >= 2 and '\x1b#3' in raw and '\x1b#4' in raw:
            print('ok  ESC#3 / ESC#4 double-height text reaches the terminal')
        elif raw.count('"Magic Word XYZZY"') >= 2:
            print('ok  double-height text drawn (this console host keeps the ESC# codes to itself)')
        else:
            raise AssertionError('the XYZZY note was not drawn twice:\n' + raw[-800:])
        # The two halves must be on consecutive rows, and no big line may be
        # followed by a row of its own wrapped padding.  Every line is written
        # as 72 columns and a double-width row holds 40: with the terminal's
        # auto wrap on, the trailing blanks made a blank single-width row
        # (ESC#5) under each one and the halves of "Adventure!!" came apart.
        big = re.compile(r'\x1b#3[^\r\n]*((?:\r|\n|\x1b#5)*)\x1b#4')
        gaps = [m.group(1) for m in big.finditer(raw)]
        assert gaps, 'no double-height pair in the stream'
        for g in gaps:
            if g.count('\n') != 1 or '\x1b#5' in g:
                raise AssertionError('top and bottom halves are not on consecutive rows: %r' % g)
        wrapped = re.findall(r'\x1b#6[^\r\n]*\r\n\x1b#5\n\x1b#[36]', raw)
        assert not wrapped, 'a double-width line wrapped its padding: %r' % wrapped[:2]
        print('ok  %d double-height pairs on consecutive rows, no wrapped padding' % len(gaps))
        # 3. quit: ends by itself, and the terminal is put back in ANSI mode
        w.type('quit\r'); w.wait_for('really want to quit'); w.wait_for('->')
        w.type('yes\r'); w.wait_for('out of a possible 350')
        code = w.wait_exit()
        assert code == 0, 'exit code %d' % code
        print('ok  the game ends by itself, exit code 0')
    finally:
        w.close()
        shutil.rmtree(save, ignore_errors=True)
    # 4. Ctrl-C in the middle of a VT100 game: the handler puts the terminal
    #    back (auto wrap on, ANSI mode) and the program ends, it does not hang
    save = tempfile.mkdtemp(prefix='advpas-console-')
    env['ADVPAS_SAVE'] = save
    w = Window([EXE, '-u', '-d', '28-OCT-1980', '-t', '120000'], env)
    try:
        w.wait_for('Are you using a VT100?'); w.wait_for('->'); w.type('y\r')
        w.wait_for('Would you like instructions?'); w.wait_for('->')
        w.type('\x03')
        # (the console read also returns "end of input" on Ctrl-C, so whichever of
        # the normal exit path and the handler gets there first restores the terminal)
        code = w.wait_exit()
        print('ok  Ctrl-C ends the game, it does not hang (exit code %#x)' % code)
    finally:
        w.close()
        shutil.rmtree(save, ignore_errors=True)
    print('console check passed')

if __name__ == '__main__':
    main()
