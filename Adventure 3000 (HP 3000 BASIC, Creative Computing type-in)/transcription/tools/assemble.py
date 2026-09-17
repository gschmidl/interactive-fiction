"""Assemble the verified row transcriptions into the program.

Writes (to ../):
  listing_as_printed.txt   every printed row of pp. 126-138 in reading order,
                           each character in the column it was printed in
                           (LIST's own indentation and spacing reproduced)
  ADVENTURE3000.BAS        one statement per line: "&" continuations joined,
                           the two rows the magazine's paste-up split back
                           together, and the two REM tails the listing
                           device cut off at column 72 restored from the
                           typeset text the editors pasted in
Rows whose own scan is damaged ({nolearn} in truth.txt) have no reliable cell
alignment; they are laid out from the typed text, which was read from the
other two scans.
"""
import json, os, re
import pagegeom as pg
import truth as T
from truth import load_cells, parse, align, HERE, TRUTH
from columns import COLUMNS

OUT = os.path.normpath(os.path.join(HERE, ".."))

# p136 c2: rows split by the paste-up, not by LIST (no "&", strip too narrow).
# key = first line number, value = text to put between the two halves
PASTEUP_JOIN = {9170: "", 9420: " "}
# listing device truncated these REMs at column 72; the magazine pasted the
# rest in typeset (see typeset.txt)
TYPESET_TAIL = {15: " OF THE", 16: "T."}


def laid_out(text, res, meta):
    """The row as printed: each aligned character at its cell column."""
    chars = [ch for ch in text if ch != " "]
    cols = [int(meta[i][3]) for _, i in res]
    width = max(cols) + 1
    line = [" "] * width
    for ch, k in zip(chars, cols):
        if ch != "~":                     # "~" marks a speck, not a character
            line[k] = ch
    return "".join(line).rstrip()


def typed_layout(text):
    """Fallback layout for rows without a reliable alignment."""
    text = text.replace("~", " ")
    m = re.match(r"(\d+) (.*)$", text)
    if m:
        return "%4s %s" % (m.group(1), m.group(2))
    return "     " + text


def main():
    img, meta, core, index = load_cells()
    rows = parse(TRUTH)
    rows.sort(key=lambda t: (t[0], t[1], t[2]))
    printed = []
    for p, c, r, text in rows:
        if p == 126 and c == 0 and r == 0:
            continue                      # "A3000" header above the listing
        res = None if T.nolearn.get((p, c, r)) else align(text, index[(p, c, r)], core)
        line = laid_out(text, res, meta) if res else typed_layout(text)
        printed.append((p, c, r, line))
    with open(os.path.join(OUT, "listing_as_printed.txt"), "w", encoding="ascii", newline="\n") as f:
        page = None
        for p, c, r, line in printed:
            if (p, c) != page:
                f.write("\n" if page else "")
                f.write("----- page %d, column %d -----\n" % (p, c + 1))
                page = (p, c)
            f.write(line + "\n")
    # join into statements
    stmts = []
    for p, c, r, line in printed:
        m = re.match(r"\s*(\d+) (.*)$", line)
        if m and not (stmts and stmts[-1][1].endswith("&")):
            stmts.append([int(m.group(1)), line.strip(), (p, c, r)])
            continue
        cont = line.strip()
        prev = stmts[-1]
        if prev[1].endswith("&"):
            head = prev[1][:-1]
            # LIST breaks long statements between tokens; two words that met
            # at the break had a space between them (outside a string)
            in_str = head.count('"') % 2 == 1
            sep = " " if (not in_str and head[-1:].isalnum() and cont[:1].isalnum()) else ""
            prev[1] = head + sep + cont
        elif prev[0] in PASTEUP_JOIN:
            prev[1] = prev[1] + PASTEUP_JOIN[prev[0]] + cont
        else:
            raise SystemExit("continuation without & after line %d: %r" % (prev[0], line))
    for s in stmts:
        if s[0] in TYPESET_TAIL:
            s[1] += TYPESET_TAIL[s[0]]
        if s[1].endswith("&"):
            raise SystemExit("dangling & at line %d" % s[0])
    nums = [s[0] for s in stmts]
    assert nums == sorted(nums) and len(set(nums)) == len(nums), "line numbers out of order or repeated"
    with open(os.path.join(OUT, "ADVENTURE3000.BAS"), "w", encoding="ascii", newline="\n") as f:
        for s in stmts:
            f.write(s[1] + "\n")
    print("printed rows", len(printed), "statements", len(stmts), "lines %d..%d" % (nums[0], nums[-1]))


if __name__ == "__main__":
    main()
