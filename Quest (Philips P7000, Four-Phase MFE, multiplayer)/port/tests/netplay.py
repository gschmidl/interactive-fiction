"""Players over TCP: the window that runs the game, scripted, and players joining it.

    python netplay.py

The first player is the port in transcript mode (terminal 0, lines from a pipe); the others
connect over TCP as the program's --join does (common.Player: keys out, the ANSI screen back
into a screen model).  The clock is counted in instructions, so the game runs fast; what
happens when is not repeatable, only what is checked here.

  1. A game for three.  Anna joins as terminal 1 and signs on with telnet's CR LF line ends;
     a fourth connection meanwhile finds all terminals taken and is refused.  Bo joins as
     terminal 2.  The game begins when the first player has signed on too, and everyone
     sees everyone.
  2. What Anna says the first player hears.
  3. Bo's window closes: the port logs Bo off (QUEST asks, and the port says YES), and the
     others read "Bo quits.".
  4. Anna logs off with Control CURSOR RETURN and YES: her connection closes, with her
     screen's last word "You have left QUEST".

    python netplay.py --show     also prints the first player's transcript
  5. The first player's script ends: the game ends (exit 0).
"""
import os
import subprocess
import sys
import tempfile
import threading
import time

from common import EXE, Player

NET_PORT = 17412


class Host:
    """the port with terminal 0 scripted"""

    def __init__(self, args):
        env = dict(os.environ, QUEST_NETLOG='1')
        self.p = subprocess.Popen([EXE, '--fixed-clock'] + args, stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                  env=env)
        self.out = self.log = ''
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()
        threading.Thread(target=self._logger, daemon=True).start()

    def _reader(self):
        for line in self.p.stdout:
            with self.lock:
                self.out += line

    def _logger(self):
        for line in self.p.stderr:
            with self.lock:
                self.log += line

    def say(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()

    def wait_for(self, what, timeout=60):
        end = time.time() + timeout
        while time.time() < end:
            with self.lock:
                if what in self.out:
                    return
            if self.p.poll() is not None:
                break
            time.sleep(0.05)
        with self.lock:
            tail = self.out[-1500:]
        raise AssertionError('host: %r never appeared; the end of the transcript:\n%s'
                             % (what, tail))

    def end(self, timeout=60):
        self.p.stdin.close()
        try:
            return self.p.wait(timeout)
        except subprocess.TimeoutExpired:
            raise AssertionError('host: did not end')

    def kill(self):
        if self.p.poll() is None:
            self.p.kill()


def main():
    ok = True
    host = players = None
    try:
        host = Host(['--players=3', '--port=%d' % NET_PORT])
        anna = Player('Anna', NET_PORT)
        players = [anna]
        anna.wait_for('What is your first name?')
        anna.send('Anna\r\n')
        anna.wait_for('Are you male?')
        anna.send('no\r\n')
        anna.wait_for("We're just waiting for everyone else to sign on.")
        print('ok    Anna joins as terminal 1 and signs on (telnet line ends)')
        bo = Player('Bo', NET_PORT)
        players.append(bo)
        bo.wait_for('What is your first name?')
        extra = Player('a fourth', NET_PORT)
        players.append(extra)
        extra.wait_for('this game is for 3 players')
        extra.wait_closed()
        print('ok    a fourth connection is refused: all terminals are taken')
        bo.type('Bo\r')
        bo.wait_for('Are you male?')
        bo.type('yes\r')
        bo.wait_for("We're just waiting for everyone else to sign on.")
        host.say('Heino')
        host.wait_for('Are you male?')
        host.say('yes')
        host.wait_for('Anna')
        host.wait_for('Bo')
        anna.wait_for('Heino')
        bo.wait_for('Anna')
        print('ok    the game begins with all three signed on; they see each other')
        anna.type('"HELLO HEINO\r')
        host.wait_for('HELLO HEINO')
        print('ok    what Anna says the first player hears')
        bo.close()
        host.wait_for('Bo quits.')
        host.say('look')
        host.wait_for('Anna is here.')
        print('ok    Bo\'s window closes: the port logs Bo off ("Bo quits.")')
        anna.send(bytes([0o376]))
        anna.wait_for('Do you really want to quit the game?')
        anna.type('yes\r')
        anna.wait_closed()
        anna.wait_for('You have left QUEST')
        host.wait_for('Anna quits.')
        print('ok    Anna logs off with Control CURSOR RETURN; her connection closes')
        code = host.end()
        if code != 0:
            raise AssertionError('host: exit %d' % code)
        print('ok    the end of the first player\'s script ends the game (exit 0)')
        if '--show' in sys.argv[1:]:
            print(host.out)
    except AssertionError as e:
        print('FAIL ', e)
        ok = False
        if host:
            name = os.path.join(tempfile.gettempdir(), 'netplay-fail.txt')
            with open(name, 'w') as f:
                f.write(host.out)
            print('      the first player\'s whole transcript is in %s' % name)
            print('      the port\'s log of the players:\n' + host.log)
    finally:
        for p in players or []:
            p.close()
        if host:
            host.kill()
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
