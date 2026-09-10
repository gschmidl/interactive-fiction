# -*- coding: utf-8 -*-
"""Turn the glossary listing into a program the interpreter can run.

Three things have to be got right, and all three were settled by comparing the
two copies of the glossary the disk carries -- document 0x01 and the readable
half of document 0x59.

**Bit 7.** Every character of live Wang text has bit 7 set.  Mask it off.

**0x03 and 0x02 are the listing's own formatting, not program.**  Document 0x01
is a *printed* glossary: it carries the line breaks (0x03) and format-line tab
stops (0x02, introduced by an 0x86 ruler) that made it readable on paper.  The
compact copy in 0x59 has none of them at the matching places -- where 0x01 reads
`(-CENTER-)<03>MISTY MARSH IS OPEN!!`, 0x59 reads `<01>MISTY MARSH IS OPEN!!`
with no separator -- so they are dropped.  The game's own line breaks are
`(-RETURN-)` tokens and survive.

**Where an entry stops.**  Entries are block-aligned: each starts on a 249-byte
block boundary, so the tail of the last block still holds whatever was there
before.  That stale text is often perfectly readable -- entry (a) runs on into a
fragment of entry (n) -- so it cannot be spotted by eye.  The rule that does
work comes from the language: an entry ends at the first `(-GO-TO-GL-)` that is
not inside an open `(-IF-)`, because that is an unconditional jump and nothing
after it can run.  Entries whose last act is a chain of guarded jumps -- (a),
the door hub, is seven `(-IF-)"n"(-GO-TO-GL-)x(-END-)` in a row -- end at the
last `(-END-)` instead.  The four entries that never jump at all are the deaths:
they finish `(-ERROR-)Game's over!(-EXECUTE-)` and simply stop, so they end at
their last `(-EXECUTE-)`.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "..", "data")

MARKER = re.compile(rb"\x03\(([ -~])\)\x03")
TOKEN = re.compile(r"\(-([A-Z0-9\-]{1,16})-\)")

# Bytes that belong to the printed listing rather than to the program.
LISTING_CR = 0x03
LISTING_TAB = 0x02
RULER = 0x86


def load(doc):
    return open(os.path.join(DATA, "doc%02X.bin" % doc), "rb").read()


def split_entries(raw):
    flat = bytes(b & 0x7F for b in raw)
    marks = list(MARKER.finditer(flat))
    out = []
    if marks and marks[0].start() > 0:
        out.append(("*", raw[:marks[0].start()]))
    for i, m in enumerate(marks):
        end = marks[i + 1].start() if i + 1 < len(marks) else len(raw)
        out.append((m.group(1).decode("latin1"), raw[m.end():end]))
    return out


def to_text(body):
    """Strip bit 7 and drop the listing's own formatting."""
    out = []
    skip_ruler = False
    for b in body:
        if b == RULER:
            skip_ruler = True
            continue
        if skip_ruler:
            # a ruler runs to the next line break
            if b == LISTING_CR:
                skip_ruler = False
            continue
        if b in (LISTING_CR, LISTING_TAB, 0x00, 0x01):
            continue
        c = b & 0x7F
        out.append(chr(c) if 32 <= c < 127 else " ")
    return "".join(out)


# The one place a block was left part-written.  Entry ( ), the last in the
# document, ends its first block 15 bytes early, so 15 characters of whatever
# the block held before survive between `(he put` and ` up the sign)`:
#
#     ...hiding in the Marsh since 1919 (he put|he dirt wall op| up the sign).
#
# Nothing is missing -- the disk's own playing copy of the game, document 0x10,
# reads "...since 1919 (he put up the sign).  He takes your treasures..." -- so
# the repair is to drop those 15 characters, not to supply any.
ERRATA = {
    " ": [("(he put" + "he dirt wall op" + " up the sign)", "(he put up the sign)")],
}


