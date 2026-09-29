"""Expand VAX FORTRAN tab-format source into standard fixed form.

VAX FORTRAN accepts a tab in place of the column 1-6 field:

  * a comment line still starts with C, c, *, ! or D in column 1;
  * otherwise an optional statement label may occupy columns 1..n, and the
    first tab ends the label field;
  * if the character following the tab is a digit 1-9 the line is a
    CONTINUATION line and that digit is the continuation character;
  * any other character starts the statement proper (column 7).

Everything else is passed through untouched.
"""
import sys


def untab(s):
    """Indentation tabs become spaces; a tab inside a character literal
    stays (one message in QUEST_ERROR indents with tabs)."""
    out, inq = [], False
    for c in s:
        if c == "'":
            inq = not inq
        out.append(' ' if (c == '	' and not inq) else c)
    return ''.join(out)


def convert(line):
    line = line.rstrip('\n').rstrip('\r')
    if not line:
        return line
    if line[0] in 'Cc*!Dd' and '\t' not in line[:6]:
        return line                                   # comment / debug line
    if '\t' not in line:
        return line                                   # already fixed form
    i = line.index('\t')
    label = line[:i]
    rest = line[i + 1:]
    if not all(c.isdigit() or c == ' ' for c in label):
        return untab(line)                            # tab inside a statement
    if rest[:1].isdigit() and rest[0] != '0':
        out = '     ' + rest[0] + rest[1:]            # continuation line
    else:
        out = label.ljust(6) + rest
    return untab(out)


def main(src, dst):
    out, longest = [], 0
    for n, line in enumerate(open(src, 'r', encoding='latin1'), 1):
        c = convert(line)
        if len(c.rstrip()) > 72:
            longest = max(longest, len(c.rstrip()))
            print('  %s:%d exceeds 72 columns (%d)' % (src, n, len(c.rstrip())),
                  file=sys.stderr)
        out.append(c)
    open(dst, 'w', encoding='latin1', newline='\n').write('\n'.join(out) + '\n')
    return longest


if __name__ == '__main__':
    print('max over-long line:', main(sys.argv[1], sys.argv[2]))
