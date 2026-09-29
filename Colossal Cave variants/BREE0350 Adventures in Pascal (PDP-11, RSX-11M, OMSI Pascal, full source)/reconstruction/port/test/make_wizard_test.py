#!/usr/bin/env python3
"""make_wizard_test.py - (re)write test/scripts/11-unlimited-wizard-hints.txt.

The session: with -u, MAGIC MODE as the first command; answer the wizard test
with the game's own hints; in maintenance mode change the magic word to
"gnome" (lower case - FIX 3) and the magic number to 12345; then a second game
at another hour, where the hints must have followed the change and must be
accepted again.  The challenge letters depend on the (frozen) time of day, so
the replies cannot be written down in advance: this plays each game as far as
the hint, reads it, checks it against the rule worked out by hand, and writes
the finished script for run_tests.py to record and compare.
"""
import os, re, shutil, subprocess, tempfile

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A1 = ['-u', '-d', '28-OCT-1980', '-t', '090000']
A2 = ['-u', '-d', '28-OCT-1980', '-t', '233000']
MAINT = ['no', 'no', 'no', '0']          # see hours? change hours? holiday? short game

def main():
    save = tempfile.mkdtemp(prefix='advpas-wiz-')
    env = dict(os.environ, ADVPAS_DATA=os.path.join(PORT, 'data'), ADVPAS_SAVE=save)

    def run(args, lines):
        p = subprocess.run([os.path.join(PORT, 'adventure.exe'), '--echo'] + args, input='\n'.join(lines) + '\n',
                           capture_output=True, text=True, encoding='latin-1', env=env)
        return p.stdout

    def hint(out, word, digits, hour):
        ch, rep = re.findall(r'\n([A-Z]{5}) *\n -u: the reply is ([A-Z]{5})', out)[-1]
        assert ' -u: the magic word is %s' % word in out, out[-400:]
        want = ''.join(chr(65 + (ord(c) - 65 + d + hour) % 26) for c, d in zip(ch, digits))
        assert rep == want, (ch, rep, want)
        return rep

    try:
        pre1 = ['no', 'no', 'magic mode', 'yes', 'dwarf', 'no']
        r1 = hint(run(A1, pre1), 'DWARF', (1, 1, 1, 1, 1), 9)
        full1 = pre1 + [r1.lower()] + MAINT + ['gnome', '12345', '0', 'no', 'quit', 'yes']
        assert 'really *are* a wizard' in run(A1, full1)
        pre2 = ['no', 'no', 'magic mode', 'yes', 'gnome', 'no']
        r2 = hint(run(A2, pre2), 'GNOME', (1, 2, 3, 4, 5), 23)
        full2 = pre2 + [r2.lower()] + MAINT + ['', '0', '0', 'no', 'quit', 'yes']
        assert 'really *are* a wizard' in run(A2, full2)
    finally:
        shutil.rmtree(save, ignore_errors=True)
    path = os.path.join(PORT, 'test', 'scripts', '11-unlimited-wizard-hints.txt')
    with open(path, 'w', newline='\n') as fh:
        fh.write('# args: ' + ' '.join(A1) + '\n' + '\n'.join(full1) + '\n')
        fh.write('# run: ' + ' '.join(A2) + '\n' + '\n'.join(full2) + '\n')
    print('hints check out (%s, %s); wrote %s' % (r1, r2, os.path.relpath(path, PORT)))

if __name__ == '__main__':
    main()