def compile_entry(text):
    """[(op, arg)] up to the entry's logical end, plus the end offset.

    Three constructs are folded here rather than left to the interpreter:

      (-IF-) ["-"] ('"c"' | token)   the "-" is NOT; the condition is either a
                                     comparison with what (-N-KEYS-) collected
                                     or the token (-PAGE-), "the cursor is on a
                                     page mark".  After a search that means the
                                     search ran off the end, i.e. not found.
      (-SEARCH-) term                the term runs to the next token that is
                                     not text or (-TAB-).
      (-INSERT-) term (-EXECUTE-)    likewise, terminated by EXECUTE.

    Marks are written and searched for as TAB + id, which is why (-INSERT-)
    (-TAB-)2(-EXECUTE-) and (-SEARCH-)(-TAB-)4 have to agree about the tab.
    """
    prog = []
    i = 0
    depth = 0
    first_goto = -1
    n = len(text)

    def gather(stop_on_execute):
        """Text and tabs following SEARCH/INSERT, as a literal string.

        An insert runs to its (-EXECUTE-), and the operator may move about
        while it is open -- the scoring entries read
        `(-INSERT-)(-GO-TO-PAGE-)(-SOUTH-)(-DEC-TAB-)10(-EXECUTE-)`, which
        puts "10" in a decimal-tabbed column on the scratch page.  Those
        movements do nothing to a page that is only ever searched, so they
        are stepped over; what was typed is what is kept.  A search has no
        terminator, so it stops at the first token that is not a tab.
        """
        nonlocal i
        buf = []
        while i < n:
            m = TOKEN.match(text, i)
            if m:
                name = m.group(1)
                if name in ("TAB", "DEC-TAB"):
                    buf.append("	" if name == "TAB" else "")
                    i = m.end()
                    continue
                if not stop_on_execute:
                    break
                i = m.end()
                if name == "EXECUTE":
                    break
                continue
            j = text.find("(-", i + 1)
            if j < 0:
                j = n
            buf.append(text[i:j])
            i = j
        return "".join(buf)

    while i < n:
        m = TOKEN.match(text, i)
        if not m:
            j = text.find("(-", i + 1)
            if j < 0:
                j = n
            lit = text[i:j]
            if lit:
                prog.append(("TEXT", lit))
            i = j
            continue
        name = m.group(1)
        i = m.end()
        if name == "IF":
            neg = False
            if i < n and text[i] == "-":
                neg = True
                i += 1
            q = re.match(r'"(.*?)"', text[i:])
            if q:
                prog.append(("IFKEY", ("!" if neg else "") + q.group(1)))
                i += q.end()
            else:
                t = TOKEN.match(text, i)
                cond = t.group(1) if t else "PAGE"
                if t:
                    i = t.end()
                prog.append(("IFCOND", ("!" if neg else "") + cond))
            depth += 1
        elif name == "END":
            prog.append(("END", ""))
            depth = max(0, depth - 1)
        elif name in ("GO-TO-GL", "GL"):
            tgt = text[i] if i < n else ""
            i += 1
            if depth == 0 and first_goto < 0:
                first_goto = len(prog)   # candidate end: an unconditional jump
            prog.append(("GOTO", tgt))
        elif name == "GO-TO-PAGE":
            # The page id is the next keystroke -- unless the next keystroke is
            # itself a token, in which case there is no id: (-NOTE-) goes back
            # to the document, and a direction key moves a page at a time.
            t = TOKEN.match(text, i)
            if t and t.group(1) == "NOTE":
                i = t.end()
                prog.append(("PAGERET", ""))
            elif t:
                prog.append(("PAGE", ""))
            else:
                tgt = text[i] if i < n else ""
                i += 1
                prog.append(("PAGE", tgt))
        elif name in ("COMMAND", "CANCEL"):
            prog.append((name, ""))
            # Text typed straight onto the command line and sent with EXECUTE is
            # a command, not something the document receives.  The glossary uses
            # three, all of them Wang's column arithmetic over the scratch page
            # the scores are written to: `+a=` opens accumulator a on the number
            # at the cursor, `a+` adds the one at the cursor, `a#` totals it.
            # The test is deliberately tight -- text, then EXECUTE, nothing else
            # -- because (-COMMAND-)(-UNDERSCORE-) and the beeping cursor walk in
            # (=) both put real text after these same keys.
            j = text.find("(-", i)          # i, not i+1: a token may start here
            if j < 0:
                j = n
            t = TOKEN.match(text, j)
            if j > i and t and t.group(1) == "EXECUTE":
                prog.append(("CMD", text[i:j]))
                i = t.end()
        elif name == "SEARCH":
            prog.append(("SEARCH", gather(False)))
        elif name == "INSERT":
            prog.append(("INSERT", gather(True)))
        else:
            prog.append((name, ""))
    # Where the entry really stops.  Two candidates: the first unconditional
    # jump, and the end of the last guarded block.  What decides between them is
    # what comes straight after that last (-END-).  A keystroke means the entry
    # is still running -- (c) is `IF "r" GOTO d END GOTO e`, the summing loop
    # (3) is `IF PAGE ... END CANCEL "a+" EXECUTE GL 3` -- so read on to the
    # jump.  Plain text means the entry is over and the page's previous occupant
    # has started showing through: the door hub (a) is seven guarded jumps and
    # then `lly.  Hit EXECUTE to guess again.`, the middle of a sentence.
    last_end = 0
    for k, (op, _) in enumerate(prog):
        if op == "END":
            last_end = k + 1
    if first_goto < 0:
        if last_end:
            return prog[:last_end], n
        # A death: no jump and no guard, just `(-ERROR-)Game's over!(-EXECUTE-)`
        # and stop.  The last keystroke is the end.
        last_x = 0
        for k, (op, _) in enumerate(prog):
            if op == "EXECUTE":
                last_x = k + 1
        return (prog[:last_x], n) if last_x else (prog, n)
    if last_end and first_goto > last_end:
        if prog[last_end][0] == "TEXT":
            return prog[:last_end], n
    return prog[:first_goto + 1], n


