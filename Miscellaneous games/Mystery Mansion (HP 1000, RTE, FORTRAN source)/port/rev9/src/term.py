"""Play Mystery Mansion revision 9 on the site's own RTE-IVB system.

Starts SIMH's hp2100 (in a minimised console window of its own, since SIMH
will not run on a pipe) on the disc image made by mkdisc.py, and is the HP
2645 terminal on its BACI port: answers the driver's status requests, turns
the 2645's display escapes into ANSI ones, sets the clock as the operator did
after the boot's SET TIME, types TR,GOMMM (the site's own procedure) at the
FMGR prompt, and hands the game to the player.  When the game is over and
FMGR prompts again, the machine is shut down."""
import ctypes
import msvcrt
import os
import queue
import shutil
import socket
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.normpath(os.path.join(HERE, '..', '.build'))
STATUS = b'\x1b\\0000000\r'           # 2645 status: nothing to report

USAGE = """Usage: play [OPTION]...
Mystery Mansion revision 9, the compiled program on the site's backup tape,
run on the site's own HP 1000 RTE-IVB system in SIMH.

      --fixed-clock  leave the system clock at its boot value (1978, day 217,
                     08:00) instead of setting it to this computer's time:
                     the same mansion every time
      --show-boot    show the boot and FMGR as the terminal saw them, and
                     leave FMGR to you after the game (Ctrl+C ends it)
  -h, --help         display this help and exit

The disc is .build\\disc.img, made from the tape's disc on first use, and
keeps suspended games; delete it to start again from the tape."""


def options(argv):
    opt = {'show_boot': False, 'fixed_clock': False}
    for a in argv:
        if a in ('-h', '--help'):
            print(USAGE)
            sys.exit(0)
        elif a in ('--show-boot', '--fixed-clock'):
            opt[a[2:].replace('-', '_')] = True
        else:
            sys.stderr.write("play: unrecognized option '%s'\nTry 'play --help' for more information.\n" % a)
            sys.exit(2)
    return opt


def ansi_console():
    k32 = ctypes.windll.kernel32
    h = k32.GetStdHandle(-11)
    mode = ctypes.c_uint32()
    if k32.GetConsoleMode(h, ctypes.byref(mode)):
        k32.SetConsoleMode(h, mode.value | 4)              # ENABLE_VIRTUAL_TERMINAL_PROCESSING


class BasicLimits(ctypes.Structure):
    _fields_ = [('PerProcessUserTimeLimit', ctypes.c_int64), ('PerJobUserTimeLimit', ctypes.c_int64),
                ('LimitFlags', ctypes.c_uint32), ('MinimumWorkingSetSize', ctypes.c_size_t),
                ('MaximumWorkingSetSize', ctypes.c_size_t), ('ActiveProcessLimit', ctypes.c_uint32),
                ('Affinity', ctypes.c_size_t), ('PriorityClass', ctypes.c_uint32),
                ('SchedulingClass', ctypes.c_uint32)]


class ExtendedLimits(ctypes.Structure):
    _fields_ = [('Basic', BasicLimits), ('Io', ctypes.c_uint64 * 6), ('ProcessMemoryLimit', ctypes.c_size_t),
                ('JobMemoryLimit', ctypes.c_size_t), ('PeakProcessMemoryUsed', ctypes.c_size_t),
                ('PeakJobMemoryUsed', ctypes.c_size_t)]


def tie_to_us(proc):
    """Put the simulator in a job that dies with this process, so closing the
    window cannot leave it running on the disc."""
    k32 = ctypes.windll.kernel32
    k32.CreateJobObjectW.restype = ctypes.c_void_p
    job = k32.CreateJobObjectW(None, None)
    info = ExtendedLimits()
    info.Basic.LimitFlags = 0x2000                         # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
    k32.SetInformationJobObject(ctypes.c_void_p(job), 9, ctypes.byref(info), ctypes.sizeof(info))
    k32.AssignProcessToJobObject(ctypes.c_void_p(job), ctypes.c_void_p(int(proc._handle)))
    return job


