#!/usr/bin/env python3
"""run_tests.py - replay the scripted sessions and compare with the recorded transcripts.

    python test/run_tests.py            compare
    python test/run_tests.py --record   (re)write test/transcripts/*.txt

Each test/scripts/NAME.txt starts with a line  "# args: ..."  giving the game
options (always with a frozen --date/--time, which also fixes the random
numbers).  A session may be several runs sharing one save directory: a line
"# run: ..." starts the next run with new options (used for suspend/resume).
Everything runs on a throwaway save directory; port/save is never touched.
"""
import os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'adventure.exe')

def session(path):
    runs, cur = [], None
    for ln in open(path, encoding='latin-1').read().split('\n'):
        if ln.startswith('# args:') or ln.startswith('# run:'):
            cur = [ln.split(':', 1)[1].split(), []]
            runs.append(cur)
        elif cur is not None:
            cur[1].append(ln)
    return runs

def play(path):
    save = tempfile.mkdtemp(prefix='advpas-test-')
    out = []
    try:
        env = dict(os.environ, ADVPAS_DATA=os.path.join(PORT, 'data'), ADVPAS_SAVE=save)
        for args, lines in session(path):
            while lines and lines[-1] == '': lines.pop()
            # --echo: the game shows each line it reads, as a terminal would have
            p = subprocess.run([EXE, '--echo'] + args, input='\n'.join(lines) + '\n', capture_output=True,
                               text=True, encoding='latin-1', env=env, timeout=120)
            text = p.stdout
            out.append('==== adventure ' + ' '.join(args) + '\n' +
                       '\n'.join(l.rstrip() for l in text.split('\n')) +
                       ('\n---- stderr\n' + p.stderr if p.stderr.strip() else '') +
                       '\n---- exit %d\n' % p.returncode)
    finally:
        shutil.rmtree(save, ignore_errors=True)
    return ''.join(out)

def main():
    record = '--record' in sys.argv
    sdir, tdir = os.path.join(HERE, 'scripts'), os.path.join(HERE, 'transcripts')
    os.makedirs(tdir, exist_ok=True)
    bad = 0
    for name in sorted(os.listdir(sdir)):
        if not name.endswith('.txt'): continue
        got = play(os.path.join(sdir, name))
        want_path = os.path.join(tdir, name)
        if record:
            open(want_path, 'w', encoding='latin-1', newline='\n').write(got)
            print('recorded', name)
        elif not os.path.exists(want_path):
            print('NO TRANSCRIPT', name); bad += 1
        elif open(want_path, encoding='latin-1').read() != got:
            open(os.path.join(HERE, 'out', name + '.got'), 'w', encoding='latin-1', newline='\n').write(got) \
                if os.path.isdir(os.path.join(HERE, 'out')) or not os.makedirs(os.path.join(HERE, 'out')) else None
            print('DIFFERS ', name, '(see test/out/%s.got)' % name); bad += 1
        else:
            print('ok      ', name)
    print('FAIL' if bad else 'PASS')
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