# The opening keystrokes live in document 0x01's first block, and that block
# has been partly overwritten: it holds the tail of some other entry, then the
# tail of a second one, and only then the opening -- which itself starts inside
# a token ("...You ge-)     (-TAB-)"), so its own marker and first keystrokes
# are gone.  What survives begins at the page-f initialisation, which is where
# the game seeds its memory with the mark "M", so that is where it is started.
# The name '' keeps it apart from the glossary's own (*) entry.
START = ""
START_AT = "(-GO-TO-PAGE-)f"


def program(doc=0x01):
    out = []
    for key, body in split_entries(load(doc)):
        text = to_text(body)
        if key == "*" and not out:                 # the damaged first block
            cut = text.find(START_AT)
            if cut < 0:
                continue
            key, text = START, text[cut:]
        for bad, good in ERRATA.get(key, ()):
            text = text.replace(bad, good)
        prog, _ = compile_entry(text)
        out.append((key, prog))
    return out


def main():
    prog = program()
    print("%d entries" % len(prog))
    ops = {}
    for key, body in prog:
        for op, _ in body:
            ops[op] = ops.get(op, 0) + 1
    print("\nopcodes used:")
    for k in sorted(ops, key=lambda k: -ops[k]):
        print("   %-12s %4d" % (k, ops[k]))
    tgts = set()
    keys = set(k for k, _ in prog)
    for key, body in prog:
        for op, a in body:
            if op == "GOTO":
                tgts.add(a)
    missing = sorted(t for t in tgts if t not in keys)
    print("\njump targets: %d distinct, %d missing: %s" %
          (len(tgts), len(missing), missing))


if __name__ == "__main__":
    main()
