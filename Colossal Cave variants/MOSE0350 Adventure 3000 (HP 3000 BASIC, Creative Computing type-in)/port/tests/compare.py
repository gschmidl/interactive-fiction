"""Diff a transcript taken from the HP 3000 against the port's output.

usage: compare.py <machine transcript> <port output> <moves file>

Both sides are reduced to the text a player would see: the terminal
enhancement escapes and the bell are dropped, the game's own LF+CR line break
counts as one line break (the port's carriage model emits the LF alone and
Windows expands it), the commands the machine echoes are removed, and the
machine's logon/logoff chatter is cut.
"""
import difflib, re, sys


def norm(raw, is_machine, moves):
    text = raw.decode("latin-1")
    text = text.replace("\r\n", "\n").replace("\n\r", "\n").replace("\r", "\n")
    text = re.sub(r"\x1b&d[A-Z@]", "", text)          # terminal enhancement on/off
    text = text.replace("\x07", "")                   # bell
    lines = []
    for line in text.split("\n"):
        line = line.rstrip()
        if is_machine and (line == "RUN" or re.fullmatch(r"ADV3000[A-Z]?", line)):
            continue
        line = line.lstrip(">")                       # the game's own prompt
        if line.startswith("?"):
            line = line[1:]
        lines.append(line.rstrip())
    for i, l in enumerate(lines):
        if l.strip() in ("EXIT", "DONE") or l.startswith(("END OF PROGRAM", "CPU=", "Disconnected", "BYE")):
            lines = lines[:i]
            break
    # both sides echo the commands; drop them so only the game's words remain
    out, i = [], 0
    for l in lines:
        if i < len(moves) and l.strip() == moves[i]:
            i += 1
            continue
        out.append(l)
    while out and not out[-1]:
        out.pop()
    return [re.sub(r"Adventure 3\.2 on .*", "Adventure 3.2 on <date>", l) for l in out]


def main(mfile, pfile, movesfile):
    moves = [l.strip() for l in open(movesfile, encoding="ascii") if l.strip()]
    a = norm(open(mfile, "rb").read(), True, moves)
    b = norm(open(pfile, "rb").read(), False, moves)
    d = list(difflib.unified_diff(a, b, "machine", "port", lineterm="", n=1))
    print("\n".join(d) if d else "IDENTICAL (%d lines of game output)" % len(a))
    return 1 if d else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3]))