class Terminal:
    """The 2645 at the far end of the BACI's telnet port."""

    def __init__(self, sock, show):
        self.s = sock
        self.show = show
        self.mute_at_end = not show
        self.in_game = False
        self.lock = threading.Condition()
        self.reads = 0              # DC1s that ask for a line
        self.status_due = False
        self.line = ''              # text of the current output line
        self.prompt_line = None     # the line a read was asked for on
        self.text = ''              # everything printed, for the start-up checks
        self.echo = ''              # our last line, which the driver echoes
        self.closed = False
        self.esc = None
        threading.Thread(target=self.reader, daemon=True).start()

    def out(self, t):
        if self.show:
            sys.stdout.write(t)
            sys.stdout.flush()

    def reader(self):
        iac = 0
        while True:
            try:
                b = self.s.recv(4096)
            except OSError:
                b = b''
            if not b:
                with self.lock:
                    self.closed = True
                    self.lock.notify_all()
                return
            for c in b:                                    # Telnet: IAC cmd [option]
                if iac == 1:
                    iac = 2 if 251 <= c <= 254 else 0
                elif iac == 2:
                    iac = 0
                elif c == 255:
                    iac = 1
                else:
                    self.byte(c & 127)

    def byte(self, c):
        if self.esc is not None:
            self.escape(c)
            return
        if c == 27:
            self.esc = ''
            return
        with self.lock:
            if c == 17:                                    # DC1: the terminal may send
                if self.status_due:
                    self.status_due = False
                    self.s.sendall(STATUS)
                else:
                    self.reads += 1
                    self.prompt_line = self.line
                    self.lock.notify_all()
                return
            ch = chr(c)
            if self.echo:
                if ch == self.echo[0]:
                    self.echo = self.echo[1:]
                    return
                self.echo = ''
            self.text += ch
            if ch == '\n':
                if 'WELCOME TO MYSTERY MANSION' in self.line:
                    self.in_game = True
                elif (self.in_game and self.mute_at_end and 'MMM' in self.line
                      and ('ABORTED' in self.line or ': STOP' in self.line)):
                    self.show = False                      # not GOMMM's softkey menu
                self.line = ''
            elif ch != '\r':
                self.line += ch
        if c in (7, 10) or c >= 32:                        # the driver ends lines CR LF
            self.out(chr(c))

    def escape(self, c):
        """2645 escapes: ESC x, or ESC & class {digits letter} ... CAPITAL,
        with ESC &f..L followed by that many characters of key text."""
        e = self.esc + chr(c)
        if e == '^' or e == '~':                           # status request
            self.esc = None
            with self.lock:
                self.status_due = True
            return
        if not e.startswith('&'):
            self.esc = None
            self.out({'H': '\x1b[H', 'h': '\x1b[H', 'J': '\x1b[J', 'K': '\x1b[K',
                      'A': '\x1b[A', 'B': '\x1b[B', 'C': '\x1b[C', 'D': '\x1b[D'}.get(e, ''))
            return
        if e.startswith('&f') and 'L' in e:               # softkey text follows
            n = int(''.join(ch for ch in e[e.rfind('k') + 1:e.index('L')] if ch.isdigit()) or 0)
            if len(e) - e.index('L') - 1 >= n:
                self.esc = None
            else:
                self.esc = e
            return
        if len(e) > 2 and ('A' <= chr(c) <= 'Z' or chr(c) == '@'):
            self.esc = None
            if e[1] == 'a':                                # cursor address
                r = col = 0
                num = ''
                for ch in e[2:]:
                    if ch.isdigit():
                        num += ch
                    elif ch in 'rR':
                        r = int(num or 0)
                        num = ''
                    elif ch in 'cC':
                        col = int(num or 0)
                        num = ''
                self.out('\x1b[%d;%dH' % (r + 1, col + 1))
            elif e[1] == 'd':                              # display enhancement
                m = ord(e[-1]) - 64
                sgr = ['0'] + (['7'] if m & 2 else []) + (['4'] if m & 4 else []) + (['2'] if m & 8 else [])
                self.out('\x1b[%sm' % ';'.join(sgr))
            return
        self.esc = e
        if len(e) > 40:
            self.esc = None

    def wait_read(self, timeout):
        end = time.time() + timeout
        with self.lock:
            while not self.reads and not self.closed and time.time() < end:
                self.lock.wait(0.2)
            return self.reads > 0

    def send(self, line):
        with self.lock:
            self.reads -= 1
            self.echo = line + '\r\n'
            self.prompt_line = None
        self.s.sendall(line.encode('latin-1') + b'\r')


