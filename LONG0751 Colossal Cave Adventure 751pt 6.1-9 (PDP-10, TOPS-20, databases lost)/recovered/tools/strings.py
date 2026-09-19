"""Every piece of text compiled into ADVENTURE 6.1/9, and how it compares
with 6.1/3.

The game's own text -- rooms, objects, most messages -- lives in the
encrypted text database, which for 6.1/9 is lost.  What the program itself
carries is its FORMAT statements and its character constants: the frame of
every line it prints, the debugging and wizard output, the log and gripe
layouts, and a few messages the authors wrote straight into the code.

    python strings.py      writes ../strings-6.1-9.txt, ../strings-6.1-3.txt
                           and ../strings-compared.txt
"""
import os, re
from pdp10 import *
from modules import real_entries

RDIR = os.path.dirname(HERE)


def printable(c):
    return 32 <= c < 127 or c in (0, 9, 10, 13)


def runs(mem, lo, hi):
    """Runs of words made only of 7-bit text, at least one word of which has
    letters in it."""
    out, cur, at = [], [], None
    for a in range(lo, hi + 1):
        x = mem.get(a)
        cs = [(x >> (29 - 7 * k)) & 0o177 for k in range(5)] if x is not None else None
        if cs is not None and x & 1 == 0 and x and all(printable(c) for c in cs):
            if not cur:
                at = a
            cur.append("".join(chr(c) for c in cs if c))
        else:
            if cur:
                out.append((at, "".join(cur)))
            cur = []
    return out


WORDY = re.compile(r"[A-Za-z]{3,}")


FORMAT_LITERAL = re.compile(r"'[^']*[A-Za-z]{2,}[^']*'")


def texty(s):
    """Keep runs that read as text: long enough, mostly letters, spaces and
    punctuation, with real words in them.  A FORMAT statement is punctuation
    by nature, so it only has to be well formed and quote some words."""
    s2 = s.strip()
    if s2.startswith("(") and s2.count("(") >= 1 and FORMAT_LITERAL.search(s2) \
            and all(32 <= ord(c) < 127 for c in s2):
        return True
    if len(s2) < 12:
        return False
    good = sum(1 for c in s2 if c.isalnum() or c in " .,;:'\"()/-!?*$=<>#%&")
    if good < 0.92 * len(s2):
        return False
    words = WORDY.findall(s2)
    return len(words) >= 2 and sum(len(w) for w in words) >= 0.45 * len(s2)


WORDTOK = re.compile(r"^(?:(?:[A-Z]+|[A-Z][a-z']+|[a-z']+|[0-9]+(?:[-/][0-9]+)*|&?IT)[.,?!/]?|[;,&?.!/])$")
# All-capital words the logs abbreviate without a vowel.
NOVOWEL_OK = {"SNK", "WMP", "SWD", "PYR", "EMR", "CTL", "RM", "HNT", "LFX", "OFX", "PRP", "TRV"}


SHORT_WORDS = {"a", "an", "am", "as", "at", "be", "by", "do", "go", "he", "if", "in", "is",
               "it", "me", "my", "no", "of", "on", "or", "so", "to", "up", "us", "we"}


def good_chunk(c):
    toks = c.split()
    if not toks:
        return False
    digits = [t for t in toks if re.match(r"^[0-9]", t)]
    if digits and len(digits) != len(toks):
        return False                        # '4 ga Dav': a number among words
    for t in toks:
        if re.match(r"^[a-z']{1,2}[.,?!]?$", t) and t.rstrip(".,?!") not in SHORT_WORDS:
            return False
        if not WORDTOK.match(t):
            return False
        core = t.rstrip(".,?!/")
        if core.isupper() and len(core) >= 3 and not re.search(r"[AEIOUY]", core) \
                and core not in NOVOWEL_OK and not core.endswith("COM"):
            return False
    return True


