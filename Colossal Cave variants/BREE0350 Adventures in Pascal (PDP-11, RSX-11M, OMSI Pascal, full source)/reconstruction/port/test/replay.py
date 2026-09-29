#!/usr/bin/env python3
"""replay.py - run a command file through the game and print a readable
transcript with each command shown after the prompt it answered.

    python test/replay.py <commands.txt> [--exe debug|release] [game options...]

Uses a throwaway save directory; never touches port/save.
"""
import os, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)

def main():
    args = sys.argv[1:]
    cmdfile = args.pop(0)
    exe = os.path.join(PORT, 'adventure.exe')
    if args[:1] == ['--exe']:
        args.pop(0)
        if args.pop(0) == 'debug':
            exe = os.path.join(PORT, 'build', 'dbg', 'adventure-dbg.exe')
    lines = open(cmdfile, encoding='latin-1').read().split('\n')
    save = tempfile.mkdtemp(prefix='advpas-replay-')
    try:
        env = dict(os.environ, ADVPAS_DATA=os.path.join(PORT, 'data'), ADVPAS_SAVE=save)
        # --echo: the game shows each line it reads, as a terminal would have
        p = subprocess.run([exe, '--echo'] + args, input='\n'.join(lines), capture_output=True, text=True,
                           encoding='latin-1', env=env, timeout=300)
    finally:
        shutil.rmtree(save, ignore_errors=True)
    text = p.stdout
    text = '\n'.join(l.rstrip() for l in text.split('\n'))
    sys.stdout.write(text.replace('\0', '<NUL>') + '\n')
    if p.stderr.strip():
        sys.stdout.write('--- stderr ---\n' + p.stderr)
    sys.stdout.write('--- exit %d ---\n' % p.returncode)

if __name__ == '__main__':
    main()
