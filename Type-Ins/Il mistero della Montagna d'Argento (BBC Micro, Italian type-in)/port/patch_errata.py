# -*- coding: utf-8 -*-
"""
Derive the runnable BBC Micro version (montagna.bas) from the literal
transcription of the printed listing (montagna_libro.bas).

Every change is listed here explicitly so it can be audited against the book.
Nothing else is altered: all text, all numbers, all game logic are as printed.
"""
import re, sys, io, os

HERE = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(HERE, "montagna_libro.bas")
dst = os.path.join(HERE, "montagna.bas")

lines = open(src, encoding="utf-8").read().split("\n")
prog = {}
order = []
for l in lines:
    if not l.strip():
        continue
    m = re.match(r"(\d+) ?(.*)$", l)
    prog[int(m.group(1))] = m.group(2)
    order.append(int(m.group(1)))

# ---------------------------------------------------------------- A: book typos
# Each entry: line -> (must-contain-before, replacement-body, why)
TYPOS = {
  910:  ('NON SAI NUOTARE',
         'IFC(8)<>0AND((R=52 AND D=4) OR (R=31 AND D=3)) THEN LET R$="NON SAI NUOTARE":RETURN',
         'printed with no closing quote and no :RETURN, so it fell through to 920'),
  2060: ('LET R$:',
         'IF B=14 OR B=2 THEN LET R$="NON E\' ATTACCATO A NULLA!"',
         'printed LET R$: instead of LET R$='),
  2090: ('LET F(40',
         'IF H=722 AND F(40)=1 THEN LET R=71:LET R$="E\' ROTTO":LET C(2)=81:LET F(40)=0',
         'line ends mid-statement at "LET F(40" (well short of the printed margin)'),
  2100: (' T LET',
         'IF H=7114 AND F(53)=1 THEN LET C(14)=71:LET F(53)=0:LET R$="CADE: PATAPUNF!"',
         'printed a bare T where THEN belongs'),
  2250: ('LET R$:',
         'IF H=2444 OR H=1870 THEN LET R$="NON SEI ABBASTANZA FORTE"',
         'printed LET R$: instead of LET R$='),
  2260: ('"EW"',
         'IF H=3756 THEN LET R$="UN PASSAGGIO!":LET E$(37)="EO"',
         'English E/W left untranslated; 1070 tests E$(37)="EO" and the mover only knows N/E/S/O/A/B'),
  2990: ('R(49)',
         'IF (B=67 OR B=68) AND C(9)=0 AND R=49 THEN LET R$="OK":LET F(47)=1',
         'R is a scalar - DIM at 3380 never declares it as an array'),
  3300: ('S4$',
         'LET R$=X4$+RIGHT$ (D$,LEN (D$)-2):RETURN',
         'S4$ is never assigned anywhere; X4$ is set at 3490 and is the string this needs'),
  4520: ('*4)*3,',
         "LET F$=MID$ (B$,1+INT (RND (1)*4)*4,1)",
         'stride must be 4: B$ holds 4-character verb slots, so the four directions '
         'N/E/S/O sit at characters 1,5,9,13.  With *3 the index lands on 1,4,7,10 = '
         'N,?,?,? so L$ is never set and 4580 dies with "No such variable"'),
  4730: ('INPUT# 1',
         'INPUT#X,G$(1):INPUT#X,G$(2)',
         'channel is X (opened at 4690), not 1'),
  4810: ('PRINT# 1',
         'PRINT#X,G$(1):PRINT#X,G$(2)',
         'channel is X (opened at 4770), not 1'),
}

# ------------------------------------------- B: BBC tokeniser / typesetting fixes
#  560: "ON VBGOSUB" - BBC BASIC skips a whole alphanumeric run when no keyword
#       matches at its start, so GOSUB inside "VBGOSUB" is never tokenised.
#       (580/600/620/640 print the same way but work, because a digit precedes
#       GOSUB there and that restarts a word.)
TOKEN_FIX = {
  560: 'ON VB GOSUB 800,800,800,800,800,800,1220,1290,1290,1470,1470,1750,1890',
}

#  Four functions must have "(" attached on a BBC:
#    MID$( LEFT$( RIGHT$(  - each is a single token that INCLUDES the bracket,
#        so a space stops them tokenising and the program dies with
#        "No such variable".  (The book sets TAB( with no space - same rule.)
#    RND(  - RND also has a valid no-argument form, so "RND (1)" parses as bare
#        RND followed by a separate "(1)" and gives "Missing )".
#  LEN (, INT (, STR$ (, ASC (, CHR$ (, VAL ( all take a mandatory argument, so
#  the compositor's space is harmless there; they are left exactly as printed.
BRACKET_FUNCS = ("MID$", "LEFT$", "RIGHT$", "RND")

# --------------------------------------------- C: playability fix (deliberate)
#  Line 4110 gives objects 72 and 74 both as "GUIDARE", so the noun scanner at
#  400-420 (first match wins) can never return 74.  The third magic word is
#  object F(52)+73 and 4490 makes F(52) a random 0/1/2, so when F(52)=1 line
#  1940 can never fire and the game cannot be won.
#
#  Taking the printed vocabulary at face value - that game's third word simply
#  IS "GUIDARE" - 1940 also accepts B=72 in that case.  No vocabulary invented,
#  all three variants stay in play.  1930 still claims B=72 first while
#  F(61)=0, so word two is unaffected.
PLAYABILITY = {
  1940: ('B=(F(52)+73)',
         'IF (B=F(52)+73 OR (F(52)=1 AND B=72)) AND F(60)=1 AND F(61)=1 THEN LET F(62)=1:RETURN',
         'object 74 duplicates 72, so B=74 is unreachable; without this 1 game in 3 is unwinnable'),
}

applied, missing = [], []
for ln, (needle, body, why) in TYPOS.items():
    if ln not in prog:
        missing.append(ln); continue
    if needle not in prog[ln]:
        missing.append((ln, needle)); continue
    prog[ln] = body
    applied.append(("typo", ln, why))

for ln, (needle, body, why) in PLAYABILITY.items():
    if ln not in prog or needle not in prog[ln]:
        missing.append((ln, needle)); continue
    prog[ln] = body
    applied.append(("playability", ln, why))

for ln, body in TOKEN_FIX.items():
    prog[ln] = body
    applied.append(("tokeniser", ln, "ON VBGOSUB does not tokenise; needs a space"))

nbracket = 0
for ln in list(prog):
    before = prog[ln]
    after = before
    for fn in BRACKET_FUNCS:
        after = after.replace(fn + " (", fn + "(")
    if after != before:
        prog[ln] = after
        nbracket += 1
if nbracket:
    applied.append(("tokeniser", "%d lines" % nbracket,
                    "MID$ ( / LEFT$ ( / RIGHT$ ( / RND ( -> bracket attached"))

if missing:
    print("!! could not apply:", missing); sys.exit(1)

with io.open(dst, "w", encoding="utf-8", newline="\n") as f:
    for n in order:
        f.write("%d %s\n" % (n, prog[n]))

print("wrote", dst)
for kind, where, why in applied:
    print("  [%s] %s: %s" % (kind, where, why))