def word_table(s):
    """FORTRAN keeps its words as A5 or A10 constants: space-padded fields
    laid end to end ('PUT  UP        BOAT      ').  Accept a run that cuts
    cleanly into such fields, nearly all of which hold words -- which is
    what separates them from machine code that happens to be printable
    (' F0w D0v', 'Dav7Dav')."""
    for width in (10, 5):
        chunks = [s[i:i + width] for i in range(0, len(s), width)]
        chunks = [c for c in chunks if c.strip()]
        if not chunks:
            continue
        good = [c for c in chunks if good_chunk(c)]
        if len(good) >= 0.8 * len(chunks) and sum(ch.isalpha() for ch in s) >= 3:
            return width
    return 0


def owner_table(entries, macro=()):
    """Which routine a string belongs to.  FORTRAN-10 lays a compiled module
    out as its formats and literals first, then the SIXBIT name and the code,
    so a string belongs to the next entry point after it.  The references
    bear that out in both builds: the wizard-only format at 045752 in 6.1/9
    is used by FOO at 045773, and 6.1/3's "The best player since" sits just
    in front of WINNER.  A hand-written MACRO module is the other way round,
    code then text, so a string inside one (given as (lo, hi, name)) belongs
    to it."""
    entries = sorted(entries)
    def owner(a):
        for lo, hi, n in macro:
            if lo <= a < hi:
                return n
        for e, n in entries:
            if e > a:
                return n
        return "(after last routine)"
    return owner


COMMON_NAME = re.compile(r"^[A-Z]{3}COM? *$")


def phrases(s):
    """The pieces of a string that are wording rather than layout: the quoted
    literals of a FORMAT, or the string itself.  The names of COMMON blocks
    that both programs plant at the head of each block are left out; the
    README compares the blocks themselves."""
    tagged = re.match(r"^\[A\d+\] ", s)
    s = re.sub(r"^\[A\d+\] ", "", s)
    if COMMON_NAME.match(s.strip()):
        return []
    if tagged:
        # 6.1/3 keeps the slash of 'CONCH/' inside the constant, 6.1/9 in
        # the FORMAT; the slash is layout either way.
        s = re.sub(r"/(?=\s|$)", " ", s)
    s = s.replace("''", "\u2019")
    q = re.findall(r"'([^']*)'", s)
    parts = q if (s.lstrip().startswith("(") and q) else [s]
    out = []
    for p in parts:
        p = re.sub(r"\s+", " ", p.replace("\u2019", "'")).strip()
        if len(p) >= 4 and re.search(r"[A-Za-z]{3,}", p):
            out.append(p)
    return out


def extract(build_name):
    mem, start, _ = build(build_name)
    macro = ()
    if build_name == "6.1/3":
        syms = symbols(mem)
        glob = {n: v & 0o777777 for k, n, v in syms if k == 1}
        mods = sorted((v & 0o777777, n) for k, n, v in syms
                      if k == 0 and n != "JOBDAT" and 0o140 < (v & 0o777777) < glob["RESET."])
        entries = [(a + 1, n) for a, n in mods] + [(start, "MAIN.")]
        i = [n for _, n in mods].index("ADVMAC")
        macro = ((mods[i][0], mods[i + 1][0], "ADVMAC"),)
        hi = glob["RESET."] - 1
        lo = 0o140
    else:
        known = set()
        m3, _, _ = build("6.1/3")
        known = set(n for k, n, v in symbols(m3) if k in (0, 1))
        lib = mem[start + 1] & 0o777777
        entries = real_entries(mem, 0o140, lib, known) + [(start, "MAIN.")]
        lo, hi = 0o140, lib - 1
    owner = owner_table(entries, macro)
    found = []
    for a, s in runs(mem, lo, hi):
        if texty(s):
            found.append((a, owner(a), s))
        elif word_table(s) and len(s.strip()) >= 5:
            found.append((a, owner(a), "[A%d] %s" % (word_table(s), s)))
    return found


