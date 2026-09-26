"""What the tests share: running advent.exe, and the numbers of Knuth's
locations and objects, read from advent.w's own enums (so the tests never
copy a number by hand)."""
import os
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'advent.exe')
WEB = os.path.join(PORT, '..', 'src_original', 'KNUT0350', 'advent.w')


def run(lines, args=(), exe=EXE, timeout=60):
    """stdout, stderr and exit code of advent.exe given LINES"""
    data = ''.join(l + '\n' for l in lines).encode('latin-1')
    r = subprocess.run([exe] + list(args), input=data, capture_output=True,
                       timeout=timeout)
    return (r.stdout.decode('latin-1').replace('\r\n', '\n'),
            r.stderr.decode('latin-1').replace('\r\n', '\n'), r.returncode)


def _enum(text, name):
    end = re.search(r'\}\s*' + name + r'\s*;', text).start()
    start = text.rindex('typedef enum', 0, end)
    body = text[text.index('{', start) + 1:end].replace('@!', '').replace('@/', '')
    out, k = {}, 0
    for item in body.split(','):
        item = item.strip()
        if not item:
            continue
        if '=' in item:
            item, v = item.split('=')
            item, k = item.strip(), int(v)
        out[item] = k
        k += 1
    return out


_text = open(WEB, encoding='latin-1').read()
LOC = _enum(_text, 'location')
OBJ = _enum(_text, 'object')
TREASURES = [t for t, n in sorted(OBJ.items(), key=lambda x: x[1])
             if n >= OBJ['GOLD'] and t != 'RUG_']
VOCAB = sorted(set(re.findall(r'new_word\("([^"]+)"', _text)))
