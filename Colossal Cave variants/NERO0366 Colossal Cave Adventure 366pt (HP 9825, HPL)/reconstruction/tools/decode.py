"""Decode the NERO0366 (HP 9825 Adventure) database into advent.dat-style text.

usage: decode.py adventure.dsk outdir

Writes, in outdir:
  raw/SECn.txt      the source sections kept on the disk (type 0x2400 files),
                    with the editor line numbers ("10  ! ") stripped
  secN_*.txt        the built data files (SECnH) decoded back to text
  saves/*.txt       the game-state files, named the way ADVENT reads them
  CHECK.txt         built files compared with their source sections
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hpldisk  # noqa: E402

disk = open(sys.argv[1], 'rb').read()
out = sys.argv[2]
os.makedirs(os.path.join(out, 'raw'), exist_ok=True)
os.makedirs(os.path.join(out, 'saves'), exist_ok=True)
files = {e['name']: e for e in hpldisk.directory(disk)}


def recs(name):
    return dict(hpldisk.records(hpldisk.file_bytes(disk, files[name])))


def stream(name):
    return hpldisk.serial(hpldisk.file_bytes(disk, files[name]))


def write(path, lines):
    with open(os.path.join(out, path), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


# --- raw source sections -------------------------------------------------
raw = {}
for name, e in files.items():
    if e['type'] != 0x2400:
        continue
    lines = [item.split('!', 1)[1] for item in stream(name) if isinstance(item, str) and '!' in item]
    raw[name.upper()] = lines
    write(f'raw/{name.upper()}.txt', lines)


# --- message files: record = [number, text, link to continuation record] --
def chained(name, first_records):
    r = recs(name)
    lines = []
    for rec in first_records:
        items = r.get(rec)
        while items and len(items) >= 2 and isinstance(items[1], str):
            lines.append((items[0], items[1]))
            link = items[2] if len(items) > 2 else 0
            items = r.get(link) if link else None
    return lines


sec1 = chained('SEC1H', range(1, 151))
write('sec1_long_descriptions.txt', [f'{n}\t{t}' for n, t in sec1])
sec2 = [(r[0], r[1]) for _, r in sorted(recs('SEC2H').items()) if len(r) >= 2 and isinstance(r[1], str) and r[0]]
write('sec2_short_descriptions.txt', [f'{n}\t{t}' for n, t in sec2])
sec6 = chained('SEC6H', range(1, 225))
write('sec6_messages.txt', [f'{n}\t{t}' for n, t in sec6])
sec10 = chained('SEC10H', range(1, 12))
write('sec10_ranks.txt', [f'{n}\t{t}' for n, t in sec10])
sec12 = chained('SEC12H', range(1, 35))
write('sec12_messages.txt', [f'{n}\t{t}' for n, t in sec12])

# --- objects: record O = [O, name, P1..P7]; P = first record of a state message
r5 = recs('SEC5H')
sec5 = []
for o in range(1, 75):
    items = r5.get(o)
    if not items or len(items) < 2 or not isinstance(items[1], str):
        continue
    sec5.append((items[0], items[1]))
    for p in items[2:]:
        if not p:
            continue
        msg = r5.get(p)
        while msg and len(msg) >= 2 and isinstance(msg[1], str):
            sec5.append((f'{msg[0]:03d}', msg[1]))
            msg = r5.get(msg[2]) if len(msg) > 2 and msg[2] else None
write('sec5_objects.txt', [f'{n}\t{t}' for n, t in sec5])

# --- travel: K$ = start index per location, T = verb + 1000 * destination,
#     the last entry of a location negative
k_str, *travel = stream('SEC3H')
keys = hpldisk.words(k_str)
sec3 = []
for loc, start in enumerate(keys, 1):
    if not start:
        continue
    i, line = start - 1, None
    while i < len(travel):
        t = travel[i]
        dest, verb = divmod(abs(t), 1000)
        if line and line[1] == dest:
            line[2].append(verb)
        else:
            line = [loc, dest, [verb]]
            sec3.append(line)
        if t < 0:
            break
        i += 1
write('sec3_travel.txt', [f'{loc}\t{dest}\t' + '\t'.join(map(str, verbs)) for loc, dest, verbs in sec3])
trvl = [n for n in stream('TRVL') if isinstance(n, int)]

# --- vocabulary: V$ = 5-letter words, I$ = their values (fti) -----------
v_str, i_str = stream('SEC4H')[:2]
values = hpldisk.words(i_str)
sec4 = []
for n in range(len(v_str) // 5):
    word = v_str[5 * n:5 * n + 5]
    if not word.strip():
        break
    sec4.append((values[n], word.rstrip()))
write('sec4_vocabulary.txt', [f'{v}\t{w}' for v, w in sec4])

# --- game-state files (ADVENT "Readrest" / A1 save order) ----------------
NAMES = (['A$', 'C$', 'E$', 'F$', 'G$', 'H$', 'J$', 'L$', 'N$', 'P$', 'Q$', 'S$', 'V$', 'Z$',
          'B[*]', 'D[*]', 'E[*]', 'C[*]', 'O[*]'] + [chr(c) for c in range(65, 91)]
         + [f'r{n}' for n in range(30)])
for name in ('ADVNEW', 'ADVSAV', 'BIGSAV', 'BSTSAV', 'LSTSAV', 'OKSAV'):
    lines = []
    for rec, items in hpldisk.records(hpldisk.file_bytes(disk, files[name])):
        for item in items:
            if isinstance(item, str):
                lines.append(f'rec {rec}: string len {len(item)}: ' + ' '.join(map(str, hpldisk.words(item))))
            else:
                lines.append(f'rec {rec}: {item}')
    write(f'saves/{name}.txt', lines)


# --- cross-check built files against the source sections -----------------
check = []


def compare(title, built, source):
    built, source = list(built), list(source)
    same = built == source
    check.append(f'{title}: {"identical" if same else "DIFFERENT"} ({len(built)} built, {len(source)} source lines)')
    if not same:
        for i, (a, b) in enumerate(zip(built, source)):
            if a != b:
                check.append(f'  first difference at line {i + 1}:\n    built : {a!r}\n    source: {b!r}')
                break


def text_rows(lines, width):
    """source line: number right after '!' in a field of `width`, then text"""
    return [(int(ln[:width]), ln[width:]) for ln in lines]


compare('section 2 (SEC2H vs Sec2)', sec2, text_rows(raw['SEC2'], 4))
compare('section 10 (SEC10H vs SEC10)', sec10, text_rows(raw['SEC10'], 4))
compare('section 12 (SEC12H vs SEC12)', sec12, text_rows(raw['SEC12'], 4))
def groups(rows):
    """object number -> its name and state messages, in the order given"""
    g, cur = {}, None
    for n, t in rows:
        if isinstance(n, int):
            cur = g.setdefault(n, [t])
        else:
            cur.append((n, t))
    return sorted(g.items())


compare('section 5 (SEC5H vs SEC5, per object)', groups(sec5),
        groups((int(ln[:4]) if 0 < int(ln[:4]) < 100 else f'{int(ln[:4]):03d}', ln[4:]) for ln in raw['SEC5']))
compare('section 4 (SEC4H vs SEC4, first 5 letters, sorted)', sorted(sec4),
        sorted((int(ln[:5]), ln[5:10].rstrip()) for ln in raw['SEC4']))
src3 = []
for ln in raw['SEC3']:
    nums = [int(x) for x in ln.split()]
    src3 += [(nums[0], nums[1], v) for v in nums[2:]]
compare('section 3 (SEC3H vs SEC3, location/destination/verb triples)',
        [(a, b, v) for a, b, c in sec3 for v in c], src3)
compare('TRVL vs SEC3H travel array', trvl[:len(travel)], travel)
write('CHECK.txt', check)
print('\n'.join(check))