def main():
    both = {}
    for b, fn in (("6.1/9", "strings-6.1-9.txt"), ("6.1/3", "strings-6.1-3.txt")):
        found = extract(b)
        both[b] = found
        with open(os.path.join(RDIR, fn), "w", newline="\n") as fp:
            fp.write("Text compiled into ADVENTURE %s, by address and routine.\n" % b)
            fp.write("Generated by tools/strings.py.\n\n")
            for a, o, s in found:
                fp.write("%06o  %-7s %s\n" % (a, o, s.replace("\r", "\\r").replace("\n", "\\n")))
        print(b, len(found), "strings")

    # The two compilations cut the same sentence into literals differently
    # (' points, out of a',/,' possible total of ' against ' points,',
    # ' out of a possible total of '), so compare wording rather than
    # literals: a word counts as shared when it sits in a run of N words that
    # the other build also has anywhere.
    N = 3
    TOK = re.compile(r"[A-Za-z0-9']+|[^\sA-Za-z0-9']")

    def streams(found):
        out = []
        for a, o, s in found:
            toks = TOK.findall(" ".join(phrases(s)))
            if toks:
                out.append((a, o, toks))
        return out

    def grams(st):
        """Every run of 1 to N consecutive words in a build, lower-cased."""
        g = set()
        for _, _, t in st:
            lt = [w.lower() for w in t]
            for n in range(1, N + 1):
                for i in range(len(lt) - n + 1):
                    g.add(tuple(lt[i:i + n]))
        return g

    def unshared(st, other):
        res = []
        for a, o, t in st:
            lt = [w.lower() for w in t]
            covered = [False] * len(t)
            if len(t) < N:
                if tuple(lt) in other:
                    covered = [True] * len(t)
            else:
                for i in range(len(t) - N + 1):
                    if tuple(lt[i:i + N]) in other:
                        for k in range(i, i + N):
                            covered[k] = True
            run, start = [], 0
            for k, w in enumerate(t + [None]):
                if w is not None and not covered[k]:
                    if not run:
                        start = k
                    run.append(w)
                else:
                    # A stretch shorter than N between shared words may still
                    # occur whole in the other build ('do with the').
                    short_shared = len(run) < N and tuple(x.lower() for x in run) in other
                    if run and not short_shared and sum(len(x) for x in run if x[0].isalnum()) >= 4:
                        text = " ".join(run)
                        text = re.sub(r" ([.,;:!?')])", r"\1", text)
                        res.append((a, o, text))
                    run = []
        return res

    s9, s3 = streams(both["6.1/9"]), streams(both["6.1/3"])
    g9, g3 = grams(s9), grams(s3)
    only9, only3 = unshared(s9, g3), unshared(s3, g9)
    with open(os.path.join(RDIR, "strings-compared.txt"), "w", newline="\n") as fp:
        fp.write("Wording compiled into the two builds, compared.\n\n")
        fp.write("Only the program's own text is compared -- FORMAT literals and character\n")
        fp.write("constants.  A stretch is listed when none of its words sits in a run of\n")
        fp.write("%d consecutive words that the other build also contains, so the same\n" % N)
        fp.write("sentence split into literals differently still counts as shared.\n\n")
        fp.write("Constants shorter than five characters are not extracted: next to\n")
        fp.write("machine code, a three-letter word cannot be told from noise.  So a word\n")
        fp.write("table can be listed here for one build only because a short entry beside\n")
        fp.write("it (THE, SAFE, IT) was not picked up in the other.  The event names of\n")
        fp.write("ENVIRN are the same thirty, in the same order, in both builds.\n")
        fp.write("Generated by tools/strings.py.\n\n")
        fp.write("== Only in 6.1/9 ==\n")
        for a, o, t in only9:
            fp.write("  %06o %-7s %s\n" % (a, o, t))
        fp.write("\n== Only in 6.1/3 ==\n")
        for a, o, t in only3:
            fp.write("  %06o %-7s %s\n" % (a, o, t))
    print("unshared stretches: 6.1/9 %d, 6.1/3 %d" % (len(only9), len(only3)))


if __name__ == "__main__":
    main()
