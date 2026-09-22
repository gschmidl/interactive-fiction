"""The port at a real console, the way play.bat runs it.

    python consoleplay.py

The console is a Windows pseudo console (ConPTY): the program sees a real console and reads
real key events, and nothing appears on the desktop.  What the program draws is kept in a
screen model (common.Screen).

  1. One player: the name, YES, LOOK; Backspace ("LOOX", Backspace, "K"); Esc blanks the
     line at terminal 0 (without taking MFE's console); QUIT, YES; the status line says the
     game is over, and a key closes the window with exit 0.
  2. Ctrl+C in the middle of a game ends it at once (exit 0).
  3. Two windows: a game for two, and a second window started the same way joins it as
     terminal 1.  Each sees the other and hears what the other says; Ctrl+Enter (QUEST asks
     as for QUIT) logs the second player off and a key closes that window; QUIT ends the game
     in the first.
  4. play.bat, from a copy of the port in a scratch folder, plays.
"""
import ctypes
import ctypes.wintypes as W
import os
import shutil
import sys
import tempfile
import threading
import time

from common import EXE, PORT, Screen, squeeze

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
NET_PORT = 17411


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
        self.screen = Screen()
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = ctypes.create_string_buffer(65536)
        got = W.DWORD()
        while k32.ReadFile(self.out, buf, 65536, ctypes.byref(got), None) and got.value:
            with self.lock:
                self.screen.feed(buf.raw[:got.value].decode('utf-8', 'replace'))

    def type(self, keys, gap=0.05):
        for ch in keys:
            data = ch.encode('latin-1')
            n = W.DWORD()
            k32.WriteFile(self.inp, data, len(data), ctypes.byref(n), None)
            time.sleep(gap)

    def shown(self):
        with self.lock:
            return self.screen.text()

    def wait_for(self, what, timeout=90):
        """wait until WHAT is on the screen"""
        end = time.time() + timeout
        want = squeeze(what)
        while True:
            gone = self.exited()
            if want in squeeze(self.shown()):
                return
            if gone or time.time() > end:
                break
            time.sleep(0.05)
        raise AssertionError('%s: %r never appeared; the screen:\n%s' % (self.name, what,
                                                                         self.shown()))

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


def sign_on(w, name, male):
    w.wait_for('What is your first name?')
    w.type(name + '\r')
    w.wait_for('Are you male?')
    w.type(('YES' if male else 'NO') + '\r')


def main():
    ok = True
    tmp = tempfile.mkdtemp(prefix='questp7c-')
    windows = []
    try:
        w = Window('one player', [EXE, '-u', '--port=%d' % NET_PORT])
        windows.append(w)
        sign_on(w, 'Heino', True)
        w.wait_for('You are ')
        w.type('LOOX\bK\r')
        w.wait_for(' LOOK')
        print('ok    one player signs on; Backspace deletes the last character typed')
        w.type('XYZ\x1b')
        time.sleep(1.5)
        w.type('INVENTORY\r')
        w.wait_for(' INVENTORY')
        w.wait_for('You have no possessions.')
        assert 'COMMAND' not in w.shown(), 'Esc took the screen to the MFE console'
        print('ok    Esc blanks the line at terminal 0')
        w.type('QUIT\r')
        w.wait_for('Do you really want to quit the game?')
        w.type('YES\r')
        w.wait_for('The game is over')
        time.sleep(0.5)
        assert not w.exited(), 'the window closed before a key was pressed'
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    QUIT ends the game; a key closes the window (exit 0)')

        w = Window('Ctrl+C', [EXE, '-u', '--port=%d' % NET_PORT])
        windows.append(w)
        sign_on(w, 'Heino', True)
        w.wait_for('You are ')
        w.type('\x03')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    Ctrl+C ends the game at once (exit 0)')

        a = Window('the first window', [EXE, '-u', '--players=2', '--port=%d' % NET_PORT])
        windows.append(a)
        a.wait_for('QUEST for 2 players')
        b = Window('the second window', [EXE, '-u', '--port=%d' % NET_PORT])
        windows.append(b)
        sign_on(a, 'Heino', True)
        sign_on(b, 'Anna', False)
        a.wait_for('Anna is here')
        b.wait_for('Heino is here')
        print('ok    a second window joins the game for two; the players see each other')
        b.type('"HELLO THERE\r')
        a.wait_for('HELLO THERE')
        print('ok    what one player says the other hears')
        b.type('\x0a')                  # Ctrl+Enter, as a terminal sends it
        b.wait_for('Do you really want to quit the game?')
        b.type('YES\r')
        b.wait_for('You have left QUEST')
        b.type(' ')
        code = b.wait_exit()
        assert code == 0, 'second window: exit %d' % code
        print('ok    Ctrl+Enter logs the second player off; a key closes that window')
        a.type('QUIT\r')
        a.wait_for('Do you really want to quit the game?')
        a.type('YES\r')
        a.wait_for('The game is over')
        a.type(' ')
        code = a.wait_exit()
        assert code == 0, 'first window: exit %d' % code
        print('ok    QUIT in the first window ends the game')

        copy = os.path.join(tmp, 'port copy')
        os.mkdir(copy)
        for f in ('play.bat', 'quest.exe', 'p7000.pack'):
            shutil.copy(os.path.join(PORT, f), copy)
        w = Window('play.bat', [os.environ.get('COMSPEC', 'cmd.exe'), '/c',
                                os.path.join(copy, 'play.bat'), '--port=%d' % NET_PORT])
        windows.append(w)
        sign_on(w, 'Heino', True)
        w.wait_for('You are ')
        w.type('QUIT\r')
        w.wait_for('Do you really want to quit the game?')
        w.type('YES\r')
        w.wait_for('The game is over')
        w.type(' ')
        code = w.wait_exit()
        assert code == 0, 'exit %d' % code
        print('ok    play.bat plays')
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
