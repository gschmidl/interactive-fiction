"""What the tests share: running the two programs, their vocabularies
(read from each program's own advword.h), and random sessions."""
import os
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
VARIANTS = ('pohl', 'daimler')


def exe(variant):
    return os.path.join(PORT, variant, 'advent.exe')


def run(variant, lines, args=(), exe_path=None, cwd=None, timeout=60):
    """stdout (LF line ends), stderr and exit code, given LINES.  The text
    files are found beside the .exe; saved games go to CWD."""
    data = ''.join(l + '\n' for l in lines).encode('latin-1')
    r = subprocess.run([exe_path or exe(variant)] + list(args), input=data,
                       capture_output=True, timeout=timeout, cwd=cwd)
    return (r.stdout.decode('latin-1').replace('\r\n', '\n'),
            r.stderr.decode('latin-1').replace('\r\n', '\n'), r.returncode)


def vocab(variant):
    with open(os.path.join(PORT, 'src', variant, 'advword.h'), encoding='latin-1') as f:
        return [w for w, _ in re.findall(r'"([^"]+)",\s*(\d+)', f.read())]


def session(rng, words, moves, save=True, many=True):
    """the instructions question, MOVES random commands of the game's own
    words (one, two, or a string of them, a word repeated, a y/n), then
    QUIT and y three times over, so that the originals, which never stop at
    the end of their input, stop.  SAVE false leaves out the words that
    save the game and stop it (SUSPEND, SAVE, PAUSE); MANY false, the
    strings of three words or more and the long repeated words."""
    if not save:
        words = [w for w in words if w not in ('suspend', 'save', 'pause')]
    out = [rng.choice(['n', 'no', 'y'])]
    for _ in range(moves):
        r = rng.random()
        if r < 0.4:
            out.append(rng.choice(words))
        elif r < 0.85:
            out.append('%s %s' % (rng.choice(words), rng.choice(words)))
        elif r < 0.9:
            out.append(rng.choice(['', 'y', 'n', 'yes', 'no']))
        elif r < 0.95 and many:
            out.append(' '.join(rng.choice(words) for _ in range(rng.randint(3, 12))))
        elif many:
            out.append(rng.choice(words) * rng.randint(3, 10))
        else:
            out.append(rng.choice(words))
    return out + ['quit', 'y'] * 3
