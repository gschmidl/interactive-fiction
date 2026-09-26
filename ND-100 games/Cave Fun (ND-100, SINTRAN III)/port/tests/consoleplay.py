"""Cave Fun at a real console, the way it is played.

    python tests/consoleplay.py [path/to/cavefun.exe]

Runs the port in a Windows pseudo console (ConPTY), so it reads real key
events and writes to a real console, with nothing on the desktop:

  1. The title; the adventure's name typed in small letters, abbreviated: it
     shows as typed, and the program gets capitals (the terminal is in
     capital-letter mode), so the game starts.
  2. The game: a move, GET LAMP, and Backspace rubbing out a letter.
  3. Esc: SINTRAN's user break and its @, where CONTINUE goes back in.
  4. SAVE with a new name typed without quotes (the port takes it), the file
     written; END, and the window held open with [press any key].
  5. The editor (--editor): its title, and EXIT.

Everything runs on a throwaway copy of the game file.
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
CR, ESC, BS = chr(13), chr(27), chr(8)
UP, DOWN, RIGHT, LEFT, HOME = '[A', '[B', '[C', '[D', '[H'
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'cavefun.exe')
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




def fresh():
    work = tempfile.mkdtemp(prefix='cavefuncon')
    shutil.copy(os.path.join(DATA, 'CAVE-FUN-MJ.ADV'), work)
    for f in ('ADV-INTER-CB-MJ.PROG', 'ADV-EDIT-CB-MJ.PROG'):
        shutil.copy(os.path.join(DATA, f), work)
    return work


def main():
    work = fresh()
    try:
        w = Window([EXE, '--data', work, '--no-hold'], work)
        w.wait_for('- ADVENTURE V4.2 -', timeout=60)
        w.wait_for('GIVE THE NAME OF THE ADVENTURE YOU WANT TO PLAY:')
        w.type('cave' + CR)
        w.wait_for('cave')
        w.wait_for("YOU'RE IN A HOUSE IN A FOREST")
        w.wait_for('OBVIOUS EXITS ARE: NORTH EAST SE SW WEST')
        print('ok    title; the adventure named in small letters, abbreviated')
        w.type('n' + CR)
        w.wait_for('OLD BRASS LAMP')
        w.type('get lamx', gap=0.05)
        w.write(BS)
        w.type('p' + CR, gap=0.05)
        w.wait_for('OLD BRASS LAMP: TAKEN')
        print('ok    a move, and Backspace rubs out a character')
        w.write(ESC)
        w.wait_for('USER BREAK AT')
        w.wait_for('@')
        w.type('continue' + CR)
        w.type('get i' + CR)
        w.wait_for('YOU ARE CARRYING: OLD BRASS LAMP')
        print('ok    Esc breaks to SINTRAN, CONTINUE goes back into the game')
        w.type('save' + CR)
        w.wait_for('SAVE FILE NAME:')
        w.type('mygame' + CR)
        w.wait_for('SAVED!')
        assert os.path.exists(os.path.join(work, 'MYGAME.SYMB')), 'no MYGAME.SYMB'
        print('ok    SAVE with a new name, no quotes: MYGAME.SYMB')
        w.type('end' + CR)
        w.wait_for('SAVE GAME:')
        w.type('n' + CR)
        w.wait_for('HOPE YOU WILL JOIN ME SOON AGAIN!', timeout=30)
        w.wait_for('[press any key]', timeout=30)
        w.type(' ')
        w.wait_exit()
        w.close()
        print('ok    END, and the window held open')
        w = Window([EXE, '--editor', '--data', work, '--no-hold'], work)
        w.wait_for('- Adventure Editor V3.0 -', timeout=60)
        w.wait_for('Please, enter your command?')
        w.type('exit' + CR)
        w.wait_for('[press any key]', timeout=30)
        w.type(' ')
        w.wait_exit()
        w.close()
        print('ok    the editor')
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
