#!/usr/bin/env python3
"""The texts of V6.2 (the FORTRAN source's text.dat) against V6.4.2 (the
browser edition's Big.js), written out as port\\TEXTS_V62_V642.md.

    python tests\\textdiff.py

V6.2's text is text.dat: numbered lines in sections (1 long and 2 short
descriptions, 5 objects, 6 messages, 7 inventory names, 10 ranks, 12 magic
messages; 3 is the vocabulary).  V6.4.2's is in the SVM image: every
string the code pushes (PUSHSTR, opcode 0x13, a 24-bit address of bytes
packed big-endian up to a 0), de-duplicated and in no text order - the
numbering is in the code that stores them, not read here.  So the
comparison is line by line: a V6.2 line found among V6.4.2's strings is
unchanged; the rest are paired with V6.4.2's new strings by similarity
(an edit), and what is left over was dropped or added.  Strings that are
V6.2 program text (FORMATs and literals in the .F files) are not counted
as added.
"""
import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.dirname(os.path.dirname(HERE))
TEXT = os.path.join(FOLDER, 'src_original', 'ROBE0665', 'text.dat')
BIG = os.path.join(FOLDER, 'archive_original', 'Browser', 'Big.js')
SOURCES = ['newadv.F', 'advlib.F', 'aparse.F', 'setup.F', 'wizard.F']
OUT = os.path.join(os.path.dirname(HERE), 'TEXTS_V62_V642.md')
TEXT_SECTIONS = {1: 'long description', 2: 'short description', 5: 'object',
                 6: 'message', 7: 'inventory name', 10: 'rank', 12: 'magic message'}
# section 7 puts numbers before its text: keep only the text.  (Section 13,
# the milestones, has text too, but setup.F reads it into CARRAY and keeps
# only the numbers: it is a comment, and no program has it.)
LABELLED = {7: r'(?:\s*-?\d+){5}\s+(.*)$'}


def v62():
    """[(section, number, text)] of the text sections, and the vocabulary"""
    lines, vocab, section = [], set(), None
    for raw in open(TEXT, encoding='latin-1').read().split('\n'):
        line = raw.rstrip('\r')
        if re.fullmatch(r'\s*-?\d+\s*', line):
            # a section's number alone on its line; -1 ends it (7 does
            # not: 8 follows at once), 0 ends the file
            n = int(line)
            section = n if n > 0 else None
            continue
        if section is None or len(line) <= 8 or not line[:8].strip().isdigit():
            continue
        # the number in columns 1-8, the text from column 9
        num, text = line[:8].strip(), line[8:].rstrip()
        if section in LABELLED:
            m = re.match(LABELLED[section], text)
            if not m:
                continue
            text = m.group(1).rstrip()
        if section in TEXT_SECTIONS:
            lines.append((section, num, text))
        elif section == 3 and text.split():
            # the word (the game reads five letters), before any comment
            vocab.add(text.split()[0].upper()[:5])
    return lines, vocab


def v642():
    src = open(BIG, encoding='latin-1').read()
    code = [int(x) for x in re.findall(r'-?\d+', src[src.index('[') + 1:src.index(']')])]

    def string_at(addr):
        out, a, shift = bytearray(), addr, 24
        while a < len(code):
            b = (code[a] & 0xFFFFFFFF) >> shift & 0xFF
            if b == 0:
                break
            out.append(b)
            shift -= 8
            if shift < 0:
                shift, a = 24, a + 1
        return out.decode('latin-1')
    addrs = {w & 0xFFFFFF for w in code if (w & 0xFFFFFFFF) >> 24 == 0x13}
    return {string_at(a).rstrip() for a in addrs}


def program_strings():
    """string literals and FORMAT texts of V6.2's own FORTRAN"""
    out = set()
    for f in SOURCES:
        path = os.path.join(FOLDER, 'src_original', 'ROBE0665', f)
        if os.path.exists(path):
            for s in re.findall(r"'([^'\n]*)'", open(path, encoding='latin-1').read()):
                out.add(s.rstrip())
    return out


def program_names():
    """every name in V6.2's FORTRAN and its COMMON includes (the image
    carries the program's names as strings too)"""
    src = os.path.join(FOLDER, 'src_original', 'ROBE0665')
    out = set()
    for f in os.listdir(src):
        if f.endswith(('.F', '.h')):
            out |= set(re.findall(r'\b[A-Za-z][A-Za-z0-9]*\b',
                                  open(os.path.join(src, f), encoding='latin-1').read().upper()))
    return out


