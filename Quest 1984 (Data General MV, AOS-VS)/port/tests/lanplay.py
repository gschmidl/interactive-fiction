"""A world open to the network (--lan), and players who join it three ways (Windows only).

    python tests/lanplay.py            (from the port directory, or anywhere)

  1. The world (--server --lan) says it is open to this network and that players join
     with "quest --join" and this computer's name.  A second world on the port, with or
     without --lan, is refused.
  2. ALICE connects by that name, which resolves to IPv6 addresses first (the world
     listens for IPv6 as well as IPv4), and makes her character.
  3. CAROL connects from WSL2's virtual machine - another computer as far as the network
     goes - over IPv4, and makes hers.  Without WSL (and python3 in it) she is left out.
  4. Both leave; the world stops by itself, named ALICE's IPv6 address and CAROL's IPv4
     one as they joined, and USER_DATA_FILE holds both.
  5. In a second world ALICE comes back with the program's own terminal, aosvs32 --join
     [::1]:PORT, fed from a file.

CAROL needs the Windows firewall to let aosvs32.exe take connections from the WSL network:
Windows asks the first time a world listens with --lan.  data/ is only read.
"""
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

from netplay import (EXE, Player, answer, characters, create, free_port, leave, start_world,
                     stream_text, wait_listening)

TESTS = os.path.dirname(os.path.abspath(__file__))
NAME = socket.gethostname()

# CAROL, in WSL: python3 -c CAROL TESTS-DIR PORT
CAROL = r'''
import socket, subprocess, sys, time
sys.path.insert(0, sys.argv[1])
from netplay import Player, create, leave
host = subprocess.check_output(['ip', 'route', 'show', 'default']).split()[2].decode()
for tries in range(4):
    try:
        p = Player('CAROL', int(sys.argv[2]), host)
        break
    except OSError as e:
        print('CAROL: no connection to %s yet (%s)' % (host, e), flush=True)
else:
    print('CAROL: could not connect - does the firewall let aosvs32.exe in?', flush=True)
    sys.exit(1)
print('CAROL: connected from %s:%d to %s:%d' % (p.s.getsockname() + p.s.getpeername()),
      flush=True)
p.settle(lambda q: q.shows('What are your initials'), changed=False)
p.send('CA\r')
p.settle(lambda q: q.shows('Player name ?'))
create(p, 'CAROL', 'CASTLE')
leave(p)
p.wait_closed()
print('CAROL: made her character and left', flush=True)
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
    save = tempfile.mkdtemp(prefix='questlan')
    port = free_port()
    ok = True
    world = None
    with_carol = wsl_ok()
    try:
        # -- 1: the world tells how to join, and keeps its port -------------------
        world = start_world(save, port, '--lan')
        wait_listening(port, world)
        how = world.stdout.readline().decode('latin-1').strip()
        assert how == 'Players join with: quest --join %s' % NAME, how
        print('ok    the world says: %s' % how)
        for more in ([], ['--lan']):
            other = start_world(tempfile.mkdtemp(prefix='questlan2'), port, *more)
            out = other.communicate(timeout=60)[0].decode('latin-1')
            assert other.returncode == 1 and 'already running on port %d' % port in out, out
        print('ok    a second world on the port, with or without --lan, is refused')

        # -- 2..4: ALICE by name (IPv6), CAROL from WSL (IPv4) -----------------------
        a = Player('ALICE', port, NAME)
        family = 'IPv6' if a.s.family == socket.AF_INET6 else 'IPv4'
        a.settle(lambda q: q.shows('What are your initials'), changed=False)
        a.send('AL\r')
        a.settle(lambda q: q.shows('Player name ?'))
        create(a, 'ALICE', 'RABBIT')
        print('ok    ALICE joined by the name %s (%s, %s) and made her character'
              % (NAME, family, a.s.getpeername()[0]))
        if with_carol:
            carol = subprocess.run(['wsl.exe', '-e', 'python3', '-c', CAROL, wsl_path(TESTS),
                                    str(port)], capture_output=True, text=True, timeout=600)
            out = carol.stdout.strip()
            assert carol.returncode == 0, 'CAROL:\n' + out + carol.stderr[-800:]
            print('      ' + out.replace('\n', '\n      '))
            print('ok    CAROL joined from WSL over IPv4 and made her character')
        else:
            print('--    no WSL with python3: CAROL is left out')
        answer(a)
        leave(a)
        a.wait_closed()
        world.wait(timeout=60)
        log = world.stdout.read().decode('latin-1')
        joined = [l.split('joined from ', 1)[1].strip() for l in log.splitlines()
                  if 'joined from' in l]
        assert len(joined) == (2 if with_carol else 1), log
        assert family == 'IPv4' or ':' in joined[0], 'ALICE joined from %s' % joined[0]
        if with_carol:
            assert joined[1].count('.') == 3 and ':' not in joined[1], \
                'CAROL joined from %s' % joined[1]
        names = characters(save)
        assert 'ALICE' in names and ('CAROL' in names or not with_carol), names
        print('ok    the world named them as they joined (%s) and stopped; saved: %s'
              % (', '.join(joined), ', '.join(names)))

        # -- 5: back again, through the join terminal, by an IPv6 address ------------
        port = free_port()
        world = start_world(save, port, '--lan')
        wait_listening(port, world)
        keys = os.path.join(save, 'keys.txt')
        with open(keys, 'wb') as f:
            f.write(b'AL\rALICE\rRABBIT\r ')
        with open(keys, 'rb') as kin:
            join = subprocess.run([EXE, '--join', '[::1]:%d' % port], stdin=kin,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  timeout=120)
        shown = stream_text(join.stdout)
        assert 'Strength' in shown and 'Your character is saved' in shown, shown[-400:]
        world.wait(timeout=60)
        log = world.stdout.read().decode('latin-1')
        assert 'joined from ::1' in log, log
        print('ok    ALICE came back with --join [::1]:%d' % port)
    except AssertionError as e:
        print('FAIL ', e)
        ok = False
    finally:
        if world is not None and world.poll() is None:
            world.kill()
        shutil.rmtree(save, ignore_errors=True)
    if ok:
        print('all passed')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
