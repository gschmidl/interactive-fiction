#!/usr/bin/env python3
"""Answer ADVENT's MAGIC MODE challenge.

The game proves you are a wizard by printing five letters and demanding five
back.  The reply is a function of the challenge, the clock and the game's
"magic number" (MAGNM, 11111 as shipped) -- see WIZARD in ADVENT.FOR:

    for each position y, with z the next position round the ring of five,
        reply[y] = chr(((|ch[y]-ch[z]| * (d mod 10)) + (t mod 10)) mod 26 + 1)
    and after each letter t and d are divided by ten,

where t is the time as hour*100 + (minute rounded down to ten).  Because only
whole tens of minutes count, an answer stays good for up to ten minutes -- and
no longer, which is what makes doing this by hand "tricky" as the distribution's
README warns.

This is a Python 3 rewrite of advent-magic-mode.py by Grawity, which shipped
with "TOPS-10 in a Box" v1.1 and is itself a port of the JavaScript at
zonadepruebas.com.  That script is Python 2 -- under Python 3 its `map` returns
an iterator and it dies with "'map' object is not subscriptable".  The original
is kept unaltered in src_original/ as part of the recovered distribution; this
is the copy you actually run.

Usage
    magicmode.py [-m MAGIC] [-t HH:MM] CHALLENGE

With the port's clock frozen (advent350 -d DD-MMM-YYYY -t HHMM, or
ADVENT_DATE/ADVENT_TIME), pass the same time here and the answer is exact:

    $ advent350 -d 06-JUN-2007 -t 1000        # prints challenge COBPX
    $ magicmode.py -t 10:00 COBPX
    MNOJV
"""
import argparse
import re
import sys
import time


def solve(challenge, hhmm=None, magic=11111):
    """Return the five-letter reply to a five-letter challenge."""
    if not re.fullmatch(r"[A-Za-z]{5}", challenge):
        raise ValueError("challenge must be exactly five letters, got %r" % challenge)
    ch = [ord(c) for c in challenge.upper()]

    if hhmm is None:
        now = time.localtime()
        hour, minute = now.tm_hour, now.tm_min
    else:
        m = re.fullmatch(r"(\d{1,2}):?(\d{2})", hhmm)
        if not m:
            raise ValueError("time must be HH:MM, got %r" % hhmm)
        hour, minute = int(m.group(1)), int(m.group(2))
        if not (0 <= hour < 24 and 0 <= minute < 60):
            raise ValueError("time out of range: %r" % hhmm)

    # Only whole tens of minutes enter the calculation.
    t = hour * 100 + minute // 10 * 10
    d = int(magic)

    reply = ""
    for y in range(5):
        z = (y + 1) % 5
        x = ((abs(ch[y] - ch[z]) * (d % 10)) + (t % 10)) % 26 + 1
        reply += chr(x + 64)
        t //= 10
        d //= 10
    return reply


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Answer ADVENT's MAGIC MODE challenge.",
        epilog="The answer is only valid for the ten-minute window it was computed for.")
    p.add_argument("challenge", help="the five letters the game printed")
    p.add_argument("-m", "--magic", default=11111, type=int,
                   help="the game's magic number (default 11111, as shipped)")
    p.add_argument("-t", "--time", default=None, metavar="HH:MM",
                   help="the game's clock (default: now)")
    a = p.parse_args(argv)
    try:
        print(solve(a.challenge, a.time, a.magic))
    except ValueError as e:
        sys.exit("%s: %s" % (p.prog, e))


if __name__ == "__main__":
    main()
