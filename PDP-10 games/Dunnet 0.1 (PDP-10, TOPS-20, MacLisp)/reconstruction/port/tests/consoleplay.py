#!/usr/bin/env python3
r"""The port at a real console, as run.bat runs it.

    python tests\\consoleplay.py

A Windows pseudo console (ConPTY) runs dunnet.exe: the program sees a real
console and reads real key events, and nothing appears on the desktop.
What a pipe cannot show:

  1. the prompt ">" is on the screen before the game waits for a command;
  2. mixed case is played, and DEL edits the line;
  3. at the TOPS-20 "@": ? lists the commands, ESC completes a keyword and
     shows its guide word, DEL back into a parsed field reparses, a bad
     keyword gives the error and a new prompt;
  4. TELNET to the dungeon, and "~c" closes the connection;
  5. QUIT: the score, and the program ends, exit 0;
  6. Ctrl-Z at the prompt ends the program, exit 0.
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
            if ch == '':
                # a lone ESC byte waits in the pseudo console until the next
                # byte shows whether it starts a sequence; send the ESC key
                # itself (win32-input-mode: Vk;Sc;Uc;Kd;Cs;Rc_, down + up)
                data = b'[27;1;27;1;0;1_[27;1;27;0;0;1_'
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




def main():
    exe = os.path.join(PORT, 'dunnet.exe')
    failed = []

    def step(name, fn):
        try:
            fn()
            print('%-58s ok' % name)
        except AssertionError as e:
            print('%-58s FAILED\n    %s' % (name, str(e).replace('\n', '\n    ')))
            failed.append(name)

    w = Window([exe], PORT)
    try:
        step('1 prompt shown before the first command',
             lambda: w.waiting_after('an unknown animal. There is a battery-powered lamp here. >'))

        def mixed_case():
            w.type('Take Lamp\r')
            w.wait_for('Taken.')
            w.type('nx\x08w\r')               # DEL corrects nx to nw
            w.waiting_after('midway down the northwest passage. The path continues to the northwest and southeast. >')
        step('2 mixed case, DEL edits the line', mixed_case)

        def to_console():
            for c, seen in (('nw', 'special phrase'), ('a special phrase', 'stops.'),
                            ('se', 'Nw passage'), ('se', 'Fork'), ('s', 'You have entered a wall!'),
                            ('take chaos', 'Taken.'), ('n', 'Fork'), ('ne', 'continues northeast and southwest.'),
                            ('ne', 'electric fence'), ('n', 'command CONSOLE.')):
                w.type(c + '\r')
                w.wait_for(seen)
            w.type('console\r')
            w.waiting_after('[Command RETURN defined] @')
        step('  (walk to the console)', to_console)

        def comnd():
            w.type('?')
            w.wait_for('? Command, one of the following:')
            w.waiting_after('TELNET TN @')
            w.type('inf\x1b')
            w.waiting_after('infORMATION (About) ')
            w.type('\x08' * 8)                # the guide word goes whole, then
            w.type('\x1b')                    # back into INFORMATION: reparse
            w.waiting_after('ATION (About) ')
            w.type('chaos\r')
            w.waiting_after('The imp is up @')
            w.type('foo\r')
            w.waiting_after('?Unrecognized command - does not match switch or keyword @')
        step('3 ? help, ESC + guide word, DEL reparse, error', comnd)

        def telnet():
            w.type('tel\x1b\r')
            w.waiting_after('TELNET>')
            w.type('con\x1bch\x1bdu\x1b\r')
            w.wait_for('conNECT (Network) chAOSNET (host) duNGEON')
            w.wait_for('Trying...open')
            w.waiting_after('grues here are friendly. >')
            w.type('light lamp\r')
            w.wait_for('only a stairway in it. The stairway leads up. >')
            w.type('~c')
            w.wait_for('Connection closed. Some force throws you off the console')
            w.type('\rlook\r')
            w.waiting_after('command CONSOLE. >')
        step('4 TELNET to the dungeon, ~c leaves it', telnet)

        def quit_():
            w.type('quit\r')
            w.wait_for('You have scored 0 points out of a possible 10 using')
            code = w.wait_exit()
            if code != 0:
                raise AssertionError('exit code %d' % code)
        step('5 QUIT: score, exit 0', quit_)
    finally:
        w.close()

    w = Window([exe], PORT)
    try:
        def ctrl_z():
            w.waiting_after('>')
            w.type('\x1a')
            code = w.wait_exit(10)
            if code != 0:
                raise AssertionError('exit code %d' % code)
        step('6 Ctrl-Z ends the program, exit 0', ctrl_z)
    finally:
        w.close()

    print('all passed' if not failed else '%d FAILED' % len(failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
