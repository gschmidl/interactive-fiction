"""Mordor at a real console, the way it is played.

    python tests/consoleplay.py [path/to/mordor.exe]

Runs the port in a Windows pseudo console (ConPTY), so it reads real key
events and writes to a real console, with nothing on the desktop:

  1. Between 08 and 16 the game refuses to start, as it did; the window is
     held open with [press any key].
  2. With --unlimited the title and the question appear before any key.
  3. The hero screen is drawn with the Facit codes turned into VT ones, the 32
     heroes one can choose and no more, and the arrow keys and Home pick
     heroes (shown in reverse video).  Arrow keys struck once the cursor is
     on Finished, and at the prompts after it, do not break the game.
  4. Names are typed as they are spelt, in any case; the Swedish letters
     show (Éomer, and Theodr`d as Theodréd).
  5. A move, the map, and QUIT-GAME ("Chicken!").
  6. The instructions page by page (Magnus Domellöf in the credits); the map
     is kept; Esc during play is a SINTRAN user break, from whose @ prompt
     CONTINUE goes back into the game where it stood and LOGOUT leaves it.
  7. The endless fight of the program as recovered (build\\original): Esc
     still breaks it while it spins.

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
CR, ESC, BS = chr(13), chr(27), chr(127)
UP, DOWN, RIGHT, LEFT, HOME = '[A', '[B', '[C', '[D', '[H'
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'mordor.exe')
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


def clock(hour):
    return str(int(time.mktime(datetime.datetime(2026, 9, 17, hour, 0, 0).timetuple())))


def main():
    work = tempfile.mkdtemp(prefix='mordorcon')
    for f in ('MORDOR-MJ.PROG', 'MORDOR-RULES-MJ.DATA', 'PASCAL-ERR.SYMB'):
        shutil.copy(os.path.join(DATA, f), work)
    try:
        w = Window([EXE, '--data', work, '-Z', clock(12)], work)
        w.wait_for('You cannot play MORDOR now!')
        w.wait_for('[press any key]')
        w.type(' ')
        w.wait_exit()
        w.close()
        print('ok    locked at noon, window held open')

        w = Window([EXE, '--data', work, '-Z', clock(12), '--unlimited'], work)
        w.wait_for('Mordor: Land of evil.  v7.52 850211')
        w.wait_for('Do you want instructions (Y/N) <N>:')
        print('ok    --unlimited: title and question drawn before any key')
        w.type('N' + CR)
        w.wait_for("When finished: Place the cursor on 'Finished'.", timeout=120)
        w.wait_for('Finished.')
        time.sleep(0.5)
        with w.lock:
            raw = w.raw
        assert 'Denethor' in raw and not re.search('Hama|Galadriel|Grimbeorn', raw), \
            'the citadel people are on the hero screen'
        print('ok    hero screen drawn, the 32 heroes one can choose')
        for key in (HOME, RIGHT, HOME, RIGHT, HOME, RIGHT, HOME):
            w.type(key, gap=0.1)
            time.sleep(0.2)
        time.sleep(0.5)
        with w.lock:
            raw = w.raw
        for name in ('Frodo', 'Sam', 'Merry', 'Pippin'):
            assert re.search(r'\[7m' + name, raw), 'no reverse video for ' + name
        print('ok    arrow keys and Home select, in reverse video')
        for i in range(7):
            w.type(DOWN, gap=0.1)
        # onto Finished, and the key struck twice more, as a held key would
        w.write(RIGHT)
        w.write(RIGHT)
        w.write(DOWN)
        w.wait_for('Who shall be the Ringbearer:')
        for key in (UP, LEFT, HOME):
            w.write(key)
            time.sleep(0.3)
        time.sleep(1)
        with w.lock:
            raw = w.raw
        assert not w.exited() and 'USER BREAK' not in raw, 'an arrow key broke the game'
        w.type('frodo' + CR)
        w.wait_for('And who shall carry (and use) the Palantir:')
        w.type('Sam' + CR)
        w.wait_for('Command:')
        print('ok    cursor to Finished; arrow keys after it do not break; names as spelt, in any case')
        w.type('REPORX' + BS + 'T' + CR)          # Backspace rubs the X out
        w.wait_for('Éomer, Theodréd')
        with w.lock:
            raw = w.raw
        assert 'REPORX\b \bT' in raw, 'Backspace did not rub the character out'
        print('ok    Swedish letters: Éomer, Theodréd (sic); Backspace rubs out')
        w.type('NE2' + CR)
        w.wait_for('Command:')
        w.type('MAP' + CR)
        w.wait_for('Command:')
        w.type('QUIT-GAME' + CR)
        w.wait_for('Chicken!')
        w.wait_for('[press any key]')
        w.type(' ')
        w.wait_exit()
        w.close()
        print('ok    move, map, QUIT-GAME')

        w = Window([EXE, '--data', work, '-u'], work)
        w.wait_for('<N>:')
        w.type('y' + CR)
        rules = bytes(b & 0x7f for b in open(os.path.join(DATA, 'MORDOR-RULES-MJ.DATA'), 'rb').read())
        pages = sum(1 for line in rules.split(b'\r\n') if line.startswith(b'*'))
        for page in range(pages):
            if page == 1:
                w.wait_for('This game simulates the struggle between the Good')
            if page == pages - 1:
                w.wait_for('Magnus Domellöf')
            w.wait_for("Press 'Return' when you've finished reading:")
            w.type(CR)
        print('ok    the instructions, %d pages' % pages)
        w.wait_for('Finished.', timeout=120)
        for i in range(8):                       # 1, 5, ... 29, 33: Finished
            w.type(DOWN, gap=0.1)
        w.wait_for('Someone has got to take the Ring!')
        w.type(HOME + RIGHT + HOME + RIGHT + HOME + RIGHT + HOME, gap=0.15)
        for i in range(7):                       # 4, 8, ... 32
            w.type(DOWN, gap=0.1)
        w.type(RIGHT)                            # 33
        w.wait_for('Ringbearer:')
        w.type('FRODO' + CR)
        w.wait_for('Palantir:')
        w.type('sam' + CR)
        w.wait_for('Command:')
        w.type(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('CONTINUE' + CR)
        w.type('REPORT' + CR)
        w.wait_for('Frodo')                      # the game went on where it stood
        w.wait_for('Command:')
        w.type(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('NONSENSE' + CR)
        w.wait_for('NO SUCH FILE NAME')
        w.type('LOGOUT' + CR)
        w.wait_for('[press any key]')
        w.type(' ')
        code = w.wait_exit()
        assert code == 1, code
        w.close()
        print('ok    the map is kept; an empty Finished is refused; Esc breaks, CONTINUE goes on')

        # the recovered program's endless loop in a fight (NOTES.md; fixed in
        # data\): Esc still gets you out.  Its names are typed shifted.
        keys = open(os.path.join(HERE, 'hang-seed11.keys'), 'rb').read().decode('latin-1')
        for facit, vt in (('A', UP), ('B', DOWN), ('C', RIGHT), ('D', LEFT), ('H', HOME)):
            keys = keys.replace(facit, vt)
        original = os.path.join(HERE, '..', 'build', 'original', 'MORDOR-MJ.PROG')
        w = Window([EXE, '--data', work, '--prog', os.path.abspath(original), '-u', '--new-map',
                    '-Z', '1789668011'], work)
        w.wait_for('<N>:')
        first, rest = keys.split(CR, 1)
        w.type(first + CR)
        w.wait_for('Finished.', timeout=120)
        for token in re.findall(r'\[[A-H]|[^]', rest):
            w.write(token)
        w.wait_for('Damrod got captured by the villains!', timeout=120)
        time.sleep(3)
        assert not w.exited()
        w.type(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('LOGOUT' + CR)
        w.wait_for('[press any key]')
        w.type(' ')
        assert w.wait_exit() == 1
        w.close()
        print('ok    Esc breaks the game while it spins')
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