def is_text(s):
    """a line of the game's text, not a name or the page's JavaScript
    (boxes and pictures, all capitals and strokes, count: they are long)"""
    if '&&' in s or '()' in s or ' ' not in s.strip():
        return False
    return bool(re.search(r'[a-z]', s)) or len(s) >= 20


def main():
    lines, vocab = v62()
    strings = v642()
    prog = program_strings()
    v62_texts = {t for _, _, t in lines}
    same = [e for e in lines if e[2] in strings or not e[2].strip()]
    gone = [e for e in lines if e[2].strip() and e[2] not in strings]
    new = sorted(s for s in strings if is_text(s) and s not in v62_texts and s not in prog)

    # pair dropped lines with new strings, the most similar first; V6.4.2
    # spells out the compass abbreviations, so they are spelled out before
    # comparing (otherwise "at NE end" is nearer "at southwest end")
    def spelled(s):
        for a, b in (('NE', 'northeast'), ('NW', 'northwest'),
                     ('SE', 'southeast'), ('SW', 'southwest')):
            s = re.sub(r'\b%s\b' % a, b, s)
        return s
    scores = []
    for i, (_, _, old) in enumerate(gone):
        for j, cand in enumerate(new):
            r = difflib.SequenceMatcher(None, spelled(old), cand).ratio()
            # short lines ("You're in X.") look alike by their frame alone
            if r >= (0.6 if min(len(old), len(cand)) >= 40 else 0.8):
                scores.append((r, i, j))
    pairs, used_i, used_j = [], set(), set()
    for r, i, j in sorted(scores, reverse=True):
        if i not in used_i and j not in used_j:
            used_i.add(i)
            used_j.add(j)
            pairs.append(gone[i] + (new[j],))
    dropped = [e for i, e in enumerate(gone) if i not in used_i]
    added = [s for j, s in enumerate(new) if j not in used_j]

    words = {s for s in strings if re.fullmatch(r"[A-Z0-9'\-.]{1,5}", s) and re.search('[A-Z]', s)}
    vocab_gone = sorted(w for w in vocab if w not in strings)
    vocab_new = sorted(words - vocab - program_names())

    def entry(sec, num):
        return '%s %s' % (TEXT_SECTIONS[sec], num)

    out = ['# The texts of V6.2 and V6.4.2', '',
           'Made by `tests\\textdiff.py`: V6.2 is `..\\src_original\\ROBE0665\\text.dat` (the FORTRAN',
           'source, newadv.F of 3 March 2010, 655 points); V6.4.2 is the browser edition\'s',
           '`..\\archive_original\\Browser\\Big.js` (database of 7 June 2021, 665 points), whose',
           'strings are read out of the SVM image. The image keeps each string once and not in',
           'text order, so the comparison is by line; entries are named by V6.2\'s section and',
           'number.', '',
           '| | lines |', '|---|---|',
           '| V6.2 text lines | %d |' % len(lines),
           '| unchanged in V6.4.2 | %d |' % len(same),
           '| changed | %d |' % len(pairs),
           '| only in V6.2 | %d |' % len(dropped),
           '| only in V6.4.2 | %d |' % len(added), '']
    out += ['## Changed', '']
    for sec, num, old, new_ in sorted(pairs, key=lambda p: (p[0], int(p[1]) if p[1].isdigit() else 0)):
        out += ['- **%s**' % entry(sec, num), '  - V6.2: `%s`' % old, '  - V6.4.2: `%s`' % new_]
    out += ['', '## Only in V6.2', '']
    out += ['- **%s**: `%s`' % (entry(s, n), t) for s, n, t in dropped] or ['(none)']
    out += ['', '## Only in V6.4.2', '']
    out += ['- `%s`' % t for t in added] or ['(none)']
    out += ['', '## Vocabulary', '',
            'Words of V6.2\'s section 3 missing from V6.4.2: %s' % (', '.join(vocab_gone) or 'none'), '',
            'Words (up to five capitals) only in V6.4.2, not V6.2 program names: %s'
            % (', '.join(vocab_new) or 'none'), '']
    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(out))
    print('textdiff: %d lines, %d unchanged, %d changed, %d only in V6.2, %d only in V6.4.2 -> %s'
          % (len(lines), len(same), len(pairs), len(dropped), len(added), OUT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
