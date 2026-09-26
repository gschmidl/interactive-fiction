"""A --lan game for four, each player joining another way (Windows only):

    python lanplay.py

  1. The window that runs the game (-u --players=4 --lan) says what the others run:
     quest --join=THIS-COMPUTER'S-NAME.  A second game on the port, with or without --lan,
     finds it taken.
  2. Anna joins by this computer's name (which resolves to IPv6 addresses first: the game
     listens for IPv6 as well as IPv4), Bo by an IPv6 address ([::1]:PORT), and Tux from
     WSL2's virtual machine, another computer as far as the network goes, over IPv4 to the
     address of WSL's gateway.  Without WSL (and python3 in it) Tux is left out.
  3. They all see each other, and what Anna says the others hear.
  4. Tux's connection drops, Bo's window closes, Anna logs off with Ctrl+Enter, and the
     first window's QUIT ends the game (exit 0).

It needs the Windows firewall to let quest.exe take connections on the WSL network (Windows
asks the first time a game listens with --lan).  The real test is a second computer:
quest --join=NAME there.
"""
import socket
import subprocess
import sys

from common import EXE, TESTS
from consoleplay import Window, sign_on

NET_PORT = 17416
NAME = socket.gethostname()

# Tux, in WSL: python3 -c TUX TESTS-DIR PORT
TUX = r'''
import subprocess, sys
sys.path.insert(0, sys.argv[1])
from common import Player
host = subprocess.check_output(['ip', 'route', 'show', 'default']).split()[2].decode()
try:
    p = Player('Tux', int(sys.argv[2]), host=host, timeout=60)
except OSError as e:
    print('Tux: cannot connect to %s: %s' % (host, e), flush=True)
    sys.exit(1)
print('Tux: connected from %s:%d to %s:%d' % (p.sock.getsockname() + p.sock.getpeername()),
      flush=True)
p.wait_for('What is your first name?', timeout=120)
p.type('Tux\r')
p.wait_for('Are you male?')
p.type('yes\r')
p.wait_for('Heino', timeout=300)
p.wait_for('HELLO FROM ANNA', timeout=300)
print('Tux: heard Anna', flush=True)
p.close()
'''


def wsl_ok():
    try:
        r = subprocess.run(['wsl.exe', '-e', 'python3', '-c', 'print(42)'], capture_output=True,
                           text=True, timeout=60)
        return r.stdout.strip() == '42'
    except (OSError, subprocess.TimeoutExpired):
        return False


def wsl_path(p):
    return '/mnt/' + p[0].lower() + p[2:].replace('\\', '/')


def main():
    windows = []
    tux = None
    with_tux = wsl_ok()
    players = ['Heino', 'Anna', 'Bo'] + (['Tux'] if with_tux else [])
    if not with_tux:
        print('--    no WSL with python3: Tux is left out, and the game is for three')
    try:
        host = Window('the first window', [EXE, '-u', '--players=%d' % len(players), '--lan',
                                           '--port=%d' % NET_PORT])
        windows.append(host)
        host.wait_for('players here: the others run quest --join=%s' % NAME, timeout=120)
        print('ok    the first window says: the others run quest --join=%s' % NAME)
        for extra in ([], ['--lan']):
            r = subprocess.run([EXE, '--players=2', '--port=%d' % NET_PORT] + extra,
                               stdin=subprocess.DEVNULL, capture_output=True, text=True,
                               timeout=60)
            assert r.returncode == 1 and 'a game is running on port %d' % NET_PORT in r.stderr, \
                'a second game%s: exit %d, %r' % (' --lan' if extra else '', r.returncode,
                                                  r.stderr)
        print('ok    a second game on the port, with or without --lan, finds it taken')
        anna = Window('Anna', [EXE, '--join=%s:%d' % (NAME, NET_PORT)])
        windows.append(anna)
        bo = Window('Bo', [EXE, '--join=[::1]:%d' % NET_PORT])
        windows.append(bo)
        if with_tux:
            tux = subprocess.Popen(['wsl.exe', '-e', 'python3', '-c', TUX, wsl_path(TESTS),
                                    str(NET_PORT)], stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True)
        sign_on(anna, 'Anna', False)
        sign_on(bo, 'Bo', True)
        sign_on(host, 'Heino', True)
        for w, me in ((host, 'Heino'), (anna, 'Anna'), (bo, 'Bo')):
            w.wait_for('You are ', timeout=180)
            for other in players:
                if other != me:
                    w.wait_for(other, timeout=60)
        print('ok    the game begins with %s; they see each other' % ', '.join(players))
        anna.type('"HELLO FROM ANNA\r')
        host.wait_for('HELLO FROM ANNA')
        bo.wait_for('HELLO FROM ANNA')
        if tux:
            out, _ = tux.communicate(timeout=300)
            assert 'Tux: heard Anna' in out, 'Tux:\n' + out
            print('      ' + out.strip().splitlines()[0])
        print('ok    what Anna (by name) says %s hear' % ', '.join(
            p for p in players if p != 'Anna'))
        if tux:
            host.wait_for('Tux quits.', timeout=60)
            print('ok    Tux\'s connection drops: the port logs Tux off')
        bo.close()
        host.wait_for('Bo quits.', timeout=60)
        print('ok    Bo\'s window closes: the port logs Bo off')
        anna.type('\x0a')               # Ctrl+Enter, as a terminal sends it
        anna.wait_for('Do you really want to quit the game?')
        anna.type('YES\r')
        anna.wait_for('You have left QUEST')
        anna.type(' ')
        code = anna.wait_exit()
        assert code == 0, 'Anna\'s window: exit %d' % code
        host.wait_for('Anna quits.', timeout=60)
        print('ok    Anna logs off with Ctrl+Enter; her window ends with exit 0')
        host.type('QUIT\r')
        host.wait_for('Do you really want to quit the game?')
        host.type('YES\r')
        host.wait_for('The game is over')
        host.type(' ')
        code = host.wait_exit()
        assert code == 0, 'the first window: exit %d' % code
        print('ok    QUIT in the first window ends the game (exit 0)')
        print('all passed')
        return 0
    except AssertionError as e:
        print('FAIL  %s' % e)
        return 1
    finally:
        for w in windows:
            w.close()
        if tux and tux.poll() is None:
            tux.kill()


if __name__ == '__main__':
    sys.exit(main())
