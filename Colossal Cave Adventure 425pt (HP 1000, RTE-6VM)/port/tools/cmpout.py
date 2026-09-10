"""Normalise and diff a SIMH console transcript against a port transcript.

The HP terminal echoes what is typed, so the emulated machine's transcript
carries the command after each '>' prompt while the port's (fed from a pipe)
does not.  Both are otherwise the same stream of characters.

  python cmpout.py <script.in> <sim.out> <port.out>
"""
import difflib
import re
import sys


def load(path):
    return open(path, encoding='latin1').read().replace(chr(13), '')


def commands(path):
    out = []
    for line in load(path).split(chr(10)):
        if line.startswith('@'):
            continue
        out.append(line.strip().upper())
    return out


def strip_echo(text, cmds):
    """Remove the terminal's echo of each command from a SIMH transcript."""
    pending = list(cmds)
    res = []
    for line in text.split(chr(10)):
        if line.startswith('>') and pending:
            rest = line[1:]
            while pending and pending[0] == '':
                pending.pop(0)
            if pending and rest.upper().startswith(pending[0]):
                line = '>' + rest[len(pending[0]):]
                pending.pop(0)
        res.append(line)
    # The terminal also echoed the RETURN, so the machine's transcript breaks
    # the line after the prompt where the port keeps writing on it.
    return chr(10).join(res).replace('>' + chr(10), '>')


TAIL = ('', '>', '[end of input]',
        ">I expect one or two words at the '>' prompt.")


def clean(text):
    """Trim trailing blanks, then the end-of-session noise.

    The two runs stop differently: the port sees end of file on its piped
    input, while the emulated machine gets one more empty line from the
    driver and complains about it.  Neither is part of the game.
    """
    lines = [l.rstrip() for l in text.split(chr(10))]
    while lines and not lines[0]:
        lines.pop(0)
    while lines and lines[-1] in TAIL:
        lines.pop()
    return lines


def truncate(lines, nprompt):
    """Keep the transcript up to the nprompt-th '>' prompt.

    Both runs stop differently -- the port sees end of file on its piped
    input, the emulated machine gets one more empty line from the driver --
    so cut both at the same point instead of comparing the tails.
    """
    seen = 0
    for i, l in enumerate(lines):
        if l.startswith('>'):
            seen += 1
            if seen >= nprompt:
                return lines[:i + 1]
    return lines


def main():
    script, simfile, portfile = sys.argv[1], sys.argv[2], sys.argv[3]
    cmds = [c for c in commands(script) if c]
    sim = clean(strip_echo(load(simfile), commands(script)))
    port = clean(load(portfile))
    # Line the two up on the first room description, so the different
    # start-up banners do not offset everything.
    anchor = 'You are standing at the end of a road before a small brick'
    for seq in (sim, port):
        for i, l in enumerate(seq):
            if anchor in l:
                del seq[:i]
                seq[0] = anchor + seq[0].split(anchor, 1)[1]
                break
    n = min(sum(1 for l in sim if l.startswith('>')),
            sum(1 for l in port if l.startswith('>')))
    sim = truncate(sim, n)
    port = truncate(port, n)
    diff = list(difflib.unified_diff(sim, port, 'simh', 'port',
                                     lineterm='', n=1))
    if not diff:
        print('IDENTICAL  (%d lines)' % len(sim))
        return 0
    print(chr(10).join(diff))
    print('--- %d differing lines of %d/%d'
          % (sum(1 for d in diff if d[:1] in '+-' and d[:3] not in ('+++', '---')),
             len(sim), len(port)))
    return 1


if __name__ == '__main__':
    sys.exit(main())
