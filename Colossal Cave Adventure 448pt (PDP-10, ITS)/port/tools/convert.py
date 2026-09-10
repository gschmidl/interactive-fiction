#!/usr/bin/env python3
r"""Mechanically convert DEC FORTRAN-10 sources to gfortran.

The port keeps the PDP-10's word layout rather than modernising it: a
word is five seven-bit characters, char k in bits 36-7k..30-7k, held in
an INTEGER*8.  The program's own DATA MASKS confirm the layout --
"774000000000 selects char 1 (bits 35..29) and the mask list
"4000000000,"20000000,"100000,"400,"2 is the low bit of each field
(29, 22, 15, 8, 1).  Because the layout is unchanged, every mask, SHIFT
and .AND./.XOR. in the game logic keeps working untouched.

What this script changes is only what the *compiler* cannot read:

  * ^L page separators (ITS printed the original formfeeds this way)
  * "nnn        DEC octal literal        -> o'nnn'
  * TYPE n      DEC output statement     -> PRINT n
  * 'ABCDE'     literal used as a word   -> o'...' packed constant

A literal is packed only outside FORMAT statements, and never in an
OPEN/CLOSE FILE= where it is a real filename.  Literals longer than five
characters take their first five, which is what FORTRAN-10 did with a
one-word literal: the source says VOCAB('PILLOW',1) and VOCAB('DAGGER',1)
while the database holds PILLO and DAGGE.
"""
import io, os, re, sys

def sx36(v):
    """A 36-bit word as a sign-extended 64-bit integer.

    Bit 35 is the sign on a PDP-10, and the program tests it: A5TOA1
    puts a blank between its second and third words only IF(C.LT.0),
    which is true exactly when the word's first character is >= 100
    octal.  Sign-extending keeps every such test, and the negations in
    GETIN, working as they did on the -10; masks are unaffected because
    the extended high bits are cleared by any 36-bit mask."""
    v &= (1 << 36) - 1
    return v - (1 << 36) if v & (1 << 35) else v


def pack(s):
    s = (s + '     ')[:5]
    v = 0
    for k, c in enumerate(s):
        v |= (ord(c) & 0x7f) << (29 - 7 * k)
    return sx36(v)

def statements(lines):
    """Yield (index_list, joined_body) for each statement."""
    out, idx, body = [], [], None
    for n, l in enumerate(lines):
        if not l.strip() or l[:1] in 'C*!c':
            continue
        cont = len(l) > 5 and l[5] not in ' 0'
        if cont and idx:
            idx.append(n); body += l[6:]
            continue
        if idx:
            out.append((idx, body))
        idx, body = [n], (l[6:] if len(l) > 6 else '')
    if idx:
        out.append((idx, body))
    return out

def convert_line(l, is_format, keep_strings):
    if is_format:
        return l
    head, text = l[:6], l[6:]
    # Literals first, while DEC octals are still "-prefixed and unquoted.
    if not keep_strings:
        text = re.sub(r"'([^']*)'", lambda m: str(pack(m.group(1)))+' ', text)
    # DEC octal "4000000000 -> decimal.  Decimal rather than a BOZ constant
    # because gfortran refuses BOZ as an actual argument or in comparisons.
    text = re.sub(r'"([0-7]+)', lambda m: str(sx36(int(m.group(1), 8)))+' ', text)
    text = re.sub(r'^( *)TYPE( +[0-9])', lambda m: m.group(1)+"PRINT"+m.group(2), text)
    # RAN is a gfortran intrinsic and the program defines its own.
    text = re.sub(r'\bRAN\b', 'RAN10', text)
    # Unit 5 was the terminal for input *and* output on the -10; under
    # gfortran unit 5 is read-only stdin, so writes go to unit 6.
    text = re.sub(r'WRITE( *)\( *5 *,',
                  lambda m: 'WRITE' + m.group(1) + '(6,', text)
    # FILE= is not permitted in CLOSE; ACCESS='SEQIN' is DEC-only.
    text = text.replace("CLOSE (UNIT=1,FILE='FT01.DAT')", "CLOSE (UNIT=1)")
    text = text.replace("OPEN (UNIT=1,FILE='FT01.DAT',ACCESS='SEQIN')",
                        "OPEN (UNIT=1,FILE='FT01.DAT',STATUS='OLD')")
    return head + text

def convert(src, dst):
    lines = io.open(src, encoding='latin-1').read().split('\n')
    lines = [l for l in lines if l.rstrip() != '^L']
    fmt, keep = set(), set()
    for idx, body in statements(lines):
        if re.search(r'\bFORMAT\b', body):
            fmt.update(idx)
        if re.search(r'\b(OPEN|CLOSE)\s*\(', body):
            keep.update(idx)
    out = [convert_line(l, n in fmt, n in keep) for n, l in enumerate(lines)]
    io.open(dst, 'w', encoding='latin-1', newline='\n').write('\n'.join(out))
    print('%-28s -> %-22s (%d lines, %d format, %d keep-string)'
          % (os.path.basename(src), os.path.basename(dst), len(out), len(fmt), len(keep)))


def drop_routines(lines, names):
    """Remove whole routines superseded by runtime.f."""
    out, skip = [], False
    for l in lines:
        body = l[6:].strip() if len(l) > 6 else ''
        if not skip and body.replace(' ', '') in [n.replace(' ', '') for n in names]:
            skip = True
            continue
        if skip:
            if body == 'END':
                skip = False
            continue
        out.append(l)
    return out


def insert_decls(lines, decls):
    """Put declarations after each named routine's IMPLICIT line."""
    out, cur = [], '@MAIN'
    for l in lines:
        out.append(l)
        body = l[6:].strip() if len(l) > 6 else ''
        u = body.upper()
        for kw in ('SUBROUTINE ', 'FUNCTION '):
            if u.startswith(kw) or (' ' + kw) in u:
                nm = u.split(kw, 1)[1].split('(')[0].strip()
                if nm:
                    cur = nm
        if u.startswith('IMPLICIT INTEGER') and cur in decls:
            out.extend(decls[cur])
            del decls[cur]
    return out


def apply_patches(lines, table):
    out, used = [], 0
    for l in lines:
        hit = None
        for old, new in table:
            if l.rstrip() == old.rstrip():
                hit = new
                break
        if hit is not None:
            out.extend(hit)
            used += 1
        else:
            out.append(l)
    return out, used


if __name__ == '__main__':
    import patches
    NL = chr(10)
    convert('src_its/ADV4MA.F4', 'port/src/adv4ma.f')
    convert('src_its/ADV4SU.F4', 'port/src/adv4su.f')
    for tag, path in (('adv4ma', 'port/src/adv4ma.f'),
                      ('adv4su', 'port/src/adv4su.f')):
        lines = io.open(path, encoding='latin-1').read().split(NL)
        if tag == 'adv4su':
            lines = drop_routines(lines, patches.DROP_ROUTINES)
            lines += patches.NEW_GETIN.split(NL)
        d = {k: v[:] for k, v in patches.DECLS.items()}
        lines = insert_decls(lines, d)
        lines, n = apply_patches(lines, patches.LINES[tag])
        io.open(path, 'w', encoding='latin-1', newline=NL).write(NL.join(lines))
        print('  %s: %d patches applied, %d decl groups unused'
              % (tag, n, len(d)))