def main():
    opt = options(sys.argv[1:])
    show_boot = opt['show_boot']
    exe = os.path.join(BUILD, 'hp2100.exe')
    if not os.path.exists(exe) or not os.path.exists(os.path.join(BUILD, 'disc0.img')):
        sys.exit('play: run build.bat first')
    lock = open(os.path.join(BUILD, 'play.lock'), 'w')
    try:
        msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
    except OSError:
        sys.exit('play: already running - one machine, one disc')
    if not os.path.exists(os.path.join(BUILD, 'disc.img')):
        shutil.copyfile(os.path.join(BUILD, 'disc0.img'), os.path.join(BUILD, 'disc.img'))
    ansi_console()
    probe = socket.socket()
    probe.bind(('127.0.0.1', 0))
    port = probe.getsockname()[1]
    probe.close()
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    si.wShowWindow = 0                                     # SW_HIDE
    sim = subprocess.Popen([exe, 'rte.sim', str(port)], cwd=BUILD,
                           creationflags=subprocess.CREATE_NEW_CONSOLE, startupinfo=si)
    job = tie_to_us(sim)                                   # noqa: F841 (held open)
    try:
        s = None
        end = time.time() + 30
        while s is None and time.time() < end:
            try:
                s = socket.create_connection(('127.0.0.1', port), timeout=2)
            except OSError:
                time.sleep(0.1)
        if s is None:
            sys.exit('play: the simulator did not start')
        s.settimeout(None)
        term = Terminal(s, show_boot)
        if not show_boot:
            print('Booting the RTE-IVB system ...')
        # after the boot, WELCOM leaves FMGR prompting on the terminal
        end = time.time() + 120
        while time.time() < end and 'LAST BACKUP' not in term.text:
            term.wait_read(0.5)
        if not term.wait_read(60):
            sys.exit('play: the system did not come up')
        if not opt['fixed_clock']:
            # the operator's answer to SET TIME, through FMGR's SY command
            t = time.localtime()
            term.send('SYTM,%d,%d,%d,%d,%d' % (t.tm_year, t.tm_yday, t.tm_hour, t.tm_min, t.tm_sec))
            if not term.wait_read(30):
                sys.exit('play: the system did not take the time')
        term.send('TR,GOMMM')
        term.show = True
        lines = queue.Queue()

        def keyboard():
            for ln in sys.stdin:
                lines.put(ln.rstrip('\r\n').upper()[:72])
            lines.put(None)
        threading.Thread(target=keyboard, daemon=True).start()
        while True:
            if not term.wait_read(0.5):
                if term.closed:
                    break
                continue
            if term.in_game and not show_boot and (term.prompt_line or '').strip() == ':':
                break                                      # the game is over: FMGR again
            ln = False
            while ln is False and not term.closed:
                try:
                    ln = lines.get(timeout=0.5)            # Ctrl+C gets through
                except queue.Empty:
                    pass
            if ln is None or ln is False:
                break
            term.send(ln)
    except KeyboardInterrupt:
        pass
    finally:
        sim.kill()
        sim.wait()
        sys.stdout.write('\x1b[0m\n')


if __name__ == '__main__':
    main()
